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

#include <string.h>

typedef struct
{
    const char *base_url;
    const char *jwt;
    const char *realm;
    const char *hw_id;
    const char *credentials_secret;
} astarte_pairing_config_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief get the credentials secret.
 *
 * @details Get the credentials secret from NVS. If it isn't present, register the device to obtain
 * it, save it and return it.
 * @param config A struct containing the pairing configuration.
 * @param out A pointer to an allocated string which the credentials secret will be written to.
 * @param length The length of the out buffer.
 * @return The status code, ASTARTE_OK if successful, otherwise an error code is returned.
 */
astarte_err_t astarte_pairing_get_credentials_secret(
    const astarte_pairing_config_t *config, char *out, size_t length);

/**
 * @brief register a device.
 *
 * @details Perform a device registration as agent.
 * @param config A struct containing the pairing configuration.
 * @return The status code, ASTARTE_OK if successful, otherwise an error code is returned.
 */
astarte_err_t astarte_pairing_register_device(const astarte_pairing_config_t *config);

/**
 * @brief obtain a new Astarte MQTT v1 certificate.
 *
 * @details Call Pairing API to obtain a new Astarte MQTT v1 certificate.
 * @param config A struct containing the pairing configuration.
 * @param csr A PEM encoded NULL-terminated string containing the CSR
 * @param out A pointer to an allocated buffer where the certificat will be written.
 * @param length The length of the out buffer.
 * @return The status code, ASTARTE_OK if successful, otherwise an error code is returned.
 */
astarte_err_t astarte_pairing_get_mqtt_v1_credentials(
    const astarte_pairing_config_t *config, const char *csr, char *out, size_t length);

/**
 * @brief get the Astarte MQTT v1 broker URL.
 *
 * @details Get the URL of the broker which the device will connect to.
 * @param config A struct containing the pairing configuration.
 * @param out A pointer to an allocated string where the URL will be written.
 * @param length The length of the out buffer.
 * @return The status code, ASTARTE_OK if successful, otherwise an error code is returned.
 */
astarte_err_t astarte_pairing_get_mqtt_v1_broker_url(
    const astarte_pairing_config_t *config, char *out, size_t length);

#ifdef __cplusplus
}
#endif

#endif
