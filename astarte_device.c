/*
 * (C) Copyright 2018, Ispirata Srl, info@ispirata.com.
 *
 * SPDX-License-Identifier: LGPL-2.1+ OR Apache-2.0
 */

#include <astarte_device.h>

#include <astarte_bson.h>
#include <astarte_bson_serializer.h>
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
#define TOPIC_LENGTH 512

struct astarte_device_t
{
    char *encoded_hwid;
    char *device_topic;
    int device_topic_len;
    char *introspection_string;
    astarte_device_data_event_callback_t data_event_callback;
    esp_mqtt_client_handle_t mqtt_client;
};

static astarte_err_t retrieve_credentials(struct astarte_pairing_config *pairing_config);
static astarte_err_t check_device(astarte_device_handle_t device);
astarte_err_t publish_bson(astarte_device_handle_t device, const char *interface_name, const char *path,
                           const struct astarte_bson_serializer_t *bs, int qos);
static void setup_subscriptions(astarte_device_handle_t device);
static void send_introspection(astarte_device_handle_t device);
static void on_connected(astarte_device_handle_t device, int session_present);
static void on_incoming(astarte_device_handle_t device, char *topic, int topic_len, char *data, int data_len);
static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event);

astarte_device_handle_t astarte_device_init(astarte_device_config_t *cfg)
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
    ret->device_topic_len = strlen(client_cert_cn);
    ret->introspection_string = NULL;
    ret->data_event_callback = cfg->data_event_callback;

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

astarte_err_t publish_bson(astarte_device_handle_t device, const char *interface_name, const char *path,
                           const struct astarte_bson_serializer_t *bs, int qos)
{
    if (path[0] != '/') {
        ESP_LOGE(TAG, "Invalid path: %s (must be start with /)", path);
        return ASTARTE_ERR;
    }

    if (qos < 0 || qos > 2) {
        ESP_LOGE(TAG, "Invalid QoS: %d (must be 0, 1 or 2)", qos);
        return ASTARTE_ERR;
    }

    esp_mqtt_client_handle_t mqtt = device->mqtt_client;

    int len;
    const void *data = astarte_bson_serializer_get_document(bs, &len);
    if (!data) {
        ESP_LOGE(TAG, "Error during BSON serialization");
        return ASTARTE_ERR;
    }

    char topic[TOPIC_LENGTH];
    snprintf(topic, TOPIC_LENGTH, "%s/%s%s", device->device_topic, interface_name, path);

    ESP_LOGI(TAG, "Publishing on %s with QoS %d", topic, qos);
    int ret = esp_mqtt_client_publish(mqtt, topic, data, len, qos, 0);
    if (ret < 0) {
        ESP_LOGE(TAG, "Publish on %s failed", topic);
        return ASTARTE_ERR;
    }

    ESP_LOGI(TAG, "Publish succeeded, msg_id: %d", ret);
    return ASTARTE_OK;
}

astarte_err_t astarte_device_stream_double(astarte_device_handle_t device, const char *interface_name, const char *path, double value, int qos)
{
    struct astarte_bson_serializer_t bs;
    astarte_bson_serializer_init(&bs);
    astarte_bson_serializer_append_double(&bs, "v", value);
    astarte_bson_serializer_append_end_of_document(&bs);

    astarte_err_t exit_code = publish_bson(device, interface_name, path, &bs, qos);

    astarte_bson_serializer_destroy(&bs);
    return exit_code;
}

astarte_err_t astarte_device_stream_integer(astarte_device_handle_t device, const char *interface_name, const char *path, int32_t value, int qos)
{
    struct astarte_bson_serializer_t bs;
    astarte_bson_serializer_init(&bs);
    astarte_bson_serializer_append_int32(&bs, "v", value);
    astarte_bson_serializer_append_end_of_document(&bs);

    astarte_err_t exit_code = publish_bson(device, interface_name, path, &bs, qos);

    astarte_bson_serializer_destroy(&bs);
    return exit_code;
}

