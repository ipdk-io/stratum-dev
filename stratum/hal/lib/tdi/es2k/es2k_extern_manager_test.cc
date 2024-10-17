// Copyright 2023-2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// Unit test for Es2kExternManager.

#include "stratum/hal/lib/tdi/es2k/es2k_extern_manager.h"

#include <iostream>
#include <memory>
#include <string>

#include "gtest/gtest.h"
#include "p4/config/v1/p4info.pb.h"
#include "stratum/glue/status/status.h"

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

  // Dummy preamble processing callback function.
  ::util::Status ProcessPreamble(const ::p4::config::v1::Preamble& preamble,
                                 const std::string& resource_type) {
    return ::util::OkStatus();
  }

  void SetUpPreambleCallback() {
    preamble_cb_ = std::bind(&Es2kExternManagerTest::ProcessPreamble, this,
                             std::placeholders::_1, std::placeholders::_2);
  }

  ::p4::config::v1::Extern* AddExtern(uint32 type_id,
                                      const std::string& type_name) {
    auto p4extern = p4info_.add_externs();
    p4extern->set_extern_type_id(type_id);
    p4extern->set_extern_type_name(type_name);
    return p4extern;
  }

  void AddMeterInstance(::p4::config::v1::Extern* p4extern, uint32 resource_id,
                        std::string& resource_name, uint32 unit) {
    auto instances = p4extern->add_instances();

    auto preamble = instances->mutable_preamble();
    preamble->set_id(resource_id);
    preamble->set_name(resource_name);
    // TODO(derek): add {info: {spec: {unit: UNIT}}}.
    // See PacketModMeter and DirectPacketModMeter in p4info.proto.
    // Return instances?
  }

  std::unique_ptr<Es2kExternManager> es2k_extern_manager_;
  PreambleCallback preamble_cb_;
  ::p4::config::v1::P4Info p4info_;
};

TEST_F(Es2kExternManagerTest, TestConstructor) {
  ASSERT_TRUE(es2k_extern_manager_);
}

TEST_F(Es2kExternManagerTest, TestCreateInstance) {
  ASSERT_TRUE(Es2kExternManager::CreateInstance());
}

TEST_F(Es2kExternManagerTest, TestInitialize) {
  es2k_extern_manager_->RegisterExterns(p4info_, preamble_cb_);
  EXPECT_EQ(es2k_extern_manager_->DirectPktModMeterMapSize(), 0);
  EXPECT_EQ(es2k_extern_manager_->PktModMeterMapSize(), 0);
}

}  // namespace tdi
}  // namespace hal
}  // namespace stratum
