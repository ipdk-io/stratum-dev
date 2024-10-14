// Copyright 2020-present Open Networking Foundation
// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// TDI Direct Counter resource handler (interface).

#ifndef STRATUM_HAL_LIB_TDI_TDI_DIRECT_COUNTER_H_
#define STRATUM_HAL_LIB_TDI_TDI_DIRECT_COUNTER_H_

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

class TdiDirectCounterHandler : public TdiResourceHandler {
 public:
  TdiDirectCounterHandler(P4InfoManager* p4_info_manager);
  virtual ~TdiDirectCounterHandler();

  ::util::Status BuildTableData(const ::p4::v1::TableEntry& table_entry,
                                TdiSdeInterface::TableDataInterface* table_data,
                                uint32 resource_id) override;

  ::util::StatusOr<::p4::v1::TableEntry> BuildP4TableEntry(
      const TdiSdeInterface::TableDataInterface* table_data,
      const ::p4::v1::TableEntry& table_entry, ::p4::v1::TableEntry& result,
      uint32 resource_id) override;

 protected:
  P4InfoManager* p4_info_manager_;
};

}  // namespace tdi
}  // namespace hal
}  // namespace stratum

#endif  // STRATUM_HAL_LIB_TDI_TDI_DIRECT_COUNTER_H_
