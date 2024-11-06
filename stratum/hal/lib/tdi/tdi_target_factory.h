// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#ifndef STRATUM_HAL_LIB_TDI_TDI_TARGET_FACTORY_H_
#define STRATUM_HAL_LIB_TDI_TDI_TARGET_FACTORY_H_

#include <memory>

#include "stratum/hal/lib/tdi/tdi_extern_manager.h"
#include "stratum/hal/lib/tdi/tdi_table_annex.h"

namespace stratum {
namespace hal {
namespace tdi {

// TdiTargetFactory is a polymorphic class that provides Factory Methods
// which TdiTableManager can use to create instances of other polymorphic
// classes.
//
// - TdiTargetFactory creates TdiExternManager and TdiTableAnnex
//   objects, for use by the DPDK and Tofino targets.
//
// - Es2kTargetFactory creates Es2kExternManager and Es2kTableAnnex
//   objects, for use by the ES2K target.
//
// Its purpose is to insulate TdiTableManager from target-specific
// dependencies, such as the P4Runtime extensions that support the
// PacketModMeter and DirectPacketModMeter resources.
//
class TdiTargetFactory {
 public:
  TdiTargetFactory() {}
  virtual ~TdiTargetFactory() = default;

  virtual std::unique_ptr<TdiExternManager> CreateTdiExternManager() {
    return TdiExternManager::CreateInstance();
  }

  virtual std::unique_ptr<TdiTableAnnex> CreateTdiTableAnnex() {
    return TdiTableAnnex::CreateInstance();
  }
};

}  // namespace tdi
}  // namespace hal
}  // namespace stratum

#endif  // STRATUM_HAL_LIB_TDI_TDI_TARGET_FACTORY_H_
