/*
 * (C) Copyright 2018, Ispirata Srl, info@ispirata.com.
 *
 * SPDX-License-Identifier: LGPL-2.1+ OR Apache-2.0
 */

/**
 * @file astarte_credentials.h
 * @brief Astarte credentials functions.
 */

#ifndef _ASTARTE_CREDENTIALS_H_
#define _ASTARTE_CREDENTIALS_H_

#include "astarte.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief initialize Astarte credentials.
 *
 * @details This function has to be called to initialize the private key and CSR needed for the MQTT transport.
 * @return The status code, ASTARTE_OK if successful, otherwise an error code is returned.
 */
astarte_err_t astarte_credentials_init();

/**
 * @brief create Astarte private key.
 *
 * @details This function creates the private key and saves it on the FAT filesystem on the SPI flash. It requires
 * a mounted FAT on the /spiflash mountpoint. This function is called from astarte_credentials_init() if the key
 * doesn't exist, but can also be called manually to generate a new key.
 * @return The status code, ASTARTE_OK if successful, otherwise an error code is returned.
 */
astarte_err_t astarte_credentials_create_key();

#ifdef __cplusplus
}
#endif

#endif
