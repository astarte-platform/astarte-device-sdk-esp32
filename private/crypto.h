/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CRYPTO_H_
#define _CRYPTO_H_

/**
 * @file crypto.h
 * @brief Functions used to generate a CSR (certificate signing request) and private key.
 *
 * @note This module relies on MbedTLS functionality.
 */

#include "astarte_device_sdk/astarte.h"
#include "astarte_device_sdk/result.h"

/** Buffer size for the TLS private key. */
#define ASTARTE_CRYPTO_PRIVKEY_BUFFER_SIZE 1024

/** Buffer size for the TLS certificate signing request (CSR). */
#define ASTARTE_CRYPTO_CSR_BUFFER_SIZE 1024

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create a private key to be used in a CSR.
 *
 * @param[out] privkey_pem Buffer where to store the computed private key, in the PEM format.
 * @param[in] privkey_pem_size Size of preallocated private key buffer.
 * @return ASTARTE_RESULT_OK if successful, otherwise an error code.
 */
astarte_result_t astarte_crypto_create_key(unsigned char *privkey_pem, size_t privkey_pem_size);

/**
 * @brief Create a CSR (certificate signing request).
 *
 * @param[in] privkey_pem Private key to use for CSR generation, in PEM format.
 * @param[out] csr_pem Buffer where to store the computed CSR, in the PEM format.
 * @param[in] csr_pem_size Size of preallocated CSR buffer.
 * @return ASTARTE_RESULT_OK if successful, otherwise an error code.
 */
astarte_result_t astarte_crypto_create_csr(
    const unsigned char *privkey_pem, unsigned char *csr_pem, size_t csr_pem_size);

/**
 * @brief Get the common name for a PEM certificate.
 *
 * @param[in] cert_pem Certificate in the PEM format.
 * @param[out] cert_cn Resulting common name.
 * @param[in] cert_cn_size Size of preallocated common name buffer
 * @return ASTARTE_RESULT_OK if successful, otherwise an error code.
 */
astarte_result_t astarte_crypto_get_certificate_info(
    const char *cert_pem, char *cert_cn, size_t cert_cn_size);

#ifdef __cplusplus
}
#endif

#endif /* _CRYPTO_H_ */
