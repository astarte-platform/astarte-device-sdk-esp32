/*
 * (C) Copyright 2018, Ispirata Srl, info@ispirata.com.
 *
 * SPDX-License-Identifier: LGPL-2.1+ OR Apache-2.0
 */

#include <astarte_device.h>

#include <astarte_credentials.h>
#include <astarte_hwid.h>
#include <astarte_pairing.h>

#include <mqtt_client.h>

#include <esp_log.h>

#define TAG "ASTARTE_DEVICE"

#define HWID_LENGTH 16
#define ENCODED_HWID_LENGTH 256
#define CREDENTIALS_SECRET_LENGTH 512
#define CSR_LENGTH 4096
#define CERT_LENGTH 4096
#define CN_LENGTH 512
#define PRIVKEY_LENGTH 8196
#define URL_LENGTH 512
#define INTROSPECTION_INTERFACE_LENGTH 512

struct astarte_device_t
{
    char *encoded_hwid;
    char *device_topic;
    char *introspection_string;
    esp_mqtt_client_handle_t mqtt_client;
};

static astarte_err_t retrieve_credentials(struct astarte_pairing_config *pairing_config);
static void send_introspection(astarte_device_handle_t device);
static void on_connected(astarte_device_handle_t device, int session_present);
static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event);

astarte_device_handle_t astarte_device_init()
{
    uint8_t hwid[HWID_LENGTH];
    astarte_hwid_get_id(hwid);
    char encoded_hwid[ENCODED_HWID_LENGTH];
    astarte_hwid_encode(encoded_hwid, ENCODED_HWID_LENGTH, hwid);
    ESP_LOGI(TAG, "hwid is: %s", encoded_hwid);

    astarte_err_t err = astarte_credentials_init();
    if (err != ASTARTE_OK) {
        ESP_LOGE(TAG, "Error in astarte_credentials_init");
        return NULL;
    }

    struct astarte_pairing_config pairing_config = {
        .base_url = CONFIG_ASTARTE_PAIRING_BASE_URL,
        .jwt = CONFIG_ASTARTE_PAIRING_JWT,
        .realm = CONFIG_ASTARTE_REALM,
        .hw_id = encoded_hwid,
    };
    char credentials_secret[CREDENTIALS_SECRET_LENGTH];
    err = astarte_pairing_get_credentials_secret(&pairing_config, credentials_secret, CREDENTIALS_SECRET_LENGTH);
    if (err != ASTARTE_OK) {
        ESP_LOGE(TAG, "Error in get_credentials_secret");
        return NULL;
    } else {
        ESP_LOGI(TAG, "credentials_secret is: %s", credentials_secret);
    }

    astarte_device_handle_t ret = NULL;
    char *client_cert_pem = NULL;
    char *client_cert_cn = NULL;
    char *broker_url = NULL;
    char *key_pem = NULL;

    if (!astarte_credentials_has_certificate() && retrieve_credentials(&pairing_config) != ASTARTE_OK) {
        goto init_failed;
    }

    key_pem = calloc(PRIVKEY_LENGTH, sizeof(char));
    if (!key_pem) {
        ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
        goto init_failed;
    }
    err = astarte_credentials_get_key(key_pem, PRIVKEY_LENGTH);
    if (err != ASTARTE_OK) {
        ESP_LOGE(TAG, "Error in get_key");
        goto init_failed;
    } else {
        ESP_LOGI(TAG, "Key is: %s", key_pem);
    }

    client_cert_pem = calloc(CERT_LENGTH, sizeof(char));
    if (!client_cert_pem) {
        ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
        goto init_failed;
    }
    err = astarte_credentials_get_certificate(client_cert_pem, CERT_LENGTH);
    if (err != ASTARTE_OK) {
        ESP_LOGE(TAG, "Error in get_certificate");
        goto init_failed;
    } else {
        ESP_LOGI(TAG, "Certificate is: %s", client_cert_pem);
    }

    client_cert_cn = calloc(CN_LENGTH, sizeof(char));
    if (!client_cert_cn) {
        ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
        goto init_failed;
    }
    err = astarte_credentials_get_certificate_common_name(client_cert_cn, CN_LENGTH);
    if (err != ASTARTE_OK) {
        ESP_LOGE(TAG, "Error in get_certificate_common_name");
        goto init_failed;
    } else {
        ESP_LOGI(TAG, "Device topic is: %s", client_cert_cn);
    }

    broker_url = calloc(URL_LENGTH, sizeof(char));
    if (!broker_url) {
        ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
        goto init_failed;
    }
    err = astarte_pairing_get_mqtt_v1_broker_url(&pairing_config, broker_url, URL_LENGTH);
    if (err != ASTARTE_OK) {
        ESP_LOGE(TAG, "Error in get_mqtt_v1_broker_url");
        goto init_failed;
    } else {
        ESP_LOGI(TAG, "Broker URL is: %s", broker_url);
    }

    ret = calloc(1, sizeof(struct astarte_device_t));
    if (!ret) {
        ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
        goto init_failed;
    }

    const esp_mqtt_client_config_t mqtt_cfg = {
        .uri = broker_url,
        .event_handle = mqtt_event_handler,
        .client_cert_pem = client_cert_pem,
        .client_key_pem = key_pem,
        .user_context = ret,
    };
    esp_mqtt_client_handle_t mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (!mqtt_client) {
        ESP_LOGE(TAG, "Error in esp_mqtt_client_init");
        goto init_failed;
    }
    ret->mqtt_client = mqtt_client;

    ret->encoded_hwid = strdup(encoded_hwid);
    if (!ret->encoded_hwid) {
        ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
        goto init_failed;
    }

    ret->device_topic = client_cert_cn;
    ret->introspection_string = NULL;

    return ret;

init_failed:
    free(ret);
    free(key_pem);
    free(client_cert_pem);
    free(client_cert_cn);
    free(broker_url);

    return NULL;
}

