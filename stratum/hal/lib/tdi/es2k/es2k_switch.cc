// Copyright 2020-present Open Networking Foundation
// Copyright 2022-2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "stratum/hal/lib/tdi/es2k/es2k_switch.h"

#include <algorithm>
#include <map>
#include <vector>

#include "absl/memory/memory.h"
#include "absl/synchronization/mutex.h"
#include "stratum/glue/gtl/map_util.h"
#include "stratum/glue/integral_types.h"
#include "stratum/glue/logging.h"
#include "stratum/glue/status/status_macros.h"
#include "stratum/hal/lib/tdi/es2k/es2k_chassis_manager.h"
#include "stratum/hal/lib/tdi/tdi_node.h"
#include "stratum/hal/lib/tdi/utils.h"
#include "stratum/lib/constants.h"
#include "stratum/lib/macros.h"

namespace stratum {
namespace hal {
namespace tdi {

Es2kSwitch::Es2kSwitch(
    Es2kChassisManager* chassis_manager,
    IPsecManager* ipsec_manager,
    const std::map<int, TdiNode*>& device_id_to_tdi_node)
    : chassis_manager_(ABSL_DIE_IF_NULL(chassis_manager)),
      ipsec_manager_(ABSL_DIE_IF_NULL(ipsec_manager)),
      device_id_to_tdi_node_(device_id_to_tdi_node),
      node_id_to_tdi_node_() {
  for (const auto& entry : device_id_to_tdi_node_) {
    CHECK_GE(entry.first, 0)
        << "Invalid device_id number " << entry.first << ".";
    CHECK_NE(entry.second, nullptr)
        << "Detected null TdiNode for device_id " << entry.first << ".";
  }
}

Es2kSwitch::~Es2kSwitch() {}

::util::Status Es2kSwitch::PushChassisConfig(const ChassisConfig& config) {
  absl::WriterMutexLock l(&chassis_lock);
  RETURN_IF_ERROR(chassis_manager_->PushChassisConfig(config));
  ASSIGN_OR_RETURN(const auto& node_id_to_device_id,
                   chassis_manager_->GetNodeIdToUnitMap());
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

::util::Status Es2kSwitch::VerifyChassisConfig(const ChassisConfig& config) {
  (void)config;
  return ::util::OkStatus();
}

::util::Status Es2kSwitch::PushForwardingPipelineConfig(
    uint64 node_id, const ::p4::v1::ForwardingPipelineConfig& config) {
  absl::WriterMutexLock l(&chassis_lock);
  ASSIGN_OR_RETURN(auto* tdi_node, GetTdiNodeFromNodeId(node_id));
  RETURN_IF_ERROR(tdi_node->PushForwardingPipelineConfig(config));
  RETURN_IF_ERROR(chassis_manager_->ReplayPortsConfig(node_id));

  LOG(INFO) << "P4-based forwarding pipeline config pushed successfully to "
            << "node with ID " << node_id << ".";

  return ::util::OkStatus();
}

::util::Status Es2kSwitch::SaveForwardingPipelineConfig(
    uint64 node_id, const ::p4::v1::ForwardingPipelineConfig& config) {
  absl::WriterMutexLock l(&chassis_lock);
  ASSIGN_OR_RETURN(auto* tdi_node, GetTdiNodeFromNodeId(node_id));
  RETURN_IF_ERROR(tdi_node->SaveForwardingPipelineConfig(config));
  RETURN_IF_ERROR(chassis_manager_->ReplayPortsConfig(node_id));

  LOG(INFO) << "P4-based forwarding pipeline config saved successfully to "
            << "node with ID " << node_id << ".";

  return ::util::OkStatus();
}

::util::Status Es2kSwitch::CommitForwardingPipelineConfig(uint64 node_id) {
  absl::WriterMutexLock l(&chassis_lock);
  ASSIGN_OR_RETURN(auto* tdi_node, GetTdiNodeFromNodeId(node_id));
  RETURN_IF_ERROR(tdi_node->CommitForwardingPipelineConfig());

  LOG(INFO) << "P4-based forwarding pipeline config committed successfully to "
            << "node with ID " << node_id << ".";

  return ::util::OkStatus();
}

::util::Status Es2kSwitch::VerifyForwardingPipelineConfig(
    uint64 node_id, const ::p4::v1::ForwardingPipelineConfig& config) {
  absl::WriterMutexLock l(&chassis_lock);
  ASSIGN_OR_RETURN(auto* tdi_node, GetTdiNodeFromNodeId(node_id));
  return tdi_node->VerifyForwardingPipelineConfig(config);
}

::util::Status Es2kSwitch::Shutdown() {
  ::util::Status status = ::util::OkStatus();
  for (const auto& entry : device_id_to_tdi_node_) {
    TdiNode* node = entry.second;
    APPEND_STATUS_IF_ERROR(status, node->Shutdown());
  }
  APPEND_STATUS_IF_ERROR(status, chassis_manager_->Shutdown());

  return status;
}

::util::Status Es2kSwitch::Freeze() { return ::util::OkStatus(); }

::util::Status Es2kSwitch::Unfreeze() { return ::util::OkStatus(); }

::util::Status Es2kSwitch::WriteForwardingEntries(
    const ::p4::v1::WriteRequest& req, std::vector<::util::Status>* results) {
  if (!req.updates_size()) return ::util::OkStatus();  // nothing to do.
  CHECK_RETURN_IF_FALSE(req.device_id()) << "No device_id in WriteRequest.";
  CHECK_RETURN_IF_FALSE(results != nullptr)
      << "Need to provide non-null results pointer for non-empty updates.";

  absl::ReaderMutexLock l(&chassis_lock);
  ASSIGN_OR_RETURN(auto* tdi_node, GetTdiNodeFromNodeId(req.device_id()));
  return tdi_node->WriteForwardingEntries(req, results);
}

::util::Status Es2kSwitch::ReadForwardingEntries(
    const ::p4::v1::ReadRequest& req,
    WriterInterface<::p4::v1::ReadResponse>* writer,
    std::vector<::util::Status>* details) {
  CHECK_RETURN_IF_FALSE(req.device_id()) << "No device_id in ReadRequest.";
  CHECK_RETURN_IF_FALSE(writer) << "Channel writer must be non-null.";
  CHECK_RETURN_IF_FALSE(details) << "Details pointer must be non-null.";

  absl::ReaderMutexLock l(&chassis_lock);
  ASSIGN_OR_RETURN(auto* tdi_node, GetTdiNodeFromNodeId(req.device_id()));
  return tdi_node->ReadForwardingEntries(req, writer, details);
}

::util::Status Es2kSwitch::RegisterStreamMessageResponseWriter(
    uint64 node_id,
    std::shared_ptr<WriterInterface<::p4::v1::StreamMessageResponse>> writer) {
  ASSIGN_OR_RETURN(auto* tdi_node, GetTdiNodeFromNodeId(node_id));
  return tdi_node->RegisterStreamMessageResponseWriter(writer);
}

::util::Status Es2kSwitch::UnregisterStreamMessageResponseWriter(
    uint64 node_id) {
  ASSIGN_OR_RETURN(auto* tdi_node, GetTdiNodeFromNodeId(node_id));
  return tdi_node->UnregisterStreamMessageResponseWriter();
}

::util::Status Es2kSwitch::HandleStreamMessageRequest(
    uint64 node_id, const ::p4::v1::StreamMessageRequest& request) {
  ASSIGN_OR_RETURN(auto* tdi_node, GetTdiNodeFromNodeId(node_id));
  return tdi_node->HandleStreamMessageRequest(request);
}

::util::Status Es2kSwitch::RegisterEventNotifyWriter(
    std::shared_ptr<WriterInterface<GnmiEventPtr>> writer) {
  return chassis_manager_->RegisterEventNotifyWriter(writer);
}

::util::Status Es2kSwitch::UnregisterEventNotifyWriter() {
  return chassis_manager_->UnregisterEventNotifyWriter();
}

::util::Status Es2kSwitch::RetrieveValue(
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
            chassis_manager_->GetUnitFromNodeId(req.node_info().node_id());
        if (!device_id.ok()) {
          status.Update(device_id.status());
        } else {
          auto* node_info = resp.mutable_node_info();
        }
        break;
      }
      // IPsecOffload request
      case DataRequest::Request::kIpsecOffloadInfo: {
        uint32 fetched_spi=0;
        auto fetch_status = ipsec_manager_->GetSpiData(fetched_spi);
        if (!fetch_status.ok()) {
          status.Update(fetch_status);
        } else {
          auto* info = resp.mutable_ipsec_offload_info();
          info->set_spi(fetched_spi);
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

::util::Status Es2kSwitch::SetValue(
    uint64 node_id, const SetRequest& request,
    std::vector<::util::Status>* details) {

  for (const auto& req : request.requests()) {
    ::util::Status status = ::util::OkStatus();
    switch (req.request_case()) {
      case SetRequest::Request::RequestCase::kIpsecOffloadConfig: {
        absl::WriterMutexLock l(&chassis_lock);
        auto op_type = req.ipsec_offload_config().ipsec_sadb_config_op();
        auto payload = const_cast<IPsecSADBConfig&>(
                          req.ipsec_offload_config().ipsec_sadb_config_info());
        status.Update(ipsec_manager_->WriteConfigSADBEntry(op_type, payload));
        break;
      }
      default:
        status = MAKE_ERROR(ERR_INTERNAL)
                 << req.ShortDebugString() << " Not supported yet!";
    }
    if (details) details->push_back(status);
  }

  return ::util::OkStatus();
}

::util::StatusOr<std::vector<std::string>> Es2kSwitch::VerifyState() {
  return std::vector<std::string>();
}

std::unique_ptr<Es2kSwitch> Es2kSwitch::CreateInstance(
    Es2kChassisManager* chassis_manager,
    IPsecManager* ipsec_manager,
    const std::map<int, TdiNode*>& device_id_to_tdi_node) {
  return absl::WrapUnique(
      new Es2kSwitch(chassis_manager, ipsec_manager, device_id_to_tdi_node));
}

::util::StatusOr<TdiNode*> Es2kSwitch::GetTdiNodeFromDeviceId(
    int device_id) const {
  TdiNode* tdi_node = gtl::FindPtrOrNull(device_id_to_tdi_node_, device_id);
  if (tdi_node == nullptr) {
    return MAKE_ERROR(ERR_INVALID_PARAM)
           << "Unit " << device_id << " is unknown.";
  }
  return tdi_node;
}

::util::StatusOr<TdiNode*> Es2kSwitch::GetTdiNodeFromNodeId(
    uint64 node_id) const {
  TdiNode* tdi_node = gtl::FindPtrOrNull(node_id_to_tdi_node_, node_id);
  if (tdi_node == nullptr) {
    return MAKE_ERROR(ERR_INVALID_PARAM)
           << "Node with ID " << node_id
           << " is unknown or no config has been pushed to it yet.";
  }
  return tdi_node;
}

}  // namespace tdi
}  // namespace hal
}  // namespace stratum
