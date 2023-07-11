# Build file for //stratum/hal/lib/p4
#
# Copyright 2022-2023 Intel Corporation
# SPDX-License-Identifier: Apache 2.0
#

add_library(stratum_hal_lib_p4_o OBJECT
    p4_info_manager.cc
    p4_info_manager.h
    utils.cc
    utils.h
)

target_include_directories(stratum_hal_lib_p4_o PRIVATE ${STRATUM_INCLUDES})

add_dependencies(stratum_hal_lib_p4_o
    stratum_proto
    p4runtime_proto
)
