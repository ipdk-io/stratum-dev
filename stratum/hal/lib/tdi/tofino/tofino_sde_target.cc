// Copyright 2019-present Barefoot Networks, Inc.
// Copyright 2022-2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// Tofino-specific SDE wrapper methods.

#include "stratum/hal/lib/tdi/tdi_sde_wrapper.h"

#include <algorithm>
#include <atomic>
#include <memory>
#include <ostream>
#include <stdio.h>
#include <string.h>
#include <string>
#include <utility>
#include <vector>

#include "absl/memory/memory.h"
#include "absl/strings/str_cat.h"
#include "absl/synchronization/mutex.h"

#include "stratum/glue/gtl/cleanup.h"
#include "stratum/glue/gtl/map_util.h"
#include "stratum/glue/integral_types.h"
#include "stratum/glue/logging.h"
#include "stratum/glue/status/status.h"
#include "stratum/glue/status/status_macros.h"
#include "stratum/glue/status/statusor.h"
#include "stratum/hal/lib/tdi/macros.h"
#include "stratum/hal/lib/tdi/tdi.pb.h"
#include "stratum/hal/lib/tdi/tdi_sde_common.h"
#include "stratum/hal/lib/tdi/tdi_sde_helpers.h"
#include "stratum/lib/channel/channel.h"
#include "stratum/lib/utils.h"

extern "C" {
#include "bf_switchd/bf_switchd.h"
#include "bf_types/bf_types.h"
#include "lld/lld_sku.h"
#include "tofino/bf_pal/dev_intf.h"
#include "tofino/bf_pal/pltfm_intf.h"
#include "tofino/pdfixed/pd_devport_mgr.h"
#include "tofino/pdfixed/pd_tm.h"

// Flag to enable detailed logging in the SDE pipe manager.
extern bool stat_mgr_enable_detail_trace;
} // extern "C"

namespace stratum {
namespace hal {
namespace tdi {

using namespace stratum::hal::tdi::helpers;

::util::StatusOr<bool> TdiSdeWrapper::IsSoftwareModel(int device) {
  bool is_sw_model = true;
  auto bf_status = bf_pal_pltfm_type_get(device, &is_sw_model);
  CHECK_RETURN_IF_FALSE(bf_status == BF_SUCCESS)
      << "Error getting software model status.";
  return is_sw_model;
}

// Helper functions around reading the switch SKU.
namespace {

std::string GetBfChipFamilyAndType(int device) {
  bf_dev_type_t dev_type = lld_sku_get_dev_type(device);
  return pipe_mgr_dev_type2str(dev_type);
}

std::string GetBfChipRevision(int device) {
  bf_sku_chip_part_rev_t revision_number;
  lld_sku_get_chip_part_revision_number(device, &revision_number);
  switch (revision_number) {
    case BF_SKU_CHIP_PART_REV_A0:
      return "A0";
    case BF_SKU_CHIP_PART_REV_B0:
      return "B0";
    default:
      return "UNKNOWN";
  }
  return "UNKNOWN";
}

std::string GetBfChipId(int device) {
  uint64 chip_id = 0;
  lld_sku_get_chip_id(device, &chip_id);
  return absl::StrCat("0x", absl::Hex(chip_id));
}

}  // namespace


std::string TdiSdeWrapper::GetChipType(int device) const {
  return absl::StrCat(GetBfChipFamilyAndType(device), ", revision ",
                      GetBfChipRevision(device), ", chip_id ",
                      GetBfChipId(device));
}

std::string TdiSdeWrapper::GetSdeVersion() const {
  return "9.11.0";
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

  // Determine if kernel mode packet driver is loaded.
  std::string bf_sysfs_fname;
  {
    char buf[128] = {};
    RETURN_IF_TDI_ERROR(switch_pci_sysfs_str_get(buf, sizeof(buf)));
    bf_sysfs_fname = buf;
  }
  absl::StrAppend(&bf_sysfs_fname, "/dev_add");
  LOG(INFO) << "bf_sysfs_fname: " << bf_sysfs_fname;
  if (PathExists(bf_sysfs_fname)) {
    // Override previous parsing if bf_kpkt KLM was loaded.
    LOG(INFO)
        << "kernel mode packet driver present, forcing kernel_pkt option!";
    switchd_main_ctx->kernel_pkt = true;
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
      dev_id, BF_DEV_WARM_INIT_FAST_RECFG, BF_DEV_SERDES_UPD_NONE,
      /* upgrade_agents */ true));
  bf_device_profile_t device_profile = {};

