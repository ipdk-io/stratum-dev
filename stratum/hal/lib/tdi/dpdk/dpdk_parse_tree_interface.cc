// Copyright 2018 Google LLC
// Copyright 2018-present Open Networking Foundation
// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// DPDK-specific setup functions for YangParseTreePaths. Used by the
// AddSubtreeInterface() and AddSubtreeInterfaceFromTrunk() methods.

// NOTE: This is a rough port of the support functions added to
// yang_parse_tree_paths.cc in P4-OVS.
//
// - Calls to tree->GetTdiChassisManager()->ValidateOnetimeConfig()
//   have been replaced with calls to IsPortParamSet(tree, ...).
// - Calls to tree->GetTdiChassisManager()->ValidateAndAdd() have been
//   replaced with calls to SetPortParam(tree, ...).
// - Calls to tree->GetTdiChassisManager()->HotplugValidateAndAdd()
//   have been replaced with calls to SetHotplugParam(tree, ...).
//
// The three replacement functions are currently stubs.

#include "stratum/hal/lib/tdi/dpdk/dpdk_parse_tree_interface.h"

#include "gnmi/gnmi.pb.h"
#include "stratum/glue/status/status.h"
#include "stratum/glue/status/status_macros.h"
#include "stratum/hal/lib/common/common.pb.h"
#include "stratum/hal/lib/common/gnmi_events.h"
#include "stratum/hal/lib/yang/yang_parse_tree_helpers.h"
#include "stratum/hal/lib/yang/yang_parse_tree.h"
#include "stratum/hal/lib/common/utils.h"
#include "stratum/public/proto/error.pb.h"

