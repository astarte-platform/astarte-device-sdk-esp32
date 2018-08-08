/*
 * (C) Copyright 2018, Ispirata Srl, info@ispirata.com.
 *
 * SPDX-License-Identifier: LGPL-2.1+ OR Apache-2.0
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
 * @return The status code, ASTARTE_OK if successful, otherwise an error code is returned
 */
astarte_err_t astarte_hwid_get_id(uint8_t *hardware_id);

#ifdef __cplusplus
}
#endif

#endif
