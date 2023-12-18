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
#include <astarte_pairing.h>
#include <astarte_storage.h>
#include <astarte_zlib.h>

#include <mqtt_client.h>

#include <esp_http_client.h>
#if CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
#include <esp_crt_bundle.h>
#endif
#include <esp_log.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <limits.h>

#define TAG "ASTARTE_DEVICE"

#define HWID_LENGTH 16
#define ENCODED_HWID_LENGTH 256
#define CREDENTIALS_SECRET_LENGTH 512
#define CSR_LENGTH 4096
#define CERT_LENGTH 4096
#define CN_LENGTH 512
#define PRIVKEY_LENGTH 8196
#define URL_LENGTH 512
#define TOPIC_LENGTH 512
#define INTERFACE_LENGTH 512
#define PATH_LENGTH 512
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
    astarte_linked_list_handle_t introspection;
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
#ifdef CONFIG_ASTARTE_USE_PROPERTY_PERSISTENCY
static void send_device_owned_properties(astarte_device_handle_t device);
static void send_purge_device_properties(
    astarte_device_handle_t device, astarte_linked_list_handle_t *list_handle);
#endif
static void on_connected(astarte_device_handle_t device, int session_present);
static void on_disconnected(astarte_device_handle_t device);
static void on_incoming(
    astarte_device_handle_t device, char *topic, int topic_len, char *data, int data_len);
static void on_control_message(
    astarte_device_handle_t device, char *control_topic, char *data, int data_len);
#ifdef CONFIG_ASTARTE_USE_PROPERTY_PERSISTENCY
static void on_purge_properties(astarte_device_handle_t device, char *data, int data_len);
static astarte_err_t uncompress_purge_properties(
    char *data, int data_len, char **output, uLongf *output_len);
#endif
static void on_certificate_error(astarte_device_handle_t device);
static void mqtt_event_handler(
    void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
static int has_connectivity();
static void maybe_append_timestamp(astarte_bson_serializer_handle_t bson, uint64_t ts_epoch_millis);
#ifdef CONFIG_ASTARTE_USE_PROPERTY_PERSISTENCY
static astarte_interface_t *get_interface_from_introspection(
    astarte_device_handle_t device, const char *name);
#endif

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

    const configSTACK_DEPTH_TYPE stack_depth = 6000;
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
        char generated_encoded_hwid[ENCODED_HWID_LENGTH] = { 0 };
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

    ret->introspection = astarte_linked_list_init();
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

    char credentials_secret[CREDENTIALS_SECRET_LENGTH] = { 0 };
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

    char broker_url[URL_LENGTH] = { 0 };
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
#ifdef CONFIG_ASTARTE_USE_PROPERTY_PERSISTENCY
              .session.disable_clean_session = true,
#endif
          };
#else
    const esp_mqtt_client_config_t mqtt_cfg
        = {.uri = broker_url,
#if CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
              .crt_bundle_attach = esp_crt_bundle_attach,
#endif
              .client_cert_pem = client_cert_pem,
              .client_key_pem = key_pem,
              .user_context = device,
#ifdef CONFIG_ASTARTE_USE_PROPERTY_PERSISTENCY
              .disable_clean_session = true,
#endif
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
    astarte_linked_list_destroy(&device->introspection);
    free(device);
}

