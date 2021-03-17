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

#include <stdbool.h>
#include <string.h>

#define ASTARTE_CREDENTIALS_DEFAULT_NVS_PARTITION NULL

enum credential_type_t
{
    ASTARTE_CREDENTIALS_CSR = 1,
    ASTARTE_CREDENTIALS_KEY,
    ASTARTE_CREDENTIALS_CERTIFICATE
};

typedef astarte_err_t (*astarte_credentials_store_t)(
    void *opaque, enum credential_type_t cred_type, const void *credential, size_t length);
typedef astarte_err_t (*astarte_credentials_fetch_t)(
    void *opaque, enum credential_type_t cred_type, char *out, size_t length);
typedef bool (*astarte_credentials_exists_t)(void *opaque, enum credential_type_t cred_type);
typedef astarte_err_t (*astarte_credentials_remove_t)(
    void *opaque, enum credential_type_t cred_type);

typedef struct
{
    astarte_credentials_store_t astarte_credentials_store;
    astarte_credentials_fetch_t astarte_credentials_fetch;
    astarte_credentials_exists_t astarte_credentials_exists;
    astarte_credentials_remove_t astarte_credentials_remove;
} astarte_credentials_storage_functions_t;

typedef struct
{
    const astarte_credentials_storage_functions_t *functions;
    void *opaque;
} astarte_credentials_context_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief replace credentials context.
 *
 * @details This function has to be called before initialize when a storage different than internal
 * flash has to be used.
 * @return The status code, ASTARTE_OK if successful, otherwise an error code is returned.
 */
astarte_err_t astarte_credentials_set_storage_context(astarte_credentials_context_t *creds_context);

/**
 * @brief use a NVS partition as credentials context.
 *
 * @details This function has to be called before any other astarte_credentials function when NVS
 * storage is required as credentials storage.
 * @param partition_label the NVS partion label. Use ASTARTE_CREDENTIALS_DEFAULT_NVS_PARTITION when
 * default must be used.
 * @return The status code, ASTARTE_OK if successful, otherwise an error code is returned.
 */
astarte_err_t astarte_credentials_use_nvs_storage(const char *partition_label);

/**
 * @brief initialize Astarte credentials.
 *
 * @details This function has to be called to initialize the private key and CSR needed for the MQTT
 * transport.
 * @return The status code, ASTARTE_OK if successful, otherwise an error code is returned.
 */
astarte_err_t astarte_credentials_init();

/**
 * @brief check if Astarte credentials are initialized.
 *
 * @return 1 if the private key and CSR exist, 0 otherwise.
 */
int astarte_credentials_is_initialized();

/**
 * @brief create Astarte private key.
 *
 * @details This function creates the private key and saves it on the FAT filesystem on the SPI
 * flash. It requires a mounted FAT on the /spiflash mountpoint. This function is called from
 * astarte_credentials_init() if the key doesn't exist, but can also be called manually to generate
 * a new key.
 * @return The status code, ASTARTE_OK if successful, otherwise an error code is returned.
 */
astarte_err_t astarte_credentials_create_key();

/**
 * @brief create Astarte CSR to be sent to Pairing API.
 *
 * @details This function creates the CSR to be signed by Pairing API and saves it on the FAT
 * filesystem on the SPI flash. It requires a mounted FAT on the /spiflash mountpoint. This function
 * is called from astarte_credentials_init() if the CSR doesn't exist, but can also be called
 * manually to generate a new CSR.
 * @return The status code, ASTARTE_OK if successful, otherwise an error code is returned.
 */
astarte_err_t astarte_credentials_create_csr();

/**
 * @brief save the certificate to connect with the Astarte MQTT v1 protocol
 *
 * @details Save the certificate in the credentials folder. This requires a mounted FAT on the
 * /spiflash mountpoint
 * @param cert_pem The buffer containing a NULL-terminated certificate in PEM form.
 * @return The status code, ASTARTE_OK if the certificate was correctly saved, otherwise an error
 * code is returned.
 */
astarte_err_t astarte_credentials_save_certificate(const char *cert_pem);

/**
 * @brief delets the saved certificate used to connect with the Astarte MQTT v1 protocol
 *
 * @details Delete the certificate from the credentials folder. This requires a mounted FAT on the
 * /spiflash mountpoint
 * @return The status code, ASTARTE_OK if the certificate was correctly deleted, otherwise an error
 * code is returned.
 */
astarte_err_t astarte_credentials_delete_certificate();

/**
 * @brief get the saved CSR
 *
 * @details Get the CSR, writing it to the out buffer, if it is present.
 * @param out A pointer to an allocated buffer where the CSR will be written.
 * @param length The length of the out buffer.
 * @return The status code, ASTARTE_OK if the certificate was correctly written, otherwise an error
 * code is returned.
 */
astarte_err_t astarte_credentials_get_csr(char *out, size_t length);

