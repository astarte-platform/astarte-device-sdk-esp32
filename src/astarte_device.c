/*
 * (C) Copyright 2018-2023, SECO Mind Srl
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later OR Apache-2.0
 */

#include <astarte_device.h>

#include <astarte_bson.h>
#include <astarte_bson_serializer.h>
#include <astarte_credentials.h>
#include <astarte_hwid.h>
#include <astarte_linked_list.h>
#include <astarte_list.h>
#include <astarte_pairing.h>
#include <astarte_set.h>
#include <astarte_storage.h>
#include <astarte_zlib.h>

#include <mqtt_client.h>

#include <esp_http_client.h>
#if CONFIG_MBEDTLS_CERTIFICATE_BUNDLE && (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 3, 0))
#include <esp_crt_bundle.h>
#endif
#include <esp_log.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

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
#define REINIT_RETRY_INTERVAL_MS (30 * 1000)

#define NOTIFY_TERMINATE (1U << 0U)
#define NOTIFY_REINIT (1U << 1U)

struct astarte_device
{
    char *encoded_hwid;
    char *credentials_secret;
    char *device_topic;
    size_t device_topic_len;
    char *client_cert_pem;
    char *key_pem;
    bool connected;
    astarte_device_data_event_callback_t data_event_callback;
    astarte_device_unset_event_callback_t unset_event_callback;
    astarte_device_connection_event_callback_t connection_event_callback;
    astarte_device_disconnection_event_callback_t disconnection_event_callback;
    void *callbacks_user_data;
    esp_mqtt_client_handle_t mqtt_client;
    TaskHandle_t reinit_task_handle;
    SemaphoreHandle_t reinit_mutex;
    astarte_list_head_t interfaces_list;
    char *realm;
};

static void astarte_device_reinit_task(void *ctx);
static astarte_err_t astarte_device_init_connection(
    astarte_device_handle_t device, const char *encoded_hwid, const char *realm);
static astarte_err_t retrieve_credentials(astarte_pairing_config_t *pairing_config);
static astarte_err_t check_device(astarte_device_handle_t device);
static astarte_err_t publish_bson(astarte_device_handle_t device, const char *interface_name,
    const char *path, astarte_bson_serializer_handle_t bson, int qos);
static astarte_err_t publish_data(astarte_device_handle_t device, const char *interface_name,
    const char *path, const void *data, int length, int qos);
static void setup_subscriptions(astarte_device_handle_t device);
static void send_introspection(astarte_device_handle_t device);
static void send_emptycache(astarte_device_handle_t device);
static void send_device_owned_properties(astarte_device_handle_t device);
static void send_purge_device_properties(astarte_device_handle_t device,
    astarte_set_handle_t properties_set_handle);
static void on_connected(astarte_device_handle_t device, int session_present);
static void on_disconnected(astarte_device_handle_t device);
static void on_incoming(
    astarte_device_handle_t device, char *topic, int topic_len, char *data, int data_len);
static void on_control_message(astarte_device_handle_t device, char *control_topic, char *data, int data_len);
static void on_purge_properties(astarte_device_handle_t device, char *data, int data_len);
static void on_certificate_error(astarte_device_handle_t device);
static void mqtt_event_handler(
    void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
static int has_connectivity();
static void maybe_append_timestamp(astarte_bson_serializer_handle_t bson, uint64_t ts_epoch_millis);
static bool interface_is_in_introspection(
    astarte_device_handle_t device, const char *interface_name);
static bool interface_is_property(astarte_device_handle_t device, const char *interface_name);
static bool interface_is_device_owned(astarte_device_handle_t device, const char *interface_name);

astarte_device_handle_t astarte_device_init(astarte_device_config_t *cfg)
{
    astarte_device_handle_t ret = calloc(1, sizeof(struct astarte_device));
    if (!ret) {
        ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
        return NULL;
    }

    ret->reinit_mutex = xSemaphoreCreateMutex();
    if (!ret->reinit_mutex) {
        ESP_LOGE(TAG, "Cannot create reinit_mutex");
        goto init_failed;
    }

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 3, 0)
    const configSTACK_DEPTH_TYPE stack_depth = 6000;
#else
    const uint32_t stack_depth = 6000;
#endif
    xTaskCreate(astarte_device_reinit_task, "astarte_device_reinit_task", stack_depth, ret,
        tskIDLE_PRIORITY, &ret->reinit_task_handle);
    if (!ret->reinit_task_handle) {
        ESP_LOGE(TAG, "Cannot start astarte_device_reinit_task");
        goto init_failed;
    }

    const char *encoded_hwid = NULL;
    if (cfg->hwid) {
        encoded_hwid = cfg->hwid;
    } else {
        uint8_t generated_hwid[HWID_LENGTH];
        astarte_err_t hwid_err = astarte_hwid_get_id(generated_hwid);
        if (hwid_err != ASTARTE_OK) {
            ESP_LOGE(TAG, "Cannot get device HWID: %d", hwid_err);
            goto init_failed;
        }
        char generated_encoded_hwid[ENCODED_HWID_LENGTH];
        hwid_err = astarte_hwid_encode(generated_encoded_hwid, ENCODED_HWID_LENGTH, generated_hwid);
        if (hwid_err != ASTARTE_OK) {
            ESP_LOGE(TAG, "Cannot encode device HWID: %d", hwid_err);
            goto init_failed;
        }
        encoded_hwid = generated_encoded_hwid;
    }

    ESP_LOGD(TAG, "hwid is: %s", encoded_hwid);

    if (cfg->credentials_secret) {
        ret->credentials_secret = strdup(cfg->credentials_secret);
    }

    const char *realm = NULL;
    if (cfg->realm) {
        realm = cfg->realm;
    } else {
        realm = CONFIG_ASTARTE_REALM;
    }

    astarte_err_t res = astarte_device_init_connection(ret, encoded_hwid, realm);
    if (res != ASTARTE_OK) {
        ESP_LOGE(TAG, "Cannot init Astarte device: %d", res);
        goto init_failed;
    }

    ret->encoded_hwid = strdup(encoded_hwid);
    if (!ret->encoded_hwid) {
        ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
        goto init_failed;
    }

    ret->realm = strdup(realm);
    if (!ret->realm) {
        ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
        goto init_failed;
    }

    astarte_list_init(&ret->interfaces_list);
    ret->data_event_callback = cfg->data_event_callback;
    ret->unset_event_callback = cfg->unset_event_callback;
    ret->connection_event_callback = cfg->connection_event_callback;
    ret->disconnection_event_callback = cfg->disconnection_event_callback;
    ret->callbacks_user_data = cfg->callbacks_user_data;

    return ret;

init_failed:
    if (ret->credentials_secret) {
        free(ret->credentials_secret);
    }

    if (ret->reinit_mutex) {
        vSemaphoreDelete(ret->reinit_mutex);
    }

    if (ret->reinit_task_handle) {
        xTaskNotify(ret->reinit_task_handle, NOTIFY_TERMINATE, eSetBits);
    }

    free(ret->encoded_hwid);
    free(ret->realm);
    free(ret);

    return NULL;
}

