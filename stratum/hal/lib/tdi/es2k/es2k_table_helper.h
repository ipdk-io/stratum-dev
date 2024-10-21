// Copyright 2022-2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#ifndef STRATUM_HAL_LIB_TDI_ES2K_ES2K_TABLE_HELPERS_H_
#define STRATUM_HAL_LIB_TDI_ES2K_ES2K_TABLE_HELPERS_H_

#include "stratum/hal/lib/tdi/es2k/es2k_extern_manager.h"
#include "stratum/hal/lib/tdi/tdi_table_helper.h"

namespace stratum {
namespace hal {
namespace tdi {

class Es2kTableHelper : public TdiTableHelper {
 public:
  Es2kTableHelper();
  virtual ~Es2kTableHelper() = default;

  // Called by TdiTargetFactory.
  static std::unique_ptr<Es2kTableHelper> CreateInstance() {
    return absl::make_unique<Es2kTableHelper>();
  }

  ::util::Status Initialize(TdiExternManager* tdi_extern_manager,
                            TdiSdeInterface* tdi_sde_interface,
                            absl::Mutex* lock, int device) override;

  ::util::Status BuildDirPktModTableData(
      const ::p4::v1::TableEntry& table_entry,
      TdiSdeInterface::TableDataInterface* table_data,
      uint32 resource_id) override;

  ::util::Status ReadDirPktModMeterEntry(
      TdiSdeInterface::TableDataInterface* table_data,
      ::p4::v1::DirectMeterEntry& result) override;

  ::util::Status ReadPktModMeterEntry(
      std::shared_ptr<TdiSdeInterface::SessionInterface> session,
      const ::p4::v1::MeterEntry& meter_entry,
      WriterInterface<::p4::v1::ReadResponse>* writer,
      uint32 table_id) override;

  ::util::Status WritePktModMeterEntry(
      std::shared_ptr<TdiSdeInterface::SessionInterface> session,
      const ::p4::v1::Update::Type type,
      const ::p4::v1::MeterEntry& meter_entry, uint32 meter_id) override;

 protected:
  TdiSdeInterface* tdi_sde_interface_;      // not owned by this class
  Es2kExternManager* es2k_extern_manager_;  // not owned by this class
  absl::Mutex* lock_;                       // not owned by this class
  int device_;
};

}  // namespace tdi
}  // namespace hal
}  // namespace stratum

#endif  // STRATUM_HAL_LIB_TDI_ES2K_ES2K_TABLE_HELPERS_H_
