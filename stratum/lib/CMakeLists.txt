# Build file for //stratum/lib
#
# Copyright 2022-2023 Intel Corporation
# SPDX-License-Identifier: Apache 2.0
#

#################
# stratum_lib_o #
#################

add_library(stratum_lib_o OBJECT
    timer_daemon.cc
    timer_daemon.h
    channel/channel.h
    channel/channel_internal.h
    macros.h
    p4runtime/p4runtime_session.cc
    p4runtime/p4runtime_session.h
    security/auth_policy_checker.cc
    security/auth_policy_checker.h
    security/credentials_manager.cc
    security/credentials_manager.h
    utils.cc
    utils.h
)

target_include_directories(stratum_lib_o PRIVATE ${STRATUM_INCLUDES})

add_dependencies(stratum_lib_o stratum_proto gnmi_proto)
