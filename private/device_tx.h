/*
 * (C) Copyright 2024-2025, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DEVICE_TX_H
#define DEVICE_TX_H

/**
 * @file device_tx.h
 * @brief Device transmission header.
 */

#include "astarte_device_sdk/astarte.h"
#include "astarte_device_sdk/device.h"
#include "astarte_device_sdk/result.h"

#include "device_private.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Send an individual value through the device connection.
 *
 * @param[in] device Handle to the device instance.
 * @param[in] interface_name Interface where to publish data.
 * @param[in] path Path where to publish data.
 * @param[in] data Astarte data value to stream.
 * @param[in] timestamp Timestamp of the message, ignored if set to NULL.
 * @return ASTARTE_RESULT_OK if successful, otherwise an error code.
 */
astarte_result_t device_tx_stream_individual(astarte_device_handle_t device,
    const char *interface_name, const char *path, astarte_data_t data, const int64_t *timestamp);

/**
 * @brief Send an aggregated object through the device connection.
 *
 * @param[in] device Handle to the device instance.
 * @param[in] interface_name Interface where to publish data.
 * @param[in] path Path where to publish data.
 * @param[in] entries The object entries to stream, organized as an array.
 * @param[in] entries_len The number of element in the @p entries array.
 * @param[in] timestamp Timestamp of the message, ignored if set to NULL.
 * @return ASTARTE_RESULT_OK if successful, otherwise an error code.
 */
astarte_result_t device_tx_stream_aggregated(astarte_device_handle_t device,
    const char *interface_name, const char *path, astarte_object_entry_t *entries,
    size_t entries_len, const int64_t *timestamp);

/**
 * @brief Set a device property to the provided data value.
 *
 * @param[in] device Handle to the device instance.
 * @param[in] interface_name Interface of the property.
 * @param[in] path Path of the property.
 * @param[in] data New data value for the property.
 * @return ASTARTE_RESULT_OK if successful, otherwise an error code.
 */
astarte_result_t device_tx_set_property(astarte_device_handle_t device, const char *interface_name,
    const char *path, astarte_data_t data);

/**
 * @brief Unset a device property.
 *
 * @param[in] device Handle to the device instance.
 * @param[in] interface_name Interface of the property.
 * @param[in] path Path of the property.
 * @return ASTARTE_RESULT_OK if successful, otherwise an error code.
 */
astarte_result_t device_tx_unset_property(
    astarte_device_handle_t device, const char *interface_name, const char *path);

#ifdef __cplusplus
}
#endif

#endif // DEVICE_TX_H
