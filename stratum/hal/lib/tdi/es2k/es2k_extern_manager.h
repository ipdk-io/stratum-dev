// Copyright 2023-2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#ifndef STRATUM_HAL_LIB_TDI_ES2K_ES2K_EXTERN_MANAGER_H_
#define STRATUM_HAL_LIB_TDI_ES2K_ES2K_EXTERN_MANAGER_H_

#include <memory>
#include <string>

#include "absl/container/flat_hash_map.h"
#include "idpf/p4info.pb.h"
#include "p4/config/v1/p4info.pb.h"
#include "stratum/glue/integral_types.h"
#include "stratum/glue/status/statusor.h"
#include "stratum/hal/lib/p4/p4_info_manager.h"
#include "stratum/hal/lib/p4/p4_resource_map.h"
#include "stratum/hal/lib/tdi/tdi_extern_manager.h"
#include "stratum/hal/lib/tdi/tdi_resource_handler.h"

namespace stratum {
namespace hal {
namespace tdi {

class Es2kExternManager : public TdiExternManager {
 public:
  typedef std::shared_ptr<TdiResourceHandler> ResourceHandler;

  struct HandlerParams {
    HandlerParams()
        : sde_interface(nullptr),
          p4_info_manager(nullptr),
          lock(nullptr),
          device(0) {}

    TdiSdeInterface* sde_interface;  // not owned by this class
    P4InfoManager* p4_info_manager;  // not owned by this class
    absl::Mutex* lock;               // not owned by this class
    int device;
  };

  Es2kExternManager();
  virtual ~Es2kExternManager() = default;

  // Performs basic initialization. Called by TdiTableManager.
  void Initialize(TdiSdeInterface* sde_interface,
                  P4InfoManager* p4_info_manager, absl::Mutex* lock,
                  int device) override;

  // Registers P4Extern resources. Called by P4InfoManager.
  void RegisterExterns(const ::p4::config::v1::P4Info& p4info,
                       const PreambleCallback& preamble_cb) override;

  // Returns the handler for the P4 resource with the specified ID,
  // or null if the resource is not registered.
  TdiResourceHandler* FindResourceHandler(uint32 resource_id);

  // Returns the number of entries in the resource handler map.
  uint32 ResourceHandlerMapSize() const { return resource_handler_map_.size(); }

  // Retrieve a PacketModMeter configuration.
  ::util::StatusOr<const ::idpf::PacketModMeter> FindPktModMeterByID(
      uint32 meter_id) const;

  ::util::StatusOr<const ::idpf::PacketModMeter> FindPktModMeterByName(
      const std::string& meter_name) const;

  // Retrieve a DirectPacketModMeter configuration.
  ::util::StatusOr<const ::idpf::DirectPacketModMeter>
  FindDirectPktModMeterByID(uint32 meter_id) const;

  ::util::StatusOr<const ::idpf::DirectPacketModMeter>
  FindDirectPktModMeterByName(const std::string& meter_name) const;

  // Returns the number of entries in the PacketModMeter map.
  uint32 PktModMeterMapSize() const { return pkt_mod_meter_map_.size(); }

  // Returns the number of entries in the DirectPacketModMeter map.
  uint32 DirectPktModMeterMapSize() const {
    return direct_pkt_mod_meter_map_.size();
  }

 private:
  void RegisterPacketModMeters(const p4::config::v1::Extern& p4extern,
                               const PreambleCallback& preamble_cb);

  void RegisterDirectPacketModMeters(const p4::config::v1::Extern& p4extern,
                                     const PreambleCallback& preamble_cb);

  void RegisterResource(const ::p4::config::v1::Preamble& preamble,
                        ResourceHandler resource_handler);

  // One P4ResourceMap for each P4 extern resource type.
  P4ResourceMap<::idpf::PacketModMeter> pkt_mod_meter_map_;
  P4ResourceMap<::idpf::DirectPacketModMeter> direct_pkt_mod_meter_map_;

  google::protobuf::RepeatedPtrField<::idpf::PacketModMeter> meter_objects_;
  google::protobuf::RepeatedPtrField<::idpf::DirectPacketModMeter>
      direct_meter_objects_;

  absl::flat_hash_map<uint32, ResourceHandler> resource_handler_map_;

  // Parameters shared with the resource handlers.
  HandlerParams params_;
};

}  // namespace tdi
}  // namespace hal
}  // namespace stratum

#endif  // STRATUM_HAL_LIB_TDI_ES2K_ES2K_EXTERN_MANAGER_H_
