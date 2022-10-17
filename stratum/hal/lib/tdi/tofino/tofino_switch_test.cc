// Copyright 2018 Google LLC
// Copyright 2018-present Open Networking Foundation
// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// adapted from bcm_switch_test

#include "stratum/hal/lib/tdi/tofino/tofino_switch.h"

#include <map>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "stratum/glue/status/canonical_errors.h"
#include "stratum/glue/status/status_test_util.h"
#include "stratum/hal/lib/barefoot/tofino_chassis_manager_mock.h"
#include "stratum/hal/lib/barefoot/tdi_sde_mock.h"
#include "stratum/hal/lib/barefoot/tdi_node_mock.h"
#include "stratum/hal/lib/common/phal_mock.h"
#include "stratum/hal/lib/common/writer_mock.h"
#include "stratum/lib/utils.h"

namespace stratum {
namespace hal {
namespace tdi {

using ::testing::_;
using ::testing::DoAll;
using ::testing::HasSubstr;
using ::testing::InSequence;
using ::testing::Invoke;
using ::testing::Pointee;
using ::testing::Return;
using ::testing::Sequence;
using ::testing::WithArg;
using ::testing::WithArgs;

namespace {

MATCHER_P(EqualsProto, proto, "") { return ProtoEqual(arg, proto); }

MATCHER_P(DerivedFromStatus, status, "") {
  if (arg.error_code() != status.error_code()) {
    return false;
  }
  if (arg.error_message().find(status.error_message()) == std::string::npos) {
    *result_listener << "\nOriginal error string: \"" << status.error_message()
                     << "\" is missing from the actual status.";
    return false;
  }
  return true;
}

constexpr uint64 kNodeId = 13579;
constexpr int kDevice = 2;
constexpr char kErrorMsg[] = "Test error message";
constexpr uint32 kPortId = 2468;

const absl::flat_hash_map<uint64, int>& NodeIdToDeviceMap() {
  static auto* map = new absl::flat_hash_map<uint64, int>({{kNodeId, kDevice}});
  return *map;
}

class TofinoSwitchTest : public ::testing::Test {
 protected:
  void SetUp() override {
    phal_mock_ = absl::make_unique<PhalMock>();
    sde_mock_ = absl::make_unique<TdiSdeMock>();
    chassis_manager_mock_ = absl::make_unique<TofinoChassisManagerMock>();
    node_mock_ = absl::make_unique<TdiNodeMock>();
    device_to_tdi_node_mock_[kDevice] = node_mock_.get();
    tofino_switch_ = TofinoSwitch::CreateInstance(
        phal_mock_.get(), chassis_manager_mock_.get(), sde_mock_.get(),
        device_to_tdi_node_mock_);

    ON_CALL(*chassis_manager_mock_, GetNodeIdToDeviceMap())
        .WillByDefault(Return(NodeIdToDeviceMap()));
  }

  void TearDown() override { device_to_tdi_node_mock_.clear(); }

  void PushChassisConfigSuccess() {
    ChassisConfig config;
    config.add_nodes()->set_id(kNodeId);
    {
      InSequence sequence;  // The order of the calls are important. Enforce it.
      EXPECT_CALL(*phal_mock_, VerifyChassisConfig(EqualsProto(config)))
          .WillOnce(Return(::util::OkStatus()));
      EXPECT_CALL(*chassis_manager_mock_,
                  VerifyChassisConfig(EqualsProto(config)))
          .WillOnce(Return(::util::OkStatus()));
      EXPECT_CALL(*node_mock_,
                  VerifyChassisConfig(EqualsProto(config), kNodeId))
          .WillOnce(Return(::util::OkStatus()));
      EXPECT_CALL(*phal_mock_, PushChassisConfig(EqualsProto(config)))
          .WillOnce(Return(::util::OkStatus()));
      EXPECT_CALL(*chassis_manager_mock_,
                  PushChassisConfig(EqualsProto(config)))
          .WillOnce(Return(::util::OkStatus()));
      EXPECT_CALL(*node_mock_,
                  PushChassisConfig(EqualsProto(config), kNodeId))
          .WillOnce(Return(::util::OkStatus()));
    }
    EXPECT_OK(tofino_switch_->PushChassisConfig(config));
  }

