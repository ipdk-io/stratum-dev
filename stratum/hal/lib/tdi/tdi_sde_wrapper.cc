// Copyright 2019-present Barefoot Networks, Inc.
// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// Target-agnostic SDE wrapper methods.

#include "stratum/hal/lib/tdi/tdi_sde_wrapper.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <set>
#include <utility>

#include "absl/hash/hash.h"
#include "absl/strings/match.h"
#include "absl/synchronization/mutex.h"
#include "absl/time/time.h"

#include "stratum/glue/integral_types.h"
#include "stratum/glue/logging.h"
#include "stratum/glue/status/status.h"
#include "stratum/glue/status/statusor.h"
#include "stratum/hal/lib/common/common.pb.h"
#include "stratum/hal/lib/p4/utils.h"
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

constexpr absl::Duration TdiSdeWrapper::kWriteTimeout;
constexpr int32 TdiSdeWrapper::kBfDefaultMtu;

TdiSdeWrapper* TdiSdeWrapper::singleton_ = nullptr;
ABSL_CONST_INIT absl::Mutex TdiSdeWrapper::init_lock_(absl::kConstInit);

TdiSdeWrapper::TdiSdeWrapper() : port_status_event_writer_(nullptr) {}

::util::Status TdiSdeWrapper::OnPortStatusEvent(int device, int port, bool up,
                                               absl::Time timestamp) {
  // Create PortStatusEvent message.
  PortState state = up ? PORT_STATE_UP : PORT_STATE_DOWN;
  PortStatusEvent event = {device, port, state, timestamp};

  {
    absl::ReaderMutexLock l(&port_status_event_writer_lock_);
    if (!port_status_event_writer_) {
      return ::util::OkStatus();
    }
    return port_status_event_writer_->Write(event, kWriteTimeout);
  }
}

::util::Status TdiSdeWrapper::UnregisterPortStatusEventWriter() {
  absl::WriterMutexLock l(&port_status_event_writer_lock_);
  port_status_event_writer_ = nullptr;
  return ::util::OkStatus();
}

// Create and start an new session.
::util::StatusOr<std::shared_ptr<TdiSdeInterface::SessionInterface>>
TdiSdeWrapper::CreateSession() {
  return Session::CreateSession();
}

::util::StatusOr<std::unique_ptr<TdiSdeInterface::TableKeyInterface>>
TdiSdeWrapper::CreateTableKey(int table_id) {
  ::absl::ReaderMutexLock l(&data_lock_);
  return TableKey::CreateTableKey(tdi_info_, table_id);
}

