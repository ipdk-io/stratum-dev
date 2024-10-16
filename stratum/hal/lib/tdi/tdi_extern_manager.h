// Copyright 2023-2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#ifndef STRATUM_HAL_LIB_TDI_TDI_EXTERN_MANAGER_H_
#define STRATUM_HAL_LIB_TDI_TDI_EXTERN_MANAGER_H_

#include "absl/memory/memory.h"
#include "p4/config/v1/p4info.pb.h"
#include "stratum/hal/lib/p4/p4_extern_manager.h"
#include "stratum/hal/lib/p4/p4_resource_map.h"

namespace stratum {
namespace hal {
namespace tdi {

class TdiExternManager : public P4ExternManager {
 public:
  TdiExternManager() {}
  virtual ~TdiExternManager() = default;

  static std::unique_ptr<TdiExternManager> CreateInstance() {
    return absl::make_unique<TdiExternManager>();
  }

  void Initialize(const ::p4::config::v1::P4Info& p4info,
                  const PreambleCallback& preamble_cb) override {}
};

}  // namespace tdi
}  // namespace hal
}  // namespace stratum

#endif  // STRATUM_HAL_LIB_TDI_TDI_EXTERN_MANAGER_H_
