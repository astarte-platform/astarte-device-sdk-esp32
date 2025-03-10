/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "astarte_device_sdk/pairing.h"
#include "pairing_private.h"

#include <stdio.h>
#include <stdlib.h>

#include <zephyr/data/json.h>
#include <zephyr/kernel.h>

#include "crypto.h"
#include "http.h"
#include "log.h"

ASTARTE_LOG_MODULE_REGISTER(astarte_pairing, CONFIG_ASTARTE_DEVICE_SDK_PAIRING_LOG_LEVEL);

/************************************************
 *       Checks over configuration values       *
 ***********************************************/

BUILD_ASSERT(sizeof(CONFIG_ASTARTE_DEVICE_SDK_REALM_NAME) != 1, "Missing realm name");

// The JSON parser module for Zephyr does not support the null keyword
// We'll replace the null keyword with a substitute string before parsing.
#define JSON_NULL "null"
#define JSON_NULL_REPLACEMENT "\"..\""
BUILD_ASSERT(sizeof(JSON_NULL) == sizeof(JSON_NULL_REPLACEMENT),
    "JSON null replacement string has incorrect size.");

/************************************************
 *        Defines, constants and typedef        *
 ***********************************************/

// For more information on the Astarte pairing API, such as possible responses, see the Astarte
// documentation.

// Payload will be a json like: {"data":{"hw_id":"<DEVICE_ID>"}}
// The device ID will be no longer than 128 bits encoded in base64 resulting in 22 useful chars.
#define REGISTER_DEVICE_PAYLOAD_MAX_SIZE 50
// Response will be a json like: {"data":{"credentials_secret":"<CRED_SECR>"}}
// The credential secret will be 44 chars long.
#define REGISTER_DEVICE_RESPONSE_MAX_SIZE (50 + ASTARTE_PAIRING_CRED_SECR_LEN)

// Correct response will be a json like: {"data":{"protocols":{"astarte_mqtt_v1":"<BROKER_URL>"}}}
#define GET_BROKER_INFO_RESPONSE_MAX_SIZE (50 + ASTARTE_PAIRING_MAX_BROKER_URL_LEN)

// Payload will be a json like: {"data":{"csr":"<CSR>"}}
#define GET_CLIENT_CRT_PAYLOAD_MAX_SIZE (25 + ASTARTE_CRYPTO_CSR_BUFFER_SIZE)
// Correct response will be a json like: {"data":{"client_crt":"<CLIENT_CRT>"}}
// The maximum size of the client certificate may vary depending on the server configuration.
#define GET_CLIENT_CRT_RESPONSE_MAX_SIZE                                                           \
    (50 + CONFIG_ASTARTE_DEVICE_SDK_ADVANCED_CLIENT_CRT_BUFFER_SIZE)

// Correct payload will be a json like: {"data":{"client_crt":"<CLIENT_CRT>"}}
// The maximum size of the client certificate may vary depending on the server configuration.
#define VERIFY_CLIENT_CRT_PAYLOAD_MAX_SIZE                                                         \
    (50 + CONFIG_ASTARTE_DEVICE_SDK_ADVANCED_CLIENT_CRT_BUFFER_SIZE)
// Correct response will be a json like:
// {"data":{"timestamp":"2024-02-27 14:03:47.229Z","until":"2024-02-27 14:08:00.000Z","valid":true}}
// {"data":{"cause":"EXPIRED","details":null,"timestamp":"2024-02-27 14:15:56.480Z","valid":false}}
#define VERIFY_CLIENT_CRT_RESPONSE_MAX_SIZE 128

// Standard authorization header start string.
#define AUTH_HEADER_BEARER_STR_START "Authorization: Bearer "
// Standard authorization header end string.
#define AUTH_HEADER_BEARER_STR_END "\r\n"
// Authorization header fixed size when used with a credential secret.
// Composed by: "Authorization: Bearer " (23) + <CRED_SECRET> (44) + "\r\n" (3) + null term (1)
#define AUTH_HEADER_CRED_SECRET_SIZE                                                               \
    (sizeof(AUTH_HEADER_BEARER_STR_START) + ASTARTE_PAIRING_CRED_SECR_LEN                          \
        + sizeof(AUTH_HEADER_BEARER_STR_END) - 1)

/** @brief Generic URL prefix to be used for all https calls to the pairing APIs. */
#define PAIRING_URL_PREFIX "/pairing/v1/" CONFIG_ASTARTE_DEVICE_SDK_REALM_NAME
/** @brief URL for https calls to the device registration utility of the pairing APIs. */
#define PAIRING_REGISTRATION_URL PAIRING_URL_PREFIX "/agent/devices"
/** @brief Generic URL prefix for https calls to the device mgmt utility of the pairing APIs. */
#define PAIRING_DEVICE_MGMT_URL_PREFIX PAIRING_URL_PREFIX "/devices/"
/** @brief URL suffix for the https call to the device certificate generation utility of the
 * pairing APIs. */
