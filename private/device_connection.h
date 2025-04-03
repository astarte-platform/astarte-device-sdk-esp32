/*
 * (C) Copyright 2024, SECO Mind Srl
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
 * @note It will be possible to re-connect the device after disconnection.
 *
 * @param[in] device Device instance to be disconnected.
 * @return ASTARTE_RESULT_OK if successful, otherwise an error code.
 */
astarte_result_t device_connection_disconnect(astarte_device_handle_t device);

// /**
//  * @brief Handler for a connection event.
//  *
//  * @details This function can be used as a connection event handler for the Astarte MQTT client.
//  *
//  * @param[in] astarte_mqtt Astarte MQTT client context.
//  * @param[in] connack_param Content of the CONNACK message.
//  */
// void astarte_device_connection_on_connected_handler(
//     astarte_mqtt_t *astarte_mqtt, struct mqtt_connack_param connack_param);

// /**
//  * @brief Handler for a disconnection event.
//  *
//  * @details This function can be used as a disconnection event handler for the Astarte MQTT
//  client.
//  *
//  * @param[in] astarte_mqtt Astarte MQTT client context.
//  */
// void astarte_device_connection_on_disconnected_handler(astarte_mqtt_t *astarte_mqtt);

// /**
//  * @brief Handler for a SUBACK event.
//  *
//  * @details This function can be used as a SUBACK event handler for the Astarte MQTT client.
//  *
//  * @param[in] astarte_mqtt Astarte MQTT client context.
//  * @param[in] message_id Message ID for the SUBACK message.
//  * @param[in] return_code Return code of the SUBACK message.
//  */
// void astarte_device_connection_on_subscribed_handler(
//     astarte_mqtt_t *astarte_mqtt, uint16_t message_id, enum mqtt_suback_return_code return_code);

// /**
//  * @brief Poll the device connection status, it will also poll the Astarte MQTT client.
//  *
//  * @param[in] device Device instance to poll.
//  * @return ASTARTE_RESULT_OK if successful, otherwise an error code.
//  */
// astarte_result_t astarte_device_connection_poll(astarte_device_handle_t device);

#ifdef __cplusplus
}
#endif

#endif // DEVICE_CONNECTION_H
