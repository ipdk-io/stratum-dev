// Copyright 2020-present Open Networking Foundation
// Copyright 2022-2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "stratum/hal/lib/tdi/tdi_table_helpers.h"

namespace stratum {
namespace hal {
namespace tdi {
namespace helpers {

// Sets a meter configuration variable from a pair of PolicerMeterConfig
// and MeterCounterData protobufs.
void SetPktModMeterConfig(TdiPktModMeterConfig& config,
                          const ::p4::v1::PolicerMeterConfig& meter_config,
                          const ::p4::v1::MeterCounterData& counter_data) {
  config.meter_prof_id = meter_config.policer_meter_prof_id();
  config.cir_unit = meter_config.policer_spec_cir_unit();
  config.cburst_unit = meter_config.policer_spec_cbs_unit();
  config.pir_unit = meter_config.policer_spec_eir_unit();
  config.pburst_unit = meter_config.policer_spec_ebs_unit();
  config.cir = meter_config.policer_spec_cir();
  config.cburst = meter_config.policer_spec_cbs();
  config.pir = meter_config.policer_spec_eir();
  config.pburst = meter_config.policer_spec_ebs();

  config.greenBytes = counter_data.green().byte_count();
  config.greenPackets = counter_data.green().packet_count();
  config.yellowBytes = counter_data.yellow().byte_count();
  config.yellowPackets = counter_data.yellow().packet_count();
  config.redBytes = counter_data.red().byte_count();
  config.redPackets = counter_data.red().packet_count();
}

// Sets a PolicerMeterConfig protobuf from a meter configuration variable.
void SetPolicerMeterConfig(::p4::v1::PolicerMeterConfig* meter_config,
                           const TdiPktModMeterConfig& cfg) {
  meter_config->set_policer_meter_prof_id(
      static_cast<int64>(cfg.meter_prof_id));
  meter_config->set_policer_spec_cir_unit(static_cast<int64>(cfg.cir_unit));
  meter_config->set_policer_spec_cbs_unit(static_cast<int64>(cfg.cburst_unit));
  meter_config->set_policer_spec_eir_unit(static_cast<int64>(cfg.pir_unit));
  meter_config->set_policer_spec_ebs_unit(static_cast<int64>(cfg.pburst_unit));
  meter_config->set_policer_spec_cir(static_cast<int64>(cfg.cir));
  meter_config->set_policer_spec_cbs(static_cast<int64>(cfg.cburst));
  meter_config->set_policer_spec_eir(static_cast<int64>(cfg.pir));
  meter_config->set_policer_spec_ebs(static_cast<int64>(cfg.pburst));
}

// Sets a MeterCounterData protobuf from a meter configuration variable.
void SetCounterData(::p4::v1::MeterCounterData* counter_data,
                    const TdiPktModMeterConfig& cfg) {
  counter_data->mutable_green()->set_byte_count(
      static_cast<int64>(cfg.greenBytes));
  counter_data->mutable_green()->set_packet_count(
      static_cast<int64>(cfg.greenPackets));
  counter_data->mutable_yellow()->set_byte_count(
      static_cast<int64>(cfg.yellowBytes));
  counter_data->mutable_yellow()->set_packet_count(
      static_cast<int64>(cfg.yellowPackets));
  counter_data->mutable_red()->set_byte_count(static_cast<int64>(cfg.redBytes));
  counter_data->mutable_red()->set_packet_count(
      static_cast<int64>(cfg.redPackets));
}

}  // namespace helpers
}  // namespace tdi
}  // namespace hal
}  // namespace stratum
