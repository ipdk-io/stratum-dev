// Copyright 2023-2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#ifndef STRATUM_HAL_LIB_TDI_ES2K_ES2K_TABLE_ANNEX_MOCK_H_
#define STRATUM_HAL_LIB_TDI_ES2K_ES2K_TABLE_ANNEX_MOCK_H_

#include "gmock/gmock.h"
#include "stratum/hal/lib/tdi/es2k/es2k_table_annex.h"

namespace stratum {
namespace hal {
namespace tdi {

class Es2kTableAnnexMock : public Es2kTableAnnex {
 public:
  MOCK_METHOD(::util::Status, Initialize,
              (TdiExternManager * tdi_extern_manager,
               TdiSdeInterface* tdi_sde_interface, absl::Mutex* lock,
               int device),
              (override));

  MOCK_METHOD(::util::Status, BuildDirPktModTableData,
              (const ::p4::v1::TableEntry& table_entry,
               TdiSdeInterface::TableDataInterface* table_data,
               uint32 resource_id),
              (override));

  MOCK_METHOD(::util::Status, ReadDirPktModMeterEntry,
              (TdiSdeInterface::TableDataInterface * table_data,
               ::p4::v1::DirectMeterEntry result),
              (override));

  MOCK_METHOD(::util::Status, ReadPktModMeterEntry,
              (std::shared_ptr<TdiSdeInterface::SessionInterface> session,
               const ::p4::v1::MeterEntry& meter_entry,
               WriterInterface<::p4::v1::ReadResponse>* writer,
               uint32 table_id),
              (override));

  MOCK_METHOD(::util::Status, WritePktModMeterEntry,
              (std::shared_ptr<TdiSdeInterface::SessionInterface> session,
               const ::p4::v1::Update::Type type,
               const ::p4::v1::MeterEntry& meter_entry, uint32 meter_id),
              (override));
};

}  // namespace tdi
}  // namespace hal
}  // namespace stratum

#endif  // STRATUM_HAL_LIB_TDI_ES2K_ES2K_TABLE_ANNEX_MOCK_H_
