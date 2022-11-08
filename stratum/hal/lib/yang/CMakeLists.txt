# CMake build file for Stratum yang code.
#
# Copyright 2022 Intel Corporation
# SPDX-License-Identifier: Apache 2.0

add_library(stratum_hal_lib_yang_o OBJECT
    yang_parse_tree.cc
    yang_parse_tree.h
    yang_parse_tree_chassis.cc
    yang_parse_tree_component.cc
    yang_parse_tree_component.h
    yang_parse_tree_helpers.cc
    yang_parse_tree_helpers.h
    yang_parse_tree_interface.cc
    yang_parse_tree_interface.h
    yang_parse_tree_node.cc
    yang_parse_tree_optical.cc
    yang_parse_tree_paths.cc
    yang_parse_tree_paths.h
    yang_parse_tree_singleton.cc
    yang_parse_tree_system.cc
)

target_include_directories(stratum_hal_lib_yang_o PRIVATE
    ${STRATUM_INCLUDES}
)

add_dependencies(stratum_hal_lib_yang_o
    gnmi_proto
    stratum_proto
)

# generic subtree interface for most targets
add_library(stratum_hal_lib_yang_subtree_interface_o OBJECT
    yang_add_subtree_interface.cc
)

target_include_directories(stratum_hal_lib_yang_subtree_interface_o PRIVATE
    ${STRATUM_INCLUDES}
)

add_dependencies(stratum_hal_lib_yang_subtree_interface_o
    gnmi_proto
    stratum_proto
)
