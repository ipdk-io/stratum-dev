// Copyright 2020-present Open Networking Foundation
// Copyright 2022-2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#ifndef STRATUM_HAL_LIB_TDI_TDI_BF_STATUS_H_
#define STRATUM_HAL_LIB_TDI_TDI_BF_STATUS_H_

extern "C" {
#ifdef ES2K_TARGET
#include "ipu_types/ipu_types.h"
#else
#include "bf_types/bf_types.h"
#endif
}

#include "stratum/glue/status/status.h"
#include "stratum/lib/macros.h"
#include "stratum/public/lib/error.h"

namespace stratum {
namespace hal {
namespace tdi {

// Wrapper object for a ipu_status code from the SDE.
class TdiBfStatus {
 public:
#ifdef ES2K_TARGET
  explicit TdiBfStatus(ipu_status_t status) : status_(status) {}
  operator bool() const { return status_ == IPU_SUCCESS; }
  inline ipu_status_t status() const { return status_; }
  inline ErrorCode error_code() const {
    switch (status_) {
      case IPU_SUCCESS:
        return ERR_SUCCESS;
      case IPU_NOT_READY:
        return ERR_NOT_INITIALIZED;
      case IPU_INVALID_ARG:
        return ERR_INVALID_PARAM;
      case IPU_ALREADY_EXISTS:
        return ERR_ENTRY_EXISTS;
      case IPU_NO_SYS_RESOURCES:
      case IPU_MAX_SESSIONS_EXCEEDED:
      case IPU_NO_SPACE:
      case IPU_EAGAIN:
        return ERR_NO_RESOURCE;
      case IPU_ENTRY_REFERENCES_EXIST:
        return ERR_FAILED_PRECONDITION;
      case IPU_TXN_NOT_SUPPORTED:
      case IPU_NOT_SUPPORTED:
        return ERR_OPER_NOT_SUPPORTED;
      case IPU_HW_COMM_FAIL:
      case IPU_HW_UPDATE_FAILED:
        return ERR_HARDWARE_ERROR;
      case IPU_NO_LEARN_CLIENTS:
        return ERR_FEATURE_UNAVAILABLE;
      case IPU_IDLE_UPDATE_IN_PROGRESS:
        return ERR_OPER_STILL_RUNNING;
      case IPU_OBJECT_NOT_FOUND:
      case IPU_TABLE_NOT_FOUND:
        return ERR_ENTRY_NOT_FOUND;
      case IPU_NOT_IMPLEMENTED:
        return ERR_UNIMPLEMENTED;
      case IPU_SESSION_NOT_FOUND:
      case IPU_INIT_ERROR:
      case IPU_TABLE_LOCKED:
      case IPU_IO:
      case IPU_UNEXPECTED:
      case IPU_DEVICE_LOCKED:
      case IPU_INTERNAL_ERROR:
      case IPU_IN_USE:
      default:
        return ERR_INTERNAL;
    }
  }
#else
  explicit TdiBfStatus(bf_status_t status) : status_(status) {}
  operator bool() const { return status_ == BF_SUCCESS; }
  inline bf_status_t status() const { return status_; }
  inline ErrorCode error_code() const {
    switch (status_) {
      case BF_SUCCESS:
        return ERR_SUCCESS;
      case BF_NOT_READY:
        return ERR_NOT_INITIALIZED;
      case BF_INVALID_ARG:
        return ERR_INVALID_PARAM;
      case BF_ALREADY_EXISTS:
        return ERR_ENTRY_EXISTS;
      case BF_NO_SYS_RESOURCES:
      case BF_MAX_SESSIONS_EXCEEDED:
      case BF_NO_SPACE:
      case BF_EAGAIN:
        return ERR_NO_RESOURCE;
      case BF_ENTRY_REFERENCES_EXIST:
        return ERR_FAILED_PRECONDITION;
      case BF_TXN_NOT_SUPPORTED:
      case BF_NOT_SUPPORTED:
        return ERR_OPER_NOT_SUPPORTED;
      case BF_HW_COMM_FAIL:
      case BF_HW_UPDATE_FAILED:
        return ERR_HARDWARE_ERROR;
      case BF_NO_LEARN_CLIENTS:
        return ERR_FEATURE_UNAVAILABLE;
      case BF_IDLE_UPDATE_IN_PROGRESS:
        return ERR_OPER_STILL_RUNNING;
      case BF_OBJECT_NOT_FOUND:
      case BF_TABLE_NOT_FOUND:
        return ERR_ENTRY_NOT_FOUND;
      case BF_NOT_IMPLEMENTED:
        return ERR_UNIMPLEMENTED;
      case BF_SESSION_NOT_FOUND:
      case BF_INIT_ERROR:
      case BF_TABLE_LOCKED:
      case BF_IO:
      case BF_UNEXPECTED:
      case BF_DEVICE_LOCKED:
      case BF_INTERNAL_ERROR:
      case BF_IN_USE:
      default:
        return ERR_INTERNAL;
    }
  }
#endif

