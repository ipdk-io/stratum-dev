// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#ifndef STRATUM_HAL_LIB_TDI_TDI_TARGET_FACTORY_H_
#define STRATUM_HAL_LIB_TDI_TDI_TARGET_FACTORY_H_

#include <memory>

#include "stratum/hal/lib/tdi/tdi_extern_manager.h"
#include "stratum/hal/lib/tdi/tdi_table_helper.h"

namespace stratum {
namespace hal {
namespace tdi {

// Creates target-specific instances of polymorphic classes.
class TdiTargetFactory {
 public:
  TdiTargetFactory() {}
  virtual ~TdiTargetFactory() = default;

  virtual std::unique_ptr<TdiExternManager> CreateTdiExternManager() {
    return TdiExternManager::CreateInstance();
  }

  virtual std::unique_ptr<TdiTableHelper> CreateTdiTableHelper() {
    return TdiTableHelper::CreateInstance();
  }
};

}  // namespace tdi
}  // namespace hal
}  // namespace stratum

#endif  // STRATUM_HAL_LIB_TDI_TDI_TARGET_FACTORY_H_
