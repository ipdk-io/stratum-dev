// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "stratum/hal/lib/tdi/tdi_target_factory.h"

#include "stratum/hal/lib/tdi/tdi_extern_manager.h"
#include "stratum/hal/lib/tdi/tdi_resource_mapper.h"

namespace stratum {
namespace hal {
namespace tdi {

TdiTargetFactory::TdiTargetFactory() {}
TdiTargetFactory::~TdiTargetFactory() {}

std::unique_ptr<TdiExternManager> TdiTargetFactory::CreateTdiExternManager()
    const {
  return TdiExternManager::CreateInstance();
}

std::unique_ptr<TdiResourceMapper> TdiTargetFactory::CreateTdiResourceMapper()
    const {
  return TdiResourceMapper::CreateInstance();
}

}  // namespace tdi
}  // namespace hal
}  // namespace stratum