static void astarte_device_reinit_task(void *ctx)
{
    // This task will just wait for a notification and if it receives one, it will
    // reinit the device. This is necessary to handle the device certificate expiration,
    // that can't be handled in the event callback since that's executed in the mqtt
    // client task, that gets stopped to create a new mqtt client with the new certificate.

    astarte_device_handle_t device = (astarte_device_handle_t) ctx;

    while (1) {
        uint32_t notification_value = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        if (notification_value & NOTIFY_TERMINATE) {
            // Terminate the task
            vTaskDelete(NULL);
        } else if (notification_value & NOTIFY_REINIT) {
            xSemaphoreTake(device->reinit_mutex, portMAX_DELAY);
            ESP_LOGI(TAG, "Reinitializing the device");
            // Delete the old certificate
            astarte_credentials_delete_certificate();
            // Retry until we succeed
            bool reinitialized = true;

            while (1) {
                astarte_err_t res
                    = astarte_device_init_connection(device, device->encoded_hwid, device->realm);
                if (res == ASTARTE_OK) {
                    break;
                }

                ESP_LOGE(TAG,
                    "Cannot reinit Astarte device: %d, trying again in %d "
                    "milliseconds",
                    res, REINIT_RETRY_INTERVAL_MS);
                vTaskDelay(REINIT_RETRY_INTERVAL_MS / portTICK_PERIOD_MS);

                // We check if the device got connected again. If it has, then we
                // can break away from the reinit process, since it was a false positive.
                // We deleted the certificate but the device will just ask for a new one
                // the next time it boots.
                if (device->connected) {
                    ESP_LOGI(TAG, "Device reconnected, skipping device reinitialization");
                    reinitialized = false;
                    break;
                }
            }

            if (reinitialized) {
                ESP_LOGI(TAG, "Device reinitialized, starting it again");
                esp_mqtt_client_start(device->mqtt_client);
            }

            xSemaphoreGive(device->reinit_mutex);
        }
    }
}

astarte_err_t astarte_device_init_connection(
    astarte_device_handle_t device, const char *encoded_hwid, const char *realm)
{
    if (!astarte_credentials_is_initialized()) {
        // TODO: this should be manually called from main before initializing the device,
        // but we just print a warning to maintain backwards compatibility for now
        ESP_LOGW(TAG,
            "You should manually call astarte_credentials_init before calling "
            "astarte_device_init");
        astarte_err_t err = astarte_credentials_init();
        if (err != ASTARTE_OK) {
            ESP_LOGE(TAG, "Error in astarte_credentials_init");
            return err;
        }
    }

    // If the device was already initialized, we free some resources first
    if (device->mqtt_client) {
        esp_mqtt_client_destroy(device->mqtt_client);
    }

    if (device->device_topic) {
        free(device->device_topic);
    }

    if (device->client_cert_pem) {
        free(device->client_cert_pem);
    }

    if (device->key_pem) {
        free(device->key_pem);
    }

    astarte_pairing_config_t pairing_config = {
        .base_url = CONFIG_ASTARTE_PAIRING_BASE_URL,
        .jwt = CONFIG_ASTARTE_PAIRING_JWT,
        .realm = realm,
        .hw_id = encoded_hwid,
    };

    if (device->credentials_secret) {
        pairing_config.credentials_secret = device->credentials_secret;
    }

    char credentials_secret[CREDENTIALS_SECRET_LENGTH];
    astarte_err_t err = astarte_pairing_get_credentials_secret(
        &pairing_config, credentials_secret, CREDENTIALS_SECRET_LENGTH);
    if (err != ASTARTE_OK) {
        ESP_LOGE(TAG, "Error in get_credentials_secret");
        return err;
    }
    ESP_LOGD(TAG, "credentials_secret is: %s", credentials_secret);

    char *client_cert_pem = NULL;
    char *client_cert_cn = NULL;
    char *key_pem = NULL;

    if (!astarte_credentials_has_certificate()) {
        err = retrieve_credentials(&pairing_config);
        if (err != ASTARTE_OK) {
            ESP_LOGE(TAG, "Could not retrieve credentials");
            goto init_failed;
        }
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
        ESP_LOGD(TAG, "Key is: %s", key_pem);
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
        ESP_LOGD(TAG, "Certificate is: %s", client_cert_pem);
    }

    client_cert_cn = calloc(CN_LENGTH, sizeof(char));
    if (!client_cert_cn) {
        ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
        goto init_failed;
    }
    err = astarte_credentials_get_certificate_common_name(
        client_cert_pem, client_cert_cn, CN_LENGTH);
    if (err != ASTARTE_OK) {
        ESP_LOGE(TAG, "Error in get_certificate_common_name");
        goto init_failed;
    } else {
        ESP_LOGD(TAG, "Device topic is: %s", client_cert_cn);
    }

    char broker_url[URL_LENGTH];
    err = astarte_pairing_get_mqtt_v1_broker_url(&pairing_config, broker_url, URL_LENGTH);
    if (err != ASTARTE_OK) {
        ESP_LOGE(TAG, "Error in get_mqtt_v1_broker_url");
        goto init_failed;
    } else {
        ESP_LOGD(TAG, "Broker URL is: %s", broker_url);
    }

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    const esp_mqtt_client_config_t mqtt_cfg
        = {.broker.address.uri = broker_url,
#if CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
              .broker.verification.crt_bundle_attach = esp_crt_bundle_attach,
#endif
              .credentials.authentication.certificate = client_cert_pem,
              .credentials.authentication.key = key_pem,
          };
#else
    const esp_mqtt_client_config_t mqtt_cfg
        = {.uri = broker_url,
#if CONFIG_MBEDTLS_CERTIFICATE_BUNDLE && (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 3, 0))
              .crt_bundle_attach = esp_crt_bundle_attach,
#endif
              .client_cert_pem = client_cert_pem,
              .client_key_pem = key_pem,
              .user_context = device,
              // TODO: At this time, the device starts with a clean session every connection,
              //  to change this behavior, it is necessary to enable disable_clean_session.
              //  Before enable it pay attention if the #issue-29 has been solved
              //  (see: https://github.com/astarte-platform/astarte-device-sdk-esp32/issues/29).
              //  .disable_clean_session = true,
          };
#endif

    esp_mqtt_client_handle_t mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (!mqtt_client) {
        ESP_LOGE(TAG, "Error in esp_mqtt_client_init");
        goto init_failed;
    }

    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, device);

    device->mqtt_client = mqtt_client;
    device->device_topic = client_cert_cn;
    device->device_topic_len = strlen(client_cert_cn);
    device->client_cert_pem = client_cert_pem;
    device->key_pem = key_pem;

    return ASTARTE_OK;

init_failed:
    free(key_pem);
    free(client_cert_pem);
    free(client_cert_cn);

    return err;
}

void astarte_device_destroy(astarte_device_handle_t device)
{
    if (!device) {
        return;
    }

    // Avoid destroying a device that is being reinitialized
    xSemaphoreTake(device->reinit_mutex, portMAX_DELAY);

    esp_mqtt_client_destroy(device->mqtt_client);
    xTaskNotify(device->reinit_task_handle, NOTIFY_TERMINATE, eSetBits);
    vSemaphoreDelete(device->reinit_mutex);
    free(device->device_topic);
    free(device->client_cert_pem);
    free(device->key_pem);
    free(device->encoded_hwid);
    free(device->credentials_secret);
    free(device->realm);
    astarte_list_head_t *item = NULL;
    astarte_list_head_t *tmp = NULL;
    MUTABLE_LIST_FOR_EACH(item, tmp, &device->interfaces_list)
    {
        astarte_ptr_list_entry_t *entry = GET_LIST_ENTRY(item, astarte_ptr_list_entry_t, head);
        free(entry);
    }
    free(device);
}

astarte_err_t astarte_device_add_interface(
    astarte_device_handle_t device, const astarte_interface_t *interface)
{
    if (xSemaphoreTake(device->reinit_mutex, (TickType_t) 10) == pdFALSE) {
        ESP_LOGE(TAG, "Trying to add an interface to a device that is being reinitialized");
        return ASTARTE_ERR;
    }

    if (interface->major_version == 0 && interface->minor_version == 0) {
        ESP_LOGE(TAG, "Trying to add an interface with both major and minor version equal 0");
        return ASTARTE_ERR_INVALID_INTERFACE_VERSION;
    }

    astarte_ptr_list_entry_t *entry = calloc(1, sizeof(astarte_ptr_list_entry_t));
    if (!entry) {
        ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
        xSemaphoreGive(device->reinit_mutex);
        return ASTARTE_ERR_OUT_OF_MEMORY;
    }

    entry->value = interface;

    astarte_list_append(&device->interfaces_list, &entry->head);

    xSemaphoreGive(device->reinit_mutex);
    return ASTARTE_OK;
}

