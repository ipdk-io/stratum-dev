// Copyright 2023-2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#ifndef STRATUM_HAL_LIB_TDI_TDI_EXTERN_MANAGER_H_
#define STRATUM_HAL_LIB_TDI_TDI_EXTERN_MANAGER_H_

#include <string>

#include "idpf/p4info.pb.h"
#include "p4/config/v1/p4info.pb.h"
#include "stratum/glue/integral_types.h"
#include "stratum/glue/status/statusor.h"
#include "stratum/hal/lib/p4/p4_extern_manager.h"
#include "stratum/hal/lib/p4/p4_resource_map.h"

namespace stratum {
namespace hal {
namespace tdi {

class TdiExternManager : public P4ExternManager {
 public:
  TdiExternManager() {}
  virtual ~TdiExternManager() = default;

  static std::unique_ptr<TdiExternManager> CreateInstance();

  void Initialize(const ::p4::config::v1::P4Info& p4info,
                  const PreambleCallback& preamble_cb) override;

  // TODO(derek): Once the resource handlers have been implemented,
  // remove the following methods from TdiExternManager and
  // dynamic_cast<Es2kExternManager>(extern_manager.get()) in
  // Es2kResourceMapper to obtain the pointer to use when
  // instantiating the Es2k PktModMeter handlers.
  ::util::StatusOr<const ::idpf::PacketModMeter> FindPktModMeterByID(
      uint32 meter_id) const;

  ::util::StatusOr<const ::idpf::PacketModMeter> FindPktModMeterByName(
      const std::string& meter_name) const;

  ::util::StatusOr<const ::idpf::DirectPacketModMeter>
  FindDirectPktModMeterByID(uint32 meter_id) const;

  ::util::StatusOr<const ::idpf::DirectPacketModMeter>
  FindDirectPktModMeterByName(const std::string& meter_name) const;
};

}  // namespace tdi
}  // namespace hal
}  // namespace stratum

#endif  // STRATUM_HAL_LIB_TDI_TDI_EXTERN_MANAGER_H_