 private:
#ifdef ES2K_TARGET
  ipu_status_t status_;
#else
  bf_status_t status_;
#endif
};

// A macro to simplify checking and logging the return value of a SDE function
// call.
#ifdef ES2K_TARGET
#define RETURN_IF_TDI_ERROR(expr)                             \
  if (const TdiBfStatus __ret = TdiBfStatus(expr)) {          \
  } else /* NOLINT */                                         \
    return MAKE_ERROR(__ret.error_code())                     \
           << "'" << #expr << "' failed with error message: " \
           << FixMessage(ipu_err_str(__ret.status()))
#else
#define RETURN_IF_TDI_ERROR(expr)                             \
  if (const TdiBfStatus __ret = TdiBfStatus(expr)) {          \
  } else /* NOLINT */                                         \
    return MAKE_ERROR(__ret.error_code())                     \
           << "'" << #expr << "' failed with error message: " \
           << FixMessage(bf_err_str(__ret.status()))
#endif
// A macro to simplify creating a new error or appending new info to an
// error based on the return value of a SDE function call. The caller function
// will not return. The variable given as "status" must be an object of type
// ::util::Status.
#ifdef ES2K_TARGET
#define APPEND_STATUS_IF_BFRT_ERROR(status, expr)                           \
  if (const TdiBfStatus __ret = TdiBfStatus(expr)) {                        \
  } else /* NOLINT */                                                       \
    status =                                                                \
        APPEND_ERROR(!status.ok() ? status                                  \
                                  : ::util::Status(StratumErrorSpace(),     \
                                                   __ret.error_code(), "")) \
            .without_logging()                                              \
        << (status.error_message().empty() ||                               \
                    status.error_message().back() == ' '                    \
                ? ""                                                        \
                : " ")                                                      \
        << "'" << #expr << "' failed with error message: "                  \
        << FixMessage(ipu_err_str(__ret.status()))
#else
#define APPEND_STATUS_IF_BFRT_ERROR(status, expr)                           \
  if (const TdiBfStatus __ret = TdiBfStatus(expr)) {                        \
  } else /* NOLINT */                                                       \
    status =                                                                \
        APPEND_ERROR(!status.ok() ? status                                  \
                                  : ::util::Status(StratumErrorSpace(),     \
                                                   __ret.error_code(), "")) \
            .without_logging()                                              \
        << (status.error_message().empty() ||                               \
                    status.error_message().back() == ' '                    \
                ? ""                                                        \
                : " ")                                                      \
        << "'" << #expr << "' failed with error message: "                  \
        << FixMessage(bf_err_str(__ret.status()))
#endif
}  // namespace tdi
}  // namespace hal
}  // namespace stratum

#endif  // STRATUM_HAL_LIB_TDI_TDI_BF_STATUS_H_
