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
    Es2kExternManager* extern_manager)
    : TdiResourceHandler("DirectPacketModMeter",
                         ::p4::config::v1::P4Ids::DIRECT_PACKET_MOD_METER),
      extern_manager_(extern_manager) {}

Es2kDirectPktModMeterHandler::~Es2kDirectPktModMeterHandler() {}

::util::Status Es2kDirectPktModMeterHandler::DoBuildTableData(
    const ::p4::v1::TableEntry& table_entry,
    TdiSdeInterface::TableDataInterface* table_data, uint32 resource_id) {
  if (table_entry.has_meter_config()) {
    bool units_in_packets;  // or bytes
    ASSIGN_OR_RETURN(auto meter,
                     extern_manager_->FindDirectPktModMeterByID(resource_id));
    RETURN_IF_ERROR(GetMeterUnitsInPackets(meter, units_in_packets));

    TdiPktModMeterConfig config;
    SetPktModMeterConfig(config, table_entry);
    config.isPktModMeter = units_in_packets;

    RETURN_IF_ERROR(table_data->SetPktModMeterConfig(config));
  }
  return ::util::OkStatus();
}

util::Status Es2kDirectPktModMeterHandler::DoReadDirectMeterEntry(
    const TdiSdeInterface::TableDataInterface* table_data,
    const ::p4::v1::TableEntry& table_entry,
    ::p4::v1::DirectMeterEntry& result) {
  TdiPktModMeterConfig cfg;
  RETURN_IF_ERROR(table_data->GetPktModMeterConfig(cfg));
  SetDirectMeterEntry(result, cfg);
  return ::util::OkStatus();
}

}  // namespace tdi
}  // namespace hal
}  // namespace stratum
