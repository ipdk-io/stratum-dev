// Copyright 2020-present Open Networking Foundation
// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ES2K PktModMeter resource handler (interface).

#ifndef ES2K_PKT_MOD_METER_HANDLER_H_
#define ES2K_PKT_MOD_METER_HANDLER_H_

#include "p4/v1/p4runtime.pb.h"
#include "stratum/glue/integral_types.h"
#include "stratum/glue/status/status.h"
#include "stratum/hal/lib/p4/p4_info_manager.h"
#include "stratum/hal/lib/tdi/tdi_extern_manager.h"
#include "stratum/hal/lib/tdi/tdi_resource_handler.h"
#include "stratum/hal/lib/tdi/tdi_sde_interface.h"

namespace stratum {
namespace hal {
namespace tdi {

class Es2kPktModMeterHandler : public TdiResourceHandler {
 public:
  Es2kPktModMeterHandler(TdiSdeInterface* sde_interface,
                         P4InfoManager* p4_info_manager,
                         TdiExternManager* tdi_extern_manager,
                         absl::Mutex& lock, int device);

  virtual ~Es2kPktModMeterHandler();

  util::Status ReadMeterEntry(
      std::shared_ptr<TdiSdeInterface::SessionInterface> session,
      const ::p4::v1::MeterEntry& meter_entry,
      WriterInterface<::p4::v1::ReadResponse>* writer, uint32 table_id,
      uint32 meter_id) override;

  util::Status WriteMeterEntry(
      std::shared_ptr<TdiSdeInterface::SessionInterface> session,
      const ::p4::v1::Update::Type type,
      const ::p4::v1::MeterEntry& meter_entry, uint32 meter_id) override;

 protected:
  TdiSdeInterface* tdi_sde_interface_;    // not owned by this class
  P4InfoManager* p4_info_manager_;        // not owned by this class
  TdiExternManager* tdi_extern_manager_;  // not owned by this class
  absl::Mutex& lock_;                     // not owned by this class
  const int device_;
};

}  // namespace tdi
}  // namespace hal
}  // namespace stratum

#endif  // ES2K_PKT_MOD_METER_HANDLER_H_
