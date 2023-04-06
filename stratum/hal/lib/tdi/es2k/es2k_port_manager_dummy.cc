// Copyright 2019-present Barefoot Networks, Inc.
// Copyright 2022-2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// Dummy implementation of ES2K Port Manager.

#include "stratum/hal/lib/tdi/es2k/es2k_port_manager.h"

#include "absl/synchronization/mutex.h"
#include "stratum/glue/integral_types.h"
#include "stratum/glue/logging.h"
#include "stratum/glue/status/status.h"
#include "stratum/glue/status/status_macros.h"
#include "stratum/glue/status/statusor.h"
#include "stratum/hal/lib/common/common.pb.h"
#include "stratum/hal/lib/common/utils.h"
#include "stratum/hal/lib/tdi/macros.h"
#include "stratum/lib/channel/channel.h"

namespace stratum {
namespace hal {
namespace tdi {

constexpr absl::Duration Es2kPortManager::kWriteTimeout;
constexpr int32 Es2kPortManager::kBfDefaultMtu;

Es2kPortManager* Es2kPortManager::singleton_ = nullptr;
ABSL_CONST_INIT absl::Mutex Es2kPortManager::init_lock_(absl::kConstInit);

Es2kPortManager::Es2kPortManager() : port_status_event_writer_(nullptr) {}

Es2kPortManager* Es2kPortManager::CreateSingleton() {
  absl::WriterMutexLock l(&init_lock_);
  if (!singleton_) {
    singleton_ = new Es2kPortManager();
  }
  return singleton_;
}

Es2kPortManager* Es2kPortManager::GetSingleton() {
  absl::ReaderMutexLock l(&init_lock_);
  return singleton_;
}

::util::StatusOr<PortState> Es2kPortManager::GetPortState(int device, int port) {
  return PORT_STATE_DOWN;
}

::util::Status Es2kPortManager::GetPortCounters(
    int device, int port, PortCounters* counters) {
  return ::util::OkStatus();
}

::util::Status Es2kPortManager::OnPortStatusEvent(
    int device, int port, bool up, absl::Time timestamp) {
  return ::util::OkStatus();
}

::util::Status Es2kPortManager::RegisterPortStatusEventWriter(
    std::unique_ptr<ChannelWriter<TdiSdeInterface::PortStatusEvent>> writer) {
  absl::WriterMutexLock l(&port_status_event_writer_lock_);
  port_status_event_writer_ = std::move(writer);
  return ::util::OkStatus();
}

::util::Status Es2kPortManager::UnregisterPortStatusEventWriter() {
  absl::WriterMutexLock l(&port_status_event_writer_lock_);
  port_status_event_writer_ = nullptr;
  return ::util::OkStatus();
}

::util::Status Es2kPortManager::GetPortInfo(
    int device, int port, TargetDatapathId *target_dp_id) {
  return ::util::OkStatus();
}

::util::Status Es2kPortManager::AddPort(int device, int port) {
  return ::util::OkStatus();
}

::util::Status Es2kPortManager::AddPort(
    int device, int port, uint64 speed_bps, FecMode fec_mode) {
  return ::util::OkStatus();
}

::util::Status Es2kPortManager::DeletePort(int device, int port) {
  return ::util::OkStatus();
}

::util::Status Es2kPortManager::EnablePort(int device, int port) {
  return ::util::OkStatus();
}

::util::Status Es2kPortManager::DisablePort(int device, int port) {
  return ::util::OkStatus();
}

::util::Status Es2kPortManager::EnablePortShaping(int device, int port,
                                                  TriState enable) {
  return ::util::OkStatus();
}

::util::Status Es2kPortManager::SetPortAutonegPolicy(
    int device, int port, TriState autoneg) {
  return ::util::OkStatus();
}

::util::Status Es2kPortManager::SetPortMtu(int device, int port, int32 mtu) {
  return ::util::OkStatus();
}

bool Es2kPortManager::IsValidPort(int device, int port) {
  return true;
}

::util::Status Es2kPortManager::SetPortLoopbackMode(
    int device, int port, LoopbackState loopback_mode) {
  return ::util::OkStatus();
}

//TODO: Check with Sandeep: Is this required?
::util::StatusOr<uint32> Es2kPortManager::GetPortIdFromPortKey(
    int device, const PortKey& port_key) {
  return 43;
}

::util::StatusOr<int> Es2kPortManager::GetPcieCpuPort(int device) {
  return 1776;
}

::util::Status Es2kPortManager::SetTmCpuPort(int device, int port) {
  return ::util::OkStatus();
}

::util::Status Es2kPortManager::SetDeflectOnDropDestination(
    int device, int port, int queue) {
  return ::util::OkStatus();
}

}  // namespace tdi
}  // namespace hal
}  // namespace stratum
