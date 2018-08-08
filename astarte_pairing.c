/*
 * (C) Copyright 2018, Ispirata Srl, info@ispirata.com.
 *
 * SPDX-License-Identifier: LGPL-2.1+ OR Apache-2.0
 */

#include "astarte_pairing.h"

#include <esp_http_client.h>
#include <esp_log.h>
#include <nvs.h>

#include <cJSON.h>

#include <string.h>

#define MAX_URL_LENGTH 512
#define MAX_HEADER_LENGTH 1024

#define TAG "ASTARTE_PAIRING"
#define PAIRING_NAMESPACE "astarte_pairing"
#define CRED_SECRET_KEY "cred_secret"

static esp_err_t http_event_handler(esp_http_client_event_t *evt);
static const char *extract_credentials_secret(cJSON *response);
static esp_err_t save_credentials_secret(const char *credentials_secret);

static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA: {
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            if (!esp_http_client_is_chunked_response(evt->client)) {
                ESP_LOGD(TAG, "Got response: %.*s", evt->data_len, (char *) evt->data);
                cJSON **resp_json = (cJSON **) evt->user_data;
                *resp_json = (void *) cJSON_Parse(evt->data);
            }

            break;
        }
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
    }

    return ESP_OK;
}

astarte_err_t astarte_pairing_get_credentials_secret(const struct astarte_pairing_config *config, char *out, unsigned int length)
{
    nvs_handle nvs;
    esp_err_t err = nvs_open(PAIRING_NAMESPACE, NVS_READONLY, &nvs);
    switch (err) {
        // NVS_NOT_FOUND is ok if we don't have credentials_secret yet
        case ESP_ERR_NVS_NOT_FOUND:
        case ESP_OK:
            break;
        case ESP_ERR_NVS_NOT_INITIALIZED:
            ESP_LOGE(TAG, "Non-volatile storage not initialized");
            ESP_LOGE(TAG, "You have to call nvs_flash_init() in your initialization code");
            return ASTARTE_ERR;
        default:
            ESP_LOGE(TAG, "nvs_open error while reading credentials_secret: %s", esp_err_to_name(err));
            return ASTARTE_ERR;
    }

    err = nvs_get_str(nvs, CRED_SECRET_KEY, out, &length);
    switch (err) {
        case ESP_OK:
            // Got it
            return ASTARTE_OK;

        // Here we come from NVS_NOT_FOUND above
        case ESP_ERR_NVS_INVALID_HANDLE:
        case ESP_ERR_NVS_NOT_FOUND:
            ESP_LOGI(TAG, "credentials_secret not found, registering device");
            break;

        default:
            ESP_LOGE(TAG, "nvs_get_str error: %s", esp_err_to_name(err));
            return ASTARTE_ERR;
    }

    astarte_err_t ast_err = astarte_pairing_register_device(config);
    if (ast_err != ASTARTE_OK) {
        ESP_LOGE(TAG, "Device registration failed: %d", ast_err);
        return ast_err;
    }

    // Open the handle again since it could have been invalid from above
    nvs_open(PAIRING_NAMESPACE, NVS_READONLY, &nvs);
    // Now we should have credentials_secret in NVS
    err = nvs_get_str(nvs, CRED_SECRET_KEY, out, &length);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Can't retrieve credentials_secret after registration: %s", esp_err_to_name(err));
        return ASTARTE_ERR;
    }

    return ASTARTE_OK;
}

astarte_err_t astarte_pairing_register_device(const struct astarte_pairing_config *config)
{
    char url[MAX_URL_LENGTH];
    snprintf(url, MAX_URL_LENGTH, "%s/v1/%s/agent/devices", config->base_url, config->realm);

    cJSON *resp = NULL;
    esp_http_client_config_t http_config = {
        .url = url,
        .event_handler = http_event_handler,
        .method = HTTP_METHOD_POST,
        .buffer_size = 2048,
        .user_data = &resp,
    };

    esp_http_client_handle_t client = esp_http_client_init(&http_config);

    cJSON *root = cJSON_CreateObject();
    cJSON *data = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "data", data);
    cJSON_AddStringToObject(data, "hw_id", config->hw_id);
    char *payload = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    ESP_ERROR_CHECK(esp_http_client_set_post_field(client, payload, strlen(payload)));

    char auth_header[MAX_HEADER_LENGTH];
    snprintf(auth_header, MAX_HEADER_LENGTH, "Bearer %s", config->jwt);
    ESP_ERROR_CHECK(esp_http_client_set_header(client, "Authorization", auth_header));
    ESP_ERROR_CHECK(esp_http_client_set_header(client, "Content-Type", "application/json"));

    esp_err_t err = esp_http_client_perform(client);

    astarte_err_t ret = ASTARTE_ERR;
    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %d",
                 status_code, esp_http_client_get_content_length(client));

        const char *credentials_secret = NULL;
        if (status_code == 201) {
            credentials_secret = extract_credentials_secret(resp);
            ESP_LOGI(TAG, "Device registered, credentials_secret is %s", credentials_secret);
        } else {
            char *json_error = cJSON_Print(resp);
            ESP_LOGE(TAG, "Device registration failed with code %d: %s", status_code, json_error);
            free(json_error);
        }

        if (credentials_secret && save_credentials_secret(credentials_secret) == ESP_OK) {
            ret = ASTARTE_OK;
        }
    } else {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }

    free(payload);
    cJSON_Delete(resp);
    esp_http_client_cleanup(client);

    return ret;
}

static const char *extract_credentials_secret(cJSON *response)
{
    const cJSON *data = cJSON_GetObjectItemCaseSensitive(response, "data");
    const cJSON *credentials_secret = cJSON_GetObjectItemCaseSensitive(data, "credentials_secret");

    if (cJSON_IsString(credentials_secret)) {
        return credentials_secret->valuestring;
    } else {
        ESP_LOGE(TAG, "Error parsing credentials_secret");
        return NULL;
    }
}

static esp_err_t save_credentials_secret(const char *credentials_secret)
{
    nvs_handle nvs;
    esp_err_t err = nvs_open(PAIRING_NAMESPACE, NVS_READWRITE, &nvs);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_open error while saving credentials_secret: %s", esp_err_to_name(err));
        return err;
    }

    err = nvs_set_str(nvs, CRED_SECRET_KEY, credentials_secret);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_set_str error while saving credentials_secret: %s", esp_err_to_name(err));
    }

    return err;
}
