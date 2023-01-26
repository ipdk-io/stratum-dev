// Copyright 2018-2019 Barefoot Networks, Inc.
// Copyright 2020-present Open Networking Foundation
// Copyright 2022-2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "stratum/hal/bin/tdi/dpdk/dpdk_main.h"

#include <map>
#include <memory>
#include <ostream>
#include <string>

#include "absl/synchronization/notification.h"
#include "gflags/gflags.h"
#include "stratum/glue/init_google.h"
#include "stratum/glue/logging.h"
#include "stratum/glue/status/status_macros.h"
#include "stratum/glue/status/statusor.h"
#include "stratum/hal/lib/common/common.pb.h"
#include "stratum/hal/lib/tdi/dpdk/dpdk_chassis_manager.h"
#include "stratum/hal/lib/tdi/dpdk/dpdk_hal.h"
#include "stratum/hal/lib/tdi/dpdk/dpdk_switch.h"
#include "stratum/hal/lib/tdi/tdi_action_profile_manager.h"
#include "stratum/hal/lib/tdi/tdi_counter_manager.h"
#include "stratum/hal/lib/tdi/tdi_node.h"
#include "stratum/hal/lib/tdi/tdi_packetio_manager.h"
#include "stratum/hal/lib/tdi/tdi_pre_manager.h"
#include "stratum/hal/lib/tdi/tdi_sde_wrapper.h"
#include "stratum/hal/lib/tdi/tdi_table_manager.h"
#include "stratum/lib/macros.h"
#include "stratum/lib/security/auth_policy_checker.h"
#include "stratum/lib/security/credentials_manager.h"

#define DEFAULT_CERTS_DIR "/usr/share/stratum/certs/"
#define DEFAULT_CONFIG_DIR "/usr/share/stratum/dpdk/"
#define DEFAULT_LOG_DIR "/var/log/stratum/"

DEFINE_string(dpdk_sde_install, "/usr",
              "Absolute path to the directory where the SDE is installed");
DEFINE_string(dpdk_infrap4d_cfg, DEFAULT_CONFIG_DIR "dpdk_skip_p4.conf",
              "Path to the infrap4d json config file");
DECLARE_string(chassis_config_file);

namespace stratum {
namespace hal {
namespace tdi {

static void InitCommandLineFlags() {
  // Chassis config file
  FLAGS_chassis_config_file = DEFAULT_CONFIG_DIR "dpdk_port_config.pb.txt";

  // Logging options
  FLAGS_log_dir = DEFAULT_LOG_DIR;
  FLAGS_logtostderr = false;
  FLAGS_alsologtostderr = false;

  // Certificate options
  FLAGS_ca_cert_file = DEFAULT_CERTS_DIR "ca.crt";
  FLAGS_server_key_file = DEFAULT_CERTS_DIR "stratum.key";
  FLAGS_server_cert_file = DEFAULT_CERTS_DIR "stratum.crt";
  FLAGS_client_key_file = DEFAULT_CERTS_DIR "client.key";
  FLAGS_client_cert_file = DEFAULT_CERTS_DIR "client.crt";

  // Client certificate verification requirement
  FLAGS_grpc_client_cert_req_type = "REQUIRE_CLIENT_CERT_AND_VERIFY";
}

::util::Status DpdkMain(int argc, char* argv[], absl::Notification* ready_sync,
                        absl::Notification* done_sync) {
  // Set our own default flag values
  InitCommandLineFlags();

  // Parse command-line flags
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  InitStratumLogging();

  // TODO(antonin): The SDE expects 0-based device ids, so we instantiate
  // components with "device_id" instead of "node_id".
  const int device_id = 0;
  const bool dpdk_shell_background = true;

  auto sde_wrapper = TdiSdeWrapper::CreateSingleton();

  RETURN_IF_ERROR(sde_wrapper->InitializeSde(
      FLAGS_dpdk_sde_install, FLAGS_dpdk_infrap4d_cfg,
      dpdk_shell_background));

  /* ========== */
  // NOTE: Rework for DPDK
  ASSIGN_OR_RETURN(bool is_sw_model,
                   sde_wrapper->IsSoftwareModel(device_id));
  const OperationMode mode =
      is_sw_model ? OPERATION_MODE_SIM : OPERATION_MODE_STANDALONE;

  VLOG(1) << "Detected is_sw_model: " << is_sw_model;
  VLOG(1) << "SDE version: " << sde_wrapper->GetSdeVersion();
  VLOG(1) << "Switch SKU: " << sde_wrapper->GetChipType(device_id);
  /* ========== */

  auto table_manager =
      TdiTableManager::CreateInstance(mode, sde_wrapper, device_id);

  auto action_profile_manager =
      TdiActionProfileManager::CreateInstance(sde_wrapper, device_id);

  auto packetio_manager =
      TdiPacketioManager::CreateInstance(sde_wrapper, device_id);

  auto pre_manager =
      TdiPreManager::CreateInstance(sde_wrapper, device_id);

  auto counter_manager =
      TdiCounterManager::CreateInstance(sde_wrapper, device_id);

  auto dpdk_node = TdiNode::CreateInstance(
      table_manager.get(), action_profile_manager.get(),
      packetio_manager.get(), pre_manager.get(),
      counter_manager.get(), sde_wrapper, device_id);

  std::map<int, TdiNode*> device_id_to_dpdk_node = {
      {device_id, dpdk_node.get()},
  };

  auto chassis_manager =
      DpdkChassisManager::CreateInstance(mode, sde_wrapper);

  auto dpdk_switch = DpdkSwitch::CreateInstance(
      chassis_manager.get(), sde_wrapper, device_id_to_dpdk_node);

  auto auth_policy_checker = AuthPolicyChecker::CreateInstance();

  // Create the 'Hal' class instance.
  auto* hal = DpdkHal::CreateSingleton(
      // NOTE: Shouldn't first parameter be 'mode'?
      stratum::hal::OPERATION_MODE_STANDALONE, dpdk_switch.get(),
      auth_policy_checker.get(), ready_sync, done_sync);
  CHECK_RETURN_IF_FALSE(hal) << "Failed to create the Stratum Hal instance.";

  // Set up P4 runtime servers.
  ::util::Status status = hal->Setup();
  if (!status.ok()) {
    LOG(ERROR)
        << "Error when setting up Stratum HAL (but we will continue running): "
        << status.error_message();
  }

  // Start serving RPCs.
  RETURN_IF_ERROR(hal->Run());  // blocking

  LOG(INFO) << "See you later!";
  return ::util::OkStatus();
}

}  // namespace tdi
}  // namespace hal
}  // namespace stratum
