/*
 * (C) Copyright 2024-2025, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DEVICE_CONNECTION_H
#define DEVICE_CONNECTION_H

/**
 * @file device_connection.h
 * @brief Device connection header.
 */

#include "astarte_device_sdk/astarte.h"
#include "astarte_device_sdk/device.h"
#include "astarte_device_sdk/result.h"

#include "device_private.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Connect a device to Astarte.
 *
 * @param[in] device Device instance to connect to Astarte.
 * @return ASTARTE_RESULT_OK if successful, otherwise an error code.
 */
astarte_result_t device_connection_connect(astarte_device_handle_t device);

/**
 * @brief Disconnect the Astarte device instance.
 *
 * @details This function will disconnect the device (if not already disconnected) and stop the
 * MQTT service.
 * @note It will be possible to re-connect the device after disconnection.
 *
 *
 * @param[in] device Device instance to be disconnected.
 * @return ASTARTE_RESULT_OK if successful, otherwise an error code. When a timeout error is
 * returned the connection may not be closed gracefully. This means that when a timeout occurrs
 * the MQTT service will be forcefully stopped and subsequently a user callback may not be
 * triggered.
 */
astarte_result_t device_connection_disconnect(astarte_device_handle_t device, size_t timeout);

/**
 * @brief Handler for a connection event.
 *
 * @details This function can be used as a connection event handler for the Astarte MQTT client.
 *
 * @param[in] astarte_mqtt Astarte MQTT client context.
 * @param[in] connack_param Content of the CONNACK message.
 */
void device_connection_on_connected_handler(
    astarte_device_handle_t device, esp_mqtt_event_handle_t mqtt_event);

/**
 * @brief Handler for a disconnection event.
 *
 * @details This function can be used as a disconnection event handler for the Astarte MQTT
 client.
 *
 * @param[in] astarte_mqtt Astarte MQTT client context.
 */
void device_connection_on_disconnected_handler(astarte_device_handle_t device);

/**
 * @brief Handler for a PUBLISH event.
 *
 * @details This function can be used as a PUBLISH event handler for the Astarte MQTT client.
 *
 * @param[in] device Astarte device context.
 * @param[in] mqtt_event MQTT event from the MQTT driver.
 */
void device_connection_on_publish_handler(
    astarte_device_handle_t device, esp_mqtt_event_handle_t mqtt_event);

/**
 * @brief Handler for a SUBACK event.
 *
 * @details This function can be used as a SUBACK event handler for the Astarte MQTT client.
 *
 * @param[in] astarte_mqtt Astarte MQTT client context.
 * @param[in] message_id Message ID for the SUBACK message.
 * @param[in] return_code Return code of the SUBACK message.
 */
void device_connection_on_subscribed_handler(
    astarte_device_handle_t device, esp_mqtt_event_handle_t mqtt_event);

/**
 * @brief Poll the device connection status, it will also poll the Astarte MQTT client.
 *
 * @param[in] device Device instance to poll.
 * @return ASTARTE_RESULT_OK if successful, otherwise an error code.
 */
void device_connection_poll(astarte_device_handle_t device);

#ifdef __cplusplus
}
#endif

#endif // DEVICE_CONNECTION_H
