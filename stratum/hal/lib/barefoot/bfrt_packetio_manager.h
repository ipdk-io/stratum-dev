// Copyright 2020-present Open Networking Foundation
// SPDX-License-Identifier: Apache-2.0

#ifndef STRATUM_HAL_LIB_BAREFOOT_BFRT_PACKETIO_MANAGER_H_
#define STRATUM_HAL_LIB_BAREFOOT_BFRT_PACKETIO_MANAGER_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "absl/base/thread_annotations.h"
#include "absl/container/flat_hash_map.h"
#include "absl/memory/memory.h"
#include "absl/synchronization/mutex.h"
#include "p4/v1/p4runtime.pb.h"
#include "stratum/glue/status/status.h"
#include "stratum/hal/lib/barefoot/bf.pb.h"
#include "stratum/hal/lib/barefoot/bf_global_vars.h"
#include "stratum/hal/lib/barefoot/bf_sde_interface.h"
#include "stratum/hal/lib/barefoot/bfrt_p4runtime_translator.h"
#include "stratum/hal/lib/common/common.pb.h"
#include "stratum/hal/lib/common/writer_interface.h"
#include "stratum/lib/utils.h"

namespace stratum {
namespace hal {
namespace barefoot {

class BfrtPacketioManager {
 public:
  virtual ~BfrtPacketioManager();

  // Pushes the parts of the given ChassisConfig proto that this class cares
  // about. If the class is not initialized (i.e. if config is pushed for the
  // first time), this function also initializes class.
  virtual ::util::Status PushChassisConfig(const ChassisConfig& config,
                                           uint64 node_id)
      ABSL_LOCKS_EXCLUDED(data_lock_);

  // Verifies the parts of ChassisConfig proto that this class cares about.
  // The given node_id is used to understand which part of the ChassisConfig is
  // intended for this class.
  virtual ::util::Status VerifyChassisConfig(const ChassisConfig& config,
                                             uint64 node_id);

  // Pushes the forwarding pipeline to this class. If this is the first time, it
  // will also set up the necessary callbacks for packet IO.
  virtual ::util::Status PushForwardingPipelineConfig(
      const BfrtDeviceConfig& config) ABSL_LOCKS_EXCLUDED(data_lock_);

  // Performs coldboot shutdown. Note that there is no public Initialize().
  // Initialization is done as part of PushChassisConfig() if the class is not
  // initialized by the time we push config.
  virtual ::util::Status Shutdown() ABSL_LOCKS_EXCLUDED(data_lock_);

  // Registers a writer to be invoked when we capture a packet on a PCIe
  // interface.
  virtual ::util::Status RegisterPacketReceiveWriter(
      const std::shared_ptr<WriterInterface<::p4::v1::PacketIn>>& writer)
      ABSL_LOCKS_EXCLUDED(rx_writer_lock_);

  virtual ::util::Status UnregisterPacketReceiveWriter()
      ABSL_LOCKS_EXCLUDED(rx_writer_lock_);

  // Transmits a packet to the PCIe interface.
  virtual ::util::Status TransmitPacket(const ::p4::v1::PacketOut& packet)
      ABSL_LOCKS_EXCLUDED(data_lock_);

  // Factory function for creating the instance of the class.
  static std::unique_ptr<BfrtPacketioManager> CreateInstance(
      BfSdeInterface* bf_sde_interface,
      BfrtP4RuntimeTranslator* bfrt_p4runtime_translator, int device);

  // BfrtPacketioManager is neither copyable nor movable.
  BfrtPacketioManager(const BfrtPacketioManager&) = delete;
  BfrtPacketioManager& operator=(const BfrtPacketioManager&) = delete;

 protected:
  // Default constructor. To be called by the Mock class instance only.
  BfrtPacketioManager();

 private:
  // Private constructor. Use CreateInstance() to create an instance of this
  // class.
  explicit BfrtPacketioManager(
      BfSdeInterface* bf_sde_interface,
      BfrtP4RuntimeTranslator* bfrt_p4runtime_translator, int device);

