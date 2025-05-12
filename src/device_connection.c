/*
 * (C) Copyright 2024-2025, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "device_connection.h"

#if defined(CONFIG_ASTARTE_DEVICE_SDK_NVS)
#include "astarte_zlib.h"
#include "device_caching.h"
#endif
#include "device_tx.h"

#include "log.h"

/************************************************
 *        Defines, constants and typedef        *
 ***********************************************/

ASTARTE_LOG_MODULE_REGISTER("astarte-device-connection");

/** @brief Wake period for the disconnection timeout in milliseconds. */
#define DISCONNECT_WAKE_PERIOD_MS 100

/************************************************
 *         Static functions declaration         *
 ***********************************************/

/**
 * @brief State machine runner code for the state DEVICE_START_HANDSHAKE.
 *
 * @param[in] device Handle to the device instance.
 */
static void state_machine_start_handshake_run(astarte_device_handle_t device);
/**
 * @brief State machine runner code for the state DEVICE_END_HANDSHAKE.
 *
 * @param[in] device Handle to the device instance.
 */
static void state_machine_end_handshake_run(astarte_device_handle_t device);
/**
 * @brief State machine runner code for the state DEVICE_HANDSHAKE_ERROR.
 *
 * @param[in] device Handle to the device instance.
 */
static void state_machine_handshake_error_run(astarte_device_handle_t device);
/**
 * @brief State machine runner code for the state DEVICE_CONNECTED.
 *
 * @param[in] device Handle to the device instance.
 */
static void state_machine_connected_run(astarte_device_handle_t device);
/**
 * @brief Setup all the MQTT subscriptions for the device.
 *
 * @param[in] device Handle to the device instance.
 * @return ASTARTE_RESULT_OK if successful, otherwise an error code.
 */
static astarte_result_t setup_subscriptions(astarte_device_handle_t device);
/**
 * @brief Send the introspection for the device.
 *
 * @param[in] device Handle to the device instance.
 * @param[in] intr_str The stringified version of the introspection to transmit.
 */
static void send_introspection(astarte_device_handle_t device, char *intr_str);
/**
 * @brief Send the emptycache message to Astarte.
 *
 * @param[in] device Handle to the device instance.
 */
static void send_emptycache(astarte_device_handle_t device);

#if defined(CONFIG_ASTARTE_DEVICE_SDK_NVS)
/**
 * @brief Send the purge properties message for the device owned properties.
 *
 * @param[in] device Handle to the device instance.
 * @return ASTARTE_RESULT_OK if successful, otherwise an error code.
 */
static astarte_result_t send_purge_device_properties(astarte_device_handle_t device);
/**
 * @brief Send the device owned properties to Astarte.
 *
 * @param[in] device Handle to the device instance.
 * @return ASTARTE_RESULT_OK if successful, otherwise an error code.
 */
static astarte_result_t send_device_owned_properties(astarte_device_handle_t device);
/**
 * @brief Send a single property if present in introspection and if device owned.
 *
 * @note If the property is not found in the introspection or if the major version does not match
 * the provided one then the property is deleted from the cache.
 *
 * @param[in] device Handle to the device instance.
 * @param[in] interface_name Name of the interface for the property as retreived from cache.
 * @param[in] path Path for the property as retreived from cache.
 * @param[in] major Major version for the interface of the property as retreived from cache.
 * @param[in] data Data for the property as retreived from cache.
 */
static void send_device_owned_property(astarte_device_handle_t device, const char *interface_name,
    const char *path, uint32_t major, astarte_data_t data);
#endif
/**
 * @brief Subscribe the device to a generic MQTT topic.
 *
 * @param[in] device Handle to the device instance.
 * @param[in] topic Topic to use for the MQTT subscription.
 * @return ASTARTE_RESULT_OK if successful, otherwise an error code.
 */
static astarte_result_t subscribe_to_topic(astarte_device_handle_t device, const char *topic);
/**
 * @brief Verify if all the expected synchronization messages have been delivered to Astarte.
 *
 * @param[in] device Handle to the device instance.
 * @return true if all synchronization messages have been delivered to Astarte, false otherwise.
 */
static bool synchronization_messages_all_delivered(astarte_device_handle_t device);
/**
 * @brief Remove and deallocate an integer from a list if it matches the input integer.
 *
 * @param[in] list Pointer to the list to modify.
 * @return true if a value ha been removed, false if the value has not been found in the list.
 */
