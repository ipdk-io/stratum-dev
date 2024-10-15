// Copyright 2020-present Open Networking Foundation
// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#ifndef STRATUM_HAL_LIB_TDI_TDI_RESOURCE_HANDLER_
#define STRATUM_HAL_LIB_TDI_TDI_RESOURCE_HANDLER_

#include <memory>
#include <string>

#include "absl/synchronization/mutex.h"
#include "p4/v1/p4runtime.pb.h"
#include "stratum/glue/integral_types.h"
#include "stratum/glue/status/status.h"
#include "stratum/glue/status/statusor.h"
#include "stratum/hal/lib/common/writer_interface.h"
#include "stratum/hal/lib/tdi/tdi_sde_interface.h"

namespace stratum {
namespace hal {
namespace tdi {

class TdiResourceHandler {
 public:
  TdiResourceHandler(const std::string& resource_type, uint32 type_id)
      : resource_type_(resource_type), type_id_(type_id) {}
  virtual ~TdiResourceHandler() = default;

  // DirectMeter, DirectCounter, DirectPacketModMeter
  virtual ::util::Status BuildTableData(
      const ::p4::v1::TableEntry& table_entry,
      TdiSdeInterface::TableDataInterface* table_data, uint32 resource_id) {
    return ::util::OkStatus();
  }

  // DirectMeter, DirectCounter
  virtual ::util::StatusOr<::p4::v1::TableEntry> BuildP4TableEntry(
      const TdiSdeInterface::TableDataInterface* table_data,
      const ::p4::v1::TableEntry& table_entry, ::p4::v1::TableEntry& result,
      uint32 resource_id) {
    return ::util::OkStatus();
  }

  // DirectMeter
  virtual ::util::Status WriteDirectMeterEntry(
      const ::p4::v1::DirectMeterEntry& direct_meter_entry,
      TdiSdeInterface::TableDataInterface* table_data, uint32 resource_id) {
    return ::util::OkStatus();
  }

  // DirectMeter, DirectPacketModMeter
  virtual ::util::StatusOr<::p4::v1::DirectMeterEntry> ReadDirectMeterEntry(
      const TdiSdeInterface::TableDataInterface* table_data,
      const ::p4::v1::TableEntry& table_entry,
      ::p4::v1::DirectMeterEntry& result) {
    return ::util::OkStatus();
  }

  // Meter, PacketModMeter
  virtual ::util::Status ReadMeterEntry(
      std::shared_ptr<TdiSdeInterface::SessionInterface> session,
      const ::p4::v1::MeterEntry& meter_entry,
      WriterInterface<::p4::v1::ReadResponse>* writer, uint32 table_id,
      uint32 meter_id) {
    return ::util::OkStatus();
  }

  // Meter, PacketModMeter
  virtual ::util::Status WriteMeterEntry(
      std::shared_ptr<TdiSdeInterface::SessionInterface> session,
      const ::p4::v1::Update::Type type,
      const ::p4::v1::MeterEntry& meter_entry, uint32 meter_id) {
    return ::util::OkStatus();
  }

  const std::string& resource_type() const { return resource_type_; }
  uint32 type_id() const { return type_id_; }

 protected:
  const std::string resource_type_;
  const uint32 type_id_;
};

}  // namespace tdi
}  // namespace hal
}  // namespace stratum

#endif  // STRATUM_HAL_LIB_TDI_TDI_RESOURCE_HANDLER_
