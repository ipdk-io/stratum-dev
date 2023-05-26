# bazel/external/es2k.BUILD

# Copyright 2023 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

load("@rules_cc//cc:defs.bzl", "cc_library")

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "es2k_libs",
    srcs = glob([
        # "es2k-bin/lib/libacccp.so",
        "es2k-bin/lib/libbf_switchd_lib.so*",
        "es2k-bin/lib/libclish.so",
        "es2k-bin/lib/libcpf.so",
        "es2k-bin/lib/libcpf_pmd_infra.so",
        "es2k-bin/lib/libdriver.so*",
        "es2k-bin/lib/libpython3.10.so*",
        "es2k-bin/lib/libtarget_sys.so",
        "es2k-bin/lib/libtarget_utils.so",
        "es2k-bin/lib/libtdi.so",
        "es2k-bin/lib/libtdi_json_parser.so",
        "es2k-bin/lib/libtdi_pna.so",
        # "es2k-bin/lib/libtdi_psa.so",
        # "es2k-bin/lib/libtdi_tna.so",
        "es2k-bin/lib/libvfio.so",
        "es2k-bin/lib/libxeoncp.so",
        "es2k-bin/lib/x86_64-linux-gnu/*.so*",
    ]),
    linkopts = [
        "-L/usr/lib/x86_64-linux-gnu",
        "-lglib-2.0",
        "-lpthread",
        "-lm",
        "-ldl",
    ],
)

cc_library(
    name = "es2k_hdrs",
    hdrs = glob([
        "es2k-bin/include/bf_pal/*.h",
        "es2k-bin/include/bf_switchd/**/*.h",
        "es2k-bin/include/bf_types/*.h",
        "es2k-bin/include/cjson/*.h",
        "es2k-bin/include/dvm/*.h",
        "es2k-bin/include/osdep/*.h",
        "es2k-bin/include/port_mgr/**/*.h",
        "es2k-bin/include/target-sys/**/*.h",
        "es2k-bin/include/target-utils/**/*.h",
        "es2k-bin/include/tdi/**/*.h",
        "es2k-bin/include/tdi/**/*.hpp",
        "es2k-bin/include/tdi_rt/*.h",
    ]),
    strip_include_prefix = "es2k-bin/include",
)

cc_library(
    name = "es2k_sde",
    deps = [
        ":es2k_libs",
        ":es2k_hdrs",
    ],
)

cc_library(
    name = "target_sys",
    srcs = glob([
        "es2k-bin/lib/libtarget_sys.so",
    ]),
)