static bool remove_if_matching(dlist_t *list, int cmp_value);

/************************************************
 *       Callbacks declaration/definition       *
 ***********************************************/

/************************************************
 *         Global functions definitions         *
 ***********************************************/

astarte_result_t device_connection_connect(astarte_device_handle_t device)
{
    switch (device->connection_state) {
        case DEVICE_MQTT_CONNECTING:
        case DEVICE_START_HANDSHAKE:
        case DEVICE_END_HANDSHAKE:
            ASTARTE_LOG_WRN("Called connect function when device is connecting.");
            return ASTARTE_RESULT_MQTT_CLIENT_ALREADY_CONNECTING;
        case DEVICE_CONNECTED:
            ASTARTE_LOG_WRN("Called connect function when device is already connected.");
            return ASTARTE_RESULT_MQTT_CLIENT_ALREADY_CONNECTED;
        default: // Other states: (DEVICE_DISCONNECTED)
            break;
    }

    esp_err_t err = esp_mqtt_client_start(device->mqtt_client);
    if (err != ESP_OK) {
        ASTARTE_LOG_ERR("Failed to start MQTT client: %s", esp_err_to_name(err));
        return ASTARTE_RESULT_MQTT_ERROR;
    }
    device->mqtt_client_running = true;

    ASTARTE_LOG_DBG("Device connection state -> MQTT_CONNECTING.");
    device->connection_state = DEVICE_MQTT_CONNECTING;
    return ASTARTE_RESULT_OK;
}

astarte_result_t device_connection_disconnect(astarte_device_handle_t device, size_t timeout)
{
    astarte_result_t ares = ASTARTE_RESULT_OK;
    esp_err_t esp_err = ESP_OK;

    if (device->connection_state == DEVICE_DISCONNECTED) {
        goto stop;
    }

    esp_err = esp_mqtt_client_disconnect(device->mqtt_client);
    if (esp_err != ESP_OK) {
        ASTARTE_LOG_ERR("Failed to disconnect the MQTT client: %s", esp_err_to_name(esp_err));
        ares = ASTARTE_RESULT_MQTT_ERROR;
        goto stop;
    }

    const TickType_t disconnect_timeout = timeout / portTICK_PERIOD_MS;
    const TickType_t start_time = xTaskGetTickCount();
    const TickType_t wake_frequency = DISCONNECT_WAKE_PERIOD_MS / portTICK_PERIOD_MS;
    TickType_t last_wake_time = start_time;
    while (device->connection_state != DEVICE_DISCONNECTED) {
        if (xTaskGetTickCount() - start_time > disconnect_timeout) {
            ares = ASTARTE_RESULT_TIMEOUT;
            goto stop;
        }
        vTaskDelayUntil(&last_wake_time, wake_frequency);
    }

stop:
    if (device->mqtt_client_running) {
        esp_err = esp_mqtt_client_stop(device->mqtt_client);
        if (esp_err != ESP_OK) {
            ASTARTE_LOG_ERR("Failed to stop the MQTT client: %s", esp_err_to_name(esp_err));
            ares = ASTARTE_RESULT_MQTT_ERROR;
        }
        device->mqtt_client_running = false;
        device->connection_state = DEVICE_DISCONNECTED;
    }
    return ares;
}

void device_connection_on_connected_handler(
    astarte_device_handle_t device, esp_mqtt_event_handle_t mqtt_event)
{
    ASTARTE_LOG_DBG("Device connection state -> START_HANDSHAKE.");
    device->connection_state = DEVICE_START_HANDSHAKE;
    device->mqtt_session_present_flag = mqtt_event->session_present;
}

void device_connection_on_disconnected_handler(astarte_device_handle_t device)
{
    ASTARTE_LOG_DBG("Device connection state -> DISCONNECTED.");
    device->connection_state = DEVICE_DISCONNECTED;

    dlist_destroy_and_release(&device->synchronization_in_msg_ids);
    dlist_destroy_and_release(&device->synchronization_out_msg_ids);

    if (device->disconnection_cbk) {
        astarte_device_disconnection_event_t event = {
            .device = device,
            .user_data = device->cbk_user_data,
        };
        device->disconnection_cbk(event);
    }
}