  ::util::Status DefaultError() {
    return ::util::Status(StratumErrorSpace(), ERR_UNKNOWN, kErrorMsg);
  }

 protected:
  std::unique_ptr<PhalMock> phal_mock_;
  std::unique_ptr<TdiSdeMock> sde_mock_;
  std::unique_ptr<TofinoChassisManagerMock> chassis_manager_mock_;
  std::unique_ptr<TdiNodeMock> node_mock_;
  absl::flat_hash_map<int, TdiNode*> device_to_tdi_node_mock_;
  std::unique_ptr<TofinoSwitch> tofino_switch_;
};

TEST_F(TofinoSwitchTest, PushChassisConfigSuccess) { PushChassisConfigSuccess(); }

TEST_F(TofinoSwitchTest, PushChassisConfigFailureWhenPhalVerifyFails) {
  ChassisConfig config;
  config.add_nodes()->set_id(kNodeId);
  EXPECT_CALL(*phal_mock_, VerifyChassisConfig(EqualsProto(config)))
      .WillOnce(Return(DefaultError()));
  EXPECT_CALL(*chassis_manager_mock_,
              VerifyChassisConfig(EqualsProto(config)))
      .WillOnce(Return(::util::OkStatus()));
  EXPECT_CALL(*node_mock_,
              VerifyChassisConfig(EqualsProto(config), kNodeId))
      .WillOnce(Return(::util::OkStatus()));

  EXPECT_THAT(tofino_switch_->PushChassisConfig(config),
              DerivedFromStatus(DefaultError()));
}

TEST_F(TofinoSwitchTest, PushChassisConfigFailureWhenNodeVerifyFails) {
  ChassisConfig config;
  config.add_nodes()->set_id(kNodeId);
  EXPECT_CALL(*phal_mock_, VerifyChassisConfig(EqualsProto(config)))
      .WillOnce(Return(::util::OkStatus()));
  EXPECT_CALL(*chassis_manager_mock_,
              VerifyChassisConfig(EqualsProto(config)))
      .WillOnce(Return(::util::OkStatus()));
  EXPECT_CALL(*node_mock_,
              VerifyChassisConfig(EqualsProto(config), kNodeId))
      .WillOnce(Return(DefaultError()));

  EXPECT_THAT(tofino_switch_->PushChassisConfig(config),
              DerivedFromStatus(DefaultError()));
}

TEST_F(TofinoSwitchTest, PushChassisConfigFailureWhenPhalPushFails) {
  ChassisConfig config;
  config.add_nodes()->set_id(kNodeId);
  EXPECT_CALL(*phal_mock_, VerifyChassisConfig(EqualsProto(config)))
      .WillOnce(Return(::util::OkStatus()));
  EXPECT_CALL(*chassis_manager_mock_,
              VerifyChassisConfig(EqualsProto(config)))
      .WillOnce(Return(::util::OkStatus()));
  EXPECT_CALL(*node_mock_,
              VerifyChassisConfig(EqualsProto(config), kNodeId))
      .WillOnce(Return(::util::OkStatus()));
  EXPECT_CALL(*phal_mock_, PushChassisConfig(EqualsProto(config)))
      .WillOnce(Return(DefaultError()));

  EXPECT_THAT(tofino_switch_->PushChassisConfig(config),
              DerivedFromStatus(DefaultError()));
}

TEST_F(TofinoSwitchTest, PushChassisConfigFailureWhenChassisManagerPushFails) {
  ChassisConfig config;
  config.add_nodes()->set_id(kNodeId);
  EXPECT_CALL(*phal_mock_, VerifyChassisConfig(EqualsProto(config)))
      .WillOnce(Return(::util::OkStatus()));
  EXPECT_CALL(*chassis_manager_mock_,
              VerifyChassisConfig(EqualsProto(config)))
      .WillOnce(Return(::util::OkStatus()));
  EXPECT_CALL(*node_mock_,
              VerifyChassisConfig(EqualsProto(config), kNodeId))
      .WillOnce(Return(::util::OkStatus()));
  EXPECT_CALL(*phal_mock_, PushChassisConfig(EqualsProto(config)))
      .WillOnce(Return(::util::OkStatus()));
  EXPECT_CALL(*chassis_manager_mock_, PushChassisConfig(EqualsProto(config)))
      .WillOnce(Return(DefaultError()));

  EXPECT_THAT(tofino_switch_->PushChassisConfig(config),
              DerivedFromStatus(DefaultError()));
}

TEST_F(TofinoSwitchTest, PushChassisConfigFailureWhenNodePushFails) {
  ChassisConfig config;
  config.add_nodes()->set_id(kNodeId);
  EXPECT_CALL(*phal_mock_, VerifyChassisConfig(EqualsProto(config)))
      .WillOnce(Return(::util::OkStatus()));
  EXPECT_CALL(*chassis_manager_mock_,
              VerifyChassisConfig(EqualsProto(config)))
      .WillOnce(Return(::util::OkStatus()));
  EXPECT_CALL(*node_mock_,
              VerifyChassisConfig(EqualsProto(config), kNodeId))
      .WillOnce(Return(::util::OkStatus()));
  EXPECT_CALL(*phal_mock_, PushChassisConfig(EqualsProto(config)))
      .WillOnce(Return(::util::OkStatus()));
  EXPECT_CALL(*chassis_manager_mock_, PushChassisConfig(EqualsProto(config)))
      .WillOnce(Return(::util::OkStatus()));
  EXPECT_CALL(*node_mock_, PushChassisConfig(EqualsProto(config), kNodeId))
      .WillOnce(Return(DefaultError()));

  EXPECT_THAT(tofino_switch_->PushChassisConfig(config),
              DerivedFromStatus(DefaultError()));
}

TEST_F(TofinoSwitchTest, VerifyChassisConfigSuccess) {
  ChassisConfig config;
  config.add_nodes()->set_id(kNodeId);
  {
    InSequence sequence;  // The order of the calls are important. Enforce it.
    EXPECT_CALL(*phal_mock_, VerifyChassisConfig(EqualsProto(config)))
        .WillOnce(Return(::util::OkStatus()));
    EXPECT_CALL(*chassis_manager_mock_,
                VerifyChassisConfig(EqualsProto(config)))
        .WillOnce(Return(::util::OkStatus()));
    EXPECT_CALL(*node_mock_,
                VerifyChassisConfig(EqualsProto(config), kNodeId))
        .WillOnce(Return(::util::OkStatus()));
  }
  EXPECT_OK(tofino_switch_->VerifyChassisConfig(config));
}

TEST_F(TofinoSwitchTest, VerifyChassisConfigFailureWhenPhalVerifyFails) {
  ChassisConfig config;
  config.add_nodes()->set_id(kNodeId);
  EXPECT_CALL(*phal_mock_, VerifyChassisConfig(EqualsProto(config)))
      .WillOnce(Return(DefaultError()));
  EXPECT_CALL(*chassis_manager_mock_,
              VerifyChassisConfig(EqualsProto(config)))
      .WillOnce(Return(::util::OkStatus()));
  EXPECT_CALL(*node_mock_,
              VerifyChassisConfig(EqualsProto(config), kNodeId))
      .WillOnce(Return(::util::OkStatus()));

  EXPECT_THAT(tofino_switch_->VerifyChassisConfig(config),
              DerivedFromStatus(DefaultError()));
}

TEST_F(TofinoSwitchTest,
       VerifyChassisConfigFailureWhenChassisManagerVerifyFails) {
  ChassisConfig config;
  config.add_nodes()->set_id(kNodeId);
  EXPECT_CALL(*phal_mock_, VerifyChassisConfig(EqualsProto(config)))
      .WillOnce(Return(::util::OkStatus()));
  EXPECT_CALL(*chassis_manager_mock_,
              VerifyChassisConfig(EqualsProto(config)))
      .WillOnce(Return(DefaultError()));
  EXPECT_CALL(*node_mock_,
              VerifyChassisConfig(EqualsProto(config), kNodeId))
      .WillOnce(Return(::util::OkStatus()));

  EXPECT_THAT(tofino_switch_->VerifyChassisConfig(config),
              DerivedFromStatus(DefaultError()));
}

TEST_F(TofinoSwitchTest, VerifyChassisConfigFailureWhenNodeVerifyFails) {
  ChassisConfig config;
  config.add_nodes()->set_id(kNodeId);
  EXPECT_CALL(*phal_mock_, VerifyChassisConfig(EqualsProto(config)))
      .WillOnce(Return(::util::OkStatus()));
  EXPECT_CALL(*chassis_manager_mock_,
              VerifyChassisConfig(EqualsProto(config)))
      .WillOnce(Return(::util::OkStatus()));
  EXPECT_CALL(*node_mock_,
              VerifyChassisConfig(EqualsProto(config), kNodeId))
      .WillOnce(Return(DefaultError()));

  EXPECT_THAT(tofino_switch_->VerifyChassisConfig(config),
              DerivedFromStatus(DefaultError()));
}

TEST_F(TofinoSwitchTest,
       VerifyChassisConfigFailureWhenMoreThanOneManagerVerifyFails) {
  ChassisConfig config;
  config.add_nodes()->set_id(kNodeId);
  EXPECT_CALL(*phal_mock_, VerifyChassisConfig(EqualsProto(config)))
      .WillOnce(Return(::util::OkStatus()));
  EXPECT_CALL(*chassis_manager_mock_,
              VerifyChassisConfig(EqualsProto(config)))
      .WillOnce(Return(DefaultError()));
  EXPECT_CALL(*node_mock_,
              VerifyChassisConfig(EqualsProto(config), kNodeId))
      .WillOnce(Return(::util::Status(StratumErrorSpace(), ERR_INVALID_PARAM,
                                      "some other text")));

  // we keep the error code from the first error
  EXPECT_THAT(tofino_switch_->VerifyChassisConfig(config),
              DerivedFromStatus(DefaultError()));
}

TEST_F(TofinoSwitchTest, ShutdownSuccess) {
  EXPECT_CALL(*node_mock_, Shutdown())
      .WillOnce(Return(::util::OkStatus()));
  EXPECT_CALL(*chassis_manager_mock_, Shutdown())
      .WillOnce(Return(::util::OkStatus()));
  EXPECT_CALL(*phal_mock_, Shutdown()).WillOnce(Return(::util::OkStatus()));

  EXPECT_OK(tofino_switch_->Shutdown());
}

TEST_F(TofinoSwitchTest, ShutdownFailureWhenSomeManagerShutdownFails) {
  EXPECT_CALL(*node_mock_, Shutdown())
      .WillOnce(Return(::util::OkStatus()));
  EXPECT_CALL(*chassis_manager_mock_, Shutdown())
      .WillOnce(Return(DefaultError()));
  EXPECT_CALL(*phal_mock_, Shutdown()).WillOnce(Return(::util::OkStatus()));

  EXPECT_THAT(tofino_switch_->Shutdown(), DerivedFromStatus(DefaultError()));
}

// PushForwardingPipelineConfig() should verify and propagate the config.
TEST_F(TofinoSwitchTest, PushForwardingPipelineConfigSuccess) {
  PushChassisConfigSuccess();

  ::p4::v1::ForwardingPipelineConfig config;
  {
    InSequence sequence;
    EXPECT_CALL(*node_mock_,
                VerifyForwardingPipelineConfig(EqualsProto(config)))
        .WillOnce(Return(::util::OkStatus()));
    EXPECT_CALL(*node_mock_,
                PushForwardingPipelineConfig(EqualsProto(config)))
        .WillOnce(Return(::util::OkStatus()));
  }
  EXPECT_OK(tofino_switch_->PushForwardingPipelineConfig(kNodeId, config));
}

// When TofinoSwitch fails to verify a forwarding config during
// PushForwardingPipelineConfig(), it should not propagate the config and fail.
TEST_F(TofinoSwitchTest, PushForwardingPipelineConfigFailureWhenVerifyFails) {
  PushChassisConfigSuccess();

  ::p4::v1::ForwardingPipelineConfig config;
  EXPECT_CALL(*node_mock_,
              VerifyForwardingPipelineConfig(EqualsProto(config)))
      .WillOnce(Return(DefaultError()));
  EXPECT_CALL(*node_mock_, PushForwardingPipelineConfig(_)).Times(0);
  EXPECT_THAT(tofino_switch_->PushForwardingPipelineConfig(kNodeId, config),
              DerivedFromStatus(DefaultError()));
}

// When TofinoSwitch fails to push a forwarding config during
// PushForwardingPipelineConfig(), it should fail immediately.
TEST_F(TofinoSwitchTest, PushForwardingPipelineConfigFailureWhenPushFails) {
  PushChassisConfigSuccess();

  ::p4::v1::ForwardingPipelineConfig config;
  EXPECT_CALL(*node_mock_,
              VerifyForwardingPipelineConfig(EqualsProto(config)))
      .WillOnce(Return(::util::OkStatus()));
  EXPECT_CALL(*node_mock_,
              PushForwardingPipelineConfig(EqualsProto(config)))
      .WillOnce(Return(DefaultError()));
  EXPECT_THAT(tofino_switch_->PushForwardingPipelineConfig(kNodeId, config),
              DerivedFromStatus(DefaultError()));
}

TEST_F(TofinoSwitchTest, VerifyForwardingPipelineConfigSuccess) {
  PushChassisConfigSuccess();

  ::p4::v1::ForwardingPipelineConfig config;
  {
    InSequence sequence;
    // Verify should always be called before push.
    EXPECT_CALL(*node_mock_,
                VerifyForwardingPipelineConfig(EqualsProto(config)))
        .WillOnce(Return(::util::OkStatus()));
  }
  EXPECT_OK(tofino_switch_->VerifyForwardingPipelineConfig(kNodeId, config));
}

// Test registration of a writer for sending gNMI events.
TEST_F(TofinoSwitchTest, RegisterEventNotifyWriterTest) {
  auto writer = std::shared_ptr<WriterInterface<GnmiEventPtr>>(
      new WriterMock<GnmiEventPtr>());

  EXPECT_CALL(*chassis_manager_mock_, RegisterEventNotifyWriter(writer))
      .WillOnce(Return(::util::OkStatus()))
      .WillOnce(Return(DefaultError()));

  // Successful BfChassisManager registration.
  EXPECT_OK(tofino_switch_->RegisterEventNotifyWriter(writer));
  // Failed BfChassisManager registration.
  EXPECT_THAT(tofino_switch_->RegisterEventNotifyWriter(writer),
              DerivedFromStatus(DefaultError()));
}

namespace {
void ExpectMockWriteDataResponse(WriterMock<DataResponse>* writer,
                                 DataResponse* resp) {
  // Mock implementation of Write() that saves the response to local variable.
  EXPECT_CALL(*writer, Write(_))
      .WillOnce(DoAll(WithArg<0>(Invoke([resp](DataResponse r) {
                        // Copy the response.
                        *resp = r;
                      })),
                      Return(true)));
}
}  // namespace

TEST_F(TofinoSwitchTest, RetrieveValueNodeInfo) {
  constexpr char kTofinoChipTypeString[] = "T32-X";

  PushChassisConfigSuccess();

  WriterMock<DataResponse> writer;
  DataResponse resp;

  // Expect successful retrieval followed by failure.
  ::util::Status error = ::util::UnknownErrorBuilder(GTL_LOC) << "error";
  EXPECT_CALL(*chassis_manager_mock_, GetDeviceFromNodeId(kNodeId))
      .WillOnce(Return(kDevice))
      .WillOnce(Return(error));
  ExpectMockWriteDataResponse(&writer, &resp);

  EXPECT_CALL(*sde_mock_, GetBfChipType(kDevice))
      .WillOnce(Return(kTofinoChipTypeString));

  DataRequest req;
  auto* req_info = req.add_requests()->mutable_node_info();
  req_info->set_node_id(kNodeId);
  std::vector<::util::Status> details;

  EXPECT_OK(tofino_switch_->RetrieveValue(kNodeId, req, &writer, &details));
  EXPECT_TRUE(resp.has_node_info());
  EXPECT_EQ(kTofinoChipTypeString, resp.node_info().chip_name());
  ASSERT_EQ(details.size(), 1);
  EXPECT_THAT(details.at(0), ::util::OkStatus());

  details.clear();
  resp.Clear();
  EXPECT_OK(tofino_switch_->RetrieveValue(kNodeId, req, &writer, &details));
  EXPECT_FALSE(resp.has_node_info());
  ASSERT_EQ(details.size(), 1);
  EXPECT_EQ(error.ToString(), details.at(0).ToString());
}

// TODO(max): add more tests, use BcmSwitch as a reference.

}  // namespace

}  // namespace tdi
}  // namespace hal
}  // namespace stratum
