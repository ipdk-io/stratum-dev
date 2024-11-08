// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#ifndef STRATUM_HAL_LIB_TDI_ES2K_ES2K_TARGET_FACTORY_MOCK_H_
#define STRATUM_HAL_LIB_TDI_ES2K_ES2K_TARGET_FACTORY_MOCK_H_

#include <memory>

#include "gmock/gmock.h"
#include "stratum/hal/lib/tdi/es2k/es2k_target_factory.h"

namespace stratum {
namespace hal {
namespace tdi {

class Es2kTargetFactoryMock : public Es2kTargetFactory {
 public:
  Es2kTargetFactoryMock() {}
  ~Es2kTargetFactoryMock() override = default;

  MOCK_METHOD(std::unique_ptr<TdiExternManager>, CreateTdiExternManager, (),
              (override));
  MOCK_METHOD(std::unique_ptr<TdiTableAnnex>, CreateTdiTableAnnex, (),
              (override));
};

}  // namespace tdi
}  // namespace hal
}  // namespace stratum

#endif  // STRATUM_HAL_LIB_TDI_ES2K_ES2K_TARGET_FACTORY_MOCK_H_
