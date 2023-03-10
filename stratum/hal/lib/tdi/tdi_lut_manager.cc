// Copyright 2020-present Open Networking Foundation
// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "stratum/hal/lib/tdi/tdi_lut_manager.h"

#include <algorithm>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "absl/strings/match.h"
#include "absl/synchronization/notification.h"
#include "gflags/gflags.h"
#include "p4/config/v1/p4info.pb.h"
#include "stratum/glue/status/status_macros.h"
#include "stratum/hal/lib/tdi/tdi_constants.h"
#include "stratum/hal/lib/tdi/utils.h"
#include "stratum/hal/lib/p4/utils.h"
#include "stratum/lib/utils.h"

DECLARE_uint32(tdi_table_sync_timeout_ms);

namespace stratum {
namespace hal {
namespace tdi {

TdiLutManager::TdiLutManager(OperationMode mode,
                             TdiSdeInterface* tdi_sde_interface, int device)
    : mode_(mode),
      tdi_sde_interface_(ABSL_DIE_IF_NULL(tdi_sde_interface)),
      p4_info_manager_(nullptr),
      device_(device) {}

std::unique_ptr<TdiLutManager> TdiLutManager::CreateInstance(
    OperationMode mode, TdiSdeInterface* tdi_sde_interface, int device) {
  return absl::WrapUnique(new TdiLutManager(mode, tdi_sde_interface, device));
}

::util::Status TdiLutManager::PushForwardingPipelineConfig(
    const TdiDeviceConfig& config) {
  absl::WriterMutexLock l(&lock_);
  CHECK_RETURN_IF_FALSE(config.programs_size() == 1)
      << "Only one P4 program is supported.";
  const auto& program = config.programs(0);
  const auto& p4_info = program.p4info();
  std::unique_ptr<P4InfoManager> p4_info_manager =
      absl::make_unique<P4InfoManager>(p4_info);
  RETURN_IF_ERROR(p4_info_manager->InitializeAndVerify());
  p4_info_manager_ = std::move(p4_info_manager);

  return ::util::OkStatus();
}

::util::Status TdiLutManager::VerifyForwardingPipelineConfig(
    const ::p4::v1::ForwardingPipelineConfig& config) const {
  return ::util::OkStatus();
}

::util::Status TdiLutManager::WriteTableEntry(
    std::shared_ptr<TdiSdeInterface::SessionInterface> session,
    const ::p4::v1::Update::Type type,
    const ::p4::v1::ExternEntry& entry) {
  // absl::ReaderMutexLock l(&lock_)
  CHECK_RETURN_IF_FALSE(type != ::p4::v1::Update::UNSPECIFIED)
      << "Invalid update type " << type;

  //absl::ReaderMutexLock l(&lock_);
  absl::WriterMutexLock l(&lock_);
  if (entry.extern_type_id() == kMvlutExactMatch ||
      entry.extern_type_id() == kMvlutTernaryMatch) {
      ::p4::v1::MvlutEntry mvlut_entry;
      CHECK_RETURN_IF_FALSE(entry.entry().UnpackTo(&mvlut_entry))
          << "Entry " << entry.ShortDebugString()
          << " is not a mvlut entr.";
      return DoWriteVlutEntry(session, mvlut_entry.mvlut_id(), type,
                                        mvlut_entry);
  } else {
      RETURN_ERROR(ERR_INVALID_PARAM)
          << "Unsupported extern type " << entry.extern_type_id() << ".";
  }
}

::util::Status TdiLutManager::BuildTableKey(
    const ::p4::v1::MvlutEntry& table_entry,
    TdiSdeInterface::TableKeyInterface* table_key) {
  CHECK_RETURN_IF_FALSE(table_key);
  bool needs_priority = false;

  for (const auto& expected_match_field : table_entry.match()) {

    auto expected_field_id = expected_match_field.field_id();
    auto it =
        std::find_if(table_entry.match().begin(), table_entry.match().end(),
                     [expected_field_id](const ::p4::v1::FieldMatch& match) {
                       return match.field_id() == expected_field_id;
                     });
    if (it != table_entry.match().end()) {
      auto mk = *it;
      switch (mk.field_match_type_case()) {
        case ::p4::v1::FieldMatch::kExact: {
          CHECK_RETURN_IF_FALSE(!IsDontCareMatch(mk.exact()));
          RETURN_IF_ERROR(
              table_key->SetExact(mk.field_id(), mk.exact().value()));
          break;
        }
        case ::p4::v1::FieldMatch::kTernary: {
          CHECK_RETURN_IF_FALSE(!IsDontCareMatch(mk.ternary()));
          RETURN_IF_ERROR(table_key->SetTernary(
              mk.field_id(), mk.ternary().value(), mk.ternary().mask()));
          break;
        }
        default:
          RETURN_ERROR(ERR_INVALID_PARAM)
              << "Invalid or unsupported match key: " << mk.ShortDebugString();
      }
    }
  }

  return ::util::OkStatus();
}

::util::Status TdiLutManager::DoWriteVlutEntry(
    std::shared_ptr<TdiSdeInterface::SessionInterface> session,
    uint32 bfrt_table_id, const ::p4::v1::Update::Type type,
    const ::p4::v1::MvlutEntry& mvlut_entry) {
  // Lock is already acquired by the caller
  CHECK_RETURN_IF_FALSE(type != ::p4::v1::Update::UNSPECIFIED)
      << "Invalid update type " << type;

  ASSIGN_OR_RETURN(auto table_key,
                   tdi_sde_interface_->CreateTableKey(bfrt_table_id));
  RETURN_IF_ERROR(BuildTableKey(mvlut_entry, table_key.get()));

  ASSIGN_OR_RETURN(
      auto table_data,
      tdi_sde_interface_->CreateTableData(bfrt_table_id, 0));

  if (type == ::p4::v1::Update::INSERT || type == ::p4::v1::Update::MODIFY) {
    for (const auto& param : mvlut_entry.param().params()) {
      RETURN_IF_ERROR(table_data->SetParam(param.param_id(), param.value()));
    }
  }

  switch (type) {
    case ::p4::v1::Update::INSERT: {
      RETURN_IF_ERROR(tdi_sde_interface_->InsertTableEntry(
          device_, session, bfrt_table_id, table_key.get(),
	  table_data.get()));
      break;
    }
    case ::p4::v1::Update::MODIFY: {
      RETURN_IF_ERROR(tdi_sde_interface_->ModifyTableEntry(
          device_, session, bfrt_table_id, table_key.get(),
	  table_data.get()));
      break;
    }
    case ::p4::v1::Update::DELETE: {
      RETURN_IF_ERROR(tdi_sde_interface_->DeleteTableEntry(
          device_, session, bfrt_table_id, table_key.get()));
      break;
    }
    default:
      RETURN_ERROR(ERR_INVALID_PARAM) << "Unsupported update type: " << type;
  }

  return ::util::OkStatus();
}

::util::Status TdiLutManager::ReadTableEntry(
    std::shared_ptr<TdiSdeInterface::SessionInterface> session,
    const ::p4::v1::ExternEntry& entry,
    WriterInterface<::p4::v1::ReadResponse>* writer) {
  absl::ReaderMutexLock l(&lock_);

  if (entry.extern_type_id() == kMvlutExactMatch ||
      entry.extern_type_id() == kMvlutTernaryMatch) {
      ::p4::v1::MvlutEntry mvlut_entry;
      CHECK_RETURN_IF_FALSE(entry.entry().UnpackTo(&mvlut_entry))
          << "Entry " << entry.ShortDebugString()
          << " is not a mvlut entr.";
      ASSIGN_OR_RETURN(uint32 bfrt_table_id,
                   tdi_sde_interface_->GetTdiRtId(mvlut_entry.mvlut_id()));
      return DoReadVlutEntry(session, bfrt_table_id,
                             mvlut_entry, writer);
  } else {
      RETURN_ERROR(ERR_INVALID_PARAM)
          << "Unsupported extern type " << entry.extern_type_id() << ".";
  }
}

::util::Status TdiLutManager::DoReadVlutEntry(
    std::shared_ptr<TdiSdeInterface::SessionInterface> session,
    uint32 table_id,
    const ::p4::v1::MvlutEntry& mvlut_entry,
    WriterInterface<::p4::v1::ReadResponse>* writer) {

    return MAKE_ERROR(ERR_INTERNAL) << "READ VLUT ENTRY IS YET TO BE SUPPORTED";
}

}  // namespace tdi
}  // namespace hal
}  // namespace stratum
