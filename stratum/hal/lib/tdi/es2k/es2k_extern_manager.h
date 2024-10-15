// Copyright 2023-2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#ifndef STRATUM_HAL_LIB_TDI_ES2K_ES2K_EXTERN_MANAGER_H_
#define STRATUM_HAL_LIB_TDI_ES2K_ES2K_EXTERN_MANAGER_H_

#include <string>

#include "idpf/p4info.pb.h"
#include "p4/config/v1/p4info.pb.h"
#include "stratum/glue/integral_types.h"
#include "stratum/glue/status/statusor.h"
#include "stratum/hal/lib/p4/p4_resource_map.h"
#include "stratum/hal/lib/tdi/tdi_extern_manager.h"

namespace stratum {
namespace hal {
namespace tdi {

class Es2kExternManager : public TdiExternManager {
 public:
  Es2kExternManager();
  virtual ~Es2kExternManager() = default;

  void Initialize(const ::p4::config::v1::P4Info& p4info,
                  const PreambleCallback& preamble_cb) override;

  ::util::StatusOr<const ::idpf::PacketModMeter> FindPktModMeterByID(
      uint32 meter_id) const;
  ::util::StatusOr<const ::idpf::PacketModMeter> FindPktModMeterByName(
      const std::string& meter_name) const;

  ::util::StatusOr<const ::idpf::DirectPacketModMeter>
  FindDirectPktModMeterByID(uint32 meter_id) const;
  ::util::StatusOr<const ::idpf::DirectPacketModMeter>
  FindDirectPktModMeterByName(const std::string& meter_name) const;

  uint32 pkt_mod_meter_map_size() const { return pkt_mod_meter_map_.size(); }
  uint32 direct_pkt_mod_meter_map_size() const {
    return direct_pkt_mod_meter_map_.size();
  }

 private:
  void InitPacketModMeters(const p4::config::v1::Extern& p4extern,
                           const PreambleCallback& preamble_cb);

  void InitDirectPacketModMeters(const p4::config::v1::Extern& p4extern,
                                 const PreambleCallback& preamble_cb);

  // One P4ResourceMap for each P4 extern resource type.
  P4ResourceMap<::idpf::PacketModMeter> pkt_mod_meter_map_;
  P4ResourceMap<::idpf::DirectPacketModMeter> direct_pkt_mod_meter_map_;

  google::protobuf::RepeatedPtrField<::idpf::PacketModMeter> meter_objects_;
  google::protobuf::RepeatedPtrField<::idpf::DirectPacketModMeter>
      direct_meter_objects_;
};

}  // namespace tdi
}  // namespace hal
}  // namespace stratum

#endif  // STRATUM_HAL_LIB_TDI_ES2K_ES2K_EXTERN_MANAGER_H_
