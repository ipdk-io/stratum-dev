// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "stratum/hal/lib/tdi/es2k/es2k_target_factory.h"

#include <memory>
#include <utility>

#include "stratum/hal/lib/tdi/es2k/es2k_extern_manager.h"
#include "stratum/hal/lib/tdi/es2k/es2k_table_annex.h"

namespace stratum {
namespace hal {
namespace tdi {

std::unique_ptr<TdiExternManager> Es2kTargetFactory::CreateTdiExternManager() {
  auto es2kPtr = Es2kExternManager::CreateInstance();
  return std::unique_ptr<TdiExternManager>(std::move(es2kPtr));
}

std::unique_ptr<TdiTableAnnex> Es2kTargetFactory::CreateTdiTableAnnex() {
  auto es2kPtr = Es2kTableAnnex::CreateInstance();
  return std::unique_ptr<TdiTableAnnex>(std::move(es2kPtr));
}

}  // namespace tdi
}  // namespace hal
}  // namespace stratum
