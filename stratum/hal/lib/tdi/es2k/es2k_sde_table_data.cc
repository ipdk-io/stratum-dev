// Copyright 2019-present Barefoot Networks, Inc.
// Copyright 2022-2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// Target-agnostic SDE wrapper Table Data methods.

#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "gflags/gflags_declare.h"
#include "stratum/glue/gtl/stl_util.h"
#include "stratum/glue/integral_types.h"
#include "stratum/glue/status/status.h"
#include "stratum/glue/status/statusor.h"
#include "stratum/hal/lib/p4/utils.h"
#include "stratum/hal/lib/tdi/macros.h"
#include "stratum/hal/lib/tdi/tdi_constants.h"
#include "stratum/hal/lib/tdi/tdi_sde_common.h"
#include "stratum/hal/lib/tdi/tdi_sde_flags.h"
#include "stratum/hal/lib/tdi/tdi_sde_helpers.h"
#include "stratum/hal/lib/tdi/tdi_sde_wrapper.h"
#include "stratum/hal/lib/tdi/utils.h"
#include "stratum/lib/macros.h"

namespace stratum {
namespace hal {
namespace tdi {

using namespace stratum::hal::tdi::helpers;

::util::Status TableData::SetPktModMeterConfig(
    bool in_pps,
    uint64 cir_unit, uint64 cburst_unit,
    uint64 pir_unit, uint64 pburst_unit,
    uint64 cir, uint64 cburst,
    uint64 pir, uint64 pburst,
    uint64 greenBytes, uint64 greenPackets,
    uint64 yellowBytes, uint64 yellowPackets,
    uint64 redBytes, uint64 redPackets) {
  if (in_pps) {
    RETURN_IF_ERROR(SetField(table_data_.get(), kMeterCirPps, cir));
    RETURN_IF_ERROR(
        SetField(table_data_.get(), kMeterCommitedBurstPackets, cburst));
    RETURN_IF_ERROR(SetField(table_data_.get(), kMeterPirPps, pir));
    RETURN_IF_ERROR(
        SetField(table_data_.get(), kMeterPeakBurstPackets, pburst));
  } else {
    RETURN_IF_ERROR(
        SetField(table_data_.get(), kEs2kMeterCirKbpsUnit, cir_unit));
    RETURN_IF_ERROR(
        SetField(table_data_.get(), kEs2kMeterCommitedBurstKbitsUnit,
                             cburst_unit));
    RETURN_IF_ERROR(
        SetField(table_data_.get(), kEs2kMeterPirKbpsUnit, pir_unit));
    RETURN_IF_ERROR(SetField(table_data_.get(), kEs2kMeterPeakBurstKbitsUnit,
                             pburst_unit));
    RETURN_IF_ERROR(
        SetField(table_data_.get(), kEs2kMeterCirKbps, BytesPerSecondToKbits(cir)));
    RETURN_IF_ERROR(SetField(table_data_.get(), kEs2kMeterCommitedBurstKbits,
                             BytesPerSecondToKbits(cburst)));
    RETURN_IF_ERROR(
        SetField(table_data_.get(), kEs2kMeterPirKbps, BytesPerSecondToKbits(pir)));
    RETURN_IF_ERROR(SetField(table_data_.get(), kEs2kMeterPeakBurstKbits,
                             BytesPerSecondToKbits(pburst)));
    RETURN_IF_ERROR(SetField(table_data_.get(), kEs2kMeterPeakBurstKbits,
                             BytesPerSecondToKbits(pburst)));
    RETURN_IF_ERROR(SetField(table_data_.get(), kEs2kMeterPeakBurstKbits,
                             BytesPerSecondToKbits(pburst)));
    RETURN_IF_ERROR(SetField(table_data_.get(), kEs2kMeterGreenCounterBytes,
                             BytesPerSecondToKbits(greenBytes)));
    RETURN_IF_ERROR(SetField(table_data_.get(), kEs2kMeterGreenCounterPackets,
                             BytesPerSecondToKbits(greenPackets)));
    RETURN_IF_ERROR(SetField(table_data_.get(), kEs2kMeterYellowCounterBytes,
                             BytesPerSecondToKbits(yellowBytes)));
    RETURN_IF_ERROR(SetField(table_data_.get(), kEs2kMeterYellowCounterPackets,
                             BytesPerSecondToKbits(yellowPackets)));
    RETURN_IF_ERROR(SetField(table_data_.get(), kEs2kMeterRedCounterBytes,
                             BytesPerSecondToKbits(redBytes)));
    RETURN_IF_ERROR(SetField(table_data_.get(), kEs2kMeterRedCounterPackets,
                             BytesPerSecondToKbits(redPackets)));
  }
  return ::util::OkStatus();
}

::util::Status TableData::GetPktModMeterConfig(bool in_pps,
		              uint64* cir_unit, uint64* cburst_unit,
                              uint64* pir_unit, uint64* pburst_unit,
                              uint64* cir, uint64* cburst,
                              uint64* pir, uint64* pburst,
			      uint64* greenBytes, uint64* greenPackets,
                              uint64* yellowBytes, uint64* yellowPackets,
                              uint64* redBytes, uint64* redPackets) {
  if (in_pps) {
    RETURN_IF_ERROR(GetField(*(table_data_.get()), kMeterCirPps, cir));
    RETURN_IF_ERROR(
        GetField(*(table_data_.get()), kMeterCommitedBurstPackets, cburst));
    RETURN_IF_ERROR(GetField(*(table_data_.get()), kMeterPirPps, pir));
    RETURN_IF_ERROR(
        GetField(*(table_data_.get()), kMeterPeakBurstPackets, pburst));
  } else {
    RETURN_IF_ERROR(GetField(*(table_data_.get()), kEs2kMeterCirKbpsUnit, cir_unit));
    RETURN_IF_ERROR(
        GetField(*(table_data_.get()), kEs2kMeterCommitedBurstKbitsUnit, cburst_unit));
    RETURN_IF_ERROR(GetField(*(table_data_.get()), kEs2kMeterPirKbpsUnit, pir_unit));
    RETURN_IF_ERROR(
        GetField(*(table_data_.get()), kEs2kMeterPeakBurstKbitsUnit, pburst_unit));
    RETURN_IF_ERROR(GetField(*(table_data_.get()), kEs2kMeterCirKbps, cir));
    RETURN_IF_ERROR(
        GetField(*(table_data_.get()), kEs2kMeterCommitedBurstKbits, cburst));
    RETURN_IF_ERROR(GetField(*(table_data_.get()), kEs2kMeterPirKbps, pir));
    RETURN_IF_ERROR(
        GetField(*(table_data_.get()), kEs2kMeterPeakBurstKbits, pburst));
    RETURN_IF_ERROR(
        GetField(*(table_data_.get()), kEs2kMeterGreenCounterBytes, greenBytes));
    RETURN_IF_ERROR(
        GetField(*(table_data_.get()), kEs2kMeterGreenCounterPackets, greenPackets));
    RETURN_IF_ERROR(
        GetField(*(table_data_.get()), kEs2kMeterYellowCounterBytes, yellowBytes));
    RETURN_IF_ERROR(
        GetField(*(table_data_.get()), kEs2kMeterYellowCounterPackets, yellowPackets));
    RETURN_IF_ERROR(
        GetField(*(table_data_.get()), kEs2kMeterRedCounterBytes, redBytes));
    RETURN_IF_ERROR(
        GetField(*(table_data_.get()), kEs2kMeterRedCounterPackets, redPackets));

    *cir = KbitsToBytesPerSecond(*cir);
    *cburst = KbitsToBytesPerSecond(*cburst);
    *pir = KbitsToBytesPerSecond(*pir);
    *pburst = KbitsToBytesPerSecond(*pburst);
    *cir_unit = KbitsToBytesPerSecond(*cir_unit);
    *cburst_unit = KbitsToBytesPerSecond(*cburst_unit);
    *pir_unit = KbitsToBytesPerSecond(*pir_unit);
    *pburst_unit = KbitsToBytesPerSecond(*pburst_unit);
    *greenBytes = KbitsToBytesPerSecond(*greenBytes);
    *greenPackets = KbitsToBytesPerSecond(*greenPackets);
    *yellowBytes = KbitsToBytesPerSecond(*yellowBytes);
    *yellowPackets = KbitsToBytesPerSecond(*yellowPackets);
    *redBytes = KbitsToBytesPerSecond(*redBytes);
    *redPackets = KbitsToBytesPerSecond(*redPackets);
  }
  return ::util::OkStatus();
}

}  // namespace tdi
}  // namespace hal
}  // namespace stratum
