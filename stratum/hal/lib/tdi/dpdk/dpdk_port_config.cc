// Copyright 2021-2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "stratum/hal/lib/tdi/dpdk/dpdk_port_config.h"

#include <cstdint>
#include <ostream>

#include "glog/logging.h"
#include "stratum/glue/status/status.h"
#include "stratum/hal/lib/common/common.pb.h"
#include "stratum/hal/lib/tdi/dpdk/dpdk_port_constants.h"

namespace stratum {
namespace hal {
namespace tdi {

// Returns the parameter mask for a ValueCase, or zero if the
// ValueCase is not known.
uint32_t DpdkPortConfig::ParamMaskForCase(ValueCase value_case) {
  switch (value_case) {
  case ValueCase::kPortType:
    return GNMI_CONFIG_PORT_TYPE;
  case ValueCase::kDeviceType:
    return GNMI_CONFIG_DEVICE_TYPE;
  case ValueCase::kQueueCount:
    return GNMI_CONFIG_QUEUE_COUNT;
  case ValueCase::kSockPath:
    return GNMI_CONFIG_SOCKET_PATH;

  case ValueCase::kHostConfig:
    return GNMI_CONFIG_HOST_NAME;
  case ValueCase::kPipelineName:
    return GNMI_CONFIG_PIPELINE_NAME;
  case ValueCase::kMempoolName:
    return GNMI_CONFIG_MEMPOOL_NAME;
  case ValueCase::kMtuValue:
    return GNMI_CONFIG_MTU_VALUE;

  case ValueCase::kPciBdf:
    return GNMI_CONFIG_PCI_BDF_VALUE;
  case ValueCase::kPacketDir:
    return GNMI_CONFIG_PACKET_DIR;
  default:
    return 0;
  }
}

::util::Status DpdkPortConfig::SetParam(
    ValueCase value_case,
    const SingletonPort& singleton_port) {

  const auto& config_params = singleton_port.config_params();

  switch (value_case) {
    case ValueCase::kPortType:
      params_set |= GNMI_CONFIG_PORT_TYPE;
      cfg.port_type = config_params.port_type();
      LOG(INFO) << "PortParam::kPortType = " << cfg.port_type;
      break;

    case ValueCase::kDeviceType:
      params_set |= GNMI_CONFIG_DEVICE_TYPE;
      cfg.device_type = config_params.device_type();
      LOG(INFO) << "PortParam::kDeviceType = " << cfg.device_type;
      break;

    case ValueCase::kQueueCount:
      params_set |= GNMI_CONFIG_QUEUE_COUNT;
      cfg.queues = config_params.queues();
      LOG(INFO) << "PortParam::kQueueCount = " << cfg.queues;
      break;

    case ValueCase::kSockPath:
      params_set |= GNMI_CONFIG_SOCKET_PATH;
      cfg.socket_path = config_params.socket_path();
      LOG(INFO) << "PortParam::kSockPath = " << cfg.socket_path;
      break;

    case ValueCase::kHostConfig:
      params_set |= GNMI_CONFIG_HOST_NAME;
      cfg.host_name = config_params.host_name();
      LOG(INFO) << "PortParam::kHostConfig = " << cfg.host_name;
      break;

    case ValueCase::kPipelineName:
      cfg.pipeline_name = config_params.pipeline_name();
      params_set |= GNMI_CONFIG_PIPELINE_NAME;
      LOG(INFO) << "PortParam::kPipelineName = " << cfg.pipeline_name;
      break;

    case ValueCase::kMempoolName:
      cfg.mempool_name = config_params.mempool_name();
      params_set |= GNMI_CONFIG_MEMPOOL_NAME;
      LOG(INFO) << "PortParam::kMempoolName = " << cfg.mempool_name;
      break;

    case ValueCase::kControlPort:
      control_port = config_params.control_port();
      LOG(INFO) << "PortParam::kControlPort = " << control_port;
      break;

    case ValueCase::kPciBdf:
      params_set |= GNMI_CONFIG_PCI_BDF_VALUE;
      cfg.pci_bdf = config_params.pci_bdf();
      LOG(INFO) << "PortParam::kPciBdf = " << cfg.pci_bdf;
      break;

    case ValueCase::kMtuValue:
      if (config_params.mtu() > MAX_MTU) {
        return MAKE_ERROR(ERR_INVALID_PARAM)
             << "Unsupported MTU = " << config_params.mtu()
             << ". MTU should be less than " << MAX_MTU << ".";
      }
      cfg.mtu = config_params.mtu();
      params_set |= GNMI_CONFIG_MTU_VALUE;
      LOG(INFO) << "PortParam::kMtuValue = " << cfg.mtu;
      break;

    case ValueCase::kPacketDir:
      params_set |= GNMI_CONFIG_PACKET_DIR;
      cfg.packet_dir = config_params.packet_dir();
      LOG(INFO) << "PortParam::kPacketDir = " << cfg.packet_dir;
      break;

    default:
      break;
  }
  return ::util::OkStatus();
}

::util::Status DpdkPortConfig::SetHotplugParam(
  DpdkHotplugParam param_type, const SingletonPort& singleton_port) {

  const auto& params = singleton_port.config_params().hotplug_config();

  switch (param_type) {
    case PARAM_SOCK_IP:
      params_set |= GNMI_CONFIG_HOTPLUG_SOCKET_IP;
      hotplug.qemu_socket_ip = params.qemu_socket_ip();
      LOG(INFO) << "HotplugParam::qemu_socket_ip = " << hotplug.qemu_socket_ip;
      break;

    case PARAM_SOCK_PORT:
      params_set |= GNMI_CONFIG_HOTPLUG_SOCKET_PORT;
      hotplug.qemu_socket_port = params.qemu_socket_port();
      LOG(INFO) << "HotplugParam::qemu_socket_port = "
                << hotplug.qemu_socket_port;
      break;

    case PARAM_HOTPLUG_MODE:
      params_set |= GNMI_CONFIG_HOTPLUG_MODE;
      hotplug.qemu_hotplug_mode = params.qemu_hotplug_mode();
      LOG(INFO) << "HotplugParam::qemu_hotplug_mode = "
                << hotplug.qemu_hotplug_mode;
      break;

    case PARAM_VM_MAC:
      params_set |= GNMI_CONFIG_HOTPLUG_VM_MAC_ADDRESS;
      hotplug.qemu_vm_mac_address = params.qemu_vm_mac_address();
      LOG(INFO) << "HotplugParam::qemu_vm_mac_address = "
                << hotplug.qemu_vm_mac_address;
      break;

    case PARAM_NETDEV_ID:
      params_set |= GNMI_CONFIG_HOTPLUG_VM_NETDEV_ID;
      hotplug.qemu_vm_netdev_id = params.qemu_vm_netdev_id();
      LOG(INFO) << "HotplugParam::qemu_vm_netdev_id = "
                << hotplug.qemu_vm_netdev_id;
      break;

    case PARAM_CHARDEV_ID:
      params_set |= GNMI_CONFIG_HOTPLUG_VM_CHARDEV_ID;
      hotplug.qemu_vm_chardev_id = params.qemu_vm_chardev_id();
      LOG(INFO) << "HotplugParam::qemu_vm_chardev_id = "
                << hotplug.qemu_vm_chardev_id;
      break;

    case PARAM_NATIVE_SOCK_PATH:
      params_set |= GNMI_CONFIG_NATIVE_SOCKET_PATH;
      hotplug.native_socket_path = params.native_socket_path();
      LOG(INFO) << "HotplugParam::qemu_native_socket_path = "
                << hotplug.native_socket_path;
      break;

    case PARAM_DEVICE_ID:
      params_set |= GNMI_CONFIG_HOTPLUG_VM_DEVICE_ID;
      hotplug.qemu_vm_device_id = params.qemu_vm_device_id();
      LOG(INFO) << "HotplugParam::qemu_vm_device_id = "
                << hotplug.qemu_vm_device_id;
      break;

    default:
      break;
  }
  return ::util::OkStatus();
}

}  // namespace tdi
}  // namespace hal
}  // namespace stratum
