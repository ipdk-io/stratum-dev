# CMake build file for //stratum/lib/p4runtime
#
# Copyright 2024 Intel Corporation
# SPDX-License-Identifier: Apache 2.0
#

###########################
# stratum_lib_p4runtime_o #
###########################

add_library(stratum_lib_p4runtime_o OBJECT
    sdn_controller_manager.cc
    sdn_controller_manager.h
)

target_include_directories(stratum_lib_p4runtime_o PRIVATE
    ${STRATUM_INCLUDES}
)
