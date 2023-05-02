// Copyright 2022-2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#ifndef STRATUM_HAL_BIN_TDI_MAIN_H_
#define STRATUM_HAL_BIN_TDI_MAIN_H_

#include "stratum/glue/status/status.h"
#include "absl/synchronization/notification.h"

namespace stratum {
namespace hal {
namespace tdi {

::util::Status Main(int argc, char* argv[]);

::util::Status Main(absl::Notification* ready_sync = nullptr,
                    absl::Notification* done_sync = nullptr);

void ParseCommandLine(int argc, char* argv[], bool remove_flags);

}  // namespace tdi
}  // namespace hal
}  // namespace stratum

#endif  // STRATUM_HAL_BIN_TDI_MAIN_H_
