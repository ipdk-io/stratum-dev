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

#######################
# p4runtime_session_o #
#######################

add_library(p4runtime_session_o OBJECT
    p4runtime_session.cc
    p4runtime_session.h
)

target_include_directories(p4runtime_session_o PRIVATE
    ${STRATUM_INCLUDES}
    ${PB_OUT_DIR}
)

target_link_libraries(p4runtime_session_o PUBLIC p4_role_config)