namespace stratum {
namespace hal {
namespace yang {
namespace interface {

using namespace stratum::hal::yang::helpers;

namespace {

// Helper function to determine whether the specified port configuration
// parameter has already been set. Once set, it may not be set again.
bool IsPortParamSet(
    YangParseTree* tree, uint64 node_id, uint32 port_id,
    SetRequest::Request::Port::ValueCase value_case) {
  return false;
}

// Helper function to set the value of a port configuration parameter.
// Once set, it may not be set again.
::util::Status SetPortParam(
    YangParseTree* tree, uint64 node_id, uint32 port_id,
    const SingletonPort& singleton_port,
    SetRequest::Request::Port::ValueCase value_case) {
  return MAKE_ERROR(ERR_UNIMPLEMENTED) << "SetPortParam not implemented!";
}

::util::Status SetHotplugParam(
    YangParseTree* tree, uint64 node_id, uint32 port_id,
    const SingletonPort& singleton_port,
    SetRequest::Request::Port::ValueCase change_field,
    SWBackendHotplugParams params) {
  return MAKE_ERROR(ERR_UNIMPLEMENTED) << "SetHotplugParam not implemented!";
}

} // namespace

////////////////////////////////////////////////////////////////////////////////
// /interfaces/virtual-interface[name=<name>]/config/host
//
void SetUpInterfacesInterfaceConfigHost(
    const char *host_val, uint64 node_id, uint64 port_id,
    TreeNode* node, YangParseTree* tree) {
  auto poll_functor = [host_val](const GnmiEvent& event, const ::gnmi::Path& path,
                              GnmiSubscribeStream* stream) {
    // This leaf represents configuration data. Return what was known when it
    // was configured!
    return SendResponse(GetResponse(path, host_val), stream);
  };
  auto on_set_functor =
      [node_id, port_id, node, tree](
          const ::gnmi::Path& path, const ::google::protobuf::Message& val,
          CopyOnWriteChassisConfig* config) -> ::util::Status {
    auto typed_val = dynamic_cast<const gnmi::TypedValue*>(&val);
    if (typed_val == nullptr) {
      return MAKE_ERROR(ERR_INVALID_PARAM) << "not a TypedValue message!";
    }

    if (IsPortParamSet(tree, node_id, port_id,
                       SetRequest::Request::Port::ValueCase::kHostConfig))
    {
        return MAKE_ERROR(ERR_INVALID_PARAM)
            << "Host is already set or the PORT is already configured";
    }

    auto host_name = typed_val->string_val();
    SetRequest req;
    auto* request = req.add_requests()->mutable_port();
    request->set_node_id(node_id);
    request->set_port_id(port_id);
    request->mutable_host_config()->set_host_name(host_name.c_str());

    // Update the chassis config and setValue
    ChassisConfig* new_config = config->writable();
    for (auto& singleton_port : *new_config->mutable_singleton_ports()) {
      if (singleton_port.node() == node_id && singleton_port.id() == port_id) {
          singleton_port.mutable_config_params()
              ->set_host_name(host_name.c_str());

        // Validate if all mandatory params are set and call SDE API
        RETURN_IF_ERROR(
            SetPortParam(tree, node_id, port_id, singleton_port,
                         SetRequest::Request::Port::ValueCase::kHostConfig));
        break;
      }
    }

    // Update the YANG parse tree.
    auto poll_functor = [host_name](const GnmiEvent& event,
                                        const ::gnmi::Path& path,
                                        GnmiSubscribeStream* stream) {
      // This leaf represents configuration data. Return what was known when
      // it was configured!
      return SendResponse(GetResponse(path, host_name), stream);
    };
    node->SetOnTimerHandler(poll_functor)->SetOnPollHandler(poll_functor);

    return ::util::OkStatus();
  };
  node->SetOnTimerHandler(poll_functor)
      ->SetOnPollHandler(poll_functor)
      ->SetOnUpdateHandler(on_set_functor)
      ->SetOnReplaceHandler(on_set_functor);
}

////////////////////////////////////////////////////////////////////////////////
// /interfaces/virtual-interface[name=<name>]/config/port-type
//
void SetUpInterfacesInterfaceConfigPortType(
    uint64 type, uint64 node_id, uint64 port_id,
    TreeNode* node, YangParseTree* tree) {
  auto poll_functor = [type](const GnmiEvent& event, const ::gnmi::Path& path,
                              GnmiSubscribeStream* stream) {
    // This leaf represents configuration data. Return what was known when it
    // was configured!
    return SendResponse(GetResponse(path, type), stream);
  };
  auto on_set_functor =
      [node_id, port_id, node, tree](
          const ::gnmi::Path& path, const ::google::protobuf::Message& val,
          CopyOnWriteChassisConfig* config) -> ::util::Status {
    auto typed_val = dynamic_cast<const gnmi::TypedValue*>(&val);
    if (typed_val == nullptr) {
      return MAKE_ERROR(ERR_INVALID_PARAM) << "not a TypedValue message!";
    }

    if (IsPortParamSet(tree, node_id, port_id,
                       SetRequest::Request::Port::ValueCase::kPortType)) {
        return MAKE_ERROR(ERR_INVALID_PARAM)
            << "port-type is already set or the PORT is already configured";
    }

    std::string port_type_string = typed_val->string_val();
    SWBackendPortType port_type = PORT_TYPE_NONE;
    if (port_type_string == "vhost" || port_type_string == "VHOST") {
        port_type = SWBackendPortType::PORT_TYPE_VHOST;
    } else if (port_type_string == "link" || port_type_string == "LINK") {
        port_type = SWBackendPortType::PORT_TYPE_LINK;
    } else if (port_type_string == "tap" || port_type_string == "TAP") {
        port_type = SWBackendPortType::PORT_TYPE_TAP;
    } else if (port_type_string == "source" || port_type_string == "SOURCE") {
        port_type = SWBackendPortType::PORT_TYPE_SOURCE;
    } else if (port_type_string == "sink" || port_type_string == "SINK") {
        port_type = SWBackendPortType::PORT_TYPE_SINK;
    } else {
      return MAKE_ERROR(ERR_INVALID_PARAM) << "wrong value for port-type!";
    }

    // Set the value.
    auto status = SetValue(node_id, port_id, tree,
                           &SetRequest::Request::Port::mutable_port_type,
                           &SWBackendPortStatus::set_type, port_type);
    if (status != ::util::OkStatus()) {
      return status;
    }

    // Update the chassis config
    ChassisConfig* new_config = config->writable();
    for (auto& singleton_port : *new_config->mutable_singleton_ports()) {
      if (singleton_port.node() == node_id && singleton_port.id() == port_id) {
        singleton_port.mutable_config_params()->set_type(port_type);

        // Validate if all mandatory params are set and call SDE API
        RETURN_IF_ERROR(
            SetPortParam(tree, node_id, port_id, singleton_port,
                         SetRequest::Request::Port::ValueCase::kPortType));
        break;
      }
    }

    // Update the YANG parse tree.
    auto poll_functor = [port_type_string](const GnmiEvent& event,
                                           const ::gnmi::Path& path,
                                           GnmiSubscribeStream* stream) {
      // This leaf represents configuration data. Return what was known when
      // it was configured!
      return SendResponse(GetResponse(path, port_type_string), stream);
    };
    node->SetOnTimerHandler(poll_functor)->SetOnPollHandler(poll_functor);

    return ::util::OkStatus();
  };
  node->SetOnTimerHandler(poll_functor)
      ->SetOnPollHandler(poll_functor)
      ->SetOnUpdateHandler(on_set_functor)
      ->SetOnReplaceHandler(on_set_functor);
}

////////////////////////////////////////////////////////////////////////////////
// /interfaces/virtual-interface[name=<name>]/config/device-type
//
void SetUpInterfacesInterfaceConfigDeviceType(
    uint64 type, uint64 node_id, uint64 port_id,
    TreeNode* node, YangParseTree* tree) {
  auto poll_functor = [type](const GnmiEvent& event, const ::gnmi::Path& path,
                              GnmiSubscribeStream* stream) {
    // This leaf represents configuration data. Return what was known when it
    // was configured!
    return SendResponse(GetResponse(path, type), stream);
  };
  auto on_set_functor =
      [node_id, port_id, node, tree](
          const ::gnmi::Path& path, const ::google::protobuf::Message& val,
          CopyOnWriteChassisConfig* config) -> ::util::Status {
    auto typed_val = dynamic_cast<const gnmi::TypedValue*>(&val);
    if (typed_val == nullptr) {
      return MAKE_ERROR(ERR_INVALID_PARAM) << "not a TypedValue message!";
    }

    if (IsPortParamSet(tree, node_id, port_id,
                       SetRequest::Request::Port::ValueCase::kDeviceType)) {
      return MAKE_ERROR(ERR_INVALID_PARAM)
          << "device-type is already set PORT is already configured";
    }

    std::string device_type_string = typed_val->string_val();
    SWBackendDeviceType device_type = DEVICE_TYPE_NONE;
    if (device_type_string == "VIRTIO_NET" ||
        device_type_string == "virtio_net") {
      device_type = SWBackendDeviceType::DEVICE_TYPE_VIRTIO_NET;
    } else if (device_type_string == "VIRTIO_BLK" ||
               device_type_string == "virtio_blk") {
      device_type = SWBackendDeviceType::DEVICE_TYPE_VIRTIO_BLK;
    } else {
      return MAKE_ERROR(ERR_INVALID_PARAM)
          << "wrong value for device-type: accepted values are "
             "case-insensitive VIRTIO_NET or VIRTIO_BLK";
    }

    // Set the value.
    auto status = SetValue(node_id, port_id, tree,
                           &SetRequest::Request::Port::mutable_device_type,
                           &SWBackendDeviceStatus::set_device_type, device_type);
    if (status != ::util::OkStatus()) {
      return status;
    }

    // Update the chassis config
    ChassisConfig* new_config = config->writable();
    for (auto& singleton_port : *new_config->mutable_singleton_ports()) {
      if (singleton_port.node() == node_id && singleton_port.id() == port_id) {
        singleton_port.mutable_config_params()->set_device_type(device_type);

        // Validate if all mandatory params are set and call SDE API
        RETURN_IF_ERROR(
            SetPortParam(tree, node_id, port_id, singleton_port,
                         SetRequest::Request::Port::ValueCase::kDeviceType));
          break;
      }
    }

    // Update the YANG parse tree.
    auto poll_functor = [device_type_string](const GnmiEvent& event,
                                           const ::gnmi::Path& path,
                                           GnmiSubscribeStream* stream) {
      // This leaf represents configuration data. Return what was known when
      // it was configured!
      return SendResponse(GetResponse(path, device_type_string), stream);
    };
    node->SetOnTimerHandler(poll_functor)->SetOnPollHandler(poll_functor);

    return ::util::OkStatus();
  };
  node->SetOnTimerHandler(poll_functor)
      ->SetOnPollHandler(poll_functor)
      ->SetOnUpdateHandler(on_set_functor)
      ->SetOnReplaceHandler(on_set_functor);
}

////////////////////////////////////////////////////////////////////////////////
// /interfaces/virtual-interface[name=<name>]/config/pipeline-name
//
void SetUpInterfacesInterfaceConfigPipelineName(
    const char *pipeline_name, uint64 node_id, uint64 port_id,
    TreeNode* node, YangParseTree* tree) {
  auto poll_functor = [pipeline_name](const GnmiEvent& event, const ::gnmi::Path& path,
                              GnmiSubscribeStream* stream) {
    // This leaf represents configuration data. Return what was known when it
    // was configured!
    return SendResponse(GetResponse(path, pipeline_name), stream);
  };
  auto on_set_functor =
      [node_id, port_id, node, tree](
          const ::gnmi::Path& path, const ::google::protobuf::Message& val,
          CopyOnWriteChassisConfig* config) -> ::util::Status {
    auto typed_val = dynamic_cast<const gnmi::TypedValue*>(&val);
    if (typed_val == nullptr) {
      return MAKE_ERROR(ERR_INVALID_PARAM) << "not a TypedValue message!";
    }

    if (IsPortParamSet(tree, node_id, port_id,
                       SetRequest::Request::Port::ValueCase::kPipelineName)) {
      return MAKE_ERROR(ERR_INVALID_PARAM)
          << "pipeline-name is already set or PORT is already configured";
    }

    auto pipeline_name = typed_val->string_val();

    // Set the value.
    SetRequest req;
    auto* request = req.add_requests()->mutable_port();
    request->set_node_id(node_id);
    request->set_port_id(port_id);
    request->SetRequest::Request::Port::mutable_pipeline_name()
        ->PipelineNameConfigured::set_pipeline_name((const char*)pipeline_name.c_str());


    // Update the chassis config
    ChassisConfig* new_config = config->writable();
    for (auto& singleton_port : *new_config->mutable_singleton_ports()) {
      if (singleton_port.node() == node_id && singleton_port.id() == port_id) {
        singleton_port.mutable_config_params()->set_pipeline((const char*)pipeline_name.c_str());

        // Validate if all mandatory params are set and call SDE API
        RETURN_IF_ERROR(
            SetPortParam(tree, node_id, port_id, singleton_port,
                         SetRequest::Request::Port::ValueCase::kPipelineName));
        break;
      }
    }

    // Update the YANG parse tree.
    auto poll_functor = [pipeline_name](const GnmiEvent& event,
                                           const ::gnmi::Path& path,
                                           GnmiSubscribeStream* stream) {
      // This leaf represents configuration data. Return what was known when
      // it was configured!
      return SendResponse(GetResponse(path, pipeline_name), stream);
    };
    node->SetOnTimerHandler(poll_functor)->SetOnPollHandler(poll_functor);

    return ::util::OkStatus();
  };
  node->SetOnTimerHandler(poll_functor)
      ->SetOnPollHandler(poll_functor)
      ->SetOnUpdateHandler(on_set_functor)
      ->SetOnReplaceHandler(on_set_functor);
}

////////////////////////////////////////////////////////////////////////////////
// /interfaces/virtual-interface[name=<name>]/config/mempool-name
//
void SetUpInterfacesInterfaceConfigMempoolName(
    const char *mempool_name, uint64 node_id, uint64 port_id,
    TreeNode* node, YangParseTree* tree) {
  auto poll_functor = [mempool_name](const GnmiEvent& event, const ::gnmi::Path& path,
                              GnmiSubscribeStream* stream) {
    // This leaf represents configuration data. Return what was known when it
    // was configured!
    return SendResponse(GetResponse(path, mempool_name), stream);
  };
  auto on_set_functor =
      [node_id, port_id, node, tree](
          const ::gnmi::Path& path, const ::google::protobuf::Message& val,
          CopyOnWriteChassisConfig* config) -> ::util::Status {
    auto typed_val = dynamic_cast<const gnmi::TypedValue*>(&val);
    if (typed_val == nullptr) {
      return MAKE_ERROR(ERR_INVALID_PARAM) << "not a TypedValue message!";
    }

    if (IsPortParamSet(tree, node_id, port_id,
                       SetRequest::Request::Port::ValueCase::kMempoolName)) {
      return MAKE_ERROR(ERR_INVALID_PARAM)
          << "mempool-name is already set or PORT is already configured";
    }

    auto mempool_name = typed_val->string_val();

    // Set the value.
    SetRequest req;
    auto* request = req.add_requests()->mutable_port();
    request->set_node_id(node_id);
    request->set_port_id(port_id);
    request->SetRequest::Request::Port::mutable_mempool_name()->MempoolNameConfigured::set_mempool_name((const char*)mempool_name.c_str());

    // Update the chassis config
    ChassisConfig* new_config = config->writable();
    for (auto& singleton_port : *new_config->mutable_singleton_ports()) {
      if (singleton_port.node() == node_id && singleton_port.id() == port_id) {
        singleton_port.mutable_config_params()->set_mempool((const char*)mempool_name.c_str());

        // Validate if all mandatory params are set and call SDE API
        RETURN_IF_ERROR(
            SetPortParam(tree, node_id, port_id, singleton_port,
                         SetRequest::Request::Port::ValueCase::kMempoolName));
        break;
      }
    }

    // Update the YANG parse tree.
    auto poll_functor = [mempool_name](const GnmiEvent& event,
                                           const ::gnmi::Path& path,
                                           GnmiSubscribeStream* stream) {
      // This leaf represents configuration data. Return what was known when
      // it was configured!
      return SendResponse(GetResponse(path, mempool_name), stream);
    };
    node->SetOnTimerHandler(poll_functor)->SetOnPollHandler(poll_functor);

    return ::util::OkStatus();
  };
  node->SetOnTimerHandler(poll_functor)
      ->SetOnPollHandler(poll_functor)
      ->SetOnUpdateHandler(on_set_functor)
      ->SetOnReplaceHandler(on_set_functor);
}

////////////////////////////////////////////////////////////////////////////////
// /interfaces/virtual-interface[name=<name>]/config/packet-dir
//
void SetUpInterfacesInterfaceConfigPacketDir(
    uint64 packet_dir, uint64 node_id, uint64 port_id,
    TreeNode* node, YangParseTree* tree) {
  auto poll_functor = [packet_dir](const GnmiEvent& event, const ::gnmi::Path& path,
                              GnmiSubscribeStream* stream) {
    // This leaf represents configuration data. Return what was known when it
    // was configured!
    return SendResponse(GetResponse(path, packet_dir), stream);
  };
  auto on_set_functor =
      [node_id, port_id, node, tree](
          const ::gnmi::Path& path, const ::google::protobuf::Message& val,
          CopyOnWriteChassisConfig* config) -> ::util::Status {
    auto typed_val = dynamic_cast<const gnmi::TypedValue*>(&val);
    if (typed_val == nullptr) {
      return MAKE_ERROR(ERR_INVALID_PARAM) << "not a TypedValue message!";
    }

    if (IsPortParamSet(tree, node_id, port_id,
                       SetRequest::Request::Port::ValueCase::kPacketDir)) {
      return MAKE_ERROR(ERR_INVALID_PARAM)
          << "packet-dir is already set or port is already configured";
    }

    std::string packet_dir_string = typed_val->string_val();
    SWBackendPktDirType direction = DIRECTION_NONE;
    if (packet_dir_string == "network" || packet_dir_string == "NETWORK") {
        direction = SWBackendPktDirType::DIRECTION_NETWORK;
    } else if (packet_dir_string == "host" || packet_dir_string == "HOST") {
        direction = SWBackendPktDirType::DIRECTION_HOST;
    } else {
      return MAKE_ERROR(ERR_INVALID_PARAM)
          << "wrong value for packet-direction: accepted values are "
             "case-insensitive network or host";
    }

    // Set the value.
    auto status = SetValue(node_id, port_id, tree,
                           &SetRequest::Request::Port::mutable_packet_dir,
                           &SWBackendPktDirStatus::set_packet_dir, direction);
    if (status != ::util::OkStatus()) {
      return status;
    }

    // Update the chassis config
    ChassisConfig* new_config = config->writable();
    for (auto& singleton_port : *new_config->mutable_singleton_ports()) {
      if (singleton_port.node() == node_id && singleton_port.id() == port_id) {
        singleton_port.mutable_config_params()->set_packet_dir(direction);

        // Validate if all mandatory params are set and call SDE API
        RETURN_IF_ERROR(
            SetPortParam(tree, node_id, port_id, singleton_port,
                         SetRequest::Request::Port::ValueCase::kPacketDir));
          break;
      }
    }

    // Update the YANG parse tree.
    auto poll_functor = [packet_dir_string](const GnmiEvent& event,
                                           const ::gnmi::Path& path,
                                           GnmiSubscribeStream* stream) {
      // This leaf represents configuration data. Return what was known when
      // it was configured!
      return SendResponse(GetResponse(path, packet_dir_string), stream);
    };
    node->SetOnTimerHandler(poll_functor)->SetOnPollHandler(poll_functor);

    return ::util::OkStatus();
  };
  node->SetOnTimerHandler(poll_functor)
      ->SetOnPollHandler(poll_functor)
      ->SetOnUpdateHandler(on_set_functor)
      ->SetOnReplaceHandler(on_set_functor);
}

////////////////////////////////////////////////////////////////////////////////
// /interfaces/virtual-interface[name=<name>]/config/control-port
//
void SetUpInterfacesInterfaceConfigControlPort(
    const char *control_port, uint64 node_id, uint64 port_id,
    TreeNode* node, YangParseTree* tree) {
  auto poll_functor = [control_port](const GnmiEvent& event, const ::gnmi::Path& path,
                              GnmiSubscribeStream* stream) {
    // This leaf represents configuration data. Return what was known when it
    // was configured!
    return SendResponse(GetResponse(path, control_port), stream);
  };
  auto on_set_functor =
      [node_id, port_id, node, tree](
          const ::gnmi::Path& path, const ::google::protobuf::Message& val,
          CopyOnWriteChassisConfig* config) -> ::util::Status {
    auto typed_val = dynamic_cast<const gnmi::TypedValue*>(&val);
    if (typed_val == nullptr) {
      return MAKE_ERROR(ERR_INVALID_PARAM) << "not a TypedValue message!";
    }

    if (IsPortParamSet(tree, node_id, port_id,
                       SetRequest::Request::Port::ValueCase::kControlPort)) {
        return MAKE_ERROR(ERR_INVALID_PARAM)
            << "control-port is already set or PORT is already configured";
    }

    auto ctl_port = typed_val->string_val();

    // Set the value.
    SetRequest req;
    auto* request = req.add_requests()->mutable_port();
    request->set_node_id(node_id);
    request->set_port_id(port_id);
    request->SetRequest::Request::Port::mutable_control_port()->ControlPortConfigured::set_control_port((const char*)ctl_port.c_str());


    // Update the chassis config
    ChassisConfig* new_config = config->writable();
    for (auto& singleton_port : *new_config->mutable_singleton_ports()) {
      if (singleton_port.node() == node_id && singleton_port.id() == port_id) {
        singleton_port.mutable_config_params()->set_control((const char*)ctl_port.c_str());

        // Validate if all mandatory params are set and call SDE API
        RETURN_IF_ERROR(
            SetPortParam(tree, node_id, port_id, singleton_port,
                         SetRequest::Request::Port::ValueCase::kControlPort));
        break;
      }
    }

    // Update the YANG parse tree.
    auto poll_functor = [ctl_port](const GnmiEvent& event,
                                   const ::gnmi::Path& path,
                                   GnmiSubscribeStream* stream) {
      // This leaf represents configuration data. Return what was known when
      // it was configured!
      return SendResponse(GetResponse(path, ctl_port), stream);
    };
    node->SetOnTimerHandler(poll_functor)->SetOnPollHandler(poll_functor);

    return ::util::OkStatus();
  };
  node->SetOnTimerHandler(poll_functor)
      ->SetOnPollHandler(poll_functor)
      ->SetOnUpdateHandler(on_set_functor)
      ->SetOnReplaceHandler(on_set_functor);
}

////////////////////////////////////////////////////////////////////////////////
// /interfaces/virtual-interface[name=<name>]/config/pci-bdf
//
void SetUpInterfacesInterfaceConfigPciBdf(
    const char *pci_bdf, uint64 node_id, uint64 port_id,
    TreeNode* node, YangParseTree* tree) {
  auto poll_functor = [pci_bdf](const GnmiEvent& event, const ::gnmi::Path& path,
                              GnmiSubscribeStream* stream) {
    // This leaf represents configuration data. Return what was known when it
    // was configured!
    return SendResponse(GetResponse(path, pci_bdf), stream);
  };
  auto on_set_functor =
      [node_id, port_id, node, tree](
          const ::gnmi::Path& path, const ::google::protobuf::Message& val,
          CopyOnWriteChassisConfig* config) -> ::util::Status {
    auto typed_val = dynamic_cast<const gnmi::TypedValue*>(&val);
    if (typed_val == nullptr) {
      return MAKE_ERROR(ERR_INVALID_PARAM) << "not a TypedValue message!";
    }

    if (IsPortParamSet(tree, node_id, port_id,
                       SetRequest::Request::Port::ValueCase::kPciBdf)) {
      return MAKE_ERROR(ERR_INVALID_PARAM)
          << "pci-bdf is already set or PORT is already configured";
    }

    auto bdf_val = typed_val->string_val();

    // Set the value.
    SetRequest req;
    auto* request = req.add_requests()->mutable_port();
    request->set_node_id(node_id);
    request->set_port_id(port_id);
    request->SetRequest::Request::Port::mutable_pci_bdf()->PciBdfConfigured::set_pci_bdf((const char*)bdf_val.c_str());

    // Update the chassis config
    ChassisConfig* new_config = config->writable();
    for (auto& singleton_port : *new_config->mutable_singleton_ports()) {
      if (singleton_port.node() == node_id && singleton_port.id() == port_id) {
        singleton_port.mutable_config_params()->set_pci((const char*)bdf_val.c_str());

        // Validate if all mandatory params are set and call SDE API
        RETURN_IF_ERROR(
            SetPortParam(tree, node_id, port_id, singleton_port,
                         SetRequest::Request::Port::ValueCase::kPciBdf));
        break;
      }
    }

    // Update the YANG parse tree.
    auto poll_functor = [bdf_val](const GnmiEvent& event,
                                  const ::gnmi::Path& path,
                                  GnmiSubscribeStream* stream) {
      // This leaf represents configuration data. Return what was known when
      // it was configured!
      return SendResponse(GetResponse(path, bdf_val), stream);
    };
    node->SetOnTimerHandler(poll_functor)->SetOnPollHandler(poll_functor);

    return ::util::OkStatus();
  };
  node->SetOnTimerHandler(poll_functor)
      ->SetOnPollHandler(poll_functor)
      ->SetOnUpdateHandler(on_set_functor)
      ->SetOnReplaceHandler(on_set_functor);
}

////////////////////////////////////////////////////////////////////////////////
// /interfaces/virtual-interface[name=<name>]/config/mtu
//
void SetUpInterfacesInterfaceConfigMtuValue(
    uint64 mtu, uint64 node_id, uint64 port_id,
    TreeNode* node, YangParseTree* tree) {
  auto poll_functor = [mtu](const GnmiEvent& event, const ::gnmi::Path& path,
                              GnmiSubscribeStream* stream) {
    // This leaf represents configuration data. Return what was known when it
    // was configured!
    return SendResponse(GetResponse(path, mtu), stream);
  };
  auto on_set_functor =
      [node_id, port_id, node, tree](
          const ::gnmi::Path& path, const ::google::protobuf::Message& val,
          CopyOnWriteChassisConfig* config) -> ::util::Status {
    auto typed_val = dynamic_cast<const gnmi::TypedValue*>(&val);
    if (typed_val == nullptr) {
      return MAKE_ERROR(ERR_INVALID_PARAM) << "not a TypedValue message!";
    }

    if (IsPortParamSet(tree, node_id, port_id,
                       SetRequest::Request::Port::ValueCase::kMtuValue)) {
      return MAKE_ERROR(ERR_INVALID_PARAM)
          << "MTU is already set or PORT is already configured";
    }

    auto mtu_val= typed_val->int_val();

    // Set the value.
    auto status = SetValue(node_id, port_id, tree,
                           &SetRequest::Request::Port::mutable_mtu_value,
                           &MtuValue::set_mtu_value, mtu_val);
    if (status != ::util::OkStatus()) {
      return status;
    }

    // Update the chassis config
    ChassisConfig* new_config = config->writable();
    for (auto& singleton_port : *new_config->mutable_singleton_ports()) {
      if (singleton_port.node() == node_id && singleton_port.id() == port_id) {
        singleton_port.mutable_config_params()->set_mtu(mtu_val);

        // Validate if all mandatory params are set and call SDE API
        RETURN_IF_ERROR(
            SetPortParam(tree, node_id, port_id, singleton_port,
                         SetRequest::Request::Port::ValueCase::kMtuValue));
        break;
      }
    }

    // Update the YANG parse tree.
    auto poll_functor = [mtu_val](const GnmiEvent& event,
                                  const ::gnmi::Path& path,
                                  GnmiSubscribeStream* stream) {
      // This leaf represents configuration data. Return what was known when
      // it was configured!
      return SendResponse(GetResponse(path, mtu_val), stream);
    };
    node->SetOnTimerHandler(poll_functor)->SetOnPollHandler(poll_functor);

    return ::util::OkStatus();
  };
  node->SetOnTimerHandler(poll_functor)
      ->SetOnPollHandler(poll_functor)
      ->SetOnUpdateHandler(on_set_functor)
      ->SetOnReplaceHandler(on_set_functor);
}

////////////////////////////////////////////////////////////////////////////////
// /interfaces/virtual-interface[name=<name>]/config/queues
//
void SetUpInterfacesInterfaceConfigQueues(
    uint64 queues_count, uint64 node_id, uint64 port_id,
    TreeNode* node, YangParseTree* tree) {
  auto poll_functor = [queues_count](const GnmiEvent& event,
                                     const ::gnmi::Path& path,
                                     GnmiSubscribeStream* stream) {
    // This leaf represents configuration data.
    // Return what was known when it was configured!
    return SendResponse(GetResponse(path, queues_count), stream);
  };
  auto on_set_functor =
      [node_id, port_id, node, tree](
          const ::gnmi::Path& path, const ::google::protobuf::Message& val,
          CopyOnWriteChassisConfig* config) -> ::util::Status {
    auto typed_val = dynamic_cast<const gnmi::TypedValue*>(&val);
    if (typed_val == nullptr) {
      return MAKE_ERROR(ERR_INVALID_PARAM) << "not a TypedValue message!";
    }

    if (IsPortParamSet(tree, node_id, port_id,
                       SetRequest::Request::Port::ValueCase::kQueueCount)) {
      return MAKE_ERROR(ERR_INVALID_PARAM)
      << "Queues is already set or PORT is already configured";
    }

    auto queues_configured = typed_val->int_val();

    // Set the value.
    auto status = SetValue(node_id, port_id, tree,
                           &SetRequest::Request::Port::mutable_queue_count,
                           &QueuesConfigured::set_queue_count, queues_configured);
    if (status != ::util::OkStatus()) {
      return status;
    }

    // Update the chassis config
    ChassisConfig* new_config = config->writable();
    for (auto& singleton_port : *new_config->mutable_singleton_ports()) {
      if (singleton_port.node() == node_id && singleton_port.id() == port_id) {
        singleton_port.mutable_config_params()->set_queues(queues_configured);

        // Validate if all mandatory params are set and call SDE API
        RETURN_IF_ERROR(
            SetPortParam(tree, node_id, port_id, singleton_port,
                         SetRequest::Request::Port::ValueCase::kQueueCount));
        break;
      }
    }

    // Update the YANG parse tree.
    auto poll_functor = [queues_configured](const GnmiEvent& event,
                                            const ::gnmi::Path& path,
                                            GnmiSubscribeStream* stream) {
      // This leaf represents configuration data. Return what was known when
      // it was configured!
      return SendResponse(GetResponse(path, queues_configured), stream);
    };
    node->SetOnTimerHandler(poll_functor)->SetOnPollHandler(poll_functor);

    return ::util::OkStatus();
  };
  node->SetOnTimerHandler(poll_functor)
      ->SetOnPollHandler(poll_functor)
      ->SetOnUpdateHandler(on_set_functor)
      ->SetOnReplaceHandler(on_set_functor);
}

////////////////////////////////////////////////////////////////////////////////
// /interfaces/virtual-interface[name=<name>]/config/socket
//
void SetUpInterfacesInterfaceConfigSocket(
    const char *default_path, uint64 node_id, uint64 port_id,
    TreeNode* node, YangParseTree* tree) {
  auto poll_functor = [default_path](const GnmiEvent& event,
                                     const ::gnmi::Path& path,
                                     GnmiSubscribeStream* stream) {
    // This leaf represents configuration data. Return what was known when it
    // was configured!
    return SendResponse(GetResponse(path, default_path), stream);
  };

  auto on_set_functor =
      [node_id, port_id, node, tree](
          const ::gnmi::Path& path, const ::google::protobuf::Message& val,
          CopyOnWriteChassisConfig* config) -> ::util::Status {
    auto typed_val = dynamic_cast<const gnmi::TypedValue*>(&val);
    if (typed_val == nullptr) {
      return MAKE_ERROR(ERR_INVALID_PARAM) << "not a TypedValue message!";
    }

    if (IsPortParamSet(tree, node_id, port_id,
                       SetRequest::Request::Port::ValueCase::kSockPath)) {
      return MAKE_ERROR(ERR_INVALID_PARAM)
      << "Socket is already set or PORT is already configured";
    }

    auto socket_path = typed_val->string_val();

    // Set the value.
    SetRequest req;
    auto* request = req.add_requests()->mutable_port();
    request->set_node_id(node_id);
    request->set_port_id(port_id);
    request->SetRequest::Request::Port::mutable_sock_path()->SocketPathConfigured::set_sock_path((const char*)socket_path.c_str());

    // Update the chassis config
    ChassisConfig* new_config = config->writable();
    for (auto& singleton_port : *new_config->mutable_singleton_ports()) {
      if (singleton_port.node() == node_id && singleton_port.id() == port_id) {
        singleton_port.mutable_config_params()->set_socket((const char*)socket_path.c_str());

        RETURN_IF_ERROR(
            SetPortParam(tree, node_id, port_id, singleton_port,
                         SetRequest::Request::Port::ValueCase::kSockPath));
        break;
      }
    }

    // Update the YANG parse tree.
    auto poll_functor = [socket_path](const GnmiEvent& event,
                                      const ::gnmi::Path& path,
                                      GnmiSubscribeStream* stream) {
      // This leaf represents configuration data. Return what was known when
      // it was configured!
      return SendResponse(GetResponse(path, socket_path), stream);
    };
    node->SetOnTimerHandler(poll_functor)->SetOnPollHandler(poll_functor);

    return ::util::OkStatus();
  };

  node->SetOnTimerHandler(poll_functor)
      ->SetOnPollHandler(poll_functor)
      ->SetOnUpdateHandler(on_set_functor)
      ->SetOnReplaceHandler(on_set_functor);
}

////////////////////////////////////////////////////////////////////////////////
// /interfaces/virtual-interface[name=<name>]/config/qemu-socket-ip
//
void SetUpInterfacesInterfaceConfigQemuSocketIp(
    const char *default_socket_ip, uint64 node_id, uint64 port_id,
    TreeNode* node, YangParseTree* tree) {
  auto poll_functor = [default_socket_ip](const GnmiEvent& event,
                                          const ::gnmi::Path& path,
                                          GnmiSubscribeStream* stream) {
    // This leaf represents configuration data. Return what was known when it
    // was configured!
    return SendResponse(GetResponse(path, default_socket_ip), stream);
  };

  auto on_set_functor =
      [node_id, port_id, node, tree](
          const ::gnmi::Path& path, const ::google::protobuf::Message& val,
          CopyOnWriteChassisConfig* config) -> ::util::Status {
    auto typed_val = dynamic_cast<const gnmi::TypedValue*>(&val);
    if (typed_val == nullptr) {
      return MAKE_ERROR(ERR_INVALID_PARAM) << "not a TypedValue message!";
    }

    auto socket_ip = typed_val->string_val();

    // Set the value.
    SetRequest req;
    auto* request = req.add_requests()->mutable_port();
    request->set_node_id(node_id);
    request->set_port_id(port_id);
    request->SetRequest::Request::Port::mutable_hotplug_config()->HotplugConfig::set_qemu_socket_ip((const char*)socket_ip.c_str());

    // Update the chassis config
    ChassisConfig* new_config = config->writable();
    for (auto& singleton_port : *new_config->mutable_singleton_ports()) {
      if (singleton_port.node() == node_id && singleton_port.id() == port_id) {
        singleton_port.mutable_config_params()->mutable_hotplug_config()->set_qemu_socket_ip((const char*)socket_ip.c_str());
        // Validate if all mandatory params are set and call SDE API
        RETURN_IF_ERROR(
            SetHotplugParam(
                tree, node_id, port_id, singleton_port,
                SetRequest::Request::Port::ValueCase::kHotplugConfig,
                PARAM_SOCK_IP));
        break;
      }
    }


    // Update the YANG parse tree.
    auto poll_functor = [socket_ip](const GnmiEvent& event,
                                    const ::gnmi::Path& path,
                                    GnmiSubscribeStream* stream) {
      // This leaf represents configuration data. Return what was known when
      // it was configured!
      return SendResponse(GetResponse(path, socket_ip), stream);
    };
    node->SetOnTimerHandler(poll_functor)->SetOnPollHandler(poll_functor);

    return ::util::OkStatus();
  };
  node->SetOnTimerHandler(poll_functor)
      ->SetOnPollHandler(poll_functor)
      ->SetOnUpdateHandler(on_set_functor)
      ->SetOnReplaceHandler(on_set_functor);
}

////////////////////////////////////////////////////////////////////////////////
// /interfaces/virtual-interface[name=<name>]/config/qemu-socket-port
//
void SetUpInterfacesInterfaceConfigQemuSocketPort(
    uint64 default_socket_port, uint64 node_id, uint64 port_id,
    TreeNode* node, YangParseTree* tree) {
  auto poll_functor = [default_socket_port](const GnmiEvent& event, const ::gnmi::Path& path,
                              GnmiSubscribeStream* stream) {
    // This leaf represents configuration data. Return what was known when it
    // was configured!
    return SendResponse(GetResponse(path, default_socket_port), stream);
  };
  auto on_set_functor =
      [node_id, port_id, node, tree](
          const ::gnmi::Path& path, const ::google::protobuf::Message& val,
          CopyOnWriteChassisConfig* config) -> ::util::Status {
    auto typed_val = dynamic_cast<const gnmi::TypedValue*>(&val);
    if (typed_val == nullptr) {
      return MAKE_ERROR(ERR_INVALID_PARAM) << "not a TypedValue message!";
    }

    auto socket_port = typed_val->int_val();

    // Set the value.
    SetRequest req;
    auto* request = req.add_requests()->mutable_port();
    request->set_node_id(node_id);
    request->set_port_id(port_id);

    request->SetRequest::Request::Port::mutable_hotplug_config()->HotplugConfig::set_qemu_socket_port(socket_port);

    // Update the chassis config
    ChassisConfig* new_config = config->writable();
    for (auto& singleton_port : *new_config->mutable_singleton_ports()) {
      if (singleton_port.node() == node_id && singleton_port.id() == port_id) {
        singleton_port.mutable_config_params()->mutable_hotplug_config()->set_qemu_socket_port(socket_port);

        // Validate if all mandatory params are set and call SDE API
        RETURN_IF_ERROR(
            SetHotplugParam(
                tree, node_id, port_id, singleton_port,
                SetRequest::Request::Port::ValueCase::kHotplugConfig,
                PARAM_SOCK_PORT));

        break;
      }
    }

    // Update the YANG parse tree.
    auto poll_functor = [socket_port](const GnmiEvent& event,
                                      const ::gnmi::Path& path,
                                      GnmiSubscribeStream* stream) {
      // This leaf represents configuration data. Return what was known when
      // it was configured!
      return SendResponse(GetResponse(path, socket_port), stream);
    };
    node->SetOnTimerHandler(poll_functor)->SetOnPollHandler(poll_functor);

    return ::util::OkStatus();
  };
  node->SetOnTimerHandler(poll_functor)
      ->SetOnPollHandler(poll_functor)
      ->SetOnUpdateHandler(on_set_functor)
      ->SetOnReplaceHandler(on_set_functor);
}

////////////////////////////////////////////////////////////////////////////////
// /interfaces/virtual-interface[name=<name>]/config/hotplug
//
void SetUpInterfacesInterfaceConfigHotplug(
    uint64 status, uint64 node_id, uint64 port_id,
    TreeNode* node, YangParseTree* tree) {
  auto poll_functor = [status](const GnmiEvent& event, const ::gnmi::Path& path,
                              GnmiSubscribeStream* stream) {
    // This leaf represents configuration data. Return what was known when it
    // was configured!
    return SendResponse(GetResponse(path, status), stream);
  };
  auto on_set_functor =
      [node_id, port_id, node, tree](
          const ::gnmi::Path& path, const ::google::protobuf::Message& val,
          CopyOnWriteChassisConfig* config) -> ::util::Status {
    auto typed_val = dynamic_cast<const gnmi::TypedValue*>(&val);
    if (typed_val == nullptr) {
      return MAKE_ERROR(ERR_INVALID_PARAM) << "not a TypedValue message!";
    }

    std::string status_string = typed_val->string_val();
    SWBackendQemuHotplugStatus hotplug_status = NO_HOTPLUG;
    if (status_string == "add" || status_string == "ADD") {
      hotplug_status = SWBackendQemuHotplugStatus::HOTPLUG_ADD;
    } else if (status_string == "del" || status_string == "DEL") {
        hotplug_status = SWBackendQemuHotplugStatus::HOTPLUG_DEL;
    } else {
      return MAKE_ERROR(ERR_INVALID_PARAM)
          << "wrong value for qemu hotplug: supported values are ADD and DEL!";
    }

    // Set the value.
    SetRequest req;
    auto* request = req.add_requests()->mutable_port();
    request->set_node_id(node_id);
    request->set_port_id(port_id);

    request->SetRequest::Request::Port::mutable_hotplug_config()->HotplugConfig::set_qemu_hotplug(hotplug_status);

    // Update the chassis config
    ChassisConfig* new_config = config->writable();
    for (auto& singleton_port : *new_config->mutable_singleton_ports()) {
      if (singleton_port.node() == node_id && singleton_port.id() == port_id) {
        singleton_port.mutable_config_params()->mutable_hotplug_config()->set_qemu_hotplug(hotplug_status);

        // Validate if all mandatory params are set and call SDE API
        RETURN_IF_ERROR(
            SetHotplugParam(
                tree, node_id, port_id, singleton_port,
                SetRequest::Request::Port::ValueCase::kHotplugConfig,
                PARAM_HOTPLUG));

        break;
      }
    }

    // Update the YANG parse tree.
    auto poll_functor = [status_string](const GnmiEvent& event,
                                        const ::gnmi::Path& path,
                                        GnmiSubscribeStream* stream) {
      // This leaf represents configuration data. Return what was known when
      // it was configured!
      return SendResponse(GetResponse(path, status_string), stream);
    };
    node->SetOnTimerHandler(poll_functor)->SetOnPollHandler(poll_functor);

    return ::util::OkStatus();
  };
  node->SetOnTimerHandler(poll_functor)
      ->SetOnPollHandler(poll_functor)
      ->SetOnUpdateHandler(on_set_functor)
      ->SetOnReplaceHandler(on_set_functor);
}

////////////////////////////////////////////////////////////////////////////////
// /interfaces/virtual-interface[name=<name>]/ethernet/config/qemu-vm-mac-address
void SetUpInterfacesInterfaceConfigQemuVmMacAddress(
    uint64 node_id, uint32 port_id, uint64 mac_address,
    TreeNode* node, YangParseTree* tree) {
  auto poll_functor = [mac_address](const GnmiEvent& event,
                                    const ::gnmi::Path& path,
                                    GnmiSubscribeStream* stream) {
    // This leaf represents configuration data. Return what was known when it
    // was configured!
    return SendResponse(GetResponse(path, MacAddressToYangString(mac_address)),
                        stream);
  };

  auto on_set_functor =
      [node_id, port_id, node, tree](
          const ::gnmi::Path& path, const ::google::protobuf::Message& val,
          CopyOnWriteChassisConfig* config) -> ::util::Status {
    auto typed_val = dynamic_cast<const gnmi::TypedValue*>(&val);
    if (typed_val == nullptr) {
      return MAKE_ERROR(ERR_INVALID_PARAM) << "not a TypedValue message!";
    }
    std::string mac_address_string = typed_val->string_val();
    if (!IsMacAddressValid(mac_address_string)) {
      return MAKE_ERROR(ERR_INVALID_PARAM) << "wrong value!";
    }

    uint64 mac_address = YangStringToMacAddress(mac_address_string);
    // Set the value.
    SetRequest req;
    auto* request = req.add_requests()->mutable_port();
    request->set_node_id(node_id);
    request->set_port_id(port_id);

    request->SetRequest::Request::Port::mutable_hotplug_config()->HotplugConfig::set_qemu_vm_mac_address(mac_address);

    // Update the chassis config
    ChassisConfig* new_config = config->writable();
    for (auto& singleton_port : *new_config->mutable_singleton_ports()) {
      if (singleton_port.node() == node_id && singleton_port.id() == port_id) {
        singleton_port.mutable_config_params()->mutable_hotplug_config()->set_qemu_vm_mac_address(mac_address);

        // Validate if all mandatory params are set and call SDE API
        RETURN_IF_ERROR(
            SetHotplugParam(
                tree, node_id, port_id, singleton_port,
                SetRequest::Request::Port::ValueCase::kHotplugConfig,
                PARAM_VM_MAC));
        break;
      }
    }

    // Update the YANG parse tree.
    auto poll_functor = [mac_address](const GnmiEvent& event,
                                      const ::gnmi::Path& path,
                                      GnmiSubscribeStream* stream) {
      // This leaf represents configuration data. Return what was known when it
      // was configured!
      return SendResponse(
          GetResponse(path, MacAddressToYangString(mac_address)), stream);
    };
    node->SetOnTimerHandler(poll_functor)->SetOnPollHandler(poll_functor);

    return ::util::OkStatus();
  };
  node->SetOnTimerHandler(poll_functor)
      ->SetOnPollHandler(poll_functor)
      ->SetOnUpdateHandler(on_set_functor)
      ->SetOnReplaceHandler(on_set_functor);
}

////////////////////////////////////////////////////////////////////////////////
// /interfaces/virtual-interface[name=<name>]/config/qemu-vm-netdev-id
//
void SetUpInterfacesInterfaceConfigQemuVmNetdevId(
    const char *default_netdev_id, uint64 node_id, uint64 port_id,
    TreeNode* node, YangParseTree* tree) {
  auto poll_functor = [default_netdev_id](const GnmiEvent& event,
                                          const ::gnmi::Path& path,
                                          GnmiSubscribeStream* stream) {
    // This leaf represents configuration data. Return what was known when it
    // was configured!
    return SendResponse(GetResponse(path, default_netdev_id), stream);
  };
  auto on_set_functor =
      [node_id, port_id, node, tree](
          const ::gnmi::Path& path, const ::google::protobuf::Message& val,
          CopyOnWriteChassisConfig* config) -> ::util::Status {
    auto typed_val = dynamic_cast<const gnmi::TypedValue*>(&val);
    if (typed_val == nullptr) {
      return MAKE_ERROR(ERR_INVALID_PARAM) << "not a TypedValue message!";
    }

    auto vm_netdev_id = typed_val->string_val();

    // Set the value.
    SetRequest req;
    auto* request = req.add_requests()->mutable_port();
    request->set_node_id(node_id);
    request->set_port_id(port_id);
    request->SetRequest::Request::Port::mutable_hotplug_config()->HotplugConfig::set_qemu_vm_netdev_id((const char*)vm_netdev_id.c_str());


    // Update the chassis config
    ChassisConfig* new_config = config->writable();
    for (auto& singleton_port : *new_config->mutable_singleton_ports()) {
      if (singleton_port.node() == node_id && singleton_port.id() == port_id) {
        singleton_port.mutable_config_params()->mutable_hotplug_config()->set_qemu_vm_netdev_id((const char*)vm_netdev_id.c_str());

        // Validate if all mandatory params are set and call SDE API
        RETURN_IF_ERROR(
            SetHotplugParam(
                tree, node_id, port_id, singleton_port,
                SetRequest::Request::Port::ValueCase::kHotplugConfig,
                PARAM_NETDEV_ID));
        break;
      }
    }

    // Update the YANG parse tree.
    auto poll_functor = [vm_netdev_id](const GnmiEvent& event,
                                       const ::gnmi::Path& path,
                                       GnmiSubscribeStream* stream) {
      // This leaf represents configuration data. Return what was known when
      // it was configured!
      return SendResponse(GetResponse(path, vm_netdev_id), stream);
    };
    node->SetOnTimerHandler(poll_functor)->SetOnPollHandler(poll_functor);

    return ::util::OkStatus();
  };
  node->SetOnTimerHandler(poll_functor)
      ->SetOnPollHandler(poll_functor)
      ->SetOnUpdateHandler(on_set_functor)
      ->SetOnReplaceHandler(on_set_functor);
}

////////////////////////////////////////////////////////////////////////////////
// /interfaces/virtual-interface[name=<name>]/config/qemu-vm-chardev-id
//
void SetUpInterfacesInterfaceConfigQemuVmChardevId(
    const char *default_chardev_id, uint64 node_id, uint64 port_id,
    TreeNode* node, YangParseTree* tree) {
  auto poll_functor = [default_chardev_id](const GnmiEvent& event,
                                           const ::gnmi::Path& path,
                                           GnmiSubscribeStream* stream) {
    // This leaf represents configuration data. Return what was known when it
    // was configured!
    return SendResponse(GetResponse(path, default_chardev_id), stream);
  };
  auto on_set_functor =
      [node_id, port_id, node, tree](
          const ::gnmi::Path& path, const ::google::protobuf::Message& val,
          CopyOnWriteChassisConfig* config) -> ::util::Status {
    auto typed_val = dynamic_cast<const gnmi::TypedValue*>(&val);
    if (typed_val == nullptr) {
      return MAKE_ERROR(ERR_INVALID_PARAM) << "not a TypedValue message!";
    }

    auto vm_chardev_id = typed_val->string_val();

    // Set the value.
    SetRequest req;
    auto* request = req.add_requests()->mutable_port();
    request->set_node_id(node_id);
    request->set_port_id(port_id);
    request->SetRequest::Request::Port::mutable_hotplug_config()->HotplugConfig::set_qemu_vm_chardev_id((const char*)vm_chardev_id.c_str());

    // Update the chassis config
    ChassisConfig* new_config = config->writable();
    for (auto& singleton_port : *new_config->mutable_singleton_ports()) {
      if (singleton_port.node() == node_id && singleton_port.id() == port_id) {
        singleton_port.mutable_config_params()->mutable_hotplug_config()->set_qemu_vm_chardev_id((const char*)vm_chardev_id.c_str());

        // Validate if all mandatory params are set and call SDE API
        RETURN_IF_ERROR(
            SetHotplugParam(
                tree, node_id, port_id, singleton_port,
                SetRequest::Request::Port::ValueCase::kHotplugConfig,
                PARAM_CHARDEV_ID));
        break;
      }
    }

    // Update the YANG parse tree.
    auto poll_functor = [vm_chardev_id](const GnmiEvent& event,
                                        const ::gnmi::Path& path,
                                        GnmiSubscribeStream* stream) {
      // This leaf represents configuration data. Return what was known when
      // it was configured!
      return SendResponse(GetResponse(path, vm_chardev_id), stream);
    };
    node->SetOnTimerHandler(poll_functor)->SetOnPollHandler(poll_functor);

    return ::util::OkStatus();
  };
  node->SetOnTimerHandler(poll_functor)
      ->SetOnPollHandler(poll_functor)
      ->SetOnUpdateHandler(on_set_functor)
      ->SetOnReplaceHandler(on_set_functor);
}

////////////////////////////////////////////////////////////////////////////////
// /interfaces/virtual-interface[name=<name>]/config/qemu-vm-device-id
//
void SetUpInterfacesInterfaceConfigQemuVmDeviceId(
    const char *default_device_id, uint64 node_id, uint64 port_id,
    TreeNode* node, YangParseTree* tree) {
  auto poll_functor = [default_device_id](const GnmiEvent& event,
                                          const ::gnmi::Path& path,
                                          GnmiSubscribeStream* stream) {
    // This leaf represents configuration data. Return what was known when it
    // was configured!
    return SendResponse(GetResponse(path, default_device_id), stream);
  };
  auto on_set_functor =
      [node_id, port_id, node, tree](
          const ::gnmi::Path& path, const ::google::protobuf::Message& val,
          CopyOnWriteChassisConfig* config) -> ::util::Status {
    auto typed_val = dynamic_cast<const gnmi::TypedValue*>(&val);
    if (typed_val == nullptr) {
      return MAKE_ERROR(ERR_INVALID_PARAM) << "not a TypedValue message!";
    }

    auto vm_device_id = typed_val->string_val();

    // Set the value.
    SetRequest req;
    auto* request = req.add_requests()->mutable_port();
    request->set_node_id(node_id);
    request->set_port_id(port_id);
    request->SetRequest::Request::Port::mutable_hotplug_config()->HotplugConfig::set_qemu_vm_device_id((const char*)vm_device_id.c_str());

    // Update the chassis config
    ChassisConfig* new_config = config->writable();
    for (auto& singleton_port : *new_config->mutable_singleton_ports()) {
      if (singleton_port.node() == node_id && singleton_port.id() == port_id) {
        singleton_port.mutable_config_params()->mutable_hotplug_config()->set_qemu_vm_device_id((const char*)vm_device_id.c_str());

        // Validate if all mandatory params are set and call SDE API
        RETURN_IF_ERROR(
            SetHotplugParam(
                tree, node_id, port_id, singleton_port,
                SetRequest::Request::Port::ValueCase::kHotplugConfig,
                PARAM_DEVICE_ID));
        break;
      }
    }

    // Update the YANG parse tree.
    auto poll_functor = [vm_device_id](const GnmiEvent& event,
                                       const ::gnmi::Path& path,
                                       GnmiSubscribeStream* stream) {
      // This leaf represents configuration data. Return what was known when
      // it was configured!
      return SendResponse(GetResponse(path, vm_device_id), stream);
    };
    node->SetOnTimerHandler(poll_functor)->SetOnPollHandler(poll_functor);

    return ::util::OkStatus();
  };
  node->SetOnTimerHandler(poll_functor)
      ->SetOnPollHandler(poll_functor)
      ->SetOnUpdateHandler(on_set_functor)
      ->SetOnReplaceHandler(on_set_functor);
}

////////////////////////////////////////////////////////////////////////////////
// /interfaces/virtual-interface[name=<name>]/config/native-socket-path
//
void SetUpInterfacesInterfaceConfigNativeSocket(
    const char *default_native_path, uint64 node_id, uint64 port_id,
    TreeNode* node, YangParseTree* tree) {
  auto poll_functor = [default_native_path](const GnmiEvent& event,
                                            const ::gnmi::Path& path,
                                            GnmiSubscribeStream* stream) {
    // This leaf represents configuration data. Return what was known when it
    // was configured!
    return SendResponse(GetResponse(path, default_native_path), stream);
  };

  auto on_set_functor =
      [node_id, port_id, node, tree](
          const ::gnmi::Path& path, const ::google::protobuf::Message& val,
          CopyOnWriteChassisConfig* config) -> ::util::Status {
    auto typed_val = dynamic_cast<const gnmi::TypedValue*>(&val);
    if (typed_val == nullptr) {
      return MAKE_ERROR(ERR_INVALID_PARAM) << "not a TypedValue message!";
    }

    auto native_socket_path = typed_val->string_val();

    // Set the value.
    SetRequest req;
    auto* request = req.add_requests()->mutable_port();
    request->set_node_id(node_id);
    request->set_port_id(port_id);
    request->SetRequest::Request::Port::mutable_hotplug_config()->HotplugConfig::set_native_socket_path((const char*)native_socket_path.c_str());

    // Update the chassis config
    ChassisConfig* new_config = config->writable();
    for (auto& singleton_port : *new_config->mutable_singleton_ports()) {
      if (singleton_port.node() == node_id && singleton_port.id() == port_id) {
        singleton_port.mutable_config_params()->mutable_hotplug_config()->set_native_socket_path((const char*)native_socket_path.c_str());

          // Validate if all mandatory params are set and call SDE API
        RETURN_IF_ERROR(
            SetHotplugParam(
                tree, node_id, port_id, singleton_port,
                SetRequest::Request::Port::ValueCase::kHotplugConfig,
                PARAM_NATIVE_SOCK_PATH));
        break;
      }
    }

    // Update the YANG parse tree.
    auto poll_functor = [native_socket_path](const GnmiEvent& event,
                                             const ::gnmi::Path& path,
                                             GnmiSubscribeStream* stream) {
      // This leaf represents configuration data. Return what was known when
      // it was configured!
      return SendResponse(GetResponse(path, native_socket_path), stream);
    };
    node->SetOnTimerHandler(poll_functor)->SetOnPollHandler(poll_functor);

    return ::util::OkStatus();
  };

  node->SetOnTimerHandler(poll_functor)
      ->SetOnPollHandler(poll_functor)
      ->SetOnUpdateHandler(on_set_functor)
      ->SetOnReplaceHandler(on_set_functor);
}

////////////////////////////////////////////////////////////////////////////////
// /interfaces/virtual-interface[name=<name>]/config/tdi-portin-id
void SetUpInterfacesInterfaceConfigTdiPortinId(
    uint32 node_id, uint32 port_id, TreeNode* node, YangParseTree* tree) {
  // Returns the Target Dp Index (tdi_portin_id) for the interface to be used by P4Runtime.
  auto on_poll_functor = GetOnPollFunctor(
      node_id, port_id, tree, &DataResponse::target_dp_id,
      &DataResponse::has_target_dp_id,
      &DataRequest::Request::mutable_target_dp_id,
      &TargetDatapathId::tdi_portin_id);
  auto on_change_functor = UnsupportedFunc();
  node->SetOnTimerHandler(on_poll_functor)
      ->SetOnPollHandler(on_poll_functor)
      ->SetOnChangeHandler(on_change_functor);
}

////////////////////////////////////////////////////////////////////////////////
// /interfaces/virtual-interface[name=<name>]/config/tdi-portout-id
void SetUpInterfacesInterfaceConfigTdiPortoutId(
    uint32 node_id, uint32 port_id, TreeNode* node, YangParseTree* tree) {
  // Returns the Target Dp Index (tdi_portout_id) for the interface to be used by P4Runtime.
  auto on_poll_functor = GetOnPollFunctor(
      node_id, port_id, tree, &DataResponse::target_dp_id,
      &DataResponse::has_target_dp_id,
      &DataRequest::Request::mutable_target_dp_id,
      &TargetDatapathId::tdi_portout_id);
  auto on_change_functor = UnsupportedFunc();
  node->SetOnTimerHandler(on_poll_functor)
      ->SetOnPollHandler(on_poll_functor)
      ->SetOnChangeHandler(on_change_functor);
}

}  // namespace interface
}  // namespace yang
}  // namespace hal
}  // namespace stratum
