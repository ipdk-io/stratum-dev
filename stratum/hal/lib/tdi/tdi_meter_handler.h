// Copyright 2020-present Open Networking Foundation
// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// TDI Meter resource handler (interface).

#ifndef STRATUM_HAL_LIB_TDI_TDI_METER_H_
#define STRATUM_HAL_LIB_TDI_TDI_METER_H_

#include "p4/v1/p4runtime.pb.h"
#include "stratum/glue/integral_types.h"
#include "stratum/glue/status/status.h"
#include "stratum/hal/lib/p4/p4_info_manager.h"
#include "stratum/hal/lib/tdi/tdi_resource_handler.h"
#include "stratum/hal/lib/tdi/tdi_sde_interface.h"

namespace stratum {
namespace hal {
namespace tdi {

class TdiMeterHandler : public TdiResourceHandler {
 public:
  TdiMeterHandler(TdiSdeInterface* sde_interface,
                  P4InfoManager* p4_info_manager, absl::Mutex& lock,
                  int device);

  virtual ~TdiMeterHandler();

  util::Status DoReadMeterEntry(
      std::shared_ptr<TdiSdeInterface::SessionInterface> session,
      const ::p4::v1::MeterEntry& meter_entry,
      WriterInterface<::p4::v1::ReadResponse>* writer,
      uint32 table_id) override;

  ::util::Status DoWriteMeterEntry(
      std::shared_ptr<TdiSdeInterface::SessionInterface> session,
      const ::p4::v1::Update::Type type,
      const ::p4::v1::MeterEntry& meter_entry, uint32 meter_id) override;

 protected:
  TdiSdeInterface* tdi_sde_interface_;  // not owned by this class
  P4InfoManager* p4_info_manager_;      // not owned by this class
  absl::Mutex& lock_;                   // not owned by this class
  const int device_;
};

}  // namespace tdi
}  // namespace hal
}  // namespace stratum

#endif  // STRATUM_HAL_LIB_TDI_TDI_METER_H_
