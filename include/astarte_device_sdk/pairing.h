/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ASTARTE_DEVICE_SDK_PAIRING_H
#define ASTARTE_DEVICE_SDK_PAIRING_H

/**
 * @file pairing.h
 * @brief Functions for device pairing.
 */

/**
 * @defgroup pairing Device pairing
 * @brief Functions for device pairing.
 * @ingroup astarte_device_sdk
 * @{
 */

#include "astarte_device_sdk/astarte.h"
#include "astarte_device_sdk/result.h"

/** Number of characters in the string representation of a Base64 encoded credential secret. */
#define ASTARTE_PAIRING_CRED_SECR_LEN 44

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Register a new device to Astarte.
 *
 * @details Will perform the registration for a new device to the Astarte instance.
 *
 * @param[in] timeout_ms Timeout to use for the registration, in ms.
 * @param[in] device_id Unique identifier to use to register the device instance.
 * This identifier is a base64 URL encoded 128 bits value.
 * @param[out] out_cred_secr Returned credential secret.
 * @param[in] out_cred_secr_size Size of the output buffer for the credential secret.
 * @return ASTARTE_RESULT_OK if successful, otherwise an error code.
 */
astarte_result_t astarte_pairing_register_device(
    int32_t timeout_ms, const char *device_id, char *out_cred_secr, size_t out_cred_secr_size);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ASTARTE_DEVICE_SDK_PAIRING_H */
