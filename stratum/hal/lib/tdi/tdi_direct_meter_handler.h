// Copyright 2020-present Open Networking Foundation
// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// TDI Direct Meter resource handler (interface).

#ifndef STRATUM_HAL_LIB_TDI_TDI_DIRECT_METER_H_
#define STRATUM_HAL_LIB_TDI_TDI_DIRECT_METER_H_

#include "p4/v1/p4runtime.pb.h"
#include "stratum/glue/integral_types.h"
#include "stratum/glue/status/status.h"
#include "stratum/glue/status/statusor.h"
#include "stratum/hal/lib/p4/p4_info_manager.h"
#include "stratum/hal/lib/tdi/tdi_resource_handler.h"
#include "stratum/hal/lib/tdi/tdi_sde_interface.h"

namespace stratum {
namespace hal {
namespace tdi {

class TdiDirectMeterHandler : public TdiResourceHandler {
 public:
  TdiDirectMeterHandler(P4InfoManager* p4_info_manager)
      : p4_info_manager_(p4_info_manager) {}

  ::util::Status BuildTableData(const ::p4::v1::TableEntry& table_entry,
                                TdiSdeInterface::TableDataInterface* table_data,
                                uint32 resource_id) override;

  ::util::StatusOr<::p4::v1::TableEntry> BuildP4TableEntry(
      const TdiSdeInterface::TableDataInterface* table_data,
      const ::p4::v1::TableEntry& table_entry, ::p4::v1::TableEntry& result,
      uint32 resource_id) override;

  ::util::StatusOr<::p4::v1::DirectMeterEntry> ReadDirectMeterEntry(
      const TdiSdeInterface::TableDataInterface* table_data,
      const ::p4::v1::TableEntry& table_entry,
      ::p4::v1::DirectMeterEntry& result) override;

  ::util::Status WriteDirectMeterEntry(
      const ::p4::v1::DirectMeterEntry& direct_meter_entry,
      TdiSdeInterface::TableDataInterface* table_data,
      uint32 resource_id) override;

 protected:
  P4InfoManager* p4_info_manager_;
};

}  // namespace tdi
}  // namespace hal
}  // namespace stratum

#endif  // STRATUM_HAL_LIB_TDI_TDI_DIRECT_METER_H_
