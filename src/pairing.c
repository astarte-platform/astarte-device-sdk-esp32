/*
 * (C) Copyright 2024-2025, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "astarte_device_sdk/pairing.h"
#include "pairing_private.h"

#include <stdio.h>
#include <string.h>

#include <cJSON.h>

#include "astarte_device_sdk/device_id.h"
#include "http.h"
#include "log.h"

ASTARTE_LOG_MODULE_REGISTER("Astarte pairing");

/************************************************
 *        Defines, constants and typedef        *
 ***********************************************/

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
 * @brief Parse the response from the device registration HTTP request.
 *
 * @param[in] response Response to parse.
 * @param[out] out_cred_secr Returned credential secret.
 * @return ASTARTE_RESULT_OK if successful, otherwise an error code.
 */
static astarte_result_t parse_register_device_response(
    const char response[HTTP_OUTPUT_BUFFER_LEN + 1],
    char out_cred_secr[NEW_AST_PAIRING_CRED_SECR_LEN + 1]);
/**
 * @brief Parse the response from the get broker url HTTP request.
 *
 * @param[in] response Response to parse.
 * @param[out] out_url Returned MQTT broker URL.
 * @return ASTARTE_RESULT_OK if successful, otherwise an error code.
 */
static astarte_result_t parse_get_borker_url_response(
    const char response[HTTP_OUTPUT_BUFFER_LEN + 1], char out_url[PAIRING_MAX_BROKER_URL_LEN + 1]);
/**
 * @brief Parse the response from the get client certificate HTTP request.
 *
 * @param[in] response Response to parse.
 * @param[out] out_url Returned client certificate in the PEM format.
 * @return ASTARTE_RESULT_OK if successful, otherwise an error code.
 */
static astarte_result_t parse_get_client_certificate_response(
    const char response[HTTP_OUTPUT_BUFFER_LEN + 1],
    char out_crt_pem[CONFIG_ASTARTE_DEVICE_SDK_ADVANCED_CLIENT_CRT_BUFFER_SIZE]);
/**
 * @brief Parse the response from the verify client certificate HTTP request.
 *
 * @param[in] response Response to parse.
 * @return ASTARTE_RESULT_OK if successful, ASTARTE_RESULT_CLIENT_CERT_INVALID when the certificate
 * is invalid, otherwise an error code.
 */
static astarte_result_t parse_verify_client_certificate_response(
    const char response[HTTP_OUTPUT_BUFFER_LEN + 1]);

/************************************************
 *         Global functions definitions         *
 ***********************************************/

astarte_result_t new_ast_pairing_register_device(
    const char *device_id, char out_cred_secr[NEW_AST_PAIRING_CRED_SECR_LEN + 1])
{
    astarte_result_t ares = ASTARTE_RESULT_OK;
    char *payload = NULL;

    // Step 1: check the configuration and input parameters
    if (sizeof(CONFIG_ASTARTE_DEVICE_SDK_PAIRING_JWT) <= 1) {
        ASTARTE_LOG_ERR("Registration of a device requires a valid pairing JWT");
        ares = ASTARTE_RESULT_INVALID_CONFIGURATION;
        goto exit;
    }
    if (strlen(device_id) != ASTARTE_DEVICE_ID_LEN) {
        ASTARTE_LOG_ERR(
            "Device ID has incorrect length, should be %d chars.", ASTARTE_DEVICE_ID_LEN);
        ares = ASTARTE_RESULT_INVALID_PARAM;
        goto exit;
    }

    // Step 2: register the device and get the credential secret
    char host[] = CONFIG_ASTARTE_DEVICE_SDK_HOSTNAME;
    char path[] = PAIRING_REGISTRATION_URL;
    char auth_bearer[] = CONFIG_ASTARTE_DEVICE_SDK_PAIRING_JWT;

    cJSON *root = cJSON_CreateObject();
    cJSON *data = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "data", data);
    cJSON_AddStringToObject(data, "hw_id", device_id);
    payload = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    uint8_t reply[HTTP_OUTPUT_BUFFER_LEN + 1] = { 0 };

    ares = http_post(host, path, auth_bearer, payload, reply);
    if (ares != ASTARTE_RESULT_OK) {
        goto exit;
    }

    // Step 3: process the result
    ares = parse_register_device_response((const char *) reply, out_cred_secr);
    ASTARTE_LOG_DBG("Received credential secret: %s", out_cred_secr);

