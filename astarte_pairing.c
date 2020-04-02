/*
 * (C) Copyright 2018, Ispirata Srl, info@ispirata.com.
 *
 * SPDX-License-Identifier: LGPL-2.1+ OR Apache-2.0
 */

#include "astarte_pairing.h"

#include "astarte_credentials.h"

#include <esp_http_client.h>
#include <esp_log.h>

#include <cJSON.h>

#include <string.h>

#define CRED_SECRET_LENGTH 256
#define MAX_URL_LENGTH 512
#define MAX_CRED_SECR_HEADER_LENGTH 64
#define MAX_JWT_HEADER_LENGTH 1024

#define TAG "ASTARTE_PAIRING"

static esp_err_t http_event_handler(esp_http_client_event_t *evt);
static const char *extract_broker_url(cJSON *response);
static const char *extract_credentials_secret(cJSON *response);
static const char *extract_client_crt(cJSON *response);

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

astarte_err_t astarte_pairing_get_credentials_secret(const struct astarte_pairing_config *config, char *out, size_t length)
{
    if (config->credentials_secret) {
        // We have an explicit credentials_secret in the config, we're done
        strncpy(out, config->credentials_secret, length);
        return ASTARTE_OK;
    }

    astarte_err_t err = astarte_credentials_get_stored_credentials_secret(out, length);
    switch (err) {
        case ASTARTE_OK:
            return ASTARTE_OK;

        case ASTARTE_ERR_NOT_FOUND:
            ESP_LOGI(TAG, "credentials_secret not found, registering device");
            break;

        default:
            // Some other error happened, bail out
            return err;
    }

    err = astarte_pairing_register_device(config);
    if (err != ASTARTE_OK) {
        ESP_LOGE(TAG, "Device registration failed: %d", err);
        return err;
    }

    // Now we should have credentials_secret in NVS
    err = astarte_credentials_get_stored_credentials_secret(out, length);
    if (err != ASTARTE_OK) {
        ESP_LOGE(TAG, "Can't retrieve credentials_secret after registration");
        return err;
    }

    return ASTARTE_OK;
}

astarte_err_t astarte_pairing_get_mqtt_v1_credentials(const struct astarte_pairing_config *config, const char *csr, char *out, size_t length)
{
    astarte_err_t ret = ASTARTE_ERR;
    char *cred_secret = NULL;
    char *url = NULL;
    char *auth_header = NULL;
    cJSON *resp = NULL;
    char *payload = NULL;
    esp_http_client_handle_t client = NULL;

    cred_secret = calloc(CRED_SECRET_LENGTH, sizeof(char));
    if (!cred_secret) {
        ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
        goto exit;
    }

    astarte_err_t err = astarte_pairing_get_credentials_secret(config, cred_secret, 256);
    if (err != ASTARTE_OK) {
        ESP_LOGE(TAG, "Can't retrieve credentials_secret");
        goto exit;
    }

    url = calloc(MAX_URL_LENGTH, sizeof(char));
    if (!url) {
        ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
        goto exit;
    }

    snprintf(url, MAX_URL_LENGTH, "%s/v1/%s/devices/%s/protocols/astarte_mqtt_v1/credentials", config->base_url, config->realm, config->hw_id);

    esp_http_client_config_t http_config = {
        .url = url,
        .event_handler = http_event_handler,
        .method = HTTP_METHOD_POST,
        .buffer_size = 2048,
        .user_data = &resp,
    };

    client = esp_http_client_init(&http_config);
    if (!client) {
        ESP_LOGE(TAG, "Could not initialize http client");
        goto exit;
    }

    cJSON *root = cJSON_CreateObject();
    cJSON *data = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "data", data);
    cJSON_AddStringToObject(data, "csr", csr);
    payload = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    ESP_ERROR_CHECK(esp_http_client_set_post_field(client, payload, strlen(payload)));

    auth_header = calloc(MAX_CRED_SECR_HEADER_LENGTH, sizeof(char));
    if (!auth_header) {
        ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
        goto exit;
    }

    snprintf(auth_header, MAX_CRED_SECR_HEADER_LENGTH, "Bearer %s", cred_secret);
    ESP_ERROR_CHECK(esp_http_client_set_header(client, "Authorization", auth_header));
    ESP_ERROR_CHECK(esp_http_client_set_header(client, "Content-Type", "application/json"));

    err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %d",
                 status_code, esp_http_client_get_content_length(client));

        const char *client_crt = NULL;
        if (status_code == 201) {
            client_crt = extract_client_crt(resp);
            ESP_LOGI(TAG, "Got credentials, client_crt is %s", client_crt);
        } else {
            char *json_error = cJSON_Print(resp);
            if (json_error) {
                ESP_LOGE(TAG, "Device credentials request failed with code %d: %s", status_code, json_error);
                free(json_error);
            } else {
                ESP_LOGE(TAG, "Device credentials request failed with code %d", status_code);
            }
        }

        if (client_crt) {
            strncpy(out, client_crt, length);
            ret = ASTARTE_OK;
        }
    } else {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }

