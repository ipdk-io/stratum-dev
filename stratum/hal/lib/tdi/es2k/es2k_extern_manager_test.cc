// Copyright 2023-2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// Unit test for Es2kExternManager.

#include "stratum/hal/lib/tdi/es2k/es2k_extern_manager.h"

#include <memory>
#include <string>

#include "absl/memory/memory.h"
#include "gtest/gtest.h"
#include "p4/config/v1/p4info.pb.h"
#include "stratum/glue/status/status.h"
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

  ::util::Status ProcessPreamble(const ::p4::config::v1::Preamble& preamble,
                                 const std::string& resource_type);

  void SetUpPreambleCallback();

  void SetUpDirectPacketModMeters();
  void SetUpDirectPacketModMeterTest();
  void SetUpPacketModMeters();
  void SetUpPacketModMeterTest();

  std::unique_ptr<Es2kExternManager> es2k_extern_manager_;
  PreambleCallback preamble_cb_;
  ::p4::config::v1::P4Info p4info_;
  std::unique_ptr<absl::Mutex> lock_;
};

void Es2kExternManagerTest::SetUpPreambleCallback() {
  preamble_cb_ = std::bind(&Es2kExternManagerTest::ProcessPreamble, this,
                           std::placeholders::_1, std::placeholders::_2);
}

// Dummy preamble processing callback function.
::util::Status Es2kExternManagerTest::ProcessPreamble(
    const ::p4::config::v1::Preamble& preamble,
    const std::string& resource_type) {
  return ::util::OkStatus();
}

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

constexpr char kZeroPreambleIdText[] = R"pb(
  extern_type_id: 134
  extern_type_name: "DirectPacketModMeter"
  instances {
    preamble {
      id: 0
      name: "my_control.meter3"
      alias: "meter3"
    }
  }
)pb";

}  // namespace tdi
}  // namespace hal
}  // namespace stratum
