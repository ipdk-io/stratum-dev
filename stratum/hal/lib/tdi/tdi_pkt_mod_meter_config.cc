// Copyright 2022-2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "stratum/hal/lib/tdi/tdi_pkt_mod_meter_config.h"

namespace stratum {
namespace hal {
namespace tdi {

void TdiPktModMeterConfig::SetPolicerMeterConfig(
    const ::p4::v1::PolicerMeterConfig& meter_config) {
  meter_prof_id = meter_config.policer_meter_prof_id();
  cir_unit = meter_config.policer_spec_cir_unit();
  cburst_unit = meter_config.policer_spec_cbs_unit();
  pir_unit = meter_config.policer_spec_eir_unit();
  pburst_unit = meter_config.policer_spec_ebs_unit();
  cir = meter_config.policer_spec_cir();
  cburst = meter_config.policer_spec_cbs();
  pir = meter_config.policer_spec_eir();
  pburst = meter_config.policer_spec_ebs();
}

void TdiPktModMeterConfig::SetMeterCounterData(
    const ::p4::v1::MeterCounterData& counter_data) {
  greenBytes = counter_data.green().byte_count();
  greenPackets = counter_data.green().packet_count();
  yellowBytes = counter_data.yellow().byte_count();
  yellowPackets = counter_data.yellow().packet_count();
  redBytes = counter_data.red().byte_count();
  redPackets = counter_data.red().packet_count();
}

void TdiPktModMeterConfig::GetPolicerMeterConfig(
    ::p4::v1::PolicerMeterConfig* meter_config) {
  meter_config->set_policer_meter_prof_id(static_cast<int64>(meter_prof_id));
  meter_config->set_policer_spec_cir_unit(static_cast<int64>(cir_unit));
  meter_config->set_policer_spec_cbs_unit(static_cast<int64>(cburst_unit));
  meter_config->set_policer_spec_eir_unit(static_cast<int64>(pir_unit));
  meter_config->set_policer_spec_ebs_unit(static_cast<int64>(pburst_unit));
  meter_config->set_policer_spec_cir(static_cast<int64>(cir));
  meter_config->set_policer_spec_cbs(static_cast<int64>(cburst));
  meter_config->set_policer_spec_eir(static_cast<int64>(pir));
  meter_config->set_policer_spec_ebs(static_cast<int64>(pburst));
}

void TdiPktModMeterConfig::GetMeterCounterData(
    ::p4::v1::MeterCounterData* counter_data) {
  counter_data->mutable_green()->set_byte_count(static_cast<int64>(greenBytes));
  counter_data->mutable_green()->set_packet_count(
      static_cast<int64>(greenPackets));
  counter_data->mutable_yellow()->set_byte_count(
      static_cast<int64>(yellowBytes));
  counter_data->mutable_yellow()->set_packet_count(
      static_cast<int64>(yellowPackets));
  counter_data->mutable_red()->set_byte_count(static_cast<int64>(redBytes));
  counter_data->mutable_red()->set_packet_count(static_cast<int64>(redPackets));
}

}  // namespace tdi
}  // namespace hal
}  // namespace stratum
