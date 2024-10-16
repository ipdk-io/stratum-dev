// Copyright 2020-present Open Networking Foundation
// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ES2K DirectPktModMeter resource handler (implementation).

#include "stratum/hal/lib/tdi/es2k/es2k_direct_pkt_mod_meter_handler.h"

#include "idpf/p4info.pb.h"
#include "p4/config/v1/p4info.pb.h"
#include "stratum/glue/status/status_macros.h"
#include "stratum/hal/lib/tdi/tdi_pkt_mod_meter_config.h"
#include "stratum/hal/lib/tdi/tdi_table_helpers.h"

namespace stratum {
namespace hal {
namespace tdi {

using namespace stratum::hal::tdi::helpers;

Es2kDirectPktModMeterHandler::Es2kDirectPktModMeterHandler(
    TdiSdeInterface* sde_interface, Es2kExternManager* extern_manager,
    absl::Mutex& lock, int device)
    : TdiResourceHandler("DirectPktModMeter",
                         ::p4::config::v1::P4Ids::DIRECT_PACKET_MOD_METER),
      tdi_sde_interface_(sde_interface),
      extern_manager_(extern_manager),
      lock_(lock),
      device_(device) {}

Es2kDirectPktModMeterHandler::~Es2kDirectPktModMeterHandler() {}

util::StatusOr<::p4::v1::DirectMeterEntry>
Es2kDirectPktModMeterHandler::ReadDirectMeterEntry(
    const TdiSdeInterface::TableDataInterface* table_data,
    const ::p4::v1::TableEntry& table_entry,
    ::p4::v1::DirectMeterEntry& result) {
  TdiPktModMeterConfig cfg;
  RETURN_IF_ERROR(table_data->GetPktModMeterConfig(cfg));
  SetDirectMeterEntry(result, cfg);
  return ::util::OkStatus();
}

util::Status Es2kDirectPktModMeterHandler::WriteMeterEntry(
    std::shared_ptr<TdiSdeInterface::SessionInterface> session,
    const ::p4::v1::Update::Type type, const ::p4::v1::MeterEntry& meter_entry,
    uint32 meter_id) {
  bool units_in_packets;
  {
    absl::ReaderMutexLock l(&lock_);
    ::idpf::PacketModMeter meter;
    ASSIGN_OR_RETURN(
        meter, extern_manager_->FindPktModMeterByID(meter_entry.meter_id()));
    RETURN_IF_ERROR(GetMeterUnitsInPackets(meter, units_in_packets));
  }

  absl::optional<uint32> meter_index;
  if (meter_entry.has_index()) {
    meter_index = meter_entry.index().index();
  } else {
    return MAKE_ERROR(ERR_INVALID_PARAM) << "Invalid meter entry index";
  }

  if (meter_entry.has_config()) {
    TdiPktModMeterConfig config;
    SetPktModMeterConfig(config, meter_entry);
    config.isPktModMeter = units_in_packets;

    RETURN_IF_ERROR(tdi_sde_interface_->WritePktModMeter(
        device_, session, meter_id, meter_index, config));
  }

  if (type == ::p4::v1::Update::DELETE) {
    RETURN_IF_ERROR(tdi_sde_interface_->DeletePktModMeterConfig(
        device_, session, meter_id, meter_index));
  }

  return ::util::OkStatus();
}

}  // namespace tdi
}  // namespace hal
}  // namespace stratum
