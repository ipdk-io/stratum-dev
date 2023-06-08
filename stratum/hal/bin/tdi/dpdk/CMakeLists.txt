# Build file for //stratum/hal/bin/tdi/dpdk
#
# Copyright 2022-2023 Intel Corporation
# SPDX-License-Identifier: Apache 2.0
#

target_sources(stratum_main_o PRIVATE
    dpdk_main.cc
)

install(
    FILES
        dpdk_port_config.pb.txt
        dpdk_skip_p4.conf
    DESTINATION
        ${CMAKE_INSTALL_DATAROOTDIR}/stratum/dpdk
)
