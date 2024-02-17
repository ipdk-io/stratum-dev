# Build file for //stratum/public/proto
#
# Copyright 2024 Intel Corporation
# SPDX-License-Identifier: Apache 2.0
#

########################
# Build p4_role_config #
########################

generate_proto_files(
    "stratum/public/proto/p4_role_config.proto"
    "${STRATUM_SOURCE_DIR}"
)

add_library(p4_role_config SHARED
    ${PB_OUT_DIR}/stratum/public/proto/p4_role_config.pb.cc
    ${PB_OUT_DIR}/stratum/public/proto/p4_role_config.pb.h
)

target_include_directories(p4_role_config PUBLIC ${PB_OUT_DIR})

install(TARGETS p4_role_config LIBRARY)

