// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "stratum/hal/lib/tdi/es2k/es2k_resource_mapper.h"

namespace stratum {
namespace hal {
namespace tdi {

::util::Status Es2kResourceMapper::Initialize(TdiSdeInterface* sde_interface,
                                              P4InfoManager* p4_info_manager,
                                              TdiExternManager* extern_manager,
                                              absl::Mutex* lock, int device) {
  // Add TDI resources to map.
  RETURN_IF_ERROR(TdiResourceMapper::Initialize(sde_interface, p4_info_manager,
                                                extern_manager, lock, device));
  // Add ES2K resources to map.
  RETURN_IF_ERROR(RegisterEs2kExterns());

  return ::util::OkStatus();
}

::util::Status Es2kResourceMapper::RegisterEs2kExterns() {
  return ::util::OkStatus();
}

}  // namespace tdi
}  // namespace hal
}  // namespace stratum
