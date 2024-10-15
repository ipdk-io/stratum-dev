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

  void RegisterResources() override;

 private:
  void RegisterEs2kExterns();
  void RegisterDirectPacketModMeters(const p4::config::v1::Extern& p4extern);
  void RegisterPacketModMeters(const p4::config::v1::Extern& p4extern);

  uint32 invalid_type_ids = 0;
};

}  // namespace tdi
}  // namespace hal
}  // namespace stratum

#endif  // STRATUM_HAL_LIB_TDI_ES2K_ES2K_RESOURCE_MAPPER_H_
