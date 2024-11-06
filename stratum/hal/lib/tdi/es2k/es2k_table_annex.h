// Copyright 2023-2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#ifndef STRATUM_HAL_LIB_TDI_ES2K_ES2K_TABLE_ANNEX_H_
#define STRATUM_HAL_LIB_TDI_ES2K_ES2K_TABLE_ANNEX_H_

#include <memory>

#include "absl/memory/memory.h"
#include "p4/v1/p4runtime.pb.h"
#include "stratum/glue/status/status.h"
#include "stratum/hal/lib/tdi/es2k/es2k_extern_manager.h"
#include "stratum/hal/lib/tdi/tdi_sde_interface.h"
#include "stratum/hal/lib/tdi/tdi_table_annex.h"

namespace stratum {
namespace hal {
namespace tdi {

// The Es2kTableAnnex class provides ES2K-specific support for
// TdiTableManager.
class Es2kTableAnnex : public TdiTableAnnex {
 public:
  Es2kTableAnnex()
      : es2k_extern_manager_(nullptr),
        tdi_sde_interface_(nullptr),
        lock_(nullptr),
        device_(0) {}

  virtual ~Es2kTableAnnex() = default;

  // Called by Es2kTargetFactory.
  static std::unique_ptr<Es2kTableAnnex> CreateInstance() {
    return absl::make_unique<Es2kTableAnnex>();
  }

  ::util::Status Initialize(TdiExternManager* tdi_extern_manager,
                            TdiSdeInterface* tdi_sde_interface,
                            absl::Mutex* lock, int device) override;

  // Supports BuildTableData() on a DirectPacketModMeter.
  ::util::Status BuildDirPktModTableData(
      const ::p4::v1::TableEntry& table_entry,
      TdiSdeInterface::TableDataInterface* table_data,
      uint32 resource_id) override;

  // Supports ReadDirectMeterEntry on a DirectPacketModMeter.
  ::util::Status ReadDirPktModMeterEntry(
      TdiSdeInterface::TableDataInterface* table_data,
      ::p4::v1::DirectMeterEntry result) override;

  // Supports ReadMeterEntry on a PacketModMeter.
  ::util::Status ReadPktModMeterEntry(
      std::shared_ptr<TdiSdeInterface::SessionInterface> session,
      const ::p4::v1::MeterEntry& meter_entry,
      WriterInterface<::p4::v1::ReadResponse>* writer,
      uint32 table_id) override;

  // Supports WriteMeterEntry on a PacketModMeter.
  ::util::Status WritePktModMeterEntry(
      std::shared_ptr<TdiSdeInterface::SessionInterface> session,
      const ::p4::v1::Update::Type type,
      const ::p4::v1::MeterEntry& meter_entry, uint32 meter_id) override;

 protected:
  Es2kExternManager* es2k_extern_manager_;
  TdiSdeInterface* tdi_sde_interface_;
  absl::Mutex* lock_;
  int device_;
};

}  // namespace tdi
}  // namespace hal
}  // namespace stratum

#endif  // STRATUM_HAL_LIB_TDI_ES2K_ES2K_TABLE_ANNEX_H_
