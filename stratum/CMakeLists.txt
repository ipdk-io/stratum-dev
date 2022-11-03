# CMake build file for Stratum code.
#
# Copyright 2022 Intel Corporation
# SPDX-License-Identifier: Apache 2.0

add_subdirectory(glue)
add_subdirectory(lib)
add_subdirectory(public)
add_subdirectory(tools/gnmi)

add_subdirectory(hal)
