/*
 * (C) Copyright 2024, SECO Mind Srl
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

#define ASTARTE_HTTP_OUTPUT_BUFFER_LEN 2048

/**
 * @brief Perform an HTTP POST request to Astarte.
 *
 * @param[in] timeout_ms Timeout to use for the HTTP operations in ms.
 * @param[in] url Partial URL to use for the POST request. Hostname and port are taken from the
 * configuration.
 * @param[in] header_fields NULL terminated list of headers for the request.
 * @param[in] payload Payload to transmit.
 * @param[out] resp_buf Output buffer where to store the response from the server.
 * @param[in] resp_buf_size Size of the response output buffer.
 * @return ASTARTE_RESULT_OK if successful, otherwise an error code.
 */
astarte_result_t astarte_http_post(const char *host, const char *path, const char *auth_bearer,
    const char *payload, uint8_t out[ASTARTE_HTTP_OUTPUT_BUFFER_LEN + 1]);

/**
 * @brief Perform an HTTP GET request to Astarte.
 *
 * @param[in] timeout_ms Timeout to use for the HTTP operations in ms.
 * @param[in] url Partial URL to use for the POST request. Hostname and port are taken from the
 * configuration.
 * @param[in] header_fields NULL terminated list of headers for the request.
 * @param[out] resp_buf Output buffer where to store the response from the server.
 * @param[in] resp_buf_size Size of the response output buffer.
 * @return ASTARTE_RESULT_OK if successful, otherwise an error code.
 */
astarte_result_t astarte_http_get(const char *host, const char *path, const char *auth_bearer,
    uint8_t out[ASTARTE_HTTP_OUTPUT_BUFFER_LEN + 1]);

#ifdef __cplusplus
}
#endif

#endif /* HTTP_H */