::util::StatusOr<std::unique_ptr<TdiSdeInterface::TableDataInterface>>
TdiSdeWrapper::CreateTableData(int table_id, int action_id) {
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

::util::Status TdiSdeWrapper::WriteCloneSession(
    int dev_id, std::shared_ptr<TdiSdeInterface::SessionInterface> session,
    uint32 session_id, int egress_port, int cos, int max_pkt_len, bool insert) {
  auto real_session = std::dynamic_pointer_cast<Session>(session);
  CHECK_RETURN_IF_FALSE(real_session);

  const ::tdi::Table* table;
  const ::tdi::Device *device = nullptr;
  const ::tdi::DataFieldInfo *dataFieldInfo;
  ::tdi::DevMgr::getInstance().deviceGet(dev_id, &device);
  std::unique_ptr<::tdi::Target> dev_tgt;
  device->createTarget(&dev_tgt);

  ::tdi::Flags *flags = new ::tdi::Flags(0);
  RETURN_IF_TDI_ERROR(
      tdi_info_->tableFromNameGet(kMirrorConfigTable, &table));
  std::unique_ptr<::tdi::TableKey> table_key;
  std::unique_ptr<::tdi::TableData> table_data;
  RETURN_IF_TDI_ERROR(table->keyAllocate(&table_key));
  tdi_id_t action_id;
  dataFieldInfo = table->tableInfoGet()->dataFieldGet("$normal");
  RETURN_IF_NULL(dataFieldInfo);
  action_id = dataFieldInfo->idGet();
  RETURN_IF_TDI_ERROR(table->dataAllocate(action_id, &table_data));

  // Key: $sid
  RETURN_IF_ERROR(SetFieldExact(table_key.get(), "$sid", session_id));
  // Data: $direction
  RETURN_IF_ERROR(SetField(table_data.get(), "$direction", "BOTH"));
  // Data: $session_enable
  RETURN_IF_ERROR(SetFieldBool(table_data.get(), "$session_enable", true));
  // Data: $ucast_egress_port
  RETURN_IF_ERROR(
      SetField(table_data.get(), "$ucast_egress_port", egress_port));
  // Data: $ucast_egress_port_valid
  RETURN_IF_ERROR(
      SetFieldBool(table_data.get(), "$ucast_egress_port_valid", true));
  // Data: $ingress_cos
  RETURN_IF_ERROR(SetField(table_data.get(), "$ingress_cos", cos));
  // Data: $max_pkt_len
  RETURN_IF_ERROR(SetField(table_data.get(), "$max_pkt_len", max_pkt_len));

  if (insert) {
    RETURN_IF_TDI_ERROR(table->entryAdd(
        *real_session->tdi_session_, *dev_tgt, *flags, *table_key, *table_data));
  } else {
    RETURN_IF_TDI_ERROR(table->entryMod(
        *real_session->tdi_session_, *dev_tgt, *flags, *table_key, *table_data));
  }

  return ::util::OkStatus();
}

::util::Status TdiSdeWrapper::InsertCloneSession(
    int dev_id, std::shared_ptr<TdiSdeInterface::SessionInterface> session,
    uint32 session_id, int egress_port, int cos, int max_pkt_len) {
  ::absl::ReaderMutexLock l(&data_lock_);
  return WriteCloneSession(dev_id, session, session_id, egress_port, cos,
                           max_pkt_len, true);

  return ::util::OkStatus();
}

::util::Status TdiSdeWrapper::ModifyCloneSession(
    int dev_id, std::shared_ptr<TdiSdeInterface::SessionInterface> session,
    uint32 session_id, int egress_port, int cos, int max_pkt_len) {
  ::absl::ReaderMutexLock l(&data_lock_);
  return WriteCloneSession(dev_id, session, session_id, egress_port, cos,
                           max_pkt_len, false);

  return ::util::OkStatus();
}

::util::Status TdiSdeWrapper::DeleteCloneSession(
    int dev_id, std::shared_ptr<TdiSdeInterface::SessionInterface> session,
    uint32 session_id) {
  ::absl::ReaderMutexLock l(&data_lock_);
  auto real_session = std::dynamic_pointer_cast<Session>(session);
  const ::tdi::DataFieldInfo *dataFieldInfo;
  CHECK_RETURN_IF_FALSE(real_session);

  const ::tdi::Table* table;
  RETURN_IF_TDI_ERROR(
      tdi_info_->tableFromNameGet(kMirrorConfigTable, &table));
  std::unique_ptr<::tdi::TableKey> table_key;
  std::unique_ptr<::tdi::TableData> table_data;
  RETURN_IF_TDI_ERROR(table->keyAllocate(&table_key));
  tdi_id_t action_id;
  dataFieldInfo = table->tableInfoGet()->dataFieldGet("$normal");
  RETURN_IF_NULL(dataFieldInfo);
  action_id = dataFieldInfo->idGet();
  RETURN_IF_TDI_ERROR(table->dataAllocate(action_id, &table_data));
  // Key: $sid
  RETURN_IF_ERROR(SetFieldExact(table_key.get(), "$sid", session_id));

  const ::tdi::Device *device = nullptr;
  ::tdi::DevMgr::getInstance().deviceGet(dev_id, &device);
  std::unique_ptr<::tdi::Target> dev_tgt;
  device->createTarget(&dev_tgt);

  ::tdi::Flags *flags = new ::tdi::Flags(0);
  RETURN_IF_TDI_ERROR(table->entryDel(*real_session->tdi_session_,
                                      *dev_tgt, *flags, *table_key));

  return ::util::OkStatus();
}

::util::Status TdiSdeWrapper::GetCloneSessions(
    int dev_id, std::shared_ptr<TdiSdeInterface::SessionInterface> session,
    uint32 session_id, std::vector<uint32>* session_ids,
    std::vector<int>* egress_ports, std::vector<int>* coss,
    std::vector<int>* max_pkt_lens) {
  CHECK_RETURN_IF_FALSE(session_ids);
  CHECK_RETURN_IF_FALSE(egress_ports);
  CHECK_RETURN_IF_FALSE(coss);
  CHECK_RETURN_IF_FALSE(max_pkt_lens);
  ::absl::ReaderMutexLock l(&data_lock_);
  auto real_session = std::dynamic_pointer_cast<Session>(session);
  const ::tdi::DataFieldInfo *dataFieldInfo;
  CHECK_RETURN_IF_FALSE(real_session);

  const ::tdi::Device *device = nullptr;
  ::tdi::DevMgr::getInstance().deviceGet(dev_id, &device);
  std::unique_ptr<::tdi::Target> dev_tgt;
  device->createTarget(&dev_tgt);

  ::tdi::Flags *flags = new ::tdi::Flags(0);
  const ::tdi::Table* table;
  RETURN_IF_TDI_ERROR(
      tdi_info_->tableFromNameGet(kMirrorConfigTable, &table));
  tdi_id_t action_id;
  dataFieldInfo = table->tableInfoGet()->dataFieldGet("$normal");
  RETURN_IF_NULL(dataFieldInfo);
  action_id = dataFieldInfo->idGet();
  std::vector<std::unique_ptr<::tdi::TableKey>> keys;
  std::vector<std::unique_ptr<::tdi::TableData>> datums;
  // Is this a wildcard read?
  if (session_id != 0) {
    keys.resize(1);
    datums.resize(1);
    RETURN_IF_TDI_ERROR(table->keyAllocate(&keys[0]));
    RETURN_IF_TDI_ERROR(table->dataAllocate(action_id, &datums[0]));
    // Key: $sid
    RETURN_IF_ERROR(SetFieldExact(keys[0].get(), "$sid", session_id));
    RETURN_IF_TDI_ERROR(table->entryGet(
        *real_session->tdi_session_, *dev_tgt, *flags, *keys[0],
        datums[0].get()));
  } else {
    RETURN_IF_ERROR(GetAllEntries(real_session->tdi_session_, *dev_tgt,
                                  table, &keys, &datums));
  }

  session_ids->resize(0);
  egress_ports->resize(0);
  coss->resize(0);
  max_pkt_lens->resize(0);
  for (size_t i = 0; i < keys.size(); ++i) {
    const std::unique_ptr<::tdi::TableData>& table_data = datums[i];
    const std::unique_ptr<::tdi::TableKey>& table_key = keys[i];
    // Key: $sid
    uint32_t session_id = 0;
    RETURN_IF_ERROR(GetFieldExact(*table_key, "$sid", &session_id));
    session_ids->push_back(session_id);
    // Data: $ingress_cos
    uint64 ingress_cos;
    RETURN_IF_ERROR(GetField(*table_data, "$ingress_cos", &ingress_cos));
    coss->push_back(ingress_cos);
    // Data: $max_pkt_len
    uint64 pkt_len;
    RETURN_IF_ERROR(GetField(*table_data, "$max_pkt_len", &pkt_len));
    max_pkt_lens->push_back(pkt_len);
    // Data: $ucast_egress_port
    uint64 port;
    RETURN_IF_ERROR(GetField(*table_data, "$ucast_egress_port", &port));
    egress_ports->push_back(port);
    // Data: $session_enable
    bool session_enable;
    RETURN_IF_ERROR(GetFieldBool(*table_data, "$session_enable", &session_enable));
    CHECK_RETURN_IF_FALSE(session_enable)
        << "Found a session that is not enabled.";
    // Data: $ucast_egress_port_valid
    bool ucast_egress_port_valid;
    RETURN_IF_ERROR(GetFieldBool(*table_data, "$ucast_egress_port_valid",
                             &ucast_egress_port_valid));
    CHECK_RETURN_IF_FALSE(ucast_egress_port_valid)
        << "Found a unicase egress port that is not set valid.";
  }

  CHECK_EQ(session_ids->size(), keys.size());
  CHECK_EQ(egress_ports->size(), keys.size());
  CHECK_EQ(coss->size(), keys.size());
  CHECK_EQ(max_pkt_lens->size(), keys.size());

  return ::util::OkStatus();
}

::util::Status TdiSdeWrapper::WriteIndirectCounter(
    int dev_id, std::shared_ptr<TdiSdeInterface::SessionInterface> session,
    uint32 counter_id, int counter_index, absl::optional<uint64> byte_count,
    absl::optional<uint64> packet_count) {
  ::absl::ReaderMutexLock l(&data_lock_);
  auto real_session = std::dynamic_pointer_cast<Session>(session);
  const ::tdi::DataFieldInfo *dataFieldInfo;
  CHECK_RETURN_IF_FALSE(real_session);

  const ::tdi::Table* table;
  RETURN_IF_TDI_ERROR(tdi_info_->tableFromIdGet(counter_id, &table));

  std::unique_ptr<::tdi::TableKey> table_key;
  std::unique_ptr<::tdi::TableData> table_data;
  RETURN_IF_TDI_ERROR(table->keyAllocate(&table_key));
  RETURN_IF_TDI_ERROR(table->dataAllocate(&table_data));

  // Counter key: $COUNTER_INDEX
  RETURN_IF_ERROR(SetFieldExact(table_key.get(), kCounterIndex, counter_index));

  // Counter data: $COUNTER_SPEC_BYTES
  if (byte_count.has_value()) {
    tdi_id_t field_id;
    dataFieldInfo = table->tableInfoGet()->dataFieldGet(kCounterBytes);
    RETURN_IF_NULL(dataFieldInfo);
    field_id = dataFieldInfo->idGet();
    RETURN_IF_TDI_ERROR(table_data->setValue(field_id, byte_count.value()));
  }
  // Counter data: $COUNTER_SPEC_PKTS
  if (packet_count.has_value()) {
    tdi_id_t field_id;
    dataFieldInfo = table->tableInfoGet()->dataFieldGet(kCounterPackets);
    RETURN_IF_NULL(dataFieldInfo);
    field_id = dataFieldInfo->idGet();
    RETURN_IF_TDI_ERROR(
          table_data->setValue(field_id, packet_count.value()));
  }
  const ::tdi::Device *device = nullptr;
  ::tdi::DevMgr::getInstance().deviceGet(dev_id, &device);
  std::unique_ptr<::tdi::Target> dev_tgt;
  device->createTarget(&dev_tgt);

  ::tdi::Flags *flags = new ::tdi::Flags(0);
  if(byte_count.value() == 0 && packet_count.value() == 0) {
    LOG(INFO) << "Resetting counters";
    RETURN_IF_TDI_ERROR(table->clear(
      *real_session->tdi_session_, *dev_tgt, *flags));
  } else {
    RETURN_IF_TDI_ERROR(table->entryMod(
     *real_session->tdi_session_, *dev_tgt, *flags, *table_key, *table_data));
  }

  return ::util::OkStatus();
}

::util::Status TdiSdeWrapper::ReadIndirectCounter(
    int dev_id, std::shared_ptr<TdiSdeInterface::SessionInterface> session,
    uint32 counter_id, absl::optional<uint32> counter_index,
    std::vector<uint32>* counter_indices,
    std::vector<absl::optional<uint64>>* byte_counts,
    std::vector<absl::optional<uint64>>* packet_counts,
    absl::Duration timeout) {
  CHECK_RETURN_IF_FALSE(counter_indices);
  CHECK_RETURN_IF_FALSE(byte_counts);
  CHECK_RETURN_IF_FALSE(packet_counts);
  ::absl::ReaderMutexLock l(&data_lock_);
  auto real_session = std::dynamic_pointer_cast<Session>(session);
  CHECK_RETURN_IF_FALSE(real_session);

  const ::tdi::Device *device = nullptr;
  ::tdi::DevMgr::getInstance().deviceGet(dev_id, &device);
  std::unique_ptr<::tdi::Target> dev_tgt;
  device->createTarget(&dev_tgt);

  ::tdi::Flags *flags = new ::tdi::Flags(0);
  const ::tdi::Table* table;
  RETURN_IF_TDI_ERROR(tdi_info_->tableFromIdGet(counter_id, &table));
  std::vector<std::unique_ptr<::tdi::TableKey>> keys;
  std::vector<std::unique_ptr<::tdi::TableData>> datums;

  RETURN_IF_ERROR(DoSynchronizeCounters(dev_id, session, counter_id, timeout));

  // Is this a wildcard read?
  if (counter_index) {
    keys.resize(1);
    datums.resize(1);
    RETURN_IF_TDI_ERROR(table->keyAllocate(&keys[0]));
    RETURN_IF_TDI_ERROR(table->dataAllocate(&datums[0]));

    // Key: $COUNTER_INDEX
    RETURN_IF_ERROR(
        SetFieldExact(keys[0].get(), kCounterIndex, counter_index.value()));
    RETURN_IF_TDI_ERROR(table->entryGet(
        *real_session->tdi_session_, *dev_tgt, *flags, *keys[0],
        datums[0].get()));

  } else {
    RETURN_IF_ERROR(GetAllEntries(real_session->tdi_session_, *dev_tgt,
                                  table, &keys, &datums));
  }

  counter_indices->resize(0);
  byte_counts->resize(0);
  packet_counts->resize(0);
  for (size_t i = 0; i < keys.size(); ++i) {
    const std::unique_ptr<::tdi::TableData>& table_data = datums[i];
    const std::unique_ptr<::tdi::TableKey>& table_key = keys[i];
    // Key: $COUNTER_INDEX
    uint32_t tdi_counter_index = 0;
    RETURN_IF_ERROR(GetFieldExact(*table_key, kCounterIndex, &tdi_counter_index));
    counter_indices->push_back(tdi_counter_index);

    absl::optional<uint64> byte_count;
    absl::optional<uint64> packet_count;
    // Counter data: $COUNTER_SPEC_BYTES
    tdi_id_t field_id;

    if (table->tableInfoGet()->dataFieldGet(kCounterBytes)) {
      field_id = table->tableInfoGet()->dataFieldGet(kCounterBytes)->idGet();
      uint64 counter_bytes;
      RETURN_IF_TDI_ERROR(table_data->getValue(field_id, &counter_bytes));
      byte_count = counter_bytes;
    }
    byte_counts->push_back(byte_count);

    // Counter data: $COUNTER_SPEC_PKTS
    if (table->tableInfoGet()->dataFieldGet(kCounterPackets)) {
      field_id = table->tableInfoGet()->dataFieldGet(kCounterPackets)->idGet();
      uint64 counter_pkts;
      RETURN_IF_TDI_ERROR(table_data->getValue(field_id, &counter_pkts));
      packet_count = counter_pkts;
    }
    packet_counts->push_back(packet_count);
  }

  CHECK_EQ(counter_indices->size(), keys.size());
  CHECK_EQ(byte_counts->size(), keys.size());
  CHECK_EQ(packet_counts->size(), keys.size());

  return ::util::OkStatus();
}

namespace {
// Helper function to get the field ID of the "f1" register data field.
// TODO(max): Maybe use table name and strip off "pipe." at the beginning?
// std::string table_name;
// RETURN_IF_TDI_ERROR(table->tableNameGet(&table_name));
// RETURN_IF_TDI_ERROR(
//     table->dataFieldIdGet(absl::StrCat(table_name, ".", "f1"), &field_id));

::util::StatusOr<tdi_id_t> GetRegisterDataFieldId(
    const ::tdi::Table* table) {
  std::vector<tdi_id_t> data_field_ids;
  const ::tdi::DataFieldInfo *dataFieldInfo;
  data_field_ids = table->tableInfoGet()->dataFieldIdListGet();
  for (const auto& field_id : data_field_ids) {
    std::string field_name;
    dataFieldInfo = table->tableInfoGet()->dataFieldGet(field_id);
    RETURN_IF_NULL(dataFieldInfo);
    field_name = dataFieldInfo->nameGet();
    if (absl::EndsWith(field_name, ".f1")) {
      return field_id;
    }
  }

  RETURN_ERROR(ERR_INTERNAL) << "Could not find register data field id.";

   return ::util::OkStatus();
}
}  // namespace

::util::Status TdiSdeWrapper::WriteRegister(
    int dev_id, std::shared_ptr<TdiSdeInterface::SessionInterface> session,
    uint32 table_id, absl::optional<uint32> register_index,
    const std::string& register_data) {
  ::absl::ReaderMutexLock l(&data_lock_);
  auto real_session = std::dynamic_pointer_cast<Session>(session);
  CHECK_RETURN_IF_FALSE(real_session);

  const ::tdi::Table* table;
  RETURN_IF_TDI_ERROR(tdi_info_->tableFromIdGet(table_id, &table));

  std::unique_ptr<::tdi::TableKey> table_key;
  std::unique_ptr<::tdi::TableData> table_data;
  RETURN_IF_TDI_ERROR(table->keyAllocate(&table_key));
  RETURN_IF_TDI_ERROR(table->dataAllocate(&table_data));

  // Register data: <register_name>.f1
  // The current bf-p4c compiler emits the fully-qualified field name, including
  // parent table and pipeline. We cannot use just "f1" as the field name.
  tdi_id_t field_id;
  ASSIGN_OR_RETURN(field_id, GetRegisterDataFieldId(table));
  size_t data_field_size_bits;
  const ::tdi::DataFieldInfo *dataFieldInfo;
  dataFieldInfo = table->tableInfoGet()->dataFieldGet(field_id);
  RETURN_IF_NULL(dataFieldInfo);
  data_field_size_bits = dataFieldInfo->sizeGet();
  // The SDE expects a string with the full width.
  std::string value = P4RuntimeByteStringToPaddedByteString(
      register_data, data_field_size_bits);
  RETURN_IF_TDI_ERROR(table_data->setValue(
      field_id, reinterpret_cast<const uint8*>(value.data()), value.size()));

  const ::tdi::Device *device = nullptr;
  ::tdi::DevMgr::getInstance().deviceGet(dev_id, &device);
  std::unique_ptr<::tdi::Target> dev_tgt;
  device->createTarget(&dev_tgt);

  ::tdi::Flags *flags = new ::tdi::Flags(0);
  if (register_index) {
    // Single index target.
    // Register key: $REGISTER_INDEX
    RETURN_IF_ERROR(
        SetFieldExact(table_key.get(), kRegisterIndex, register_index.value()));
    RETURN_IF_TDI_ERROR(table->entryMod(
        *real_session->tdi_session_, *dev_tgt, *flags,
        *table_key, *table_data));
  } else {
    // Wildcard write to all indices.
    size_t table_size;
    RETURN_IF_TDI_ERROR(table->sizeGet(*real_session->tdi_session_,
                                       *dev_tgt, *flags, &table_size));
    for (size_t i = 0; i < table_size; ++i) {
      // Register key: $REGISTER_INDEX
      RETURN_IF_ERROR(SetFieldExact(table_key.get(), kRegisterIndex, i));
      RETURN_IF_TDI_ERROR(table->entryMod(
          *real_session->tdi_session_, *dev_tgt, *flags, *table_key, *table_data));
    }
  }

  return ::util::OkStatus();
}

::util::Status TdiSdeWrapper::ReadRegisters(
    int dev_id, std::shared_ptr<TdiSdeInterface::SessionInterface> session,
    uint32 table_id, absl::optional<uint32> register_index,
    std::vector<uint32>* register_indices, std::vector<uint64>* register_values,
    absl::Duration timeout) {
  CHECK_RETURN_IF_FALSE(register_indices);
  CHECK_RETURN_IF_FALSE(register_values);
  ::absl::ReaderMutexLock l(&data_lock_);
  auto real_session = std::dynamic_pointer_cast<Session>(session);
  CHECK_RETURN_IF_FALSE(real_session);

  RETURN_IF_ERROR(SynchronizeRegisters(dev_id, session, table_id, timeout));

  const ::tdi::Device *device = nullptr;
  ::tdi::DevMgr::getInstance().deviceGet(dev_id, &device);
  std::unique_ptr<::tdi::Target> dev_tgt;
  device->createTarget(&dev_tgt);

  ::tdi::Flags *flags = new ::tdi::Flags(0);
  const ::tdi::Table* table;
  RETURN_IF_TDI_ERROR(tdi_info_->tableFromIdGet(table_id, &table));
  std::vector<std::unique_ptr<::tdi::TableKey>> keys;
  std::vector<std::unique_ptr<::tdi::TableData>> datums;

  // Is this a wildcard read?
  if (register_index) {
    keys.resize(1);
    datums.resize(1);
    RETURN_IF_TDI_ERROR(table->keyAllocate(&keys[0]));
    RETURN_IF_TDI_ERROR(table->dataAllocate(&datums[0]));

    // Key: $REGISTER_INDEX
    RETURN_IF_ERROR(
        SetFieldExact(keys[0].get(), kRegisterIndex, register_index.value()));
    RETURN_IF_TDI_ERROR(table->entryGet(
        *real_session->tdi_session_, *dev_tgt, *flags, *keys[0],
        datums[0].get()));
  } else {
    RETURN_IF_ERROR(GetAllEntries(real_session->tdi_session_, *dev_tgt,
                                  table, &keys, &datums));
  }

  register_indices->resize(0);
  register_values->resize(0);
  for (size_t i = 0; i < keys.size(); ++i) {
    const std::unique_ptr<::tdi::TableData>& table_data = datums[i];
    const std::unique_ptr<::tdi::TableKey>& table_key = keys[i];
    // Key: $REGISTER_INDEX
    uint32_t tdi_register_index = 0;
    RETURN_IF_ERROR(GetFieldExact(*table_key, kRegisterIndex, &tdi_register_index));
    register_indices->push_back(tdi_register_index);
    // Data: <register_name>.f1
    ASSIGN_OR_RETURN(auto f1_field_id, GetRegisterDataFieldId(table));

    tdi_field_data_type_e data_type;
    const ::tdi::DataFieldInfo *dataFieldInfo;
    dataFieldInfo = table->tableInfoGet()->dataFieldGet(f1_field_id);
    RETURN_IF_NULL(dataFieldInfo);
    data_type = dataFieldInfo->dataTypeGet();
    switch (data_type) {
      case TDI_FIELD_DATA_TYPE_BYTE_STREAM: {
        // Even though the data type says byte stream, the SDE can only allows
        // fetching the data in an uint64 vector with one entry per pipe.
        std::vector<uint64> register_data;
        RETURN_IF_TDI_ERROR(table_data->getValue(f1_field_id, &register_data));
        CHECK_RETURN_IF_FALSE(register_data.size() > 0);
        register_values->push_back(register_data[0]);
        break;
      }
      default:
        RETURN_ERROR(ERR_INVALID_PARAM)
            << "Unsupported register data type " << static_cast<int>(data_type)
            << " for register in table " << table_id;
    }
  }

  CHECK_EQ(register_indices->size(), keys.size());
  CHECK_EQ(register_values->size(), keys.size());

  return ::util::OkStatus();
}

::util::Status TdiSdeWrapper::WriteIndirectMeter(
    int dev_id, std::shared_ptr<TdiSdeInterface::SessionInterface> session,
    uint32 table_id, absl::optional<uint32> meter_index, bool in_pps,
    uint64 cir, uint64 cburst, uint64 pir, uint64 pburst) {
  ::absl::ReaderMutexLock l(&data_lock_);
  auto real_session = std::dynamic_pointer_cast<Session>(session);
  CHECK_RETURN_IF_FALSE(real_session);

  const ::tdi::Table* table;
  RETURN_IF_TDI_ERROR(tdi_info_->tableFromIdGet(table_id, &table));

  std::unique_ptr<::tdi::TableKey> table_key;
  std::unique_ptr<::tdi::TableData> table_data;
  RETURN_IF_TDI_ERROR(table->keyAllocate(&table_key));
  RETURN_IF_TDI_ERROR(table->dataAllocate(&table_data));

  // Meter data: $METER_SPEC_*
  if (in_pps) {
    RETURN_IF_ERROR(SetField(table_data.get(), kMeterCirPps, cir));
    RETURN_IF_ERROR(
        SetField(table_data.get(), kMeterCommitedBurstPackets, cburst));
    RETURN_IF_ERROR(SetField(table_data.get(), kMeterPirPps, pir));
    RETURN_IF_ERROR(SetField(table_data.get(), kMeterPeakBurstPackets, pburst));
  } else {
    RETURN_IF_ERROR(
        SetField(table_data.get(), kMeterCirKbps, BytesPerSecondToKbits(cir)));
    RETURN_IF_ERROR(SetField(table_data.get(), kMeterCommitedBurstKbits,
                             BytesPerSecondToKbits(cburst)));
    RETURN_IF_ERROR(
        SetField(table_data.get(), kMeterPirKbps, BytesPerSecondToKbits(pir)));
    RETURN_IF_ERROR(SetField(table_data.get(), kMeterPeakBurstKbits,
                             BytesPerSecondToKbits(pburst)));
  }

  const ::tdi::Device *device = nullptr;
  ::tdi::DevMgr::getInstance().deviceGet(dev_id, &device);
  std::unique_ptr<::tdi::Target> dev_tgt;
  device->createTarget(&dev_tgt);

  ::tdi::Flags *flags = new ::tdi::Flags(0);
  if (meter_index) {
    // Single index target.
    // Meter key: $METER_INDEX
    RETURN_IF_ERROR(
        SetFieldExact(table_key.get(), kMeterIndex, meter_index.value()));
    RETURN_IF_TDI_ERROR(table->entryMod(
        *real_session->tdi_session_, *dev_tgt, *flags, *table_key, *table_data));
  } else {
    // Wildcard write to all indices.
    size_t table_size;
    RETURN_IF_TDI_ERROR(table->sizeGet(*real_session->tdi_session_,
                                       *dev_tgt, *flags, &table_size));
    for (size_t i = 0; i < table_size; ++i) {
      // Meter key: $METER_INDEX
      RETURN_IF_ERROR(SetFieldExact(table_key.get(), kMeterIndex, i));
      RETURN_IF_TDI_ERROR(table->entryMod(
          *real_session->tdi_session_, *dev_tgt, *flags, *table_key, *table_data));
    }
  }

  return ::util::OkStatus();
}

::util::Status TdiSdeWrapper::ReadIndirectMeters(
    int dev_id, std::shared_ptr<TdiSdeInterface::SessionInterface> session,
    uint32 table_id, absl::optional<uint32> meter_index,
    std::vector<uint32>* meter_indices, std::vector<uint64>* cirs,
    std::vector<uint64>* cbursts, std::vector<uint64>* pirs,
    std::vector<uint64>* pbursts, std::vector<bool>* in_pps) {
  CHECK_RETURN_IF_FALSE(meter_indices);
  CHECK_RETURN_IF_FALSE(cirs);
  CHECK_RETURN_IF_FALSE(cbursts);
  CHECK_RETURN_IF_FALSE(pirs);
  CHECK_RETURN_IF_FALSE(pbursts);
  ::absl::ReaderMutexLock l(&data_lock_);
  auto real_session = std::dynamic_pointer_cast<Session>(session);
  CHECK_RETURN_IF_FALSE(real_session);

  const ::tdi::Device *device = nullptr;
  ::tdi::DevMgr::getInstance().deviceGet(dev_id, &device);
  std::unique_ptr<::tdi::Target> dev_tgt;
  device->createTarget(&dev_tgt);

  ::tdi::Flags *flags = new ::tdi::Flags(0);
  const ::tdi::Table* table;
  RETURN_IF_TDI_ERROR(tdi_info_->tableFromIdGet(table_id, &table));
  std::vector<std::unique_ptr<::tdi::TableKey>> keys;
  std::vector<std::unique_ptr<::tdi::TableData>> datums;

  // Is this a wildcard read?
  if (meter_index) {
    keys.resize(1);
    datums.resize(1);
    RETURN_IF_TDI_ERROR(table->keyAllocate(&keys[0]));
    RETURN_IF_TDI_ERROR(table->dataAllocate(&datums[0]));
    // Key: $METER_INDEX
    RETURN_IF_ERROR(SetFieldExact(keys[0].get(), kMeterIndex,
                    meter_index.value()));
    RETURN_IF_TDI_ERROR(table->entryGet(
        *real_session->tdi_session_, *dev_tgt, *flags, *keys[0],
        datums[0].get()));
  } else {
    RETURN_IF_ERROR(GetAllEntries(real_session->tdi_session_, *dev_tgt,
                                  table, &keys, &datums));
  }

  meter_indices->resize(0);
  cirs->resize(0);
  cbursts->resize(0);
  pirs->resize(0);
  pbursts->resize(0);
  in_pps->resize(0);
  for (size_t i = 0; i < keys.size(); ++i) {
    const std::unique_ptr<::tdi::TableData>& table_data = datums[i];
    const std::unique_ptr<::tdi::TableKey>& table_key = keys[i];
    // Key: $METER_INDEX
    uint32_t tdi_meter_index = 0;
    RETURN_IF_ERROR(GetFieldExact(*table_key, kMeterIndex, &tdi_meter_index));
    meter_indices->push_back(tdi_meter_index);

    // Data: $METER_SPEC_*
    std::vector<tdi_id_t> data_field_ids;
    data_field_ids = table->tableInfoGet()->dataFieldIdListGet();
    for (const auto& field_id : data_field_ids) {
      std::string field_name;
      const ::tdi::DataFieldInfo *dataFieldInfo;
      dataFieldInfo = table->tableInfoGet()->dataFieldGet(field_id);
      RETURN_IF_NULL(dataFieldInfo);
      field_name = dataFieldInfo->nameGet();
      if (field_name == kMeterCirKbps) {  // kbits
        uint64 cir;
        RETURN_IF_TDI_ERROR(table_data->getValue(field_id, &cir));
        cirs->push_back(KbitsToBytesPerSecond(cir));
        in_pps->push_back(false);
      } else if (field_name == kMeterCommitedBurstKbits) {
        uint64 cburst;
        RETURN_IF_TDI_ERROR(table_data->getValue(field_id, &cburst));
        cbursts->push_back(KbitsToBytesPerSecond(cburst));
      } else if (field_name == kMeterPirKbps) {
        uint64 pir;
        RETURN_IF_TDI_ERROR(table_data->getValue(field_id, &pir));
        pirs->push_back(KbitsToBytesPerSecond(pir));
      } else if (field_name == kMeterPeakBurstKbits) {
        uint64 pburst;
        RETURN_IF_TDI_ERROR(table_data->getValue(field_id, &pburst));
        pbursts->push_back(KbitsToBytesPerSecond(pburst));
      } else if (field_name == kMeterCirPps) {  // Packets
        uint64 cir;
        RETURN_IF_TDI_ERROR(table_data->getValue(field_id, &cir));
        cirs->push_back(cir);
        in_pps->push_back(true);
      } else if (field_name == kMeterCommitedBurstPackets) {
        uint64 cburst;
        RETURN_IF_TDI_ERROR(table_data->getValue(field_id, &cburst));
        cbursts->push_back(cburst);
      } else if (field_name == kMeterPirPps) {
        uint64 pir;
        RETURN_IF_TDI_ERROR(table_data->getValue(field_id, &pir));
        pirs->push_back(pir);
      } else if (field_name == kMeterPeakBurstPackets) {
        uint64 pburst;
        RETURN_IF_TDI_ERROR(table_data->getValue(field_id, &pburst));
        pbursts->push_back(pburst);
      } else {
        RETURN_ERROR(ERR_INVALID_PARAM)
            << "Unknown meter field " << field_name
            << " in meter with id " << table_id << ".";
      }
    }
  }

  CHECK_EQ(meter_indices->size(), keys.size());
  CHECK_EQ(cirs->size(), keys.size());
  CHECK_EQ(cbursts->size(), keys.size());
  CHECK_EQ(pirs->size(), keys.size());
  CHECK_EQ(pbursts->size(), keys.size());
  CHECK_EQ(in_pps->size(), keys.size());

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

::util::Status TdiSdeWrapper::SynchronizeCounters(
    int dev_id, std::shared_ptr<TdiSdeInterface::SessionInterface> session,
    uint32 table_id, absl::Duration timeout) {
  ::absl::ReaderMutexLock l(&data_lock_);
  return DoSynchronizeCounters(dev_id, session, table_id, timeout);
}

::util::Status TdiSdeWrapper::DoSynchronizeCounters(
    int dev_id, std::shared_ptr<TdiSdeInterface::SessionInterface> session,
    uint32 table_id, absl::Duration timeout) {
  auto real_session = std::dynamic_pointer_cast<Session>(session);
  CHECK_RETURN_IF_FALSE(real_session);

  const ::tdi::Table* table;
  RETURN_IF_TDI_ERROR(tdi_info_->tableFromIdGet(table_id, &table));

  const ::tdi::Device *device = nullptr;
  ::tdi::DevMgr::getInstance().deviceGet(dev_id, &device);
  std::unique_ptr<::tdi::Target> dev_tgt;
  device->createTarget(&dev_tgt);

  // Sync table counter
  std::set<tdi_operations_type_e> supported_ops;
  supported_ops = table->tableInfoGet()->operationsSupported();
  // TODO TDI comments : Uncomment this after SDE exposes counterSyncSet
#if 0
  if (supported_ops.count(static_cast<tdi_operations_type_e>(tdi_rt_operations_type_e::COUNTER_SYNC))) {
    auto sync_notifier = std::make_shared<absl::Notification>();
    std::weak_ptr<absl::Notification> weak_ref(sync_notifier);
    std::unique_ptr<::tdi::TableOperations> table_op;
    RETURN_IF_TDI_ERROR(table->operationsAllocate(
          static_cast<tdi_operations_type_e>(tdi_rt_operations_type_e::COUNTER_SYNC), &table_op));
    RETURN_IF_TDI_ERROR(table_op->counterSyncSet(
        *real_session->tdi_session_, dev_tgt,
        [table_id, weak_ref](const ::tdi::Target& dev_tgt, void* cookie) {
          if (auto notifier = weak_ref.lock()) {
            VLOG(1) << "Table counter for table " << table_id << " synced.";
            notifier->Notify();
          } else {
            VLOG(1) << "Notifier expired before table " << table_id
                    << " could be synced.";
          }
        },
        nullptr));
    RETURN_IF_TDI_ERROR(table->tableOperationsExecute(*table_op.get()));
    // Wait until sync done or timeout.
    if (!sync_notifier->WaitForNotificationWithTimeout(timeout)) {
      return MAKE_ERROR(ERR_OPER_TIMEOUT)
             << "Timeout while syncing (indirect) table counters of table "
             << table_id << ".";
    }
  }
#endif
  return ::util::OkStatus();
}

::util::Status TdiSdeWrapper::SynchronizeRegisters(
    int dev_id, std::shared_ptr<TdiSdeInterface::SessionInterface> session,
    uint32 table_id, absl::Duration timeout) {

  auto real_session = std::dynamic_pointer_cast<Session>(session);
  CHECK_RETURN_IF_FALSE(real_session);

  const ::tdi::Table* table;
  RETURN_IF_TDI_ERROR(tdi_info_->tableFromIdGet(table_id, &table));

  const ::tdi::Device *device = nullptr;
  ::tdi::DevMgr::getInstance().deviceGet(dev_id, &device);
  std::unique_ptr<::tdi::Target> dev_tgt;
  device->createTarget(&dev_tgt);

  // Sync table registers.
  // TDI comments ; its supposed to be tdi_rt_operations_type_e ??
  //const std::set<tdi_rt_operations_type_e> supported_ops;
  //supported_ops = static_cast<tdi_rt_operations_type_e>(table->tableInfoGet()->operationsSupported());

  std::set<tdi_operations_type_e> supported_ops;
  supported_ops = table->tableInfoGet()->operationsSupported();
  // TODO TDI comments : Need to uncomment this after SDE exposes registerSyncSet
#if 0
  if (supported_ops.count(static_cast<tdi_operations_type_e>(tdi_rt_operations_type_e::REGISTER_SYNC))) {
    auto sync_notifier = std::make_shared<absl::Notification>();
    std::weak_ptr<absl::Notification> weak_ref(sync_notifier);
    std::unique_ptr<::tdi::TableOperations> table_op;
    RETURN_IF_TDI_ERROR(table->operationsAllocate(
          static_cast<tdi_operations_type_e>(tdi_rt_operations_type_e::REGISTER_SYNC), &table_op));
    RETURN_IF_TDI_ERROR(table_op->registerSyncSet(
        *real_session->tdi_session_, dev_tgt,
        [table_id, weak_ref](const ::tdi::Target& dev_tgt, void* cookie) {
          if (auto notifier = weak_ref.lock()) {
            VLOG(1) << "Table registers for table " << table_id << " synced.";
            notifier->Notify();
          } else {
            VLOG(1) << "Notifier expired before table " << table_id
                    << " could be synced.";
          }
        },
        nullptr));
    RETURN_IF_TDI_ERROR(table->tableOperationsExecute(*table_op.get()));
    // Wait until sync done or timeout.
    if (!sync_notifier->WaitForNotificationWithTimeout(timeout)) {
      return MAKE_ERROR(ERR_OPER_TIMEOUT)
             << "Timeout while syncing (indirect) table registers of table "
             << table_id << ".";
    }
  }
#endif
  return ::util::OkStatus();
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

}  // namespace tdi
}  // namespace hal
}  // namespace stratum
