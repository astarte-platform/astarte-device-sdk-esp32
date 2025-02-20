/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "http.h"

#include <sys/param.h>

#include <esp_http_client.h>
#if defined(CONFIG_MBEDTLS_CERTIFICATE_BUNDLE)
#include <esp_crt_bundle.h>
#endif /* defined(CONFIG_MBEDTLS_CERTIFICATE_BUNDLE) */
#include <esp_log.h>
#include <esp_tls.h>

#define TAG "ASTARTE_HTTP"

/************************************************
 *       Checks over configuration values       *
 ***********************************************/

#if defined(CONFIG_ESP_TLS_INSECURE) && defined(CONFIG_ESP_TLS_SKIP_SERVER_CERT_VERIFY)
#warning "TLS has been disabled (unsafe)!"
#endif /* defined(CONFIG_ESP_TLS_INSECURE) && defined(CONFIG_ESP_TLS_SKIP_SERVER_CERT_VERIFY) */

/************************************************
 *       Callbacks declaration/definition       *
 ***********************************************/

static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    // Stores number of bytes read
    static int output_len;
    switch (evt->event_id) {
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
            ESP_LOGD(
                TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA: {
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            // Clean the buffer in case of a new request
            if (output_len == 0 && evt->user_data) {
                // we are just starting to copy the output data into the user data
                memset(evt->user_data, 0, ASTARTE_HTTP_OUTPUT_BUFFER_LEN);
            }
            if (!esp_http_client_is_chunked_response(evt->client)) {
                ESP_LOGD(TAG, "Got response: %.*s", evt->data_len, (char *) evt->data);
                // If user_data buffer is configured, copy the response into the buffer
                int copy_len = 0;
                if (evt->user_data) {
                    // The last byte in evt->user_data is kept for the NULL character in case of
                    // out-of-bound access.
                    copy_len = MIN(evt->data_len, (ASTARTE_HTTP_OUTPUT_BUFFER_LEN - output_len));
                    if (copy_len) {
                        memcpy(evt->user_data + output_len, evt->data, copy_len);
                    }
                }
                output_len += copy_len;
            }
            break;
        }
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            output_len = 0;
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
            int mbedtls_err = 0;
            esp_err_t err = esp_tls_get_and_clear_last_error(
                (esp_tls_error_handle_t) evt->data, &mbedtls_err, NULL);
            if (err != 0) {
                ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
                ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
            }
            output_len = 0;
            break;
        case HTTP_EVENT_REDIRECT:
            ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
            break;
    }

    return ESP_OK;
}

/************************************************
 *         Static functions declaration         *
 ***********************************************/

/************************************************
 *         Global functions definitions         *
 ***********************************************/

astarte_result_t astarte_http_post(const char *host, const char *path, const char *auth_bearer,
    const char *payload, uint8_t out[ASTARTE_HTTP_OUTPUT_BUFFER_LEN + 1])
{
    astarte_result_t ares = ASTARTE_RESULT_OK;
    char *auth_header = NULL;
    esp_http_client_handle_t client = NULL;

    esp_http_client_config_t config = {
        .host = host,
        .path = path,
        .event_handler = http_event_handler,
#if !defined(CONFIG_ESP_TLS_SKIP_SERVER_CERT_VERIFY)
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
#else /* !defined(CONFIG_ESP_TLS_SKIP_SERVER_CERT_VERIFY) */
        .transport_type = HTTP_TRANSPORT_OVER_TCP,
#endif /* !defined(CONFIG_ESP_TLS_SKIP_SERVER_CERT_VERIFY) */
#if defined(CONFIG_MBEDTLS_CERTIFICATE_BUNDLE)
        .crt_bundle_attach = esp_crt_bundle_attach,
#endif /* defined(CONFIG_MBEDTLS_CERTIFICATE_BUNDLE) */
        .user_data = out,
    };
    client = esp_http_client_init(&config);

    ESP_ERROR_CHECK(esp_http_client_set_method(client, HTTP_METHOD_POST));
    ESP_ERROR_CHECK(esp_http_client_set_post_field(client, payload, strlen(payload)));

    const size_t auth_header_len = strlen("Bearer ") + strlen(auth_bearer);
    auth_header = calloc(auth_header_len + 1, sizeof(char));
    if (!auth_header) {
        ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
        ares = ASTARTE_RESULT_OUT_OF_MEMORY;
        goto exit;
    }

    int print_ret = snprintf(auth_header, auth_header_len + 1, "Bearer %s", auth_bearer);
    if (print_ret != auth_header_len) {
        ESP_LOGE(TAG, "Error encoding authorization header");
        ares = ASTARTE_RESULT_INTERNAL_ERROR;
        goto exit;
    }
    ESP_ERROR_CHECK(esp_http_client_set_header(client, "Authorization", auth_header));
    ESP_ERROR_CHECK(esp_http_client_set_header(client, "Content-Type", "application/json"));

    esp_err_t esp_err = esp_http_client_perform(client);
    if (esp_err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        ESP_LOGD(TAG, "HTTP POST Status = %d, content_length = %" PRIi64, status_code,
            esp_http_client_get_content_length(client));
            if ((status_code < 200) || (status_code >= 300)) {
            ESP_LOGE(TAG, "HTTP POST Status = %d, content_length = %" PRIi64, status_code,
                esp_http_client_get_content_length(client));
            ares = ASTARTE_RESULT_HTTP_REQUEST_ERROR;
            goto exit;
        }
    } else {
        ares = ASTARTE_RESULT_HTTP_REQUEST_ERROR;
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(esp_err));
    }

