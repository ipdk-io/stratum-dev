// Copyright 2018 Google LLC
// Copyright 2018-present Open Networking Foundation
// Copyright 2023-2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ES2K-specific P4 extern manager.

#ifndef STRATUM_HAL_LIB_TDI_ES2K_ES2K_EXTERN_MANAGER_H_
#define STRATUM_HAL_LIB_TDI_ES2K_ES2K_EXTERN_MANAGER_H_

#include <string>

#include "idpf/p4info.pb.h"
#include "p4/config/v1/p4info.pb.h"
#include "stratum/glue/integral_types.h"
#include "stratum/glue/status/statusor.h"
#include "stratum/hal/lib/p4/p4_extern_manager.h"
#include "stratum/hal/lib/p4/p4_resource_map.h"

namespace stratum {
namespace hal {

class Es2kExternManager : public P4ExternManager {
 public:
  Es2kExternManager();
  virtual ~Es2kExternManager() {}

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

  // Instance is neither copyable nor movable.
  Es2kExternManager(const Es2kExternManager&) = delete;
  Es2kExternManager& operator=(const Es2kExternManager&) = delete;

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

}  // namespace hal
}  // namespace stratum

#endif  // STRATUM_HAL_LIB_TDI_ES2K_ES2K_EXTERN_MANAGER_H_
