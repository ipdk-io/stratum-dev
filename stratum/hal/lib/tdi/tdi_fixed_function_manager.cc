// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "stratum/hal/lib/tdi/tdi_fixed_function_manager.h"

#include <string>

#include "absl/strings/match.h"
#include "absl/synchronization/notification.h"
#include "stratum/hal/lib/tdi/tdi_constants.h"
#include "stratum/hal/lib/tdi/utils.h"
#include "stratum/lib/utils.h"

namespace stratum {
namespace hal {
namespace tdi {

TdiFixedFunctionManager::TdiFixedFunctionManager (OperationMode mode,
                                   TdiSdeInterface* tdi_sde_interface, int device)
    : mode_(mode),
      tdi_sde_interface_(ABSL_DIE_IF_NULL(tdi_sde_interface)),
      device_(device) {}

std::unique_ptr<TdiFixedFunctionManager> TdiFixedFunctionManager::CreateInstance(
    OperationMode mode, TdiSdeInterface* tdi_sde_interface, int device) {
  return absl::WrapUnique(new TdiFixedFunctionManager(mode, tdi_sde_interface, device));
}

::util::Status TdiFixedFunctionManager::InitNotificationTableWithCallback(
    std::string table_name,
    void (*ipsec_notif_cb)(uint32_t, uint32_t, bool, uint8_t, char*, bool, void*),
    void *cookie) {

  absl::ReaderMutexLock l(&lock_);

  ASSIGN_OR_RETURN(auto session, tdi_sde_interface_->CreateSession());

  return tdi_sde_interface_->InitNotificationTableWithCallback(device_, session,
                                                               table_name, ipsec_notif_cb,
                                                               cookie);
}

::util::Status TdiFixedFunctionManager::WriteSadbEntry(
  std::shared_ptr<TdiSdeInterface::SessionInterface> session,
  std::string table_name, enum IPsecSadOp op_type,
  IPsecSADConfig *sadb_config) {

  absl::ReaderMutexLock l(&lock_);

  ASSIGN_OR_RETURN(uint32 table_id, tdi_sde_interface_->GetTableId(table_name));
  ASSIGN_OR_RETURN(auto table_key,
                   tdi_sde_interface_->CreateTableKey(table_id));
  RETURN_IF_ERROR(BuildSadbTableKey(table_key.get(), sadb_config));
  ASSIGN_OR_RETURN(auto table_data,
                  tdi_sde_interface_->CreateTableData(table_id, 0));

  if (op_type == IPSEC_SADB_CONFIG_OP_ADD_ENTRY ||
      op_type == IPSEC_SADB_CONFIG_OP_MOD_ENTRY) {
      RETURN_IF_ERROR(BuildSadbTableData(table_data.get(), sadb_config));
  }

  switch (op_type) {
    case IPSEC_SADB_CONFIG_OP_ADD_ENTRY:
      RETURN_IF_ERROR(tdi_sde_interface_->InsertTableEntry(
          device_, session, table_id, table_key.get(), table_data.get()));
      break;
    case IPSEC_SADB_CONFIG_OP_DEL_ENTRY:
      RETURN_IF_ERROR(tdi_sde_interface_->DeleteTableEntry(
          device_, session, table_id, table_key.get()));
      break;
    default:
      RETURN_ERROR(ERR_INTERNAL)
          << "Unsupported update type: " << op_type << " for IPSEC SADB table.";
  }

  return ::util::OkStatus();
}

::util::Status TdiFixedFunctionManager::FetchSpi(
  std::shared_ptr<TdiSdeInterface::SessionInterface> session,
  std::string table_name, uint32 *spi) {

  absl::ReaderMutexLock l(&lock_);

  uint64 spi_val;
  ASSIGN_OR_RETURN(uint32 table_id, tdi_sde_interface_->GetTableId(table_name));
  ASSIGN_OR_RETURN(auto table_data, tdi_sde_interface_->CreateTableData(table_id, 0));

  RETURN_IF_ERROR(tdi_sde_interface_->GetDefaultTableEntry(
            device_, session, table_id, table_data.get()));

  RETURN_IF_ERROR(table_data->GetParam(kIpsecFetchSpi, &spi_val));
  *spi = (uint32) spi_val;

  return ::util::OkStatus();

}

::util::Status TdiFixedFunctionManager::BuildSadbTableKey(
  TdiSdeInterface::TableKeyInterface* table_key,
  IPsecSADConfig *sadb_config) {
  CHECK_RETURN_IF_FALSE(table_key);
  RETURN_IF_ERROR(table_key->SetExact(
      kIpsecSadbOffloadId, sadb_config->offload_id()));
  RETURN_IF_ERROR(table_key->SetExact(
      kIpsecSadbDir, ((uint8)sadb_config->direction())));
  return ::util::OkStatus();
}

::util::Status TdiFixedFunctionManager::BuildSadbTableData(
  TdiSdeInterface::TableDataInterface* table_data,
  IPsecSADConfig *sadb_config) {
  CHECK_RETURN_IF_FALSE(table_data);

  //Set the req_id Value
  RETURN_IF_ERROR(table_data->SetParam(kIpsecSadbReqId, sadb_config->req_id()));

  //Set the Spi Value
  RETURN_IF_ERROR(table_data->SetParam(kIpsecSadbSpi, sadb_config->spi()));

  //Set the ext_seq_num Value
  RETURN_IF_ERROR(table_data->SetParam(
      kIpsecSadbSeqNum, ((uint8)sadb_config->ext_seq_num())));

  //Set the anti_replay_window_size Value
  RETURN_IF_ERROR(table_data->SetParam(
      kIpsecSadbReplayWindow, sadb_config->anti_replay_window_size()));

  //Set the Proto-Param Value
  RETURN_IF_ERROR(table_data->SetParam(
      kIpsecSadbProtoParams, ((uint8)sadb_config->protocol_parameters())));

  //Set the Ipsec Mode Value
  RETURN_IF_ERROR(table_data->SetParam(
      kIpsecSadbMode, ((uint8)sadb_config->mode())));

  if(sadb_config->has_esp_payload()) {
      //Set the Encyption Algorithm Value
      RETURN_IF_ERROR(table_data->SetParam(
          kIpsecSadbEspAlgo,
          ((uint16)sadb_config->esp_payload().encryption().encryption_algorithm())));

      //Set the Encyption Key Value
      RETURN_IF_ERROR(table_data->SetParam(
          kIpsecSadbEspKey,
          sadb_config->esp_payload().encryption().key()));

      //Set the Encyption Keylen Value
      RETURN_IF_ERROR(table_data->SetParam(
        kIpsecSadbEspKeylen,
        ((uint8)sadb_config->esp_payload().encryption().key_len())));
  }

  if(sadb_config->has_sa_lifetime_hard()) {
      //Set the Lifetime Hard Value
      RETURN_IF_ERROR(table_data->SetParam(
          kIpsecSaLtHard, sadb_config->sa_lifetime_hard().bytes()));
   }

  if(sadb_config->has_sa_lifetime_soft()) {
      //Set the Lifetime Soft Value
      RETURN_IF_ERROR(table_data->SetParam(
          kIpsecSaLtSoft, sadb_config->sa_lifetime_soft().bytes()));
  }

  return ::util::OkStatus();
}

}  // namespace tdi
}  // namespace hal
}  // namespace stratum