astarte_err_t astarte_device_add_interface(
    astarte_device_handle_t device, const astarte_interface_t *interface)
{
    astarte_err_t result = ASTARTE_OK;
    if (xSemaphoreTake(device->reinit_mutex, (TickType_t) 10) == pdFALSE) {
        ESP_LOGE(TAG, "Trying to add an interface to a device that is being reinitialized");
        return ASTARTE_ERR;
    }

    if (interface->major_version == 0 && interface->minor_version == 0) {
        ESP_LOGE(TAG, "Trying to add an interface with both major and minor version equal 0");
        result = ASTARTE_ERR_INVALID_INTERFACE_VERSION;
        goto end;
    }

    // Loop over any interface in introspection searching for an interface with the same name
    size_t interface_name_len = strlen(interface->name);
    astarte_linked_list_iterator_t list_iter;
    astarte_err_t iter_err = astarte_linked_list_iterator_init(&device->introspection, &list_iter);
    while (iter_err != ASTARTE_ERR_NOT_FOUND) {
        astarte_interface_t *tmp_interface = NULL;
        astarte_linked_list_iterator_get_item(&list_iter, (void **) &tmp_interface);

        // Check if the interface has the same name
        size_t tmp_interface_name_len = strlen(tmp_interface->name);
        size_t interfaces_min_len = (interface_name_len < tmp_interface_name_len)
            ? interface_name_len
            : tmp_interface_name_len;
        if (strncmp(interface->name, tmp_interface->name, interfaces_min_len) == 0) {
            ESP_LOGW(TAG, "Trying to add an interface already present in introspection");
            // Check if ownership and type are the same
            if ((interface->ownership != tmp_interface->ownership)
                || (interface->type != tmp_interface->type)) {
                ESP_LOGE(TAG, "Interface ownership/type conflicts with the one in introspection");
                result = ASTARTE_ERR_CONFLICTING_INTERFACE;
                goto end;
            }
            // Check if major versions align correctly
            if (interface->major_version < tmp_interface->major_version) {
                ESP_LOGE(TAG, "Interface with smaller major version than one in introspection");
                result = ASTARTE_ERR_CONFLICTING_INTERFACE;
                goto end;
            }
            // Check if minor versions aligns correctly
            if ((interface->major_version == tmp_interface->major_version)
                && (interface->minor_version < tmp_interface->minor_version)) {
                ESP_LOGE(TAG,
                    "Interface with same major version and smaller minor version than one in "
                    "introspection");
                result = ASTARTE_ERR_CONFLICTING_INTERFACE;
                goto end;
            }
            ESP_LOGW(TAG, "Overwriting interface %s", interface->name);
            astarte_linked_list_iterator_replace_item(
                &list_iter, (astarte_interface_t *) interface);
            goto end;
        }

        iter_err = astarte_linked_list_iterator_advance(&list_iter);
    }

    ESP_LOGD(TAG, "Adding interface %s to device", interface->name);
    astarte_err_t list_err
        = astarte_linked_list_append(&device->introspection, (astarte_interface_t *) interface);
    if (list_err != ASTARTE_OK) {
        ESP_LOGE(TAG, "Can't add interface to introspection %s", astarte_err_to_name(list_err));
        result = list_err;
        goto end;
    }

end:
    xSemaphoreGive(device->reinit_mutex);
    return result;
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
#ifdef CONFIG_ASTARTE_USE_PROPERTY_PERSISTENCY
    astarte_interface_t *interface = get_interface_from_introspection(device, interface_name);
    if (interface && (interface->type == TYPE_PROPERTIES)) {
        // Open storage
        astarte_storage_handle_t storage_handle;
        astarte_err_t storage_err = astarte_storage_open(&storage_handle);
        if (storage_err != ASTARTE_OK) {
            ESP_LOGE(TAG, "Error opening storage.");
            return ASTARTE_ERR;
        }
        // Check if property is already stored in nvs with same value
        bool is_contained = false;
        storage_err = astarte_storage_contains_property(storage_handle, interface_name, path,
            interface->major_version, data, len, &is_contained);
        if (storage_err != ASTARTE_OK) {
            ESP_LOGE(TAG, "Error checking if property is in storage.");
            astarte_storage_close(storage_handle);
            return ASTARTE_ERR;
        }
        if (is_contained) {
            ESP_LOGW(TAG, "Trying to set a property twice: '%s%s'", interface_name, path);
            astarte_storage_close(storage_handle);
            return ASTARTE_OK;
        }
        // Store property
        ESP_LOGD(TAG, "Storing device property: '%s%s'.", interface_name, path);
        storage_err = astarte_storage_store_property(
            storage_handle, interface_name, path, interface->major_version, data, len);
        if (storage_err != ASTARTE_OK) {
            ESP_LOGE(TAG, "Error storing property.");
            astarte_storage_close(storage_handle);
            return ASTARTE_ERR;
        }
        // Close storage
        astarte_storage_close(storage_handle);
    }
#endif

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

    char topic[TOPIC_LENGTH] = { 0 };
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
#ifdef CONFIG_ASTARTE_USE_PROPERTY_PERSISTENCY
    astarte_interface_t *interface = get_interface_from_introspection(device, interface_name);
    if (interface && (interface->type == TYPE_PROPERTIES)) {
        // Open storage
        astarte_storage_handle_t storage_handle;
        astarte_err_t storage_err = astarte_storage_open(&storage_handle);
        if (storage_err != ASTARTE_OK) {
            ESP_LOGE(TAG, "Error opening storage.");
            return ASTARTE_ERR;
        }
        // Delete property
        ESP_LOGD(TAG, "Deleting device property '%s%s' from storage", interface_name, path);
        storage_err = astarte_storage_delete_property(storage_handle, interface_name, path);
        if ((storage_err != ASTARTE_OK) && (storage_err != ASTARTE_ERR_NOT_FOUND)) {
            ESP_LOGE(TAG, "Error deleting property from storage.");
            astarte_storage_close(storage_handle);
            return ASTARTE_ERR;
        }
        if (storage_err == ASTARTE_ERR_NOT_FOUND) {
            ESP_LOGW(TAG, "Trying to unset property already unset: '%s%s'.", interface_name, path);
            astarte_storage_close(storage_handle);
            return ASTARTE_OK;
        }
        // Close storage
        astarte_storage_close(storage_handle);
    }
#endif
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
    size_t introspection_size = 0;
    astarte_linked_list_iterator_t list_iter;
    astarte_err_t iter_err = astarte_linked_list_iterator_init(&device->introspection, &list_iter);
    while (iter_err != ASTARTE_ERR_NOT_FOUND) {
        astarte_interface_t *interface = NULL;
        astarte_linked_list_iterator_get_item(&list_iter, (void **) &interface);

        size_t major_digits = get_int_string_size(interface->major_version);
        size_t minor_digits = get_int_string_size(interface->minor_version);

        // The interface name in introspection is composed as  "name:major:minor;"
        introspection_size += strlen(interface->name) + major_digits + minor_digits + 3;

        iter_err = astarte_linked_list_iterator_advance(&list_iter);
    }
    return introspection_size;
}

