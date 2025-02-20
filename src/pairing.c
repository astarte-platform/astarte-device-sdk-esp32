/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "astarte_device_sdk/pairing.h"
#include "pairing_private.h"

#include <string.h>
#include <stdio.h>

#include <cJSON.h>

#include <esp_log.h>

#include "http.h"
#include "astarte_device_sdk/device_id.h"

#define TAG "ASTARTE_PAIRING"

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
 * @param[in] resp_buf Response to parse.
 * @param[out] out_cred_secr Returned credential secret.
 * @param[in] out_cred_secr_size Output credential secret buffer size
 * @return ASTARTE_RESULT_OK if successful, otherwise an error code.
 */
static astarte_result_t parse_register_device_response(
    const char response[ASTARTE_HTTP_OUTPUT_BUFFER_LEN + 1],
    char out_cred_secr[NEW_AST_PAIRING_CRED_SECR_LEN + 1]);

/**
 * @brief Parse the response from the get broker url HTTP request.
 *
 * @param[in] resp_buf Response to parse.
 * @param[out] out_url Returned MQTT broker URL.
 * @param[in] out_url_size Output MQTT broker URL buffer size
 * @return ASTARTE_RESULT_OK if successful, otherwise an error code.
 */
static astarte_result_t parse_get_borker_url_response(
    const char response[ASTARTE_HTTP_OUTPUT_BUFFER_LEN + 1],
    char out_url[NEW_AST_PAIRING_MAX_BROKER_URL_LEN + 1]);

/************************************************
 *         Global functions definitions         *
 ***********************************************/

astarte_result_t new_ast_pairing_register_device(
    const char *device_id,
    char out_cred_secr[NEW_AST_PAIRING_CRED_SECR_LEN + 1])
{
    astarte_result_t ares = ASTARTE_RESULT_OK;
    char *payload = NULL;

    // Step 1: check the configuration and input parameters
    if (sizeof(CONFIG_ASTARTE_DEVICE_SDK_PAIRING_JWT) <= 1) {
        ESP_LOGE(TAG, "Registration of a device requires a valid pairing JWT");
        ares = ASTARTE_RESULT_INVALID_CONFIGURATION;
        goto exit;
    }
    if (strlen(device_id) != ASTARTE_DEVICE_ID_LEN) {
        ESP_LOGE(TAG, "Device ID has incorrect length, should be %d chars.", ASTARTE_DEVICE_ID_LEN);
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

    uint8_t reply[ASTARTE_HTTP_OUTPUT_BUFFER_LEN + 1] = {0};

    ares = astarte_http_post(host, path, auth_bearer, payload, reply);
    if (ares != ASTARTE_RESULT_OK) {
        goto exit;
    }

    // Step 3: process the result
    ares = parse_register_device_response((const char *) reply, out_cred_secr);
    ESP_LOGD(TAG, "Received credential secret: %s", out_cred_secr);

    // TODO: this is only for developement, remove it once done

    char broker_url[NEW_AST_PAIRING_MAX_BROKER_URL_LEN + 1] = {0};
    new_ast_pairing_get_mqtt_broker_url(device_id, out_cred_secr, broker_url);
    ESP_LOGE(TAG, "MQTT broker URL: %s", broker_url);

exit:
    free(payload);
    return ares;
}

astarte_result_t new_ast_pairing_get_mqtt_broker_url(
    const char *device_id, const char *cred_secr,
    char out_url[NEW_AST_PAIRING_MAX_BROKER_URL_LEN + 1])
{
    astarte_result_t ares = ASTARTE_RESULT_OK;
    // Step 1: check the input parameters
    if (strlen(device_id) != ASTARTE_DEVICE_ID_LEN) {
        ESP_LOGE(TAG,
            "Device ID has incorrect length, should be %d chars.", ASTARTE_DEVICE_ID_LEN);
        return ASTARTE_RESULT_INVALID_PARAM;
    }
    if (strlen(cred_secr) != NEW_AST_PAIRING_CRED_SECR_LEN) {
        ESP_LOGE(TAG,
            "Credential secret has incorrect length, should be %d chars.", NEW_AST_PAIRING_CRED_SECR_LEN);
        return ASTARTE_RESULT_INVALID_PARAM;
    }

    // Step 2: Get the MQTT broker URL
    char host[] = CONFIG_ASTARTE_DEVICE_SDK_HOSTNAME;
    char path[PAIRING_DEVICE_GET_BROKER_INFO_URL_LEN + 1] = { 0 };
    int snprintf_rc = snprintf(path, PAIRING_DEVICE_GET_BROKER_INFO_URL_LEN + 1,
        PAIRING_DEVICE_MGMT_URL_PREFIX "%s", device_id);
    if (snprintf_rc != PAIRING_DEVICE_GET_BROKER_INFO_URL_LEN) {
        ESP_LOGE(TAG, "Error encoding URL for get client certificate request.");
        return ASTARTE_RESULT_INTERNAL_ERROR;
    }

    uint8_t reply[ASTARTE_HTTP_OUTPUT_BUFFER_LEN + 1] = {0};
    ares = astarte_http_get(host, path, cred_secr, reply);
    if (ares != ASTARTE_RESULT_OK) {
        return ares;
    }

    // Step 3: process the result
    return parse_get_borker_url_response((const char *) reply, out_url);
}

/************************************************
 *         Static functions definitions         *
 ***********************************************/

static astarte_result_t parse_get_borker_url_response(
    const char response[ASTARTE_HTTP_OUTPUT_BUFFER_LEN + 1],
    char out_url[NEW_AST_PAIRING_MAX_BROKER_URL_LEN + 1])
{
    const cJSON *response_json = cJSON_Parse(response);
    const cJSON *data = cJSON_GetObjectItemCaseSensitive(response_json, "data");
    const cJSON *protocols = cJSON_GetObjectItemCaseSensitive(data, "protocols");
    const cJSON *astarte_mqtt_v1 = cJSON_GetObjectItemCaseSensitive(protocols, "astarte_mqtt_v1");
    const cJSON *broker_url = cJSON_GetObjectItemCaseSensitive(astarte_mqtt_v1, "broker_url");
    if (!cJSON_IsString(broker_url)) {
        ESP_LOGE(TAG, "Parsing the MQTT broker URL failed.");
        return ASTARTE_RESULT_INTERNAL_ERROR;
    }
    strncpy(out_url, broker_url->valuestring, NEW_AST_PAIRING_MAX_BROKER_URL_LEN);
    return ASTARTE_RESULT_OK;
}

static astarte_result_t parse_register_device_response(
    const char response[ASTARTE_HTTP_OUTPUT_BUFFER_LEN + 1],
    char out_cred_secr[NEW_AST_PAIRING_CRED_SECR_LEN + 1])
{
    const cJSON *response_json = cJSON_Parse(response);
    const cJSON *data = cJSON_GetObjectItemCaseSensitive(response_json, "data");
    const cJSON *credentials_secret = cJSON_GetObjectItemCaseSensitive(data, "credentials_secret");
    if (!cJSON_IsString(credentials_secret)) {
        ESP_LOGE(TAG, "Parsing the credentials secret failed.");
        return ASTARTE_RESULT_INTERNAL_ERROR;
    }
    strncpy(out_cred_secr, credentials_secret->valuestring, NEW_AST_PAIRING_CRED_SECR_LEN);
    return ASTARTE_RESULT_OK;
}
