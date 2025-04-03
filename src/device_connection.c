/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "device_connection.h"

#if defined(CONFIG_ASTARTE_DEVICE_SDK_NVS)
#include "astarte_zlib.h"
#include "device_caching.h"
#endif

#include <esp_log.h>

/************************************************
 *        Defines, constants and typedef        *
 ***********************************************/

#define TAG "ASTARTE DEVICE CONNECTION"

/************************************************
 *         Static functions declaration         *
 ***********************************************/

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
            ESP_LOGW(TAG, "Called connect function when device is connecting.");
            return ASTARTE_RESULT_MQTT_CLIENT_ALREADY_CONNECTING;
        case DEVICE_CONNECTED:
            ESP_LOGW(TAG, "Called connect function when device is already connected.");
            return ASTARTE_RESULT_MQTT_CLIENT_ALREADY_CONNECTED;
        default: // Other states: (DEVICE_DISCONNECTED)
            break;
    }

    esp_err_t err = esp_mqtt_client_start(device->mqtt_client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start MQTT client: %s", esp_err_to_name(err));
        return ASTARTE_RESULT_MQTT_ERROR;
    }

    ESP_LOGD(TAG, "Device connection state -> MQTT_CONNECTING.");
    device->connection_state = DEVICE_MQTT_CONNECTING;
    return ASTARTE_RESULT_OK;
}

astarte_result_t device_connection_disconnect(astarte_device_handle_t device)
{
    if (device->connection_state == DEVICE_DISCONNECTED) {
        ESP_LOGE(TAG, "Disconnection request for a disconnected client will be ignored.");
        return ASTARTE_RESULT_DEVICE_NOT_READY;
    }

    esp_err_t err = esp_mqtt_client_stop(device->mqtt_client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to stop MQTT client: %s", esp_err_to_name(err));
        return ASTARTE_RESULT_MQTT_ERROR;
    }

    // on_disconnected(device);
    return ASTARTE_RESULT_OK;
}

/************************************************
 *         Static functions definitions         *
 ***********************************************/