void device_connection_on_publish_handler(
    astarte_device_handle_t device, esp_mqtt_event_handle_t mqtt_event)
{
    if (device->connection_state != DEVICE_CONNECTED) {
        ASTARTE_LOG_DBG("Add published message to incoming list, ID: %d.", mqtt_event->msg_id);
        dlist_append_int(&device->synchronization_in_msg_ids, mqtt_event->msg_id);
    }
}

void device_connection_on_subscribed_handler(
    astarte_device_handle_t device, esp_mqtt_event_handle_t mqtt_event)
{
    if (mqtt_event->error_handle->error_type != MQTT_ERROR_TYPE_NONE) {
        device->subscription_failure = true;
        ASTARTE_LOG_ERR("Failed subscription, error: %d.", mqtt_event->error_handle->error_type);
    }

    ASTARTE_LOG_DBG("Add subscribed message to incoming list, ID: %d.", mqtt_event->msg_id);
    dlist_append_int(&device->synchronization_in_msg_ids, mqtt_event->msg_id);
}

void device_connection_poll(astarte_device_handle_t device)
{
    switch (device->connection_state) {
        case DEVICE_DISCONNECTED:
        case DEVICE_MQTT_CONNECTING:
            break;
        case DEVICE_START_HANDSHAKE:
            state_machine_start_handshake_run(device);
            break;
        case DEVICE_END_HANDSHAKE:
            state_machine_end_handshake_run(device);
            break;
        case DEVICE_HANDSHAKE_ERROR:
            state_machine_handshake_error_run(device);
            break;
        case DEVICE_CONNECTED:
            state_machine_connected_run(device);
            break;
        default: // nop
            break;
    }
}

/************************************************
 *         Static functions definitions         *
 ***********************************************/

static void state_machine_start_handshake_run(astarte_device_handle_t device)
{
    device->subscription_failure = false;

    char *intr_str = NULL;
    size_t intr_str_size = introspection_get_string_size(&device->introspection);

    intr_str = calloc(intr_str_size, sizeof(char));
    if (!intr_str) {
        ASTARTE_LOG_ERR("Out of memory %s: %d", __FILE__, __LINE__);
        ASTARTE_LOG_DBG("Device connection state -> HANDSHAKE_ERROR.");
        device->connection_state = DEVICE_HANDSHAKE_ERROR;
        goto exit;
    }
    introspection_fill_string(&device->introspection, intr_str, intr_str_size);

#if defined(CONFIG_ASTARTE_DEVICE_SDK_NVS)
    if ((device->mqtt_session_present_flag != 0) && device->synchronization_completed) {
        astarte_result_t ares = device_caching_introspection_check(intr_str, intr_str_size);
        if (ares == ASTARTE_RESULT_OK) {
            ASTARTE_LOG_DBG("Device connection state -> END_HANDSHAKE.");
            device->connection_state = DEVICE_END_HANDSHAKE;
            goto exit;
        }
    }
#else
    if ((device->mqtt_session_present_flag != 0) && device->synchronization_completed) {
        ASTARTE_LOG_DBG("Device connection state -> END_HANDSHAKE.");
        device->connection_state = DEVICE_END_HANDSHAKE;
        goto exit;
    }
#endif

    ASTARTE_LOG_DBG("Setup subscriptions.");
    if (setup_subscriptions(device) != ASTARTE_RESULT_OK) {
        ASTARTE_LOG_DBG("Device connection state -> HANDSHAKE_ERROR.");
        device->connection_state = DEVICE_HANDSHAKE_ERROR;
        goto exit;
    }
    ASTARTE_LOG_DBG("Send introspection.");
    send_introspection(device, intr_str);
    ASTARTE_LOG_DBG("Send empty cache.");
    send_emptycache(device);
#if defined(CONFIG_ASTARTE_DEVICE_SDK_NVS)
    ASTARTE_LOG_DBG("Send purge device properties.");
    if (send_purge_device_properties(device) != ASTARTE_RESULT_OK) {
        ASTARTE_LOG_DBG("Device connection state -> HANDSHAKE_ERROR.");
        device->connection_state = DEVICE_HANDSHAKE_ERROR;
        goto exit;
    }
    ASTARTE_LOG_DBG("Send device owned properities.");
    if (send_device_owned_properties(device) != ASTARTE_RESULT_OK) {
        ASTARTE_LOG_DBG("Device connection state -> HANDSHAKE_ERROR.");
        device->connection_state = DEVICE_HANDSHAKE_ERROR;
        goto exit;
    }
#endif
    ASTARTE_LOG_DBG("Device connection state -> END_HANDSHAKE.");
    device->connection_state = DEVICE_END_HANDSHAKE;

exit:
    free(intr_str);
}

