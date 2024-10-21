// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#ifndef STRATUM_HAL_LIB_TDI_ES2K_ES2K_TARGET_FACTORY_H_
#define STRATUM_HAL_LIB_TDI_ES2K_ES2K_TARGET_FACTORY_H_

#include <memory>
#include <utility>

#include "stratum/hal/lib/tdi/es2k/es2k_extern_manager.h"
#include "stratum/hal/lib/tdi/es2k/es2k_table_helper.h"
#include "stratum/hal/lib/tdi/tdi_table_helper.h"
#include "stratum/hal/lib/tdi/tdi_target_factory.h"

namespace stratum {
namespace hal {
namespace tdi {

// Creates ES2K-specific instances of polymorphic classes.
class Es2kTargetFactory : public TdiTargetFactory {
 public:
  Es2kTargetFactory() {}
  virtual ~Es2kTargetFactory() = default;

  std::unique_ptr<TdiExternManager> CreateTdiExternManager() override {
    auto es2kPtr = Es2kExternManager::CreateInstance();
    return std::unique_ptr<TdiExternManager>(std::move(es2kPtr));
  }

  std::unique_ptr<TdiTableHelper> CreateTdiTableHelper() override {
    auto es2kPtr = Es2kTableHelper::CreateInstance();
    return std::unique_ptr<TdiTableHelper>(std::move(es2kPtr));
  }
};

}  // namespace tdi
}  // namespace hal
}  // namespace stratum

#endif  // STRATUM_HAL_LIB_TDI_ES2K_ES2K_TARGET_FACTORY_H_
