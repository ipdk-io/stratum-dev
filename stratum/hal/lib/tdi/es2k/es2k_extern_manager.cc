// Copyright 2023-2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "stratum/hal/lib/tdi/es2k/es2k_extern_manager.h"

#include "stratum/glue/logging.h"
#include "stratum/glue/status/status_macros.h"
#include "stratum/hal/lib/tdi/es2k/es2k_direct_pkt_mod_meter_handler.h"
#include "stratum/hal/lib/tdi/es2k/es2k_pkt_mod_meter_handler.h"
#include "stratum/hal/lib/p4/utils.h"

namespace stratum {
namespace hal {
namespace tdi {

Es2kExternManager::Es2kExternManager()
    : pkt_mod_meter_map_("PacketModMeter"),
      direct_pkt_mod_meter_map_("DirectPacketModMeter") {}

::util::Status Es2kExternManager::Initialize(TdiSdeInterface* sde_interface,
                                             P4InfoManager* p4_info_manager,
                                             absl::Mutex* lock, int device) {
  RET_CHECK(sde_interface != nullptr);
  RET_CHECK(p4_info_manager != nullptr);
  RET_CHECK(lock != nullptr);

  params_.sde_interface = sde_interface;
  params_.p4_info_manager = p4_info_manager;
  params_.lock = lock;
  params_.device = device;

  return ::util::OkStatus();
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
          ++stats_.unknown_extern_id;
          LOG(INFO) << "Unrecognized P4Extern type: "
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
    // Instantiate a PktModMeter resource handler.
    ResourceHandler handler =
        std::make_shared<Es2kPktModMeterHandler>(params_, this);

    for (const auto& extern_instance : instances) {
      const auto& preamble = extern_instance.preamble();
      if (!ValidatePreamble(preamble, handler).ok()) continue;

      // Add an entry to the resource map.
      if (!RegisterResource(preamble, handler).ok()) continue;

      // Create a PacketModMeter configuration object.
      ::idpf::PacketModMeter pkt_mod_meter;
      *pkt_mod_meter.mutable_preamble() = preamble;

      p4::config::v1::MeterSpec meter_spec;
      meter_spec.set_unit(p4::config::v1::MeterSpec::PACKETS);
      *pkt_mod_meter.mutable_spec() = meter_spec;

      pkt_mod_meter.set_size(1024);
      pkt_mod_meter.set_index_width(20);

      // Add PacketModMeter object to list.
      meter_objects_.Add(std::move(pkt_mod_meter));
    }

    // Update P4InfoManager maps.
    pkt_mod_meter_map_.BuildMaps(meter_objects_, preamble_cb);
  }
}

void Es2kExternManager::RegisterDirectPacketModMeters(
    const p4::config::v1::Extern& p4extern,
    const PreambleCallback& preamble_cb) {
  const auto& instances = p4extern.instances();
  if (!instances.empty()) {
    // Instantiate a DirectPacketModMeter resource handler.
    ResourceHandler handler =
        std::make_shared<Es2kDirectPktModMeterHandler>(this);

    for (const auto& extern_instance : instances) {
      const auto& preamble = extern_instance.preamble();
      if (!ValidatePreamble(preamble, handler).ok()) continue;

      // Add an entry to the resource map.
      if (!RegisterResource(preamble, handler).ok()) continue;

      // Create a DirectPacketModMeter configuration object.
      ::idpf::DirectPacketModMeter direct_pkt_mod_meter;
      *direct_pkt_mod_meter.mutable_preamble() = preamble;

      p4::config::v1::MeterSpec meter_spec;
      meter_spec.set_unit(p4::config::v1::MeterSpec::BYTES);
      *direct_pkt_mod_meter.mutable_spec() = meter_spec;

      // Add configuration object to list.
      direct_meter_objects_.Add(std::move(direct_pkt_mod_meter));
    }

    // Update P4InfoManager maps.
    direct_pkt_mod_meter_map_.BuildMaps(direct_meter_objects_, preamble_cb);
  }
}

::util::Status Es2kExternManager::ValidatePreamble(
    const ::p4::config::v1::Preamble& preamble,
    const ResourceHandler& handler) {
  if (preamble.id() == 0) {
    ++stats_.zero_resource_id;
    return MAKE_ERROR(ERR_INVALID_P4_INFO)
           << "P4Extern " << handler->resource_type()
           << " requires a non-zero ID in preamble";
  }

  if (preamble.name().empty()) {
    ++stats_.empty_resource_name;
    return MAKE_ERROR(ERR_INVALID_P4_INFO)
           << "P4Extern " << handler->resource_type()
           << " requires a non-empty name in preamble";
  }
  return ::util::OkStatus();
}

::util::Status Es2kExternManager::RegisterResource(
    const ::p4::config::v1::Preamble& preamble,
    const ResourceHandler& handler) {
  auto result = resource_handler_map_.emplace(preamble.id(), handler);
  if (!result.second) {
    ++stats_.duplicate_resource_id;
    return MAKE_ERROR(ERR_INVALID_P4_INFO)
           << "Duplicate P4 object ID " << PrintP4ObjectID(preamble.id());
  }
  return ::util::OkStatus();
}

}  // namespace tdi
}  // namespace hal
}  // namespace stratum
