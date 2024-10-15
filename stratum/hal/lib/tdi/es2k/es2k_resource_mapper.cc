// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "stratum/hal/lib/tdi/es2k/es2k_resource_mapper.h"

#include <memory>

#include "stratum/hal/lib/tdi/es2k/es2k_direct_pkt_mod_meter_handler.h"
#include "stratum/hal/lib/tdi/es2k/es2k_pkt_mod_meter_handler.h"

namespace stratum {
namespace hal {
namespace tdi {

void Es2kResourceMapper::RegisterResources() {
  TdiResourceMapper::RegisterResources();
  RegisterEs2kExterns();
}

void Es2kResourceMapper::RegisterEs2kExterns() {
  for (const auto& p4extern : p4info_->externs()) {
    switch (p4extern.extern_type_id()) {
      case ::p4::config::v1::P4Ids_Prefix_DIRECT_PACKET_MOD_METER:
        RegisterDirectPacketModMeters(p4extern);
        break;
      case ::p4::config::v1::P4Ids_Prefix_PACKET_MOD_METER:
        RegisterPacketModMeters(p4extern);
        break;
      default:
        ++invalid_type_ids;
        break;
    }
  }

  if (invalid_type_ids) {
    LOG(INFO) << invalid_type_ids << " invalid P4Info extern "
              << (invalid_type_ids == 1 ? "type" : "types");
  }
}

void Es2kResourceMapper::RegisterDirectPacketModMeters(
    const p4::config::v1::Extern& p4extern) {
  const auto& instances = p4extern.instances();
  if (!instances.empty()) {
    auto resource_handler = std::make_shared<Es2kDirectPktModMeterHandler>(
        tdi_sde_interface_, tdi_extern_manager_, *lock_, device_);
    for (const auto& instance : instances) {
      RegisterResource(instance.preamble(), resource_handler);
    }
  }
}

void Es2kResourceMapper::RegisterPacketModMeters(
    const p4::config::v1::Extern& p4extern) {
  const auto& instances = p4extern.instances();
  if (!instances.empty()) {
    auto resource_handler = std::make_shared<Es2kPktModMeterHandler>(
        tdi_sde_interface_, p4_info_manager_, tdi_extern_manager_, *lock_,
        device_);
    for (const auto& instance : instances) {
      RegisterResource(instance.preamble(), resource_handler);
    }
  }
}

}  // namespace tdi
}  // namespace hal
}  // namespace stratum
