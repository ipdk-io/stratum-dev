# CMake build file for Stratum code.
#
# Copyright 2022 Intel Corporation
# SPDX-License-Identifier: Apache 2.0

add_library(stratum_public_o OBJECT
    lib/error.cc
    lib/error.h
)

target_include_directories(stratum_public_o PRIVATE ${STRATUM_INCLUDES})

add_dependencies(stratum_public_o stratum_proto)
