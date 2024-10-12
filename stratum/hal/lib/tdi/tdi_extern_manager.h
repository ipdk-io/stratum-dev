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

class TdiExternManager : public P4ExternManager {
 public:
  TdiExternManager();
  virtual ~TdiExternManager() {}

  virtual void Initialize(const ::p4::config::v1::P4Info& p4info,
                          const PreambleCallback& preamble_cb);

  virtual ::util::StatusOr<const ::idpf::PacketModMeter> FindPktModMeterByID(
      uint32 meter_id) const;

  ::util::StatusOr<const ::idpf::PacketModMeter> FindPktModMeterByName(
      const std::string& meter_name) const;

  virtual ::util::StatusOr<const ::idpf::DirectPacketModMeter>
  FindDirectPktModMeterByID(uint32 meter_id) const;

  virtual ::util::StatusOr<const ::idpf::DirectPacketModMeter>
  FindDirectPktModMeterByName(const std::string& meter_name) const;

  // Instance is neither copyable nor movable.
  TdiExternManager(const TdiExternManager&) = delete;
  TdiExternManager& operator=(const TdiExternManager&) = delete;
};

}  // namespace hal
}  // namespace stratum

#endif  // STRATUM_HAL_LIB_TDI_TDI_EXTERN_MANAGER_H_
