// Copyright 2020-present Open Networking Foundation
// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ES2K PktModMeter resource handler (interface).

#ifndef ES2K_PKT_MOD_METER_HANDLER_H_
#define ES2K_PKT_MOD_METER_HANDLER_H_

#include "p4/v1/p4runtime.pb.h"
#include "stratum/glue/integral_types.h"
#include "stratum/glue/status/status.h"
#include "stratum/hal/lib/p4/p4_info_manager.h"
#include "stratum/hal/lib/tdi/es2k/es2k_extern_manager.h"
#include "stratum/hal/lib/tdi/tdi_resource_handler.h"
#include "stratum/hal/lib/tdi/tdi_sde_interface.h"

namespace stratum {
namespace hal {
namespace tdi {

using HandlerParams = Es2kExternManager::HandlerParams;

class Es2kPktModMeterHandler : public TdiResourceHandler {
 public:
  Es2kPktModMeterHandler(const HandlerParams& params,
                         Es2kExternManager* extern_manager);

  virtual ~Es2kPktModMeterHandler();

  util::Status DoReadMeterEntry(
      std::shared_ptr<TdiSdeInterface::SessionInterface> session,
      const ::p4::v1::MeterEntry& meter_entry,
      WriterInterface<::p4::v1::ReadResponse>* writer,
      uint32 table_id) override;

  util::Status DoWriteMeterEntry(
      std::shared_ptr<TdiSdeInterface::SessionInterface> session,
      const ::p4::v1::Update::Type type,
      const ::p4::v1::MeterEntry& meter_entry, uint32 meter_id) override;

 protected:
  const HandlerParams& params_;        // not owned by this class
  Es2kExternManager* extern_manager_;  // not owned by this class
};

}  // namespace tdi
}  // namespace hal
}  // namespace stratum

#endif  // ES2K_PKT_MOD_METER_HANDLER_H_