astarte_err_t astarte_device_start(astarte_device_handle_t device)
{
    if (xSemaphoreTake(device->reinit_mutex, (TickType_t) 10) == pdFALSE) {
        ESP_LOGE(TAG, "Trying to start device that is being reinitialized");
        return ASTARTE_ERR_DEVICE_NOT_READY;
    }

    astarte_err_t ret = ASTARTE_OK;

    esp_err_t err = esp_mqtt_client_start(device->mqtt_client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start MQTT client: %s", esp_err_to_name(err));
        ret = ASTARTE_ERR;
    }

    xSemaphoreGive(device->reinit_mutex);

    return ret;
}

astarte_err_t astarte_device_stop(astarte_device_handle_t device)
{
    if (xSemaphoreTake(device->reinit_mutex, (TickType_t) 10) == pdFALSE) {
        ESP_LOGE(TAG, "Trying to stop device that is being reinitialized");
        return ASTARTE_ERR_DEVICE_NOT_READY;
    }

    astarte_err_t ret = ASTARTE_OK;

    esp_err_t err = esp_mqtt_client_stop(device->mqtt_client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to stop MQTT client: %s", esp_err_to_name(err));
        ret = ASTARTE_ERR;
    }

    xSemaphoreGive(device->reinit_mutex);

    if (ret == ASTARTE_OK) {
        // If we succesfully disconnected, call on_disconnected since
        // MQTT_EVENT_DISCONNECTED is not triggered for manual disconnections
        on_disconnected(device);
    }

    return ret;
}

static astarte_err_t publish_bson(astarte_device_handle_t device, const char *interface_name,
    const char *path, astarte_bson_serializer_handle_t bson, int qos)
{
    int len = 0;
    const void *data = astarte_bson_serializer_get_document(bson, &len);
    if (!data) {
        ESP_LOGE(TAG, "Error during BSON serialization");
        return ASTARTE_ERR;
    }
    if (len < 0) {
        ESP_LOGE(TAG, "BSON document is too long for MQTT publish.");
        ESP_LOGE(TAG, "Interface: %s, path: %s", interface_name, path);
        return ASTARTE_ERR;
    }

    if (interface_is_property(device, interface_name)) {
        astarte_storage_handle_t astarte_storage_handle;
        astarte_storage_open(&astarte_storage_handle);
        astarte_storage_store_property(astarte_storage_handle, interface_name, path, data, len);
        astarte_storage_close(astarte_storage_handle);
    }

    return publish_data(device, interface_name, path, data, len, qos);
}

static astarte_err_t publish_data(astarte_device_handle_t device, const char *interface_name,
    const char *path, const void *data, int length, int qos)
{
    if (path[0] != '/') {
        ESP_LOGE(TAG, "Invalid path: %s (must be start with /)", path);
        return ASTARTE_ERR_INVALID_INTERFACE_PATH;
    }

    if (qos < 0 || qos > 2) {
        ESP_LOGE(TAG, "Invalid QoS: %d (must be 0, 1 or 2)", qos);
        return ASTARTE_ERR_INVALID_QOS;
    }

    char topic[TOPIC_LENGTH];
    int print_ret
        = snprintf(topic, TOPIC_LENGTH, "%s/%s%s", device->device_topic, interface_name, path);
    if ((print_ret < 0) || (print_ret >= TOPIC_LENGTH)) {
        ESP_LOGE(TAG, "Error encoding topic");
        return ASTARTE_ERR;
    }

    if (xSemaphoreTake(device->reinit_mutex, (TickType_t) 10) == pdFALSE) {
        ESP_LOGE(TAG, "Trying to publish to a device that is being reinitialized");
        return ASTARTE_ERR_DEVICE_NOT_READY;
    }

    esp_mqtt_client_handle_t mqtt = device->mqtt_client;

    ESP_LOGD(TAG, "Publishing on %s with QoS %d", topic, qos);
    int ret = esp_mqtt_client_publish(mqtt, topic, data, length, qos, 0);
    xSemaphoreGive(device->reinit_mutex);
    if (ret < 0) {
        ESP_LOGE(TAG, "Publish on %s failed", topic);
        return ASTARTE_ERR_PUBLISH;
    }

    ESP_LOGD(TAG, "Publish succeeded, msg_id: %d", ret);
    return ASTARTE_OK;
}

static void maybe_append_timestamp(astarte_bson_serializer_handle_t bson, uint64_t ts_epoch_millis)
{
    if (ts_epoch_millis != ASTARTE_INVALID_TIMESTAMP) {
        astarte_bson_serializer_append_datetime(bson, "t", ts_epoch_millis);
    }
}

astarte_err_t astarte_device_stream_double_with_timestamp(astarte_device_handle_t device,
    const char *interface_name, const char *path, double value, uint64_t ts_epoch_millis, int qos)
{
    astarte_bson_serializer_handle_t bson = astarte_bson_serializer_new();
    astarte_bson_serializer_append_double(bson, "v", value);
    maybe_append_timestamp(bson, ts_epoch_millis);
    astarte_bson_serializer_append_end_of_document(bson);

    astarte_err_t exit_code = publish_bson(device, interface_name, path, bson, qos);

    astarte_bson_serializer_destroy(bson);
    return exit_code;
}

astarte_err_t astarte_device_stream_integer_with_timestamp(astarte_device_handle_t device,
    const char *interface_name, const char *path, int32_t value, uint64_t ts_epoch_millis, int qos)
{
    astarte_bson_serializer_handle_t bson = astarte_bson_serializer_new();
    astarte_bson_serializer_append_int32(bson, "v", value);
    maybe_append_timestamp(bson, ts_epoch_millis);
    astarte_bson_serializer_append_end_of_document(bson);

    astarte_err_t exit_code = publish_bson(device, interface_name, path, bson, qos);

    astarte_bson_serializer_destroy(bson);
    return exit_code;
}

astarte_err_t astarte_device_stream_longinteger_with_timestamp(astarte_device_handle_t device,
    const char *interface_name, const char *path, int64_t value, uint64_t ts_epoch_millis, int qos)
{
    astarte_bson_serializer_handle_t bson = astarte_bson_serializer_new();
    astarte_bson_serializer_append_int64(bson, "v", value);
    maybe_append_timestamp(bson, ts_epoch_millis);
    astarte_bson_serializer_append_end_of_document(bson);

    astarte_err_t exit_code = publish_bson(device, interface_name, path, bson, qos);

    astarte_bson_serializer_destroy(bson);
    return exit_code;
}

astarte_err_t astarte_device_stream_boolean_with_timestamp(astarte_device_handle_t device,
    const char *interface_name, const char *path, bool value, uint64_t ts_epoch_millis, int qos)
{
    astarte_bson_serializer_handle_t bson = astarte_bson_serializer_new();
    astarte_bson_serializer_append_boolean(bson, "v", value);
    maybe_append_timestamp(bson, ts_epoch_millis);
    astarte_bson_serializer_append_end_of_document(bson);

    astarte_err_t exit_code = publish_bson(device, interface_name, path, bson, qos);

    astarte_bson_serializer_destroy(bson);
    return exit_code;
}

astarte_err_t astarte_device_stream_string_with_timestamp(astarte_device_handle_t device,
    const char *interface_name, const char *path, const char *value, uint64_t ts_epoch_millis,
    int qos)
{
    astarte_bson_serializer_handle_t bson = astarte_bson_serializer_new();
    astarte_bson_serializer_append_string(bson, "v", value);
    maybe_append_timestamp(bson, ts_epoch_millis);
    astarte_bson_serializer_append_end_of_document(bson);

    astarte_err_t exit_code = publish_bson(device, interface_name, path, bson, qos);

    astarte_bson_serializer_destroy(bson);
    return exit_code;
}