/**
 * @brief get the certificate to connect with the Astarte MQTT v1 protocol
 *
 * @details Get the certificate, writing it to the out buffer, if it is present.
 * @param out A pointer to an allocated buffer where the certificate will be written.
 * @param length The length of the out buffer.
 * @return The status code, ASTARTE_OK if the certificate was correctly written, otherwise an error
 * code is returned.
 */
astarte_err_t astarte_credentials_get_certificate(char *out, size_t length);

/**
 * @brief get the certificate Common Name
 *
 * @details Get the certificate Common Name, writing it to the out buffer.
 * @param cert_pem A pointer to buffer containing the PEM encoded certificate.
 * @param out A pointer to an allocated buffer where the CN will be written.
 * @param length The length of the out buffer.
 * @return The status code, ASTARTE_OK if the certificate was correctly written,
 * otherwise an error code is returned.
 */
astarte_err_t astarte_credentials_get_certificate_common_name(
    const char *cert_pem, char *out, size_t length);

/**
 * @brief get the private key to connect with the Astarte MQTT v1 protocol
 *
 * @details Get the private key, writing it to the out buffer, if it is present.
 * @param out A pointer to an allocated buffer where the key will be written.
 * @param length The length of the out buffer.
 * @return The status code, ASTARTE_OK if the certificate was correctly written, otherwise an error
 * code is returned.
 */
astarte_err_t astarte_credentials_get_key(char *out, size_t length);

/**
 * @brief get the stored credentials_secret
 *
 * @details Get the credentials_secret stored in the NVS, writing it to the out buffer, if it is
 * present.
 * @param out A pointer to an allocated buffer where the credentials_secret will be written.
 * @param length The length of the out buffer.
 * @return The status code, ASTARTE_OK if the credentials_secret was found, ASTARTE_ERR_NOT_FOUND if
 * the credentials secret is not present in the NVS, another astarte_err_t if an error occurs.
 */
astarte_err_t astarte_credentials_get_stored_credentials_secret(char *out, size_t length);

/**
 * @brief save the credentials_secret in the NVS
 *
 * @details Save the credentials_secret in the NVS, where it can be used internally by Astarte
 * Pairing to obtain Astarte MQTT v1 credentials.
 * @param credentials_secret A pointer to the buffer that contains the credentials_secret.
 * @return The status code, ASTARTE_OK if the credentials_secret was correctly written, otherwise an
 * error code is returned.
 */
astarte_err_t astarte_credentials_set_stored_credentials_secret(const char *credentials_secret);

/**
 * @brief delete the credentials_secret from the NVS
 *
 * @details Delete the credentials_secret from the NVS. Keep in mind that if you lose access to the
 * credentials_secret of a device, you have to unregister it from Astarte before being able to
 * make it register again.
 * @return The status code, ASTARTE_OK if the credentials_secret was found, ASTARTE_ERR_NOT_FOUND if
 * the credentials secret is not present in the NVS, another astarte_err_t if an error occurs.
 */
astarte_err_t astarte_credentials_erase_stored_credentials_secret();

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

/*
 * @brief store a credential using filesystem storage
 *
 * @details this API might change in future versions.
 */
astarte_err_t astarte_credentials_store(
    void *opaque, enum credential_type_t cred_type, const void *credential, size_t length);

/*
 * @brief fetch a credential using filesystem storage
 *
 * @details this API might change in future versions.
 */
astarte_err_t astarte_credentials_fetch(
    void *opaque, enum credential_type_t cred_type, char *out, size_t length);

/*
 * @brief return true whether a credential exists on fileystem storage
 *
 * @details this API might change in future versions.
 */
bool astarte_credentials_exists(void *opaque, enum credential_type_t cred_type);

/*
 * @brief remove a credential from filesystem storage
 *
 * @details this API might change in future versions.
 */
astarte_err_t astarte_credentials_remove(void *opaque, enum credential_type_t cred_type);

/*
 * @brief store a credential using NVS
 *
 * @details this API might change in future versions.
 */
astarte_err_t astarte_credentials_nvs_store(
    void *opaque, enum credential_type_t cred_type, const void *credential, size_t length);

/*
 * @brief fetch a credential using NVS
 *
 * @details this API might change in future versions.
 */
astarte_err_t astarte_credentials_nvs_fetch(
    void *opaque, enum credential_type_t cred_type, char *out, size_t length);

/*
 * @brief return true whether a credential exists on NVS
 *
 * @details this API might change in future versions.
 */
bool astarte_credentials_nvs_exists(void *opaque, enum credential_type_t cred_type);

/*
 * @brief remove a credential from NVS
 *
 * @details this API might change in future versions.
 */
astarte_err_t astarte_credentials_nvs_remove(void *opaque, enum credential_type_t cred_type);

#ifdef __cplusplus
}
#endif

#endif
