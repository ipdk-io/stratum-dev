// Copyright 2019-present Barefoot Networks, Inc.
// Copyright 2022-2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ES2K-specific port methods.

#include "stratum/hal/lib/tdi/es2k/es2k_port_manager.h"

#include <algorithm>
#include <memory>
#include <ostream>
#include <stdint.h>
#include <stdio.h>
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
#include "stratum/hal/lib/tdi/macros.h"
#include "stratum/hal/lib/tdi/tdi_sde_common.h"
#include "stratum/lib/channel/channel.h"

extern "C" {
#include "bf_types/bf_types.h"
#include "bf_pal/bf_pal_port_intf.h"
}

namespace stratum {
namespace hal {
namespace tdi {

constexpr absl::Duration Es2kPortManager::kWriteTimeout;
constexpr int32 Es2kPortManager::kBfDefaultMtu;

Es2kPortManager* Es2kPortManager::singleton_ = nullptr;
ABSL_CONST_INIT absl::Mutex Es2kPortManager::init_lock_(absl::kConstInit);

Es2kPortManager::Es2kPortManager() : port_status_event_writer_(nullptr) {}

namespace {

::util::StatusOr<int> AutonegHalToBf(TriState autoneg) {
  switch (autoneg) {
    case TRI_STATE_UNKNOWN:
      return 0;
    case TRI_STATE_TRUE:
      return 1;
    case TRI_STATE_FALSE:
      return 2;
    default:
      RETURN_ERROR(ERR_INVALID_PARAM) << "Invalid autoneg state.";
  }
}

// A callback function executed in SDE port state change thread context.
bf_status_t sde_port_status_callback(
    bf_dev_id_t device, bf_dev_port_t dev_port, bool up, void* cookie) {
  absl::Time timestamp = absl::Now();
  Es2kPortManager* es2k_port_manager = Es2kPortManager::GetSingleton();
  if (!es2k_port_manager) {
    LOG(ERROR) << "Es2kPortManager singleton instance is not initialized.";
    return BF_INTERNAL_ERROR;
  }
  // Forward the event.
  auto status =
      es2k_port_manager->OnPortStatusEvent(device, dev_port, up, timestamp);

  return status.ok() ? BF_SUCCESS : BF_INTERNAL_ERROR;
}

}  // namespace

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
  // Unsupported. Returns PORT_STATE_DOWN if called.
  return PORT_STATE_DOWN;
}

::util::Status Es2kPortManager::GetPortCounters(
    int device, int port, PortCounters* counters) {
  uint64_t stats[BF_PORT_NUM_COUNTERS] = {0};

  return ::util::OkStatus();
}

::util::Status Es2kPortManager::OnPortStatusEvent(
    int device, int port, bool up, absl::Time timestamp) {
  // Create PortStatusEvent message.
  PortState state = up ? PORT_STATE_UP : PORT_STATE_DOWN;
  TdiSdeInterface::PortStatusEvent event = {device, port, state, timestamp};

  {
    absl::ReaderMutexLock l(&port_status_event_writer_lock_);
    if (!port_status_event_writer_) {
      return ::util::OkStatus();
    }
    return port_status_event_writer_->Write(event, kWriteTimeout);
  }
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

//TODO: Check with Sandeep which Add port is applicable or do we really need it since we don't add port for MEV
::util::Status Es2kPortManager::AddPort(int device, int port) {
  return ::util::OkStatus();
}

::util::Status Es2kPortManager::AddPort(
    int device, int port, uint64 speed_bps, FecMode fec_mode) {
  auto port_attrs = absl::make_unique<port_attributes_t>();
  RETURN_IF_TDI_ERROR(bf_pal_port_add(static_cast<bf_dev_id_t>(device),
                                      static_cast<bf_dev_port_t>(port),
                                      port_attrs.get()));
  return ::util::OkStatus();
}

::util::Status Es2kPortManager::DeletePort(int device, int port) {
  RETURN_IF_TDI_ERROR(bf_pal_port_del(static_cast<bf_dev_id_t>(device),
                                      static_cast<bf_dev_port_t>(port)));
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
  return BF_SUCCESS;
}

::util::Status Es2kPortManager::SetPortLoopbackMode(
    int device, int port, LoopbackState loopback_mode) {
  if (loopback_mode == LOOPBACK_STATE_UNKNOWN) {
    // Do nothing if we try to set loopback mode to the default one (UNKNOWN).
    return ::util::OkStatus();
  }
  return ::util::OkStatus();
}

//TODO: Check with Sandeep: Is this required?
::util::StatusOr<uint32> Es2kPortManager::GetPortIdFromPortKey(
    int device, const PortKey& port_key) {
  const int port = port_key.port;
  CHECK_RETURN_IF_FALSE(port >= 0)
      << "Port ID must be non-negative. Attempted to get port " << port
      << " on dev " << device << ".";

  // PortKey uses three possible values for channel:
  //     > 0: port is channelized (first channel is 1)
  //     0: port is not channelized
  //     < 0: port channel is not important (e.g. for port groups)
  // BF SDK expects the first channel to be 0
  //     Convert base-1 channel to base-0 channel if port is channelized
  //     Otherwise, port is already 0 in the non-channelized case
  const int channel =
      (port_key.channel > 0) ? port_key.channel - 1 : port_key.channel;
  CHECK_RETURN_IF_FALSE(channel >= 0)
      << "Channel must be set for port " << port << " on dev " << device << ".";

  char port_string[MAX_PORT_HDL_STRING_LEN];
  int r = snprintf(port_string, sizeof(port_string), "%d/%d", port, channel);
  CHECK_RETURN_IF_FALSE(r > 0 && r < sizeof(port_string))
      << "Failed to build port string for port " << port << " channel "
      << channel << " on dev " << device << ".";

  bf_dev_port_t dev_port;
  RETURN_IF_TDI_ERROR(bf_pal_port_str_to_dev_port_map(
      static_cast<bf_dev_id_t>(device), port_string, &dev_port));
  return static_cast<uint32>(dev_port);
}

::util::StatusOr<int> Es2kPortManager::GetPcieCpuPort(int device) {
  int port = 0;
  return port;
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
