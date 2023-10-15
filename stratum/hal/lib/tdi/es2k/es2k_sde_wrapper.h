// Copyright 2019-present Barefoot Networks, Inc.
// Copyright 2022-2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#ifndef STRATUM_HAL_LIB_TDI_ES2K_SDE_WRAPPER_H_
#define STRATUM_HAL_LIB_TDI_ES2K_SDE_WRAPPER_H_

#include <string>

#include "absl/synchronization/mutex.h"
#include "stratum/hal/lib/tdi/tdi.pb.h"
#include "stratum/hal/lib/tdi/tdi_sde_wrapper.h"

namespace stratum {
namespace hal {
namespace tdi {

class Es2kSdeWrapper : public TdiSdeWrapper {
 public:
  ::util::StatusOr<bool> IsSoftwareModel(int device) override;
  std::string GetChipType(int device) const override;
  std::string GetSdeVersion() const override;

  ::util::Status InitializeSde(const std::string& sde_install_path,
                               const std::string& sde_config_file,
                               bool run_in_background) override;
  ::util::Status AddDevice(int device,
                           const TdiDeviceConfig& device_config) override;

  ::util::Status InitNotificationTableWithCallback(
      int dev_id, std::shared_ptr<TdiSdeInterface::SessionInterface> session,
      const std::string& table_name, notification_table_callback_t callback,
      void* cookie) const override LOCKS_EXCLUDED(data_lock_);

  // Creates the singleton instance. Expected to be called once to initialize
  // the instance.
  static Es2kSdeWrapper* CreateSingleton() LOCKS_EXCLUDED(init_lock_);

  // Return the singleton instance to be used in the SDE callbacks.
  static Es2kSdeWrapper* GetSingleton() LOCKS_EXCLUDED(init_lock_);

  // Es2kSdeWrapper is neither copyable nor movable.
  Es2kSdeWrapper(const Es2kSdeWrapper&) = delete;
  Es2kSdeWrapper& operator=(const Es2kSdeWrapper&) = delete;
  Es2kSdeWrapper(Es2kSdeWrapper&&) = delete;
  Es2kSdeWrapper& operator=(Es2kSdeWrapper&&) = delete;

 protected:
  // RW mutex lock for protecting the singleton instance initialization and
  // reading it back from other threads. Unlike other singleton classes, we
  // use RW lock as we need the pointer to class to be returned.
  static absl::Mutex init_lock_;

  // The singleton instance.
  static Es2kSdeWrapper* singleton_ GUARDED_BY(init_lock_);

 private:
  // Private constructor; use CreateSingleton and GetSingleton().
  Es2kSdeWrapper();
};

}  // namespace tdi
}  // namespace hal
}  // namespace stratum

#endif  // STRATUM_HAL_LIB_TDI_ES2K_SDE_WRAPPER_H_