void astarte_device_destroy(astarte_device_handle_t device)
{
    if (!device) {
        return;
    }

    esp_mqtt_client_destroy(device->mqtt_client);
    free(device->encoded_hwid);
    free(device);
}

astarte_err_t astarte_device_add_interface(astarte_device_handle_t device, const char *interface_name, int major_version, int minor_version)
{
    char new_interface[INTROSPECTION_INTERFACE_LENGTH];
    snprintf(new_interface, INTROSPECTION_INTERFACE_LENGTH, "%s:%d:%d", interface_name, major_version, minor_version);

    if (!device->introspection_string) {
        device->introspection_string = strdup(new_interface);
        if (!device->introspection_string) {
            ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
            return ASTARTE_ERR;
        }
    } else {
        // + 2 for ; and terminator
        int len = strlen(device->introspection_string) + strlen(new_interface) + 2;
        char *new_introspection_string = calloc(1, len);
        if (!new_introspection_string) {
            ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
            return ASTARTE_ERR;
        }

        snprintf(new_introspection_string, len, "%s;%s", device->introspection_string, new_interface);
        free(device->introspection_string);
        device->introspection_string = new_introspection_string;
    }

    return ASTARTE_OK;
}

void astarte_device_start(astarte_device_handle_t device)
{
    esp_mqtt_client_start(device->mqtt_client);
}

static astarte_err_t retrieve_credentials(struct astarte_pairing_config *pairing_config)
{
    astarte_err_t ret = ASTARTE_ERR;
    char *cert_pem = NULL;
    char *csr = calloc(CSR_LENGTH, sizeof(char));
    if (!csr) {
        ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
        goto exit;
    }
    ret = astarte_credentials_get_csr(csr, CSR_LENGTH);
    if (ret != ASTARTE_OK) {
        ESP_LOGE(TAG, "Error in get_csr");
        goto exit;
    }

    cert_pem = calloc(CERT_LENGTH, sizeof(char));
    if (!cert_pem) {
        ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
        goto exit;
    }
    ret = astarte_pairing_get_mqtt_v1_credentials(pairing_config, csr, cert_pem, CERT_LENGTH);
    if (ret != ASTARTE_OK) {
        ESP_LOGE(TAG, "Error in get_mqtt_v1_credentials");
        goto exit;
    } else {
        ESP_LOGI(TAG, "Got credentials");
    }

    ret = astarte_credentials_save_certificate(cert_pem);
    if (ret != ASTARTE_OK) {
        ESP_LOGE(TAG, "Error in get_mqtt_v1_credentials");
        goto exit;
    } else {
        ESP_LOGI(TAG, "Certificate saved");
    }

    ret = ASTARTE_OK;

exit:
    free(csr);
    free(cert_pem);
    return ret;
}

static void send_introspection(astarte_device_handle_t device)
{
    if (!device->introspection_string) {
        ESP_LOGE(TAG, "NULL introspection_string in send_introspection");
        return;
    }

    if (!device->mqtt_client) {
        ESP_LOGE(TAG, "NULL mqtt_client in send_introspection");
        return;
    }

    if (!device->device_topic) {
        ESP_LOGE(TAG, "NULL device_topic in send_introspection");
        return;
    }

    esp_mqtt_client_handle_t mqtt = device->mqtt_client;
    int len = strlen(device->introspection_string);
    ESP_LOGI(TAG, "Publishing introspection: %s", device->introspection_string);
    esp_mqtt_client_publish(mqtt, device->device_topic, device->introspection_string, len, 2, 0);
}

static void on_connected(astarte_device_handle_t device, int session_present)
{
    if (session_present) {
        return;
    }

    send_introspection(device);
}

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    astarte_device_handle_t device = (astarte_device_handle_t) event->user_context;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            // Session present is always false for now
            int session_present = 0;
            on_connected(device, session_present);
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            break;
    }
    return ESP_OK;
}
