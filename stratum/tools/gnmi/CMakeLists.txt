# CMake build file for Stratum code.
#
# Copyright 2022 Intel Corporation
# SPDX-License-Identifier: Apache 2.0

add_executable(gnmi_cli
    gnmi_cli.cc
    $<TARGET_OBJECTS:stratum_glue_o>
    $<TARGET_OBJECTS:stratum_lib_o>
    $<TARGET_OBJECTS:stratum_public_o>
)

target_link_libraries(gnmi_cli
    PUBLIC
        gnmi_proto grpc_proto stratum_proto p4runtime_proto
        grpc protobuf gflags grpc++ glog
        pthread re2 absl::raw_hash_set
)

if(HAVE_POSIX_AIO)
    target_link_libraries(gnmi_cli PUBLIC rt)
endif()

target_include_directories(gnmi_cli
    PRIVATE
        ${STRATUM_SOURCE_DIR}
        ${PROTO_OUT_DIR}
)

add_dependencies(gnmi_cli
    gnmi_proto
    grpc_proto
)

# TODO
if(NOT DPDK_TARGET)
    install(TARGETS gnmi_cli RUNTIME)
endif()
