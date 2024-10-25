// Copyright 2023-2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// Unit test for Es2kExternManager.

#include "stratum/hal/lib/tdi/es2k/es2k_extern_manager.h"

#include <memory>
#include <string>

#include "absl/memory/memory.h"
#include "absl/strings/substitute.h"
#include "gtest/gtest.h"
#include "p4/config/v1/p4info.pb.h"
#include "stratum/glue/status/status.h"
#include "stratum/hal/lib/p4/p4_info_manager.h"
#include "stratum/hal/lib/tdi/es2k/es2k_target_factory.h"
#include "stratum/lib/utils.h"

namespace stratum {
namespace hal {
namespace tdi {

constexpr uint32 PACKET_MOD_METER_ID =
    ::p4::config::v1::P4Ids::PACKET_MOD_METER;
constexpr uint32 DIRECT_PACKET_MOD_METER_ID =
    ::p4::config::v1::P4Ids::DIRECT_PACKET_MOD_METER;

// The bits 31:24 of the resource ID contain the P4 type ID.
constexpr uint32 kPktModMeterBase = PACKET_MOD_METER_ID << 24;
constexpr uint32 kDirectPktModMeterBase = DIRECT_PACKET_MOD_METER_ID << 24;

class Es2kExternManagerTest : public testing::Test {
 protected:
  Es2kExternManagerTest() : es2k_extern_manager_(new Es2kExternManager) {}

  void SetUpDirectPacketModMeters();
  void SetUpDirectPacketModMeterTest();
  void SetUpPacketModMeters();
  void SetUpPacketModMeterTest();

  //----------------------------------------------------

  void SetUpPreambleCallback() {
    preamble_cb_ = std::bind(&Es2kExternManagerTest::ProcessPreamble, this,
                             std::placeholders::_1, std::placeholders::_2);
  }

  // Dummy preamble processing callback function.
  ::util::Status ProcessPreamble(const ::p4::config::v1::Preamble& preamble,
                                 const std::string& resource_type) {
    return ::util::OkStatus();
  }

  //----------------------------------------------------

  // The following code was lifted from p4_info_manager_test.

  static const int kNumTestTables = 2;
  static const int kNumActionsPerTable = 3;
  static const int kFirstActionID = 10000;

  void SetUpTestTables(bool need_actions = true) {
    int32 dummy_action_id = kFirstActionID;

    // Each table entry is assigned an ID and name in the preamble.  Each table
    // optionally gets a set of action IDs.
    for (int t = 1; t <= kNumTestTables; ++t) {
      ::p4::config::v1::Table* new_table = p4info_.add_tables();
      new_table->mutable_preamble()->set_id(t);
      new_table->mutable_preamble()->set_name(absl::Substitute("Table-$0", t));
      for (int a = 0; need_actions && a < kNumActionsPerTable; ++a) {
        auto action_ref = new_table->add_action_refs();
        action_ref->set_id(dummy_action_id++);
      }
    }
  }

  void SetUpTestActions() {
    const int kNumTestActions = kNumTestTables * kNumActionsPerTable;
    const int kNumParamsPerAction = 2;
    int32 dummy_param_id = 100000;

    for (int a = 0; a < kNumTestActions; ++a) {
      ::p4::config::v1::Action* new_action = p4info_.add_actions();
      new_action->mutable_preamble()->set_id(a + kFirstActionID);
      new_action->mutable_preamble()->set_name(
          absl::Substitute("Action-$0", a));
      for (int p = 0; p < kNumParamsPerAction; ++p) {
        auto new_param = new_action->add_params();
        new_param->set_id(dummy_param_id);
        new_param->set_name(
            absl::Substitute("Action-Param-$0", dummy_param_id++));
      }
    }
  }

  //----------------------------------------------------

