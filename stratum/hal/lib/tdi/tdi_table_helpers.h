// Copyright 2020-present Open Networking Foundation
// Copyright 2022-2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#ifndef STRATUM_HAL_LIB_TDI_TDI_TABLE_HELPERS_H_
#define STRATUM_HAL_LIB_TDI_TDI_TABLE_HELPERS_H_

#include "p4/config/v1/p4info.pb.h"
#include "p4/v1/p4runtime.pb.h"
#include "stratum/glue/status/status_macros.h"
#include "stratum/hal/lib/tdi/tdi_pkt_mod_meter_config.h"
#include "stratum/public/proto/error.pb.h"

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

// Sets a meter configuration variable from a pair of PolicerMeterConfig
// and MeterCounterData protobufs.
void SetPktModMeterConfig(TdiPktModMeterConfig& config,
                          const ::p4::v1::PolicerMeterConfig& meter_config,
                          const ::p4::v1::MeterCounterData& counter_data);

// Convenience function to set a meter configuration variable from a
// MeterEntry protobuf.
inline void SetPktModMeterConfig(TdiPktModMeterConfig& config,
                                 const ::p4::v1::MeterEntry& meter_entry) {
  return SetPktModMeterConfig(config,
                              meter_entry.config().policer_meter_config(),
                              meter_entry.counter_data());
}

// Convenience function to set a meter configuration variable from a
// TableEntry protobuf.
inline void SetPktModMeterConfig(TdiPktModMeterConfig& config,
                                 const ::p4::v1::TableEntry& table_entry) {
  return SetPktModMeterConfig(config,
                              table_entry.meter_config().policer_meter_config(),
                              table_entry.meter_counter_data());
}

// Sets a PolicerMeterConfig protobuf from a meter configuration variable.
void SetPolicerMeterConfig(::p4::v1::PolicerMeterConfig* meter_config,
                           const TdiPktModMeterConfig& cfg);

// Sets a MeterCounterData protobuf from a meter configuration variable.
void SetCounterData(::p4::v1::MeterCounterData* counter_data,
                    const TdiPktModMeterConfig& cfg);

// Convenience function to set a DirectMeterEntry protobuf from a meter
// configuration variable.
inline void SetDirectMeterEntry(::p4::v1::DirectMeterEntry& meter_entry,
                                const TdiPktModMeterConfig& cfg) {
  SetPolicerMeterConfig(
      meter_entry.mutable_config()->mutable_policer_meter_config(), cfg);
  SetCounterData(meter_entry.mutable_counter_data(), cfg);
}

// Convenience function to set a MeterEntry protobuf from a meter
// configuration variable.
inline void SetMeterEntry(::p4::v1::MeterEntry& meter_entry,
                          const TdiPktModMeterConfig& cfg) {
  SetPolicerMeterConfig(
      meter_entry.mutable_config()->mutable_policer_meter_config(), cfg);
  SetCounterData(meter_entry.mutable_counter_data(), cfg);
}

}  // namespace helpers
}  // namespace tdi
}  // namespace hal
}  // namespace stratum

#endif  // STRATUM_HAL_LIB_TDI_TDI_TABLE_HELPERS_H_
