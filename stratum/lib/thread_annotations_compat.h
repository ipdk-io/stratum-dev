// Copyright 2025 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#ifndef STRATUM_LIB_THREAD_ANNOTATIONS_COMPAT_H_
#define STRATUM_LIB_THREAD_ANNOTATIONS_COMPAT_H_

#include "absl/base/thread_annotations.h"

// Thread annotation compatibility layer for abseil-cpp upgrade
// Maps the unprefixed annotations to their 'ABSL_' prefixed versions
//
// After upgrading from gRPC v1.59.2 to v1.69.0, the thread annotations
// without the prefixed 'ABSL_' do not work. Instead of making changes all
// across the codebase, this compatibility layer is created to map the old
// annotations to the new ones.

#ifndef GUARDED_BY
#define GUARDED_BY(x) ABSL_GUARDED_BY(x)
#endif

#ifndef LOCKS_EXCLUDED
#define LOCKS_EXCLUDED(...) ABSL_LOCKS_EXCLUDED(__VA_ARGS__)
#endif

#ifndef EXCLUSIVE_LOCKS_REQUIRED
#define EXCLUSIVE_LOCKS_REQUIRED(...) ABSL_EXCLUSIVE_LOCKS_REQUIRED(__VA_ARGS__)
#endif

#ifndef SHARED_LOCKS_REQUIRED
#define SHARED_LOCKS_REQUIRED(...) ABSL_SHARED_LOCKS_REQUIRED(__VA_ARGS__)
#endif

#ifndef NO_THREAD_SAFETY_ANALYSIS
#define NO_THREAD_SAFETY_ANALYSIS ABSL_NO_THREAD_SAFETY_ANALYSIS
#endif

#ifndef LOCKABLE
#define LOCKABLE ABSL_LOCKABLE
#endif

#ifndef SCOPED_LOCKABLE
#define SCOPED_LOCKABLE ABSL_SCOPED_LOCKABLE
#endif

#ifndef EXCLUSIVE_LOCK_FUNCTION
#define EXCLUSIVE_LOCK_FUNCTION(...) ABSL_EXCLUSIVE_LOCK_FUNCTION(__VA_ARGS__)
#endif

#ifndef SHARED_LOCK_FUNCTION
#define SHARED_LOCK_FUNCTION(...) ABSL_SHARED_LOCK_FUNCTION(__VA_ARGS__)
#endif

#ifndef UNLOCK_FUNCTION
#define UNLOCK_FUNCTION(...) ABSL_UNLOCK_FUNCTION(__VA_ARGS__)
#endif

#ifndef EXCLUSIVE_TRYLOCK_FUNCTION
#define EXCLUSIVE_TRYLOCK_FUNCTION(...) ABSL_EXCLUSIVE_TRYLOCK_FUNCTION(__VA_ARGS__)
#endif

#ifndef SHARED_TRYLOCK_FUNCTION
#define SHARED_TRYLOCK_FUNCTION(...) ABSL_SHARED_TRYLOCK_FUNCTION(__VA_ARGS__)
#endif

#endif  // STRATUM_LIB_THREAD_ANNOTATIONS_COMPAT_H_
