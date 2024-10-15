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

  RegisterResources();

  if (invalid_entities) {
    return MAKE_ERROR(ERR_INVALID_P4_INFO)
           << invalid_entities << " invalid P4 "
           << (invalid_entities == 1 ? "entity" : "entities");
  }
  return ::util::OkStatus();
}

::util::StatusOr<TdiResourceHandler*> TdiResourceMapper::FindResourceHandler(
    uint32 resource_id) {
  auto iter = resource_handler_map_.find(resource_id);
  if (iter == resource_handler_map_.end()) {
    return nullptr;
  }
  return iter->second.get();
}

void TdiResourceMapper::RegisterResources() {
  RegisterDirectCounters();
  RegisterDirectMeters();
  RegisterMeters();
}

void TdiResourceMapper::RegisterDirectCounters() {
  const auto& direct_counters = p4info_->direct_counters();
  if (!direct_counters.empty()) {
    auto resource_handler =
        std::make_shared<TdiDirectCounterHandler>(p4_info_manager_);
    for (const auto& counter : direct_counters) {
      RegisterResource(counter.preamble(), resource_handler);
    }
  }
}

void TdiResourceMapper::RegisterDirectMeters() {
  const auto& direct_meters = p4info_->direct_meters();
  if (!direct_meters.empty()) {
    auto resource_handler =
        std::make_shared<TdiDirectMeterHandler>(p4_info_manager_);
    for (const auto& meter : direct_meters) {
      RegisterResource(meter.preamble(), resource_handler);
    }
  }
}

void TdiResourceMapper::RegisterMeters() {
  const auto& meters = p4info_->meters();
  if (!meters.empty()) {
    auto resource_handler = std::make_shared<TdiMeterHandler>(
        tdi_sde_interface_, p4_info_manager_, *lock_, device_);
    for (const auto& meter : meters) {
      RegisterResource(meter.preamble(), resource_handler);
    }
  }
}

void TdiResourceMapper::RegisterResource(
    const ::p4::config::v1::Preamble& preamble,
    ResourceHandler resource_handler) {
  if (preamble.id() == 0 || preamble.name().empty()) {
    ++invalid_entities;
    return;
  }
  auto result = resource_handler_map_.emplace(preamble.id(), resource_handler);
  if (!result.second) {
    ++invalid_entities;
  }
}

}  // namespace tdi
}  // namespace hal
}  // namespace stratum
