# Copyright 2023 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

# Load macros and repository rules.
load("@bazel_gazelle//:deps.bzl", "go_repository")

def golang_dependencies():
    go_repository(
        name = "org_golang_x_sys",
        importpath = "golang.org/x/sys",
        sum = "h1:1BGLXjeY4akVXGgbC9HugT3Jv3hCI0z56oJR5vAMgBU=",
        version = "v0.0.0-20190215142949-d0b11bdaac8a",
    )
