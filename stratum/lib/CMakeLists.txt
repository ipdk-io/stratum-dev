# CMake build file for Stratum code.
#
# Copyright 2022 Intel Corporation
# SPDX-License-Identifier: Apache 2.0

add_library(stratum_lib_o OBJECT
    timer_daemon.cc
    timer_daemon.h
    channel/channel.h
    channel/channel_internal.h
    macros.h
    security/auth_policy_checker.cc
    security/auth_policy_checker.h
    utils.cc
    utils.h
)

target_include_directories(stratum_lib_o PRIVATE ${STRATUM_INCLUDES})

add_dependencies(stratum_lib_o stratum_proto)