astarte_err_t astarte_device_stream_binaryblob_with_timestamp(astarte_device_handle_t device,
    const char *interface_name, const char *path, void *value, size_t size,
    uint64_t ts_epoch_millis, int qos)
{
    astarte_bson_serializer_handle_t bson = astarte_bson_serializer_new();
    astarte_bson_serializer_append_binary(bson, "v", value, size);
    maybe_append_timestamp(bson, ts_epoch_millis);
    astarte_bson_serializer_append_end_of_document(bson);

    astarte_err_t exit_code = publish_bson(device, interface_name, path, bson, qos);

    astarte_bson_serializer_destroy(bson);
    return exit_code;
}

astarte_err_t astarte_device_stream_datetime_with_timestamp(astarte_device_handle_t device,
    const char *interface_name, const char *path, int64_t value, uint64_t ts_epoch_millis, int qos)
{
    astarte_bson_serializer_handle_t bson = astarte_bson_serializer_new();
    astarte_bson_serializer_append_datetime(bson, "v", value);
    maybe_append_timestamp(bson, ts_epoch_millis);
    astarte_bson_serializer_append_end_of_document(bson);

    astarte_err_t exit_code = publish_bson(device, interface_name, path, bson, qos);

    astarte_bson_serializer_destroy(bson);
    return exit_code;
}

#define IMPL_ASTARTE_DEVICE_STREAM_ARRAY_T_WITH_TIMESTAMP(TYPE, TYPE_NAME, BSON_TYPE_NAME)         \
    astarte_err_t astarte_device_stream_##TYPE_NAME##_with_timestamp(                              \
        astarte_device_handle_t device, const char *interface_name, const char *path, TYPE value,  \
        int count, uint64_t ts_epoch_millis, int qos)                                              \
    {                                                                                              \
        astarte_bson_serializer_handle_t bson = astarte_bson_serializer_new();                     \
        astarte_bson_serializer_append_##BSON_TYPE_NAME(bson, "v", value, count);                  \
        maybe_append_timestamp(bson, ts_epoch_millis);                                             \
        astarte_bson_serializer_append_end_of_document(bson);                                      \
                                                                                                   \
        astarte_err_t exit_code = publish_bson(device, interface_name, path, bson, qos);           \
                                                                                                   \
        astarte_bson_serializer_destroy(bson);                                                     \
        return exit_code;                                                                          \
    }

IMPL_ASTARTE_DEVICE_STREAM_ARRAY_T_WITH_TIMESTAMP(const double *, double_array, double_array)
IMPL_ASTARTE_DEVICE_STREAM_ARRAY_T_WITH_TIMESTAMP(const int32_t *, integer_array, int32_array)
IMPL_ASTARTE_DEVICE_STREAM_ARRAY_T_WITH_TIMESTAMP(const int64_t *, longinteger_array, int64_array)
IMPL_ASTARTE_DEVICE_STREAM_ARRAY_T_WITH_TIMESTAMP(const bool *, boolean_array, boolean_array)
IMPL_ASTARTE_DEVICE_STREAM_ARRAY_T_WITH_TIMESTAMP(const char *const *, string_array, string_array)
IMPL_ASTARTE_DEVICE_STREAM_ARRAY_T_WITH_TIMESTAMP(const int64_t *, datetime_array, datetime_array)

astarte_err_t astarte_device_stream_binaryblob_array_with_timestamp(astarte_device_handle_t device,
    const char *interface_name, const char *path, const void *const *values, const int *sizes,
    int count, uint64_t ts_epoch_millis, int qos)
{
    astarte_bson_serializer_handle_t bson = astarte_bson_serializer_new();
    astarte_err_t exit_code
        = astarte_bson_serializer_append_binary_array(bson, "v", values, sizes, count);
    maybe_append_timestamp(bson, ts_epoch_millis);
    astarte_bson_serializer_append_end_of_document(bson);

    if (exit_code == ASTARTE_OK) {
        exit_code = publish_bson(device, interface_name, path, bson, qos);
    }

    astarte_bson_serializer_destroy(bson);
    return exit_code;
}

astarte_err_t astarte_device_stream_aggregate_with_timestamp(astarte_device_handle_t device,
    const char *interface_name, const char *path_prefix, const void *bson_document,
    uint64_t ts_epoch_millis, int qos)
{
    astarte_bson_serializer_handle_t bson = astarte_bson_serializer_new();
    astarte_bson_serializer_append_document(bson, "v", bson_document);
    maybe_append_timestamp(bson, ts_epoch_millis);
    astarte_bson_serializer_append_end_of_document(bson);

    astarte_err_t exit_code = publish_bson(device, interface_name, path_prefix, bson, qos);

    astarte_bson_serializer_destroy(bson);
    return exit_code;
}

astarte_err_t astarte_device_stream_double(astarte_device_handle_t device,
    const char *interface_name, const char *path, double value, int qos)
{
    return astarte_device_stream_double_with_timestamp(
        device, interface_name, path, value, ASTARTE_INVALID_TIMESTAMP, qos);
}

astarte_err_t astarte_device_stream_integer(astarte_device_handle_t device,
    const char *interface_name, const char *path, int32_t value, int qos)
{
    return astarte_device_stream_integer_with_timestamp(
        device, interface_name, path, value, ASTARTE_INVALID_TIMESTAMP, qos);
}

astarte_err_t astarte_device_stream_longinteger(astarte_device_handle_t device,
    const char *interface_name, const char *path, int64_t value, int qos)
{
    return astarte_device_stream_longinteger_with_timestamp(
        device, interface_name, path, value, ASTARTE_INVALID_TIMESTAMP, qos);
}

astarte_err_t astarte_device_stream_boolean(astarte_device_handle_t device,
    const char *interface_name, const char *path, bool value, int qos)
{
    return astarte_device_stream_boolean_with_timestamp(
        device, interface_name, path, value, ASTARTE_INVALID_TIMESTAMP, qos);
}

astarte_err_t astarte_device_stream_string(astarte_device_handle_t device,
    const char *interface_name, const char *path, const char *value, int qos)
{
    return astarte_device_stream_string_with_timestamp(
        device, interface_name, path, value, ASTARTE_INVALID_TIMESTAMP, qos);
}

astarte_err_t astarte_device_stream_binaryblob(astarte_device_handle_t device,
    const char *interface_name, const char *path, void *value, size_t size, int qos)
{
    return astarte_device_stream_binaryblob_with_timestamp(
        device, interface_name, path, value, size, ASTARTE_INVALID_TIMESTAMP, qos);
}

astarte_err_t astarte_device_stream_datetime(astarte_device_handle_t device,
    const char *interface_name, const char *path, int64_t value, int qos)
{
    return astarte_device_stream_datetime_with_timestamp(
        device, interface_name, path, value, ASTARTE_INVALID_TIMESTAMP, qos);
}

astarte_err_t astarte_device_stream_aggregate(astarte_device_handle_t device,
    const char *interface_name, const char *path_prefix, const void *bson_document, int qos)
{
    return astarte_device_stream_aggregate_with_timestamp(
        device, interface_name, path_prefix, bson_document, ASTARTE_INVALID_TIMESTAMP, qos);
}

astarte_err_t astarte_device_set_double_property(
    astarte_device_handle_t device, const char *interface_name, const char *path, double value)
{
    return astarte_device_stream_double(device, interface_name, path, value, 2);
}

astarte_err_t astarte_device_set_integer_property(
    astarte_device_handle_t device, const char *interface_name, const char *path, int32_t value)
{
    return astarte_device_stream_integer(device, interface_name, path, value, 2);
}

