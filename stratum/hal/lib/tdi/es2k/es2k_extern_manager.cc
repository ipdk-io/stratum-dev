// Copyright 2023-2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "stratum/hal/lib/tdi/es2k/es2k_extern_manager.h"

#include "stratum/glue/logging.h"
#include "stratum/hal/lib/tdi/es2k/es2k_direct_pkt_mod_meter_handler.h"
#include "stratum/hal/lib/tdi/es2k/es2k_pkt_mod_meter_handler.h"

namespace stratum {
namespace hal {
namespace tdi {

Es2kExternManager::Es2kExternManager()
    : pkt_mod_meter_map_("PacketModMeter"),
      direct_pkt_mod_meter_map_("DirectPacketModMeter"),
      tdi_sde_interface_(nullptr),
      p4_info_manager_(nullptr),
      lock_(nullptr),
      device_(0) {}

void Es2kExternManager::Initialize(TdiSdeInterface* sde_interface,
                                   P4InfoManager* p4_info_manager,
                                   absl::Mutex* lock, int device) {
  tdi_sde_interface_ = sde_interface;
  p4_info_manager_ = p4_info_manager;
  lock_ = lock;
  device_ = device;
}

void Es2kExternManager::RegisterExterns(const ::p4::config::v1::P4Info& p4info,
                                        const PreambleCallback& preamble_cb) {
  if (!p4info.externs().empty()) {
    for (const auto& p4extern : p4info.externs()) {
      switch (p4extern.extern_type_id()) {
        case ::p4::config::v1::P4Ids::PACKET_MOD_METER:
          RegisterPacketModMeters(p4extern, preamble_cb);
          break;
        case ::p4::config::v1::P4Ids::DIRECT_PACKET_MOD_METER:
          RegisterDirectPacketModMeters(p4extern, preamble_cb);
          break;
        default:
          LOG(INFO) << "Unrecognized p4_info extern type: "
                    << p4extern.extern_type_id() << " (ignored)";
          break;
      }
    }
  }
}

TdiResourceHandler* Es2kExternManager::FindResourceHandler(uint32 resource_id) {
  auto iter = resource_handler_map_.find(resource_id);
  if (iter == resource_handler_map_.end()) {
    return nullptr;
  }
  return iter->second.get();
}

::util::StatusOr<const ::idpf::PacketModMeter>
Es2kExternManager::FindPktModMeterByID(uint32 meter_id) const {
  return pkt_mod_meter_map_.FindByID(meter_id);
}

::util::StatusOr<const ::idpf::PacketModMeter>
Es2kExternManager::FindPktModMeterByName(const std::string& meter_name) const {
  return pkt_mod_meter_map_.FindByName(meter_name);
}

::util::StatusOr<const ::idpf::DirectPacketModMeter>
Es2kExternManager::FindDirectPktModMeterByID(uint32 meter_id) const {
  return direct_pkt_mod_meter_map_.FindByID(meter_id);
}

::util::StatusOr<const ::idpf::DirectPacketModMeter>
Es2kExternManager::FindDirectPktModMeterByName(
    const std::string& meter_name) const {
  return direct_pkt_mod_meter_map_.FindByName(meter_name);
}

void Es2kExternManager::RegisterPacketModMeters(
    const p4::config::v1::Extern& p4extern,
    const PreambleCallback& preamble_cb) {
  const auto& instances = p4extern.instances();
  if (!instances.empty()) {
    auto resource_handler = std::make_shared<Es2kPktModMeterHandler>(
        tdi_sde_interface_, p4_info_manager_, this, *lock_, device_);

    for (const auto& extern_instance : instances) {
      RegisterResource(extern_instance.preamble(), resource_handler);

      ::idpf::PacketModMeter pkt_mod_meter;
      *pkt_mod_meter.mutable_preamble() = extern_instance.preamble();

      p4::config::v1::MeterSpec meter_spec;
      meter_spec.set_unit(p4::config::v1::MeterSpec::PACKETS);
      *pkt_mod_meter.mutable_spec() = meter_spec;

      pkt_mod_meter.set_size(1024);
      pkt_mod_meter.set_index_width(20);

      meter_objects_.Add(std::move(pkt_mod_meter));
    }
    pkt_mod_meter_map_.BuildMaps(meter_objects_, preamble_cb);
  }
}

void Es2kExternManager::RegisterDirectPacketModMeters(
    const p4::config::v1::Extern& p4extern,
    const PreambleCallback& preamble_cb) {
  const auto& instances = p4extern.instances();
  if (!instances.empty()) {
    auto resource_handler = std::make_shared<Es2kDirectPktModMeterHandler>(
        tdi_sde_interface_, this, *lock_, device_);

    for (const auto& extern_instance : instances) {
      ::idpf::DirectPacketModMeter direct_pkt_mod_meter;
      *direct_pkt_mod_meter.mutable_preamble() = extern_instance.preamble();

      p4::config::v1::MeterSpec meter_spec;
      meter_spec.set_unit(p4::config::v1::MeterSpec::BYTES);
      *direct_pkt_mod_meter.mutable_spec() = meter_spec;

      direct_meter_objects_.Add(std::move(direct_pkt_mod_meter));
    }
    direct_pkt_mod_meter_map_.BuildMaps(direct_meter_objects_, preamble_cb);
  }
}

}  // namespace tdi
}  // namespace hal
}  // namespace stratum
