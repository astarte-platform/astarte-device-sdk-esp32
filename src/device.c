/*
 * (C) Copyright 2024-2025, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "astarte_device_sdk/device.h"
#include "device_private.h"

#include "device_caching.h"
#include "pairing_private.h"
#include "device_connection.h"

#include <esp_log.h>
#include <mqtt_client.h>
#include <string.h>

#if defined(CONFIG_MBEDTLS_CERTIFICATE_BUNDLE)
#include <esp_crt_bundle.h>
#endif

/************************************************
 *        Defines, constants and typedef        *
 ***********************************************/

#define TAG "ASTARTE DEVICE"

#define DEVICE_CACHING_NAMESPACE "astarte_caching"

/************************************************
 *         Static functions declaration         *
 ***********************************************/

/**
 * @brief Initialize the device introspection.
 *
 * @param[in] device Handle to the device instance.
 * @param[in] interfaces A list of pointers to interfaces.
 * @param[in] interfaces_size Number of interfaces in the list.
 * @return ASTARTE_RESULT_OK on success, an error code otherwise.
 */
static astarte_result_t initialize_introspection(
    astarte_device_handle_t device, const astarte_interface_t **interfaces, size_t interfaces_size);
/**
 * @brief Initialize MQTT topics.
 *
 * @param[in] device Handle to the device instance.
 * @return ASTARTE_RESULT_OK on success, an error code otherwise.
 */
static astarte_result_t initialize_mqtt_topics(astarte_device_handle_t device);

/************************************************
 *       Callbacks declaration/definition       *
 ***********************************************/