static void state_machine_end_handshake_run(astarte_device_handle_t device)
{
    char *intr_str = NULL;
    if (device->subscription_failure) {
        ASTARTE_LOG_ERR("Subscription request has been denied.");
        ASTARTE_LOG_DBG("Device connection state -> HANDSHAKE_ERROR.");
        device->connection_state = DEVICE_HANDSHAKE_ERROR;
        goto exit;
    }
    if (synchronization_messages_all_delivered(device)) {
        ASTARTE_LOG_DBG("Device synchronization completed.");
        device->synchronization_completed = true;
        ASTARTE_LOG_DBG("Device connection state -> CONNECTED.");
        device->connection_state = DEVICE_CONNECTED;

#if defined(CONFIG_ASTARTE_DEVICE_SDK_NVS)
        astarte_result_t ares = device_caching_synchronization_set(true);
        if (ares != ASTARTE_RESULT_OK) {
            ASTARTE_LOG_ERR("Synchronization state set failure %s.", astarte_result_to_name(ares));
        }

        size_t intr_str_size = introspection_get_string_size(&device->introspection);
        intr_str = calloc(intr_str_size, sizeof(char));
        if (!intr_str) {
            ASTARTE_LOG_ERR("Out of memory %s: %d", __FILE__, __LINE__);
            ASTARTE_LOG_DBG("Device connection state -> HANDSHAKE_ERROR.");
            device->connection_state = DEVICE_HANDSHAKE_ERROR;
            goto exit;
        }
        introspection_fill_string(&device->introspection, intr_str, intr_str_size);

        ares = device_caching_introspection_check(intr_str, intr_str_size);
        if (ares == ASTARTE_RESULT_DEVICE_CACHING_OUTDATED_INTROSPECTION) {
            ASTARTE_LOG_DBG("Introspection requires updating.");
            ares = device_caching_introspection_set(intr_str, intr_str_size);
        }
        if (ares != ASTARTE_RESULT_OK) {
            ASTARTE_LOG_DBG("Introspection update failed: %s", astarte_result_to_name(ares));
        }
#endif

        if (device->connection_cbk) {
            astarte_device_connection_event_t event = {
                .device = device,
                .user_data = device->cbk_user_data,
            };
            device->connection_cbk(event);
        }
    }

exit:
    free(intr_str);
}

static void state_machine_handshake_error_run(astarte_device_handle_t device)
{
    if (device->synchronization_completed) {
        device->synchronization_completed = false;
#if defined(CONFIG_ASTARTE_DEVICE_SDK_NVS)
        astarte_result_t ares = device_caching_synchronization_set(false);
        if (ares != ASTARTE_RESULT_OK) {
            ASTARTE_LOG_ERR("Synchronization state set failure %s.", astarte_result_to_name(ares));
        }
#endif
    } else {
        dlist_destroy_and_release(&device->synchronization_in_msg_ids);
        dlist_destroy_and_release(&device->synchronization_out_msg_ids);
    }
    if (xTaskGetTickCount() > device->reconnection_timepoint) {
        // Repeat the handshake procedure
        device->connection_state = DEVICE_START_HANDSHAKE;

        // Update backoff for the next attempt
        uint32_t next_backoff_ms = 0;
        backoff_get_next(&device->backoff_ctx, &next_backoff_ms);
        device->reconnection_timepoint
            = xTaskGetTickCount() + (next_backoff_ms / portTICK_PERIOD_MS);
    }
}

static void state_machine_connected_run(astarte_device_handle_t device)
{
    backoff_context_init(&device->backoff_ctx,
        CONFIG_ASTARTE_DEVICE_SDK_RECONNECTION_ASTARTE_BACKOFF_INITIAL_MS,
        CONFIG_ASTARTE_DEVICE_SDK_RECONNECTION_ASTARTE_BACKOFF_MAX_MS, true);
}

