/*
 * (C) Copyright 2024-2025, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TLS_CREDENTIALS_H
#define TLS_CREDENTIALS_H

/**
 * @file tls_credentials.h
 * @brief TLS credentials information.
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
    unsigned char privkey_pem[ASTARTE_CRYPTO_PRIVKEY_BUFFER_SIZE];
    /** @brief Buffer containing the client certificate (PEM format). */
    char crt_pem[CONFIG_ASTARTE_DEVICE_SDK_ADVANCED_CLIENT_CRT_BUFFER_SIZE];
} astarte_tls_credentials_client_crt_t;

#endif // TLS_CREDENTIALS_H
