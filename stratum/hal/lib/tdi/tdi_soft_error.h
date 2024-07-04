// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#ifndef STRATUM_HAL_LIB_TDI_TDI_SOFT_ERROR_H_
#define STRATUM_HAL_LIB_TDI_TDI_SOFT_ERROR_H_

#include "stratum/glue/status/status.h"
#include "stratum/hal/lib/tdi/tdi_status.h"
#include "stratum/public/lib/error.h"

namespace stratum {
namespace hal {
namespace tdi {

// Checks whether a TDI status code is a soft error.
static inline bool IsSoftTdiError(tdi_status_t err) {
  return (err == TDI_ALREADY_EXISTS) || (err == TDI_OBJECT_NOT_FOUND) ||
         (err == TDI_TABLE_NOT_FOUND);
}

// Checks whether a Stratum error code is a soft error.
static inline bool IsSoftError(ErrorCode err) {
  return (err == ERR_ENTRY_EXISTS) || (err == ERR_ENTRY_NOT_FOUND);
}

// Checks whether a canonical error code is a soft error.
static inline bool IsSoftError(::util::error::Code err) {
  return (err == ::util::error::ALREADY_EXISTS) ||
         (err == ::util::error::NOT_FOUND);
}

// Checks whether the integer error_code() value of a Status object
// is a soft error.
static inline bool IsSoftError(int err) {
  return IsSoftError(static_cast<::util::error::Code>(err));
}

// Special version of RETURN_IF_ERROR() that suppresses logging if this
// is a soft error.
#define FILTERED_RETURN_IF_ERROR(expr)                                       \
  do {                                                                       \
    const ::util::Status _status = (expr);                                   \
    if (ABSL_PREDICT_FALSE(!_status.ok())) {                                 \
      if (IsSoftError(_status.error_code())) {                               \
        return _status;                                                      \
      }                                                                      \
      LOG(ERROR) << "Return Error: " << #expr << " failed with " << _status; \
      return _status;                                                        \
    }                                                                        \
  } while (0)

}  // namespace tdi
}  // namespace hal
}  // namespace stratum

#endif  // STRATUM_HAL_LIB_TDI_TDI_SOFT_ERROR_H_