exit:
    free(cred_secret);
    free(url);
    free(auth_header);
    free(payload);
    if (resp) {
        cJSON_Delete(resp);
    }
    if (client) {
        esp_http_client_cleanup(client);
    }

    return ret;
}

astarte_err_t astarte_pairing_get_mqtt_v1_broker_url(const struct astarte_pairing_config *config, char *out, size_t length)
{
    astarte_err_t ret = ASTARTE_ERR;
    char *cred_secret = NULL;
    char *url = NULL;
    char *auth_header = NULL;
    cJSON *resp = NULL;
    esp_http_client_handle_t client = NULL;

    cred_secret = calloc(CRED_SECRET_LENGTH, sizeof(char));
    if (!cred_secret) {
        ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
        goto exit;
    }

    astarte_err_t err = astarte_pairing_get_credentials_secret(config, cred_secret, 256);
    if (err != ASTARTE_OK) {
        ESP_LOGE(TAG, "Can't retrieve credentials_secret");
        goto exit;
    }

    url = calloc(MAX_URL_LENGTH, sizeof(char));
    if (!url) {
        ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
        goto exit;
    }

    snprintf(url, MAX_URL_LENGTH, "%s/v1/%s/devices/%s", config->base_url, config->realm, config->hw_id);

    esp_http_client_config_t http_config = {
        .url = url,
        .event_handler = http_event_handler,
        .method = HTTP_METHOD_GET,
        .buffer_size = 2048,
        .user_data = &resp,
    };

    client = esp_http_client_init(&http_config);
    if (!client) {
        ESP_LOGE(TAG, "Could not initialize http client");
        goto exit;
    }

    auth_header = calloc(MAX_CRED_SECR_HEADER_LENGTH, sizeof(char));
    if (!auth_header) {
        ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
        goto exit;
    }

    snprintf(auth_header, MAX_CRED_SECR_HEADER_LENGTH, "Bearer %s", cred_secret);
    ESP_ERROR_CHECK(esp_http_client_set_header(client, "Authorization", auth_header));
    ESP_ERROR_CHECK(esp_http_client_set_header(client, "Content-Type", "application/json"));

    err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d",
                 status_code, esp_http_client_get_content_length(client));

        const char *broker_url = NULL;
        if (status_code == 200) {
            broker_url = extract_broker_url(resp);
            ESP_LOGI(TAG, "Got info, broker_url is %s", broker_url);
        } else {
            char *json_error = cJSON_Print(resp);
            if (json_error) {
                ESP_LOGE(TAG, "Device info failed with code %d: %s", status_code, json_error);
                free(json_error);
            } else {
                ESP_LOGE(TAG, "Device info failed with code %d", status_code);
            }
        }

        if (broker_url) {
            strncpy(out, broker_url, length);
            ret = ASTARTE_OK;
        }
    } else {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }

