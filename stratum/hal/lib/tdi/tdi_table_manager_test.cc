// Copyright 2020-present Open Networking Foundation
// Copyright 2022-2004 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// adapted from bfrt_table_manager_test.cc

#include "stratum/hal/lib/tdi/tdi_table_manager.h"

#include <string>
#include <utility>

#include "absl/memory/memory.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "stratum/glue/status/status_test_util.h"
#include "stratum/hal/lib/common/writer_mock.h"
#include "stratum/hal/lib/tdi/tdi_sde_mock.h"
#include "stratum/lib/test_utils/matchers.h"
#include "stratum/lib/utils.h"

// FIXME
DEFINE_string(tdi_sde_config_dir, "/var/run/stratum/tdi_config",
              "The dir used by the SDE to load the device configuration.");

namespace stratum {
namespace hal {
namespace tdi {

using test_utils::EqualsProto;
using ::testing::_;
using ::testing::ByMove;
using ::testing::DoAll;
using ::testing::HasSubstr;
using ::testing::Invoke;
using ::testing::InvokeWithoutArgs;
using ::testing::NiceMock;
using ::testing::Optional;
using ::testing::Return;
using ::testing::SetArgPointee;

class TdiTableManagerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    tdi_sde_wrapper_mock_ = absl::make_unique<NiceMock<TdiSdeMock>>();
    tdi_table_manager_ = TdiTableManager::CreateInstance(
        OPERATION_MODE_STANDALONE, tdi_sde_wrapper_mock_.get(), kDevice1);
  }

  ::util::Status PushTestConfig() {
    const std::string kSamplePipelineText = R"pb(
      programs {
        name: "test pipeline config",
        p4info {
          pkg_info {
            arch: "tna"
          }
          tables {
            preamble {
              id: 33583783
              name: "Ingress.control.table1"
            }
            match_fields {
              id: 1
              name: "field1"
              bitwidth: 9
              match_type: EXACT
            }
            match_fields {
              id: 2
              name: "field2"
              bitwidth: 12
              match_type: TERNARY
            }
            action_refs {
              id: 16794911
            }
            const_default_action_id: 16836487
            direct_resource_ids: 318814845
            direct_resource_ids: 361104180
            size: 1024
          }
          actions {
            preamble {
              id: 16794911
              name: "Ingress.control.action1"
            }
            params {
              id: 1
              name: "vlan_id"
              bitwidth: 12
            }
          }
          direct_counters {
            preamble {
              id: 318814845
              name: "Ingress.control.counter1"
            }
            spec {
              unit: BOTH
            }
            direct_table_id: 33583783
          }
          direct_meters {
            preamble {
              id: 361104180
              name: "Ingress.control.meter1"
            }
            spec {
              unit: BYTES
            }
            direct_table_id: 33583783
          }
          meters {
            preamble {
              id: 55555
              name: "Ingress.control.meter_bytes"
              alias: "meter_bytes"
            }
            spec {
              unit: BYTES
            }
            size: 500
          }
          meters {
            preamble {
              id: 55556
              name: "Ingress.control.meter_packets"
              alias: "meter_packets"
            }
            spec {
              unit: PACKETS
            }
            size: 500
          }
        }
      }
    )pb";
    TdiDeviceConfig config;
    RETURN_IF_ERROR(ParseProtoFromString(kSamplePipelineText, &config));
    return tdi_table_manager_->PushForwardingPipelineConfig(config);
  }

  static constexpr int kDevice1 = 0;

  std::unique_ptr<TdiSdeMock> tdi_sde_wrapper_mock_;
  std::unique_ptr<TdiTableManager> tdi_table_manager_;
};

constexpr int TdiTableManagerTest::kDevice1;

