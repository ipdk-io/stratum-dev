// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#ifndef STRATUM_HAL_LIB_TDI_TDI_TARGET_FACTORY_H_
#define STRATUM_HAL_LIB_TDI_TDI_TARGET_FACTORY_H_

#include <memory>

#include "stratum/hal/lib/tdi/tdi_extern_manager.h"

namespace stratum {
namespace hal {

class TdiTargetFactory {
 public:
  TdiTargetFactory() {}
  virtual ~TdiTargetFactory() = default;

  virtual std::unique_ptr<TdiExternManager> CreateTdiExternManager() const {
    return nullptr;
  }
};

}  // namespace hal
}  // namespace stratum

#endif  // STRATUM_HAL_LIB_TDI_TDI_TARGET_FACTORY_H_
