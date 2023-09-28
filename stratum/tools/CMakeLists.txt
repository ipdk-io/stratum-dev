# Build file for //stratum/tools
#
# Copyright 2022-2023 Intel Corporation
# SPDX-License-Identifier: Apache 2.0
#

############
# gnmi_cli #
############

add_executable(gnmi_cli gnmi/gnmi_cli.cc)

set_install_rpath(gnmi_cli ${EXEC_ELEMENT} ${DEP_ELEMENT})

target_link_libraries(gnmi_cli PUBLIC
    stratum_static
    gnmi_proto
    grpc_proto
    gflags::gflags_shared
    gRPC::grpc
    gRPC::grpc++
    gRPC::re2
    protobuf::libprotobuf
    pthread
)

if(HAVE_POSIX_AIO)
    target_link_libraries(gnmi_cli PUBLIC rt)
endif()

target_include_directories(gnmi_cli PRIVATE
    ${STRATUM_SOURCE_DIR}
    ${PB_OUT_DIR}
)

if(NOT DPDK_TARGET)
    install(TARGETS gnmi_cli RUNTIME)
endif()