astarte_err_t astarte_device_set_longinteger_property(
    astarte_device_handle_t device, const char *interface_name, const char *path, int64_t value)
{
    return astarte_device_stream_longinteger(device, interface_name, path, value, 2);
}

astarte_err_t astarte_device_set_boolean_property(
    astarte_device_handle_t device, const char *interface_name, const char *path, bool value)
{
    return astarte_device_stream_boolean(device, interface_name, path, value, 2);
}

astarte_err_t astarte_device_set_string_property(
    astarte_device_handle_t device, const char *interface_name, const char *path, const char *value)
{
    return astarte_device_stream_string(device, interface_name, path, value, 2);
}

astarte_err_t astarte_device_set_binaryblob_property(astarte_device_handle_t device,
    const char *interface_name, const char *path, void *value, size_t size)
{
    return astarte_device_stream_binaryblob(device, interface_name, path, value, size, 2);
}

astarte_err_t astarte_device_set_datetime_property(
    astarte_device_handle_t device, const char *interface_name, const char *path, int64_t value)
{
    return astarte_device_stream_datetime(device, interface_name, path, value, 2);
}

astarte_err_t astarte_device_unset_path(
    astarte_device_handle_t device, const char *interface_name, const char *path)
{
    return publish_data(device, interface_name, path, "", 0, 2);
}

bool astarte_device_is_connected(astarte_device_handle_t device)
{
    return device->connected;
}

char *astarte_device_get_encoded_id(astarte_device_handle_t device)
{
    return device->encoded_hwid;
}

static astarte_err_t retrieve_credentials(astarte_pairing_config_t *pairing_config)
{
    astarte_err_t ret = ASTARTE_ERR;
    char *cert_pem = NULL;
    char *csr = calloc(CSR_LENGTH, sizeof(char));
    if (!csr) {
        ret = ASTARTE_ERR_OUT_OF_MEMORY;
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
        ret = ASTARTE_ERR_OUT_OF_MEMORY;
        ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
        goto exit;
    }
    ret = astarte_pairing_get_mqtt_v1_credentials(pairing_config, csr, cert_pem, CERT_LENGTH);
    if (ret != ASTARTE_OK) {
        ESP_LOGE(TAG, "Error in get_mqtt_v1_credentials");
        goto exit;
    } else {
        ESP_LOGD(TAG, "Got credentials");
    }

    ret = astarte_credentials_save_certificate(cert_pem);
    if (ret != ASTARTE_OK) {
        ESP_LOGE(TAG, "Error in get_mqtt_v1_credentials");
        goto exit;
    } else {
        ESP_LOGD(TAG, "Certificate saved");
    }

    ret = ASTARTE_OK;

exit:
    free(csr);
    free(cert_pem);
    return ret;
}

static astarte_err_t check_device(astarte_device_handle_t device)
{
    if (!device->mqtt_client) {
        ESP_LOGE(TAG, "NULL mqtt_client");
        return ASTARTE_ERR;
    }

    if (!device->device_topic) {
        ESP_LOGE(TAG, "NULL device_topic");
        return ASTARTE_ERR;
    }

    return ASTARTE_OK;
}

static size_t get_int_string_size(int number)
{
    size_t digits = 1;
    const int max_number_in_single_digit = 9;
    const int min_number_in_double_digit = 10;
    while (number > max_number_in_single_digit) {
        number /= min_number_in_double_digit;
        digits++;
    }
    return digits;
}

static size_t get_introspection_string_size(astarte_device_handle_t device)
{
    astarte_list_head_t *item = NULL;
    size_t introspection_size = 0;
    LIST_FOR_EACH(item, &device->interfaces_list)
    {
        astarte_ptr_list_entry_t *entry = GET_LIST_ENTRY(item, astarte_ptr_list_entry_t, head);
        const astarte_interface_t *interface = entry->value;

        size_t major_digits = get_int_string_size(interface->major_version);
        size_t minor_digits = get_int_string_size(interface->minor_version);

        // The interface name in introspection is composed as  "name:major:minor;"
        introspection_size += strlen(interface->name) + major_digits + minor_digits + 3;
    }
    return introspection_size;
}

static void send_introspection(astarte_device_handle_t device)
{
    if (check_device(device) != ASTARTE_OK) {
        return;
    }

    esp_mqtt_client_handle_t mqtt = device->mqtt_client;

    astarte_list_head_t *item = NULL;
    size_t introspection_size = get_introspection_string_size(device);

    // if introspection size is > 4KiB print a warning
    const size_t max_introspection_size = 4096;
    if (introspection_size > max_introspection_size) {
        ESP_LOGW(TAG, "The introspection size is > 4KiB");
    }

    char *introspection_string = calloc(introspection_size + 1, sizeof(char));
    if (!introspection_string) {
        ESP_LOGE(TAG, "Unable to allocate memory for introspection string");
        return;
    }
    int len = 0;
    LIST_FOR_EACH(item, &device->interfaces_list)
    {
        astarte_ptr_list_entry_t *entry = GET_LIST_ENTRY(item, astarte_ptr_list_entry_t, head);
        const astarte_interface_t *interface = entry->value;
        len += sprintf(introspection_string + len, "%s:%d:%d;", interface->name,
            interface->major_version, interface->minor_version);
    }
    // Remove last ; from introspection
    introspection_string[len - 1] = 0;
    // Decrease len accordingly
    len -= 1;

    ESP_LOGD(TAG, "Publishing introspection: %s", introspection_string);
    esp_mqtt_client_publish(mqtt, device->device_topic, introspection_string, len, 2, 0);
    free(introspection_string);
}

static void setup_subscriptions(astarte_device_handle_t device)
{
    if (check_device(device) != ASTARTE_OK) {
        return;
    }

    char topic[TOPIC_LENGTH];

    esp_mqtt_client_handle_t mqtt = device->mqtt_client;

    // Subscribe to control messages
    int ret = snprintf(topic, TOPIC_LENGTH, "%s/control/consumer/properties", device->device_topic);
    if ((ret < 0) || (ret >= TOPIC_LENGTH)) {
        ESP_LOGE(TAG, "Error encoding topic");
        return;
    }

    ESP_LOGD(TAG, "Subscribing to %s", topic);
    esp_mqtt_client_subscribe(mqtt, topic, 2);

    astarte_list_head_t *item = NULL;
    LIST_FOR_EACH(item, &device->interfaces_list)
    {
        astarte_ptr_list_entry_t *entry = GET_LIST_ENTRY(item, astarte_ptr_list_entry_t, head);
        const astarte_interface_t *interface = entry->value;
        if (interface->ownership == OWNERSHIP_SERVER) {
            // Subscribe to server interface subtopics
            ret = snprintf(topic, TOPIC_LENGTH, "%s/%s/#", device->device_topic, interface->name);
            if ((ret < 0) || (ret >= TOPIC_LENGTH)) {
                ESP_LOGE(TAG, "Error encoding topic");
                continue;
            }
            ESP_LOGD(TAG, "Subscribing to %s", topic);
            esp_mqtt_client_subscribe(mqtt, topic, 2);
        }
    }
}

static void send_emptycache(astarte_device_handle_t device)
{
    if (check_device(device) != ASTARTE_OK) {
        return;
    }

    esp_mqtt_client_handle_t mqtt = device->mqtt_client;

    char topic[TOPIC_LENGTH];
    int ret = snprintf(topic, TOPIC_LENGTH, "%s/control/emptyCache", device->device_topic);
    if ((ret < 0) || (ret >= TOPIC_LENGTH)) {
        ESP_LOGE(TAG, "Error encoding topic");
        return;
    }
    ESP_LOGD(TAG, "Sending emptyCache to %s", topic);
    esp_mqtt_client_publish(mqtt, topic, "1", 1, 2, 0);
}

