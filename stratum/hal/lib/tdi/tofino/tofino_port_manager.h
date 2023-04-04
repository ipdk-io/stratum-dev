// Copyright 2020-present Open Networking Foundation
// Copyright 2022-2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#ifndef STRATUM_HAL_LIB_TDI_TOFINO_TOFINO_PORT_MANAGER_H_
#define STRATUM_HAL_LIB_TDI_TOFINO_TOFINO_PORT_MANAGER_H_

#include <memory>

#include "absl/synchronization/mutex.h"
#include "stratum/hal/lib/tdi/tdi_sde_interface.h"

// Suppress clang errors
#undef LOCKS_EXCLUDED
#define LOCKS_EXCLUDED(...)

namespace stratum {
namespace hal {
namespace tdi {

class TofinoPortManager : public TdiSdeInterface::TdiPortManager {
 public:
  TofinoPortManager();
  virtual ~TofinoPortManager() {}

  // TdiPortManager public methods
  ::util::Status RegisterPortStatusEventWriter(
      std::unique_ptr<ChannelWriter<TdiSdeInterface::PortStatusEvent>> writer)
      LOCKS_EXCLUDED(port_status_event_writer_lock_);
  ::util::Status UnregisterPortStatusEventWriter()
      LOCKS_EXCLUDED(port_status_event_writer_lock_);
  ::util::Status GetPortInfo(int device, int port,
                             TargetDatapathId *target_dp_id);
  ::util::StatusOr<PortState> GetPortState(int device, int port);
  ::util::Status GetPortCounters(int device, int port,
                                 PortCounters* counters);
  ::util::StatusOr<uint32> GetPortIdFromPortKey(
      int device, const PortKey& port_key);
  bool IsValidPort(int device, int port);
  ::util::Status AddPort(int device, int port);
  ::util::Status DeletePort(int device, int port);
  ::util::Status EnablePort(int device, int port);
  ::util::Status DisablePort(int device, int port);

  // Tofino-specific methods
  virtual ::util::Status AddPort(int device, int port, uint64 speed_bps,
                                 FecMode fec_mode);
  virtual ::util::Status SetPortShapingRate(
      int device, int port, bool is_in_pps, uint32 burst_size,
      uint64 rate_per_second);
  virtual ::util::Status EnablePortShaping(int device, int port,
                                           TriState enable);
  virtual ::util::Status SetPortAutonegPolicy(int device, int port,
                                              TriState autoneg);
  virtual ::util::Status SetPortMtu(int device, int port, int32 mtu);
  virtual ::util::Status SetPortLoopbackMode(int uint, int port,
                                             LoopbackState loopback_mode);
  virtual ::util::StatusOr<int> GetPcieCpuPort(int device);
  virtual ::util::Status SetTmCpuPort(int device, int port);
  virtual ::util::Status SetDeflectOnDropDestination(int device, int port,
                                                     int queue);

  // Creates the singleton instance. Expected to be called once to initialize
  // the instance.
  static TofinoPortManager* CreateSingleton() LOCKS_EXCLUDED(init_lock_);

  // The following public functions are specific to this class. They are to be
  // called by SDE callbacks only.

  // Return the singleton instance to be used in the SDE callbacks.
  static TofinoPortManager* GetSingleton() LOCKS_EXCLUDED(init_lock_);

  // Called whenever a port status event is received from SDK. It forwards the
  // port status event to the module who registered a callback by calling
  // RegisterPortStatusEventWriter().
  ::util::Status OnPortStatusEvent(int device, int dev_port, bool up,
                                   absl::Time timestamp)
      LOCKS_EXCLUDED(port_status_event_writer_lock_);

 protected:
  // RW mutex lock for protecting the singleton instance initialization and
  // reading it back from other threads. Unlike other singleton classes, we
  // use RW lock as we need the pointer to class to be returned.
  static absl::Mutex init_lock_;

  // The singleton instance.
  static TofinoPortManager* singleton_ GUARDED_BY(init_lock_);

 private:
  // Timeout for Write() operations on port status events.
  static constexpr absl::Duration kWriteTimeout = absl::InfiniteDuration();

  // Default MTU for ports on Tofino.
  static constexpr int32 kBfDefaultMtu = 10 * 1024;  // 10K

  // RM Mutex to protect the port status writer.
  mutable absl::Mutex port_status_event_writer_lock_;

  // Writer to forward the port status change message to. It is registered
  // by chassis manager to receive SDE port status change events.
  std::unique_ptr<ChannelWriter<TdiSdeInterface::PortStatusEvent>>
    port_status_event_writer_ GUARDED_BY(port_status_event_writer_lock_);
};

}  // namespace tdi
}  // namespace hal
}  // namespace stratum

#endif  // STRATUM_HAL_LIB_TDI_TOFINO_TOFINO_PORT_MANAGER_H_