static astarte_result_t setup_subscriptions(astarte_device_handle_t device)
{
    astarte_result_t ares = ASTARTE_RESULT_OK;
    char *topic = NULL;

    ares = subscribe_to_topic(device, device->control_consumer_prop_topic);
    if (ares != ASTARTE_RESULT_OK) {
        ASTARTE_LOG_ERR("Subscription failure %s.", astarte_result_to_name(ares));
        goto exit;
    }

    introspection_iterator_t iterator = { 0 };
    ares = introspection_iterator_init(&device->introspection, &iterator);
    while (ares == ASTARTE_RESULT_OK) {
        const astarte_interface_t *interface = introspection_iterator_get_interface(&iterator);

        if (interface->ownership == ASTARTE_INTERFACE_OWNERSHIP_SERVER) {
            size_t topic_len = strlen(CONFIG_ASTARTE_DEVICE_SDK_REALM_NAME "///#")
                + ASTARTE_DEVICE_ID_LEN + strlen(interface->name);
            topic = calloc(topic_len + 1, sizeof(char));
            if (!topic) {
                ASTARTE_LOG_ERR("Out of memory %s: %d", __FILE__, __LINE__);
                ares = ASTARTE_RESULT_OUT_OF_MEMORY;
                goto exit;
            }

            int ret
                = snprintf(topic, topic_len + 1, CONFIG_ASTARTE_DEVICE_SDK_REALM_NAME "/%s/%s/#",
                    device->device_id, interface->name);
            if (ret != topic_len) {
                ASTARTE_LOG_ERR("Error encoding MQTT topic.");
                ares = ASTARTE_RESULT_INTERNAL_ERROR;
                goto exit;
            }

            ares = subscribe_to_topic(device, topic);
            if (ares != ASTARTE_RESULT_OK) {
                ASTARTE_LOG_ERR("Subscription failure %s.", astarte_result_to_name(ares));
                goto exit;
            }
            free(topic);
            topic = NULL;
        }

        ares = introspection_iterator_advance(&iterator);
    }

    if (ares == ASTARTE_RESULT_NOT_FOUND) {
        ares = ASTARTE_RESULT_OK;
    }

exit:
    free(topic);
    return ares;
}

static void send_introspection(astarte_device_handle_t device, char *intr_str)
{
    const char *topic = device->base_topic;
    ASTARTE_LOG_DBG("Publishing introspection: %s", intr_str);
    size_t intr_str_len = strlen(intr_str);
    if (intr_str_len > INT_MAX) {
        ASTARTE_LOG_ERR("Introspection is too long, can't be published with MQTT.");
        return;
    }
    int msg_id
        = esp_mqtt_client_publish(device->mqtt_client, topic, intr_str, (int) intr_str_len, 2, 0);
    if (msg_id < 0) {
        return;
    }
    ASTARTE_LOG_DBG("Add introspection message to list, ID: %d.", msg_id);
    dlist_append_int(&device->synchronization_out_msg_ids, msg_id);
}

static void send_emptycache(astarte_device_handle_t device)
{
    const char *topic = device->control_empty_cache_topic;
    ASTARTE_LOG_DBG("Sending emptyCache to %s", topic);
    int msg_id = esp_mqtt_client_publish(device->mqtt_client, topic, "1", strlen("1"), 2, 0);
    if (msg_id < 0) {
        return;
    }
    ASTARTE_LOG_DBG("Add empty cache message to list, ID: %d.", msg_id);
    dlist_append_int(&device->synchronization_out_msg_ids, msg_id);
}