astarte_err_t astarte_device_stream_longinteger(astarte_device_handle_t device, const char *interface_name, const char *path, int64_t value, int qos)
{
    struct astarte_bson_serializer_t bs;
    astarte_bson_serializer_init(&bs);
    astarte_bson_serializer_append_int64(&bs, "v", value);
    astarte_bson_serializer_append_end_of_document(&bs);

    astarte_err_t exit_code = publish_bson(device, interface_name, path, &bs, qos);

    astarte_bson_serializer_destroy(&bs);
    return exit_code;
}

astarte_err_t astarte_device_stream_boolean(astarte_device_handle_t device, const char *interface_name, const char *path, int value, int qos)
{
    struct astarte_bson_serializer_t bs;
    astarte_bson_serializer_init(&bs);
    astarte_bson_serializer_append_boolean(&bs, "v", value);
    astarte_bson_serializer_append_end_of_document(&bs);

    astarte_err_t exit_code = publish_bson(device, interface_name, path, &bs, qos);

    astarte_bson_serializer_destroy(&bs);
    return exit_code;
}

astarte_err_t astarte_device_stream_string(astarte_device_handle_t device, const char *interface_name, const char *path, char *value, int qos)
{
    struct astarte_bson_serializer_t bs;
    astarte_bson_serializer_init(&bs);
    astarte_bson_serializer_append_string(&bs, "v", value);
    astarte_bson_serializer_append_end_of_document(&bs);

    astarte_err_t exit_code = publish_bson(device, interface_name, path, &bs, qos);

    astarte_bson_serializer_destroy(&bs);
    return exit_code;
}

astarte_err_t astarte_device_stream_binaryblob(astarte_device_handle_t device, const char *interface_name, const char *path, void *value,
                                               size_t size, int qos)
{
    struct astarte_bson_serializer_t bs;
    astarte_bson_serializer_init(&bs);
    astarte_bson_serializer_append_binary(&bs, "v", value, size);
    astarte_bson_serializer_append_end_of_document(&bs);

    astarte_err_t exit_code = publish_bson(device, interface_name, path, &bs, qos);

    astarte_bson_serializer_destroy(&bs);
    return exit_code;
}

