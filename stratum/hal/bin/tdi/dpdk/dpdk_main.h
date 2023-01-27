// Copyright 2022-2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#ifndef STRATUM_HAL_BIN_TDI_DPDK_DPDK_MAIN_H_
#define STRATUM_HAL_BIN_TDI_DPDK_DPDK_MAIN_H_

#include "stratum/glue/status/status.h"
#include "absl/synchronization/notification.h"

namespace stratum {
namespace hal {
namespace tdi {

::util::Status DpdkMain(int argc, char* argv[]);

::util::Status DpdkMain(absl::Notification* ready_sync = nullptr,
                        absl::Notification* done_sync = nullptr);

void InitCommandLineFlags();

} // namespace tdi
} // namespace hal
} // namespace stratum

#endif  // STRATUM_HAL_BIN_TDI_DPDK_DPDK_MAIN_H_