static void send_introspection(astarte_device_handle_t device)
{
    if (check_device(device) != ASTARTE_OK) {
        return;
    }

    esp_mqtt_client_handle_t mqtt = device->mqtt_client;

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

    astarte_linked_list_iterator_t list_iter;
    astarte_err_t iter_err = astarte_linked_list_iterator_init(&device->introspection, &list_iter);
    while (iter_err != ASTARTE_ERR_NOT_FOUND) {
        astarte_interface_t *interface = NULL;
        astarte_linked_list_iterator_get_item(&list_iter, (void **) &interface);

        len += sprintf(introspection_string + len, "%s:%d:%d;", interface->name,
            interface->major_version, interface->minor_version);

        iter_err = astarte_linked_list_iterator_advance(&list_iter);
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

    char topic[TOPIC_LENGTH] = { 0 };

    esp_mqtt_client_handle_t mqtt = device->mqtt_client;

    // Subscribe to control messages
    int ret = snprintf(topic, TOPIC_LENGTH, "%s/control/consumer/properties", device->device_topic);
    if ((ret < 0) || (ret >= TOPIC_LENGTH)) {
        ESP_LOGE(TAG, "Error encoding topic");
        return;
    }

    ESP_LOGD(TAG, "Subscribing to %s", topic);
    esp_mqtt_client_subscribe(mqtt, topic, 2);

    astarte_linked_list_iterator_t list_iter;
    astarte_err_t iter_err = astarte_linked_list_iterator_init(&device->introspection, &list_iter);
    while (iter_err != ASTARTE_ERR_NOT_FOUND) {
        astarte_interface_t *interface = NULL;
        astarte_linked_list_iterator_get_item(&list_iter, (void **) &interface);

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

        iter_err = astarte_linked_list_iterator_advance(&list_iter);
    }
}

static void send_emptycache(astarte_device_handle_t device)
{
    if (check_device(device) != ASTARTE_OK) {
        return;
    }

    esp_mqtt_client_handle_t mqtt = device->mqtt_client;

    char topic[TOPIC_LENGTH] = { 0 };
    int ret = snprintf(topic, TOPIC_LENGTH, "%s/control/emptyCache", device->device_topic);
    if ((ret < 0) || (ret >= TOPIC_LENGTH)) {
        ESP_LOGE(TAG, "Error encoding topic");
        return;
    }
    ESP_LOGD(TAG, "Sending emptyCache to %s", topic);
    esp_mqtt_client_publish(mqtt, topic, "1", 1, 2, 0);
}

#ifdef CONFIG_ASTARTE_USE_PROPERTY_PERSISTENCY
static void send_device_owned_properties(astarte_device_handle_t device)
{
    if (check_device(device) != ASTARTE_OK) {
        return;
    }

    char *interface_name = NULL;
    char *path = NULL;
    uint8_t *value = NULL;

    astarte_linked_list_handle_t list_handle = astarte_linked_list_init();

    // Open storage
    astarte_storage_handle_t storage_handle;
    astarte_err_t storage_err = astarte_storage_open(&storage_handle);
    if (storage_err != ASTARTE_OK) {
        ESP_LOGE(TAG, "Error opening storage.");
        goto end;
    }

    // Create iterator
    astarte_storage_iterator_t storage_iterator;
    storage_err = astarte_storage_iterator_create(storage_handle, &storage_iterator);
    if ((storage_err != ASTARTE_OK) && (storage_err != ASTARTE_ERR_NOT_FOUND)) {
        ESP_LOGE(TAG, "Error creating the properties iterator.");
        astarte_storage_close(storage_handle);
        goto end;
    }

    while (storage_err == ASTARTE_OK) {
        // Fetch property len
        size_t interface_name_len = 0;
        size_t path_len = 0;
        size_t value_len = 0;
        storage_err = astarte_storage_iterator_get_property(
            &storage_iterator, NULL, &interface_name_len, NULL, &path_len, NULL, NULL, &value_len);
        if (storage_err != ASTARTE_OK) {
            ESP_LOGE(TAG, "Error preparing to get one of the properties.");
            astarte_storage_close(storage_handle);
            goto end;
        }
        // Allocate memory for property data
        interface_name = calloc(interface_name_len, sizeof(char));
        path = calloc(path_len, sizeof(char));
        value = calloc(value_len, sizeof(uint8_t));
        if (!interface_name || !path || !value) {
            ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
            astarte_storage_close(storage_handle);
            goto end;
        }
        // Fetch property content
        int32_t major = 0;
        storage_err = astarte_storage_iterator_get_property(&storage_iterator, interface_name,
            &interface_name_len, path, &path_len, &major, value, &value_len);
        if (storage_err != ASTARTE_OK) {
            ESP_LOGE(TAG, "Error getting one of the properties.");
            astarte_storage_close(storage_handle);
            goto end;
        }

        bool advance_iterator = true;
        astarte_interface_t *interface = get_interface_from_introspection(device, interface_name);
        // If property is not in introspection anymore, delete it from storage
        if ((!interface) || (interface->major_version != major)) {
            // Check if this is the last iterable item
            bool has_next = false;
            storage_err = astarte_storage_iterator_peek(&storage_iterator, &has_next);
            if (storage_err != ASTARTE_OK) {
                ESP_LOGE(TAG, "Error peaking next property.");
                astarte_storage_close(storage_handle);
                goto end;
            }
            // Delete the property
            ESP_LOGD(TAG, "Deleting old property '%s%s' from storage", interface_name, path);
            storage_err = astarte_storage_delete_property(storage_handle, interface_name, path);
            if ((storage_err != ASTARTE_OK) && (storage_err != ASTARTE_ERR_NOT_FOUND)) {
                ESP_LOGE(TAG, "Error deleting the property.");
                astarte_storage_close(storage_handle);
                goto end;
            }
            // If the deleted iterm was the last one, break the loop here
            if (!has_next) {
                break;
            }
            // Prevent the iterator from advancing
            advance_iterator = false;
        }
        // If property is device owned send it to Astarte
        else if (interface->ownership == OWNERSHIP_DEVICE) {
            // Assuming that since the property is present in storage the size of the value
            // has already been checked and does not exceed the max value for an integer.
            publish_data(device, interface_name, path, value, (int) value_len, 2);

            // Get the combined interface_name and path for the property
            size_t property_full_path_len = interface_name_len + path_len - 1;
            char *property_full_path = calloc(property_full_path_len, sizeof(char));
            strncat(strncpy(property_full_path, interface_name, interface_name_len), path,
                path_len - 1);

            // Store the property full path in a temporary set
            astarte_err_t list_err = astarte_linked_list_append(&list_handle, property_full_path);
            if (list_err != ASTARTE_OK) {
                ESP_LOGE(TAG, "Error adding a property name to the set %s.",
                    astarte_err_to_name(list_err));
                astarte_storage_close(storage_handle);
                goto end;
            }
        }
        // Free memory of property data
        free(interface_name);
        interface_name = NULL;
        free(path);
        path = NULL;
        free(value);
        value = NULL;
        // Advance the iterator if required
        if (advance_iterator) {
            storage_err = astarte_storage_iterator_advance(&storage_iterator);
            if (storage_err != ASTARTE_OK && storage_err != ASTARTE_ERR_NOT_FOUND) {
                ESP_LOGE(TAG, "Error advancing properties iterator.");
            }
        }
    }

    // Close astarte storage
    astarte_storage_close(storage_handle);

    // Send purge device properties
    send_purge_device_properties(device, &list_handle);

end:
    // Destroy the set
    astarte_linked_list_destroy_and_release(&list_handle);

    // Free all data
    free(interface_name);
    free(path);
    free(value);
}

static void send_purge_device_properties(
    astarte_device_handle_t device, astarte_linked_list_handle_t *list_handle)
{
    // Pointer to string that will contain a list of interface names separated by the ';' char
    char *properties_list = NULL;
    // Size of the string contained in properties_list including the '\0' terminating char
    size_t properties_list_len = 0;

    char *payload = NULL;
    size_t payload_len = 0;

    // Create the uncompressed properties string
    char *property_path = NULL;
    while (astarte_linked_list_remove_tail(list_handle, (void **) &property_path)
        != ASTARTE_ERR_NOT_FOUND) {
        // Append interface name to the ';' separated string
        // properties_list_len includes a +1 for the '\0' char
        // We add 1 since we need an extra char to store the ';'
        size_t ext_properties_list_len = properties_list_len + strlen(property_path) + 1;
        char *ext_properties_list = (char *) realloc(properties_list, ext_properties_list_len);
        if (!ext_properties_list) {
            ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
            free(property_path);
            // Deallocate everything from the set
            while (astarte_linked_list_remove_tail(list_handle, (void **) &property_path)
                != ASTARTE_ERR_NOT_FOUND) {
                free(property_path);
            }
            goto end;
        }
        if (properties_list_len == 0) {
            // Set the first char to '\0' for the first interface name
            *ext_properties_list = '\0';
        } else {
            // Append a ';' for any subsequent interface name
            strcat(ext_properties_list, ";");
        }
        // Append the new interface name
        properties_list = strcat(ext_properties_list, property_path);
        properties_list_len = ext_properties_list_len;

        free(property_path);
    }

    // Estimate compression result size and payload size
    size_t compression_input_len = (properties_list) ? (properties_list_len - 1) : 0;
    uLongf compressed_len = compressBound(compression_input_len);
    // Allocate enough memory for the payload
    payload_len = 4 + compressed_len;
    payload = calloc(payload_len, sizeof(char));
    if (!payload) {
        ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
        goto end;
    }
    // Fill the first 32 bits of the payload
    uint32_t *payload_uint32 = (uint32_t *) payload;
    *payload_uint32 = __builtin_bswap32(compression_input_len);
    // Perform the compression and store result in the payload
    int compress_res = astarte_zlib_compress((char unsigned *) &payload[4], &compressed_len,
        (char unsigned *) properties_list, compression_input_len);
    if (compress_res != Z_OK) {
        ESP_LOGE(TAG, "Compression error %d.", compress_res);
        goto end;
    }
    // 'astarte_zlib_compress' updates 'compressed_len' to the actual size of the compressed data
    payload_len = 4 + compressed_len;
    // Check if payload is not too large for a MQTT message
    if (payload_len > INT_MAX) {
        // MQTT supports sending a maximum payload length of INT_MAX
        ESP_LOGE(TAG, "Purge properties payload is too long for a single MQTT message.");
        goto end;
    }

    // Compose MQTT topic
    char topic[TOPIC_LENGTH] = { 0 };
    int ret = snprintf(topic, TOPIC_LENGTH, "%s/control/producer/properties", device->device_topic);
    if ((ret < 0) || (ret >= TOPIC_LENGTH)) {
        ESP_LOGE(TAG, "Error encoding topic");
        goto end;
    }
    // Set constant for MQTT QoS
    const int qos = 2;
    // Publish MQTT message
    ESP_LOGD(TAG, "Sending purge properties to: '%s', with uncompressed content: '%s'", topic,
        (properties_list) ? properties_list : "");
    esp_mqtt_client_publish(device->mqtt_client, topic, payload, (int) payload_len, qos, 0);

end:
    free(properties_list);
    free(payload);
}
#endif

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
#ifdef CONFIG_ASTARTE_USE_PROPERTY_PERSISTENCY
    send_device_owned_properties(device);
#endif
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

    char control_prefix[TOPIC_LENGTH] = { 0 };
    int ret = snprintf(control_prefix, TOPIC_LENGTH, "%s/control", device->device_topic);
    if ((ret < 0) || (ret >= TOPIC_LENGTH)) {
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
    char interface_name[INTERFACE_LENGTH] = { 0 };
    ret = snprintf(
        interface_name, INTERFACE_LENGTH, "%.*s", interface_name_len, interface_name_begin);
    if ((ret < 0) || (ret >= INTERFACE_LENGTH)) {
        ESP_LOGE(TAG, "Error encoding interface name");
        return;
    }

    size_t path_len = topic_len - device->device_topic_len - strlen("/") - interface_name_len;
    char path[PATH_LENGTH] = { 0 };
    ret = snprintf(path, PATH_LENGTH, "%.*s", path_len, path_begin);
    if ((ret < 0) || (ret >= PATH_LENGTH)) {
        ESP_LOGE(TAG, "Error encoding path");
        return;
    }

    if (!data && data_len == 0) {
#ifdef CONFIG_ASTARTE_USE_PROPERTY_PERSISTENCY
        astarte_interface_t *interface = get_interface_from_introspection(device, interface_name);
        if (interface && (interface->type == TYPE_PROPERTIES)) {
            // Open storage
            astarte_storage_handle_t storage_handle;
            astarte_err_t storage_err = astarte_storage_open(&storage_handle);
            if (storage_err != ASTARTE_OK) {
                ESP_LOGE(TAG, "Error opening storage.");
                return;
            }
            // Delete property
            ESP_LOGD(TAG, "Deleting server property '%s%s' from storage", interface_name, path);
            storage_err = astarte_storage_delete_property(storage_handle, interface_name, path);
            if ((storage_err != ASTARTE_OK) && (storage_err != ASTARTE_ERR_NOT_FOUND)) {
                ESP_LOGE(TAG, "Error deleting property from storage.");
            }
            // Close storage
            astarte_storage_close(storage_handle);
            if (storage_err != ASTARTE_OK) {
                return;
            }
        }
#endif
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

#ifdef CONFIG_ASTARTE_USE_PROPERTY_PERSISTENCY
    astarte_interface_t *interface = get_interface_from_introspection(device, interface_name);
    if (interface && (interface->type == TYPE_PROPERTIES)) {
        // Open storage
        astarte_storage_handle_t storage_handle;
        astarte_err_t storage_err = astarte_storage_open(&storage_handle);
        if (storage_err != ASTARTE_OK) {
            ESP_LOGE(TAG, "Error opening storage.");
            return;
        }
        // Check if property is already stored in nvs with same value
        bool is_contained = false;
        storage_err = astarte_storage_contains_property(storage_handle, interface_name, path,
            interface->major_version, data, data_len, &is_contained);
        if (storage_err != ASTARTE_OK) {
            ESP_LOGE(TAG, "Error checking if received property is in storage.");
            astarte_storage_close(storage_handle);
            return;
        }
        if (is_contained) {
            ESP_LOGD(TAG,
                "Trying to set a server property already stored with the same value: %s%s.",
                interface_name, path);
            astarte_storage_close(storage_handle);
            return;
        }

        // Store property
        ESP_LOGD(TAG, "Storing server property: '%s%s'.", interface_name, path);
        storage_err = astarte_storage_store_property(
            storage_handle, interface_name, path, interface->major_version, data, data_len);
        if (storage_err != ASTARTE_OK) {
            ESP_LOGE(TAG, "Error storing received property.");
            astarte_storage_close(storage_handle);
            return;
        }
        // Close storage
        astarte_storage_close(storage_handle);
    }
#endif

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

// NOLINTBEGIN(misc-unused-parameters)
static void on_control_message(
    astarte_device_handle_t device, char *control_topic, char *data, int data_len)
// NOLINTEND(misc-unused-parameters)
{
    if (strcmp(control_topic, "/consumer/properties") == 0) {
#ifdef CONFIG_ASTARTE_USE_PROPERTY_PERSISTENCY
        on_purge_properties(device, data, data_len);
#endif
    } else {
        ESP_LOGE(TAG, "Received unrecognized control message: %s.", control_topic);
    }
}

#ifdef CONFIG_ASTARTE_USE_PROPERTY_PERSISTENCY
static void on_purge_properties(astarte_device_handle_t device, char *data, int data_len)
{
    char *uncompressed = NULL;
    uLongf uncompressed_len = 0;
    astarte_err_t uncompress_err
        = uncompress_purge_properties(data, data_len, &uncompressed, &uncompressed_len);
    if (uncompress_err != ASTARTE_OK) {
        ESP_LOGE(TAG, "Failed uncompressing the purge properties %s.",
            astarte_err_to_name(uncompress_err));
        return;
    }

    ESP_LOGD(TAG, "Received purge properties: '%s'", (uncompressed) ? uncompressed : "");

    // Split the payload in individual properties and store them in a list
    astarte_linked_list_handle_t list_handle = astarte_linked_list_init();
    if (uncompressed_len != 0) {
        char *property = strtok(uncompressed, ";");
        if (!property) {
            ESP_LOGE(TAG, "Error parsing the purge property message %s.", uncompressed);
            goto end;
        }
        do {
            if (ASTARTE_OK != astarte_linked_list_append(&list_handle, property)) {
                ESP_LOGE(TAG, "Error appending entry to linked list.");
                goto end;
            }
            property = strtok(NULL, ";");
        } while (property);
    }

    // Open storage
    astarte_storage_handle_t storage_handle;
    if (ASTARTE_OK != astarte_storage_open(&storage_handle)) {
        ESP_LOGE(TAG, "Error opening storage.");
        goto end;
    }

    // Create storage iterator
    astarte_storage_iterator_t storage_iterator;
    astarte_err_t storage_err = astarte_storage_iterator_create(storage_handle, &storage_iterator);
    if ((storage_err != ASTARTE_OK) && (storage_err != ASTARTE_ERR_NOT_FOUND)) {
        ESP_LOGE(TAG, "Error creating the properties iterator.");
        astarte_storage_close(storage_handle);
        goto end;
    }

    // Iterate through all the properties stored in memory
    while (storage_err != ASTARTE_ERR_NOT_FOUND) {
        // Fetch property lengths
        size_t interface_name_len = 0;
        size_t path_len = 0;
        size_t value_len = 0;
        storage_err = astarte_storage_iterator_get_property(
            &storage_iterator, NULL, &interface_name_len, NULL, &path_len, NULL, NULL, &value_len);
        if (storage_err != ASTARTE_OK) {
            ESP_LOGE(TAG, "Error fetching property lengths.");
            astarte_storage_close(storage_handle);
            goto end;
        }
        // Allocate memory for property data
        char *interface_name = calloc(interface_name_len, sizeof(char));
        char *path = calloc(path_len, sizeof(char));
        if (!interface_name || !path) {
            ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
            free(interface_name);
            free(path);
            astarte_storage_close(storage_handle);
            goto end;
        }
        // Fetch property interface name and path
        int32_t major = 0;
        storage_err = astarte_storage_iterator_get_property(&storage_iterator, interface_name,
            &interface_name_len, path, &path_len, &major, NULL, &value_len);
        if (storage_err != ASTARTE_OK) {
            ESP_LOGE(TAG, "Error fetching property data.");
            free(interface_name);
            free(path);
            astarte_storage_close(storage_handle);
            goto end;
        }

        // If interface is in introspection and is server owned, search for it in the purge
        // properties list
        astarte_interface_t *interface = get_interface_from_introspection(device, interface_name);
        bool advance_iterator = true;
        bool interface_in_purge_prop_list = false;
        if (interface && (interface->ownership == OWNERSHIP_SERVER)
            && !astarte_linked_list_is_empty(&list_handle)) {

            // Format full property name
            size_t full_prop_len = interface_name_len + path_len - 1;
            char *full_prop = calloc(full_prop_len, sizeof(char));
            full_prop = strncat(strncpy(full_prop, interface_name, full_prop_len), path,
                full_prop_len - strlen(interface_name) - 1);
            // Iterate over the purge properties list
            astarte_linked_list_iterator_t iterator;
            astarte_err_t err = astarte_linked_list_iterator_init(&list_handle, &iterator);
            // Iterator init and advance only return ASTARTE_ERR_NOT_FOUND or ASTARTE_OK
            while (err != ASTARTE_ERR_NOT_FOUND) {
                char *maybe_full_prop = NULL;
                astarte_linked_list_iterator_get_item(&iterator, (void **) &maybe_full_prop);
                size_t maybe_full_prop_len = strlen(maybe_full_prop) + 1;
                // Check if the two properties are the same
                if ((maybe_full_prop_len == full_prop_len)
                    && (strncmp(maybe_full_prop, full_prop, maybe_full_prop_len - 1) == 0)) {
                    interface_in_purge_prop_list = true;
                    break;
                }
                err = astarte_linked_list_iterator_advance(&iterator);
            }
            // Free full property name
            free(full_prop);
        }

        // Delete from storage if interface:
        // - not in introspection or
        // - major version is different compare to the one in introspection
        // - is server owned but is not in purge properties list
        if ((!interface) || (interface->major_version != major)
            || ((interface->ownership == OWNERSHIP_SERVER) && !interface_in_purge_prop_list)) {
            // Check if this is the last property
            bool has_next = false;
            storage_err = astarte_storage_iterator_peek(&storage_iterator, &has_next);
            if (storage_err != ASTARTE_OK) {
                ESP_LOGE(TAG, "Error peaking next property.");
                free(interface_name);
                free(path);
                astarte_storage_close(storage_handle);
                goto end;
            }
            // Delete property
            ESP_LOGD(TAG, "Deleting server property '%s%s' from storage", interface_name, path);
            storage_err = astarte_storage_delete_property(storage_handle, interface_name, path);
            if ((storage_err != ASTARTE_OK) && (storage_err != ASTARTE_ERR_NOT_FOUND)) {
                ESP_LOGE(TAG, "Error deleting the property.");
                free(interface_name);
                free(path);
                astarte_storage_close(storage_handle);
                goto end;
            }
            // If it was the last property, exit
            if (!has_next) {
                free(interface_name);
                free(path);
                astarte_storage_close(storage_handle);
                goto end;
            }
            advance_iterator = false; // Iterator has been advanced by the delete function
        }

        // Free individual property data
        free(interface_name);
        free(path);

        // Advance the iterator if required
        if (advance_iterator) {
            storage_err = astarte_storage_iterator_advance(&storage_iterator);
            if ((storage_err != ASTARTE_OK) && (storage_err != ASTARTE_ERR_NOT_FOUND)) {
                ESP_LOGE(TAG, "Error iterating through the properties.");
                astarte_storage_close(storage_handle);
                goto end;
            }
        }
    }

    // Close storage
    astarte_storage_close(storage_handle);

end:
    // Destroy the linked list
    // No need to free the memory as all the data contained in this list is part of uncompressed
    astarte_linked_list_destroy(&list_handle);
    // Free uncompressed payload
    free(uncompressed);
}

static astarte_err_t uncompress_purge_properties(
    char *data, int data_len, char **output, uLongf *output_len)
{
    char *uncompressed = NULL;
    uLongf uncompressed_len = __builtin_bswap32(*(uint32_t *) data);
    if (uncompressed_len != 0) {
        uncompressed = calloc(uncompressed_len + 1, sizeof(char));
        if (!uncompressed) {
            ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
            return ASTARTE_ERR_OUT_OF_MEMORY;
        }
        int uncompress_res = uncompress((char unsigned *) uncompressed, &uncompressed_len,
            (char unsigned *) data + 4, data_len - 4);
        if (uncompress_res != Z_OK) {
            ESP_LOGE(TAG, "Decompression error %d.", uncompress_res);
            free(uncompressed);
            return ASTARTE_ERR;
        }
    }

    *output = uncompressed;
    *output_len = uncompressed_len;
    return ASTARTE_OK;
}
#endif

static int has_connectivity()
{
    esp_http_client_config_t config
        = {.url = CONFIG_ASTARTE_CONNECTIVITY_TEST_URL,
#if CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
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

#ifdef CONFIG_ASTARTE_USE_PROPERTY_PERSISTENCY
static astarte_interface_t *get_interface_from_introspection(
    astarte_device_handle_t device, const char *name)
{
    astarte_linked_list_iterator_t list_iter;
    astarte_err_t iter_err = astarte_linked_list_iterator_init(&device->introspection, &list_iter);
    while (iter_err != ASTARTE_ERR_NOT_FOUND) {
        astarte_interface_t *interface = NULL;
        astarte_linked_list_iterator_get_item(&list_iter, (void **) &interface);
        if (strcmp(interface->name, name) == 0) {
            return interface;
        }
        iter_err = astarte_linked_list_iterator_advance(&list_iter);
    }
    return NULL;
}
#endif
