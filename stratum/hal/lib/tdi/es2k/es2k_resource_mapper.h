// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#ifndef STRATUM_HAL_LIB_TDI_ES2K_ES2K_RESOURCE_MAPPER_H_
#define STRATUM_HAL_LIB_TDI_ES2K_ES2K_RESOURCE_MAPPER_H_

#include "stratum/hal/lib/tdi/tdi_resource_mapper.h"

namespace stratum {
namespace hal {
namespace tdi {

class Es2kResourceMapper : public TdiResourceMapper {
 public:
  Es2kResourceMapper() {}
  virtual ~Es2kResourceMapper() = default;

  util::Status Initialize(TdiSdeInterface* sde_interface,
                          P4InfoManager* p4_info_manager,
                          TdiExternManager* extern_manager, absl::Mutex* lock,
                          int device) override;

 protected:
  ::util::Status RegisterEs2kExterns();
};

}  // namespace tdi
}  // namespace hal
}  // namespace stratum

#endif  // STRATUM_HAL_LIB_TDI_ES2K_ES2K_RESOURCE_MAPPER_H_
