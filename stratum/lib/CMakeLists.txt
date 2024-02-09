# CMake build file for //stratum/lib
#
# Copyright 2022-2024 Intel Corporation
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
    utils.cc
    utils.h
)

target_include_directories(stratum_lib_o PRIVATE ${STRATUM_INCLUDES})

add_dependencies(stratum_lib_o stratum_proto gnmi_proto)

add_subdirectory(p4runtime)
add_subdirectory(security)