static void mqtt_event_handler(
    void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    (void) base;

    esp_mqtt_event_handle_t event = event_data;
    // astarte_device_handle_t device = (astarte_device_handle_t) handler_args;
    switch ((esp_mqtt_event_id_t) event_id) {
        case MQTT_EVENT_BEFORE_CONNECT:
            ESP_LOGD(TAG, "MQTT_EVENT_BEFORE_CONNECT");
            break;

        case MQTT_EVENT_CONNECTED:
            ESP_LOGD(TAG, "MQTT_EVENT_CONNECTED");
            // on_connected(device, event->session_present);
            break;

        case MQTT_EVENT_DATA:
            ESP_LOGD(TAG, "MQTT_EVENT_DATA");
            // on_incoming(device, event->topic, event->topic_len, event->data, event->data_len);
            break;

        case MQTT_EVENT_DELETED:
            ESP_LOGD(TAG, "MQTT_EVENT_DELETED");
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGD(TAG, "MQTT_EVENT_DISCONNECTED");
            // on_disconnected(device);
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGD(TAG, "MQTT_EVENT_ERROR");
            // if (event->error_handle->error_type == MQTT_ERROR_TYPE_ESP_TLS) {
            //     on_certificate_error(device);
            // }
            break;

        case MQTT_EVENT_PUBLISHED:
            ESP_LOGD(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGD(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGD(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;

        case MQTT_USER_EVENT:
            ESP_LOGD(TAG, "MQTT_USER_EVENT");
            break;

        default:
            // Handle MQTT_EVENT_ANY introduced in esp-idf 3.2
            break;
    }
}
/************************************************
 *         Global functions definitions         *
 ***********************************************/

astarte_result_t astarte_device_new(astarte_device_config_t *cfg, astarte_device_handle_t *device)
{
    astarte_result_t ares = ASTARTE_RESULT_OK;
    astarte_device_handle_t handle = NULL;

    if (!cfg || !device) {
        ESP_LOGE(TAG, "Received NULL reference for configuration or device handle");
        ares = ASTARTE_RESULT_INVALID_PARAM;
        goto failure;
    }

    handle = calloc(1, sizeof(struct astarte_device));
    if (!handle) {
        ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
        ares = ASTARTE_RESULT_OUT_OF_MEMORY;
        goto failure;
    }

    memcpy(handle->device_id, cfg->device_id, ASTARTE_DEVICE_ID_LEN + 1);
    memcpy(handle->cred_secr, cfg->cred_secr, ASTARTE_PAIRING_CRED_SECR_LEN + 1);
    handle->connection_cbk = cfg->connection_cbk;
    handle->disconnection_cbk = cfg->disconnection_cbk;
    handle->datastream_individual_cbk = cfg->datastream_individual_cbk;
    handle->datastream_object_cbk = cfg->datastream_object_cbk;
    handle->property_set_cbk = cfg->property_set_cbk;
    handle->property_unset_cbk = cfg->property_unset_cbk;
    handle->cbk_user_data = cfg->cbk_user_data;

    handle->synchronization_completed = false;
#if defined(CONFIG_ASTARTE_DEVICE_SDK_NVS)
    ESP_LOGD(TAG, "Getting stored synchronization");
    device_caching_t caching_handle = { 0 };
    ares = device_caching_open_namespace(&caching_handle, DEVICE_CACHING_NAMESPACE);
    if ((ares != ASTARTE_RESULT_OK) && (ares != ASTARTE_RESULT_NOT_FOUND)) {
        ESP_LOGE(TAG, "Failed opening caching %s.", astarte_result_to_name(ares));
        goto failure;
    }

    ares = device_caching_synchronization_get(caching_handle, &handle->synchronization_completed);
    device_caching_close_namespace(caching_handle);
    if ((ares != ASTARTE_RESULT_OK) && (ares != ASTARTE_RESULT_NOT_FOUND)) {
        ESP_LOGE(TAG, "Synchronization state getter failure %s.", astarte_result_to_name(ares));
        goto failure;
    }
    ESP_LOGD(TAG, "Device synchronization completed '%d'", handle->synchronization_completed);
#endif
    handle->connection_state = DEVICE_DISCONNECTED;

    ESP_LOGD(TAG, "Initializing introspection");
    ares = initialize_introspection(handle, cfg->interfaces, cfg->interfaces_size);
    if (ares != ASTARTE_RESULT_OK) {
        goto failure;
    }

    ares = initialize_mqtt_topics(handle);
    if (ares != ASTARTE_RESULT_OK) {
        ESP_LOGE(TAG, "Failed initialization for MQTT topics %s.", astarte_result_to_name(ares));
        goto failure;
    }

    ESP_LOGD(TAG, "Getting MQTT broker hostname and port");
    char broker_url[PAIRING_MAX_BROKER_URL_LEN + 1] = { 0 };
    ares = pairing_get_mqtt_broker_url(handle->device_id, handle->cred_secr, broker_url);
    if (ares != ASTARTE_RESULT_OK) {
        ESP_LOGE(TAG, "Failed in fetching the MQTT broker URL %s.", astarte_result_to_name(ares));
        goto failure;
    }

    tls_credentials_client_crt_t client_crt = { 0 };
    ares = pairing_get_client_certificate(handle->device_id, handle->cred_secr, &client_crt);
    if (ares != ASTARTE_RESULT_OK) {
        ESP_LOGE(TAG, "Failed getting the client TLS cert: %s.", astarte_result_to_name(ares));
        return ares;
    }

    ESP_LOGW(TAG, "%s", client_crt.crt_pem);
    ESP_LOGW(TAG, "%s", client_crt.privkey_pem);

    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = broker_url,
#if defined(CONFIG_MBEDTLS_CERTIFICATE_BUNDLE)
        .broker.verification.crt_bundle_attach = esp_crt_bundle_attach,
#endif
        .credentials.authentication.certificate = client_crt.crt_pem,
        .credentials.authentication.key = (const char *) client_crt.privkey_pem,
#if defined(CONFIG_ASTARTE_DEVICE_SDK_NVS)
        .session.disable_clean_session = true,
#endif
    };
    esp_mqtt_client_handle_t mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (!mqtt_client) {
        ESP_LOGE(TAG, "Error in esp_mqtt_client_init");
        goto failure;
    }

    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, handle);
    handle->mqtt_client = mqtt_client;

    // Initialize the handle data to be used during the handshake with Astarte
    handle->mqtt_session_present_flag = 0;
    backoff_context_init(&handle->backoff_ctx,
        CONFIG_ASTARTE_DEVICE_SDK_RECONNECTION_ASTARTE_BACKOFF_INITIAL_MS,
        CONFIG_ASTARTE_DEVICE_SDK_RECONNECTION_ASTARTE_BACKOFF_MAX_MS, true);

    *device = handle;

    return ares;

failure:

    if (handle) {
        introspection_free(handle->introspection);
    }
    free(handle);
    return ares;
}

astarte_result_t astarte_device_destroy(astarte_device_handle_t device)
{
    // astarte_result_t ares = ASTARTE_RESULT_OK;

    if (!device) {
        return ASTARTE_RESULT_OK;
    }

    // TODO: disconnect from MQTT if not already disconnected

    // TODO: clear all pending MQTT messages (if possible)

    introspection_free(device->introspection);
    free(device);
    return ASTARTE_RESULT_OK;
}

astarte_result_t astarte_device_add_interface(
    astarte_device_handle_t device, const astarte_interface_t *interface)
{
    if (!device || !interface) {
        ESP_LOGE(TAG, "Received NULL reference for device handle or interface");
        return ASTARTE_RESULT_INVALID_PARAM;
    }
    return introspection_update(&device->introspection, interface);
}

astarte_result_t astarte_device_connect(astarte_device_handle_t device)
{
    if (!device) {
        ESP_LOGE(TAG, "Received NULL reference for device handle");
        return ASTARTE_RESULT_INVALID_PARAM;
    }
    return device_connection_connect(device);
}

astarte_result_t astarte_device_disconnect(astarte_device_handle_t device)
{
    if (!device) {
        ESP_LOGE(TAG, "Received NULL reference for device handle");
        return ASTARTE_RESULT_INVALID_PARAM;
    }
    return device_connection_disconnect(device);
}

astarte_result_t astarte_device_poll(astarte_device_handle_t device)
{
    if (!device) {
        ESP_LOGE(TAG, "Received NULL reference for device handle");
        return ASTARTE_RESULT_INVALID_PARAM;
    }
    return ASTARTE_RESULT_OK;
}

#if defined(CONFIG_ASTARTE_DEVICE_SDK_NVS)
astarte_result_t astarte_device_get_property(astarte_device_handle_t device,
    const char *interface_name, const char *path, astarte_device_property_loader_cbk_t loader_cbk,
    void *user_data)
{
    astarte_result_t ares = ASTARTE_RESULT_OK;
    if (!device || !interface_name || !path || !loader_cbk) {
        ESP_LOGE(TAG, "Received a NULL reference for a required input parameter.");
        return ASTARTE_RESULT_INVALID_PARAM;
    }

    device_caching_t caching_handle = { 0 };
    ares = device_caching_open_namespace(&caching_handle, DEVICE_CACHING_NAMESPACE);
    if ((ares != ASTARTE_RESULT_OK) && (ares != ASTARTE_RESULT_NOT_FOUND)) {
        ESP_LOGE(TAG, "Failed opening caching %s.", astarte_result_to_name(ares));
        return ares;
    }

    astarte_data_t data = { 0 };
    uint32_t out_major = 0U;
    ares = device_caching_property_load(caching_handle, interface_name, path, &out_major, &data);
    device_caching_close_namespace(caching_handle);
    if (ares != ASTARTE_RESULT_OK) {
        if (ares != ASTARTE_RESULT_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed getting property: %s.", astarte_result_to_name(ares));
        }
        return ares;
    }

    astarte_device_property_loader_event_t event = { .device = device,
        .interface_name = interface_name,
        .path = path,
        .data = data,
        .user_data = user_data };
    loader_cbk(event);

    device_caching_property_destroy_loaded(data);
    return ares;
}
#endif

/************************************************
 *         Static functions definitions         *
 ***********************************************/

static astarte_result_t initialize_introspection(
    astarte_device_handle_t device, const astarte_interface_t **interfaces, size_t interfaces_size)
{
    device->introspection = introspection_new();
    if (interfaces) {
        for (size_t i = 0; i < interfaces_size; i++) {
            astarte_result_t ares = introspection_add(&device->introspection, interfaces[i]);
            if (ares != ASTARTE_RESULT_OK) {
                ESP_LOGE(TAG, "Introspection add failure %s.", astarte_result_to_name(ares));
                return ares;
            }
        }
    }
    return ASTARTE_RESULT_OK;
}

static astarte_result_t initialize_mqtt_topics(astarte_device_handle_t device)
{
    int snprintf_rc = snprintf(
        device->base_topic, MQTT_BASE_TOPIC_LEN + 1, MQTT_TOPIC_PREFIX "%s", device->device_id);
    if (snprintf_rc != MQTT_BASE_TOPIC_LEN) {
        ESP_LOGE(TAG, "Error encoding base topic.");
        return ASTARTE_RESULT_INTERNAL_ERROR;
    }
    snprintf_rc = snprintf(device->control_topic, MQTT_CONTROL_TOPIC_LEN + 1,
        MQTT_TOPIC_PREFIX "%s" MQTT_CONTROL_TOPIC_SUFFIX, device->device_id);
    if (snprintf_rc != MQTT_CONTROL_TOPIC_LEN) {
        ESP_LOGE(TAG, "Error encoding base control topic.");
        return ASTARTE_RESULT_INTERNAL_ERROR;
    }
    snprintf_rc
        = snprintf(device->control_empty_cache_topic, MQTT_CONTROL_EMPTY_CACHE_TOPIC_LEN + 1,
            MQTT_TOPIC_PREFIX "%s" MQTT_CONTROL_EMPTY_CACHE_TOPIC_SUFFIX, device->device_id);
    if (snprintf_rc != MQTT_CONTROL_EMPTY_CACHE_TOPIC_LEN) {
        ESP_LOGE(TAG, "Error encoding empty cache publish topic.");
        return ASTARTE_RESULT_INTERNAL_ERROR;
    }
    snprintf_rc
        = snprintf(device->control_consumer_prop_topic, MQTT_CONTROL_CONSUMER_PROP_TOPIC_LEN + 1,
            MQTT_TOPIC_PREFIX "%s" MQTT_CONTROL_CONSUMER_PROP_TOPIC_SUFFIX, device->device_id);
    if (snprintf_rc != MQTT_CONTROL_CONSUMER_PROP_TOPIC_LEN) {
        ESP_LOGE(TAG, "Error encoding Astarte purge properties topic.");
        return ASTARTE_RESULT_INTERNAL_ERROR;
    }
    snprintf_rc
        = snprintf(device->control_producer_prop_topic, MQTT_CONTROL_PRODUCER_PROP_TOPIC_LEN + 1,
            MQTT_TOPIC_PREFIX "%s" MQTT_CONTROL_PRODUCER_PROP_TOPIC_SUFFIX, device->device_id);
    if (snprintf_rc != MQTT_CONTROL_PRODUCER_PROP_TOPIC_LEN) {
        ESP_LOGE(TAG, "Error encoding device purge properties topic.");
        return ASTARTE_RESULT_INTERNAL_ERROR;
    }
    return ASTARTE_RESULT_OK;
}