#if defined(CONFIG_ASTARTE_DEVICE_SDK_NVS)
static astarte_result_t send_purge_device_properties(astarte_device_handle_t device)
{
    astarte_result_t ares = ASTARTE_RESULT_OK;
    char *intr_str = NULL;
    uint8_t *payload = NULL;

    size_t intr_str_size = 0U;
    ares = device_caching_property_get_device_properties_string(
        &device->introspection, NULL, &intr_str_size);
    if (ares != ASTARTE_RESULT_OK) {
        ASTARTE_LOG_ERR("Error getting cached properties string: %s", astarte_result_to_name(ares));
        goto exit;
    }
    if (intr_str_size != 0) {
        intr_str = calloc(intr_str_size, sizeof(char));
        if (!intr_str) {
            ASTARTE_LOG_ERR("Out of memory %s: %d", __FILE__, __LINE__);
            ares = ASTARTE_RESULT_OUT_OF_MEMORY;
            goto exit;
        }

        ares = device_caching_property_get_device_properties_string(
            &device->introspection, intr_str, &intr_str_size);
        if (ares != ASTARTE_RESULT_OK) {
            ASTARTE_LOG_ERR("Can't get cached properties string: %s", astarte_result_to_name(ares));
            goto exit;
        }
    }

    // Estimate compression result size and payload size
    char *compression_input = intr_str;
    size_t compression_input_len = (compression_input) ? (intr_str_size - 1) : 0;
    uLongf compressed_len = compressBound(compression_input_len);
    // Allocate enough memory for the payload
    size_t payload_size = 4 + compressed_len;
    payload = calloc(payload_size, sizeof(uint8_t));
    if (!payload) {
        ASTARTE_LOG_ERR("Out of memory %s: %d", __FILE__, __LINE__);
        ares = ASTARTE_RESULT_OUT_OF_MEMORY;
        goto exit;
    }
    // Fill the first 32 bits of the payload
    uint32_t *payload_uint32 = (uint32_t *) payload;
    *payload_uint32 = __builtin_bswap32(compression_input_len);
    // Perform the compression and store result in the payload
    int compress_res = astarte_zlib_compress((char unsigned *) &payload[4], &compressed_len,
        (char unsigned *) compression_input, compression_input_len);
    if (compress_res != Z_OK) {
        ASTARTE_LOG_ERR("Error compressing the purge properties message %d.", compress_res);
        ares = ASTARTE_RESULT_INTERNAL_ERROR;
        goto exit;
    }
    // 'astarte_zlib_compress' updates 'compressed_len' to the actual size of the compressed data
    payload_size = 4 + compressed_len;
    // Check if payload is not too large for a MQTT message
    if (payload_size > INT_MAX) {
        // MQTT supports sending a maximum payload length of INT_MAX
        ASTARTE_LOG_ERR("Purge properties payload is too long for a single MQTT message.");
        ares = ASTARTE_RESULT_MQTT_ERROR;
        goto exit;
    }

    // Transmit the payload
    const char *topic = device->control_producer_prop_topic;
    const int qos = 2;
    ASTARTE_LOG_INF("Sending purge properties to: '%s', with uncompressed content: '%s'", topic,
        (compression_input) ? compression_input : "");
    esp_mqtt_client_publish(
        device->mqtt_client, topic, (const char *) payload, (int) payload_size, qos, 0);

exit:
    free(intr_str);
    free(payload);
    return ares;
}

static astarte_result_t send_device_owned_properties(astarte_device_handle_t device)
{
    astarte_result_t ares = ASTARTE_RESULT_OK;
    device_caching_property_iterator_t iter = { 0 };
    char *interface_name = NULL;
    char *path = NULL;
    astarte_data_t data = { 0 };

    ares = device_caching_property_iterator_init(&iter);
    if ((ares != ASTARTE_RESULT_OK) && (ares != ASTARTE_RESULT_NOT_FOUND)) {
        ASTARTE_LOG_ERR("Properties iterator init failed: %s", astarte_result_to_name(ares));
        return ares;
    }

    while (ares != ASTARTE_RESULT_NOT_FOUND) {
        size_t interface_name_size = 0U;
        size_t path_size = 0U;
        ares = device_caching_property_iterator_get(
            &iter, NULL, &interface_name_size, NULL, &path_size);
        if (ares != ASTARTE_RESULT_OK) {
            ASTARTE_LOG_ERR("Properties iterator get error: %s", astarte_result_to_name(ares));
            goto end;
        }

        // Allocate space for the name and path
        interface_name = calloc(interface_name_size, sizeof(char));
        path = calloc(path_size, sizeof(char));
        if (!interface_name || !path) {
            ASTARTE_LOG_ERR("Out of memory %s: %d", __FILE__, __LINE__);
            ares = ASTARTE_RESULT_OUT_OF_MEMORY;
            goto end;
        }

        ares = device_caching_property_iterator_get(
            &iter, interface_name, &interface_name_size, path, &path_size);
        if (ares != ASTARTE_RESULT_OK) {
            ASTARTE_LOG_ERR("Properties iterator get error: %s", astarte_result_to_name(ares));
            goto end;
        }

        uint32_t major = 0U;
        ares = device_caching_property_load(interface_name, path, &major, &data);
        if (ares != ASTARTE_RESULT_OK) {
            ASTARTE_LOG_ERR("Properties load property error: %s", astarte_result_to_name(ares));
            goto end;
        }

        send_device_owned_property(device, interface_name, path, major, data);

        free(interface_name);
        interface_name = NULL;
        free(path);
        path = NULL;
        device_caching_property_destroy_loaded(data);
        data = (astarte_data_t) { 0 };

        ares = device_caching_property_iterator_next(&iter);
        if ((ares != ASTARTE_RESULT_OK) && (ares != ASTARTE_RESULT_NOT_FOUND)) {
            ASTARTE_LOG_ERR("Iterator next error: %s", astarte_result_to_name(ares));
            goto end;
        }
    }

end:
    device_caching_property_iterator_terminate(&iter);
    free(interface_name);
    free(path);
    device_caching_property_destroy_loaded(data);
    return (ares == ASTARTE_RESULT_NOT_FOUND) ? ASTARTE_RESULT_OK : ares;
}

