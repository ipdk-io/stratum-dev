# CMake build file for Stratum code.
#
# Copyright 2022 Intel Corporation
# SPDX-License-Identifier: Apache 2.0

add_library(stratum_tofino_o OBJECT
    tofino_main.cc
    tofino_main.h
)

target_include_directories(stratum_tofino_o PRIVATE
    ${STRATUM_SOURCE_DIR}
    ${PROTO_OUT_DIR}
    ${SDE_INSTALL_DIR}/include)

add_dependencies(stratum_tofino_o stratum_proto)

add_executable(stratum_tofino
    main.cc
    $<TARGET_OBJECTS:stratum_tofino_o>
)

target_include_directories(stratum_tofino PRIVATE
    ${STRATUM_SOURCE_DIR}
    ${PROTO_OUT_DIR}
    ${SDE_INSTALL_DIR}/include)

target_link_libraries(stratum_tofino PRIVATE stratum_tdi)

install(TARGETS stratum_tofino RUNTIME)

install(
    FILES
        tofino_skip_p4_no_bsp.conf
        tofino_skip_p4.conf
    DESTINATION
        ${CMAKE_INSTALL_PREFIX}/share/stratum/tofino
)
