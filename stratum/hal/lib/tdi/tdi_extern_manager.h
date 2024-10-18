// Copyright 2023-2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#ifndef STRATUM_HAL_LIB_TDI_TDI_EXTERN_MANAGER_H_
#define STRATUM_HAL_LIB_TDI_TDI_EXTERN_MANAGER_H_

#include "absl/memory/memory.h"
#include "p4/config/v1/p4info.pb.h"
#include "stratum/hal/lib/p4/p4_extern_manager.h"
#include "stratum/hal/lib/p4/p4_resource_map.h"
#include "stratum/hal/lib/tdi/tdi_resource_handler.h"

namespace stratum {
namespace hal {
namespace tdi {

class TdiExternManager : public P4ExternManager {
 public:
  TdiExternManager() {}
  virtual ~TdiExternManager() = default;

  // Called by TdiTargetFactory.
  static std::unique_ptr<TdiExternManager> CreateInstance() {
    return absl::make_unique<TdiExternManager>();
  }

  // Performs basic initialization. Called by TdiTableManager.
  virtual ::util::Status Initialize(TdiSdeInterface* sde_interface,
                                    P4InfoManager* p4_info_manager,
                                    absl::Mutex* lock, int device) {
    return ::util::OkStatus();
  }

  // Registers P4Extern resources. Called by P4InfoManager.
  void RegisterExterns(const ::p4::config::v1::P4Info& p4info,
                       const PreambleCallback& preamble_cb) override {}

  // Returns the handler for the P4 resource with the specified ID,
  // or null if the resource is not registered.
  virtual TdiResourceHandler* FindResourceHandler(uint32 resource_id) {
    return nullptr;
  }
};

}  // namespace tdi
}  // namespace hal
}  // namespace stratum

#endif  // STRATUM_HAL_LIB_TDI_TDI_EXTERN_MANAGER_H_
