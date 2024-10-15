// Copyright 2020-present Open Networking Foundation
// Copyright 2023-2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// TDI Meter resource handler (implementation).

#include "stratum/hal/lib/tdi/tdi_meter_handler.h"

#include "absl/synchronization/mutex.h"
#include "p4/config/v1/p4info.pb.h"
#include "stratum/glue/status/status_macros.h"
#include "stratum/hal/lib/tdi/tdi_table_helpers.h"

namespace stratum {
namespace hal {
namespace tdi {

using namespace stratum::hal::tdi::helpers;

TdiMeterHandler::TdiMeterHandler(TdiSdeInterface* sde_interface,
                                 P4InfoManager* p4_info_manager,
                                 absl::Mutex& lock, int device)
    : TdiResourceHandler("Meter", ::p4::config::v1::P4Ids::METER),
      tdi_sde_interface_(sde_interface),
      p4_info_manager_(p4_info_manager),
      lock_(lock),
      device_(device) {}

TdiMeterHandler::~TdiMeterHandler() {}

util::Status TdiMeterHandler::ReadMeterEntry(
    std::shared_ptr<TdiSdeInterface::SessionInterface> session,
    const ::p4::v1::MeterEntry& meter_entry,
    WriterInterface<::p4::v1::ReadResponse>* writer, uint32 table_id,
    // TODO(derek): resource_id not used.
    uint32 resource_id) {
  {
    absl::ReaderMutexLock l(&lock_);
    ASSIGN_OR_RETURN(auto meter,
                     p4_info_manager_->FindMeterByID(meter_entry.meter_id()));

    bool units_in_packets;
    RETURN_IF_ERROR(GetMeterUnitsInPackets(meter, units_in_packets));
  }

  // Index 0 is a valid value and not a wildcard.
  absl::optional<uint32> optional_meter_index;
  if (meter_entry.has_index()) {
    optional_meter_index = meter_entry.index().index();
  }

  std::vector<uint32> meter_indices;
  std::vector<uint64> cirs;
  std::vector<uint64> cbursts;
  std::vector<uint64> pirs;
  std::vector<uint64> pbursts;
  std::vector<bool> in_pps;

  RETURN_IF_ERROR(tdi_sde_interface_->ReadIndirectMeters(
      device_, session, table_id, optional_meter_index, &meter_indices, &cirs,
      &cbursts, &pirs, &pbursts, &in_pps));

  ::p4::v1::ReadResponse resp;
  for (size_t i = 0; i < meter_indices.size(); ++i) {
    ::p4::v1::MeterEntry result;
    result.set_meter_id(meter_entry.meter_id());
    result.mutable_index()->set_index(meter_indices[i]);
    result.mutable_config()->set_cir(cirs[i]);
    result.mutable_config()->set_cburst(cbursts[i]);
    result.mutable_config()->set_pir(pirs[i]);
    result.mutable_config()->set_pburst(pbursts[i]);
    *resp.add_entities()->mutable_meter_entry() = result;
  }

  VLOG(1) << "ReadMeterEntry resp " << resp.DebugString();

  if (!writer->Write(resp)) {
    return MAKE_ERROR(ERR_INTERNAL) << "Write to stream for failed.";
  }
  return ::util::OkStatus();
}

::util::Status TdiMeterHandler::WriteMeterEntry(
    std::shared_ptr<TdiSdeInterface::SessionInterface> session,
    const ::p4::v1::Update::Type type, const ::p4::v1::MeterEntry& meter_entry,
    uint32 meter_id) {
  bool units_in_packets;
  {
    absl::ReaderMutexLock l(&lock_);
    ASSIGN_OR_RETURN(auto meter,
                     p4_info_manager_->FindMeterByID(meter_entry.meter_id()));
    RETURN_IF_ERROR(GetMeterUnitsInPackets(meter, units_in_packets));
  }

  absl::optional<uint32> meter_index;
  if (meter_entry.has_index()) {
    meter_index = meter_entry.index().index();
  } else {
    return MAKE_ERROR(ERR_INVALID_PARAM) << "Invalid meter entry index";
  }

  RETURN_IF_ERROR(tdi_sde_interface_->WriteIndirectMeter(
      device_, session, meter_id, meter_index, units_in_packets,
      meter_entry.config().cir(), meter_entry.config().cburst(),
      meter_entry.config().pir(), meter_entry.config().pburst()));

  return ::util::OkStatus();
}

}  // namespace tdi
}  // namespace hal
}  // namespace stratum
