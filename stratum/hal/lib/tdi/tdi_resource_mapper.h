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

  ::util::Status Initialize(TdiSdeInterface* sde_interface,
                            P4InfoManager* p4_info_manager,
                            TdiExternManager* extern_manager, absl::Mutex* lock,
                            int device);

  ::util::StatusOr<TdiResourceHandler*> FindResourceHandler(uint32 resource_id);

  bool empty() const { return resource_handler_map_.empty(); }
  uint32 size() const { return resource_handler_map_.size(); }

 protected:
  virtual void RegisterResources();

  void RegisterResource(const ::p4::config::v1::Preamble& preamble,
                        ResourceHandler resource_handler);

 private:
  void RegisterDirectCounters();
  void RegisterDirectMeters();
  void RegisterMeters();

 protected:
  TdiSdeInterface* tdi_sde_interface_;      // not owned by this class
  P4InfoManager* p4_info_manager_;          // not owned by this class
  TdiExternManager* tdi_extern_manager_;    // not owned by this class
  const ::p4::config::v1::P4Info* p4info_;  // not owned by this class
  absl::Mutex* lock_;                       // not owned by this class
  int device_;

  absl::flat_hash_map<uint32, ResourceHandler> resource_handler_map_;
  uint32 invalid_entities = 0;
};

}  // namespace tdi
}  // namespace hal
}  // namespace stratum

#endif  // STRATUM_HAL_LIB_TDI_TDI_RESOURCE_MAPPER_H_
