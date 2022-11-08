# CMake build file for Stratum dpdk lib code.
#
# Copyright 2022 Intel Corporation
# SPDX-License-Identifier: Apache 2.0

add_library(stratum_hal_lib_tdi_dpdk_o OBJECT
    dpdk_chassis_manager.cc
    dpdk_chassis_manager.h
    dpdk_hal.cc
    dpdk_hal.h
    dpdk_port_config.cc
    dpdk_port_config.h
    dpdk_sde_target.cc
    dpdk_sde_utils.cc
    dpdk_switch.cc
    dpdk_switch.h
    dpdk_add_subtree_interface.cc
    dpdk_parse_tree_interface.cc
    dpdk_parse_tree_interface.h
)

target_include_directories(stratum_hal_lib_tdi_dpdk_o PRIVATE
    ${STRATUM_INCLUDES}
    ${SDE_INSTALL_DIR}/include
)

add_dependencies(stratum_hal_lib_tdi_dpdk_o stratum_proto)