#define PAIRING_DEVICE_CERT_URL_SUFFIX "/protocols/astarte_mqtt_v1/credentials"
/** @brief URL suffix for the https call to the device certificate verification utility of the
 * pairing APIs. */
#define PAIRING_DEVICE_CERT_CHECK_URL_SUFFIX PAIRING_DEVICE_CERT_URL_SUFFIX "/verify"

/** @brief Size in chars of the #PAIRING_DEVICE_MGMT_URL_PREFIX string. */
#define PAIRING_DEVICE_MGMT_URL_PREFIX_LEN (sizeof(PAIRING_DEVICE_MGMT_URL_PREFIX) - 1)
/** @brief Size in chars of the URL for a 'get broker info' HTTPs request to the pairing APIs. */
#define PAIRING_DEVICE_GET_BROKER_INFO_URL_LEN                                                     \
    (PAIRING_DEVICE_MGMT_URL_PREFIX_LEN + ASTARTE_DEVICE_ID_LEN)
/** @brief Size in chars of the #PAIRING_DEVICE_CERT_URL_SUFFIX string. */
#define PAIRING_DEVICE_CERT_URL_SUFFIX_LEN (sizeof(PAIRING_DEVICE_CERT_URL_SUFFIX) - 1)
/** @brief Size in chars of the URL for a 'get device cert' HTTPs request to the pairing APIs. */
#define PAIRING_DEVICE_GET_DEVICE_CERT_URL_LEN                                                     \
    (PAIRING_DEVICE_MGMT_URL_PREFIX_LEN + ASTARTE_DEVICE_ID_LEN                                    \
        + PAIRING_DEVICE_CERT_URL_SUFFIX_LEN)
/** @brief Size in chars of the #PAIRING_DEVICE_CERT_CHECK_URL_SUFFIX string. */
#define PAIRING_DEVICE_CERT_CHECK_URL_SUFFIX_LEN (sizeof(PAIRING_DEVICE_CERT_CHECK_URL_SUFFIX) - 1)
/** @brief Size in chars of the URL for a 'verify device cert' HTTPs request to the pairing APIs. */
#define PAIRING_DEVICE_CERT_CHECK_URL_LEN                                                          \
    (PAIRING_DEVICE_MGMT_URL_PREFIX_LEN + ASTARTE_DEVICE_ID_LEN                                    \
        + PAIRING_DEVICE_CERT_CHECK_URL_SUFFIX_LEN)

/************************************************
 *         Static functions declaration         *
 ***********************************************/

/**
 * @brief Fetch the MQTT broker URL from Astarte.
 *
 * @param[in] timeout_ms Timeout to use for the HTTP operations in ms.
 * @param[in] device_id Unique identifier to use to register the device instance.
 * @param[in] cred_secr Credential secret to use as authorization token.
 * @param[out] out_url Output buffer where to store the fetched ULR.
 * @param[in] out_url_size Size of the output buffer for the URL.
 * @return ASTARTE_RESULT_OK if successful, otherwise an error code.
 */
static astarte_result_t get_broker_info(int32_t timeout_ms, const char *device_id,
    const char *cred_secr, char *out_url, size_t out_url_size);

/**
 * @brief Encode the payload for the device registration HTTP request.
 *
 * @param[in] device_id Device ID to be encoded in the payload.
 * @param[out] out_buff Buffer where to store the encoded payload.
 * @param[in] out_buff_size Size of the output buffer.
 * @return ASTARTE_RESULT_OK if successful, otherwise an error code.
 */
static astarte_result_t encode_register_device_payload(
    const char *device_id, char *out_buff, size_t out_buff_size);

/**
 * @brief Parse the response from the device registration HTTP request.
 *
 * @param[in] resp_buf Response to parse.
 * @param[out] out_cred_secr Returned credential secret.
 * @param[in] out_cred_secr_size Output credential secret buffer size
 * @return ASTARTE_RESULT_OK if successful, otherwise an error code.
 */
static astarte_result_t parse_register_device_response(
    char *resp_buf, char *out_cred_secr, size_t out_cred_secr_size);

/**
 * @brief Parse the response from the get broker url HTTP request.
 *
 * @param[in] resp_buf Response to parse.
 * @param[out] out_url Returned MQTT broker URL.
 * @param[in] out_url_size Output MQTT broker URL buffer size
 * @return ASTARTE_RESULT_OK if successful, otherwise an error code.
 */
static astarte_result_t parse_get_borker_url_response(
    char *resp_buf, char *out_url, size_t out_url_size);

/**
 * @brief Encode the payload for the get client certificate HTTP request.
 *
 * @param[in] csr Certificate signing request to be encoded in the payload.
 * @param[out] out_buff Buffer where to store the encoded payload.
 * @param[in] out_buff_size Size of the output buffer.
 * @return ASTARTE_RESULT_OK if successful, otherwise an error code.
 */
static astarte_result_t encode_get_client_certificate_payload(
    const char *csr, char *out_buff, size_t out_buff_size);

/**
 * @brief Parse the response from the get client certificate HTTP request.
 *
 * @param[in] resp_buf Response to parse.
 * @param[out] out_deb Returned deb certificate.
 * @param[in] out_deb_size Size of the output buffer.
 * @return ASTARTE_RESULT_OK if successful, otherwise an error code.
 */
