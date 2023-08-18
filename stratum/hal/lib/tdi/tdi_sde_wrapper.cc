// Copyright 2019-present Barefoot Networks, Inc.
// Copyright 2022-2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// Target-agnostic SDE wrapper methods.

#include "stratum/hal/lib/tdi/tdi_sde_wrapper.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <ostream>
#include <set>
#include <utility>

#include "absl/hash/hash.h"
#include "absl/synchronization/mutex.h"
#include "absl/time/time.h"
#include "stratum/glue/integral_types.h"
#include "stratum/glue/logging.h"
#include "stratum/glue/status/status.h"
#include "stratum/glue/status/statusor.h"
#include "stratum/hal/lib/common/common.pb.h"
#include "stratum/hal/lib/tdi/macros.h"
#include "stratum/hal/lib/tdi/tdi_constants.h"
#include "stratum/hal/lib/tdi/tdi_sde_common.h"
#include "stratum/hal/lib/tdi/tdi_sde_helpers.h"
#include "stratum/lib/channel/channel.h"

DEFINE_string(tdi_sde_config_dir, "/var/run/stratum/tdi_config",
              "The dir used by the SDE to load the device configuration.");

namespace stratum {
namespace hal {
namespace tdi {

using namespace stratum::hal::tdi::helpers;

TdiSdeWrapper* TdiSdeWrapper::singleton_ = nullptr;
ABSL_CONST_INIT absl::Mutex TdiSdeWrapper::init_lock_(absl::kConstInit);

TdiSdeWrapper::TdiSdeWrapper()
    : port_status_event_writer_(nullptr),
      device_to_packet_rx_writer_(),
      tdi_info_(nullptr) {}

// Create and start an new session.
::util::StatusOr<std::shared_ptr<TdiSdeInterface::SessionInterface>>
TdiSdeWrapper::CreateSession() {
  return Session::CreateSession();
}

::util::StatusOr<std::unique_ptr<TdiSdeInterface::TableKeyInterface>>
TdiSdeWrapper::CreateTableKey(uint32 table_id) {
  ::absl::ReaderMutexLock l(&data_lock_);
  return TableKey::CreateTableKey(tdi_info_, table_id);
}

::util::StatusOr<std::unique_ptr<TdiSdeInterface::TableDataInterface>>
TdiSdeWrapper::CreateTableData(uint32 table_id, uint32 action_id) {
  ::absl::ReaderMutexLock l(&data_lock_);
  return TableData::CreateTableData(tdi_info_, table_id, action_id);
}

::util::Status TdiSdeWrapper::RegisterPacketReceiveWriter(
    int device, std::unique_ptr<ChannelWriter<std::string>> writer) {
  absl::WriterMutexLock l(&packet_rx_callback_lock_);
  device_to_packet_rx_writer_[device] = std::move(writer);
  return ::util::OkStatus();
}

::util::Status TdiSdeWrapper::UnregisterPacketReceiveWriter(int device) {
  absl::WriterMutexLock l(&packet_rx_callback_lock_);
  device_to_packet_rx_writer_.erase(device);
  return ::util::OkStatus();
}

::util::StatusOr<uint32> TdiSdeWrapper::GetTdiRtId(uint32 p4info_id) const {
  ::absl::ReaderMutexLock l(&data_lock_);
  return tdi_id_mapper_->GetTdiRtId(p4info_id);
}

::util::StatusOr<uint32> TdiSdeWrapper::GetP4InfoId(uint32 tdi_id) const {
  ::absl::ReaderMutexLock l(&data_lock_);
  return tdi_id_mapper_->GetP4InfoId(tdi_id);
}

::util::StatusOr<uint32> TdiSdeWrapper::GetActionSelectorTdiRtId(
    uint32 action_profile_id) const {
  ::absl::ReaderMutexLock l(&data_lock_);
  return tdi_id_mapper_->GetActionSelectorTdiRtId(action_profile_id);
}

::util::StatusOr<uint32> TdiSdeWrapper::GetActionProfileTdiRtId(
    uint32 action_selector_id) const {
  ::absl::ReaderMutexLock l(&data_lock_);
  return tdi_id_mapper_->GetActionProfileTdiRtId(action_selector_id);
}

TdiSdeWrapper* TdiSdeWrapper::CreateSingleton() {
  absl::WriterMutexLock l(&init_lock_);
  if (!singleton_) {
    singleton_ = new TdiSdeWrapper();
  }

  return singleton_;
}

TdiSdeWrapper* TdiSdeWrapper::GetSingleton() {
  absl::ReaderMutexLock l(&init_lock_);
  return singleton_;
}

::util::StatusOr<uint32> TdiSdeWrapper::GetTableId(
    std::string& table_name) const {
  const ::tdi::Table* table;
  if (nullptr != tdi_info_) {
    RETURN_IF_TDI_ERROR(tdi_info_->tableFromNameGet(table_name, &table));
    return (table->tableInfoGet()->idGet());
  }

  RETURN_ERROR(ERR_INTERNAL) << "Error retreiving information from TDI";
}

}  // namespace tdi
}  // namespace hal
}  // namespace stratum
