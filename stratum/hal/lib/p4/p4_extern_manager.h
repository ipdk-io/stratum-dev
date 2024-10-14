// Copyright 2023-2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#ifndef STRATUM_HAL_LIB_P4_P4_EXTERN_MANAGER_H_
#define STRATUM_HAL_LIB_P4_P4_EXTERN_MANAGER_H_

#include "p4/config/v1/p4info.pb.h"
#include "stratum/hal/lib/p4/p4_resource_map.h"

namespace stratum {
namespace hal {

class P4ExternManager {
 public:
  virtual ~P4ExternManager() = default;

  virtual void Initialize(const ::p4::config::v1::P4Info& p4info,
                          const PreambleCallback& preamble_pb) = 0;

 protected:
  // Default constructor.
  P4ExternManager() {}
};

}  // namespace hal
}  // namespace stratum

#endif  // STRATUM_HAL_LIB_P4_P4_EXTERN_MANAGER_H_
