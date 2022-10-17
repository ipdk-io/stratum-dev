// Copyright 2020-present Open Networking Foundation
// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "stratum/hal/lib/tdi/tofino/tofino_switch.h"

#include <algorithm>
#include <vector>

#include "absl/memory/memory.h"
#include "absl/synchronization/mutex.h"
#include "stratum/glue/gtl/map_util.h"
#include "stratum/glue/integral_types.h"
#include "stratum/glue/logging.h"
#include "stratum/glue/status/status_macros.h"
#include "stratum/hal/lib/tdi/tofino/tofino_chassis_manager.h"
#include "stratum/hal/lib/tdi/tdi_node.h"
#include "stratum/hal/lib/tdi/utils.h"
#include "stratum/lib/constants.h"
#include "stratum/lib/macros.h"

namespace stratum {
namespace hal {
namespace tdi {

TofinoSwitch::TofinoSwitch(
    PhalInterface* phal_interface,
    TofinoChassisManager* chassis_manager,
    TdiSdeInterface* sde_interface,
    const absl::flat_hash_map<int, TdiNode*>& device_id_to_tdi_node)
    : phal_interface_(ABSL_DIE_IF_NULL(phal_interface)),
      sde_interface_(ABSL_DIE_IF_NULL(sde_interface)),
      chassis_manager_(ABSL_DIE_IF_NULL(chassis_manager)),
      device_id_to_tdi_node_(device_id_to_tdi_node),
      node_id_to_tdi_node_() {
  for (const auto& entry : device_id_to_tdi_node_) {
    CHECK_GE(entry.first, 0)
        << "Invalid device_id number " << entry.first << ".";

    CHECK_NE(entry.second, nullptr)
        << "Detected null TdiNode for device_id " << entry.first << ".";
  }
}

TofinoSwitch::~TofinoSwitch() {}

::util::Status TofinoSwitch::PushChassisConfig(const ChassisConfig& config) {
  absl::WriterMutexLock l(&chassis_lock);
  RETURN_IF_ERROR(DoVerifyChassisConfig(config));
  RETURN_IF_ERROR(phal_interface_->PushChassisConfig(config));
  RETURN_IF_ERROR(chassis_manager_->PushChassisConfig(config));
  ASSIGN_OR_RETURN(const auto& node_id_to_device_id,
                   chassis_manager_->GetNodeIdToDeviceMap());
  node_id_to_tdi_node_.clear();
  for (const auto& entry : node_id_to_device_id) {
    uint64 node_id = entry.first;
    int device_id = entry.second;
    ASSIGN_OR_RETURN(auto* tdi_node, GetTdiNodeFromDeviceId(device_id));
    RETURN_IF_ERROR(tdi_node->PushChassisConfig(config, node_id));
    node_id_to_tdi_node_[node_id] = tdi_node;
  }

  LOG(INFO) << "Chassis config pushed successfully.";

  return ::util::OkStatus();
}

::util::Status TofinoSwitch::VerifyChassisConfig(const ChassisConfig& config) {
  absl::ReaderMutexLock l(&chassis_lock);
  return DoVerifyChassisConfig(config);
}

::util::Status TofinoSwitch::PushForwardingPipelineConfig(
    uint64 node_id, const ::p4::v1::ForwardingPipelineConfig& config) {
  absl::WriterMutexLock l(&chassis_lock);
  RETURN_IF_ERROR(DoVerifyForwardingPipelineConfig(node_id, config));
  ASSIGN_OR_RETURN(auto* tdi_node, GetTdiNodeFromNodeId(node_id));
  RETURN_IF_ERROR(tdi_node->PushForwardingPipelineConfig(config));
  RETURN_IF_ERROR(chassis_manager_->ReplayChassisConfig(node_id));

  LOG(INFO) << "P4-based forwarding pipeline config pushed successfully to "
            << "node with ID " << node_id << ".";

#ifdef TOFINO_ATRGET
// TODO (Ravi): Check if this is required
#if 0
  ASSIGN_OR_RETURN(const auto& node_id_to_device_id,
                   chassis_manager_->GetNodeIdToDeviceMap());

  RET_CHECK(gtl::ContainsKey(node_id_to_device_id, node_id))
      << "Unable to find device_id number for node " << node_id;
  int device_id = gtl::FindOrDie(node_id_to_device_id, node_id);
  ASSIGN_OR_RETURN(auto cpu_port, sde_interface_->GetPcieCpuPort(device_id));
  RETURN_IF_ERROR(sde_interface_->SetTmCpuPort(device_id, cpu_port));
#endif
#endif

  return ::util::OkStatus();
}

::util::Status TofinoSwitch::SaveForwardingPipelineConfig(
    uint64 node_id, const ::p4::v1::ForwardingPipelineConfig& config) {
  absl::WriterMutexLock l(&chassis_lock);
  ASSIGN_OR_RETURN(auto* tdi_node, GetTdiNodeFromNodeId(node_id));
  RETURN_IF_ERROR(tdi_node->SaveForwardingPipelineConfig(config));
  RETURN_IF_ERROR(chassis_manager_->ReplayChassisConfig(node_id));

  LOG(INFO) << "P4-based forwarding pipeline config saved successfully to "
            << "node with ID " << node_id << ".";

  return ::util::OkStatus();
}

::util::Status TofinoSwitch::CommitForwardingPipelineConfig(uint64 node_id) {
  absl::WriterMutexLock l(&chassis_lock);
  ASSIGN_OR_RETURN(auto* tdi_node, GetTdiNodeFromNodeId(node_id));
  RETURN_IF_ERROR(tdi_node->CommitForwardingPipelineConfig());

  LOG(INFO) << "P4-based forwarding pipeline config committed successfully to "
            << "node with ID " << node_id << ".";

  return ::util::OkStatus();
}

::util::Status TofinoSwitch::VerifyForwardingPipelineConfig(
    uint64 node_id, const ::p4::v1::ForwardingPipelineConfig& config) {
  absl::WriterMutexLock l(&chassis_lock);
  return DoVerifyForwardingPipelineConfig(node_id, config);
}

::util::Status TofinoSwitch::Shutdown() {
  ::util::Status status = ::util::OkStatus();
  for (const auto& entry : device_id_to_tdi_node_) {
    TdiNode* node = entry.second;
    APPEND_STATUS_IF_ERROR(status, node->Shutdown());
  }
  APPEND_STATUS_IF_ERROR(status, chassis_manager_->Shutdown());
  APPEND_STATUS_IF_ERROR(status, phal_interface_->Shutdown());
  // APPEND_STATUS_IF_ERROR(status, sde_interface_->Shutdown());

  return status;
}

::util::Status TofinoSwitch::Freeze() { return ::util::OkStatus(); }

::util::Status TofinoSwitch::Unfreeze() { return ::util::OkStatus(); }

::util::Status TofinoSwitch::WriteForwardingEntries(
    const ::p4::v1::WriteRequest& req, std::vector<::util::Status>* results) {
  if (!req.updates_size()) return ::util::OkStatus();  // nothing to do.
  RET_CHECK(req.device_id()) << "No device_id in WriteRequest.";
  RET_CHECK(results != nullptr)
      << "Need to provide non-null results pointer for non-empty updates.";

  absl::ReaderMutexLock l(&chassis_lock);
  ASSIGN_OR_RETURN(auto* tdi_node, GetTdiNodeFromNodeId(req.device_id()));
  return tdi_node->WriteForwardingEntries(req, results);
}

::util::Status TofinoSwitch::ReadForwardingEntries(
    const ::p4::v1::ReadRequest& req,
    WriterInterface<::p4::v1::ReadResponse>* writer,
    std::vector<::util::Status>* details) {
  RET_CHECK(req.device_id()) << "No device_id in ReadRequest.";
  RET_CHECK(writer) << "Channel writer must be non-null.";
  RET_CHECK(details) << "Details pointer must be non-null.";

  absl::ReaderMutexLock l(&chassis_lock);
  ASSIGN_OR_RETURN(auto* tdi_node, GetTdiNodeFromNodeId(req.device_id()));
  return tdi_node->ReadForwardingEntries(req, writer, details);
}

::util::Status TofinoSwitch::RegisterStreamMessageResponseWriter(
    uint64 node_id,
    std::shared_ptr<WriterInterface<::p4::v1::StreamMessageResponse>> writer) {
  ASSIGN_OR_RETURN(auto* tdi_node, GetTdiNodeFromNodeId(node_id));
  return tdi_node->RegisterStreamMessageResponseWriter(writer);
}

::util::Status TofinoSwitch::UnregisterStreamMessageResponseWriter(
    uint64 node_id) {
  ASSIGN_OR_RETURN(auto* tdi_node, GetTdiNodeFromNodeId(node_id));
  return tdi_node->UnregisterStreamMessageResponseWriter();
}

::util::Status TofinoSwitch::HandleStreamMessageRequest(
    uint64 node_id, const ::p4::v1::StreamMessageRequest& request) {
  ASSIGN_OR_RETURN(auto* tdi_node, GetTdiNodeFromNodeId(node_id));
  return tdi_node->HandleStreamMessageRequest(request);
}

::util::Status TofinoSwitch::RegisterEventNotifyWriter(
    std::shared_ptr<WriterInterface<GnmiEventPtr>> writer) {
  return chassis_manager_->RegisterEventNotifyWriter(writer);
}

::util::Status TofinoSwitch::UnregisterEventNotifyWriter() {
  return chassis_manager_->UnregisterEventNotifyWriter();
}

::util::Status TofinoSwitch::RetrieveValue(
    uint64 node_id,
    const DataRequest& request,
    WriterInterface<DataResponse>* writer,
    std::vector<::util::Status>* details) {
  absl::ReaderMutexLock l(&chassis_lock);
  for (const auto& req : request.requests()) {
    DataResponse resp;
    ::util::Status status = ::util::OkStatus();
    switch (req.request_case()) {
      // Port data request
      case DataRequest::Request::kOperStatus:
      case DataRequest::Request::kAdminStatus:
      case DataRequest::Request::kMacAddress:
      case DataRequest::Request::kPortSpeed:
      case DataRequest::Request::kNegotiatedPortSpeed:
      case DataRequest::Request::kLacpRouterMac:
      case DataRequest::Request::kPortCounters:
      case DataRequest::Request::kForwardingViability:
      case DataRequest::Request::kHealthIndicator:
      case DataRequest::Request::kAutonegStatus:
      case DataRequest::Request::kFrontPanelPortInfo:
      case DataRequest::Request::kLoopbackStatus:
      case DataRequest::Request::kSdnPortId: {
        auto port_data = chassis_manager_->GetPortData(req);
        if (!port_data.ok()) {
          status.Update(port_data.status());
        } else {
          resp = port_data.ConsumeValueOrDie();
        }
        break;
      }
      // Node information request
      case DataRequest::Request::kNodeInfo: {
        auto device_id =
            chassis_manager_->GetDeviceFromNodeId(req.node_info().node_id());
        if (!device_id.ok()) {
          status.Update(device_id.status());
        } else {
          auto* node_info = resp.mutable_node_info();
          node_info->set_vendor_name("Barefoot");
          node_info->set_chip_name(
              sde_interface_->GetBfChipType(device_id.ValueOrDie()));
        }
        break;
      }
      default:
        status =
            MAKE_ERROR(ERR_UNIMPLEMENTED)
            << "DataRequest field "
            << req.descriptor()->FindFieldByNumber(req.request_case())->name()
            << " is not supported yet!";
        break;
    }
    if (status.ok()) {
      // If everything is OK send it to the caller.
      writer->Write(resp);
    }
    if (details) details->push_back(status);
  }
  return ::util::OkStatus();
}

::util::Status TofinoSwitch::SetValue(
    uint64 node_id, const SetRequest& request,
    std::vector<::util::Status>* details) {
  LOG(INFO) << "TofinoSwitch::SetValue is not implemented yet. Changes will "
            << "be applied when ChassisConfig is pushed again. "
            << request.ShortDebugString() << ".";

  return ::util::OkStatus();
}

::util::StatusOr<std::vector<std::string>> TofinoSwitch::VerifyState() {
  return std::vector<std::string>();
}

std::unique_ptr<TofinoSwitch> TofinoSwitch::CreateInstance(
    PhalInterface* phal_interface, TofinoChassisManager* chassis_manager,
    TdiSdeInterface* sde_interface,
    const absl::flat_hash_map<int, TdiNode*>& device_id_to_tdi_node) {
  return absl::WrapUnique(
      new TofinoSwitch(phal_interface, chassis_manager,
                       sde_interface, device_id_to_tdi_node));
}

::util::Status TofinoSwitch::DoVerifyForwardingPipelineConfig(
    uint64 node_id, const ::p4::v1::ForwardingPipelineConfig& config) {
  // Get the TdiNode pointer first. No need to continue if we cannot find one.
  ASSIGN_OR_RETURN(auto* tdi_node, GetTdiNodeFromNodeId(node_id));
  // Verify the forwarding config in all the managers and nodes.
  auto status = ::util::OkStatus();
  APPEND_STATUS_IF_ERROR(status,
                         tdi_node->VerifyForwardingPipelineConfig(config));

  if (status.ok()) {
    LOG(INFO) << "P4-based forwarding pipeline config verified successfully"
              << " for node with ID " << node_id << ".";
  }

  return status;
}

::util::Status TofinoSwitch::DoVerifyChassisConfig(const ChassisConfig& config) {
  // First make sure PHAL is happy with the config then continue with the rest
  // of the managers and nodes.
  ::util::Status status = ::util::OkStatus();
  APPEND_STATUS_IF_ERROR(status, phal_interface_->VerifyChassisConfig(config));
  APPEND_STATUS_IF_ERROR(status,
                         chassis_manager_->VerifyChassisConfig(config));
  // Get the current copy of the node_id_to_device from chassis manager. If this
  // fails with ERR_NOT_INITIALIZED, do not verify anything at the node level.
  // Note that we do not expect any change in node_id_to_device. Any change in
  // this map will be detected in chassis_manager_->VerifyChassisConfig.
  auto ret = chassis_manager_->GetNodeIdToDeviceMap();
  if (!ret.ok()) {
    if (ret.status().error_code() != ERR_NOT_INITIALIZED) {
      APPEND_STATUS_IF_ERROR(status, ret.status());
    }
  } else {
    const auto& node_id_to_device_id = ret.ValueOrDie();
    for (const auto& entry : node_id_to_device_id) {
      uint64 node_id = entry.first;
      int device_id = entry.second;
      TdiNode* tdi_node =
          gtl::FindPtrOrNull(device_id_to_tdi_node_, device_id);
      if (tdi_node == nullptr) {
        ::util::Status error = MAKE_ERROR(ERR_ENTRY_NOT_FOUND)
                               << "Node ID " << node_id
                               << " mapped to unknown device " << device_id
                               << ".";
        APPEND_STATUS_IF_ERROR(status, error);
        continue;
      }
      APPEND_STATUS_IF_ERROR(status,
                             tdi_node->VerifyChassisConfig(config, node_id));
    }
  }

  if (status.ok()) {
    LOG(INFO) << "Chassis config verified successfully.";
  }

  return status;
}
::util::StatusOr<TdiNode*> TofinoSwitch::GetTdiNodeFromDeviceId(
    int device_id) const {
  TdiNode* tdi_node = gtl::FindPtrOrNull(device_id_to_tdi_node_, device_id);
  if (tdi_node == nullptr) {
    return MAKE_ERROR(ERR_ENTRY_NOT_FOUND)
           << "Device " << device_id << " is unknown.";
  }
  return tdi_node;
}

::util::StatusOr<TdiNode*> TofinoSwitch::GetTdiNodeFromNodeId(
    uint64 node_id) const {
  TdiNode* tdi_node = gtl::FindPtrOrNull(node_id_to_tdi_node_, node_id);
  if (tdi_node == nullptr) {
    return MAKE_ERROR(ERR_ENTRY_NOT_FOUND)
           << "Node with ID " << node_id
           << " is unknown or no config has been pushed to it yet.";
  }
  return tdi_node;
}

}  // namespace tdi
}  // namespace hal
}  // namespace stratum
