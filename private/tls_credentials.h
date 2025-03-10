/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TLS_CREDENTIALS_H
#define TLS_CREDENTIALS_H

/**
 * @file tls_credentials.h
 * @brief Soft wrapper for Zephyr's TLS authentication API.
 */

#include "astarte_device_sdk/astarte.h"
#include "astarte_device_sdk/result.h"

#include "crypto.h"

/** @brief Generic structure used to store a TLS client certificate and associated private key.
 *
 * @details This structure should be used with mutual TLS authentication and will contain the
 * private key and client certificate that can be used to act as a TLS client.
 */
typedef struct
{
    /** @brief Buffer containing the private key bound to the client certificate (PEM format). */
    char privkey_pem[ASTARTE_CRYPTO_PRIVKEY_BUFFER_SIZE];
    /** @brief Buffer containing the client certificate (PEM format). */
    char crt_pem[CONFIG_ASTARTE_DEVICE_SDK_ADVANCED_CLIENT_CRT_BUFFER_SIZE];
} astarte_tls_credentials_client_crt_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Add client TLS credentials for mutual authentication.
 *
 * @param[in] client_crt Private key and client certificate to add.
 * @return ASTARTE_RESULT_OK if successful, otherwise an error code.
 */
astarte_result_t astarte_tls_credential_add(astarte_tls_credentials_client_crt_t *client_crt);

/**
 * @brief Remove client TLS credentials for mutual authentication.
 *
 * @return ASTARTE_RESULT_OK if successful, otherwise an error code.
 */
astarte_result_t astarte_tls_credential_delete(void);

#ifdef __cplusplus
}
#endif

#endif /* TLS_CREDENTIALS_H */
