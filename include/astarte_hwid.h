/*
 * (C) Copyright 2018-2023, SECO Mind Srl
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later OR Apache-2.0
 */

/**
 * @file astarte_hwid.h
 * @brief Astarte hardware ID functions.
 */

#ifndef _ASTARTE_HWID_H_
#define _ASTARTE_HWID_H_

#include "astarte.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief get a unique hardware ID.
 *
 * @details Generate an unique hardware ID using device MAC address and other identification bits.
 * @param hardware_id a 16 bytes buffer where device ID will be written.
 * @return ASTARTE_OK if successful, otherwise an error code is returned.
 */
astarte_err_t astarte_hwid_get_id(uint8_t *hardware_id);

/**
 * @brief encode a binary hardware ID to a C string.
 *
 * @details base64url encode a 128 bits hardware id to an output string.
 * @param encoded the destination buffer where encoded string will be written.
 * @param dest_size the destination buffer size.
 * @param hardware_id 16 bytes hardware id.
 * @return ASTARTE_OK if successful, otherwise an error code is returned.
 */
astarte_err_t astarte_hwid_encode(char *encoded, int dest_size, const uint8_t *hardware_id);

#ifdef __cplusplus
}
#endif

#endif