TEST_F(TdiTableManagerTest, WriteDirectCounterEntryTest) {
  ASSERT_OK(PushTestConfig());
  constexpr int kP4TableId = 33583783;
  constexpr int kTdiRtTableId = 20;
  constexpr int kTdiPriority = 16777205;  // Inverted
  auto table_key_mock = absl::make_unique<NiceMock<TableKeyMock>>();
  auto table_data_mock = absl::make_unique<TableDataMock>();
  auto session_mock = std::make_shared<SessionMock>();

  EXPECT_CALL(*table_key_mock, SetPriority(kTdiPriority))
      .WillOnce(Return(::util::OkStatus()));
  EXPECT_CALL(*table_data_mock, SetCounterData(200, 100))
      .WillOnce(Return(::util::OkStatus()));
  EXPECT_CALL(*tdi_sde_wrapper_mock_, GetTdiRtId(kP4TableId))
      .WillOnce(Return(kTdiRtTableId));
  // TODO(max): figure out how to expect the session mock here.
  EXPECT_CALL(*tdi_sde_wrapper_mock_,
              ModifyTableEntry(kDevice1, _, kTdiRtTableId, table_key_mock.get(),
                               table_data_mock.get()))
      .WillOnce(Return(::util::OkStatus()));
  EXPECT_CALL(*tdi_sde_wrapper_mock_, CreateTableKey(kTdiRtTableId))
      .WillOnce(Return(ByMove(
          ::util::StatusOr<std::unique_ptr<TdiSdeInterface::TableKeyInterface>>(
              std::move(table_key_mock)))));
  EXPECT_CALL(*tdi_sde_wrapper_mock_, CreateTableData(kTdiRtTableId, _))
      .WillOnce(
          Return(ByMove(::util::StatusOr<
                        std::unique_ptr<TdiSdeInterface::TableDataInterface>>(
              std::move(table_data_mock)))));

  const std::string kDirectCounterEntryText = R"pb(
    table_entry {
      table_id: 33583783
      match {
        field_id: 1
        exact { value: "\000\001" }
      }
      match {
        field_id: 2
        ternary { value: "\x000" mask: "\xfff" }
      }
      priority: 10
    }
    data {
      byte_count: 200
      packet_count: 100
    }
  )pb";

  ::p4::v1::DirectCounterEntry entry;
  ASSERT_OK(ParseProtoFromString(kDirectCounterEntryText, &entry));

  EXPECT_OK(tdi_table_manager_->WriteDirectCounterEntry(
      session_mock, ::p4::v1::Update::MODIFY, entry));
}

TEST_F(TdiTableManagerTest, WriteDirectMeterEntryTest) {
  ASSERT_OK(PushTestConfig());
  constexpr int kP4TableId = 33583783;
  constexpr int kTdiRtTableId = 20;
  constexpr int kTdiPriority = 16777205;  // Inverted
  auto table_key_mock = absl::make_unique<NiceMock<TableKeyMock>>();
  auto table_data_mock = absl::make_unique<TableDataMock>();
  auto session_mock = std::make_shared<SessionMock>();

  EXPECT_CALL(*table_key_mock, SetPriority(kTdiPriority))
      .WillOnce(Return(::util::OkStatus()));
  EXPECT_CALL(*table_data_mock, SetMeterConfig(false, 200, 2000, 100, 1000))
      .WillOnce(Return(::util::OkStatus()));
  EXPECT_CALL(*tdi_sde_wrapper_mock_, GetTdiRtId(kP4TableId))
      .WillOnce(Return(kTdiRtTableId));
  // TODO(max): figure out how to expect the session mock here.
  EXPECT_CALL(*tdi_sde_wrapper_mock_,
              ModifyTableEntry(kDevice1, _, kTdiRtTableId, table_key_mock.get(),
                               table_data_mock.get()))
      .WillOnce(Return(::util::OkStatus()));
  EXPECT_CALL(*tdi_sde_wrapper_mock_, CreateTableKey(kTdiRtTableId))
      .WillOnce(Return(ByMove(
          ::util::StatusOr<std::unique_ptr<TdiSdeInterface::TableKeyInterface>>(
              std::move(table_key_mock)))));
  EXPECT_CALL(*tdi_sde_wrapper_mock_, CreateTableData(kTdiRtTableId, _))
      .WillOnce(
          Return(ByMove(::util::StatusOr<
                        std::unique_ptr<TdiSdeInterface::TableDataInterface>>(
              std::move(table_data_mock)))));

  const std::string kDirectMeterEntryText = R"pb(
    table_entry {
      table_id: 33583783
      match {
        field_id: 1
        exact { value: "\000\001" }
      }
      match {
        field_id: 2
        ternary { value: "\x000" mask: "\xfff" }
      }
      priority: 10
    }
    config {
      cir: 200
      cburst: 2000
      pir: 100
      pburst: 1000
    }
  )pb";

  ::p4::v1::DirectMeterEntry entry;
  ASSERT_OK(ParseProtoFromString(kDirectMeterEntryText, &entry));

  EXPECT_OK(tdi_table_manager_->WriteDirectMeterEntry(
      session_mock, ::p4::v1::Update::MODIFY, entry));
}

