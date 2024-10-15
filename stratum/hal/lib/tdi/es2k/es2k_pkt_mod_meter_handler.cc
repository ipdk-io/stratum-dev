// Copyright 2020-present Open Networking Foundation
// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ES2K PacketModMeter resource handler (implementation).

#include "stratum/hal/lib/tdi/es2k/es2k_pkt_mod_meter_handler.h"

#include "absl/synchronization/mutex.h"
#include "idpf/p4info.pb.h"
#include "stratum/glue/status/status_macros.h"
#include "stratum/hal/lib/tdi/tdi_table_helpers.h"

namespace stratum {
namespace hal {
namespace tdi {

using namespace stratum::hal::tdi::helpers;

Es2kPktModMeterHandler::Es2kPktModMeterHandler(
    TdiSdeInterface* sde_interface, P4InfoManager* p4_info_manager,
    TdiExternManager* tdi_extern_manager, absl::Mutex& lock, int device)
    : TdiResourceHandler("PktModMeter",
                         ::p4::config::v1::P4Ids::PACKET_MOD_METER),
      tdi_sde_interface_(sde_interface),
      p4_info_manager_(p4_info_manager),
      tdi_extern_manager_(tdi_extern_manager),
      lock_(lock),
      device_(device) {}

Es2kPktModMeterHandler::~Es2kPktModMeterHandler() {}

util::Status Es2kPktModMeterHandler::ReadMeterEntry(
    std::shared_ptr<TdiSdeInterface::SessionInterface> session,
    const ::p4::v1::MeterEntry& meter_entry,
    WriterInterface<::p4::v1::ReadResponse>* writer, uint32 table_id,
    uint32 meter_id) {
  bool units_in_packets;
  {
    absl::ReaderMutexLock l(&lock_);
    ::idpf::PacketModMeter meter;
    ASSIGN_OR_RETURN(meter, tdi_extern_manager_->FindPktModMeterByID(
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
    SetMeterEntry(result, cfg[i]);
    *resp.add_entities()->mutable_meter_entry() = result;
  }

  VLOG(1) << "ReadMeterEntry resp " << resp.DebugString();
  if (!writer->Write(resp)) {
    return MAKE_ERROR(ERR_INTERNAL) << "Write to stream for failed.";
  }

  return ::util::OkStatus();
}

util::Status Es2kPktModMeterHandler::WriteMeterEntry(
    std::shared_ptr<TdiSdeInterface::SessionInterface> session,
    const ::p4::v1::Update::Type type, const ::p4::v1::MeterEntry& meter_entry,
    uint32 meter_id) {
  bool units_in_packets;
  {
    absl::ReaderMutexLock l(&lock_);
    ::idpf::PacketModMeter meter;
    ASSIGN_OR_RETURN(meter, tdi_extern_manager_->FindPktModMeterByID(
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
    SetPktModMeterConfig(config, meter_entry);
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
