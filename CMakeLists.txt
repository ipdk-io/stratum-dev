# CMake build file for Stratum code.
#
# Copyright 2022 Intel Corporation
# SPDX-License-Identifier: Apache 2.0

cmake_minimum_required(VERSION 3.5)
project(stratum)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_INSTALL_PREFIX}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_INSTALL_PREFIX}/lib")

find_package(absl REQUIRED)

option(BUILD_SHARED_LIBS "Build shared library" OFF)

# Suppress "warning: attribute ignored" on ABSL_MUST_USE_RESULT [[nodiscard]]
add_compile_options(-Wno-attributes)

set(STRATUM_SOURCE_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}" CACHE PATH
    "Path to Stratum source directory")

set(STRATUM_INCLUDES
    ${STRATUM_SOURCE_DIR}
    # Protobuf C++ header files.
    # TODO(comes from net-rec for now
    ${PROTO_OUT_DIR}
)

add_subdirectory(stratum)