  // Test fixture variables.
  std::unique_ptr<Es2kExternManager> es2k_extern_manager_;
  PreambleCallback preamble_cb_;
  ::p4::config::v1::P4Info p4info_;
};

//----------------------------------------------------------------------
// Setup tests
//----------------------------------------------------------------------

TEST_F(Es2kExternManagerTest, TestConstructor) {
  ASSERT_TRUE(es2k_extern_manager_);
}

TEST_F(Es2kExternManagerTest, TestCreateInstance) {
  ASSERT_TRUE(Es2kExternManager::CreateInstance());
}

TEST_F(Es2kExternManagerTest, TestFactoryCreateInstance) {
  Es2kTargetFactory target_factory;
  auto extern_manager = target_factory.CreateTdiExternManager();
  ASSERT_TRUE(extern_manager);
}

//----------------------------------------------------------------------
// DirectPacketModMeter tests
//----------------------------------------------------------------------

const std::string kDirectPacketModMeterExternText = R"pb(
  extern_type_id: 134
  extern_type_name: "DirectPacketModMeter"
  instances {
    preamble {
      id: 2264139482
      name: "my_control.meter3"
      alias: "meter3"
    }
    info {
      [type.googleapis.com/idpf.DirectPacketModMeter] {
        spec {
          unit: BYTES
        }
      }
    }
  }
  instances {
    preamble {
      id: 2249256208
      name: "my_control.meter4"
      alias: "meter4"
    }
    info {
      [type.googleapis.com/idpf.DirectPacketModMeter] {
        spec {
          unit: BYTES
        }
      }
    }
  }
)pb";

constexpr char kDirectPacketModMeterTypeName[] = "DirectPacketModMeter";
constexpr uint32 kDirectPacketModMeterTypeID = 134;

constexpr uint32 kDirectPacketModMeterID1 = 2264139482;  // 0x86F406DA
constexpr uint32 kDirectPacketModMeterID2 = 2249256208;  // 0x8610ED10
constexpr char kDirectPacketModMeterName1[] = "my_control.meter3";
constexpr char kDirectPacketModMeterName2[] = "my_control.meter4";

constexpr uint32 kBadDirectPacketModMeterID = 0x8500ACED;
constexpr char kBadDirectPacketModMeterName[] = "self_control.meter.99";

//----------------------------------------------------------------------

void Es2kExternManagerTest::SetUpDirectPacketModMeters() {
  auto p4extern = p4info_.add_externs();
  ASSERT_TRUE(
      ParseProtoFromString(kDirectPacketModMeterExternText, p4extern).ok());
}

void Es2kExternManagerTest::SetUpDirectPacketModMeterTest() {
  SetUpDirectPacketModMeters();
  SetUpPreambleCallback();
  es2k_extern_manager_->RegisterExterns(p4info_, preamble_cb_);
}

//----------------------------------------------------------------------

TEST_F(Es2kExternManagerTest, TestParseDirectPacketModMeters) {
  SetUpDirectPacketModMeters();
  SetUpPreambleCallback();
  es2k_extern_manager_->RegisterExterns(p4info_, preamble_cb_);

  EXPECT_EQ(es2k_extern_manager_->direct_pkt_mod_meter_size(), 2);
  EXPECT_EQ(es2k_extern_manager_->pkt_mod_meter_map_size(), 0);
}

TEST_F(Es2kExternManagerTest, TestFindDirectPacketModMeterByID) {
  SetUpDirectPacketModMeterTest();

  auto directMeter1 =
      es2k_extern_manager_->FindDirectPktModMeterByID(kDirectPacketModMeterID1);
  EXPECT_TRUE(directMeter1.ok());

  auto directMeter3 =
      es2k_extern_manager_->FindDirectPktModMeterByID(kDirectPacketModMeterID2);
  EXPECT_TRUE(directMeter3.ok());
}

TEST_F(Es2kExternManagerTest, TestFindDirectPacketModMeterByName) {
  SetUpDirectPacketModMeterTest();

  auto directMeter2 = es2k_extern_manager_->FindDirectPktModMeterByName(
      kDirectPacketModMeterName1);
  EXPECT_TRUE(directMeter2.ok());

  auto directMeter4 = es2k_extern_manager_->FindDirectPktModMeterByName(
      kDirectPacketModMeterName2);
  EXPECT_TRUE(directMeter4.ok());
}

TEST_F(Es2kExternManagerTest, TestFindBadDirectPacketModMeter) {
  SetUpDirectPacketModMeterTest();

  auto badMeter1 = es2k_extern_manager_->FindDirectPktModMeterByID(
      kBadDirectPacketModMeterID);
  EXPECT_FALSE(badMeter1.ok());

  auto badMeter2 = es2k_extern_manager_->FindDirectPktModMeterByName(
      kBadDirectPacketModMeterName);
  EXPECT_FALSE(badMeter2.ok());
}

//----------------------------------------------------------------------
// PacketModMeter tests
//----------------------------------------------------------------------

const std::string kPacketModMeterExternText = R"pb(
  extern_type_id: 133
  extern_type_name: "PacketModMeter"
  instances {
    preamble {
      id: 2244878476
      name: "my_control.meter1"
      alias: "meter1"
    }
    info {
      [type.googleapis.com/idpf.PacketModMeter] {
        spec {
          unit: PACKETS
        }
        size: 1024
        index_width: 20
      }
    }
  }
  instances {
    preamble {
      id: 2242552391
      name: "my_control.meter2"
      alias: "meter2"
    }
    info {
      [type.googleapis.com/idpf.PacketModMeter] {
        spec {
          unit: PACKETS
        }
        size: 1024
        index_width: 20
      }
    }
  }
)pb";

constexpr uint32 kPacketModMeterID1 = 2244878476;  // 0x85CE208C
constexpr uint32 kPacketModMeterID2 = 2242552391;  // 0x85AAA247
constexpr char kPacketModMeterName1[] = "my_control.meter1";
constexpr char kPacketModMeterName2[] = "my_control.meter2";

constexpr uint32 kBadPacketModMeterID = 0x8500DEAD;
constexpr char kBadPacketModMeterName[] = "mind_control.meter";

constexpr char kPacketModMeterTypeName[] = "PacketModMeter";
constexpr uint32 kPacketModMeterTypeID = 133;

//----------------------------------------------------------------------

void Es2kExternManagerTest::SetUpPacketModMeters() {
  auto p4extern = p4info_.add_externs();
  ASSERT_TRUE(ParseProtoFromString(kPacketModMeterExternText, p4extern).ok());
}

void Es2kExternManagerTest::SetUpPacketModMeterTest() {
  SetUpPacketModMeters();
  SetUpPreambleCallback();
  es2k_extern_manager_->RegisterExterns(p4info_, preamble_cb_);
}

//----------------------------------------------------------------------

TEST_F(Es2kExternManagerTest, TestParsePacketModMeters) {
  SetUpPacketModMeters();
  SetUpPreambleCallback();
  es2k_extern_manager_->RegisterExterns(p4info_, preamble_cb_);

  EXPECT_EQ(es2k_extern_manager_->direct_pkt_mod_meter_size(), 0);
  EXPECT_EQ(es2k_extern_manager_->pkt_mod_meter_map_size(), 2);
}

TEST_F(Es2kExternManagerTest, TestFindPacketModMeterByID) {
  SetUpPacketModMeterTest();

  auto directMeter1 =
      es2k_extern_manager_->FindPktModMeterByID(kPacketModMeterID1);
  EXPECT_TRUE(directMeter1.ok());

  auto directMeter3 =
      es2k_extern_manager_->FindPktModMeterByID(kPacketModMeterID2);
  EXPECT_TRUE(directMeter3.ok());
}

TEST_F(Es2kExternManagerTest, TestFindPacketModMeterByName) {
  SetUpPacketModMeterTest();

  auto directMeter2 =
      es2k_extern_manager_->FindPktModMeterByName(kPacketModMeterName1);
  EXPECT_TRUE(directMeter2.ok());

  auto directMeter4 =
      es2k_extern_manager_->FindPktModMeterByName(kPacketModMeterName2);
  EXPECT_TRUE(directMeter4.ok());
}

TEST_F(Es2kExternManagerTest, TestFindBadPacketModMeter) {
  SetUpPacketModMeterTest();

  auto badMeter1 =
      es2k_extern_manager_->FindPktModMeterByID(kBadPacketModMeterID);
  EXPECT_FALSE(badMeter1.ok());

  auto badMeter2 =
      es2k_extern_manager_->FindPktModMeterByName(kBadPacketModMeterName);
  EXPECT_FALSE(badMeter2.ok());
}

//----------------------------------------------------------------------
// Miscellaneous tests
//----------------------------------------------------------------------

TEST_F(Es2kExternManagerTest, TestFindMeterWhenEmpty) {
  auto directMeter1 =
      es2k_extern_manager_->FindPktModMeterByID(kPacketModMeterID1);
  EXPECT_FALSE(directMeter1.ok());

  auto directMeter2 = es2k_extern_manager_->FindDirectPktModMeterByName(
      kDirectPacketModMeterName2);
  EXPECT_FALSE(directMeter2.ok());
}

TEST_F(Es2kExternManagerTest, TestParseBothExternMeterTypes) {
  SetUpPacketModMeters();
  SetUpDirectPacketModMeters();
  SetUpPreambleCallback();
  es2k_extern_manager_->RegisterExterns(p4info_, preamble_cb_);

  EXPECT_EQ(es2k_extern_manager_->direct_pkt_mod_meter_size(), 2);
  EXPECT_EQ(es2k_extern_manager_->pkt_mod_meter_map_size(), 2);
}

//----------------------------------------------------------------------
// Error tests
//----------------------------------------------------------------------

TEST_F(Es2kExternManagerTest, TestUnknownExternType) {
  auto p4extern = p4info_.add_externs();
  p4extern->set_extern_type_id(0);
  es2k_extern_manager_->RegisterExterns(p4info_, preamble_cb_);

  auto stats = es2k_extern_manager_->statistics();
  EXPECT_EQ(stats.unknown_extern_id, 1);
}

// An ES2K-specific method should return error status when invoked on a
// TdiExternManager object.
TEST_F(Es2kExternManagerTest, TestUnsupportedMethod) {
  auto tdi_extern_manager = TdiExternManager::CreateInstance();
  auto meter = tdi_extern_manager->FindPktModMeterByID(kPacketModMeterID1);
  EXPECT_FALSE(meter.ok());
  EXPECT_EQ(meter.status().error_code(), ERR_UNIMPLEMENTED);
}

//----------------------------------------------------------------------
// P4InfoManager integration test
//----------------------------------------------------------------------

TEST_F(Es2kExternManagerTest, TestP4InfoManagerIntegration) {
  SetUpTestTables();
  SetUpTestActions();
  SetUpPacketModMeters();
  SetUpDirectPacketModMeters();

  auto p4_info_manager = absl::make_unique<P4InfoManager>(p4info_);
  auto status =
      p4_info_manager->InitializeAndVerify(es2k_extern_manager_.get());
  EXPECT_TRUE(status.ok());

  EXPECT_EQ(es2k_extern_manager_->direct_pkt_mod_meter_size(), 2);
  EXPECT_EQ(es2k_extern_manager_->pkt_mod_meter_map_size(), 2);
}

}  // namespace tdi
}  // namespace hal
}  // namespace stratum