exit:
    free(auth_header);
    if (client) {
        esp_http_client_cleanup(client);
    }

    return ares;
}

astarte_result_t astarte_http_get(const char *host, const char *path, const char *auth_bearer,
    uint8_t out[ASTARTE_HTTP_OUTPUT_BUFFER_LEN + 1])
{
    astarte_result_t ares = ASTARTE_RESULT_OK;
    char *auth_header = NULL;
    esp_http_client_handle_t client = NULL;

    esp_http_client_config_t config = {
        .host = host,
        .path = path,
        .event_handler = http_event_handler,
#if !defined(CONFIG_ESP_TLS_SKIP_SERVER_CERT_VERIFY)
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
#else /* !defined(CONFIG_ESP_TLS_SKIP_SERVER_CERT_VERIFY) */
        .transport_type = HTTP_TRANSPORT_OVER_TCP,
#endif /* !defined(CONFIG_ESP_TLS_SKIP_SERVER_CERT_VERIFY) */
#if defined(CONFIG_MBEDTLS_CERTIFICATE_BUNDLE)
        .crt_bundle_attach = esp_crt_bundle_attach,
#endif /* defined(CONFIG_MBEDTLS_CERTIFICATE_BUNDLE) */
        .user_data = out,
    };
    client = esp_http_client_init(&config);

    ESP_ERROR_CHECK(esp_http_client_set_method(client, HTTP_METHOD_GET));

    const size_t auth_header_len = strlen("Bearer ") + strlen(auth_bearer);
    auth_header = calloc(auth_header_len + 1, sizeof(char));
    if (!auth_header) {
        ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
        ares = ASTARTE_RESULT_OUT_OF_MEMORY;
        goto exit;
    }

    int print_ret = snprintf(auth_header, auth_header_len + 1, "Bearer %s", auth_bearer);
    if (print_ret != auth_header_len) {
        ESP_LOGE(TAG, "Error encoding authorization header");
        ares = ASTARTE_RESULT_INTERNAL_ERROR;
        goto exit;
    }
    ESP_ERROR_CHECK(esp_http_client_set_header(client, "Authorization", auth_header));
    ESP_ERROR_CHECK(esp_http_client_set_header(client, "Content-Type", "application/json"));

    esp_err_t esp_err = esp_http_client_perform(client);
    if (esp_err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        ESP_LOGD(TAG, "HTTP GET Status = %d, content_length = %" PRIi64, status_code,
            esp_http_client_get_content_length(client));
        if ((status_code < 200) || (status_code >= 300)) {
            ESP_LOGE(TAG, "HTTP GET Status = %d, content_length = %" PRIi64, status_code,
                esp_http_client_get_content_length(client));
            ares = ASTARTE_RESULT_HTTP_REQUEST_ERROR;
            goto exit;
        }
    } else {
        ares = ASTARTE_RESULT_HTTP_REQUEST_ERROR;
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(esp_err));
    }

exit:
    free(auth_header);
    if (client) {
        esp_http_client_cleanup(client);
    }

    return ares;
}

/************************************************
 *         Static functions definitions         *
 ***********************************************/