static void send_device_owned_properties(astarte_device_handle_t device)
{
    if (check_device(device) != ASTARTE_OK) {
        return;
    }

    char *interface_name = NULL;
    char *path = NULL;
    uint8_t *value = NULL;

    astarte_set_handle_t interfaces_set_handle;
    astarte_set_init(&interfaces_set_handle);

    // Open storage
    astarte_storage_handle_t astarte_storage_handle;
    astarte_storage_err_t astarte_storage_err = astarte_storage_open(&astarte_storage_handle);
    if (astarte_storage_err != ASTARTE_STORAGE_ERR_OK) {
        ESP_LOGE(TAG, "Error opening storage %d.", astarte_storage_err);
        goto end;
    }

    // Create iterator
    astarte_storage_iterator_t astarte_storage_iterator;
    astarte_storage_err =
        astarte_storage_iterator_create(astarte_storage_handle, &astarte_storage_iterator);
    if ((astarte_storage_err != ASTARTE_STORAGE_ERR_OK) &&
        (astarte_storage_err != ASTARTE_STORAGE_ERR_NOT_FOUND)) {
        ESP_LOGE(TAG, "Error creating the properties iterator %d.", astarte_storage_err);
        goto end;
    }

    while (astarte_storage_err == ASTARTE_STORAGE_ERR_OK) {
        // Fetch property len
        size_t interface_name_len = 0;
        size_t path_len = 0;
        size_t value_len = 0;
        astarte_storage_err = astarte_storage_iterator_get_property(&astarte_storage_iterator, NULL,
            &interface_name_len, NULL, &path_len, NULL, &value_len);
        if (astarte_storage_err != ASTARTE_STORAGE_ERR_OK) {
            ESP_LOGE(TAG, "Error preparing to get one of the properties %d.", astarte_storage_err);
            astarte_storage_close(astarte_storage_handle);
            goto end;
        }
        // Allocate memory for property data
        interface_name = calloc(interface_name_len, sizeof(char));
        path = calloc(path_len, sizeof(char));
        value = calloc(value_len, sizeof(uint8_t));
        if ((interface_name == NULL) || (path == NULL) || (value == NULL)) {
            ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
            astarte_storage_close(astarte_storage_handle);
            goto end;
        }
        // Fetch property content
        astarte_storage_err = astarte_storage_iterator_get_property(&astarte_storage_iterator,
            interface_name, &interface_name_len, path, &path_len, value, &value_len);
        if (astarte_storage_err != ASTARTE_STORAGE_ERR_OK) {
            ESP_LOGE(TAG, "Error getting one of the properties %d.", astarte_storage_err);
            astarte_storage_close(astarte_storage_handle);
            goto end;
        }
        // If property is not in introspection anymore, delete it from storage
        if (!interface_is_in_introspection(device, interface_name)) {
            // TODO figure out if it is safe to uncomment this
            // Commented out since this operation could break the current iterator, or lead to
            // skip some properties
            // astarte_storage_delete_property(interface_name, path);
        }
        // If property is device owned send it to Astarte
        else if (interface_is_device_owned(device, interface_name)) {
            // Assuming that since the property is present in storage the size of the value
            // has already been checked and does not exceed the max value for an integer.
            publish_data(device, interface_name, path, value, (int) value_len, 2);

            // Get the combined interface_name and path for the property
            size_t property_full_path_len = interface_name_len + path_len - 1;
            char *property_full_path = calloc(property_full_path_len, sizeof(char));
            strncat(memcpy(property_full_path, interface_name, interface_name_len), path, path_len - 1);

            // Store the property full path in a temporary set
            astarte_set_element_t set_element = {
                .value = property_full_path,
                .value_len = property_full_path_len
            };
            astarte_set_add(interfaces_set_handle, set_element);
        }
        // Free memory of property data
        free(interface_name);
        interface_name = NULL;
        free(path);
        path = NULL;
        free(value);
        value = NULL;
        // Advance iterator
        astarte_storage_err = astarte_storage_iterator_advance(&astarte_storage_iterator);
        if (astarte_storage_err != ASTARTE_STORAGE_ERR_OK) {
            if (astarte_storage_err != ASTARTE_STORAGE_ERR_NOT_FOUND) {
                ESP_LOGE(TAG, "Error advancing properties iterator %d.", astarte_storage_err);
            }
            break;
        }
    }

    // Close astarte storage
    astarte_storage_close(astarte_storage_handle);

    // Send purge device properties
    send_purge_device_properties(device, interfaces_set_handle);

end:
    // Destroy the set
    astarte_set_destroy(interfaces_set_handle);

    // Free all data
    free(interface_name);
    free(path);
    free(value);
}

static void send_purge_device_properties(astarte_device_handle_t device,
    astarte_set_handle_t properties_set_handle)
{
    // Pointer to string that will contain a list of interface names separated by the ';' char
    char *properties_full_paths = NULL;
    // Size of the string contained in properties_full_paths including the '\0' terminating char
    size_t properties_full_paths_len = 0;

    char *purge_properties_payload = NULL;
    size_t purge_properties_payload_len = 0;

    while(!astarte_set_is_empty(properties_set_handle))
    {
        // Get an interface name from the set
        astarte_set_element_t property_full_path;
        astarte_set_pop(properties_set_handle, &property_full_path);

        // Append interface name to the ';' separated string
        // Both lengths include a +1 for the '\0' char
        // We do a simple sum since we need an extra char to store the ';'
        size_t extended_properties_full_path_len = properties_full_paths_len + property_full_path.value_len;
        char *extended_properties_full_path = (char *) realloc(properties_full_paths, extended_properties_full_path_len);
        if (extended_properties_full_path == NULL) {
            ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
            free(property_full_path.value);
            goto end;
        }
        if (properties_full_paths_len == 0) {
            // Set the first char to '\0' for the first interface name
            *extended_properties_full_path = '\0';
        } else {
            // Append a ';' for any subsequent interface name
            strcat(extended_properties_full_path, ";");
        }
        // Append the new interface name
        properties_full_paths = strcat(extended_properties_full_path, property_full_path.value);
        properties_full_paths_len = extended_properties_full_path_len;

        free(property_full_path.value);
    }

    // Step 1 estimate compression result size and payload size
    size_t compression_input_len = (properties_full_paths) ? (properties_full_paths_len - 1) : 0;
    uLongf compressed_len = compressBound(compression_input_len);
    // Step 2 allocate enough memory for the payload
    purge_properties_payload_len = 4 + compressed_len;
    purge_properties_payload = calloc(purge_properties_payload_len, sizeof(char));
    if (purge_properties_payload == NULL) {
        ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
        goto end;
    }
    // Fill the first 32 bits of the payload
    purge_properties_payload[0] = (uint8_t) (compression_input_len >>  24u);
    purge_properties_payload[1] = (uint8_t) (compression_input_len >>  16u);
    purge_properties_payload[2] = (uint8_t) (compression_input_len >>  8u);
    purge_properties_payload[3] = (uint8_t) (compression_input_len >>  0u);
    // Perform the compression and store result in the payload
    int compress_res = astarte_zlib_compress(
        (char unsigned *) &purge_properties_payload[4], &compressed_len,
        (char unsigned *) properties_full_paths, compression_input_len);
    if (compress_res != Z_OK) {
        ESP_LOGE(TAG, "Compression error %d.", compress_res);
        goto end;
    }

    // Compose MQTT topic
    char topic[TOPIC_LENGTH];
    int ret = snprintf(topic, TOPIC_LENGTH, "%s/control/producer/properties", device->device_topic);
    if ((ret < 0) || (ret >= TOPIC_LENGTH)) {
        ESP_LOGE(TAG, "Error encoding topic");
        goto end;
    }
    // Set constant for MQTT QoS
    const int qos = 2;
    // Publish MQTT message
    ESP_LOGD(TAG, "Sending purge properties to: '%s', with uncompressed content: '%s'",
        topic, (properties_full_paths) ? properties_full_paths : "");
    esp_mqtt_client_publish(device->mqtt_client, topic, purge_properties_payload,
        purge_properties_payload_len, qos, 0);

end:
    free(properties_full_paths);
    free (purge_properties_payload);
}