exit:
    free(payload);
    return ares;
}

astarte_result_t pairing_get_mqtt_broker_url(
    const char *device_id, const char *cred_secr, char out_url[PAIRING_MAX_BROKER_URL_LEN + 1])
{
    astarte_result_t ares = ASTARTE_RESULT_OK;
    // Step 1: check the input parameters
    if (strlen(device_id) != ASTARTE_DEVICE_ID_LEN) {
        ASTARTE_LOG_ERR(
            "Device ID has incorrect length, should be %d chars.", ASTARTE_DEVICE_ID_LEN);
        return ASTARTE_RESULT_INVALID_PARAM;
    }
    if (strlen(cred_secr) != NEW_AST_PAIRING_CRED_SECR_LEN) {
        ASTARTE_LOG_ERR("Credential secret has incorrect length, should be %d chars.",
            NEW_AST_PAIRING_CRED_SECR_LEN);
        return ASTARTE_RESULT_INVALID_PARAM;
    }

    // Step 2: Get the MQTT broker URL
    char host[] = CONFIG_ASTARTE_DEVICE_SDK_HOSTNAME;
    char path[PAIRING_DEVICE_GET_BROKER_INFO_URL_LEN + 1] = { 0 };
    int snprintf_rc = snprintf(path, PAIRING_DEVICE_GET_BROKER_INFO_URL_LEN + 1,
        PAIRING_DEVICE_MGMT_URL_PREFIX "%s", device_id);
    if (snprintf_rc != PAIRING_DEVICE_GET_BROKER_INFO_URL_LEN) {
        ASTARTE_LOG_ERR("Error encoding URL for get client certificate request.");
        return ASTARTE_RESULT_INTERNAL_ERROR;
    }

    uint8_t reply[HTTP_OUTPUT_BUFFER_LEN + 1] = { 0 };
    ares = http_get(host, path, cred_secr, reply);
    if (ares != ASTARTE_RESULT_OK) {
        return ares;
    }

    // Step 3: process the result
    return parse_get_borker_url_response((const char *) reply, out_url);
}

astarte_result_t pairing_get_client_certificate(
    const char *device_id, const char *cred_secr, tls_credentials_client_crt_t *client_crt)
{
    astarte_result_t ares = ASTARTE_RESULT_OK;
    char *payload = NULL;

    // Step 1: check the configuration and input parameters
    if (strlen(device_id) != ASTARTE_DEVICE_ID_LEN) {
        ASTARTE_LOG_ERR(
            "Device ID has incorrect length, should be %d chars.", ASTARTE_DEVICE_ID_LEN);
        ares = ASTARTE_RESULT_INVALID_PARAM;
        goto exit;
    }
    if (strlen(cred_secr) != NEW_AST_PAIRING_CRED_SECR_LEN) {
        ASTARTE_LOG_ERR("Credential secret has incorrect length, should be %d chars.",
            NEW_AST_PAIRING_CRED_SECR_LEN);
        ares = ASTARTE_RESULT_INVALID_PARAM;
        goto exit;
    }

    // Step 2: create a private key and a CSR
    ares = crypto_create_key(client_crt->privkey_pem, ARRAY_SIZE(client_crt->privkey_pem));
    if (ares != ASTARTE_RESULT_OK) {
        ASTARTE_LOG_ERR("Failed in creating a private key.");
        goto exit;
    }
    unsigned char csr_buf[CRYPTO_CSR_BUFFER_SIZE];
    ares = crypto_create_csr(client_crt->privkey_pem, csr_buf, sizeof(csr_buf));
    if (ares != ASTARTE_RESULT_OK) {
        ASTARTE_LOG_ERR("Failed in creating a CSR.");
        goto exit;
    }

    // Step 3: get the client certificate from the server
    char host[] = CONFIG_ASTARTE_DEVICE_SDK_HOSTNAME;
    char path[PAIRING_DEVICE_GET_DEVICE_CERT_URL_LEN + 1] = { 0 };
    int snprintf_rc = snprintf(path, PAIRING_DEVICE_GET_DEVICE_CERT_URL_LEN + 1,
        PAIRING_DEVICE_MGMT_URL_PREFIX "%s" PAIRING_DEVICE_CERT_URL_SUFFIX, device_id);
    if (snprintf_rc != PAIRING_DEVICE_GET_DEVICE_CERT_URL_LEN) {
        ASTARTE_LOG_ERR("Error encoding URL for get client certificate request.");
        ares = ASTARTE_RESULT_INTERNAL_ERROR;
        goto exit;
    }

    cJSON *root = cJSON_CreateObject();
    cJSON *data = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "data", data);
    cJSON_AddStringToObject(data, "csr", (const char *) csr_buf);
    payload = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    uint8_t reply[HTTP_OUTPUT_BUFFER_LEN + 1] = { 0 };

    ares = http_post(host, path, cred_secr, payload, reply);
    if (ares != ASTARTE_RESULT_OK) {
        goto exit;
    }

    // Step 4: process the result
    ares = parse_get_client_certificate_response((const char *) reply, client_crt->crt_pem);
    if (ares != ASTARTE_RESULT_OK) {
        goto exit;
    }
    ASTARTE_LOG_DBG("Received client certificate: %s", client_crt->crt_pem);

