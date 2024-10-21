// Copyright 2022-2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "stratum/hal/lib/tdi/es2k/es2k_table_helper.h"

#include "idpf/p4info.pb.h"
#include "stratum/glue/logging.h"
#include "stratum/hal/lib/tdi/tdi_get_meter_units.h"
#include "stratum/hal/lib/tdi/tdi_pkt_mod_meter_config.h"

namespace stratum {
namespace hal {
namespace tdi {

Es2kTableHelper::Es2kTableHelper()
    : tdi_sde_interface_(nullptr),
      es2k_extern_manager_(nullptr),
      lock_(nullptr),
      device_(0) {}

::util::Status Es2kTableHelper::Initialize(TdiExternManager* tdi_extern_manager,
                                           TdiSdeInterface* tdi_sde_interface,
                                           absl::Mutex* lock, int device) {
  auto es2kPtr = dynamic_cast<Es2kExternManager*>(tdi_extern_manager);
  RET_CHECK(es2k_extern_manager_ != nullptr);
  es2k_extern_manager_ = es2kPtr;

  tdi_sde_interface_ = tdi_sde_interface;
  lock_ = lock;
  device_ = device;

  return ::util::OkStatus();
}

::util::Status Es2kTableHelper::BuildDirPktModTableData(
    const ::p4::v1::TableEntry& table_entry,
    TdiSdeInterface::TableDataInterface* table_data, uint32 resource_id) {
  if (table_entry.has_meter_config()) {
    bool units_in_packets;
    ASSIGN_OR_RETURN(
        auto meter,
        es2k_extern_manager_->FindDirectPktModMeterByID(resource_id));
    RETURN_IF_ERROR(GetMeterUnitsInPackets(meter, units_in_packets));

    TdiPktModMeterConfig config;
    config.SetTableEntry(table_entry);
    config.isPktModMeter = units_in_packets;

    RETURN_IF_ERROR(table_data->SetPktModMeterConfig(config));
  }
  return ::util::OkStatus();
}

::util::Status Es2kTableHelper::ReadDirPktModMeterEntry(
    TdiSdeInterface::TableDataInterface* table_data,
    ::p4::v1::DirectMeterEntry& result) {
  TdiPktModMeterConfig cfg;
  RETURN_IF_ERROR(table_data->GetPktModMeterConfig(cfg));
  cfg.GetDirectMeterEntry(&result);
  return ::util::OkStatus();
}

::util::Status Es2kTableHelper::ReadPktModMeterEntry(
    std::shared_ptr<TdiSdeInterface::SessionInterface> session,
    const ::p4::v1::MeterEntry& meter_entry,
    WriterInterface<::p4::v1::ReadResponse>* writer, uint32 table_id) {
  bool units_in_packets;
  {
    absl::ReaderMutexLock l(lock_);
    ::idpf::PacketModMeter meter;
    ASSIGN_OR_RETURN(meter, es2k_extern_manager_->FindPktModMeterByID(
                                meter_entry.meter_id()));
    RETURN_IF_ERROR(GetMeterUnitsInPackets(meter, units_in_packets));
  }

  // Index 0 is a valid value and not a wildcard.
  absl::optional<uint32> optional_meter_index;
  if (meter_entry.has_index()) {
    optional_meter_index = meter_entry.index().index();
  }

  std::vector<uint32> meter_indices;
  std::vector<TdiPktModMeterConfig> cfg;

  RETURN_IF_ERROR(tdi_sde_interface_->ReadPktModMeters(
      device_, session, table_id, optional_meter_index, &meter_indices, cfg));

  ::p4::v1::ReadResponse resp;
  for (size_t i = 0; i < meter_indices.size(); ++i) {
    ::p4::v1::MeterEntry result;
    result.set_meter_id(meter_entry.meter_id());
    result.mutable_index()->set_index(meter_indices[i]);
    cfg[i].GetMeterEntry(&result);
    *resp.add_entities()->mutable_meter_entry() = result;
  }

  VLOG(1) << "ReadMeterEntry resp " << resp.DebugString();
  if (!writer->Write(resp)) {
    return MAKE_ERROR(ERR_INTERNAL) << "Write to stream for failed.";
  }
  return ::util::OkStatus();
}

::util::Status Es2kTableHelper::WritePktModMeterEntry(
    std::shared_ptr<TdiSdeInterface::SessionInterface> session,
    const ::p4::v1::Update::Type type, const ::p4::v1::MeterEntry& meter_entry,
    uint32 meter_id) {
  bool units_in_packets;
  {
    absl::ReaderMutexLock l(lock_);
    ::idpf::PacketModMeter meter;
    ASSIGN_OR_RETURN(meter, es2k_extern_manager_->FindPktModMeterByID(
                                meter_entry.meter_id()));
    RETURN_IF_ERROR(GetMeterUnitsInPackets(meter, units_in_packets));
  }

  absl::optional<uint32> meter_index;
  if (meter_entry.has_index()) {
    meter_index = meter_entry.index().index();
  } else {
    return MAKE_ERROR(ERR_INVALID_PARAM) << "Invalid meter entry index";
  }

  if (meter_entry.has_config()) {
    TdiPktModMeterConfig config;
    config.SetMeterEntry(meter_entry);
    config.isPktModMeter = units_in_packets;

    RETURN_IF_ERROR(tdi_sde_interface_->WritePktModMeter(
        device_, session, meter_id, meter_index, config));
  }

  if (type == ::p4::v1::Update::DELETE) {
    RETURN_IF_ERROR(tdi_sde_interface_->DeletePktModMeterConfig(
        device_, session, meter_id, meter_index));
  }
  return ::util::OkStatus();
}

}  // namespace tdi
}  // namespace hal
}  // namespace stratum
