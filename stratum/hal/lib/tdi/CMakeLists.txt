# CMake build file for Stratum code.
#
# Copyright 2022 Intel Corporation
# SPDX-License-Identifier: Apache 2.0

add_library(stratum_hal_lib_tdi_o OBJECT
    tdi_action_profile_manager.cc
    tdi_action_profile_manager.h
    tdi_counter_manager.cc
    tdi_counter_manager.h
    tdi_id_mapper.cc
    tdi_id_mapper.h
    tdi_node.cc
    tdi_node.h
    tdi_packetio_manager.cc
    tdi_packetio_manager.h
    tdi_pipeline_utils.cc
    tdi_pipeline_utils.h
    tdi_pre_manager.cc
    tdi_pre_manager.h
    tdi_sde_action_profile.cc
    tdi_sde_clone_session.cc
    tdi_sde_common.h
    tdi_sde_counter.cc
    tdi_sde_helpers.cc
    tdi_sde_helpers.h
    tdi_sde_interface.h
    tdi_sde_meter.cc
    tdi_sde_multicast.cc
    tdi_sde_register.cc
    tdi_sde_table_data.cc
    tdi_sde_table_entry.cc
    tdi_sde_table_key.cc
    tdi_sde_utils.h
    tdi_sde_wrapper.cc
    tdi_sde_wrapper.h
    tdi_table_manager.cc
    tdi_table_manager.h
    utils.cc
    utils.h
)

target_include_directories(stratum_hal_lib_tdi_o PRIVATE
    ${STRATUM_INCLUDES}
    ${SDE_INSTALL_DIR}/include
)

add_dependencies(stratum_hal_lib_tdi_o stratum_proto)

if(TOFINO_TARGET)
  add_subdirectory(tofino)
elseif(DPDK_TARGET)
  add_subdirectory(dpdk)
endif()

add_library(stratum_hal_lib_tdi SHARED
    $<TARGET_OBJECTS:stratum_glue_o>
    $<TARGET_OBJECTS:stratum_lib_o>
    $<TARGET_OBJECTS:stratum_public_o>
    $<TARGET_OBJECTS:stratum_hal_lib_common_o>
    $<TARGET_OBJECTS:stratum_hal_lib_phal_o>
    $<TARGET_OBJECTS:stratum_hal_lib_p4_o>
    $<TARGET_OBJECTS:stratum_hal_lib_yang_o>
    $<TARGET_OBJECTS:stratum_hal_lib_tdi_o>

    $<$<BOOL:${TOFINO_TARGET}>:$<TARGET_OBJECTS:stratum_hal_lib_yang_subtree_interface_o>>
    $<$<BOOL:${TOFINO_TARGET}>:$<TARGET_OBJECTS:stratum_hal_lib_tdi_tofino_o>>

    $<$<BOOL:${DPDK_TARGET}>:$<TARGET_OBJECTS:stratum_hal_lib_tdi_dpdk_o>>
)

target_link_libraries(stratum_hal_lib_tdi PUBLIC
    absl::strings absl::synchronization absl::graphcycles_internal
    absl::stacktrace absl::symbolize absl::malloc_internal
    absl::debugging_internal absl::demangle_internal absl::time
    absl::strings_internal absl::throw_delegate
    absl::base absl::spinlock_wait absl::int128 absl::raw_logging_internal
    absl::log_severity absl::civil_time absl::civil_time absl::time_zone
    absl::status absl::cord absl::cord_internal absl::cordz_info
    absl::cordz_handle absl::cordz_sample_token absl::cordz_functions
    absl::exponential_biased absl::str_format_internal absl::hash
    absl::raw_hash_set absl::city absl::bad_optional_access
    absl::bad_variant_access absl::low_level_hash
)

target_link_libraries(stratum_hal_lib_tdi PUBLIC glog gflags grpc protobuf grpc++)
target_link_libraries(stratum_hal_lib_tdi PUBLIC pthread)

target_link_libraries(stratum_hal_lib_tdi PUBLIC
    openconfig_proto
    stratum_proto
    gnmi_proto
    grpc_proto
    p4runtime_proto
)

if(TOFINO_TARGET)
    target_link_libraries(stratum_hal_lib_tdi PUBLIC
        driver
        target_sys
    )
elseif(DPDK_TARGET)
    target_link_libraries(stratum_hal_lib_tdi PUBLIC
        driver
        bf_switchd_lib
        tdi
        tdi_json_parser
        target_utils
        target_sys
    )

    target_link_directories(stratum_hal_lib_tdi PUBLIC 
        ${SDE_INSTALL_DIR}/lib
        ${DPDK_LIBRARY_DIRS}
    )

    target_link_options(stratum_hal_lib_tdi PUBLIC
        ${DPDK_LD_FLAGS}
        ${DPDK_LDFLAGS_OTHER}
    )
endif()

install(TARGETS stratum_hal_lib_tdi RUNTIME)
