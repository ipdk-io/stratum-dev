// Copyright 2021-present Open Networking Foundation
// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// This file contains variables that will be populated by Bazel via the
// linkstamping feature. It must be kept as simple as possible to compile
// without external headers, compiler flags or build variables.
// https://docs.bazel.build/versions/master/be/c-cpp.html#cc_library.linkstamp
// For a full list of supported variables, see getVolatileKeys():
// https://github.com/bazelbuild/bazel/blob/master/src/main/java/com/google/devtools/build/lib/bazel/BazelWorkspaceStatusModule.java#L319

#include "stratum/glue/stamping.h"

#if !defined(BUILD_TIMESTAMP)
#define BUILD_HOST "None"
#define BUILD_SCM_REVISION "0"
#define BUILD_SCM_STATUS "None"
#define BUILD_USER "None"
#define BUILD_TIMESTAMP 0
#define STAMPING_ENABLED false
#else
#define STAMPING_ENABLED true
#endif

namespace stratum {
const char kBuildHost[] = BUILD_HOST;
const char kBuildScmRevision[] = BUILD_SCM_REVISION;
const char kBuildScmStatus[] = BUILD_SCM_STATUS;
const char kBuildUser[] = BUILD_USER;
const int kBuildTimestamp = BUILD_TIMESTAMP;
const bool kStampingEnabled = STAMPING_ENABLED;
}  // namespace stratum