static astarte_result_t parse_get_client_certificate_response(
    char *resp_buf, char *out_deb, size_t out_deb_size);

/**
 * @brief Encode the payload for the verify client certificate HTTP request.
 *
 * @param[in] crt_pem Certificate to be encoded in the payload.
 * @param[out] out_buff Buffer where to store the encoded payload.
 * @param[in] out_buff_size Size of the output buffer.
 * @return ASTARTE_RESULT_OK if successful, otherwise an error code.
 */
static astarte_result_t encode_verify_client_certificate_payload(
    const char *crt_pem, char *out_buff, size_t out_buff_size);

/**
 * @brief Parse the response from the verify client certificate HTTP request.
 *
 * @param[in] resp_buf Response to parse.
 * @return ASTARTE_RESULT_OK if successful, otherwise an error code.
 */
static astarte_result_t parse_verify_client_certificate_response(char *resp_buf);

/************************************************
 *         Global functions definitions         *
 ***********************************************/

astarte_result_t astarte_pairing_register_device(
    int32_t timeout_ms, const char *device_id, char *out_cred_secr, size_t out_cred_secr_size)
{
    astarte_result_t ares = ASTARTE_RESULT_OK;
    // Step 1: check the configuration and input parameters
    if (sizeof(CONFIG_ASTARTE_DEVICE_SDK_PAIRING_JWT) <= 1) {
        ASTARTE_LOG_ERR("Registration of a device requires a valid pairing JWT");
        return ASTARTE_RESULT_INVALID_CONFIGURATION;
    }
    if (strlen(device_id) != ASTARTE_DEVICE_ID_LEN) {
        ASTARTE_LOG_ERR(
            "Device ID has incorrect length, should be %d chars.", ASTARTE_DEVICE_ID_LEN);
        return ASTARTE_RESULT_INVALID_PARAM;
    }
    if (out_cred_secr_size <= ASTARTE_PAIRING_CRED_SECR_LEN) {
        ASTARTE_LOG_ERR("Insufficient output buffer size for credential secret.");
        return ASTARTE_RESULT_INVALID_PARAM;
    }

    // Step 2: register the device and get the credential secret
    char url[] = PAIRING_REGISTRATION_URL;
    char auth_header[] = { AUTH_HEADER_BEARER_STR_START CONFIG_ASTARTE_DEVICE_SDK_PAIRING_JWT
            AUTH_HEADER_BEARER_STR_END };
    const char *header_fields[] = { auth_header, NULL };
    char payload[REGISTER_DEVICE_PAYLOAD_MAX_SIZE] = { 0 };
    ares = encode_register_device_payload(device_id, payload, REGISTER_DEVICE_PAYLOAD_MAX_SIZE);
    if (ares != ASTARTE_RESULT_OK) {
        return ares;
    }
    char resp_buf[REGISTER_DEVICE_RESPONSE_MAX_SIZE] = { 0 };

    ares = astarte_http_post(timeout_ms, url, header_fields, payload, resp_buf, sizeof(resp_buf));
    if (ares != ASTARTE_RESULT_OK) {
        return ares;
    }

    // Step 3: process the result
    return parse_register_device_response(resp_buf, out_cred_secr, out_cred_secr_size);
}

astarte_result_t astarte_pairing_get_mqtt_broker_hostname_and_port(int32_t http_timeout_ms,
    const char *device_id, const char *cred_secr,
    char broker_hostname[static ASTARTE_MQTT_MAX_BROKER_HOSTNAME_LEN + 1],
    char broker_port[static ASTARTE_MQTT_MAX_BROKER_PORT_LEN + 1])
{
    char broker_url[ASTARTE_PAIRING_MAX_BROKER_URL_LEN + 1] = { 0 };
    astarte_result_t ares
        = get_broker_info(http_timeout_ms, device_id, cred_secr, broker_url, sizeof(broker_url));
    if (ares != ASTARTE_RESULT_OK) {
        ASTARTE_LOG_ERR("Failed in obtaining the MQTT broker URL");
        return ares;
    }
    int strncmp_rc = strncmp(broker_url, "mqtts://", strlen("mqtts://"));
    if (strncmp_rc != 0) {
        ASTARTE_LOG_ERR("MQTT broker URL is malformed");
        return ASTARTE_RESULT_HTTP_REQUEST_ERROR;
    }
    char *saveptr = NULL;
    char *broker_url_token = strtok_r(&broker_url[strlen("mqtts://")], ":", &saveptr);
    if (!broker_url_token) {
        ASTARTE_LOG_ERR("MQTT broker URL is malformed");
        return ASTARTE_RESULT_HTTP_REQUEST_ERROR;
    }
    int ret = snprintf(
        broker_hostname, ASTARTE_MQTT_MAX_BROKER_HOSTNAME_LEN + 1, "%s", broker_url_token);
    if ((ret <= 0) || (ret > ASTARTE_MQTT_MAX_BROKER_HOSTNAME_LEN)) {
        ASTARTE_LOG_ERR("Error encoding broker hostname.");
        return ASTARTE_RESULT_INTERNAL_ERROR;
    }
    broker_url_token = strtok_r(NULL, "/", &saveptr);
    if (!broker_url_token) {
        ASTARTE_LOG_ERR("MQTT broker URL is malformed");
        return ASTARTE_RESULT_HTTP_REQUEST_ERROR;
    }
    ret = snprintf(broker_port, ASTARTE_MQTT_MAX_BROKER_PORT_LEN + 1, "%s", broker_url_token);
    if ((ret <= 0) || (ret > ASTARTE_MQTT_MAX_BROKER_PORT_LEN)) {
        ASTARTE_LOG_ERR("Error encoding broker port.");
        return ASTARTE_RESULT_INTERNAL_ERROR;
    }
    return ASTARTE_RESULT_OK;
}