static void send_device_owned_property(astarte_device_handle_t device, const char *interface_name,
    const char *path, uint32_t major, astarte_data_t data)
{
    astarte_result_t ares = ASTARTE_RESULT_OK;
    const astarte_interface_t *interface = introspection_get(
        &device->introspection, interface_name);
    if ((!interface) || (interface->major_version != major)) {
        ASTARTE_LOG_DBG("Removing property from storage: '%s%s'", interface_name, path);

        ares = device_caching_property_delete(interface_name, path);
        if ((ares != ASTARTE_RESULT_OK) && (ares != ASTARTE_RESULT_NOT_FOUND)) {
            if (ares != ASTARTE_RESULT_OK) {
                ASTARTE_LOG_ERR(
                    "Failed deleting the cached property: %s", astarte_result_to_name(ares));
            }
        }
        return;
    }

    if (interface->ownership == ASTARTE_INTERFACE_OWNERSHIP_DEVICE) {
        ares = device_tx_stream_individual(device, interface_name, path, data, NULL);
        if (ares != ASTARTE_RESULT_OK) {
            ASTARTE_LOG_ERR("Failed sending cached property: %s", astarte_result_to_name(ares));
        }
    }
}
#endif

static astarte_result_t subscribe_to_topic(astarte_device_handle_t device, const char *topic)
{
    ASTARTE_LOG_DBG("Subscribing to: %s", topic);
    int msg_id = esp_mqtt_client_subscribe(device->mqtt_client, topic, 2);
    if (msg_id < 0) {
        return ASTARTE_RESULT_MQTT_ERROR;
    }
    ASTARTE_LOG_DBG("Add subscription message to list, ID: %d.", msg_id);
    return dlist_append_int(&device->synchronization_out_msg_ids, msg_id);
}

static bool synchronization_messages_all_delivered(astarte_device_handle_t device)
{
    astarte_result_t ares = ASTARTE_RESULT_OK;
    int *msg_id = NULL;
    dlist_iterator_t iterator = { 0 };

    ares = dlist_iterator_init(&device->synchronization_out_msg_ids, &iterator);
    while (ares == ASTARTE_RESULT_OK) {
        msg_id = dlist_iterator_get_item(&iterator);
        if (remove_if_matching(&device->synchronization_in_msg_ids, *msg_id)) {
            dlist_iterator_remove_item(&iterator);
            free(msg_id);
            if (dlist_is_empty(&device->synchronization_out_msg_ids)) {
                break;
            }
        } else {
            ares = dlist_iterator_advance(&iterator);
        }
    }

    if (dlist_is_empty(&device->synchronization_out_msg_ids)) {
        return true;
    }
    return false;
}

static bool remove_if_matching(dlist_t *list, int cmp_value)
{
    astarte_result_t ares = ASTARTE_RESULT_OK;
    dlist_iterator_t iterator = { 0 };
    int *list_value = NULL;
    ares = dlist_iterator_init(list, &iterator);
    while (ares == ASTARTE_RESULT_OK) {
        list_value = dlist_iterator_get_item(&iterator);
        if (*list_value == cmp_value) {
            ASTARTE_LOG_DBG("Removing and freeing integer from list, value: %d.", cmp_value);
            dlist_iterator_remove_item(&iterator);
            free(list_value);
            return true;
        }
        ares = dlist_iterator_advance(&iterator);
    }
    return false;
}
