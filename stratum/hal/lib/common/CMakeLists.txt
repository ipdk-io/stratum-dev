# CMake build file for Stratum code.
#
# Copyright 2022 Intel Corporation
# SPDX-License-Identifier: Apache 2.0

add_library(stratum_hal_lib_common_o OBJECT
    channel_writer_wrapper.h
    config_monitoring_service.cc
    config_monitoring_service.h
    error_buffer.cc
    error_buffer.h
    gnmi_publisher.cc
    gnmi_publisher.h
    openconfig_converter.cc
    openconfig_converter.h
    p4_service.cc
    p4_service.h
    server_writer_wrapper.h
    utils.cc
    utils.h
    writer_interface.h
)

target_include_directories(stratum_hal_lib_common_o PRIVATE
    ${STRATUM_INCLUDES}
    ${SDE_INSTALL_DIR}/include
)

add_dependencies(stratum_hal_lib_common_o
    gnmi_proto
    stratum_proto
)
