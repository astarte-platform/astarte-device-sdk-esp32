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

typedef struct {
    astarte_device_handle_t device;
    const char *interface_name;
    const char *path;
    const void *bson_value;
    int bson_value_type;
} astarte_device_data_event_t;

typedef void (*astarte_device_data_event_callback_t)(astarte_device_data_event_t *event);

typedef struct {
   astarte_device_data_event_callback_t data_event_callback;
} astarte_device_config_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief initialize Astarte device.
 *
 * @details This function has to be called to initialize the device SDK before doing anything else.
 * @return The handle to the device, NULL if an error occurred.
 */
astarte_device_handle_t astarte_device_init(astarte_device_config_t *cfg);

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

/**
 * @brief send a boolean value on a datastream endpoint.
 *
 * @details This function sends a boolean value on a path of a given datastream interface.
 * @param device A started Astarte device handle.
 * @param interface_name A string containing the name of the interface.
 * @param path A string containing the path (beginning with /).
 * @param value The value to be sent (0 or 1).
 * @param qos The MQTT QoS to be used for the publish (0, 1 or 2).
 * @return ASTARTE_OK if the value was correctly published, another astarte_err_t otherwise. Note that this
 * just checks that the publish sequence correctly started, i.e. it doesn't wait for PUBACK for QoS 1 messages
 * or for PUBCOMP for QoS 2 messages
 */
astarte_err_t astarte_device_stream_boolean(astarte_device_handle_t device, const char *interface_name, char *path, int value, int qos);

#ifdef __cplusplus
}
#endif

#endif
