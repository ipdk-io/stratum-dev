// Copyright 2022-2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#ifndef STRATUM_HAL_LIB_TDI_TDI_PKT_MOD_METER_CONFIG_
#define STRATUM_HAL_LIB_TDI_TDI_PKT_MOD_METER_CONFIG_

#include "p4/config/v1/p4runtime.pb.h"
#include "stratum/glue/integral_types.h"

namespace stratum {
namespace hal {
namespace tdi {

// ES2K PacketModMeter Configuration and stats structure
struct TdiPktModMeterConfig {
  TdiPktModMeterConfig()
      : meter_prof_id(0),
        cir_unit(0),
        cburst_unit(0),
        pir_unit(0),
        pburst_unit(0),
        cir(0),
        cburst(0),
        pir(0),
        pburst(0),
        greenBytes(0),
        greenPackets(0),
        yellowBytes(0),
        yellowPackets(0),
        redBytes(0),
        redPackets(0),
        isPktModMeter(false) {}

  ~TdiPktModMeterConfig() = default;

  // PolicerMeterConfig
  uint64 meter_prof_id;  // Meter Profile ID
  uint64 cir_unit;       // Committed Information Rate Unit
  uint64 cburst_unit;    // Committed Burst Unit
  uint64 pir_unit;       // Peak Information Rate Unit
  uint64 pburst_unit;    // Peak Burst Unit
  uint64 cir;            // Committed Information
  uint64 cburst;         // Committed Burst Size
  uint64 pir;            // Peak Information Rate
  uint64 pburst;         // Peak Burst Size

  // MeterCounterData
  uint64 greenBytes;     // Green Byte counter
  uint64 greenPackets;   // Green Packet counter
  uint64 yellowBytes;    // Yellow Byte counter
  uint64 yellowPackets;  // Yellow Packet counter
  uint64 redBytes;       // Red Byte counter
  uint64 redPackets;     // Red Packet counter

  bool isPktModMeter;    // Flag to check direct or indirect PacketModMeter

  // 'Set' methods retrieve data from the specified object and store it in
  // the configuration. The parameter is always a const reference.

  void SetPolicerMeterConfig(const ::p4::v1::PolicerMeterConfig& meter_config);
  void SetMeterCounterData(const ::p4::v1::MeterCounterData& counter_data);

  inline void SetMeterEntry(const ::p4::v1::MeterEntry& meter_entry) {
    SetPolicerMeterConfig(meter_entry.config().policer_meter_config);
    SetMeterCounterData(meter_entry.counter_data());
  }

  inline void SetTableEntry(const ::p4::v1::TableEntry& table_entry) {
    SetPolicerMeterConfig(table_entry.meter_config().policer_meter_config());
    SetMeterCounterData(table_entry.meter_counter_data());
  }

  // 'Get' methods retrieve data from the configuration and store it in
  // the specified object. The parameter is always a pointer.

  void GetPolicerMeterConfig(::p4::v1::PolicerMeterConfig* meter_config);
  void GetMeterCounterData(::p4::v1::MeterCounterData* counter_data);

  inline void GetDirectMeterEntry(::p4::v1::DirectMeterEntry* meter_entry) {
    GetPolicerMeterConfig(
        meter_entry->mutable_config()->mutable_policer_meter_config());
    GetMeterCounterData(meter_entry->mutable_counter_data());
  }

  inline void SetMeterEntry(::p4::v1::MeterEntry* meter_entry) {
    GetPolicerMeterConfig(
        meter_entry->mutable_config()->mutable_policer_meter_config());
    GetMeterCounterData(meter_entry->mutable_counter_data());
  }
};

}  // namespace tdi
}  // namespace hal
}  // namespace stratum

#endif  // STRATUM_HAL_LIB_TDI_TDI_PKT_MOD_METER_CONFIG_
