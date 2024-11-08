// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "stratum/hal/lib/tdi/tdi_target_factory.h"

#include <memory>

#include "stratum/hal/lib/tdi/tdi_extern_manager.h"
#include "stratum/hal/lib/tdi/tdi_table_annex.h"

namespace stratum {
namespace hal {
namespace tdi {

std::unique_ptr<TdiExternManager> TdiTargetFactory::CreateTdiExternManager() {
  return TdiExternManager::CreateInstance();
}

std::unique_ptr<TdiTableAnnex> TdiTargetFactory::CreateTdiTableAnnex() {
  return TdiTableAnnex::CreateInstance();
}

}  // namespace tdi
}  // namespace hal
}  // namespace stratum
