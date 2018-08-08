/*
 * (C) Copyright 2018, Ispirata Srl, info@ispirata.com.
 *
 * SPDX-License-Identifier: LGPL-2.1+ OR Apache-2.0
 */

/**
 * @file astarte_pairing.h
 * @brief Astarte pairing functions.
 */

#ifndef _ASTARTE_PAIRING_H_
#define _ASTARTE_PAIRING_H_

#include "astarte.h"

struct astarte_pairing_config
{
    const char* base_url;
    const char* jwt;
    const char* realm;
    const char* hw_id;
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief register a device.
 *
 * @details Perform a device registration as agent.
 * @param config A struct containing the pairing configuration.
 * @return The status code, ASTARTE_OK if successful, otherwise an error code is returned.
 */
astarte_err_t astarte_pairing_register_device(const struct astarte_pairing_config *config);

#ifdef __cplusplus
}
#endif

#endif
