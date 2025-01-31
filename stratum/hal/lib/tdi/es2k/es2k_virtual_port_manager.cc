// Copyright 2019-present Barefoot Networks, Inc.
// Copyright 2022-2025 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ES2K-specific virtual port methods.

#include "stratum/hal/lib/tdi/es2k/es2k_virtual_port_manager.h"

#include <stdint.h>
#include <stdio.h>

#include <algorithm>
#include <memory>
#include <ostream>
#include <utility>

#include "absl/memory/memory.h"
#include "absl/synchronization/mutex.h"
#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "stratum/glue/integral_types.h"
#include "stratum/glue/logging.h"
#include "stratum/glue/status/status.h"
#include "stratum/glue/status/status_macros.h"
#include "stratum/glue/status/statusor.h"
#include "stratum/hal/lib/common/common.pb.h"
#include "stratum/hal/lib/common/utils.h"
#include "stratum/hal/lib/tdi/tdi_constants.h"
#include "stratum/hal/lib/tdi/tdi_sde_common.h"
#include "stratum/hal/lib/tdi/tdi_status.h"
#include "stratum/lib/channel/channel.h"

extern "C" {
#include "ipu_pal/port_intf.h"
#include "ipu_types/ipu_types.h"
}

#define VPORT_STATE_TABLE_NAME \
  "openconfig-virtual-ports.virtual-ports.virtual-port.state"

namespace stratum {
namespace hal {
namespace tdi {

Es2kVirtualPortManager* Es2kVirtualPortManager::singleton_ = nullptr;

#if 0
namespace {

// A callback function executed in SDE port state change thread context.
ipu_status_t sde_port_status_callback(ipu_dev_id_t device,
                                      ipu_dev_port_t dev_port, bool up,
                                      void* cookie) {
  absl::Time timestamp = absl::Now();
  Es2kVirtualPortManager* es2k_port_manager =
      Es2kVirtualPortManager::GetSingleton();
  if (!es2k_port_manager) {
    LOG(ERROR)
        << "Es2kVirtualPortManager singleton instance is not initialized.";
    return IPU_INTERNAL_ERROR;
  }
  // Forward the event.
  auto status =
      es2k_port_manager->OnPortStatusEvent(device, dev_port, up, timestamp);

  return status.ok() ? IPU_SUCCESS : IPU_INTERNAL_ERROR;
}

}  // namespace

#endif

Es2kVirtualPortManager* Es2kVirtualPortManager::CreateSingleton() {
  absl::WriterMutexLock l(&init_lock_);
  if (!singleton_) {
    singleton_ = new Es2kVirtualPortManager();
  }

  return singleton_;
}

Es2kVirtualPortManager* Es2kVirtualPortManager::GetSingleton() {
  absl::ReaderMutexLock l(&init_lock_);
  return singleton_;
}

void Es2kVirtualPortManager::SetTdiSdeInterface(TdiSdeInterface* tdi_sde_intf) {
  tdi_sde_interface_ = tdi_sde_intf;
}

void Es2kVirtualPortManager::SetTdiFixedFunctionManager(
    TdiFixedFunctionManager* tdi_fixed_func_mgr) {
  tdi_fixed_function_manager_ = tdi_fixed_func_mgr;
}

::util::StatusOr<uint32> Es2kVirtualPortManager::GetVSI(
    uint32 global_resource_id) {
  uint64 data;
  ASSIGN_OR_RETURN(auto session, tdi_sde_interface_->CreateSession());
  auto status = tdi_fixed_function_manager_->FetchVportTableData(
      session, VPORT_STATE_TABLE_NAME, global_resource_id, kVsi, &data);
  if (!status.ok()) {
    return MAKE_ERROR(ERR_AT_LEAST_ONE_OPER_FAILED)
           << "One or more read operations failed.";
  }
  return static_cast<uint32>(data);
}

::util::StatusOr<PortState> Es2kVirtualPortManager::GetPortState(
    uint32 global_resource_id) {
  uint64 data;
  ASSIGN_OR_RETURN(auto session, tdi_sde_interface_->CreateSession());
  auto status = tdi_fixed_function_manager_->FetchVportTableData(
      session, VPORT_STATE_TABLE_NAME, global_resource_id, kOperStatus, &data);
  if (!status.ok()) {
    return MAKE_ERROR(ERR_AT_LEAST_ONE_OPER_FAILED)
           << "One or more read operations failed.";
  }
  return static_cast<PortState>(data);
}

// Stratum's common.proto uses uint64 for MacAddress
::util::StatusOr<uint64> Es2kVirtualPortManager::GetMacAddress(
    uint32 global_resource_id) {
  uint64 data;
  ASSIGN_OR_RETURN(auto session, tdi_sde_interface_->CreateSession());
  auto status = tdi_fixed_function_manager_->FetchVportTableData(
      session, VPORT_STATE_TABLE_NAME, global_resource_id, kMacAddress, &data);
  if (!status.ok()) {
    return MAKE_ERROR(ERR_AT_LEAST_ONE_OPER_FAILED)
           << "One or more read operations failed.";
  }

  // Return mac-address with correct byte order
  uint64 swapped_mac = ((data & 0xFF) << 40) | ((data & 0xFF00) << 24) |
                       ((data & 0xFF0000) << 8) | ((data & 0xFF000000) >> 8) |
                       ((data & 0xFF00000000) >> 24) |
                       ((data & 0xFF0000000000) >> 40);
  return swapped_mac;
}

::util::Status Es2kVirtualPortManager::OnPortStatusEvent(int device, int port,
                                                         bool up,
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

}  // namespace tdi
}  // namespace hal
}  // namespace stratum