astarte_result_t astarte_pairing_get_client_certificate(int32_t timeout_ms, const char *device_id,
    const char *cred_secr, astarte_tls_credentials_client_crt_t *client_crt)
{
    astarte_result_t ares = ASTARTE_RESULT_OK;
    // Step 1: check the configuration and input parameters
    if (strlen(device_id) != ASTARTE_DEVICE_ID_LEN) {
        ASTARTE_LOG_ERR(
            "Device ID has incorrect length, should be %d chars.", ASTARTE_DEVICE_ID_LEN);
        return ASTARTE_RESULT_INVALID_PARAM;
    }

    // Step 2: create a private key and a CSR
    ares = astarte_crypto_create_key(client_crt->privkey_pem, ARRAY_SIZE(client_crt->privkey_pem));
    if (ares != ASTARTE_RESULT_OK) {
        ASTARTE_LOG_ERR("Failed in creating a private key.");
        return ares;
    }

    unsigned char csr_buf[ASTARTE_CRYPTO_CSR_BUFFER_SIZE];
    ares = astarte_crypto_create_csr(client_crt->privkey_pem, csr_buf, sizeof(csr_buf));
    if (ares != ASTARTE_RESULT_OK) {
        ASTARTE_LOG_ERR("Failed in creating a CSR.");
        return ares;
    }

    // Step 3: get the client certificate from the server
    char auth_header[AUTH_HEADER_CRED_SECRET_SIZE] = { 0 };
    int snprintf_rc = snprintf(auth_header, AUTH_HEADER_CRED_SECRET_SIZE,
        AUTH_HEADER_BEARER_STR_START "%s" AUTH_HEADER_BEARER_STR_END, cred_secr);
    if (snprintf_rc != AUTH_HEADER_CRED_SECRET_SIZE - 1) {
        ASTARTE_LOG_ERR("Incorrect length/format for credential secret.");
        return ASTARTE_RESULT_INVALID_PARAM;
    }
    const char *header_fields[] = { auth_header, NULL };
    char payload[GET_CLIENT_CRT_PAYLOAD_MAX_SIZE] = { 0 };
    ares = encode_get_client_certificate_payload(csr_buf, payload, GET_CLIENT_CRT_PAYLOAD_MAX_SIZE);
    if (ares != ASTARTE_RESULT_OK) {
        return ares;
    }
    char resp_buf[GET_CLIENT_CRT_RESPONSE_MAX_SIZE] = { 0 };

    char url[PAIRING_DEVICE_GET_DEVICE_CERT_URL_LEN + 1] = { 0 };
    snprintf_rc = snprintf(url, PAIRING_DEVICE_GET_DEVICE_CERT_URL_LEN + 1,
        PAIRING_DEVICE_MGMT_URL_PREFIX "%s" PAIRING_DEVICE_CERT_URL_SUFFIX, device_id);
    if (snprintf_rc != PAIRING_DEVICE_GET_DEVICE_CERT_URL_LEN) {
        ASTARTE_LOG_ERR("Error encoding URL for get client certificate request.");
        return ASTARTE_RESULT_INTERNAL_ERROR;
    }

    ares = astarte_http_post(timeout_ms, url, header_fields, payload, resp_buf, sizeof(resp_buf));
    if (ares != ASTARTE_RESULT_OK) {
        return ares;
    }

    // Step 4: process the result
    ares = parse_get_client_certificate_response(
        resp_buf, client_crt->crt_pem, ARRAY_SIZE(client_crt->crt_pem));
    if (ares != ASTARTE_RESULT_OK) {
        return ares;
    }

    // Step 5: convert the received certificate to a valid PEM certificate
    // Replace "\n" with newlines chars
    char *tmp = NULL;
    while ((tmp = strstr(client_crt->crt_pem, "\\n")) != NULL) {
        tmp[0] = '\n';
        tmp[1] = '\n';
    }

    ASTARTE_LOG_DBG("Received client certificate: %s", client_crt->crt_pem);
    return ASTARTE_RESULT_OK;
}

