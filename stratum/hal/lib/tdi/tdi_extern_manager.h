// Copyright 2023-2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#ifndef STRATUM_HAL_LIB_TDI_TDI_EXTERN_MANAGER_H_
#define STRATUM_HAL_LIB_TDI_TDI_EXTERN_MANAGER_H_

#include "absl/memory/memory.h"
#include "idpf/p4info.pb.h"
#include "p4/config/v1/p4info.pb.h"
#include "stratum/glue/integral_types.h"
#include "stratum/glue/status/statusor.h"
#include "stratum/hal/lib/p4/p4_extern_manager.h"
#include "stratum/hal/lib/p4/p4_info_manager.h"
#include "stratum/hal/lib/p4/p4_resource_map.h"

namespace stratum {
namespace hal {
namespace tdi {

class TdiExternManager : public P4ExternManager {
 public:
  TdiExternManager() {}
  virtual ~TdiExternManager() = default;

  // Called by TdiTargetFactory.
  static std::unique_ptr<TdiExternManager> CreateInstance() {
    return absl::make_unique<TdiExternManager>();
  }

  // Registers P4Extern resources. Called by P4InfoManager.
  void RegisterExterns(const ::p4::config::v1::P4Info& p4info,
                       const PreambleCallback& preamble_cb) override {}

  // Returns the handler for the P4 resource with the specified ID,
  // or null if the resource is not registered.
  virtual TdiResourceHandler* FindResourceHandler(uint32 resource_id) {
    return nullptr;
  }

  // Retrieve a PacketModMeter configuration.
  virtual ::util::StatusOr<const ::idpf::PacketModMeter> FindPktModMeterByID(
      uint32 meter_id) const;

  virtual ::util::StatusOr<const ::idpf::PacketModMeter> FindPktModMeterByName(
      const std::string& meter_name) const;

  // Retrieve a DirectPacketModMeter configuration.
  virtual ::util::StatusOr<const ::idpf::DirectPacketModMeter>
  FindDirectPktModMeterByID(uint32 meter_id) const;

  virtual ::util::StatusOr<const ::idpf::DirectPacketModMeter>
  FindDirectPktModMeterByName(const std::string& meter_name) const;
};

}  // namespace tdi
}  // namespace hal
}  // namespace stratum

#endif  // STRATUM_HAL_LIB_TDI_TDI_EXTERN_MANAGER_H_