exit:
    free(payload);
    return ares;
}

astarte_result_t pairing_verify_client_certificate(
    const char *device_id, const char *cred_secr, const char *crt_pem)
{
    astarte_result_t ares = ASTARTE_RESULT_OK;
    char *payload = NULL;

    // Step 1: check the configuration and input parameters
    if (strlen(device_id) != ASTARTE_DEVICE_ID_LEN) {
        ASTARTE_LOG_ERR(
            "Device ID has incorrect length, should be %d chars.", ASTARTE_DEVICE_ID_LEN);
        return ASTARTE_RESULT_INVALID_PARAM;
    }
    if (strlen(cred_secr) != NEW_AST_PAIRING_CRED_SECR_LEN) {
        ASTARTE_LOG_ERR("Credential secret has incorrect length, should be %d chars.",
            NEW_AST_PAIRING_CRED_SECR_LEN);
        ares = ASTARTE_RESULT_INVALID_PARAM;
        goto exit;
    }
    if (!crt_pem) {
        ASTARTE_LOG_ERR("Attempting to validate an undefined client certificate.");
        ares = ASTARTE_RESULT_INVALID_PARAM;
        goto exit;
    }

    // Step 2: register the device and get the credential secret
    char host[] = CONFIG_ASTARTE_DEVICE_SDK_HOSTNAME;
    char path[PAIRING_DEVICE_CERT_CHECK_URL_LEN + 1] = { 0 };
    int snprintf_rc = snprintf(path, PAIRING_DEVICE_CERT_CHECK_URL_LEN + 1,
        PAIRING_DEVICE_MGMT_URL_PREFIX "%s" PAIRING_DEVICE_CERT_CHECK_URL_SUFFIX, device_id);
    if (snprintf_rc != PAIRING_DEVICE_CERT_CHECK_URL_LEN) {
        ASTARTE_LOG_ERR("Error encoding URL for verify client certificate request.");
        ares = ASTARTE_RESULT_INTERNAL_ERROR;
        goto exit;
    }

    cJSON *root = cJSON_CreateObject();
    cJSON *data = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "data", data);
    cJSON_AddStringToObject(data, "client_crt", crt_pem);
    payload = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    uint8_t reply[HTTP_OUTPUT_BUFFER_LEN + 1] = { 0 };

    ares = http_post(host, path, cred_secr, payload, reply);
    if (ares != ASTARTE_RESULT_OK) {
        goto exit;
    }

    // Step 3: process the result
    ares = parse_verify_client_certificate_response((char *) reply);

exit:
    free(payload);
    return ares;
}

/************************************************
 *         Static functions definitions         *
 ***********************************************/

