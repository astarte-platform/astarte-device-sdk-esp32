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

#include <string.h>

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
 * @brief save the certificate to connect with the Astarte MQTT v1 protocol
 *
 * @details Save the certificate in the credentials folder. This requires a mounted FAT on the /spiflash mountpoint
 * @param cert_pem The buffer containing a NULL-terminated certificate in PEM form.
 * @return The status code, ASTARTE_OK if the certificate was correctly saved, otherwise an error code is returned.
 */
astarte_err_t astarte_credentials_save_certificate(const char *cert_pem);

/**
 * @brief delets the saved certificate used to connect with the Astarte MQTT v1 protocol
 *
 * @details Delete the certificate from the credentials folder. This requires a mounted FAT on the /spiflash mountpoint
 * @return The status code, ASTARTE_OK if the certificate was correctly deleted, otherwise an error code is returned.
 */
astarte_err_t astarte_credentials_delete_certificate();

/**
 * @brief get the saved CSR
 *
 * @details Get the CSR, writing it to the out buffer, if it is present.
 * @param out A pointer to an allocated buffer where the CSR will be written.
 * @param length The length of the out buffer.
 * @return The status code, ASTARTE_OK if the certificate was correctly written, otherwise an error code is returned.
 */
astarte_err_t astarte_credentials_get_csr(char *out, size_t length);

/**
 * @brief get the certificate to connect with the Astarte MQTT v1 protocol
 *
 * @details Get the certificate, writing it to the out buffer, if it is present.
 * @param out A pointer to an allocated buffer where the certificate will be written.
 * @param length The length of the out buffer.
 * @return The status code, ASTARTE_OK if the certificate was correctly written, otherwise an error code is returned.
 */
astarte_err_t astarte_credentials_get_certificate(char *out, size_t length);

/**
 * @brief get the certificate Common Name
 *
 * @details Get the certificate Common Name, writing it to the out buffer, if it is present.
 * @param out A pointer to an allocated buffer where the CN will be written.
 * @param length The length of the out buffer.
 * @return The status code, ASTARTE_OK if the certificate was correctly written, otherwise an error code is returned.
 */
astarte_err_t astarte_credentials_get_certificate_common_name(char *out, size_t length);

/**
 * @brief get the private key to connect with the Astarte MQTT v1 protocol
 *
 * @details Get the private key, writing it to the out buffer, if it is present.
 * @param out A pointer to an allocated buffer where the key will be written.
 * @param length The length of the out buffer.
 * @return The status code, ASTARTE_OK if the certificate was correctly written, otherwise an error code is returned.
 */
astarte_err_t astarte_credentials_get_key(char *out, size_t length);

/**
 * @brief check if the certificate exists
 *
 * @details Check if the file containing the certificate exists and is readable.
 * @return 1 if the file exists and is readable, 0 otherwise.
 */
int astarte_credentials_has_certificate();

/**
 * @brief check if the CSR exists
 *
 * @details Check if the file containing the CSR exists and is readable.
 * @return 1 if the file exists and is readable, 0 otherwise.
 */
int astarte_credentials_has_csr();

/**
 * @brief check if the private key exists
 *
 * @details Check if the file containing the private key exists and is readable.
 * @return 1 if the file exists and is readable, 0 otherwise.
 */
int astarte_credentials_has_key();

#ifdef __cplusplus
}
#endif

#endif