exit:
    free(cred_secret);
    free(url);
    free(auth_header);
    if (resp) {
        cJSON_Delete(resp);
    }
    if (client) {
        esp_http_client_cleanup(client);
    }

    return ret;
}

astarte_err_t astarte_pairing_register_device(const struct astarte_pairing_config *config)
{
    if (!config->jwt || strlen(config->jwt) == 0) {
        ESP_LOGE(TAG, "ASTARTE_PAIRING_JWT is not configured, device can't be registered. "
                      "Configure it using make menuconfig");
        return ASTARTE_ERR;
    }

    astarte_err_t ret = ASTARTE_ERR;
    char *url = NULL;
    char *auth_header = NULL;
    cJSON *resp = NULL;
    char *payload = NULL;
    esp_http_client_handle_t client = NULL;

    url = calloc(MAX_URL_LENGTH, sizeof(char));
    if (!url) {
        ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
        goto exit;
    }

    snprintf(url, MAX_URL_LENGTH, "%s/v1/%s/agent/devices", config->base_url, config->realm);

    esp_http_client_config_t http_config = {
        .url = url,
        .event_handler = http_event_handler,
        .method = HTTP_METHOD_POST,
        .buffer_size = 2048,
        .user_data = &resp,
    };

    client = esp_http_client_init(&http_config);
    if (!client) {
        ESP_LOGE(TAG, "Could not initialize http client");
        goto exit;
    }

    cJSON *root = cJSON_CreateObject();
    cJSON *data = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "data", data);
    cJSON_AddStringToObject(data, "hw_id", config->hw_id);
    payload = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    ESP_ERROR_CHECK(esp_http_client_set_post_field(client, payload, strlen(payload)));

    auth_header = calloc(MAX_JWT_HEADER_LENGTH, sizeof(char));
    if (!auth_header) {
        ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
        goto exit;
    }

    snprintf(auth_header, MAX_JWT_HEADER_LENGTH, "Bearer %s", config->jwt);
    ESP_ERROR_CHECK(esp_http_client_set_header(client, "Authorization", auth_header));
    ESP_ERROR_CHECK(esp_http_client_set_header(client, "Content-Type", "application/json"));

    esp_err_t err = esp_http_client_perform(client);

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
            if (json_error) {
                ESP_LOGE(TAG, "Device registration failed with code %d: %s", status_code, json_error);
                free(json_error);
            } else {
                ESP_LOGE(TAG, "Device registration failed with code %d", status_code);
            }
        }
        if (credentials_secret &&
            astarte_credentials_set_stored_credentials_secret(credentials_secret) == ASTARTE_OK) {
            ret = ASTARTE_OK;
        }
    } else {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }

exit:
    free(url);
    free(auth_header);
    free(payload);
    if (resp) {
        cJSON_Delete(resp);
    }
    if (client) {
        esp_http_client_cleanup(client);
    }

    return ret;
}

static const char *extract_broker_url(cJSON *response)
{
    const cJSON *data = cJSON_GetObjectItemCaseSensitive(response, "data");
    const cJSON *protocols = cJSON_GetObjectItemCaseSensitive(data, "protocols");
    const cJSON *astarte_mqtt_v1 = cJSON_GetObjectItemCaseSensitive(protocols, "astarte_mqtt_v1");
    const cJSON *broker_url = cJSON_GetObjectItemCaseSensitive(astarte_mqtt_v1, "broker_url");

    if (cJSON_IsString(broker_url)) {
        return broker_url->valuestring;
    } else {
        ESP_LOGE(TAG, "Error parsing broker_url");
        return NULL;
    }
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

static const char *extract_client_crt(cJSON *response)
{
    const cJSON *data = cJSON_GetObjectItemCaseSensitive(response, "data");
    const cJSON *client_crt = cJSON_GetObjectItemCaseSensitive(data, "client_crt");

    if (cJSON_IsString(client_crt)) {
        return client_crt->valuestring;
    } else {
        ESP_LOGE(TAG, "Error parsing client_crt");
        return NULL;
    }
}