static void on_connected(astarte_device_handle_t device, int session_present)
{
    device->connected = true;

    if (device->connection_event_callback) {
        astarte_device_connection_event_t event = {
            .device = device,
            .session_present = session_present,
            .user_data = device->callbacks_user_data,
        };

        device->connection_event_callback(&event);
    }

    if (session_present) {
        return;
    }

    setup_subscriptions(device);
    send_introspection(device);
    send_emptycache(device);
    send_device_owned_properties(device);
}

static void on_disconnected(astarte_device_handle_t device)
{
    device->connected = false;

    if (device->disconnection_event_callback) {
        astarte_device_disconnection_event_t event = {
            .device = device,
            .user_data = device->callbacks_user_data,
        };

        device->disconnection_event_callback(&event);
    }
}

static void on_incoming(
    astarte_device_handle_t device, char *topic, int topic_len, char *data, int data_len)
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

    const size_t maximum_control_prefix_len = 512;
    char control_prefix[maximum_control_prefix_len];
    int ret
        = snprintf(control_prefix, maximum_control_prefix_len, "%s/control", device->device_topic);
    if ((ret < 0) || (ret >= maximum_control_prefix_len)) {
        ESP_LOGE(TAG, "Error encoding control prefix");
        return;
    }

    // Control message
    size_t control_prefix_len = strlen(control_prefix);
    if (strstr(topic, control_prefix)) {
        char *control_topic = topic + control_prefix_len;
        ESP_LOGD(TAG, "Received control message on control topic %s", control_topic);
        on_control_message(device, control_topic, data, data_len);
        return;
    }

    // Data message
    if (topic_len < device->device_topic_len + strlen("/")
        || topic[device->device_topic_len] != '/') {
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
    const size_t maximum_interface_name_len = 512;
    char interface_name[maximum_interface_name_len];
    ret = snprintf(interface_name, maximum_interface_name_len, "%.*s", interface_name_len,
        interface_name_begin);
    if ((ret < 0) || (ret >= maximum_interface_name_len)) {
        ESP_LOGE(TAG, "Error encoding interface name");
        return;
    }

    size_t path_len = topic_len - device->device_topic_len - strlen("/") - interface_name_len;
    const size_t maximum_path_len = 512;
    char path[maximum_path_len];
    ret = snprintf(path, maximum_path_len, "%.*s", path_len, path_begin);
    if ((ret < 0) || (ret >= maximum_path_len)) {
        ESP_LOGE(TAG, "Error encoding path");
        return;
    }

    if (!data && data_len == 0) {
        if (device->unset_event_callback) {
            astarte_device_unset_event_t event = {
                .device = device,
                .interface_name = interface_name,
                .path = path,
                .user_data = device->callbacks_user_data,
            };
            device->unset_event_callback(&event);
        } else {
            ESP_LOGE(
                TAG, "Unset data for %s received, but unset_event_callback is not defined", path);
        }
        return;
    }

    if (!astarte_bson_deserializer_check_validity(data, data_len)) {
        ESP_LOGE(TAG, "Invalid BSON document in data");
        return;
    }

    if (interface_is_property(device, interface_name)) {
        astarte_storage_handle_t astarte_storage_handle;
        astarte_storage_open(&astarte_storage_handle);
        astarte_storage_store_property(astarte_storage_handle, interface_name, path, data, data_len);
        astarte_storage_close(astarte_storage_handle);
    }

    // Keep old deserializer for compatibility
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    uint8_t bson_value_type = 0U;
    const void *bson_value = astarte_bson_key_lookup("v", data, &bson_value_type);
    if (!bson_value) {
        ESP_LOGE(TAG, "Cannot retrieve BSON value from data");
        return;
    }
#pragma GCC diagnostic pop

    // Use new deserializer
    astarte_bson_document_t full_document = astarte_bson_deserializer_init_doc(data);
    astarte_bson_element_t v_elem;
    if (astarte_bson_deserializer_element_lookup(full_document, "v", &v_elem) != ASTARTE_OK) {
        ESP_LOGE(TAG, "Cannot retrieve BSON value from data");
        return;
    }

    astarte_device_data_event_t event = {
        .device = device,
        .interface_name = interface_name,
        .path = path,
        .bson_value = bson_value,
        .bson_value_type = bson_value_type,
        .bson_element = v_elem,
        .user_data = device->callbacks_user_data,
    };

    device->data_event_callback(&event);
}

static void on_control_message(astarte_device_handle_t device, char *control_topic, char *data, int data_len)
{
    if (strcmp(control_topic, "/consumer/properties") == 0) {
        on_purge_properties(device, data, data_len);
    }
}

