// Copyright 2020-present Open Networking Foundation
// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ES2K DirectPktModMeter resource handler (interface).

#ifndef ES2K_DIRECT_PKT_MOD_METER_HANDLER_H_
#define ES2K_DIRECT_PKT_MOD_METER_HANDLER_H_

#include "absl/synchronization/mutex.h"
#include "p4/v1/p4runtime.pb.h"
#include "stratum/glue/integral_types.h"
#include "stratum/glue/status/status.h"
#include "stratum/glue/status/statusor.h"
#include "stratum/hal/lib/tdi/es2k/es2k_extern_manager.h"
#include "stratum/hal/lib/tdi/tdi_resource_handler.h"
#include "stratum/hal/lib/tdi/tdi_sde_interface.h"

namespace stratum {
namespace hal {
namespace tdi {

class Es2kDirectPktModMeterHandler : public TdiResourceHandler {
 public:
  Es2kDirectPktModMeterHandler(Es2kExternManager* extern_manager);

  virtual ~Es2kDirectPktModMeterHandler();

  ::util::Status DoBuildTableData(
      const ::p4::v1::TableEntry& table_entry,
      TdiSdeInterface::TableDataInterface* table_data,
      uint32 resource_id) override;

  util::Status DoReadDirectMeterEntry(
      const TdiSdeInterface::TableDataInterface* table_data,
      const ::p4::v1::TableEntry& table_entry,
      ::p4::v1::DirectMeterEntry& result) override;

 protected:
  Es2kExternManager* extern_manager_;  // not owned by this class
};

}  // namespace tdi
}  // namespace hal
}  // namespace stratum

#endif  // ES2K_DIRECT_PKT_MOD_METER_HANDLER_H_
