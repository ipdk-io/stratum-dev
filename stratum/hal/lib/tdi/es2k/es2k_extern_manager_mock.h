// Copyright 2023-2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#ifndef STRATUM_HAL_LIB_TDI_ES2K_ES2K_EXTERN_MANAGER_MOCK_H_
#define STRATUM_HAL_LIB_TDI_ES2K_ES2K_EXTERN_MANAGER_MOCK_H_

#include "gmock/gmock.h"
#include "stratum/hal/lib/tdi/es2k/es2k_extern_manager.h"

namespace stratum {
namespace hal {
namespace tdi {

class Es2kExternManagerMock : public Es2kExternManager {
 public:
  MOCK_METHOD(void, RegisterExterns,
              (const ::p4::config::v1::P4Info& p4info,
               const PreambleCallback& preamble_cb),
              (override));

  MOCK_METHOD(::util::StatusOr<const ::idpf::PacketModMeter>,
              FindPktModMeterByID, (uint32 meter_id), (const, override));

  MOCK_METHOD(::util::StatusOr<const ::idpf::PacketModMeter>,
              FindPktModMeterByName, (const std::string& meter_name),
              (const, override));

  MOCK_METHOD(::util::StatusOr<const ::idpf::DirectPacketModMeter>,
              FindDirectPktModMeterByID, (uint32 meter_id), (const, override));

  MOCK_METHOD(::util::StatusOr<const ::idpf::DirectPacketModMeter>,
              FindDirectPktModMeterByName, (const std::string& meter_name),
              (const, override));

  MOCK_METHOD(uint32, direct_pkt_mod_meter_size, (), (const, override));

  MOCK_METHOD(uint32, pkt_mod_meter_map_size, (), (const, override));

  MOCK_METHOD(const struct Es2kExternManager::Statistics&, statistics, (),
              (const, override));
};

}  // namespace tdi
}  // namespace hal
}  // namespace stratum

#endif  // STRATUM_HAL_LIB_TDI_ES2K_ES2K_EXTERN_MANAGER_MOCK_H_
