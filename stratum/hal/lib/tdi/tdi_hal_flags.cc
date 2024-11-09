// Copyright 2018 Google LLC
// Copyright 2018-present Open Networking Foundation
// Copyright 2022-2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// Common HAL command-line flags.

#include "gflags/gflags.h"
#include "stratum/lib/constants.h"

// TODO(unknown): Use FLAG_DEFINE for all flags.
DEFINE_string(external_stratum_urls, stratum::kExternalStratumUrls,
              "Comma-separated list of URLs for server to listen to, for "
              "external calls from SDN controller, etc.");
DEFINE_string(local_stratum_url, stratum::kLocalStratumUrl,
              "URL for listening to local calls from stratum stub.");

DEFINE_bool(warmboot, false, "Determines whether HAL is in warmboot stage.");
DEFINE_string(persistent_config_dir, "/etc/stratum/",
              "The persistent dir where all the config files will be stored.");

DEFINE_int32(grpc_keepalive_time_ms, 600000, "grpc keep-alive time");
DEFINE_int32(grpc_keepalive_timeout_ms, 20000,
             "grpc keep-alive timeout period");
DEFINE_int32(grpc_keepalive_min_ping_interval, 10000,
             "grpc keep-alive minimum ping interval");
DEFINE_int32(grpc_keepalive_permit, 1, "grpc keep-alive permit");
DEFINE_uint32(grpc_max_recv_msg_size, 256 * 1024 * 1024,
              "grpc server max receive message size (0 = gRPC default).");
DEFINE_uint32(grpc_max_send_msg_size, 0,
              "grpc server max send message size (0 = gRPC default).");
DEFINE_bool(grpc_open_insecure_mode, false,
            "Open grpc server ports in insecure mode for gNMI, gNOI, and P4RT");