astarte_err_t astarte_device_stream_datetime(astarte_device_handle_t device, const char *interface_name, const char *path, int64_t value, int qos)
{
    struct astarte_bson_serializer_t bs;
    astarte_bson_serializer_init(&bs);
    astarte_bson_serializer_append_datetime(&bs, "v", value);
    astarte_bson_serializer_append_end_of_document(&bs);

    astarte_err_t exit_code = publish_bson(device, interface_name, path, &bs, qos);

    astarte_bson_serializer_destroy(&bs);
    return exit_code;
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

static astarte_err_t check_device(astarte_device_handle_t device)
{
    if (!device->introspection_string) {
        ESP_LOGE(TAG, "NULL introspection_string in send_introspection");
        return ASTARTE_ERR;
    }

    if (!device->mqtt_client) {
        ESP_LOGE(TAG, "NULL mqtt_client in send_introspection");
        return ASTARTE_ERR;
    }

    if (!device->device_topic) {
        ESP_LOGE(TAG, "NULL device_topic in send_introspection");
        return ASTARTE_ERR;
    }

    return ASTARTE_OK;
}

static void send_introspection(astarte_device_handle_t device)
{
    if (check_device(device) != ASTARTE_OK) {
        return;
    }

    esp_mqtt_client_handle_t mqtt = device->mqtt_client;
    int len = strlen(device->introspection_string);
    ESP_LOGI(TAG, "Publishing introspection: %s", device->introspection_string);
    esp_mqtt_client_publish(mqtt, device->device_topic, device->introspection_string, len, 2, 0);
}

static void setup_subscriptions(astarte_device_handle_t device)
{
    if (check_device(device) != ASTARTE_OK) {
        return;
    }

    char topic[TOPIC_LENGTH];

    esp_mqtt_client_handle_t mqtt = device->mqtt_client;

    // TODO: should we just subscribe to device_topic/#?
    // Subscribe to control messages
    snprintf(topic, TOPIC_LENGTH, "%s/control/#", device->device_topic);
    ESP_LOGI(TAG, "Subscribing to %s", topic);
    esp_mqtt_client_subscribe(mqtt, topic, 2);

    char *interface_name_begin = device->introspection_string;
    char *interface_name_end = strchr(device->introspection_string, ':');
    int interface_name_len = interface_name_end - interface_name_begin;
    while (interface_name_begin != NULL) {
        // Subscribe to interface root topic
        snprintf(topic, TOPIC_LENGTH, "%s/%.*s", device->device_topic, interface_name_len, interface_name_begin);
        ESP_LOGI(TAG, "Subscribing to %s", topic);
        esp_mqtt_client_subscribe(mqtt, topic, 2);

        // Subscribe to all interface subtopics
        snprintf(topic, TOPIC_LENGTH, "%s/%.*s/#", device->device_topic, interface_name_len, interface_name_begin);
        ESP_LOGI(TAG, "Subscribing to %s", topic);
        esp_mqtt_client_subscribe(mqtt, topic, 2);

        interface_name_begin = strchr(interface_name_begin, ';');
        if (!interface_name_begin) {
            break;
        }
        // interface_name begins the character after ;
        ++interface_name_begin;
        interface_name_end = strchr(interface_name_begin, ':');
        interface_name_len = interface_name_end - interface_name_begin;
    }
}

static void on_connected(astarte_device_handle_t device, int session_present)
{
    if (session_present) {
        return;
    }

    setup_subscriptions(device);
    send_introspection(device);
}

static void on_incoming(astarte_device_handle_t device, char *topic, int topic_len, char *data, int data_len)
{
    if (check_device(device) != ASTARTE_OK) {
        return;
    }

    if (!device->data_event_callback) {
        ESP_LOGE(TAG, "data_event_callback not set");
        return;
    }

    if (strstr(topic, device->device_topic) != topic) {
        ESP_LOGE(TAG, "Incoming message topic doesn't begin with device_topic: %s", topic);
        return;
    }

    char control_prefix[512];
    snprintf(control_prefix, 512, "%s/control", device->device_topic);
    int control_prefix_len = strlen(control_prefix);
    if (strstr(topic, control_prefix)) {
        // Control message
        char *control_topic = topic + control_prefix_len;
        ESP_LOGI(TAG, "Received control message on control topic %s", control_topic);
        // TODO: on_control_message(device, control_topic, data, data_len);
        return;
    }

    // Data message
    if (topic_len < device->device_topic_len + strlen("/") || topic[device->device_topic_len] != '/') {
        ESP_LOGE(TAG, "No / after device_topic, can't find interface: %s", topic);
        return;
    }

    char *interface_name_begin = topic + device->device_topic_len + strlen("/");
    char *path_begin = strchr(interface_name_begin, '/');
    if (!path_begin) {
        ESP_LOGE(TAG, "No / after interface_name, can't find path: %s", topic);
        return;
    }

    int interface_name_len = path_begin - interface_name_begin;
    char interface_name[512];
    snprintf(interface_name, 512, "%.*s", interface_name_len, interface_name_begin);

    int path_len = topic_len - device->device_topic_len - strlen("/") - interface_name_len;
    char path[512];
    snprintf(path, 512, "%.*s", path_len, path_begin);

    uint8_t bson_value_type;
    const void *bson_value = astarte_bson_key_lookup("v", data, &bson_value_type);
    if (!bson_value) {
        ESP_LOGE(TAG, "Cannot retrieve BSON value from data");
        return;
    }

    astarte_device_data_event_t event = {
        .device = device,
        .interface_name = interface_name,
        .path = path,
        .bson_value = bson_value,
        .bson_value_type = bson_value_type,
    };

    device->data_event_callback(&event);
}

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    astarte_device_handle_t device = (astarte_device_handle_t) event->user_context;
    switch (event->event_id) {
        case MQTT_EVENT_BEFORE_CONNECT:
            ESP_LOGI(TAG, "MQTT_EVENT_BEFORE_CONNECT");
            break;

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
            on_incoming(device, event->topic, event->topic_len, event->data, event->data_len);
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            break;
    }
    return ESP_OK;
}
