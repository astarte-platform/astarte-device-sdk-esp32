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

/**
 * @brief create Astarte CSR to be sent to Pairing API.
 *
 * @details This function creates the CSR to be signed by Pairing API and saves it on the FAT filesystem on the
 * SPI flash. It requires a mounted FAT on the /spiflash mountpoint. This function is called from
 * astarte_credentials_init() if the CSR doesn't exist, but can also be called manually to generate a new CSR.
 * @return The status code, ASTARTE_OK if successful, otherwise an error code is returned.
 */
astarte_err_t astarte_credentials_create_csr();

/**
 * @brief get the saved CSR
 *
 * @details Get the CSR, writing it to the out buffer, if it is present.
 * @param out A pointer to an allocated buffer where the CSR will be written.
 * @param length The length of the out buffer.
 * @return The status code, ASTARTE_OK if the certificate was correctly written, otherwise an error code is returned.
 */
astarte_err_t astarte_credentials_get_csr(char *out, unsigned int length);

#ifdef __cplusplus
}
#endif

#endif
