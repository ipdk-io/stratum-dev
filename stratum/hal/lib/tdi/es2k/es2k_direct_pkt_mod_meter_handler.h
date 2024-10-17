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
  Es2kDirectPktModMeterHandler(TdiSdeInterface* sde_interface,
                               Es2kExternManager* extern_manager,
                               absl::Mutex& lock, int device);

  virtual ~Es2kDirectPktModMeterHandler();

  util::Status DoReadDirectMeterEntry(
      const TdiSdeInterface::TableDataInterface* table_data,
      const ::p4::v1::TableEntry& table_entry,
      ::p4::v1::DirectMeterEntry& result) override;

  util::Status DoWriteMeterEntry(
      std::shared_ptr<TdiSdeInterface::SessionInterface> session,
      const ::p4::v1::Update::Type type,
      const ::p4::v1::MeterEntry& meter_entry, uint32 meter_id) override;

 protected:
  TdiSdeInterface* tdi_sde_interface_;  // not owned by this class
  Es2kExternManager* extern_manager_;   // not owned by this class
  absl::Mutex& lock_;                   // not owned by this class
  const int device_;
};

}  // namespace tdi
}  // namespace hal
}  // namespace stratum

#endif  // ES2K_DIRECT_PKT_MOD_METER_HANDLER_H_
