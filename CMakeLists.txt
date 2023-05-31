# Build file for stratum project
#
# Copyright 2022-2023 Intel Corporation
# SPDX-License-Identifier: Apache 2.0

cmake_minimum_required(VERSION 3.15)
project(stratum)

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(DEPEND_INSTALL_DIR "$ENV{DEPEND_INSTALL}" CACHE PATH
        "Dependencies install directory")

    if(NOT DEPEND_INSTALL_DIR STREQUAL "")
        list(APPEND CMAKE_PREFIX_PATH ${DEPEND_INSTALL_DIR})
    endif()

    set(SDE_INSTALL_DIR "$ENV{SDE_INSTALL}" CACHE PATH "SDE install directory")
endif()

find_package(gflags)
find_package(glog)

set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)

# Suppress "warning: attribute ignored" on ABSL_MUST_USE_RESULT [[nodiscard]]
add_compile_options(-Wno-attributes)

#set(CMAKE_BUILD_RPATH_USE_ORIGIN TRUE CACHE BOOL
#    "Use relative paths for the build RUNPATH")
#
#set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE CACHE BOOL
#    "Use link path in the install RUNPATH")

add_subdirectory(stratum)

#######################
# add_stratum_objects #
#######################

function(add_stratum_objects _TGT)
    target_sources(${_TGT} PRIVATE
        $<TARGET_OBJECTS:stratum_glue_o>
        $<TARGET_OBJECTS:stratum_hal_lib_common_o>
        $<TARGET_OBJECTS:stratum_hal_lib_p4_o>
        $<TARGET_OBJECTS:stratum_hal_lib_phal_o>
        $<TARGET_OBJECTS:stratum_lib_o>
        $<TARGET_OBJECTS:stratum_main_o>
        $<TARGET_OBJECTS:stratum_public_o>
        $<TARGET_OBJECTS:stratum_tdi_common_o>
        $<TARGET_OBJECTS:stratum_tdi_target_o>
        $<TARGET_OBJECTS:stratum_yang_parse_tree_o>
    )
endfunction(add_stratum_objects)

#########################
# add_stratum_libraries #
#########################

# Abseil libraries
function(add_stratum_abseil_libs _TGT)
    target_link_libraries(${_TGT} PUBLIC
        absl::strings
        absl::synchronization
        absl::graphcycles_internal
        absl::stacktrace
        absl::symbolize
        absl::malloc_internal
        absl::debugging_internal
        absl::demangle_internal
        absl::time
        absl::strings_internal
        absl::throw_delegate
        absl::base
        absl::spinlock_wait
        absl::int128
        absl::raw_logging_internal
        absl::log_severity
        absl::civil_time
        absl::time_zone
        absl::status
        absl::cord
        absl::exponential_biased
        absl::str_format_internal
        absl::hash
        absl::raw_hash_set
        absl::city
        absl::bad_optional_access
        absl::bad_variant_access
    )
endfunction()

# Google libraries
function(add_stratum_google_libs _TGT)
    target_link_libraries(${_TGT} PUBLIC
        glog
        gflags
        grpc
        protobuf
        grpc++
    )
endfunction()

# Protobuf libraries
function(add_stratum_proto_libs _TGT)
    target_link_libraries(${_TGT} PUBLIC
        openconfig_proto
        stratum_proto
        gnmi_proto
        grpc_proto
        p4runtime_proto
    )
endfunction()

function(add_stratum_libraries _TGT)
    add_stratum_abseil_libs(${_TGT})
    add_stratum_google_libs(${_TGT})
    add_stratum_proto_libs(${_TGT})
    target_link_libraries(${_TGT} PUBLIC pthread)
endfunction()

#####################
# libstratum_static #
#####################

add_library(stratum_static STATIC)
add_stratum_objects(stratum_static)
add_stratum_libraries(stratum_static)

#####################
# libstratum_shared #
#####################

add_library(stratum_shared SHARED)
add_stratum_objects(stratum_shared)
add_stratum_libraries(stratum_shared)
