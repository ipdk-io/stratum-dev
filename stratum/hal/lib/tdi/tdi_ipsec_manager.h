// Copyright (c) 2022-2023 Intel Corporation.
// SPDX-License-Identifier: Apache-2.0

#ifndef STRATUM_HAL_LIB_TDI_IPSEC_MANAGER_H_
#define STRATUM_HAL_LIB_TDI_IPSEC_MANAGER_H_

#include <map>
#include <memory>

#include "absl/base/thread_annotations.h"
#include "absl/memory/memory.h"
#include "absl/synchronization/mutex.h"
#include "absl/time/time.h"
#include "absl/types/optional.h"
#include "stratum/glue/integral_types.h"
#include "stratum/hal/lib/common/gnmi_events.h"
#include "stratum/hal/lib/common/utils.h"
#include "stratum/hal/lib/common/writer_interface.h"
#include "stratum/hal/lib/tdi/tdi_fixed_function_manager.h"
#include "stratum/hal/lib/tdi/tdi_sde_interface.h"
#include "stratum/lib/channel/channel.h"

#ifdef __cplusplus
extern "C" {
#endif

void ipsec_notification_callback(uint32_t dev_id,
                                 uint32_t ipsec_sa_spi,
                                 bool soft_lifetime_expire,
                                 uint8_t ipsec_sa_protocol,
                                 char *ipsec_sa_dest_address,
                                 bool ipv4,
                                 void *cookie);

#ifdef __cplusplus
}  // extern "C"
#endif

namespace stratum {
namespace hal {
namespace tdi {

// Lock which protects IPsecMgr state across the entire switch.
extern absl::Mutex _ipsec_mgr_lock;

class IPsecManager {
 public:
  virtual ~IPsecManager();

  // Factory function for creating the instance of the class.
  static std::unique_ptr<IPsecManager> CreateInstance(
      TdiSdeInterface* tdi_sde_interface,
      TdiFixedFunctionManager* tdi_fixed_function_manager);

  virtual ::util::Status InitializeNotificationCallback();

  virtual ::util::Status GetSpiData(uint32 &fetched_spi)
      SHARED_LOCKS_REQUIRED(_ipsec_mgr_lock);

  virtual ::util::Status WriteConfigSADEntry(const IPsecSadbOp op_type,
                                             const IPsecSADConfig &msg)
      SHARED_LOCKS_REQUIRED(_ipsec_mgr_lock);

  // IPsecManager is neither copyable nor movable.
  IPsecManager(const IPsecManager&) = delete;
  IPsecManager& operator=(const IPsecManager&) = delete;
  IPsecManager(IPsecManager&&) = delete;
  IPsecManager& operator=(IPsecManager&&) = delete;

  virtual ::util::Status RegisterEventNotifyWriter(
      const std::shared_ptr<WriterInterface<GnmiEventPtr>>& writer) {
          absl::WriterMutexLock l(&gnmi_event_lock_);
          gnmi_event_writer_ = writer;
          return ::util::OkStatus();
  }

  virtual ::util::Status UnregisterEventNotifyWriter() {
    absl::WriterMutexLock l(&gnmi_event_lock_);
    gnmi_event_writer_ = nullptr;
    return ::util::OkStatus();
  }

  void SendSADExpireNotificationEvent(uint32_t dev_id,
                                      uint32_t ipsec_sa_spi,
                                      bool soft_lifetime_expire,
                                      uint8_t ipsec_sa_protocol,
                                      char *ipsec_sa_dest_address,
                                      bool ipv4);

 protected:
  // Default constructor. To be called by the Mock class instance only.
  IPsecManager();

 private:
  // ReaderArgs encapsulates the arguments for a Channel reader thread.
  template <typename T>
  struct ReaderArgs {
    IPsecManager* manager;
    std::unique_ptr<ChannelReader<T>> reader;
  };

  // Private constructor. Use CreateInstance() to create an instance of this
  // class.
  IPsecManager(TdiSdeInterface* tdi_sde_interface, TdiFixedFunctionManager* tdi_fixed_function_manager);

  // WriterInterface<GnmiEventPtr> object for sending event notifications.
  mutable absl::Mutex gnmi_event_lock_;
  std::shared_ptr<WriterInterface<GnmiEventPtr>> gnmi_event_writer_
      GUARDED_BY(gnmi_event_lock_);

  // Pointer to TdiSdeInterface implementation and FixedFunctionManager
  TdiSdeInterface* tdi_sde_interface_;  // not owned by this class.
  TdiFixedFunctionManager* tdi_fixed_function_manager_;  // not owned by this class.
  bool notif_initialized_;

  friend class IPsecManagerTest;
};

}  // namespace tdi
}  // namespace hal
}  // namespace stratum

#endif  // STRATUM_HAL_LIB_TDI_IPSEC_MANAGER_H_
