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
DEFINE_bool(incompatible_enable_tdi_legacy_bytestring_responses, true,
            "Enables the legacy padded byte string format in P4Runtime "
            "responses for Stratum-tdi. The strings are left unchanged from "
            "the underlying SDE.");

namespace stratum {
namespace hal {
namespace tdi {

using namespace stratum::hal::tdi::helpers;

TdiSdeWrapper* TdiSdeWrapper::singleton_ = nullptr;
ABSL_CONST_INIT absl::Mutex TdiSdeWrapper::init_lock_(absl::kConstInit);

TdiSdeWrapper::TdiSdeWrapper() : port_status_event_writer_(nullptr),
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

::util::StatusOr<uint32> TdiSdeWrapper::GetTableId(std::string &table_name) const {
  const ::tdi::Table* table;
  if (nullptr != tdi_info_) {
    RETURN_IF_TDI_ERROR(tdi_info_->tableFromNameGet(table_name, &table));
    return(table->tableInfoGet()->idGet());
  }

  RETURN_ERROR(ERR_INTERNAL)
              << "Error retreiving information from TDI";
}

::util::Status TdiSdeWrapper::InitNotificationTableWithCallback(
    int dev_id, std::shared_ptr<TdiSdeInterface::SessionInterface> session,
    std::string &table_name,
    void (*ipsec_notif_cb)(uint32_t, uint32_t, bool, uint8_t, char*, bool, void*),
    void *cookie) const {

  if (nullptr == tdi_info_) {
    RETURN_ERROR(ERR_INTERNAL)
                << "Unable to initialize notification table due to TDI internal error";
  }

  auto real_session = std::dynamic_pointer_cast<Session>(session);
  CHECK_RETURN_IF_FALSE(real_session);

  const ::tdi::Table* notifTable;
  RETURN_IF_TDI_ERROR(tdi_info_->tableFromNameGet(table_name, &notifTable));
  
  // tdi_attributes_allocate
  std::unique_ptr<::tdi::TableAttributes> attributes_field;
  ::tdi::TableAttributes *attributes_object;
  RETURN_IF_TDI_ERROR(notifTable->attributeAllocate((tdi_attributes_type_e) TDI_RT_ATTRIBUTES_TYPE_IPSEC_SADB_EXPIRE_NOTIF,
                                                  &attributes_field));
  attributes_object = attributes_field.get();

  // tdi_attributes_set_values
  const uint64_t enable = 1;
  RETURN_IF_TDI_ERROR(attributes_object->setValue(
                (tdi_attributes_field_type_e) TDI_RT_ATTRIBUTES_IPSEC_SADB_EXPIRE_TABLE_FIELD_TYPE_ENABLE,
                enable));

  RETURN_IF_TDI_ERROR(attributes_object->setValue(
                (tdi_attributes_field_type_e) TDI_RT_ATTRIBUTES_IPSEC_SADB_EXPIRE_TABLE_FIELD_TYPE_CALLBACK_C,
                (uint64_t)ipsec_notif_cb));

  RETURN_IF_TDI_ERROR(attributes_object->setValue(
                (tdi_attributes_field_type_e) TDI_RT_ATTRIBUTES_IPSEC_SADB_EXPIRE_TABLE_FIELD_TYPE_COOKIE,
                (const uint64_t)cookie));

  // target & flag create
  const ::tdi::Device *device = nullptr;
  ::tdi::DevMgr::getInstance().deviceGet(dev_id, &device);
  std::unique_ptr<::tdi::Target> target;
  device->createTarget(&target);

  const auto flags = ::tdi::Flags(0);
  RETURN_IF_TDI_ERROR(notifTable->tableAttributesSet(*real_session->tdi_session_,
                                                    *target, flags,
                                                    *attributes_object));

  return ::util::OkStatus();
}

}  // namespace tdi
}  // namespace hal
}  // namespace stratum
