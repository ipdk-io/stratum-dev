# CMake build file for Stratum code.
#
# Copyright 2022 Intel Corporation
# SPDX-License-Identifier: Apache 2.0

add_library(stratum_dpdk_o OBJECT
    dpdk_main.cc
    dpdk_main.h
)

target_include_directories(stratum_dpdk_o PRIVATE
    ${STRATUM_SOURCE_DIR}
    ${PROTO_OUT_DIR}
    ${SDE_INSTALL_DIR}/include)

add_dependencies(stratum_dpdk_o stratum_proto)

add_executable(stratum_dpdk 
    main.cc
    $<TARGET_OBJECTS:stratum_dpdk_o>
)

target_include_directories(stratum_dpdk PRIVATE
    ${STRATUM_SOURCE_DIR}
    ${PROTO_OUT_DIR}
    ${SDE_INSTALL_DIR}/include)

target_link_libraries(stratum_dpdk PRIVATE stratum_tdi)

install(TARGETS stratum_dpdk RUNTIME)

install(
    FILES
        dpdk_port_config.pb.txt
        dpdk_skip_p4.conf
    DESTINATION
        ${CMAKE_INSTALL_PREFIX}/share/stratum/dpdk
)
