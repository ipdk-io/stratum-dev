// Copyright (c) 2022-2023 Intel Corporation.
// SPDX-License-Identifier: Apache-2.0

#include "stratum/hal/lib/tdi/tdi_ipsec_manager.h"

#include <map>
#include <memory>
#include <set>
#include <utility>

#include "absl/base/thread_annotations.h"
#include "absl/memory/memory.h"
#include "absl/synchronization/mutex.h"
#include "absl/time/time.h"
#include "absl/types/optional.h"
#include "stratum/glue/integral_types.h"
#include "stratum/hal/lib/common/constants.h"
#include "stratum/hal/lib/common/utils.h"
#include "stratum/hal/lib/common/writer_interface.h"
#include "stratum/lib/channel/channel.h"
#include "stratum/lib/constants.h"
#include "stratum/lib/macros.h"
#include "stratum/lib/utils.h"
#include "stratum/glue/logging.h"

#define IPSEC_CONFIG_SADB_TABLE_NAME \
  "ipsec-offload.ipsec-offload.sad.sad-entry.ipsec-sa-config"
#define IPSEC_FETCH_SPI_TABLE_NAME \
  "ipsec-offload.ipsec-offload.ipsec-spi"
#define IPSEC_NOTIFICATION_TABLE_NAME \
  "ipsec-offload"
#define IPSEC_STATE_SADB_TABLE_NAME \
  "ipsec-offload.ipsec-offload.sad.sad-entry.ipsec-sa-state"

namespace stratum {
namespace hal {
namespace tdi {

ABSL_CONST_INIT absl::Mutex _ipsec_mgr_lock(absl::kConstInit);

/* #### C function callback #### */
void ipsec_notification_callback(uint32_t dev_id,
                                 uint32_t ipsec_sa_spi,
                                 bool soft_lifetime_expire,
                                 uint8_t ipsec_sa_protocol,
                                 char *ipsec_sa_dest_address,
                                 bool ipv4,
                                 void *cookie) {
  //printf("IPsec callback: dev_id=%d, ipsec_sa_spi=%d, soft_lifetime=%d, \
  //       ipsec_sa_protocol=%d, \
  //       ipsec_sa_dest_address=%s, ipv4=%d, cookie=%p\n",
  //       dev_id, ipsec_sa_spi, soft_lifetime_expire, ipsec_sa_protocol,
  //       ipsec_sa_dest_address, ipv4, cookie);
  auto ipsec_mgr_hdl = reinterpret_cast<IPsecManager *>(cookie);
  ipsec_mgr_hdl->SendSADExpireNotificationEvent(dev_id,
                                                ipsec_sa_spi,
                                                soft_lifetime_expire,
                                                ipsec_sa_protocol,
                                                ipsec_sa_dest_address,
                                                ipv4);
}

IPsecManager::IPsecManager(TdiSdeInterface* tdi_sde_interface,
                           TdiFixedFunctionManager* tdi_fixed_function_manager)
    : tdi_sde_interface_(ABSL_DIE_IF_NULL(tdi_sde_interface)),
      tdi_fixed_function_manager_(ABSL_DIE_IF_NULL(tdi_fixed_function_manager)),
      gnmi_event_writer_(nullptr),
      notif_initialized_(false) {}

IPsecManager::IPsecManager()
    : tdi_sde_interface_(nullptr),
      tdi_fixed_function_manager_(nullptr),
      gnmi_event_writer_(nullptr),
      notif_initialized_(false) {}

IPsecManager::~IPsecManager() = default;

::util::Status IPsecManager::InitializeNotificationCallback() {
  auto status = tdi_fixed_function_manager_->InitNotificationTableWithCallback(
      IPSEC_NOTIFICATION_TABLE_NAME, &ipsec_notification_callback, this);

  if (status != ::util::OkStatus()) {
    LOG(ERROR) << "Failed to register IPsec notification callback";
  }
  return ::util::OkStatus();
}

::util::Status IPsecManager::GetSpiData(uint32 &fetched_spi) {
  // TODO (5abeel): Initilizing the notification callback on FetchSPI because
  // TDI layer is not initialized until 'set-pipe' is completed by user via P4RT
  if (!notif_initialized_) {
      auto status = InitializeNotificationCallback();
      if (status == ::util::OkStatus()) {
        notif_initialized_ = true;
      }
  }
  
  ASSIGN_OR_RETURN(auto session, tdi_sde_interface_->CreateSession());
  auto status = tdi_fixed_function_manager_->FetchSpi(session, 
                                                      IPSEC_FETCH_SPI_TABLE_NAME,
                                                      &fetched_spi);
  if (status != ::util::OkStatus()) {
    return MAKE_ERROR(ERR_AT_LEAST_ONE_OPER_FAILED)
           << "One or more read operations failed.";
  }
  return ::util::OkStatus();
}

::util::Status IPsecManager::WriteConfigSADBEntry(const IPsecSadbConfigOp op_type,
                                                  IPsecSADBConfig &msg) {
  // TODO (5abeel): Initilizing the notification callback on FetchSPI because
  // TDI layer is not initialized until 'set-pipe' is completed by user via P4RT
  if (!notif_initialized_) {
      auto status = InitializeNotificationCallback();
      if (status == ::util::OkStatus()) {
        notif_initialized_ = true;
      }
  }

  ASSIGN_OR_RETURN(auto session, tdi_sde_interface_->CreateSession());
  auto status = tdi_fixed_function_manager_->WriteSadbEntry(session,
                                          IPSEC_CONFIG_SADB_TABLE_NAME,
                                          op_type,
                                          msg);
  if (status != ::util::OkStatus()) {
    return MAKE_ERROR(ERR_AT_LEAST_ONE_OPER_FAILED)
           << "One or more write operations failed. "
           << "offload-id=" << msg.offload_id()
           << ", direction=" << msg.direction()
           << ", op type=" << op_type
           << ", table_name=" << IPSEC_CONFIG_SADB_TABLE_NAME;
  }
  return ::util::OkStatus();
}

void IPsecManager::SendSADExpireNotificationEvent(uint32_t dev_id,
                                                  uint32_t ipsec_sa_spi,
                                                  bool soft_lifetime_expire,
                                                  uint8_t ipsec_sa_protocol,
                                                  char *ipsec_sa_dest_address,
                                                  bool ipv4) {
  absl::ReaderMutexLock l(&gnmi_event_lock_);
  if (!gnmi_event_writer_) return;
  // Allocate and initialize a IPsecNotificationEvent event and pass it to
  // the gNMI publisher using the gNMI event notification channel.
  // The GnmiEventPtr is a smart pointer (shared_ptr<>) and it takes care of
  // the memory allocated to this event object once the event is handled by
  // the GnmiPublisher.
  if (!gnmi_event_writer_->Write(GnmiEventPtr(new IPsecNotificationEvent(
          absl::ToUnixNanos(absl::UnixEpoch()),
          dev_id, ipsec_sa_spi, soft_lifetime_expire,
          ipsec_sa_protocol, ipsec_sa_dest_address, ipv4)))) {
    // Remove WriterInterface if it is no longer operational.
    gnmi_event_writer_.reset();
  }
}

std::unique_ptr<IPsecManager> IPsecManager::CreateInstance(
    TdiSdeInterface* tdi_sde_interface,
    TdiFixedFunctionManager* tdi_fixed_function_manager) {
  return absl::WrapUnique(
      new IPsecManager(tdi_sde_interface, tdi_fixed_function_manager));
}

}  // namespace tdi
}  // namespace hal
}  // namespace stratum
