/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _PAIRING_PRIVATE_H_
#define _PAIRING_PRIVATE_H_

/**
 * @file pairing_private.h
 * @brief Private pairing APIs for new Astarte devices
 */

#include "astarte_device_sdk/astarte.h"
#include "astarte_device_sdk/result.h"

// #include "crypto.h"
// #include "mqtt.h"
// #include "tls_credentials.h"

/** @brief Maximum length for the MQTT broker URL in chars.
 *
 * @details It is the sum of:
 * - Size of the string `mqtts://` (8)
 * - Maximum length for a hostname (DNS) (253)
 * - Size of the string `:` (1)
 * - Maximum length for a port number (5)
 * - Size of the string `/` (1)
 *
 * Total: 268 characters
 */
#define NEW_AST_PAIRING_MAX_BROKER_URL_LEN 268

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Fetch the MQTT broker URL from Astarte pairing API.
 *
 * @param[in] device_id Device ID to use to obtain the MQTT broker URL.
 * @param[in] cred_secr Credential secret to use to obtain the MQTT broker URL.
 * @param[out] out_url Output buffer where to store the fetched ULR.
 * @return ASTARTE_RESULT_OK if publish has been successful, an error code otherwise.
 */
astarte_result_t new_ast_pairing_get_mqtt_broker_url(
    const char *device_id, const char *cred_secr,
    char out_url[NEW_AST_PAIRING_MAX_BROKER_URL_LEN + 1]);

// /**
//  * @brief Fetch the client x509 certificate from Astarte.
//  *
//  * @warning This is often a very memory intensive operation.
//  *
//  * @param[in] timeout_ms Timeout to use for the HTTP operations in ms.
//  * @param[in] device_id Unique identifier to use to register the device instance.
//  * @param[in] cred_secr Credential secret to use as authorization token.
//  * @param[out] client_crt Client private key and certificate for mutual TLS authentication.
//  * @return ASTARTE_RESULT_OK if successful, otherwise an error code.
//  */
// astarte_result_t astarte_pairing_get_client_certificate(const char *device_id,
//     const char *cred_secr, astarte_tls_credentials_client_crt_t *client_crt);

// /**
//  * @brief Fetch the client x509 certificate from Astarte.
//  *
//  * @warning This is often a very memory intensive operation, more than 20kB of stack are required.
//  *
//  * @param[in] timeout_ms Timeout to use for the HTTP operations in ms.
//  * @param[in] device_id Unique identifier to use to register the device instance.
//  * @param[in] cred_secr Credential secret to use as authorization token.
//  * @param[in] crt_pem Input buffer containing the PEM certificate to verify.
//  * @return ASTARTE_RESULT_OK if successful, otherwise an error code.
//  */
// astarte_result_t astarte_pairing_verify_client_certificate(
//     int32_t timeout_ms, const char *device_id, const char *cred_secr, const char *crt_pem);

#ifdef __cplusplus
}
#endif

#endif /* _PAIRING_PRIVATE_H_ */
