// Copyright 2020-present Open Networking Foundation
// Copyright 2022-2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// TDI Direct Meter resource handler (implementation).

#include "stratum/hal/lib/tdi/tdi_direct_meter_handler.h"

#include "stratum/glue/status/status_macros.h"
#include "stratum/hal/lib/tdi/tdi_table_helpers.h"

namespace stratum {
namespace hal {
namespace tdi {

using namespace stratum::hal::tdi::helpers;

::util::Status TdiDirectMeterHandler::BuildTableData(
    const ::p4::v1::TableEntry& table_entry,
    TdiSdeInterface::TableDataInterface* table_data, uint32 resource_id) {
  if (table_entry.has_meter_config()) {
    ASSIGN_OR_RETURN(auto meter,
                     p4_info_manager_->FindDirectMeterByID(resource_id));

    bool units_in_packets;
    RETURN_IF_ERROR(GetMeterUnitsInPackets(meter, units_in_packets));

    RETURN_IF_ERROR(table_data->SetMeterConfig(
        units_in_packets, table_entry.meter_config().cir(),
        table_entry.meter_config().cburst(), table_entry.meter_config().pir(),
        table_entry.meter_config().pburst()));
  }
  return ::util::OkStatus();
}

::util::StatusOr<::p4::v1::TableEntry> TdiDirectMeterHandler::BuildP4TableEntry(
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

::util::StatusOr<::p4::v1::DirectMeterEntry>
TdiDirectMeterHandler::ReadDirectMeterEntry(
    const TdiSdeInterface::TableDataInterface* table_data,
    const ::p4::v1::TableEntry& table_entry, ::p4::v1::DirectMeterEntry& result,
    uint32 resource_id) {
  if (table_entry.has_meter_config()) {
    // TODO(derek): repeated logic
    uint64 cir = 0;
    uint64 cburst = 0;
    uint64 pir = 0;
    uint64 pburst = 0;

    RETURN_IF_ERROR(
        table_data->GetMeterConfig(false, &cir, &cburst, &pir, &pburst));

    result.mutable_config()->set_cir(static_cast<int64>(cir));
    result.mutable_config()->set_cburst(static_cast<int64>(cburst));
    result.mutable_config()->set_pir(static_cast<int64>(pir));
    result.mutable_config()->set_pburst(static_cast<int64>(pburst));
  }
  return ::util::OkStatus();
}

::util::Status TdiDirectMeterHandler::WriteDirectMeterEntry(
    const ::p4::v1::DirectMeterEntry& direct_meter_entry,
    TdiSdeInterface::TableDataInterface* table_data, uint32 resource_id) {
  ASSIGN_OR_RETURN(auto meter,
                   p4_info_manager_->FindDirectMeterByID(resource_id));

  bool units_in_packets;
  RETURN_IF_ERROR(GetMeterUnitsInPackets(meter, units_in_packets));

  RETURN_IF_ERROR(table_data->SetMeterConfig(
      units_in_packets, direct_meter_entry.config().cir(),
      direct_meter_entry.config().cburst(), direct_meter_entry.config().pir(),
      direct_meter_entry.config().pburst()));

  return ::util::OkStatus();
}

}  // namespace tdi
}  // namespace hal
}  // namespace stratum