  // Commit new files to disk and build device profile for SDE to load.
  RETURN_IF_ERROR(RecursivelyCreateDir(FLAGS_tdi_sde_config_dir));
  // Need to extend the lifetime of the path strings until the SDE reads them.
  std::vector<std::unique_ptr<std::string>> path_strings;
  device_profile.num_p4_programs = device_config.programs_size();
  for (int i = 0; i < device_config.programs_size(); ++i) {
    const auto& program = device_config.programs(i);
    const std::string program_path =
        absl::StrCat(FLAGS_tdi_sde_config_dir, "/", program.name());
    auto tdi_path = absl::make_unique<std::string>(
        absl::StrCat(program_path, "/bfrt.json"));
    RETURN_IF_ERROR(RecursivelyCreateDir(program_path));
    RETURN_IF_ERROR(WriteStringToFile(program.bfrt(), *tdi_path));

    bf_p4_program_t* p4_program = &device_profile.p4_programs[i];
    ::snprintf(p4_program->prog_name, _PI_UPDATE_MAX_NAME_SIZE, "%s",
               program.name().c_str());
    p4_program->bfrt_json_file = &(*tdi_path)[0];
    p4_program->num_p4_pipelines = program.pipelines_size();
    path_strings.emplace_back(std::move(tdi_path));
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
  stat_mgr_enable_detail_trace = false;
  if (VLOG_IS_ON(2)) {
    CHECK_RETURN_IF_FALSE(bf_sys_log_level_set(BF_MOD_PIPE, BF_LOG_DEST_STDOUT,
                                               BF_LOG_WARN) == 0);
    stat_mgr_enable_detail_trace = true;
  }

  ::tdi::DevMgr::getInstance().deviceGet(dev_id, &device);
  RETURN_IF_TDI_ERROR(device->tdiInfoGet(
       device_config.programs(0).name(), &tdi_info_));

  // FIXME: if all we ever do is create and push, this could be one call.
  tdi_id_mapper_ = TdiIdMapper::CreateInstance();
  RETURN_IF_ERROR(
      tdi_id_mapper_->PushForwardingPipelineConfig(device_config, tdi_info_));

  int port = p4_devport_mgr_pcie_cpu_port_get(dev_id);
  CHECK_RETURN_IF_FALSE(port != -1);
  CHECK_RETURN_IF_FALSE(p4_pd_tm_set_cpuport(dev_id, port) == 0)
      << "Unable to set CPU port " << port << " on device " << dev_id;

  return ::util::OkStatus();
}

//  Packetio

::util::Status TdiSdeWrapper::TxPacket(int device, const std::string& buffer) {
  bf_pkt* pkt = nullptr;
  RETURN_IF_TDI_ERROR(
      bf_pkt_alloc(device, &pkt, buffer.size(), BF_DMA_CPU_PKT_TRANSMIT_0));
  auto pkt_cleaner =
      gtl::MakeCleanup([pkt, device]() { bf_pkt_free(device, pkt); });
  RETURN_IF_TDI_ERROR(bf_pkt_data_copy(
      pkt, reinterpret_cast<const uint8*>(buffer.data()), buffer.size()));
  RETURN_IF_TDI_ERROR(bf_pkt_tx(device, pkt, BF_PKT_TX_RING_0, pkt));
  pkt_cleaner.release();
  return ::util::OkStatus();
}

::util::Status TdiSdeWrapper::StartPacketIo(int device) {
  if (!bf_pkt_is_inited(device)) {
    RETURN_IF_TDI_ERROR(bf_pkt_init());
  }

  // type of i should be bf_pkt_tx_ring_t?
  for (int tx_ring = BF_PKT_TX_RING_0; tx_ring < BF_PKT_TX_RING_MAX;
       ++tx_ring) {
    RETURN_IF_TDI_ERROR(bf_pkt_tx_done_notif_register(
        device, TdiSdeWrapper::BfPktTxNotifyCallback,
        static_cast<bf_pkt_tx_ring_t>(tx_ring)));
  }

  for (int rx_ring = BF_PKT_RX_RING_0; rx_ring < BF_PKT_RX_RING_MAX;
       ++rx_ring) {
    RETURN_IF_TDI_ERROR(
        bf_pkt_rx_register(device, TdiSdeWrapper::BfPktRxNotifyCallback,
                           static_cast<bf_pkt_rx_ring_t>(rx_ring), nullptr));
  }
  VLOG(1) << "Registered packetio callbacks on device " << device << ".";
  return ::util::OkStatus();
}

::util::Status TdiSdeWrapper::StopPacketIo(int device) {
  for (int tx_ring = BF_PKT_TX_RING_0; tx_ring < BF_PKT_TX_RING_MAX;
       ++tx_ring) {
    RETURN_IF_TDI_ERROR(bf_pkt_tx_done_notif_deregister(
        device, static_cast<bf_pkt_tx_ring_t>(tx_ring)));
  }

  for (int rx_ring = BF_PKT_RX_RING_0; rx_ring < BF_PKT_RX_RING_MAX;
       ++rx_ring) {
    RETURN_IF_TDI_ERROR(
        bf_pkt_rx_deregister(device, static_cast<bf_pkt_rx_ring_t>(rx_ring)));
  }
  VLOG(1) << "Unregistered packetio callbacks on device " << device << ".";
  return ::util::OkStatus();
}

::util::Status TdiSdeWrapper::HandlePacketRx(
    bf_dev_id_t device, bf_pkt* pkt, bf_pkt_rx_ring_t rx_ring) {
  absl::ReaderMutexLock l(&packet_rx_callback_lock_);
  auto rx_writer = gtl::FindOrNull(device_to_packet_rx_writer_, device);
  CHECK_RETURN_IF_FALSE(rx_writer)
      << "No Rx callback registered for device id " << device << ".";

  std::string buffer(reinterpret_cast<const char*>(bf_pkt_get_pkt_data(pkt)),
                     bf_pkt_get_pkt_size(pkt));
  if (!(*rx_writer)->TryWrite(buffer).ok()) {
    LOG_EVERY_N(INFO, 500) << "Dropped packet received from CPU.";
  }
  VLOG(1) << "Received " << buffer.size() << " byte packet from CPU "
          << StringToHex(buffer);
  return ::util::OkStatus();
}

bf_status_t TdiSdeWrapper::BfPktTxNotifyCallback(
    bf_dev_id_t device, bf_pkt_tx_ring_t tx_ring, uint64 tx_cookie,
    uint32 status) {
  VLOG(1) << "Tx done notification for device: " << device
          << " tx ring: " << tx_ring << " tx cookie: " << tx_cookie
          << " status: " << status;

  bf_pkt* pkt = reinterpret_cast<bf_pkt*>(tx_cookie);
  return bf_pkt_free(device, pkt);
}

bf_status_t TdiSdeWrapper::BfPktRxNotifyCallback(
    bf_dev_id_t device, bf_pkt* pkt, void* cookie, bf_pkt_rx_ring_t rx_ring) {
  TdiSdeWrapper* tdi_sde_wrapper = TdiSdeWrapper::GetSingleton();
  // TODO(max): Handle error
  tdi_sde_wrapper->HandlePacketRx(device, pkt, rx_ring);
  return bf_pkt_free(device, pkt);
}

// IPsec notification

::util::Status TdiSdeWrapper::InitNotificationTableWithCallback(
    int dev_id, std::shared_ptr<TdiSdeInterface::SessionInterface> session,
    const std::string& table_name, notification_table_callback_t callback,
    void* cookie) const LOCKS_EXCLUDED(data_lock_) {
  RETURN_ERROR(ERR_OPER_NOT_SUPPORTED) << "Notification Table not supported";
}

}  // namespace tdi
}  // namespace hal
}  // namespace stratum
