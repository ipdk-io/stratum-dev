// Copyright 2020-present Open Networking Foundation
// Copyright 2022-2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#ifndef STRATUM_HAL_LIB_TDI_TDI_TABLE_HELPERS_H_
#define STRATUM_HAL_LIB_TDI_TDI_TABLE_HELPERS_H_

#include "p4/config/v1/p4info.pb.h"
#include "stratum/glue/status/status_macros.h"

namespace stratum {
namespace hal {
namespace tdi {
namespace helpers {

// Sets a Boolean variable to indicate whether the specified meter is
// configured to measure traffic in packets (true) or bytes (false).
template <typename T>
::util::Status GetMeterUnitsInPackets(const T& meter, bool& units_in_packets) {
  switch (meter.spec().unit()) {
    case ::p4::config::v1::MeterSpec::BYTES:
      units_in_packets = false;
      break;
    case ::p4::config::v1::MeterSpec::PACKETS:
      units_in_packets = true;
      break;
    default:
      return MAKE_ERROR(ERR_INVALID_PARAM) << "Unsupported meter spec on meter "
                                           << meter.ShortDebugString() << ".";
  }
  return ::util::OkStatus();
}

}  // namespace helpers
}  // namespace tdi
}  // namespace hal
}  // namespace stratum

#endif  // STRATUM_HAL_LIB_TDI_TDI_TABLE_HELPERS_H_
