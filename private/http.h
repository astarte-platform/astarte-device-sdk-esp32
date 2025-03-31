/*
 * (C) Copyright 2024-2025, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef HTTP_H
#define HTTP_H

/**
 * @file http.h
 * @brief Low level connectivity functions
 */

#include "astarte_device_sdk/astarte.h"
#include "astarte_device_sdk/result.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HTTP_OUTPUT_BUFFER_LEN 2048

/**
 * @brief Perform an HTTP POST request to Astarte.
 *
 * @param[in] host Host name for the HTTP request.
 * @param[in] path Path for the HTTP request.
 * @param[in] auth_bearer Authentication bearer.
 * @param[in] payload Payload to transmit.
 * @param[out] out Output buffer where to store the response from the server.
 * @return ASTARTE_RESULT_OK if successful, otherwise an error code.
 */
astarte_result_t http_post(const char *host, const char *path, const char *auth_bearer,
    const char *payload, uint8_t out[HTTP_OUTPUT_BUFFER_LEN + 1]);

/**
 * @brief Perform an HTTP GET request to Astarte.
 *
 * @param[in] host Host name for the HTTP request.
 * @param[in] path Path for the HTTP request.
 * @param[in] auth_bearer Authentication bearer.
 * @param[out] out Output buffer where to store the response from the server.
 * @return ASTARTE_RESULT_OK if successful, otherwise an error code.
 */
astarte_result_t http_get(const char *host, const char *path, const char *auth_bearer,
    uint8_t out[HTTP_OUTPUT_BUFFER_LEN + 1]);

#ifdef __cplusplus
}
#endif

#endif // HTTP_H
