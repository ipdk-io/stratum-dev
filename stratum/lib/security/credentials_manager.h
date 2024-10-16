// Copyright 2018 Google LLC
// Copyright 2018-present Open Networking Foundation
// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#ifndef STRATUM_LIB_SECURITY_CREDENTIALS_MANAGER_H_
#define STRATUM_LIB_SECURITY_CREDENTIALS_MANAGER_H_

#include <memory>
#include <string>

#include "grpcpp/grpcpp.h"
#include "grpcpp/security/server_credentials.h"
#include "grpcpp/security/tls_credentials_options.h"
#include "stratum/glue/status/status.h"
#include "stratum/glue/status/statusor.h"

namespace stratum {

// CredentialsManager manages the server credentials for (external facing) gRPC
// servers. It handles starting and shutting down TSI as well as generating the
// server credentials. This class is supposed to be created once for each
// binary.
class CredentialsManager {
 public:
  virtual ~CredentialsManager();

  // Generates server credentials for an external facing gRPC server.
  virtual std::shared_ptr<::grpc::ServerCredentials>
  GenerateExternalFacingServerCredentials() const;

  // Generates client credentials for contacting an external facing gRPC server.
  virtual std::shared_ptr<::grpc::ChannelCredentials>
  GenerateExternalFacingClientCredentials() const;

  // Loads new server credentials.
  virtual ::util::Status LoadNewServerCredentials(const std::string& ca_cert,
                                                  const std::string& cert,
                                                  const std::string& key);

  // Loads new client credentials.
  virtual ::util::Status LoadNewClientCredentials(const std::string& ca_cert,
                                                  const std::string& cert,
                                                  const std::string& key);

  // Factory function for creating the instance of the class.
  static ::util::StatusOr<std::unique_ptr<CredentialsManager>> CreateInstance(
      bool secure_only = false);

  // CredentialsManager is neither copyable nor movable.
  CredentialsManager(const CredentialsManager&) = delete;
  CredentialsManager& operator=(const CredentialsManager&) = delete;

 protected:
  // Default constructor. To be called by the Mock class instance as well as
  // CreateInstance().
  explicit CredentialsManager(bool secure_only = false);

 private:
  static constexpr unsigned int kFileRefreshIntervalSeconds = 1;

  // Function to initialize the credentials manager.
  ::util::Status Initialize();

  ::util::Status InitializeServerCredentials();
  ::util::Status InitializeClientCredentials();

  std::shared_ptr<::grpc::ServerCredentials> server_credentials_;
  std::shared_ptr<::grpc::ChannelCredentials> client_credentials_;

  // Whether connections must be secure.
  bool secure_only_;

  friend class CredentialsManagerTest;
};

}  // namespace stratum
#endif  // STRATUM_LIB_SECURITY_CREDENTIALS_MANAGER_H_