// clang-format off
//[ RUN      ] TdiTableManagerTest.WriteIndirectMeterEntryTest
//E20241008 22:01:52.255509    12 p4_info_manager.cc:297] StratumErrorSpace::ERR_INVALID_P4_INFO: P4Info ID UNSPECIFIED/0x2b67 (0x2b67) is not found
//E20241008 22:01:52.255726    12 tdi_table_manager.cc:1257] Return Error: p4_info_manager_->FindResourceTypeByID(meter_id) at stratum/hal/lib/tdi/tdi_table_manager.cc:1257
//stratum/hal/lib/tdi/tdi_table_manager_test.cc:294: Failure
//Value of: tdi_table_manager_->WriteMeterEntry( session_mock, ::p4::v1::Update::MODIFY, entry)
//Expected: is OK
//  Actual: StratumErrorSpace::ERR_INVALID_P4_INFO: P4Info ID UNSPECIFIED/0x2b67 (0x2b67) is not found (of type util::Status)
//stratum/hal/lib/tdi/tdi_table_manager_test.cc:274: Failure
//Actual function call count doesn't match EXPECT_CALL(*tdi_sde_wrapper_mock_, WriteIndirectMeter(kDevice1, _, kTdiRtTableId, Optional(kMeterIndex), false, 1, 100, 2, 200))...
//         Expected: to be called once
//           Actual: never called - unsatisfied and active
//[  FAILED  ] TdiTableManagerTest.WriteIndirectMeterEntryTest (1 ms)
// clang-format on
TEST_F(TdiTableManagerTest, DISABLED_WriteIndirectMeterEntryTest) {
  ASSERT_OK(PushTestConfig());
  constexpr int kP4MeterId = 55555;
  constexpr int kTdiRtTableId = 11111;
  constexpr int kMeterIndex = 12345;
  auto session_mock = std::make_shared<SessionMock>();

  EXPECT_CALL(*tdi_sde_wrapper_mock_, GetTdiRtId(kP4MeterId))
      .WillOnce(Return(kTdiRtTableId));
  // TODO(max): figure out how to expect the session mock here.
  EXPECT_CALL(*tdi_sde_wrapper_mock_,
              WriteIndirectMeter(kDevice1, _, kTdiRtTableId,
                                 Optional(kMeterIndex), false, 1, 100, 2, 200))
      .WillOnce(Return(::util::OkStatus()));

  const std::string kMeterEntryText = R"pb(
    meter_id: 55555
    index {
      index: 12345
    }
    config {
      cir: 1
      cburst: 100
      pir: 2
      pburst: 200
    }
  )pb";
  ::p4::v1::MeterEntry entry;
  ASSERT_OK(ParseProtoFromString(kMeterEntryText, &entry));

  EXPECT_OK(tdi_table_manager_->WriteMeterEntry(
      session_mock, ::p4::v1::Update::MODIFY, entry));
}

