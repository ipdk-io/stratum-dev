// Copyright 2020-present Open Networking Foundation
// Copyright 2022-2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <string>

#include "absl/strings/str_cat.h"
#include "gflags/gflags.h"
#include "nlohmann/json.hpp"
#include "p4/config/v1/p4info.pb.h"
#include "stratum/glue/init_google.h"
#include "stratum/glue/logging.h"
#include "stratum/glue/status/status.h"
#include "stratum/glue/status/status_macros.h"
#include "stratum/hal/lib/tdi/tdi.pb.h"
#include "stratum/lib/macros.h"
#include "stratum/lib/utils.h"

DEFINE_string(p4c_conf_file, "",
              "Path to the JSON output .conf file of the p4c compiler");
DEFINE_string(tdi_pipeline_config_binary_file, "tdi_pipeline_config.pb.bin",
              "Path to file for serialized TdiPipelineConfig output");
DEFINE_string(unpack_dir, "",
              "Directory to recreate the compiler output from the serialized "
              "TdiPipelineConfig by unpacking the files to disk");

namespace stratum {
namespace hal {
namespace tdi {
namespace {

constexpr char kUsage[] =
    R"USAGE(usage: -p4c_conf_file=/path/to/p4c/output/program.conf -tdi_pipeline_config_binary_file=$PWD/tdi-pipeline.pb.bin

This program assembles a Stratum-tdi pipeline protobuf message from the output
of the P4 compiler. The resulting message can be pushed to Stratum in the
p4_device_config field of the P4Runtime SetForwardingPipelineConfig message.
)USAGE";

::util::Status Unpack() {
  RET_CHECK(!FLAGS_tdi_pipeline_config_binary_file.empty())
      << "pipeline_config_binary_file must be specified.";

  BfPipelineConfig bf_config;
  RETURN_IF_ERROR(
      ReadProtoFromBinFile(FLAGS_tdi_pipeline_config_binary_file, &bf_config));

  // TODO(max): replace with <filesystem> once we move to C++17
  char* resolved_path = realpath(FLAGS_unpack_dir.c_str(), nullptr);
  if (!resolved_path) {
    return MAKE_ERROR(ERR_INTERNAL)
           << "Unable to resolve path " << FLAGS_unpack_dir;
  }
  std::string base_path(resolved_path);
  free(resolved_path);

  RET_CHECK(!bf_config.p4_name().empty());
  LOG(INFO) << "Found P4 program: " << bf_config.p4_name();
  RETURN_IF_ERROR(
      RecursivelyCreateDir(absl::StrCat(base_path, "/", bf_config.p4_name())));
  RETURN_IF_ERROR(WriteStringToFile(
      bf_config.bfruntime_info(),
      absl::StrCat(base_path, "/", bf_config.p4_name(), "/", "bfrt.json")));
  for (const auto& profile : bf_config.profiles()) {
    RET_CHECK(!profile.profile_name().empty());
    LOG(INFO) << "\tFound profile: " << profile.profile_name();
    RETURN_IF_ERROR(RecursivelyCreateDir(absl::StrCat(
        base_path, "/", bf_config.p4_name(), "/", profile.profile_name())));
    RETURN_IF_ERROR(WriteStringToFile(
        profile.context(),
        absl::StrCat(base_path, "/", bf_config.p4_name(), "/",
                     profile.profile_name(), "/", "context.json")));
    RETURN_IF_ERROR(WriteStringToFile(
        profile.binary(),
        absl::StrCat(base_path, "/", bf_config.p4_name(), "/",
                     profile.profile_name(), "/", "tofino.bin")));
  }

  return ::util::OkStatus();
}

::util::Status Main(int argc, char* argv[]) {
  ::gflags::SetUsageMessage(kUsage);
  InitGoogle(argv[0], &argc, &argv, true);
  InitStratumLogging();

  if (!FLAGS_unpack_dir.empty()) {
    return Unpack();
  }

  RET_CHECK(!FLAGS_p4c_conf_file.empty()) << "p4c_conf_file must be specified.";

  nlohmann::json conf;
  try {
    std::string conf_content;
    RETURN_IF_ERROR(ReadFileToString(FLAGS_p4c_conf_file, &conf_content));
    conf = nlohmann::json::parse(conf_content, nullptr, false);
    RET_CHECK(!conf.is_discarded()) << "Failed to parse .conf";
    VLOG(1) << ".conf content: " << conf.dump();
  } catch (std::exception& e) {
    return MAKE_ERROR(ERR_INTERNAL) << e.what();
  }

  // Translate compiler output JSON conf to protobuf.
  // Taken from bf_pipeline_utils.cc
  BfPipelineConfig bf_config;
  try {
    RET_CHECK(conf["p4_devices"].size() == 1)
        << "Stratum only supports single devices.";
    // Only support single devices for now.
    const auto& device = conf["p4_devices"][0];

    // parse pktio_args
    if (device.contains("pktio-args")) {
      auto pktio_args = device["pktio-args"];
      PacketIoConfig pktio_config;
      for (const auto& port : pktio_args["ports"]) {
        pktio_config.add_ports(port);
      }
      pktio_config.set_nb_rxqs(pktio_args["nb_rxqs"]);
      pktio_config.set_nb_txqs(pktio_args["nb_txqs"]);

      *bf_config.mutable_packet_io_config() = pktio_config;
    }

    RET_CHECK(device["p4_programs"].size() == 1)
        << "BfPipelineConfig only supports single P4 programs.";
    const auto& program = device["p4_programs"][0];
    bf_config.set_p4_name(program["program-name"].get<std::string>());
    LOG(INFO) << "Found P4 program: " << bf_config.p4_name();
    std::string tdi_content;
#ifdef ES2K_TARGET
    RETURN_IF_ERROR(ReadFileToString(program["tdi-config"], &tdi_content));
#else
    RETURN_IF_ERROR(ReadFileToString(program["bfrt-config"], &tdi_content));
#endif
    bf_config.set_bfruntime_info(tdi_content);
    for (const auto& pipeline : program["p4_pipelines"]) {
      auto profile = bf_config.add_profiles();
      profile->set_profile_name(
          pipeline["p4_pipeline_name"].get<std::string>());
      LOG(INFO) << "\tFound pipeline: " << profile->profile_name();
      for (const auto& scope : pipeline["pipe_scope"]) {
        profile->add_pipe_scope(scope);
      }
      std::string context_content;
      RETURN_IF_ERROR(ReadFileToString(pipeline["context"], &context_content));
      profile->set_context(context_content);
      std::string config_content;
      RETURN_IF_ERROR(ReadFileToString(pipeline["config"], &config_content));
      profile->set_binary(config_content);
    }
  } catch (nlohmann::json::exception& e) {
    return MAKE_ERROR(ERR_INTERNAL) << e.what();
  }

  RETURN_IF_ERROR(
      WriteProtoToBinFile(bf_config, FLAGS_tdi_pipeline_config_binary_file));

  return ::util::OkStatus();
}

}  // namespace
}  // namespace tdi
}  // namespace hal
}  // namespace stratum

int main(int argc, char** argv) {
  return stratum::hal::tdi::Main(argc, argv).error_code();
}
