# Build file for //stratum/glue/status
#
# Copyright 2022-2023 Intel Corporation
# SPDX-License-Identifier: Apache 2.0
#

##################
# stratum_glue_o #
##################

target_sources(stratum_glue_o PRIVATE
    canonical_errors.cc
    canonical_errors.h
    posix_error_space.cc
    posix_error_space.h
    status.cc
    status.h
    status_macros.cc
    status_macros.h
    statusor.cc
    statusor.h
)

target_include_directories(stratum_glue_o PUBLIC ${DEPEND_INSTALL_DIR}/include)

target_link_libraries(stratum_glue_o PUBLIC
    gflags::gflags_shared
    glog::glog
)
