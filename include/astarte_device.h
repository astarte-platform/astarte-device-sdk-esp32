/*
 * (C) Copyright 2018, Ispirata Srl, info@ispirata.com.
 *
 * SPDX-License-Identifier: LGPL-2.1+ OR Apache-2.0
 */

/**
 * @file astarte_device.h
 * @brief Astarte device SDK high level API.
 */

#ifndef _ASTARTE_DEVICE_H_
#define _ASTARTE_DEVICE_H_

#include "astarte.h"

typedef struct astarte_device_t *astarte_device_handle_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief initialize Astarte device.
 *
 * @details This function has to be called to initialize the device SDK before doing anything else.
 * @return The handle to the device, NULL if an error occurred.
 */
astarte_device_handle_t astarte_device_init();

/**
 * @brief destroy Astarte device.
 *
 * @details This function destroys the device, freeing all its resources.
 * @param device A valid Astarte device handle.
 */
void astarte_device_destroy(astarte_device_handle_t device);

/**
 * @brief add an interface to the device.
 *
 * @details This function has to be called before astarte_device_start to add all the needed Astarte interfaces,
 * that will be sent in the device introspection when it connects.
 * @param device A valid Astarte device handle.
 * @param interface_name A string containing the name of the interface.
 * @param major_version The major version of the interface.
 * @param minor_version The minor_version of the interface.
 * @return ASTARTE_OK if the interface was succesfully added, another astarte_err_t otherwise.
 */
astarte_err_t astarte_device_add_interface(astarte_device_handle_t device, const char *interface_name, int major_version, int minor_version);

/**
 * @brief start Astarte device.
 *
 * @details This function starts the device, making it connect to the broker and perform its work.
 * @param device A valid Astarte device handle.
 */
void astarte_device_start(astarte_device_handle_t device);

#ifdef __cplusplus
}
#endif

#endif
