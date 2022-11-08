# CMake build file for Stratum tofino lib code.
#
# Copyright 2022 Intel Corporation
# SPDX-License-Identifier: Apache 2.0

add_library(stratum_hal_lib_tdi_tofino_o OBJECT
  tofino_chassis_manager.cc
  tofino_chassis_manager.h
  tofino_hal.cc
  tofino_hal.h
  tofino_sde_target.cc
  tofino_sde_utils.cc
  tofino_switch.cc
  tofino_switch.h
)

target_compile_definitions(stratum_hal_lib_tdi_tofino_o PRIVATE SDE_9_11_0)

target_include_directories(stratum_hal_lib_tdi_tofino_o PRIVATE
    ${STRATUM_INCLUDES}
    ${SDE_INSTALL_DIR}/include
)

add_dependencies(stratum_hal_lib_tdi_tofino_o stratum_proto)