astarte_result_t astarte_pairing_verify_client_certificate(
    int32_t timeout_ms, const char *device_id, const char *cred_secr, const char *crt_pem)
{
    astarte_result_t ares = ASTARTE_RESULT_OK;
    // Step 1: check the configuration and input parameters
    if (strlen(device_id) != ASTARTE_DEVICE_ID_LEN) {
        ASTARTE_LOG_ERR(
            "Device ID has incorrect length, should be %d chars.", ASTARTE_DEVICE_ID_LEN);
        return ASTARTE_RESULT_INVALID_PARAM;
    }

    // Step 2: register the device and get the credential secret
    char auth_header[AUTH_HEADER_CRED_SECRET_SIZE] = { 0 };
    int snprintf_rc = snprintf(auth_header, AUTH_HEADER_CRED_SECRET_SIZE,
        AUTH_HEADER_BEARER_STR_START "%s" AUTH_HEADER_BEARER_STR_END, cred_secr);
    if (snprintf_rc != AUTH_HEADER_CRED_SECRET_SIZE - 1) {
        ASTARTE_LOG_ERR("Incorrect length/format for credential secret.");
        return ASTARTE_RESULT_INVALID_PARAM;
    }
    const char *header_fields[] = { auth_header, NULL };
    char payload[VERIFY_CLIENT_CRT_PAYLOAD_MAX_SIZE] = { 0 };
    ares = encode_verify_client_certificate_payload(
        crt_pem, payload, VERIFY_CLIENT_CRT_PAYLOAD_MAX_SIZE);
    if (ares != ASTARTE_RESULT_OK) {
        return ares;
    }

    char url[PAIRING_DEVICE_CERT_CHECK_URL_LEN + 1] = { 0 };
    snprintf_rc = snprintf(url, PAIRING_DEVICE_CERT_CHECK_URL_LEN + 1,
        PAIRING_DEVICE_MGMT_URL_PREFIX "%s" PAIRING_DEVICE_CERT_CHECK_URL_SUFFIX, device_id);
    if (snprintf_rc != PAIRING_DEVICE_CERT_CHECK_URL_LEN) {
        ASTARTE_LOG_ERR("Error encoding URL for verify client certificate request.");
        return ASTARTE_RESULT_INTERNAL_ERROR;
    }

    char resp_buf[VERIFY_CLIENT_CRT_RESPONSE_MAX_SIZE] = { 0 };
    ares = astarte_http_post(timeout_ms, url, header_fields, payload, resp_buf, sizeof(resp_buf));
    if (ares != ASTARTE_RESULT_OK) {
        return ares;
    }

    // Step 3: process the result
    return parse_verify_client_certificate_response(resp_buf);
}

/************************************************
 *         Static functions definitions         *
 ***********************************************/

static astarte_result_t get_broker_info(int32_t timeout_ms, const char *device_id,
    const char *cred_secr, char *out_url, size_t out_url_size)
{
    astarte_result_t ares = ASTARTE_RESULT_OK;
    // Step 1: check the input parameters
    if (out_url_size <= ASTARTE_PAIRING_MAX_BROKER_URL_LEN) {
        ASTARTE_LOG_ERR("Insufficient output buffer size for broker URL.");
        return ASTARTE_RESULT_INVALID_PARAM;
    }
    if (strlen(device_id) != ASTARTE_DEVICE_ID_LEN) {
        ASTARTE_LOG_ERR(
            "Device ID has incorrect length, should be %d chars.", ASTARTE_DEVICE_ID_LEN);
        return ASTARTE_RESULT_INVALID_PARAM;
    }

    // Step 2: Get the MQTT broker URL
    char auth_header[AUTH_HEADER_CRED_SECRET_SIZE] = { 0 };
    int snprintf_rc = snprintf(auth_header, AUTH_HEADER_CRED_SECRET_SIZE,
        AUTH_HEADER_BEARER_STR_START "%s" AUTH_HEADER_BEARER_STR_END, cred_secr);
    if (snprintf_rc != AUTH_HEADER_CRED_SECRET_SIZE - 1) {
        ASTARTE_LOG_ERR("Incorrect length/format for credential secret.");
        return ASTARTE_RESULT_INVALID_PARAM;
    }
    const char *header_fields[] = { auth_header, NULL };

    char url[PAIRING_DEVICE_GET_BROKER_INFO_URL_LEN + 1] = { 0 };
    snprintf_rc = snprintf(url, PAIRING_DEVICE_GET_BROKER_INFO_URL_LEN + 1,
        PAIRING_DEVICE_MGMT_URL_PREFIX "%s", device_id);
    if (snprintf_rc != PAIRING_DEVICE_GET_BROKER_INFO_URL_LEN) {
        ASTARTE_LOG_ERR("Error encoding URL for get client certificate request.");
        return ASTARTE_RESULT_INTERNAL_ERROR;
    }

    char resp_buf[GET_BROKER_INFO_RESPONSE_MAX_SIZE] = { 0 };
    ares = astarte_http_get(timeout_ms, url, header_fields, resp_buf, sizeof(resp_buf));
    if (ares != ASTARTE_RESULT_OK) {
        return ares;
    }

    // Step 3: process the result
    return parse_get_borker_url_response(resp_buf, out_url, out_url_size);
}

