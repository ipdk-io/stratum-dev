// Copyright 2019-present Barefoot Networks, Inc.
// Copyright 2022-2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// Es2k-specific SDE wrapper methods.

#include "stratum/hal/lib/tdi/tdi_sde_wrapper.h"

#include <algorithm>
#include <atomic>
#include <memory>
#include <ostream>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <utility>
#include <vector>

#include "absl/memory/memory.h"
#include "absl/strings/str_cat.h"
#include "absl/synchronization/mutex.h"
#include "absl/time/clock.h"
#include "absl/time/time.h"

#include "stratum/glue/gtl/cleanup.h"
#include "stratum/glue/gtl/map_util.h"
#include "stratum/glue/integral_types.h"
#include "stratum/glue/logging.h"
#include "stratum/glue/status/status.h"
#include "stratum/glue/status/status_macros.h"
#include "stratum/glue/status/statusor.h"
#include "stratum/hal/lib/common/common.pb.h"
#include "stratum/hal/lib/common/utils.h"
#include "stratum/hal/lib/tdi/macros.h"
#include "stratum/hal/lib/tdi/tdi.pb.h"
#include "stratum/hal/lib/tdi/tdi_sde_common.h"
#include "stratum/hal/lib/tdi/tdi_sde_helpers.h"
#include "stratum/hal/lib/tdi/es2k/es2k_port_manager.h"
#include "stratum/lib/channel/channel.h"
#include "stratum/lib/constants.h"
#include "stratum/lib/utils.h"

extern "C" {
#include "bf_switchd/lib/bf_switchd_lib_init.h"
#include "bf_types/bf_types.h"
#include "tdi_rt/tdi_rt_defs.h"
#include "bf_pal/bf_pal_port_intf.h"
#include "bf_pal/dev_intf.h"

// Flag to enable detailed logging in the SDE pipe manager.
extern bool stat_mgr_enable_detail_trace;
} // extern "C"

