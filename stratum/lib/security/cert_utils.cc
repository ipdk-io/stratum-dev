// Copyright 2021-present Open Networking Foundation
// SPDX-License-Identifier: Apache-2.0

#include "stratum/lib/security/cert_utils.h"

#include "openssl/bio.h"
#include "openssl/bn.h"
#include "openssl/buffer.h"
#include "openssl/evp.h"
#include "openssl/pem.h"
#include "openssl/rsa.h"
#include "stratum/lib/macros.h"

namespace stratum {

util::StatusOr<std::string> GetRSAPrivateKeyAsString(EVP_PKEY* pkey) {
  // Returns a reference to the underlying key; no need to free.
  RSA* rsa = EVP_PKEY_get0_RSA(pkey);
  RET_CHECK(rsa) << "Key is not an RSA key.";

  BIO_ptr bio(BIO_new(BIO_s_mem()), BIO_free);
  RET_CHECK(bio.get()) << "Failed to allocate string buffer.";
  RET_CHECK(
      PEM_write_bio_RSAPrivateKey(bio.get(), rsa, NULL, NULL, 0, NULL, NULL))
      << "Failed to write private key to buffer.";

  BUF_MEM* mem = nullptr;
  // Returns a reference to the underlying bio; no need to free.
  BIO_get_mem_ptr(bio.get(), &mem);
  if (mem != nullptr && mem->data && mem->length) {
    return std::string(mem->data, mem->length);
  }
  return MAKE_ERROR(ERR_INVALID_PARAM)
      << "Failed to write private key in PEM format.";
}

util::StatusOr<std::string> GetCertAsString(X509* x509) {
  BIO_ptr bio(BIO_new(BIO_s_mem()), BIO_free);
  RET_CHECK(bio.get()) << "Failed to allocate string buffer.";
  RET_CHECK(PEM_write_bio_X509(bio.get(), x509))
      << "Failed to write certificate to buffer.";

  BUF_MEM* mem = nullptr;
  // Returns a reference to the underlying bio; no need to free.
  BIO_get_mem_ptr(bio.get(), &mem);
  if (mem != nullptr && mem->data && mem->length) {
    return std::string(mem->data, mem->length);
  }
  return MAKE_ERROR(ERR_INVALID_PARAM)
      << "Failed to write certificate in PEM format.";
}

util::Status GenerateRSAKeyPair(EVP_PKEY* evp, int bits) {
  BIGNUM_ptr exp(BN_new(), BN_free);
  BN_set_word(exp.get(), RSA_F4);
  RSA* rsa = RSA_new();
  RET_CHECK(RSA_generate_key_ex(rsa, bits, exp.get(), NULL))
      << "Failed to generate RSA key.";

  // Store this keypair in evp
  // It will be freed when EVP is freed, so only free on failure.
  if (!EVP_PKEY_assign_RSA(evp, rsa)) {
    RSA_free(rsa);
    return MAKE_ERROR(ERR_INVALID_PARAM) << "Failed to assign key.";
  }
  return util::OkStatus();
}

util::Status GenerateUnsignedCert(X509* unsigned_cert,
                                  EVP_PKEY* unsigned_cert_key,
                                  const std::string& common_name, int serial,
                                  int days) {
  RET_CHECK(
      ASN1_INTEGER_set(X509_get_serialNumber(unsigned_cert), serial));
  RET_CHECK(X509_gmtime_adj(X509_get_notBefore(unsigned_cert), 0));
  RET_CHECK(
      X509_gmtime_adj(X509_get_notAfter(unsigned_cert), 60 * 60 * 24 * days));
  RET_CHECK(X509_set_pubkey(unsigned_cert, unsigned_cert_key));

  X509_NAME* name = X509_get_subject_name(unsigned_cert);

  RET_CHECK(X509_NAME_add_entry_by_txt(
      name, "CN", MBSTRING_UTF8,
      reinterpret_cast<const unsigned char*>(common_name.c_str()), -1, -1, 0));
  return util::OkStatus();
}

util::Status SignCert(X509* unsigned_cert, EVP_PKEY* unsigned_cert_key,
                      X509* issuer, EVP_PKEY* issuer_key,
                      const std::string& common_name, int serial, int days) {
  X509_NAME* name = X509_get_subject_name(unsigned_cert);
  X509_NAME* issuer_name;
  if (issuer == NULL || issuer_key == NULL) {
    // then self sign the cert
    issuer_name = name;
    issuer_key = unsigned_cert_key;
  } else {
    issuer_name = X509_get_subject_name(issuer);
  }
  RET_CHECK(X509_set_issuer_name(unsigned_cert, issuer_name));
  RET_CHECK(X509_sign(unsigned_cert, issuer_key, EVP_sha256()));
  return util::OkStatus();
}

util::Status GenerateSignedCert(X509* unsigned_cert,
                                EVP_PKEY* unsigned_cert_key, X509* issuer,
                                EVP_PKEY* issuer_key,
                                const std::string& common_name, int serial,
                                int days) {
  RETURN_IF_ERROR(GenerateUnsignedCert(unsigned_cert, unsigned_cert_key,
                                       common_name, serial, days));
  RETURN_IF_ERROR(SignCert(unsigned_cert, unsigned_cert_key, issuer, issuer_key,
                           common_name, serial, days));
  return util::OkStatus();
}

Certificate::Certificate(const std::string& common_name, int serial_number)
    : key_(EVP_PKEY_ptr(EVP_PKEY_new(), EVP_PKEY_free)),
      certificate_(X509_ptr(X509_new(), X509_free)),
      common_name_(common_name),
      serial_number_(serial_number) {}

util::StatusOr<std::string> Certificate::GetPrivateKey() {
  return GetRSAPrivateKeyAsString(key_.get());
}

util::StatusOr<std::string> Certificate::GetCertificate() {
  return GetCertAsString(certificate_.get());
}

util::Status Certificate::GenerateKeyPair(int bits) {
  return GenerateRSAKeyPair(this->key_.get(), bits);
}

util::Status Certificate::SignCertificate(const Certificate& issuer,
                                          int days_valid) {
  X509* unsigned_cert = this->certificate_.get();
  EVP_PKEY* unsigned_key = this->key_.get();
  X509* issuer_cert;
  EVP_PKEY* issuer_key;
  if (this == &issuer) {  // self sign
    issuer_cert = NULL;
    issuer_key = NULL;
  } else {
    issuer_cert = issuer.certificate_.get();
    issuer_key = issuer.key_.get();
  }
  return GenerateSignedCert(unsigned_cert, unsigned_key, issuer_cert,
                            issuer_key, this->common_name_,
                            this->serial_number_, days_valid);
}

}  // namespace stratum
