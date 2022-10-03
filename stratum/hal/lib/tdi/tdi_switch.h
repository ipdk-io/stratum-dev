// Copyright 2020-present Open Networking Foundation
// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#ifndef STRATUM_HAL_LIB_TDI_TDI_SWITCH_H_
#define STRATUM_HAL_LIB_TDI_TDI_SWITCH_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "absl/synchronization/mutex.h"
#include "stratum/hal/lib/tdi/tdi_sde_interface.h"
#include "stratum/hal/lib/tdi/tdi_node.h"
#include "stratum/hal/lib/common/phal_interface.h"
#include "stratum/hal/lib/common/switch_interface.h"

namespace stratum {
namespace hal {
namespace barefoot {

class TdiChassisManager;

class TdiSwitch : public SwitchInterface {
 public:
  ~TdiSwitch() override;

  // SwitchInterface public methods.
  ::util::Status PushChassisConfig(const ChassisConfig& config) override;
  ::util::Status VerifyChassisConfig(const ChassisConfig& config) override;
  ::util::Status PushForwardingPipelineConfig(
      uint64 node_id,
      const ::p4::v1::ForwardingPipelineConfig& config) override;
  ::util::Status SaveForwardingPipelineConfig(
      uint64 node_id,
      const ::p4::v1::ForwardingPipelineConfig& config) override;
  ::util::Status CommitForwardingPipelineConfig(uint64 node_id) override;
  ::util::Status VerifyForwardingPipelineConfig(
      uint64 node_id,
      const ::p4::v1::ForwardingPipelineConfig& config) override;
  ::util::Status Shutdown() override;
  ::util::Status Freeze() override;
  ::util::Status Unfreeze() override;
  ::util::Status WriteForwardingEntries(
      const ::p4::v1::WriteRequest& req,
      std::vector<::util::Status>* results) override;
  ::util::Status ReadForwardingEntries(
      const ::p4::v1::ReadRequest& req,
      WriterInterface<::p4::v1::ReadResponse>* writer,
      std::vector<::util::Status>* details) override;
  ::util::Status RegisterStreamMessageResponseWriter(
      uint64 node_id,
      std::shared_ptr<WriterInterface<::p4::v1::StreamMessageResponse>> writer)
      override;
  ::util::Status UnregisterStreamMessageResponseWriter(uint64 node_id) override;
  ::util::Status HandleStreamMessageRequest(
      uint64 node_id, const ::p4::v1::StreamMessageRequest& request) override;
  ::util::Status RegisterEventNotifyWriter(
      std::shared_ptr<WriterInterface<GnmiEventPtr>> writer) override;
  ::util::Status UnregisterEventNotifyWriter() override;
  ::util::Status RetrieveValue(uint64 node_id, const DataRequest& requests,
                               WriterInterface<DataResponse>* writer,
                               std::vector<::util::Status>* details) override;
  ::util::Status SetValue(uint64 node_id, const SetRequest& request,
                          std::vector<::util::Status>* details) override;
  ::util::StatusOr<std::vector<std::string>> VerifyState() override;

  // Factory function for creating the instance of the class.
  static std::unique_ptr<TdiSwitch> CreateInstance(
      PhalInterface* phal_interface, TdiChassisManager* tdi_chassis_manager,
      TdiSdeInterface* tdi_sde_interface,
      const std::map<int, TdiNode*>& device_id_to_tdi_node);

  // TdiSwitch is neither copyable nor movable.
  TdiSwitch(const TdiSwitch&) = delete;
  TdiSwitch& operator=(const TdiSwitch&) = delete;
  TdiSwitch(TdiSwitch&&) = delete;
  TdiSwitch& operator=(TdiSwitch&&) = delete;

 private:
  // Private constructor. Use CreateInstance() to create an instance of this
  // class.
  TdiSwitch(PhalInterface* phal_interface,
            TdiChassisManager* tdi_chassis_manager,
            TdiSdeInterface* tdi_sde_interface,
            const std::map<int, TdiNode*>& device_id_to_tdi_node);

  // Helper to get TdiNode pointer from device_id number or return error
  // indicating invalid device_id.
  ::util::StatusOr<TdiNode*> GetTdiNodeFromDeviceId(int device_id) const;

  // Helper to get TdiNode pointer from node id or return error indicating
  // invalid/unknown/uninitialized node.
  ::util::StatusOr<TdiNode*> GetTdiNodeFromNodeId(uint64 node_id) const;

  // Pointer to a PhalInterface implementation. The pointer has been also
  // passed to a few managers for accessing HW. Note that there is only one
  // instance of this class per chassis.
  PhalInterface* phal_interface_;  // not owned by this class.

  // Pointer to a TdiSdeInterface implementation that wraps PD API calls.
  TdiSdeInterface* tdi_sde_interface_;  // not owned by this class.

  // Per chassis Managers. Note that there is only one instance of this class
  // per chassis.
  TdiChassisManager* tdi_chassis_manager_;  // not owned by the class.

  // Map from zero-based device_id number corresponding to a node/ASIC to a
  // pointer to TdiNode which contain all the per-node managers for that
  // node/ASIC. This map is initialized in the constructor and will not change
  // during the lifetime of the class.
  // TODO(max): Does this need to be protected by chassis_lock?
  const std::map<int, TdiNode*> device_id_to_tdi_node_;  // pointers not owned

  // Map from the node ids to to a pointer to TdiNode which contain all the
  // per-node managers for that node/ASIC. Created everytime a config is pushed.
  // At any point of time this map will contain a keys the ids of the nodes
  // which had a successful config push.
  // TODO(max): Does this need to be protected by chassis_lock?
  std::map<uint64, TdiNode*> node_id_to_tdi_node_;  //  pointers not owned
};

}  // namespace barefoot
}  // namespace hal
}  // namespace stratum

#endif  // STRATUM_HAL_LIB_TDI_TDI_SWITCH_H_
