// Copyright 2023-2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "stratum/hal/lib/tdi/es2k/es2k_extern_manager.h"

#include "stratum/glue/logging.h"

namespace stratum {
namespace hal {
namespace tdi {

Es2kExternManager::Es2kExternManager()
    : pkt_mod_meter_map_("PacketModMeter"),
      direct_pkt_mod_meter_map_("DirectPacketModMeter") {}

void Es2kExternManager::Initialize(const ::p4::config::v1::P4Info& p4info,
                                   const PreambleCallback& preamble_cb) {
  if (!p4info.externs().empty()) {
    for (const auto& p4extern : p4info.externs()) {
      switch (p4extern.extern_type_id()) {
        case ::p4::config::v1::P4Ids::PACKET_MOD_METER:
          InitPacketModMeters(p4extern, preamble_cb);
          break;
        case ::p4::config::v1::P4Ids::DIRECT_PACKET_MOD_METER:
          InitDirectPacketModMeters(p4extern, preamble_cb);
          break;
        default:
          LOG(INFO) << "Unrecognized p4_info extern type: "
                    << p4extern.extern_type_id() << " (ignored)";
          break;
      }
    }
  }
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

void Es2kExternManager::InitPacketModMeters(
    const p4::config::v1::Extern& p4extern,
    const PreambleCallback& preamble_cb) {
  const auto& extern_instances = p4extern.instances();
  for (const auto& extern_instance : extern_instances) {
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

void Es2kExternManager::InitDirectPacketModMeters(
    const p4::config::v1::Extern& p4extern,
    const PreambleCallback& preamble_cb) {
  const auto& extern_instances = p4extern.instances();
  for (const auto& extern_instance : extern_instances) {
    ::idpf::DirectPacketModMeter direct_pkt_mod_meter;
    *direct_pkt_mod_meter.mutable_preamble() = extern_instance.preamble();

    p4::config::v1::MeterSpec meter_spec;
    meter_spec.set_unit(p4::config::v1::MeterSpec::BYTES);
    *direct_pkt_mod_meter.mutable_spec() = meter_spec;

    direct_meter_objects_.Add(std::move(direct_pkt_mod_meter));
  }
  direct_pkt_mod_meter_map_.BuildMaps(direct_meter_objects_, preamble_cb);
}

}  // namespace tdi
}  // namespace hal
}  // namespace stratum
