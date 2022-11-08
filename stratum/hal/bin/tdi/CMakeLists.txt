# CMake build file for Stratum code.
#
# Copyright 2022 Intel Corporation
# SPDX-License-Identifier: Apache 2.0

if(TOFINO_TARGET)
    add_subdirectory(tofino)
elseif(DPDK_TARGET)
    add_subdirectory(dpdk)
endif()

##############################
# Build tdi_pipeline_builder #
##############################

add_executable(tdi_pipeline_builder
    tdi_pipeline_builder.cc
    $<TARGET_OBJECTS:stratum_glue_o>
    $<TARGET_OBJECTS:stratum_lib_o>
    $<TARGET_OBJECTS:stratum_public_o>
)

target_include_directories(tdi_pipeline_builder PRIVATE
    ${STRATUM_SOURCE_DIR}
    ${PROTO_OUT_DIR}
)

target_link_libraries(tdi_pipeline_builder PUBLIC stratum_proto p4runtime_proto)

target_link_libraries(tdi_pipeline_builder PUBLIC
    protobuf gflags glog grpc grpc++
    absl::raw_hash_set
)

install(TARGETS tdi_pipeline_builder RUNTIME)
