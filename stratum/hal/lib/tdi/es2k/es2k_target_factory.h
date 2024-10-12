// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#ifndef STRATUM_HAL_LIB_TDI_ES2K_ES2K_TARGET_FACTORY_H_
#define STRATUM_HAL_LIB_TDI_ES2K_ES2K_TARGET_FACTORY_H_

#include <memory>

#include "absl/memory/memory.h"
#include "stratum/hal/lib/tdi/es2k/es2k_extern_manager.h"
#include "stratum/hal/lib/tdi/tdi_target_factory.h"

namespace stratum {
namespace hal {

class Es2kTargetFactory : public TdiTargetFactory {
 public:
  Es2kTargetFactory() {}
  virtual ~Es2kTargetFactory() = default;

  std::unique_ptr<TdiExternManager> CreateTdiExternManager() const override {
    return absl::make_unique<Es2kExternManager>();
  }
};

}  // namespace hal
}  // namespace stratum

#endif  // STRATUM_HAL_LIB_TDI_ES2K_ES2K_TARGET_FACTORY_H_
