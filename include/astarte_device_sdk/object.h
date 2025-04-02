/*
 * (C) Copyright 2024-2025, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ASTARTE_DEVICE_SDK_OBJECT_H
#define ASTARTE_DEVICE_SDK_OBJECT_H

/**
 * @file object.h
 * @brief Definitions for Astarte object data types.
 */

/**
 * @defgroup objects Objects
 * @brief Definitions for Astarte object data types.
 * @ingroup astarte_device_sdk
 * @{
 */

#include "astarte_device_sdk/astarte.h"
#include "astarte_device_sdk/data.h"
#include "astarte_device_sdk/result.h"

/**
 * @brief Object entry data type.
 *
 *
 * @warning Do not inspect its content directly. Use the provided functions to modify it.
 * @details This struct is the base building block for an Astarte object. Concatenate multiple
 * #astarte_object_entry_t in an array to create the payload for an Astarte object transmission and
 * reception.
 *
 * See the following initialization/getter methods:
 *  - #astarte_object_entry_new
 *  - #astarte_object_entry_to_path_and_data
 */
typedef struct
{
    /** @brief Path for the entry */
    const char *path;
    /** @brief Data for the entry */
    astarte_data_t data;
} astarte_object_entry_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize an Astarte object entry.
 *
 * @param[in] path Path to store in the entry.
 * @param[in] data Data to store in the entry.
 * @return The Astarte object entry created from the inputs.
 */
astarte_object_entry_t astarte_object_entry_new(const char *path, astarte_data_t data);

/**
 * @brief Convert an Astarte object entry to an Astarte data and path.
 *
 * @param[in] entry Input Astarte object entry.
 * @param[out] path Path extracted from the entry.
 * @param[out] data Data extracted from the entry.
 * @return ASTARTE_RESULT_OK if the conversion was successful, an error otherwise.
 */
astarte_result_t astarte_object_entry_to_path_and_data(
    astarte_object_entry_t entry, const char **path, astarte_data_t *data);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif // ASTARTE_DEVICE_SDK_OBJECT_H
