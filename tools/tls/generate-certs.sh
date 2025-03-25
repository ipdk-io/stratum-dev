#!/bin/bash
# Copyright 2020-present Open Networking Foundation
# Copyright 2023,2025 Intel Corporation
# SPDX-License-Identifier: Apache-2.0
set -e

THIS_DIR=$(dirname "${BASH_SOURCE[0]}")
COMMON_NAME=${COMMON_NAME:-"127.0.0.1"}

echo "Creating certificates for CN=$COMMON_NAME"

# Create directory for certificates
mkdir -p "$THIS_DIR/certs"
rm -rf "$THIS_DIR/certs/"*

# Create temporary server config with environment variables replaced
SERVER_CONF_FILE="$(mktemp)"
cat "$THIS_DIR/grpc-server.conf" | sed "s/\${COMMON_NAME}/$COMMON_NAME/g" > "$SERVER_CONF_FILE"

echo "=== Generating CA certificate ==="
# Generate CA private key and certificate
openssl genrsa -out "$THIS_DIR/certs/ca.key" 4096
openssl req -new -x509 -key "$THIS_DIR/certs/ca.key" -out "$THIS_DIR/certs/ca.crt" \
    -config "$THIS_DIR/ca.conf" -days 365 -sha512
echo "CA certificate generated successfully"

echo "=== Generating server certificate ==="
# Generate server private key and CSR
openssl genrsa -out "$THIS_DIR/certs/stratum.key" 4096
openssl req -new -key "$THIS_DIR/certs/stratum.key" -out "$THIS_DIR/certs/stratum.csr" \
    -config "$SERVER_CONF_FILE" -sha512

# Sign server certificate with CA
openssl x509 -req -in "$THIS_DIR/certs/stratum.csr" -CA "$THIS_DIR/certs/ca.crt" \
    -CAkey "$THIS_DIR/certs/ca.key" -CAcreateserial -out "$THIS_DIR/certs/stratum.crt" \
    -days 30 -sha512 -extfile "$SERVER_CONF_FILE" -extensions server_ext
echo "Server certificate generated successfully"

echo "=== Generating client certificate ==="
# Generate client private key and CSR
openssl genrsa -out "$THIS_DIR/certs/client.key" 4096
openssl req -new -key "$THIS_DIR/certs/client.key" -out "$THIS_DIR/certs/client.csr" \
    -config "$THIS_DIR/grpc-client.conf" -sha512

# Sign client certificate with CA
openssl x509 -req -in "$THIS_DIR/certs/client.csr" -CA "$THIS_DIR/certs/ca.crt" \
    -CAkey "$THIS_DIR/certs/ca.key" -CAcreateserial -out "$THIS_DIR/certs/client.crt" \
    -days 30 -sha512 -extfile "$THIS_DIR/grpc-client.conf" -extensions client_ext
echo "Client certificate generated successfully"

# Install CA certificate to system trust store
echo "=== Installing CA certificate to system trust store ==="
cp "$THIS_DIR/certs/ca.crt" /etc/pki/ca-trust/source/anchors/
update-ca-trust extract
echo "CA certificate installation complete"

# Cleanup
rm "$SERVER_CONF_FILE"

echo "=== Certificate generation completed successfully ==="
echo ""