namespace stratum {
namespace hal {
namespace tdi {

using namespace stratum::hal::tdi::helpers;

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
  // Unsupportied. Returning PORT_STATE_DOWN if called
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

::util::StatusOr<bool> TdiSdeWrapper::IsSoftwareModel(int device) {
  bool is_sw_model = true;
  return is_sw_model;
}

// NOTE: This is Tofino-specific.
std::string TdiSdeWrapper::GetSdeVersion() const {
  return "1.0.0";
}

// Helper functions around reading the switch SKU.
namespace {

std::string GetBfChipFamilyAndType(int device) {
  return "UNKNOWN";
}

std::string GetBfChipRevision(int device) {
  return "UNKNOWN";
}

std::string GetBfChipId(int device) {
  uint64 chip_id = 0;
  return absl::StrCat("0x", absl::Hex(chip_id));
}

}  // namespace


std::string TdiSdeWrapper::GetChipType(int device) const {
  return absl::StrCat(GetBfChipFamilyAndType(device), ", revision ",
                      GetBfChipRevision(device), ", chip_id ",
                      GetBfChipId(device));
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

::util::Status TdiSdeWrapper::InitializeSde(
    const std::string& sde_install_path, const std::string& sde_config_file,
    bool run_in_background) {
  CHECK_RETURN_IF_FALSE(sde_install_path != "")
      << "sde_install_path is required";
  CHECK_RETURN_IF_FALSE(sde_config_file != "") << "sde_config_file is required";

  // Parse bf_switchd arguments.
  auto switchd_main_ctx = absl::make_unique<bf_switchd_context_t>();
  switchd_main_ctx->install_dir = strdup(sde_install_path.c_str());
  switchd_main_ctx->conf_file = strdup(sde_config_file.c_str());
  switchd_main_ctx->skip_p4 = true;
  if (run_in_background) {
    switchd_main_ctx->running_in_background = true;
  } else {
    switchd_main_ctx->shell_set_ucli = true;
  }

  RETURN_IF_TDI_ERROR(bf_switchd_lib_init(switchd_main_ctx.get()))
      << "Error when starting switchd.";
  LOG(INFO) << "switchd started successfully";

  return ::util::OkStatus();
}

::util::Status TdiSdeWrapper::AddDevice(
    int dev_id, const TdiDeviceConfig& device_config) {
  const ::tdi::Device *device = nullptr;
  absl::WriterMutexLock l(&data_lock_);

  CHECK_RETURN_IF_FALSE(device_config.programs_size() > 0);

  tdi_id_mapper_.reset();

  RETURN_IF_TDI_ERROR(bf_pal_device_warm_init_begin(
      dev_id, BF_DEV_WARM_INIT_FAST_RECFG,
      /* upgrade_agents */ true));
  bf_device_profile_t device_profile = {};

  // Commit new files to disk and build device profile for SDE to load.
  RETURN_IF_ERROR(RecursivelyCreateDir(FLAGS_tdi_sde_config_dir));
  // Need to extend the lifetime of the path strings until the SDE read them.
  std::vector<std::unique_ptr<std::string>> path_strings;
  device_profile.num_p4_programs = device_config.programs_size();
  for (int i = 0; i < device_config.programs_size(); ++i) {
    const auto& program = device_config.programs(i);
    const std::string program_path =
        absl::StrCat(FLAGS_tdi_sde_config_dir, "/", program.name());
    auto tdirt_path = absl::make_unique<std::string>(
        absl::StrCat(program_path, "/bfrt.json"));
    RETURN_IF_ERROR(RecursivelyCreateDir(program_path));
    RETURN_IF_ERROR(WriteStringToFile(program.bfrt(), *tdirt_path));

    bf_p4_program_t* p4_program = &device_profile.p4_programs[i];
    ::snprintf(p4_program->prog_name, _PI_UPDATE_MAX_NAME_SIZE, "%s",
               program.name().c_str());
    p4_program->bfrt_json_file = &(*tdirt_path)[0];
    p4_program->num_p4_pipelines = program.pipelines_size();
    path_strings.emplace_back(std::move(tdirt_path));
    CHECK_RETURN_IF_FALSE(program.pipelines_size() > 0);
    for (int j = 0; j < program.pipelines_size(); ++j) {
      const auto& pipeline = program.pipelines(j);
      const std::string pipeline_path =
          absl::StrCat(program_path, "/", pipeline.name());
      auto context_path = absl::make_unique<std::string>(
          absl::StrCat(pipeline_path, "/context.json"));
      auto config_path = absl::make_unique<std::string>(
          absl::StrCat(pipeline_path, "/tofino.bin"));
      RETURN_IF_ERROR(RecursivelyCreateDir(pipeline_path));
      RETURN_IF_ERROR(WriteStringToFile(pipeline.context(), *context_path));
      RETURN_IF_ERROR(WriteStringToFile(pipeline.config(), *config_path));

      bf_p4_pipeline_t* pipeline_profile = &p4_program->p4_pipelines[j];
      ::snprintf(pipeline_profile->p4_pipeline_name, _PI_UPDATE_MAX_NAME_SIZE,
                 "%s", pipeline.name().c_str());
      pipeline_profile->cfg_file = &(*config_path)[0];
      pipeline_profile->runtime_context_file = &(*context_path)[0];
      path_strings.emplace_back(std::move(config_path));
      path_strings.emplace_back(std::move(context_path));

      CHECK_RETURN_IF_FALSE(pipeline.scope_size() <= MAX_P4_PIPELINES);
      pipeline_profile->num_pipes_in_scope = pipeline.scope_size();
      for (int p = 0; p < pipeline.scope_size(); ++p) {
        const auto& scope = pipeline.scope(p);
        pipeline_profile->pipe_scope[p] = scope;
      }
    }
  }

  // This call re-initializes most SDE components.
  RETURN_IF_TDI_ERROR(bf_pal_device_add(dev_id, &device_profile));
  RETURN_IF_TDI_ERROR(bf_pal_device_warm_init_end(dev_id));

  // Set SDE log levels for modules of interest.
  // TODO(max): create story around SDE logs. How to get them into glog? What
  // levels to enable for which modules?
  CHECK_RETURN_IF_FALSE(
      bf_sys_log_level_set(BF_MOD_BFRT, BF_LOG_DEST_STDOUT, BF_LOG_WARN) == 0);
  CHECK_RETURN_IF_FALSE(
      bf_sys_log_level_set(BF_MOD_PKT, BF_LOG_DEST_STDOUT, BF_LOG_WARN) == 0);
  CHECK_RETURN_IF_FALSE(
      bf_sys_log_level_set(BF_MOD_PIPE, BF_LOG_DEST_STDOUT, BF_LOG_WARN) == 0);
  if (VLOG_IS_ON(2)) {
    CHECK_RETURN_IF_FALSE(bf_sys_log_level_set(BF_MOD_PIPE, BF_LOG_DEST_STDOUT,
                                               BF_LOG_WARN) == 0);
  }

  ::tdi::DevMgr::getInstance().deviceGet(dev_id, &device);
  RETURN_IF_TDI_ERROR(device->tdiInfoGet(
       device_config.programs(0).name(), &tdi_info_));

  // FIXME: if all we ever do is create and push, this could be one call.
  tdi_id_mapper_ = TdiIdMapper::CreateInstance();
  RETURN_IF_ERROR(
      tdi_id_mapper_->PushForwardingPipelineConfig(device_config, tdi_info_));

  return ::util::OkStatus();
}

//  Packetio

::util::Status TdiSdeWrapper::TxPacket(int device, const std::string& buffer) {
  return ::util::OkStatus();
}

::util::Status TdiSdeWrapper::StartPacketIo(int device) {
  return ::util::OkStatus();
}

::util::Status TdiSdeWrapper::StopPacketIo(int device) {
  return ::util::OkStatus();
}

}  // namespace tdi
}  // namespace hal
}  // namespace stratum
