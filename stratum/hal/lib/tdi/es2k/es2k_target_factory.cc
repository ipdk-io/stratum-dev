// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "stratum/hal/lib/tdi/es2k/es2k_target_factory.h"

#include <memory>

#include "absl/memory/memory.h"
#include "stratum/hal/lib/tdi/es2k/es2k_extern_manager.h"
#include "stratum/hal/lib/tdi/es2k/es2k_resource_mapper.h"

namespace stratum {
namespace hal {
namespace tdi {

Es2kTargetFactory::Es2kTargetFactory() {}
Es2kTargetFactory::~Es2kTargetFactory() {}

std::unique_ptr<TdiExternManager> Es2kTargetFactory::CreateTdiExternManager()
    const {
  return absl::make_unique<Es2kExternManager>();
}

std::unique_ptr<TdiResourceMapper> Es2kTargetFactory::CreateTdiResourceMapper()
    const {
  return absl::make_unique<Es2kResourceMapper>();
}

}  // namespace tdi
}  // namespace hal
}  // namespace stratum
