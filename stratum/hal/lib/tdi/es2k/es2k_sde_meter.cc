// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ES2K-specific PktModMeterConfig methods.

#include <stddef.h>
#include <stdint.h>

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "absl/synchronization/mutex.h"
#include "absl/types/optional.h"
#include "stratum/glue/integral_types.h"
#include "stratum/glue/status/status.h"
#include "stratum/glue/status/status_macros.h"
#include "stratum/hal/lib/tdi/tdi_bf_status.h"
#include "stratum/hal/lib/tdi/tdi_constants.h"
#include "stratum/hal/lib/tdi/tdi_pkt_mod_meter_config.h"
#include "stratum/hal/lib/tdi/tdi_sde_common.h"
#include "stratum/hal/lib/tdi/tdi_sde_helpers.h"
#include "stratum/hal/lib/tdi/tdi_sde_wrapper.h"
#include "stratum/lib/macros.h"
#include "stratum/public/proto/error.pb.h"

namespace stratum {
namespace hal {
namespace tdi {

using namespace stratum::hal::tdi::helpers;

::util::Status TdiSdeWrapper::WritePktModMeter(
    int dev_id, std::shared_ptr<TdiSdeInterface::SessionInterface> session,
    uint32 table_id, absl::optional<uint32> meter_index,
    TdiPktModMeterConfig& cfg) {
  ::absl::ReaderMutexLock l(&data_lock_);
  auto real_session = std::dynamic_pointer_cast<Session>(session);
  RET_CHECK(real_session);

  const ::tdi::Table* table;
  RETURN_IF_TDI_ERROR(tdi_info_->tableFromIdGet(table_id, &table));

  std::unique_ptr<::tdi::TableKey> table_key;
  std::unique_ptr<::tdi::TableData> table_data;
  RETURN_IF_TDI_ERROR(table->keyAllocate(&table_key));
  RETURN_IF_TDI_ERROR(table->dataAllocate(&table_data));

  // Meter data: $METER_SPEC_*
  RETURN_IF_ERROR(
      SetField(table_data.get(), kEs2kMeterProfileIdKPps, cfg.meter_prof_id));
  RETURN_IF_ERROR(
      SetField(table_data.get(), kEs2kMeterCirKPpsUnit, cfg.cir_unit));
  RETURN_IF_ERROR(SetField(table_data.get(), kEs2kMeterCommitedBurstPacketsUnit,
                           cfg.cburst_unit));
  RETURN_IF_ERROR(
      SetField(table_data.get(), kEs2kMeterPirPpsUnit, cfg.pir_unit));
  RETURN_IF_ERROR(SetField(table_data.get(), kEs2kMeterPeakBurstPacketsUnit,
                           cfg.pburst_unit));
  RETURN_IF_ERROR(SetField(table_data.get(), kEs2kMeterCirPps,
                           BytesPerSecondToKbits(cfg.cir)));
  RETURN_IF_ERROR(SetField(table_data.get(), kEs2kMeterCommitedBurstPackets,
                           BytesPerSecondToKbits(cfg.cburst)));
  RETURN_IF_ERROR(SetField(table_data.get(), kEs2kMeterPirPps,
                           BytesPerSecondToKbits(cfg.pir)));
  RETURN_IF_ERROR(SetField(table_data.get(), kEs2kMeterPeakBurstPackets,
                           BytesPerSecondToKbits(cfg.pburst)));
  RETURN_IF_ERROR(SetField(table_data.get(), kEs2kMeterGreenCounterBytes,
                           BytesPerSecondToKbits(cfg.greenBytes)));
  RETURN_IF_ERROR(SetField(table_data.get(), kEs2kMeterGreenCounterPackets,
                           BytesPerSecondToKbits(cfg.greenPackets)));
  RETURN_IF_ERROR(SetField(table_data.get(), kEs2kMeterYellowCounterBytes,
                           BytesPerSecondToKbits(cfg.yellowBytes)));
  RETURN_IF_ERROR(SetField(table_data.get(), kEs2kMeterYellowCounterPackets,
                           BytesPerSecondToKbits(cfg.yellowPackets)));
  RETURN_IF_ERROR(SetField(table_data.get(), kEs2kMeterRedCounterBytes,
                           BytesPerSecondToKbits(cfg.redBytes)));
  RETURN_IF_ERROR(SetField(table_data.get(), kEs2kMeterRedCounterPackets,
                           BytesPerSecondToKbits(cfg.redPackets)));

  //    RETURN_IF_ERROR(SetPktModMeterConfig(config));
  // write code here of set config
  const ::tdi::Device* device = nullptr;
  ::tdi::DevMgr::getInstance().deviceGet(dev_id, &device);
  std::unique_ptr<::tdi::Target> dev_tgt;
  device->createTarget(&dev_tgt);

  const auto flags = ::tdi::Flags(0);
  if (meter_index) {
    // Single index target.
    // Meter key: $METER_INDEX
    RETURN_IF_ERROR(
        SetFieldExact(table_key.get(), kMeterIndex, meter_index.value()));
    RETURN_IF_TDI_ERROR(table->entryAdd(*real_session->tdi_session_, *dev_tgt,
                                        flags, *table_key, *table_data));
  } else {
    // Wildcard write to all indices.
    size_t table_size;
    RETURN_IF_TDI_ERROR(table->sizeGet(*real_session->tdi_session_, *dev_tgt,
                                       flags, &table_size));
    for (size_t i = 0; i < table_size; ++i) {
      // Meter key: $METER_INDEX
      RETURN_IF_ERROR(SetFieldExact(table_key.get(), kMeterIndex, i));
      RETURN_IF_TDI_ERROR(table->entryAdd(*real_session->tdi_session_, *dev_tgt,
                                          flags, *table_key, *table_data));
    }
  }

  return ::util::OkStatus();
}

::util::Status TdiSdeWrapper::ReadPktModMeters(
    int dev_id, std::shared_ptr<TdiSdeInterface::SessionInterface> session,
    uint32 table_id, absl::optional<uint32> meter_index,
    std::vector<uint32>* meter_indices,
    std::vector<TdiPktModMeterConfig>& cfg) {
  RET_CHECK(meter_indices);
  // RET_CHECK(cfg);
  ::absl::ReaderMutexLock l(&data_lock_);
  auto real_session = std::dynamic_pointer_cast<Session>(session);
  RET_CHECK(real_session);

  const ::tdi::Device* device = nullptr;
  ::tdi::DevMgr::getInstance().deviceGet(dev_id, &device);
  std::unique_ptr<::tdi::Target> dev_tgt;
  device->createTarget(&dev_tgt);

  const auto flags = ::tdi::Flags(0);
  const ::tdi::Table* table;
  RETURN_IF_TDI_ERROR(tdi_info_->tableFromIdGet(table_id, &table));
  std::vector<std::unique_ptr<::tdi::TableKey>> keys;
  std::vector<std::unique_ptr<::tdi::TableData>> datums;

  // Is this a wildcard read?
  if (meter_index) {
    keys.resize(1);
    datums.resize(1);
    RETURN_IF_TDI_ERROR(table->keyAllocate(&keys[0]));
    RETURN_IF_TDI_ERROR(table->dataAllocate(&datums[0]));

    // Key: $METER_INDEX
    RETURN_IF_ERROR(
        SetFieldExact(keys[0].get(), kMeterIndex, meter_index.value()));
    RETURN_IF_TDI_ERROR(table->entryGet(*real_session->tdi_session_, *dev_tgt,
                                        flags, *keys[0], datums[0].get()));
  } else {
    RETURN_IF_ERROR(GetAllEntries(real_session->tdi_session_, *dev_tgt, table,
                                  &keys, &datums));
  }

  meter_indices->resize(0);
  cfg.resize(keys.size());
  // cfg->resize(0);
  for (size_t i = 0; i < keys.size(); ++i) {
    const std::unique_ptr<::tdi::TableData>& table_data = datums[i];
    const std::unique_ptr<::tdi::TableKey>& table_key = keys[i];
    // Key: $METER_INDEX
    uint32_t tdi_meter_index = 0;
    RETURN_IF_ERROR(GetFieldExact(*table_key, kMeterIndex, &tdi_meter_index));
    meter_indices->push_back(tdi_meter_index);

    // Data: $METER_SPEC_*
    std::vector<tdi_id_t> data_field_ids;
    data_field_ids = table->tableInfoGet()->dataFieldIdListGet();
    for (const auto& field_id : data_field_ids) {
      std::string field_name;
      const ::tdi::DataFieldInfo* dataFieldInfo;
      dataFieldInfo = table->tableInfoGet()->dataFieldGet(field_id);
      RETURN_IF_NULL(dataFieldInfo);
      field_name = dataFieldInfo->nameGet();
      if (field_name == kEs2kMeterProfileIdKPps) {
        uint64 prof_id;
        RETURN_IF_TDI_ERROR(table_data->getValue(field_id, &prof_id));
        cfg[i].meter_prof_id = KbitsToBytesPerSecond(prof_id);
        cfg[i].isPktModMeter = false;
      } else if (field_name == kEs2kMeterCirPps) {
        uint64 cir;
        RETURN_IF_TDI_ERROR(table_data->getValue(field_id, &cir));
        cfg[i].cir = KbitsToBytesPerSecond(cir);
      } else if (field_name == kEs2kMeterCommitedBurstPackets) {
        uint64 cburst;
        RETURN_IF_TDI_ERROR(table_data->getValue(field_id, &cburst));
        cfg[i].cburst = KbitsToBytesPerSecond(cburst);
      } else if (field_name == kEs2kMeterPirPps) {
        uint64 pir;
        RETURN_IF_TDI_ERROR(table_data->getValue(field_id, &pir));
        cfg[i].pir = KbitsToBytesPerSecond(pir);
      } else if (field_name == kEs2kMeterPeakBurstPackets) {
        uint64 pburst;
        RETURN_IF_TDI_ERROR(table_data->getValue(field_id, &pburst));
        cfg[i].pburst = KbitsToBytesPerSecond(pburst);
      } else if (field_name == kEs2kMeterCirKPpsUnit) {
        uint64 cir_unit;
        RETURN_IF_TDI_ERROR(table_data->getValue(field_id, &cir_unit));
        cfg[i].cir_unit = KbitsToBytesPerSecond(cir_unit);
      } else if (field_name == kEs2kMeterCommitedBurstPacketsUnit) {
        uint64 cburst_unit;
        RETURN_IF_TDI_ERROR(table_data->getValue(field_id, &cburst_unit));
        cfg[i].cburst_unit = KbitsToBytesPerSecond(cburst_unit);
      } else if (field_name == kEs2kMeterPirPpsUnit) {
        uint64 pir_unit;
        RETURN_IF_TDI_ERROR(table_data->getValue(field_id, &pir_unit));
        cfg[i].pir_unit = KbitsToBytesPerSecond(pir_unit);
      } else if (field_name == kEs2kMeterPeakBurstPacketsUnit) {
        uint64 pburst_unit;
        RETURN_IF_TDI_ERROR(table_data->getValue(field_id, &pburst_unit));
        cfg[i].pburst_unit = KbitsToBytesPerSecond(pburst_unit);
      } else if (field_name == kEs2kMeterGreenCounterBytes) {
        uint64 greenBytes;
        RETURN_IF_TDI_ERROR(table_data->getValue(field_id, &greenBytes));
        cfg[i].greenBytes = KbitsToBytesPerSecond(greenBytes);
      } else if (field_name == kEs2kMeterGreenCounterPackets) {
        uint64 greenPackets;
        RETURN_IF_TDI_ERROR(table_data->getValue(field_id, &greenPackets));
        cfg[i].greenPackets = KbitsToBytesPerSecond(greenPackets);
      } else if (field_name == kEs2kMeterYellowCounterBytes) {
        uint64 yellowBytes;
        RETURN_IF_TDI_ERROR(table_data->getValue(field_id, &yellowBytes));
        cfg[i].yellowBytes = KbitsToBytesPerSecond(yellowBytes);
      } else if (field_name == kEs2kMeterYellowCounterPackets) {
        uint64 yellowPackets;
        RETURN_IF_TDI_ERROR(table_data->getValue(field_id, &yellowPackets));
        cfg[i].yellowPackets = KbitsToBytesPerSecond(yellowPackets);
      } else if (field_name == kEs2kMeterRedCounterBytes) {
        uint64 redBytes;
        RETURN_IF_TDI_ERROR(table_data->getValue(field_id, &redBytes));
        cfg[i].redBytes = KbitsToBytesPerSecond(redBytes);
      } else if (field_name == kEs2kMeterRedCounterPackets) {
        uint64 redPackets;
        RETURN_IF_TDI_ERROR(table_data->getValue(field_id, &redPackets));
        cfg[i].redPackets = KbitsToBytesPerSecond(redPackets);
      } else {
        MAKE_ERROR(ERR_INVALID_PARAM)
            << "Unknown meter field " << field_name << " in meter with id "
            << table_id << ".";
      }
    }
  }
  CHECK_EQ(meter_indices->size(), keys.size());
  CHECK_EQ(cfg.size(), keys.size());

  return ::util::OkStatus();
}

}  // namespace tdi
}  // namespace hal
}  // namespace stratum
