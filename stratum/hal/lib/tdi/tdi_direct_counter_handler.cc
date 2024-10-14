// Copyright 2020-present Open Networking Foundation
// Copyright 2022-2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// TDI Direct Counter resource handler (implementation).

#include "stratum/hal/lib/tdi/tdi_direct_counter_handler.h"

#include "stratum/glue/status/status_macros.h"
#include "stratum/hal/lib/tdi/tdi_table_helpers.h"

namespace stratum {
namespace hal {
namespace tdi {

using namespace stratum::hal::tdi::helpers;

TdiDirectCounterHandler::TdiDirectCounterHandler(P4InfoManager* p4_info_manager)
    : p4_info_manager_(p4_info_manager) {}

TdiDirectCounterHandler::~TdiDirectCounterHandler() {}

::util::Status TdiDirectCounterHandler::BuildTableData(
    const ::p4::v1::TableEntry& table_entry,
    TdiSdeInterface::TableDataInterface* table_data, uint32 resource_id) {
  if (table_entry.has_counter_data()) {
    RETURN_IF_ERROR(
        table_data->SetCounterData(table_entry.counter_data().byte_count(),
                                   table_entry.counter_data().packet_count()));
  }
  return ::util::OkStatus();
}

::util::StatusOr<::p4::v1::TableEntry>
TdiDirectCounterHandler::BuildP4TableEntry(
    const TdiSdeInterface::TableDataInterface* table_data,
    const ::p4::v1::TableEntry& table_entry, ::p4::v1::TableEntry& result,
    uint32 resource_id) {
  if (table_entry.has_meter_config()) {
    // TODO(derek): repeated logic
    ASSIGN_OR_RETURN(auto meter,
                     p4_info_manager_->FindDirectMeterByID(resource_id));

    {
      bool units_in_packets;
      RETURN_IF_ERROR(GetMeterUnitsInPackets(meter, units_in_packets));
    }

    // TODO(derek): repeated logic
    uint64 cir = 0;
    uint64 cburst = 0;
    uint64 pir = 0;
    uint64 pburst = 0;

    RETURN_IF_ERROR(
        table_data->GetMeterConfig(false, &cir, &cburst, &pir, &pburst));

    result.mutable_meter_config()->set_cir(static_cast<int64>(cir));
    result.mutable_meter_config()->set_cburst(static_cast<int64>(cburst));
    result.mutable_meter_config()->set_pir(static_cast<int64>(pir));
    result.mutable_meter_config()->set_pburst(static_cast<int64>(pburst));
  }
  return ::util::OkStatus();
}

}  // namespace tdi
}  // namespace hal
}  // namespace stratum
