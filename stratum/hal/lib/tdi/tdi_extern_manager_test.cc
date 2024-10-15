// Copyright 2023-2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// Interim unit test for TdiExternManager.
//
// TODO(derek): To be deleted when the PktModMeter and DirectPktModMeter
// methods are removed from TdiExternManager.

#include "stratum/hal/lib/tdi/tdi_extern_manager.h"

#include <iostream>
#include <memory>

#include "gtest/gtest.h"
#include "p4/config/v1/p4info.pb.h"

namespace stratum {
namespace hal {
namespace tdi {

constexpr uint32 kPktModMeterBase = ::p4::config::v1::P4Ids::PACKET_MOD_METER
                                    << 24;
constexpr uint32 kPktModMeterId = kPktModMeterBase + 1;
constexpr char kDirectPktModMeterName[] = "direct-pkt-mod-meter-1";

class TdiExternManagerTest : public testing::Test {
 protected:
  TdiExternManagerTest() : tdi_extern_manager_(new TdiExternManager) {}

  std::unique_ptr<TdiExternManager> tdi_extern_manager_;
  ::p4::config::v1::P4Info p4info_;
};

TEST_F(TdiExternManagerTest, TestConstructor) {
  ASSERT_TRUE(tdi_extern_manager_);
}

TEST_F(TdiExternManagerTest, TestCreateInstance) {
  ASSERT_TRUE(TdiExternManager::CreateInstance());
}

TEST_F(TdiExternManagerTest, TestFindPktModMeterByID) {
  auto pktModMeter = tdi_extern_manager_->FindPktModMeterByID(kPktModMeterId);
  ASSERT_FALSE(pktModMeter.ok());
  ASSERT_EQ(pktModMeter.status().error_code(), ERR_UNIMPLEMENTED);
}

TEST_F(TdiExternManagerTest, TestFindDirectPktModMeterByName) {
  auto pktModMeter =
      tdi_extern_manager_->FindDirectPktModMeterByName(kDirectPktModMeterName);
  ASSERT_FALSE(pktModMeter.ok());
  ASSERT_EQ(pktModMeter.status().error_code(), ERR_UNIMPLEMENTED);
}

}  // namespace tdi
}  // namespace hal
}  // namespace stratum
