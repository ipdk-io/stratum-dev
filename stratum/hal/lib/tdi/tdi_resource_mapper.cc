// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "stratum/hal/lib/tdi/tdi_resource_mapper.h"

#include "absl/memory/memory.h"
#include "stratum/hal/lib/p4/utils.h"
#include "stratum/hal/lib/tdi/tdi_direct_counter_handler.h"
#include "stratum/hal/lib/tdi/tdi_direct_meter_handler.h"
#include "stratum/hal/lib/tdi/tdi_meter_handler.h"

namespace stratum {
namespace hal {
namespace tdi {

namespace {

// Validates the resource ID.
::util::Status VerifyID(uint32 resource_id, const std::string& resource_type) {
  if (resource_id == 0) {
    return MAKE_ERROR(ERR_INVALID_P4_INFO)
           << "P4Info " << resource_type
           << " requires a non-zero ID in preamble";
  }
  return ::util::OkStatus();
}

// Validates the resource name.
::util::Status VerifyName(const std::string& name,
                          const std::string& resource_type) {
  if (name.empty()) {
    return MAKE_ERROR(ERR_INVALID_P4_INFO)
           << "P4Info " << resource_type
           << " requires a non-empty name in preamble";
  }
  return ::util::OkStatus();
}

}  // namespace

TdiResourceMapper::TdiResourceMapper()
    : tdi_sde_interface_(nullptr),
      p4_info_manager_(nullptr),
      tdi_extern_manager_(nullptr),
      p4info_(nullptr),
      lock_(nullptr),
      device_(0) {}

std::unique_ptr<TdiResourceMapper> TdiResourceMapper::CreateInstance() {
  return absl::make_unique<TdiResourceMapper>();
}

::util::Status TdiResourceMapper::Initialize(TdiSdeInterface* sde_interface,
                                             P4InfoManager* p4_info_manager,
                                             TdiExternManager* extern_manager,
                                             absl::Mutex* lock, int device) {
  tdi_sde_interface_ = sde_interface;
  p4_info_manager_ = p4_info_manager;
  tdi_extern_manager_ = extern_manager;
  p4info_ = &p4_info_manager->p4_info();
  lock_ = lock;
  device_ = device;

  RETURN_IF_ERROR(RegisterDirectCounters());
  RETURN_IF_ERROR(RegisterDirectMeters());
  RETURN_IF_ERROR(RegisterMeters());

  return ::util::OkStatus();
}

::util::Status TdiResourceMapper::RegisterDirectCounters() {
  const auto& direct_counters = p4info_->direct_counters();
  if (!direct_counters.empty()) {
    auto resource_handler =
        std::make_shared<TdiDirectCounterHandler>(p4_info_manager_);
    for (const auto& counter : direct_counters) {
      return RegisterResource(counter.preamble(), "DirectCounter",
                              resource_handler);
    }
  }
  return ::util::OkStatus();
}

::util::Status TdiResourceMapper::RegisterDirectMeters() {
  const auto& direct_meters = p4info_->direct_meters();
  if (!direct_meters.empty()) {
    auto resource_handler =
        std::make_shared<TdiDirectMeterHandler>(p4_info_manager_);
    for (const auto& meter : direct_meters) {
      return RegisterResource(meter.preamble(), "DirectMeter",
                              resource_handler);
    }
  }
  return ::util::OkStatus();
}

::util::Status TdiResourceMapper::RegisterMeters() {
  const auto& meters = p4info_->meters();
  if (!meters.empty()) {
    auto resource_handler = std::make_shared<TdiMeterHandler>(
        tdi_sde_interface_, p4_info_manager_, *lock_, device_);
    for (const auto& meter : meters) {
      return RegisterResource(meter.preamble(), "Meter", resource_handler);
    }
  }
  return ::util::OkStatus();
}

::util::Status TdiResourceMapper::RegisterResource(
    const ::p4::config::v1::Preamble& preamble,
    const std::string& resource_type, ResourceHandler resource_handler) {
  // Validate preamble.
  auto retval = VerifyID(preamble.id(), resource_type);
  auto status = VerifyName(preamble.name(), resource_type);
  APPEND_STATUS_IF_ERROR(retval, status);
  if (!retval.ok()) return retval;

  // Add entry to resource map.
  return AddMapEntry(preamble.id(), resource_type, resource_handler);
}

::util::Status TdiResourceMapper::AddMapEntry(
    uint32 resource_id, const std::string& resource_type,
    ResourceHandler resource_handler) {
  auto result = resource_handler_map_.emplace(resource_id, resource_handler);
  if (!result.second) {
    return MAKE_ERROR(ERR_INVALID_P4_INFO)
           << "P4Info " << resource_type << " ID "
           << PrintP4ObjectID(resource_id) << " is not unique";
  }
  return ::util::OkStatus();
}

}  // namespace tdi
}  // namespace hal
}  // namespace stratum
