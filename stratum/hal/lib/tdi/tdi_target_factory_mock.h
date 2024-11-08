// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#ifndef STRATUM_HAL_LIB_TDI_TDI_TDI_TARGET_FACTORY_MOCK_H_
#define STRATUM_HAL_LIB_TDI_TDI_TDI_TARGET_FACTORY_MOCK_H_

#include "gmock/gmock.h"
#include "stratum/hal/lib/tdi/tdi/tdi_target_factory.h"

namespace stratum {
namespace hal {
namespace tdi {

class TdiTargetFactoryMock : public TdiTargetFactory {
 public:
  TdiTargetFactoryMock() {}
  ~TdiTargetFactoryMock() override = default;

  MOCK_METHOD(std::unique_ptr<TdiExternManager>, CreateTdiExternManager, (),
              (override));
  MOCK_METHOD(std::unique_ptr<TdiTableAnnex>, CreateTdiTableAnnex, (),
              (override));
};

}  // namespace tdi
}  // namespace hal
}  // namespace stratum

#endif  // STRATUM_HAL_LIB_TDI_TDI_TDI_TARGET_FACTORY_MOCK_H_
