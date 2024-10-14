// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#ifndef STRATUM_HAL_LIB_TDI_TDI_RESOURCE_MAPPER_H_
#define STRATUM_HAL_LIB_TDI_TDI_RESOURCE_MAPPER_H_

#include <memory>

#include "absl/container/flat_hash_map.h"
#include "p4/config/v1/p4info.pb.h"
#include "stratum/glue/integral_types.h"
#include "stratum/glue/status/status.h"
#include "stratum/hal/lib/p4/p4_info_manager.h"
#include "stratum/hal/lib/tdi/tdi_resource_handler.h"

namespace stratum {
namespace hal {
namespace tdi {

class TdiExternManager;

class TdiResourceMapper {
 public:
  typedef std::shared_ptr<TdiResourceHandler> ResourceHandler;

  TdiResourceMapper();
  virtual ~TdiResourceMapper() = default;

  static std::unique_ptr<TdiResourceMapper> CreateInstance();

  virtual ::util::Status Initialize(TdiSdeInterface* sde_interface,
                                    P4InfoManager* p4_info_manager,
                                    TdiExternManager* extern_manager,
                                    absl::Mutex* lock, int device);

 protected:
  ::util::Status RegisterDirectCounters();
  ::util::Status RegisterDirectMeters();
  ::util::Status RegisterMeters();

  ::util::Status RegisterResource(const ::p4::config::v1::Preamble& preamble,
                                  const std::string& resource_type,
                                  ResourceHandler resource_handler);

  ::util::Status AddMapEntry(uint32 resource_id,
                             const std::string& resource_type,
                             ResourceHandler resource_handler);

  TdiSdeInterface* tdi_sde_interface_;      // not owned by this class
  P4InfoManager* p4_info_manager_;          // not owned by this class
  TdiExternManager* tdi_extern_manager_;    // not owned by this class
  const ::p4::config::v1::P4Info* p4info_;  // not owned by this class
  absl::Mutex* lock_;                       // not owned by this class
  int device_;

  absl::flat_hash_map<uint32, ResourceHandler> resource_handler_map_;
};

}  // namespace tdi
}  // namespace hal
}  // namespace stratum

#endif  // STRATUM_HAL_LIB_TDI_TDI_RESOURCE_MAPPER_H_