static void on_purge_properties(astarte_device_handle_t device, char *data, int data_len)
{
    // Uncompress the payload
    char *uncompressed = NULL;
    uLongf uncompressed_len = (size_t) data[0] << 24 |
        (size_t) data[1] << 16 | (size_t) data[2] << 8 | (size_t) data[3];

    if (uncompressed_len != 0) {
        uncompressed = calloc(uncompressed_len + 1, sizeof(char));
        if (uncompressed == NULL) {
            ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
            goto end;
        }
        int uncompress_res = uncompress((char unsigned *) uncompressed, &uncompressed_len,
            (char unsigned *) data + 4, data_len);
        if (uncompress_res != Z_OK) {
            ESP_LOGE(TAG, "Decompression error %d.", uncompress_res);
            goto end;
        }
    }

    ESP_LOGD(TAG, "Received purge properties with content: '%s'",
        (uncompressed) ? uncompressed : "");

    // Split the payload in individual properties and store them in a list
    astarte_linked_list_handle_t list_handle;
    if (ASTARTE_OK != astarte_linked_list_init(&list_handle)) {
        ESP_LOGE(TAG, "Error intializing linked list.");
        goto end;
    }
    if (uncompressed_len != 0) {
        char *property = strtok(uncompressed, ";");
        if (!property) {
            ESP_LOGE(TAG, "Error parsing the purge property message %s.", uncompressed);
            goto end;
        }
        do {
            astarte_linked_list_item_t list_item = {
                .value = property,
                .value_len = strlen(property) + 1
            };
            if (ASTARTE_OK != astarte_linked_list_append(list_handle, list_item)) {
                astarte_linked_list_destroy(list_handle);
                ESP_LOGE(TAG, "Error appending entry to linked list.");
                goto end;
            }
            property = strtok(NULL, ";");
        } while (property);
    }

    // Open storage
    astarte_storage_handle_t storage_handle;
    if (ASTARTE_STORAGE_ERR_OK != astarte_storage_open(&storage_handle)) {
        astarte_linked_list_destroy(list_handle);
        ESP_LOGE(TAG, "Error opening storage.");
        goto end;
    }

    // Create storage iterator
    astarte_storage_iterator_t storage_iterator;
    astarte_storage_err_t storage_err = astarte_storage_iterator_create(storage_handle,
        &storage_iterator);
    if (storage_err == ASTARTE_STORAGE_ERR_NOT_FOUND) {
        astarte_storage_close(storage_handle);
        astarte_linked_list_destroy(list_handle);
        goto end;
    }
    if (storage_err != ASTARTE_STORAGE_ERR_OK) {
        ESP_LOGE(TAG, "Error creating the properties iterator %d.", storage_err);
        astarte_storage_close(storage_handle);
        astarte_linked_list_destroy(list_handle);
        goto end;
    }

    // Iterate through all the properties stored in memory
    do
    {
        // Fetch one property lengths
        size_t interface_name_len, path_len, value_len;
        storage_err = astarte_storage_iterator_get_property(&storage_iterator, NULL,
            &interface_name_len, NULL, &path_len, NULL, &value_len);
        if (storage_err != ASTARTE_STORAGE_ERR_OK) {
            ESP_LOGE(TAG, "Error fetching property lengths %d.", storage_err);
            astarte_storage_close(storage_handle);
            astarte_linked_list_destroy(list_handle);
            goto end;
        }

        // Allocate memory for property data
        char *interface_name = calloc(interface_name_len, sizeof(char));
        char *path = calloc(path_len, sizeof(char));
        if ((interface_name == NULL) || (path == NULL)) {
            ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
            free(interface_name);
            free(path);
            astarte_storage_close(storage_handle);
            astarte_linked_list_destroy(list_handle);
            goto end;
        }

        // Fetch one property data
        storage_err = astarte_storage_iterator_get_property(&storage_iterator, interface_name,
                &interface_name_len, path, &path_len, NULL, &value_len);
        if (storage_err != ASTARTE_STORAGE_ERR_OK) {
            ESP_LOGE(TAG, "Error fetching property data %d.", storage_err);
            free(interface_name);
            free(path);
            astarte_storage_close(storage_handle);
            astarte_linked_list_destroy(list_handle);
            goto end;
        }

        bool advance_iterator = true;
        // Only server owned interfaces should be evaluated
        if(!interface_is_device_owned(device, interface_name)){
            // Compare against the properties received in the purge prop
            bool find_res = false;
            if (!astarte_linked_list_is_empty(list_handle)) {
                // Create full property name
                size_t full_prop_len = interface_name_len + path_len - 1;
                char *full_prop = calloc(full_prop_len, sizeof(char));
                full_prop = strncat(
                    strncat(full_prop, interface_name, interface_name_len - 1), path, path_len - 1);

                // Find if it's in the purge properties list
                astarte_linked_list_item_t item = {.value = full_prop, .value_len = full_prop_len};
                esp_err_t list_err = astarte_linked_list_find(list_handle, item, &find_res);
                if (list_err != ASTARTE_OK) {
                    ESP_LOGE(TAG, "Error in list finding %d.", list_err);
                    free(interface_name);
                    free(path);
                    free(full_prop);
                    astarte_storage_close(storage_handle);
                    astarte_linked_list_destroy(list_handle);
                    goto end;
                }

                // Free full property name
                free(full_prop);
            }

            // If property is not in purge property list, remove it from storage
            if (!find_res) {
                ESP_LOGD(TAG, "Purging property: '%s', '%s'", interface_name, path);
                storage_err = astarte_storage_delete_property(storage_handle, interface_name, path);
                if (storage_err != ASTARTE_STORAGE_ERR_OK) {
                    ESP_LOGE(TAG, "Error deleting the property: %d.", storage_err);
                    free(interface_name);
                    free(path);
                    astarte_storage_close(storage_handle);
                    astarte_linked_list_destroy(list_handle);
                    goto end;
                }
                advance_iterator = false; // Iterator has been advanced by the delete function
            }
        }

        // Free individual property data
        free(interface_name);
        free(path);

        // Advance the iterator if required
        if (advance_iterator) {
            storage_err = astarte_storage_iterator_advance(&storage_iterator);
            if ((storage_err != ASTARTE_STORAGE_ERR_OK) &&
                (storage_err != ASTARTE_STORAGE_ERR_NOT_FOUND)) {
                    ESP_LOGE(TAG, "Error iterating through the properties: %d.", storage_err);
                    astarte_storage_close(storage_handle);
                    astarte_linked_list_destroy(list_handle);
                    goto end;
            }
        }

    } while (storage_err != ASTARTE_STORAGE_ERR_NOT_FOUND);

    // Close storage
    astarte_storage_close(storage_handle);
    // Destroy the linked list
    astarte_linked_list_destroy(list_handle);

end:
    free(uncompressed);
}

static int has_connectivity()
{
    esp_http_client_config_t config
        = {.url = CONFIG_ASTARTE_CONNECTIVITY_TEST_URL,
#if CONFIG_MBEDTLS_CERTIFICATE_BUNDLE && (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 3, 0))
              .crt_bundle_attach = esp_crt_bundle_attach,
#endif
          };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);

    int res = 0;
    const int http_bad_request = 400;
    if ((err == ESP_OK) && (esp_http_client_get_status_code(client) < http_bad_request)) {
        res = 1;
    }
    esp_http_client_cleanup(client);

    return res;
}

static void on_certificate_error(astarte_device_handle_t device)
{
    if (has_connectivity()) {
        ESP_LOGW(TAG, "Certificate error, notifying the reinit task");
        xTaskNotify(device->reinit_task_handle, NOTIFY_REINIT, eSetBits);
    } else {
        ESP_LOGD(TAG, "TLS error due to missing connectivity, ignoring");
        // Do nothing, the mqtt client will try to connect again
    }
}

static void mqtt_event_handler(
    void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    (void) base;

    esp_mqtt_event_handle_t event = event_data;
    astarte_device_handle_t device = (astarte_device_handle_t) handler_args;
    switch ((esp_mqtt_event_id_t) event_id) {
        case MQTT_EVENT_BEFORE_CONNECT:
            ESP_LOGD(TAG, "MQTT_EVENT_BEFORE_CONNECT");
            break;

        case MQTT_EVENT_CONNECTED:
            ESP_LOGD(TAG, "MQTT_EVENT_CONNECTED");
            on_connected(device, event->session_present);
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGD(TAG, "MQTT_EVENT_DISCONNECTED");
            on_disconnected(device);
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGD(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGD(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_PUBLISHED:
            ESP_LOGD(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_DATA:
            ESP_LOGD(TAG, "MQTT_EVENT_DATA");
            on_incoming(device, event->topic, event->topic_len, event->data, event->data_len);
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGD(TAG, "MQTT_EVENT_ERROR");
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_ESP_TLS) {
                on_certificate_error(device);
            }
            break;

        default:
            // Handle MQTT_EVENT_ANY introduced in esp-idf 3.2
            break;
    }
}

static bool interface_is_in_introspection(
    astarte_device_handle_t device, const char *interface_name)
{
    astarte_list_head_t *item = NULL;
    LIST_FOR_EACH(item, &device->interfaces_list)
    {
        astarte_ptr_list_entry_t *entry = GET_LIST_ENTRY(item, astarte_ptr_list_entry_t, head);
        const astarte_interface_t *interface = entry->value;
        if (strcmp(interface->name, interface_name) == 0) {
            return true;
        }
    }
    return false;
}

static bool interface_is_property(astarte_device_handle_t device, const char *interface_name)
{
    astarte_list_head_t *item = NULL;
    LIST_FOR_EACH(item, &device->interfaces_list)
    {
        astarte_ptr_list_entry_t *entry = GET_LIST_ENTRY(item, astarte_ptr_list_entry_t, head);
        const astarte_interface_t *interface = entry->value;
        if (strcmp(interface->name, interface_name) == 0) {
            return interface->type == TYPE_PROPERTIES;
        }
    }
    return false;
}

static bool interface_is_device_owned(astarte_device_handle_t device, const char *interface_name)
{
    astarte_list_head_t *item = NULL;
    LIST_FOR_EACH(item, &device->interfaces_list)
    {
        astarte_ptr_list_entry_t *entry = GET_LIST_ENTRY(item, astarte_ptr_list_entry_t, head);
        const astarte_interface_t *interface = entry->value;
        if (strcmp(interface->name, interface_name) == 0) {
            return interface->ownership == OWNERSHIP_DEVICE;
        }
    }
    return false;
}