  // Builds the packet header structure for controller packets.
  ::util::Status BuildMetadataMapping(const p4::config::v1::P4Info& p4_info)
      ABSL_EXCLUSIVE_LOCKS_REQUIRED(data_lock_);

  // Deparses a PacketOut into the buffer by serializing the metadata fields in
  // front of the payload.
  ::util::Status DeparsePacketOut(const ::p4::v1::PacketOut& packet,
                                  std::string* buffer)
      ABSL_LOCKS_EXCLUDED(data_lock_);

  // Parses a binary string into a PacketIn, filling the metadata fields.
  ::util::Status ParsePacketIn(const std::string& buffer,
                               ::p4::v1::PacketIn* packet)
      ABSL_LOCKS_EXCLUDED(data_lock_);

  // Handles a received packets and hands it over the registered receive writer.
  ::util::Status HandleSdePacketRx()
      ABSL_LOCKS_EXCLUDED(data_lock_, rx_writer_lock_);

  // Handles a received packets and hands it over the registered receive writer.
  ::util::Status HandleVirtualCpuIntfPacketRx() ABSL_LOCKS_EXCLUDED(data_lock_);

  // SDE CPU interface RX thread function.
  static void* SdeRxThreadFunc(void* arg);

  // Virtual CPU interface RX thread function.
  static void* VirtualCpuIntfRxThreadFunc(void* arg);

  // Mutex lock for protecting rx_writer_.
  mutable absl::Mutex rx_writer_lock_;

  // Mutex lock to protect the metadata mappings.
  mutable absl::Mutex data_lock_;

  // Initialized to false, set once only on first PushForwardingPipelineConfig.
  bool initialized_ ABSL_GUARDED_BY(data_lock_);

  // Stores the registered writer for PacketIns.
  std::shared_ptr<WriterInterface<::p4::v1::PacketIn>> rx_writer_
      ABSL_GUARDED_BY(rx_writer_lock_);

  // List of metadata id and bitwidth pairs. Stores the size and structure of
  // the CPU packet headers.
  std::vector<std::pair<uint32, int>> packetin_header_ ABSL_GUARDED_BY(data_lock_);
  std::vector<std::pair<uint32, int>> packetout_header_ ABSL_GUARDED_BY(data_lock_);
  size_t packetin_header_size_ ABSL_GUARDED_BY(data_lock_);
  size_t packetout_header_size_ ABSL_GUARDED_BY(data_lock_);

  // Buffer channel for packets coming from the SDE to this manager.
  std::shared_ptr<Channel<std::string>> packet_receive_channel_
      ABSL_GUARDED_BY(data_lock_);

  // File descriptor of the virtual TAP port used to simulate a CPU port.
  int tap_intf_fd_ ABSL_GUARDED_BY(data_lock_);

  // The ID of the RX thread which handles receiving packets from the SDE.
  pthread_t sde_rx_thread_id_ ABSL_GUARDED_BY(data_lock_);

  // The ID of the RX thread which handles receiving packets from the virtual
  // CPU interface.
  pthread_t virtual_cpu_intf_rx_thread_id_ ABSL_GUARDED_BY(data_lock_);

  // Pointer to a BfSdeInterface implementation that wraps all the SDE calls.
  BfSdeInterface* bf_sde_interface_ = nullptr;  // not owned by this class.

  // Pointer to a BfrtTranslator implementation that translate P4Runtime
  // entities, not owned by this class.
  BfrtP4RuntimeTranslator* bfrt_p4runtime_translator_ = nullptr;

  // Fixed zero-based Tofino device number corresponding to the node/ASIC
  // managed by this class instance. Assigned in the class constructor.
  const int device_;

  friend class BfrtPacketioManagerTest;
};

}  // namespace barefoot
}  // namespace hal
}  // namespace stratum

#endif  // STRATUM_HAL_LIB_BAREFOOT_BFRT_PACKETIO_MANAGER_H_
