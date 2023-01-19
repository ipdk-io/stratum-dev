// Copyright 2018 Google LLC
// Copyright 2018-present Open Networking Foundation
// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#ifndef STRATUM_LIB_SECURITY_CREDENTIALS_MANAGER_H_
#define STRATUM_LIB_SECURITY_CREDENTIALS_MANAGER_H_

#include <memory>
#include <string>

#include "grpc/grpc_security_constants.h"
#include "grpcpp/grpcpp.h"
#include "grpcpp/security/server_credentials.h"
#include "grpcpp/security/tls_credentials_options.h"
#include "stratum/glue/gtl/map_util.h"
#include "stratum/glue/status/status.h"
#include "stratum/glue/status/statusor.h"

// Declaring the flags in the header file, so that all other files that
// "#include" the header file already get the flags declared. This is one of
// the recommended methods of using gflags according to documentation
DECLARE_string(ca_cert_file);
DECLARE_string(server_key_file);
DECLARE_string(server_cert_file);
DECLARE_string(client_key_file);
DECLARE_string(client_cert_file);
DECLARE_string(grpc_client_cert_req_type);

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

  // Factory functions for creating the instance of the class.
  // CreateInstance updated to have a flag parameter with a default to false
  // which maintains backward compatibility. If caller uses secure_only=true
  // the CredentialsManager will only attempt to initiate using TLS certificates
  static ::util::StatusOr<std::unique_ptr<CredentialsManager>> CreateInstance(bool secure_only=false);

  // CredentialsManager is neither copyable nor movable.
  CredentialsManager(const CredentialsManager&) = delete;
  CredentialsManager& operator=(const CredentialsManager&) = delete;

 protected:
  // Default constructor. To be called by the Mock class instance as well as
  // CreateInstance().
  CredentialsManager();

 private:
  static constexpr unsigned int kFileRefreshIntervalSeconds = 1;

  std::map<std::string, grpc_ssl_client_certificate_request_type>
    client_cert_verification_map_;

  // Function to initialize the certificate verification map
  ::util::Status InitClientCertVerificationMap();

  // Function to initialize the credentials manager.
  ::util::Status Initialize(bool secure_only);
  std::shared_ptr<::grpc::ServerCredentials> server_credentials_;
  std::shared_ptr<::grpc::ChannelCredentials> client_credentials_;

  friend class CredentialsManagerTest;
};

}  // namespace stratum
#endif  // STRATUM_LIB_SECURITY_CREDENTIALS_MANAGER_H_