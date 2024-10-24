// Copyright 2023-2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#ifndef STRATUM_HAL_LIB_TDI_TDI_TABLE_ANNEX_H_
#define STRATUM_HAL_LIB_TDI_TDI_TABLE_ANNEX_H_

#include <memory>

#include "absl/memory/memory.h"
#include "absl/synchronization/mutex.h"
#include "p4/v1/p4runtime.pb.h"
#include "stratum/glue/integral_types.h"
#include "stratum/glue/status/status.h"
#include "stratum/hal/lib/common/writer_interface.h"
#include "stratum/hal/lib/tdi/tdi_extern_manager.h"
#include "stratum/hal/lib/tdi/tdi_sde_interface.h"

namespace stratum {
namespace hal {
namespace tdi {

// The TdiTableAnnex class allows us to abstract out portions of
// TdiTableManager that are target-specific, together with their
// dependencies. The methods all return OK status by default.
// A subclass only needs to override the methods it cares about.
class TdiTableAnnex {
 public:
  TdiTableAnnex() {}
  virtual ~TdiTableAnnex() = default;

  // Called by TdiTargetFactory.
  static std::unique_ptr<TdiTableAnnex> CreateInstance() {
    return absl::make_unique<TdiTableAnnex>();
  }

  virtual ::util::Status Initialize(TdiExternManager* tdi_extern_manager,
                                    TdiSdeInterface* tdi_sde_interface,
                                    absl::Mutex* lock, int device) {
    return ::util::OkStatus();
  }

  virtual ::util::Status BuildDirPktModTableData(
      const ::p4::v1::TableEntry& table_entry,
      TdiSdeInterface::TableDataInterface* table_data, uint32 resource_id) {
    return ::util::OkStatus();
  }

  virtual ::util::Status ReadDirPktModMeterEntry(
      TdiSdeInterface::TableDataInterface* table_data,
      ::p4::v1::DirectMeterEntry result) {
    return ::util::OkStatus();
  }

  virtual ::util::Status ReadPktModMeterEntry(
      std::shared_ptr<TdiSdeInterface::SessionInterface> session,
      const ::p4::v1::MeterEntry& meter_entry,
      WriterInterface<::p4::v1::ReadResponse>* writer,
      TdiSdeInterface::TableDataInterface* table_data, uint32 table_id) {
    return ::util::OkStatus();
  }

  virtual ::util::Status WritePktModMeterEntry(
      std::shared_ptr<TdiSdeInterface::SessionInterface> session,
      const ::p4::v1::Update::Type type,
      const ::p4::v1::MeterEntry& meter_entry, uint32 meter_id) {
    return ::util::OkStatus();
  }
};

}  // namespace tdi
}  // namespace hal
}  // namespace stratum

#endif  // STRATUM_HAL_LIB_TDI_TDI_TABLE_ANNEX_H_
