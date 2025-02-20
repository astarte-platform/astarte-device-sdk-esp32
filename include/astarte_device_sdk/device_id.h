/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ASTARTE_DEVICE_SDK_DEVICE_ID_H_
#define _ASTARTE_DEVICE_SDK_DEVICE_ID_H_

/**
 * @file device_id.h
 * @brief Functions for the generation of Astarte device identifiers.
 */

/**
 * @defgroup device_id Device ID
 * @brief Functions for the generation of Astarte device identifiers.
 * @ingroup astarte_device_sdk
 * @{
 */

#include "astarte_device_sdk/astarte.h"
#include "astarte_device_sdk/result.h"

/**
 * @brief Number of characters in the string version of an Astarte device ID.
 *
 * @details The Astarte device IDs are 128 bit identifiers. Their string representation is a base 64
 * (RFC 4648 sec. 5) URL and filename safe encoding.
 */
#define ASTARTE_DEVICE_ID_LEN 22

/** @brief Size in bytes for the namespace used to generate a deterministic device IDs. */
#define ASTARTE_DEVICE_ID_NAMESPACE_SIZE 16

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Generate a random Astarte device ID.
 *
 * @details The Astarte device ID will be generated using an UUIDv4.
 *
 * @param[out] out A buffer where the result will be written.
 * @return ASTARTE_RESULT_OK if successful, otherwise an error code.
 */
astarte_result_t astarte_device_id_generate_random(char out[static ASTARTE_DEVICE_ID_LEN + 1]);

/**
 * @brief Generate a deterministic Astarte device ID.
 *
 * @details The Astarte device ID will be generated using an UUIDv5. An UUIDv5 is created hasing
 * a namespace and name. The namespace is an UUID itself (of any version).
 *
 * @param[in] namespace The namespace used to generate the device ID.
 * @param[in] name The name used to generate the device ID.
 * @param[in] name_size The size of the @p name buffer.
 * @param[out] out A buffer where the result will be written.
 * @return ASTARTE_RESULT_OK if successful, otherwise an error code.
 */
astarte_result_t astarte_device_id_generate_deterministic(
    const uint8_t namespace[static ASTARTE_DEVICE_ID_NAMESPACE_SIZE], const uint8_t *name,
    size_t name_size, char out[static ASTARTE_DEVICE_ID_LEN + 1]);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif // _ASTARTE_DEVICE_SDK_DEVICE_ID_H_