TEST_F(TdiTableManagerTest, RejectMeterEntryModifyWithoutMeterId) {
  ASSERT_OK(PushTestConfig());
  auto session_mock = std::make_shared<SessionMock>();

  const std::string kMeterEntryText = R"pb(
    meter_id: 0
    index {
      index: 12345
    }
    config {
      cir: 1
      cburst: 100
      pir: 2
      pburst: 200
    }
  )pb";
  ::p4::v1::MeterEntry entry;
  ASSERT_OK(ParseProtoFromString(kMeterEntryText, &entry));

  ::util::Status ret = tdi_table_manager_->WriteMeterEntry(
      session_mock, ::p4::v1::Update::MODIFY, entry);
  ASSERT_FALSE(ret.ok());
  EXPECT_EQ(ERR_INVALID_PARAM, ret.error_code());
  EXPECT_THAT(ret.error_message(), HasSubstr("Missing meter id"));
}

// clang-format off
//[ RUN      ] TdiTableManagerTest.RejectMeterEntryInsertDelete
//E20241008 22:00:27.954881    12 tdi_table_manager.cc:1246] StratumErrorSpace::ERR_INVALID_PARAM: RET_CHECK failure (stratum/hal/lib/tdi/tdi_table_manager.cc:1246) 'type == ::p4::v1::Update::MODIFY || type == ::p4::v1::Update::DELETE' is false. Update type of RegisterEntry meter_id: 55555 index { index: 12345 } config { cir: 1 cburst: 100 pir: 2 pburst: 200 } must be MODIFY or DELETE.
//E20241008 22:00:27.954906    12 tdi_table_manager.cc:1254] Return Error: tdi_sde_interface_->GetTdiRtId(meter_entry.meter_id()) at stratum/hal/lib/tdi/tdi_table_manager.cc:1254
//stratum/hal/lib/tdi/tdi_table_manager_test.cc:353: Failure
//Expected equality of these values:
//  ERR_INVALID_PARAM
//    Which is: 503
//  ret.error_code()
//    Which is: 2
//[  FAILED  ] TdiTableManagerTest.RejectMeterEntryInsertDelete (0 ms)
// clang-format on
TEST_F(TdiTableManagerTest, DISABLED_RejectMeterEntryInsertDelete) {
  ASSERT_OK(PushTestConfig());
  auto session_mock = std::make_shared<SessionMock>();

  const std::string kMeterEntryText = R"pb(
    meter_id: 55555
    index {
      index: 12345
    }
    config {
      cir: 1
      cburst: 100
      pir: 2
      pburst: 200
    }
  )pb";
  ::p4::v1::MeterEntry entry;
  ASSERT_OK(ParseProtoFromString(kMeterEntryText, &entry));

  ::util::Status ret = tdi_table_manager_->WriteMeterEntry(
      session_mock, ::p4::v1::Update::INSERT, entry);
  ASSERT_FALSE(ret.ok());
  EXPECT_EQ(ERR_INVALID_PARAM, ret.error_code());

  ret = tdi_table_manager_->WriteMeterEntry(session_mock,
                                            ::p4::v1::Update::DELETE, entry);
  ASSERT_FALSE(ret.ok());
  // TODO(derek): fails because WriteMeterEntry returns UNKNOWN (2)
  // instead of ERR_INVALID_PARAM (503).
  EXPECT_EQ(ERR_INVALID_PARAM, ret.error_code());
}

