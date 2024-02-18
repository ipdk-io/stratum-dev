# Build file for //stratum/hal/lib/p4
#
# Copyright 2022-2024 Intel Corporation
# SPDX-License-Identifier: Apache 2.0
#

add_library(stratum_hal_lib_p4_o OBJECT
    p4_info_manager.cc
    p4_info_manager.h
    utils.cc
    utils.h
)

target_include_directories(stratum_hal_lib_p4_o PRIVATE ${STRATUM_INCLUDES})

add_dependencies(stratum_hal_lib_p4_o
    stratum_proto
    p4runtime_proto
)

###################################
# Build common_flow_entry_proto_o #
###################################

generate_proto_files(
    "stratum/hal/lib/p4/common_flow_entry.proto"
    "${STRATUM_SOURCE_DIR}"
)

add_library(common_flow_entry_proto_o OBJECT
    ${PB_OUT_DIR}/stratum/hal/lib/p4/common_flow_entry.pb.cc
    ${PB_OUT_DIR}/stratum/hal/lib/p4/common_flow_entry.pb.h
)

target_link_libraries(common_flow_entry_proto_o PRIVATE
    p4_annotation_proto_o
    p4_table_defs_proto_o
    p4runtime_proto_o
)

target_include_directories(common_flow_entry_proto_o PUBLIC ${PB_OUT_DIR})

#############################################
# Build forwarding_pipeline_configs_proto_o #
#############################################

generate_proto_files(
    "stratum/hal/lib/p4/forwarding_pipeline_configs.proto"
    "${STRATUM_SOURCE_DIR}"
)

add_library(forwarding_pipeline_configs_proto_o OBJECT
    ${PB_OUT_DIR}/stratum/hal/lib/p4/forwarding_pipeline_configs.pb.cc
    ${PB_OUT_DIR}/stratum/hal/lib/p4/forwarding_pipeline_configs.pb.h
)

add_dependencies(forwarding_pipeline_configs_proto_o
    p4runtime_proto_o
)

target_include_directories(forwarding_pipeline_configs_proto_o PUBLIC ${PB_OUT_DIR})

############################
# Build p4_control_proto_o #
############################

generate_proto_files(
    "stratum/hal/lib/p4/p4_control.proto"
    "${STRATUM_SOURCE_DIR}"
)

add_library(p4_control_proto_o OBJECT
    ${PB_OUT_DIR}/stratum/hal/lib/p4/p4_control.pb.cc
    ${PB_OUT_DIR}/stratum/hal/lib/p4/p4_control.pb.h
)

add_dependencies(p4_control_proto_o
    p4_annotation_proto_o
    p4_table_defs_proto_o
)

target_include_directories(p4_control_proto_o PUBLIC ${PB_OUT_DIR})

#####################################
# Build p4_pipeline_configs_proto_o #
#####################################

generate_proto_files(
    "stratum/hal/lib/p4/p4_pipeline_configs.proto"
    "${STRATUM_SOURCE_DIR}"
)

add_library(p4_pipeline_configs_proto_o OBJECT
    ${PB_OUT_DIR}/stratum/hal/lib/p4/p4_pipeline_configs.pb.cc
    ${PB_OUT_DIR}/stratum/hal/lib/p4/p4_pipeline_configs.pb.h
)

add_dependencies(p4_pipeline_configs_proto_o
    p4_annotation_proto_o
    p4_control_proto_o
    p4_table_map_proto_o
    p4runtime_proto_o
)

target_include_directories(p4_pipeline_configs_proto_o PUBLIC ${PB_OUT_DIR})

##############################
# Build p4_table_map_proto_o #
##############################

generate_proto_files(
    "stratum/hal/lib/p4/p4_table_map.proto"
    "${STRATUM_SOURCE_DIR}"
)

add_library(p4_table_map_proto_o OBJECT
    ${PB_OUT_DIR}/stratum/hal/lib/p4/p4_table_map.pb.cc
    ${PB_OUT_DIR}/stratum/hal/lib/p4/p4_table_map.pb.h
)

add_dependencies(p4_table_map_proto_o
    common_flow_entry_proto_o
    p4_annotation_proto_o
    p4_table_defs_proto_o
    p4info_proto_o
)

target_include_directories(p4_table_map_proto_o PUBLIC ${PB_OUT_DIR})
