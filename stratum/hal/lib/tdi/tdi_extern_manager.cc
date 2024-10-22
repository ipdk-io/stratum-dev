// Copyright 2023-2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "stratum/hal/lib/tdi/tdi_extern_manager.h"

#include "stratum/glue/status/status_macros.h"

namespace stratum {
namespace hal {
namespace tdi {

// Retrieve a PacketModMeter configuration.
::util::StatusOr<const ::idpf::PacketModMeter>
TdiExternManager::FindPktModMeterByID(uint32 meter_id) const {
  return MAKE_ERROR(ERR_UNIMPLEMENTED) << __func__ << " not implemented.";
}

::util::StatusOr<const ::idpf::PacketModMeter>
TdiExternManager::FindPktModMeterByName(const std::string& meter_name) const {
  return MAKE_ERROR(ERR_UNIMPLEMENTED) << __func__ << " not implemented.";
}

// Retrieve a DirectPacketModMeter configuration.
::util::StatusOr<const ::idpf::DirectPacketModMeter>
TdiExternManager::FindDirectPktModMeterByID(uint32 meter_id) const {
  return MAKE_ERROR(ERR_UNIMPLEMENTED) << __func__ << " not implemented.";
}

::util::StatusOr<const ::idpf::DirectPacketModMeter>
TdiExternManager::FindDirectPktModMeterByName(
    const std::string& meter_name) const {
  return MAKE_ERROR(ERR_UNIMPLEMENTED) << __func__ << " not implemented.";
}

}  // namespace tdi
}  // namespace hal
}  // namespace stratum