static astarte_result_t parse_get_borker_url_response(
    const char response[HTTP_OUTPUT_BUFFER_LEN + 1], char out_url[PAIRING_MAX_BROKER_URL_LEN + 1])
{
    const cJSON *response_json = cJSON_Parse(response);
    const cJSON *data = cJSON_GetObjectItemCaseSensitive(response_json, "data");
    const cJSON *protocols = cJSON_GetObjectItemCaseSensitive(data, "protocols");
    const cJSON *astarte_mqtt_v1 = cJSON_GetObjectItemCaseSensitive(protocols, "astarte_mqtt_v1");
    const cJSON *broker_url = cJSON_GetObjectItemCaseSensitive(astarte_mqtt_v1, "broker_url");
    if (!cJSON_IsString(broker_url)) {
        ASTARTE_LOG_ERR("Parsing the MQTT broker URL failed.");
        return ASTARTE_RESULT_INTERNAL_ERROR;
    }
    strncpy(out_url, broker_url->valuestring, PAIRING_MAX_BROKER_URL_LEN);
    return ASTARTE_RESULT_OK;
}

static astarte_result_t parse_register_device_response(
    const char response[HTTP_OUTPUT_BUFFER_LEN + 1],
    char out_cred_secr[NEW_AST_PAIRING_CRED_SECR_LEN + 1])
{
    const cJSON *response_json = cJSON_Parse(response);
    const cJSON *data = cJSON_GetObjectItemCaseSensitive(response_json, "data");
    const cJSON *credentials_secret = cJSON_GetObjectItemCaseSensitive(data, "credentials_secret");
    if (!cJSON_IsString(credentials_secret)) {
        ASTARTE_LOG_ERR("Parsing the credentials secret failed.");
        return ASTARTE_RESULT_INTERNAL_ERROR;
    }
    strncpy(out_cred_secr, credentials_secret->valuestring, NEW_AST_PAIRING_CRED_SECR_LEN);
    return ASTARTE_RESULT_OK;
}

static astarte_result_t parse_get_client_certificate_response(
    const char response[HTTP_OUTPUT_BUFFER_LEN + 1],
    char out_crt_pem[CONFIG_ASTARTE_DEVICE_SDK_ADVANCED_CLIENT_CRT_BUFFER_SIZE])
{

    const cJSON *response_json = cJSON_Parse(response);
    const cJSON *data = cJSON_GetObjectItemCaseSensitive(response_json, "data");
    const cJSON *client_crt = cJSON_GetObjectItemCaseSensitive(data, "client_crt");
    if (!cJSON_IsString(client_crt)) {
        ASTARTE_LOG_ERR("Parsing the client certificate failed.");
        return ASTARTE_RESULT_INTERNAL_ERROR;
    }
    strncpy(out_crt_pem, client_crt->valuestring,
        CONFIG_ASTARTE_DEVICE_SDK_ADVANCED_CLIENT_CRT_BUFFER_SIZE - 1);
    // Replace "\n" with newlines chars
    char *tmp = NULL;
    while ((tmp = strstr(out_crt_pem, "\\n")) != NULL) {
        tmp[0] = '\n';
        tmp[1] = '\n';
    }
    return ASTARTE_RESULT_OK;
}

static astarte_result_t parse_verify_client_certificate_response(
    const char response[HTTP_OUTPUT_BUFFER_LEN + 1])
{
    const cJSON *response_json = cJSON_Parse(response);
    const cJSON *data = cJSON_GetObjectItemCaseSensitive(response_json, "data");
    const cJSON *valid = cJSON_GetObjectItemCaseSensitive(data, "valid");
    if (!cJSON_IsBool(valid)) {
        ASTARTE_LOG_ERR("Parsing the client certificate failed.");
        return ASTARTE_RESULT_INTERNAL_ERROR;
    }
    if (cJSON_IsFalse(valid)) {
        const cJSON *cause = cJSON_GetObjectItemCaseSensitive(data, "cause");
        if (!cJSON_IsString(cause)) {
            ASTARTE_LOG_ERR("Parsing the client certificate failed.");
            return ASTARTE_RESULT_INTERNAL_ERROR;
        }
        ASTARTE_LOG_ERR("Invalid certificate, reason: %s", cause->valuestring);
        return ASTARTE_RESULT_CLIENT_CERT_INVALID;
    }
    return ASTARTE_RESULT_OK;
}
