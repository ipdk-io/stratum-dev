# Build file for //stratum/glue
#
# Copyright 2022-2023 Intel Corporation
# SPDX-License-Identifier: Apache 2.0
#

##################
# stratum_glue_o #
##################

add_library(stratum_glue_o OBJECT
    init_google.h
    integral_types.h
    logging.cc
    logging.h
    stamping.cc
    stamping.h
)

target_include_directories(stratum_glue_o PRIVATE
    ${PROJECT_SOURCE_DIR}
    ${DEPEND_INSTALL_DIR}/include
)

add_subdirectory(gtl)
add_subdirectory(status)

#################
# stratum_utils #
#################

add_library(stratum_utils SHARED
    $<TARGET_OBJECTS:stratum_glue_o>
    $<TARGET_OBJECTS:stratum_public_o>
)

target_link_libraries(stratum_utils PUBLIC
    absl::strings
    absl::synchronization
    gflags::gflags_shared
    glog::glog
)

target_link_directories(stratum_utils PRIVATE
    ${DEPEND_INSTALL_DIR}/lib
)

#install(TARGETS stratum_utils LIBRARY)
