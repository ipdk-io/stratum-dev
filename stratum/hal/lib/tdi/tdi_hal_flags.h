// Copyright 2018 Google LLC
// Copyright 2018-present Open Networking Foundation
// Copyright 2022-2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// Common HAL command-line flags.

#ifndef STRATUM_HAL_LIB_TDI_TDI_HAL_FLAGS_H_
#define STRATUM_HAL_LIB_TDI_TDI_HAL_FLAGS_H_

#include "stratum/glue/logging.h"

// External URLs the server listens to.
DECLARE_string(external_stratum_urls);

// URL for local calls from the stratum stub.
DECLARE_string(local_stratum_url);

// Whether this is a warm boot.
DECLARE_bool(warmboot);

// Persistent directory in which config files are stored.
DECLARE_string(persistent_config_dir);

// gRPC parameters.
DECLARE_int32(grpc_keepalive_time_ms);
DECLARE_int32(grpc_keepalive_timeout_ms);
DECLARE_int32(grpc_keepalive_min_ping_interval);
DECLARE_int32(grpc_keepalive_permit);
DECLARE_uint32(grpc_max_recv_msg_size);
DECLARE_uint32(grpc_max_send_msg_size);

// Whether to open gRPC server ports in insecure mode.
DECLARE_bool(grpc_open_insecure_mode);

#endif  // STRATUM_HAL_LIB_TDI_TDI_HAL_FLAGS_H_