static astarte_result_t encode_register_device_payload(
    const char *device_id, char *out_buff, size_t out_buff_size)
{
    struct payload_s
    {
        struct data_s
        {
            const char *hw_id;
        } data;
    };

    static const struct json_obj_descr data_descr[] = {
        JSON_OBJ_DESCR_PRIM(struct data_s, hw_id, JSON_TOK_STRING),
    };

    static const struct json_obj_descr payload_descr[] = {
        JSON_OBJ_DESCR_OBJECT(struct payload_s, data, data_descr),
    };

    struct payload_s payload = { .data = { .hw_id = device_id } };

    ssize_t len = json_calc_encoded_len(payload_descr, ARRAY_SIZE(payload_descr), &payload);
    if ((len <= 0) || (len > out_buff_size)) {
        ASTARTE_LOG_ERR("Insufficient buffer, requiring %zd bytes", len);
        return ASTARTE_RESULT_JSON_ERROR;
    }

    int ret = json_obj_encode_buf(
        payload_descr, ARRAY_SIZE(payload_descr), &payload, out_buff, out_buff_size);
    if (ret != 0) {
        ASTARTE_LOG_ERR("Error encoding payload: %d", ret);
        return ASTARTE_RESULT_JSON_ERROR;
    }
    return ASTARTE_RESULT_OK;
}

static astarte_result_t parse_register_device_response(
    char *resp_buf, char *out_cred_secr, size_t out_cred_secr_size)
{
    // Define the structure of the expected JSON file
    struct full_json_s
    {
        struct data_s
        {
            const char *credentials_secret;
        } data;
    };
    // Create the descriptors list containing each element of the struct
    static const struct json_obj_descr data_descr[] = {
        JSON_OBJ_DESCR_PRIM(struct data_s, credentials_secret, JSON_TOK_STRING),
    };
    static const struct json_obj_descr full_json_descr[] = {
        JSON_OBJ_DESCR_OBJECT(struct full_json_s, data, data_descr),
    };
    // This is only for the outside level, nested elements should be checked individually.
    int expected_return_code = (1U << (size_t) ARRAY_SIZE(full_json_descr)) - 1;
    // Parse the json into a structure
    struct full_json_s parsed_json = { .data.credentials_secret = NULL };
    int64_t ret = json_obj_parse(
        resp_buf, strlen(resp_buf) + 1, full_json_descr, ARRAY_SIZE(full_json_descr), &parsed_json);
    if (ret != expected_return_code) {
        ASTARTE_LOG_ERR("JSON Parse Error: %lld", ret);
        return ASTARTE_RESULT_JSON_ERROR;
    }
    if (!parsed_json.data.credentials_secret) {
        ASTARTE_LOG_ERR("Parsed JSON is missing the credentials_secret field.");
        return ASTARTE_RESULT_JSON_ERROR;
    }
    if (strlen(parsed_json.data.credentials_secret) != ASTARTE_PAIRING_CRED_SECR_LEN) {
        ASTARTE_LOG_ERR("Received credential secret of unexpected length: '%s'.",
            parsed_json.data.credentials_secret);
        return ASTARTE_RESULT_JSON_ERROR;
    }

    // Copy the received credential secret in the output buffer
    int snprintf_rc
        = snprintf(out_cred_secr, out_cred_secr_size, "%s", parsed_json.data.credentials_secret);
    if ((snprintf_rc < 0) || (snprintf_rc >= out_cred_secr_size)) {
        ASTARTE_LOG_ERR("Error extracting the credential secret from the parsed json.");
        return ASTARTE_RESULT_JSON_ERROR;
    }
    ASTARTE_LOG_DBG("Received credentials secret: %s", out_cred_secr);
    return ASTARTE_RESULT_OK;
}

