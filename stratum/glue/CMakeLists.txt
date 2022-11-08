# CMake build file for Stratum glue code.
#
# Copyright 2022 Intel Corporation
# SPDX-License-Identifier: Apache 2.0

add_library(stratum_glue_o OBJECT
    gtl/cleanup.h
    integral_types.h
    logging.cc
    logging.h
    status/posix_error_space.cc
    status/posix_error_space.h
    status/status.cc
    status/status.h
    status/status_macros.cc
    status/status_macros.h
    status/statusor.cc
    status/statusor.h
)

target_include_directories(stratum_glue_o PRIVATE ${STRATUM_INCLUDES})