// clang-format off
//[ RUN      ] TdiTableManagerTest.ReadSingleIndirectMeterEntryTest
//E20241008 21:45:59.952771    12 p4_info_manager.cc:297] StratumErrorSpace::ERR_INVALID_P4_INFO: P4Info ID UNSPECIFIED/0x2b67 (0x2b67) is not found
//E20241008 21:45:59.952782    12 tdi_table_manager.cc:1081] Return Error: p4_info_manager_->FindResourceTypeByID(table_id) at stratum/hal/lib/tdi/tdi_table_manager.cc:1081
//stratum/hal/lib/tdi/tdi_table_manager_test.cc:411: Failure
//Value of: tdi_table_manager_->ReadMeterEntry(session_mock, entry, &writer_mock)
//Expected: is OK
//  Actual: StratumErrorSpace::ERR_INVALID_P4_INFO: P4Info ID UNSPECIFIED/0x2b67 (0x2b67) is not found (of type util::Status)
//stratum/hal/lib/tdi/tdi_table_manager_test.cc:399: Failure
//Actual function call count doesn't match EXPECT_CALL(writer_mock, Write(EqualsProto(resp)))...
//         Expected: to be called once
//           Actual: never called - unsatisfied and active
//stratum/hal/lib/tdi/tdi_table_manager_test.cc:374: Failure
//Actual function call count doesn't match EXPECT_CALL(*tdi_sde_wrapper_mock_, ReadIndirectMeters(kDevice1, _, kTdiRtTableId, Optional(kMeterIndex), _, _, _, _, _, _))...
//         Expected: to be called once
//           Actual: never called - unsatisfied and active
//[  FAILED  ] TdiTableManagerTest.ReadSingleIndirectMeterEntryTest (0 ms)
// clang-format on
TEST_F(TdiTableManagerTest, DISABLED_ReadSingleIndirectMeterEntryTest) {
  ASSERT_OK(PushTestConfig());
  auto session_mock = std::make_shared<SessionMock>();
  constexpr int kP4MeterId = 55555;
  constexpr int kTdiRtTableId = 11111;
  constexpr int kMeterIndex = 12345;
  WriterMock<::p4::v1::ReadResponse> writer_mock;

  {
    EXPECT_CALL(*tdi_sde_wrapper_mock_, GetTdiRtId(kP4MeterId))
        .WillOnce(Return(kTdiRtTableId));

    std::vector<uint32> meter_indices = {kMeterIndex};
    std::vector<uint64> cirs = {1};
    std::vector<uint64> cbursts = {100};
    std::vector<uint64> pirs = {2};
    std::vector<uint64> pbursts = {200};
    std::vector<bool> in_pps = {true};
    EXPECT_CALL(*tdi_sde_wrapper_mock_,
                ReadIndirectMeters(kDevice1, _, kTdiRtTableId,
                                   Optional(kMeterIndex), _, _, _, _, _, _))
        .WillOnce(DoAll(SetArgPointee<4>(meter_indices), SetArgPointee<5>(cirs),
                        SetArgPointee<6>(cbursts), SetArgPointee<7>(pirs),
                        SetArgPointee<8>(pbursts), SetArgPointee<9>(in_pps),
                        Return(::util::OkStatus())));
    const std::string kMeterResponseText = R"pb(
      entities {
        meter_entry {
          meter_id: 55555
          index {
            index: 12345
          }
          config {
            cir: 1
            cburst: 100
            pir: 2
            pburst: 200
          }
        }
      }
    )pb";
    ::p4::v1::ReadResponse resp;
    ASSERT_OK(ParseProtoFromString(kMeterResponseText, &resp));
    EXPECT_CALL(writer_mock, Write(EqualsProto(resp))).WillOnce(Return(true));
  }

  const std::string kMeterEntryText = R"pb(
    meter_id: 55555
    index {
      index: 12345
    }
  )pb";
  ::p4::v1::MeterEntry entry;
  ASSERT_OK(ParseProtoFromString(kMeterEntryText, &entry));

  EXPECT_OK(
      tdi_table_manager_->ReadMeterEntry(session_mock, entry, &writer_mock));
}

TEST_F(TdiTableManagerTest, RejectMeterEntryReadWithoutId) {
  ASSERT_OK(PushTestConfig());
  auto session_mock = std::make_shared<SessionMock>();
  WriterMock<::p4::v1::ReadResponse> writer_mock;

  const std::string kMeterEntryText = R"pb(
    meter_id: 0
    index {
      index: 12345
    }
    config {
      cir: 1
      cburst: 100
      pir: 2
      pburst: 200
    }
  )pb";
  ::p4::v1::MeterEntry entry;
  ASSERT_OK(ParseProtoFromString(kMeterEntryText, &entry));

  ::util::Status ret =
      tdi_table_manager_->ReadMeterEntry(session_mock, entry, &writer_mock);
  ASSERT_FALSE(ret.ok());
  EXPECT_EQ(ERR_INVALID_PARAM, ret.error_code());
}

}  // namespace tdi
}  // namespace hal
}  // namespace stratum