static astarte_result_t parse_get_borker_url_response(
    char *resp_buf, char *out_url, size_t out_url_size)
{
    // Define the structure of the expected JSON file
    struct full_json_s
    {
        struct data_s
        {
            struct protocols_s
            {
                struct astarte_mqtt_v1_s
                {
                    const char *broker_url;
                } astarte_mqtt_v1;
            } protocols;
        } data;
    };
    // Create the descriptors list containing each element of the struct
    static const struct json_obj_descr astarte_mqtt_v1_descr[] = {
        JSON_OBJ_DESCR_PRIM(struct astarte_mqtt_v1_s, broker_url, JSON_TOK_STRING),
    };
    static const struct json_obj_descr protocols_descr[] = {
        JSON_OBJ_DESCR_OBJECT(struct protocols_s, astarte_mqtt_v1, astarte_mqtt_v1_descr),
    };
    static const struct json_obj_descr data_descr[] = {
        JSON_OBJ_DESCR_OBJECT(struct data_s, protocols, protocols_descr),
    };
    static const struct json_obj_descr full_json_descr[] = {
        JSON_OBJ_DESCR_OBJECT(struct full_json_s, data, data_descr),
    };
    // This is only for the outside level, nested elements should be checked individually.
    int expected_return_code = (1U << (size_t) ARRAY_SIZE(full_json_descr)) - 1;
    // Parse the json into a structure
    struct full_json_s parsed_json = { .data.protocols.astarte_mqtt_v1.broker_url = NULL };
    int64_t ret = json_obj_parse(
        resp_buf, strlen(resp_buf) + 1, full_json_descr, ARRAY_SIZE(full_json_descr), &parsed_json);
    if (ret != expected_return_code) {
        ASTARTE_LOG_ERR("JSON Parse Error: %lld", ret);
        return ASTARTE_RESULT_JSON_ERROR;
    }
    if (!parsed_json.data.protocols.astarte_mqtt_v1.broker_url) {
        ASTARTE_LOG_ERR("Parsed JSON is missing the broker URL field.");
        return ASTARTE_RESULT_JSON_ERROR;
    }
    if (strlen(parsed_json.data.protocols.astarte_mqtt_v1.broker_url)
        > ASTARTE_PAIRING_MAX_BROKER_URL_LEN) {
        ASTARTE_LOG_ERR("Received a broker URL exceeding the maximum allowed length: '%s'.",
            parsed_json.data.protocols.astarte_mqtt_v1.broker_url);
        return ASTARTE_RESULT_JSON_ERROR;
    }

    // Copy the received credential secret in the output buffer
    int snprintf_rc = snprintf(
        out_url, out_url_size, "%s", parsed_json.data.protocols.astarte_mqtt_v1.broker_url);
    if ((snprintf_rc < 0) || (snprintf_rc >= out_url_size)) {
        ASTARTE_LOG_ERR("Error extracting the broker URL from the parsed json.");
        return ASTARTE_RESULT_JSON_ERROR;
    }
    ASTARTE_LOG_DBG("Received broker URL: %s", out_url);
    return ASTARTE_RESULT_OK;
}

static astarte_result_t encode_get_client_certificate_payload(
    const char *csr, char *out_buff, size_t out_buff_size)
{
    struct payload_s
    {
        struct data_s
        {
            const char *csr;
        } data;
    };

    static const struct json_obj_descr data_descr[] = {
        JSON_OBJ_DESCR_PRIM(struct data_s, csr, JSON_TOK_STRING),
    };

    static const struct json_obj_descr payload_descr[] = {
        JSON_OBJ_DESCR_OBJECT(struct payload_s, data, data_descr),
    };

    struct payload_s payload = { .data = { .csr = csr } };

    ssize_t len = json_calc_encoded_len(payload_descr, ARRAY_SIZE(payload_descr), &payload);
    if ((len <= 0) || (len > out_buff_size)) {
        ASTARTE_LOG_ERR("Insufficient buffer, requiring %zd bytes", len);
        return ASTARTE_RESULT_JSON_ERROR;
    }

    int ret = json_obj_encode_buf(
        payload_descr, ARRAY_SIZE(payload_descr), &payload, out_buff, out_buff_size);
    if (ret != 0) {
        ASTARTE_LOG_ERR("Error encoding payload: %d", ret);
        return ASTARTE_RESULT_JSON_ERROR;
    }
    return ASTARTE_RESULT_OK;
}

static astarte_result_t parse_get_client_certificate_response(
    char *resp_buf, char *out_deb, size_t out_deb_size)
{
    // Define the structure of the expected JSON file
    struct full_json_s
    {
        struct data_s
        {
            const char *client_crt;
        } data;
    };
    // Create the descriptors list containing each element of the struct
    static const struct json_obj_descr data_descr[] = {
        JSON_OBJ_DESCR_PRIM(struct data_s, client_crt, JSON_TOK_STRING),
    };
    static const struct json_obj_descr full_json_descr[] = {
        JSON_OBJ_DESCR_OBJECT(struct full_json_s, data, data_descr),
    };
    // This is only for the outside level, nested elements should be checked individually.
    int expected_return_code = (1U << (size_t) ARRAY_SIZE(full_json_descr)) - 1;
    // Parse the json into a structure
    struct full_json_s parsed_json = { .data.client_crt = NULL };
    int64_t ret = json_obj_parse(
        resp_buf, strlen(resp_buf) + 1, full_json_descr, ARRAY_SIZE(full_json_descr), &parsed_json);
    if (ret != expected_return_code) {
        ASTARTE_LOG_ERR("JSON Parse Error: %lld", ret);
        return ASTARTE_RESULT_JSON_ERROR;
    }
    if (!parsed_json.data.client_crt) {
        ASTARTE_LOG_ERR("Parsed JSON is missing the client_crt field.");
        return ASTARTE_RESULT_JSON_ERROR;
    }
    if (strlen(parsed_json.data.client_crt) == 0) {
        ASTARTE_LOG_ERR("Received empty client certificate");
        return ASTARTE_RESULT_JSON_ERROR;
    }

    // Copy the received client certificate in the output buffer
    if (strlen(parsed_json.data.client_crt)
        >= CONFIG_ASTARTE_DEVICE_SDK_ADVANCED_CLIENT_CRT_BUFFER_SIZE) {
        ASTARTE_LOG_ERR("Received client certificate is too long.");
        return ASTARTE_RESULT_INVALID_PARAM;
    }
    int snprintf_rc = snprintf(out_deb, out_deb_size, "%s", parsed_json.data.client_crt);
    if ((snprintf_rc < 0) || (snprintf_rc >= out_deb_size)) {
        ASTARTE_LOG_ERR("Error extracting the client certificate from the parsed json.");
        return ASTARTE_RESULT_JSON_ERROR;
    }
    return ASTARTE_RESULT_OK;
}

