// Copyright 2018 Google LLC
// Copyright 2018-present Open Networking Foundation
// Copyright 2023-2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// Extracted from the P4InfoManager class.
// Note that is module is covered by P4InfoManagerTest.

#ifndef STRATUM_HAL_LIB_P4_P4_RESOURCE_MAP
#define STRATUM_HAL_LIB_P4_P4_RESOURCE_MAP

#include <functional>
#include <string>

#include "absl/container/flat_hash_map.h"
#include "google/protobuf/repeated_field.h"
#include "p4/config/v1/p4info.pb.h"
#include "stratum/glue/integral_types.h"
#include "stratum/glue/logging.h"
#include "stratum/glue/status/status.h"
#include "stratum/glue/status/status_macros.h"
#include "stratum/glue/status/statusor.h"
#include "stratum/hal/lib/p4/utils.h"
#include "stratum/lib/macros.h"

namespace stratum {
namespace hal {

// This type defines a callback that verifies the Preamble content.
typedef std::function<::util::Status(const ::p4::config::v1::Preamble& preamble,
                                     const std::string& resource_type)>
    PreambleCallback;

// This class provides a common implementation for mapping P4 IDs and names
// to a specific P4 resource of type T, i.e. name/ID to Table, name/ID to
// Action, etc.
template <class T>
class P4ResourceMap {
 public:
  // The resource_type is a descriptive string for logging and error messages.
  explicit P4ResourceMap(const std::string& resource_type)
      : resource_type_(resource_type) {}
  virtual ~P4ResourceMap() {}

  // Iterates over all the P4 resources of type T and builds the internal
  // maps for ID and name lookup.
  ::util::Status BuildMaps(
      const ::google::protobuf::RepeatedPtrField<T>& p4_resources,
      PreambleCallback preamble_cb) {
    ::util::Status status = ::util::OkStatus();
    for (const auto& resource : p4_resources) {
      auto preamble_status = preamble_cb(resource.preamble(), resource_type_);
      if (preamble_status.ok()) {
        AddIdMapEntry(resource);
        AddNameMapEntry(resource);
      } else {
        APPEND_STATUS_IF_ERROR(status, preamble_status);
      }
    }
    return status;
  }

  // Attempts to find the P4 resource matching the input ID.
  ::util::StatusOr<const T> FindByID(uint32 id) const {
    auto iter = id_to_resource_map_.find(id);
    if (iter == id_to_resource_map_.end()) {
      return MAKE_ERROR(ERR_INVALID_P4_INFO)
             << "P4Info " << resource_type_ << " ID " << PrintP4ObjectID(id)
             << " is not found";
    }
    return *iter->second;
  }

  // Attempts to find the P4 resource matching the input name.
  ::util::StatusOr<const T> FindByName(const std::string& name) const {
    auto iter = name_to_resource_map_.find(name);
    if (iter == name_to_resource_map_.end()) {
      return MAKE_ERROR(ERR_INVALID_P4_INFO)
             << "P4Info " << resource_type_ << " name " << name
             << " is not found";
    }
    return *iter->second;
  }

  // Outputs LOG messages with name to ID translations for all members of
  // this P4ResourceMap.
  void DumpNamesToIDs() const {
    for (auto iter : name_to_resource_map_) {
      LOG(INFO) << resource_type_ << " name " << iter.first << " has ID "
                << PrintP4ObjectID(iter.second->preamble().id());
    }
  }

  // Accessors.
  const std::string& resource_type() const { return resource_type_; }
  uint32 size() const { return id_to_resource_map_.size(); }

 private:
  // The next two methods create lookup map entries for the input resource.
  // They expect that the preamble ID and name have been validated before
  // they are called.
  void AddIdMapEntry(const T& p4_resource) {
    uint32 id_key = p4_resource.preamble().id();
    auto id_result = id_to_resource_map_.emplace(id_key, &p4_resource);
    DCHECK(id_result.second) << "P4Info unexpected duplicate " << resource_type_
                             << " ID " << PrintP4ObjectID(id_key);
  }

  void AddNameMapEntry(const T& p4_resource) {
    const std::string& name_key = p4_resource.preamble().name();
    auto name_result = name_to_resource_map_.emplace(name_key, &p4_resource);
    DCHECK(name_result.second) << "P4Info unexpected duplicate "
                               << resource_type_ << " name " << name_key;
  }

  const std::string resource_type_;  // String used in errors and logs.

  // These maps facilitate lookups from P4 name/ID to resource type T.
  absl::flat_hash_map<uint32, const T*> id_to_resource_map_;
  absl::flat_hash_map<std::string, const T*> name_to_resource_map_;
};

}  // namespace hal
}  // namespace stratum

#endif  // STRATUM_HAL_LIB_P4_P4_RESOURCE_MAP
