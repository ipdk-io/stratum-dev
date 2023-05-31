# Build file for //stratum/hal/bin/tdi/tofino
#
# Copyright 2022-2023 Intel Corporation
# SPDX-License-Identifier: Apache 2.0
#

target_sources(stratum_main_o PRIVATE
    tofino_main.cc
)

set(STRATUM_BFRT_BIN_DIR ${STRATUM_SOURCE_DIR}/stratum/hal/bin/barefoot)

install(
    FILES
        ${STRATUM_BFRT_BIN_DIR}/tofino_skip_p4_no_bsp.conf
        ${STRATUM_BFRT_BIN_DIR}/tofino_skip_p4.conf
    DESTINATION
        ${CMAKE_INSTALL_DATAROOTDIR}/stratum
)
