/*
 * (C) Copyright 2024-2025, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DEVICE_RX_H
#define DEVICE_RX_H

/**
 * @file device_rx.h
 * @brief Device reception header.
 */

#include "astarte_device_sdk/astarte.h"
#include "astarte_device_sdk/device.h"
#include "astarte_device_sdk/result.h"

#include "device_private.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Handler for a reception event.
 *
 * @details This function can be used as a reception event handler for the Astarte MQTT client.
 *
 * @param[in] device Astarte device handle.
 */
void device_rx_on_incoming_handler(astarte_device_handle_t device, esp_mqtt_event_handle_t event);

#ifdef __cplusplus
}
#endif

#endif // DEVICE_RX_H