static astarte_result_t encode_verify_client_certificate_payload(
    const char *crt_pem, char *out_buff, size_t out_buff_size)
{
    // Define the structure of the expected JSON file
    struct payload_s
    {
        struct data_s
        {
            const char *client_crt;
        } data;
    };

    static const struct json_obj_descr data_descr[] = {
        JSON_OBJ_DESCR_PRIM(struct data_s, client_crt, JSON_TOK_STRING),
    };

    static const struct json_obj_descr payload_descr[] = {
        JSON_OBJ_DESCR_OBJECT(struct payload_s, data, data_descr),
    };

    struct payload_s payload = { .data = { .client_crt = crt_pem } };

    ssize_t len = json_calc_encoded_len(payload_descr, ARRAY_SIZE(payload_descr), &payload);
    if ((len <= 0) || (len > out_buff_size)) {
        ASTARTE_LOG_ERR("Insufficient buffer, requiring %zd bytes", len);
        return ASTARTE_RESULT_JSON_ERROR;
    }

    int ret = json_obj_encode_buf(
        payload_descr, ARRAY_SIZE(payload_descr), &payload, out_buff, out_buff_size);
    if (ret != 0) {
        ASTARTE_LOG_ERR("Error encoding payload: %d", ret);
        return ASTARTE_RESULT_JSON_ERROR;
    }
    return ASTARTE_RESULT_OK;
}

static astarte_result_t parse_verify_client_certificate_response(char *resp_buf)
{
    // Zephyr JSON parser does not support null values in parsing. Since a certificate invalid
    // response contains a null value, it should be replaced with an empty string.
    char *details_null = strstr(resp_buf, "\"details\":" JSON_NULL);
    if (details_null) {
        char details_null_replacement[] = "\"details\":" JSON_NULL_REPLACEMENT;
        // We perform a memcpy if a string excluding the null termination char on purpose as
        // this is only a substring of an already null terminated string
        // NOLINTNEXTLINE(bugprone-not-null-terminated-result)
        memcpy(details_null, details_null_replacement, strlen(details_null_replacement));
    }

    // Define the structure of the expected JSON file
    struct full_json_s
    {
        struct data_s
        {
            const char *timestamp;
            bool valid;
            const char *until;
            const char *details;
            const char *cause;
        } data;
    };
    // Create the descriptors list containing each element of the struct
    static const struct json_obj_descr data_descr[] = {
        JSON_OBJ_DESCR_PRIM(struct data_s, timestamp, JSON_TOK_STRING),
        JSON_OBJ_DESCR_PRIM(struct data_s, valid, JSON_TOK_TRUE),
        JSON_OBJ_DESCR_PRIM(struct data_s, until, JSON_TOK_STRING),
        JSON_OBJ_DESCR_PRIM(struct data_s, details, JSON_TOK_STRING),
        JSON_OBJ_DESCR_PRIM(struct data_s, cause, JSON_TOK_STRING),
    };
    static const struct json_obj_descr full_json_descr[] = {
        JSON_OBJ_DESCR_OBJECT(struct full_json_s, data, data_descr),
    };
    // This is only for the outside level, nested elements should be checked individually.
    int expected_return_code = (1U << (size_t) ARRAY_SIZE(full_json_descr)) - 1;
    // Parse the json into a structure
    struct full_json_s parsed_json = { .data.timestamp = NULL,
        .data.valid = false,
        .data.until = NULL,
        .data.details = NULL,
        .data.cause = NULL };
    int64_t ret = json_obj_parse(
        resp_buf, strlen(resp_buf) + 1, full_json_descr, ARRAY_SIZE(full_json_descr), &parsed_json);
    if (ret != expected_return_code) {
        ASTARTE_LOG_ERR("JSON Parse Error: %lld", ret);
        return ASTARTE_RESULT_JSON_ERROR;
    }

    // Check the certificate validity
    if (!parsed_json.data.valid) {
        if (!parsed_json.data.cause) {
            ASTARTE_LOG_ERR("Parsed JSON is missing the cause field.");
            return ASTARTE_RESULT_JSON_ERROR;
        }
        if (strlen(parsed_json.data.cause) == 0) {
            ASTARTE_LOG_ERR("Received empty cause field.");
            return ASTARTE_RESULT_JSON_ERROR;
        }
        ASTARTE_LOG_WRN("Invalid certificate, reason: %s", parsed_json.data.cause);
        return ASTARTE_RESULT_CLIENT_CERT_INVALID;
    }
    return ASTARTE_RESULT_OK;
}
