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

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

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
    const char *hwid;
    const char *credentials_secret;
} astarte_device_config_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief initialize Astarte device.
 *
 * @details This function has to be called to initialize the device SDK before doing anything else.
 *
 * If hwid is not defined explicitly in cfg, the device will use an hwid derived from unique device
 * features (e.g. MAC address, CPU features...).
 * An explicit hwid can be passed in the astarte_device_config_t struct. For example, it is possible to
 * use a UUIDv5 as hwid, using a personal UUID namespace and some custom device data to derive it.
 *
 * Example:
 *
 *  uuid_t uuid_ns;
 *  if (uuid_from_string("de40ff58-5696-4b35-a6d6-0cc7280bcd56", uuid_ns) != 0) {
 *      ESP_LOGE(TAG, "Error while parsing namespace UUID");
 *  }
 *
 *  uuid_t device_uuid;
 *  const char *unique_data = "my_unique_data"
 *  uuid_generate_v5(uuid_ns, unique_data, strlen(unique_data), device_uuid);
 *
 *  char hwid[32];
 *  astarte_hwid_encode(hwid, sizeof(hwid), device_uuid);
 *
 *  astarte_device_config_t cfg = {
 *      .data_event_callback = astarte_data_events_handler,
 *      .hwid = hwid,
 *  };
 *
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
 * @brief send a double value on a datastream endpoint.
 *
 * @details This function sends a double value on a path of a given datastream interface.
 * @param device A started Astarte device handle.
 * @param interface_name A string containing the name of the interface.
 * @param path A string containing the path (beginning with /).
 * @param value The value to be sent.
 * @param qos The MQTT QoS to be used for the publish (0, 1 or 2).
 * @return ASTARTE_OK if the value was correctly published, another astarte_err_t otherwise. Note that this
 * just checks that the publish sequence correctly started, i.e. it doesn't wait for PUBACK for QoS 1 messages
 * or for PUBCOMP for QoS 2 messages
 */
astarte_err_t astarte_device_stream_double(astarte_device_handle_t device, const char *interface_name, const char *path, double value, int qos);

/**
 * @brief send a 32 bit integer value on a datastream endpoint.
 *
 * @details This function sends a 32 bit int value on a path of a given datastream interface.
 * @param device A started Astarte device handle.
 * @param interface_name A string containing the name of the interface.
 * @param path A string containing the path (beginning with /).
 * @param value The value to be sent.
 * @param qos The MQTT QoS to be used for the publish (0, 1 or 2).
 * @return ASTARTE_OK if the value was correctly published, another astarte_err_t otherwise. Note that this
 * just checks that the publish sequence correctly started, i.e. it doesn't wait for PUBACK for QoS 1 messages
 * or for PUBCOMP for QoS 2 messages
 */
astarte_err_t astarte_device_stream_integer(astarte_device_handle_t device, const char *interface_name, const char *path, int32_t value, int qos);

/**
 * @brief send a 64 bit integer value on a datastream endpoint.
 *
 * @details This function sends a 64 bit int value on a path of a given datastream interface.
 * @param device A started Astarte device handle.
 * @param interface_name A string containing the name of the interface.
 * @param path A string containing the path (beginning with /).
 * @param value The value to be sent.
 * @param qos The MQTT QoS to be used for the publish (0, 1 or 2).
 * @return ASTARTE_OK if the value was correctly published, another astarte_err_t otherwise. Note that this
 * just checks that the publish sequence correctly started, i.e. it doesn't wait for PUBACK for QoS 1 messages
 * or for PUBCOMP for QoS 2 messages
 */
astarte_err_t astarte_device_stream_longinteger(astarte_device_handle_t device, const char *interface_name, const char *path, int64_t value, int qos);

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
astarte_err_t astarte_device_stream_boolean(astarte_device_handle_t device, const char *interface_name, const char *path, int value, int qos);

/**
 * @brief send a UTF8 encoded string on a datastream endpoint.
 *
 * @details This function sends a UTF8 encoded string on a path of a given datastream interface.
 * @param device A started Astarte device handle.
 * @param interface_name A string containing the name of the interface.
 * @param path A string containing the path (beginning with /).
 * @param value The value to be sent (a NULL terminated string).
 * @param qos The MQTT QoS to be used for the publish (0, 1 or 2).
 * @return ASTARTE_OK if the value was correctly published, another astarte_err_t otherwise. Note that this
 * just checks that the publish sequence correctly started, i.e. it doesn't wait for PUBACK for QoS 1 messages
 * or for PUBCOMP for QoS 2 messages
 */
astarte_err_t astarte_device_stream_string(astarte_device_handle_t device, const char *interface_name, const char *path, char *value, int qos);

/**
 * @brief send a binary value on a datastream endpoint.
 *
 * @details This function sends a binary value on a path of a given datastream interface.
 * @param device A started Astarte device handle.
 * @param interface_name A string containing the name of the interface.
 * @param path A string containing the path (beginning with /).
 * @param value A pointer to the binary data to be sent.
 * @param size The length in bytes of the binary data to be sent.
 * @param qos The MQTT QoS to be used for the publish (0, 1 or 2).
 * @return ASTARTE_OK if the value was correctly published, another astarte_err_t otherwise. Note that this
 * just checks that the publish sequence correctly started, i.e. it doesn't wait for PUBACK for QoS 1 messages
 * or for PUBCOMP for QoS 2 messages
 */
astarte_err_t astarte_device_stream_binaryblob(astarte_device_handle_t device, const char *interface_name, const char *path, void *value,
                                               size_t size, int qos);

/**
 * @brief send a datetime value on a datastream endpoint.
 *
 * @details This function sends a datetime value on a path of a given datastream interface. The datetime
 * represents the number of milliseconds since Unix epoch (1970-01-01).
 * @param device A started Astarte device handle.
 * @param interface_name A string containing the name of the interface.
 * @param path A string containing the path (beginning with /).
 * @param value The value to be sent, representing the number of milliseconds since Unix epoch (1970-01-01).
 * @param qos The MQTT QoS to be used for the publish (0, 1 or 2).
 * @return ASTARTE_OK if the value was correctly published, another astarte_err_t otherwise. Note that this
 * just checks that the publish sequence correctly started, i.e. it doesn't wait for PUBACK for QoS 1 messages
 * or for PUBCOMP for QoS 2 messages
 */
astarte_err_t astarte_device_stream_datetime(astarte_device_handle_t device, const char *interface_name, const char *path, int64_t value, int qos);

/**
 * @brief send an aggregate value on a datastream endpoint of an interface with object aggregation.
 *
 * @details This function sends an aggregate value on a path of a given datastream interface. The
 * value is represented with a BSON document that can be built with astarte_bson_serializer.
 * @param device A started Astarte device handle.
 * @param interface_name A string containing the name of the interface.
 * @param path_prefix A string containing the path prefix of the aggregate (beginning with /).
 * The path prefix is the common prefix of the endpoints in the aggregate interface.
 * @param bson_document A pointer to the document buffer containing the aggregate. Here's an example
 * of how you can obtain the bson_document for an AirSensor interface containing 3 endpoints:
 *
 *  astarte_bson_serializer_init(&bs);
 *  astarte_bson_serializer_append_double(&bs, "co2", 4.0);
 *  astarte_bson_serializer_append_double(&bs, "temperature", 20.0);
 *  astarte_bson_serializer_append_double(&bs, "humidity", 77.2);
 *  astarte_bson_serializer_append_end_of_document(&bs);
 *  int size;
 *  const void *document = astarte_bson_serializer_get_document(&bs, &size);
 * @param qos The MQTT QoS to be used for the publish (0, 1 or 2).
 * @return ASTARTE_OK if the value was correctly published, another astarte_err_t otherwise. Note
 * that this just checks that the publish sequence correctly started, i.e. it doesn't wait for
 * PUBACK for QoS 1 messages or for PUBCOMP for QoS 2 messages
 */
astarte_err_t astarte_device_stream_aggregate(astarte_device_handle_t device,
                                              const char *interface_name, const char *path_prefix,
                                              const void *bson_document, int qos);

/**
 * @brief check if the device is connected.
 *
 * @details check if the Astarte device is currently connected to the MQTT broker.
 * @param device An Astarte device handle.
 * @return true if the device is currently connected to the broker, false if it's not.
 */
bool astarte_device_is_connected(astarte_device_handle_t device);

#ifdef __cplusplus
}
#endif

#endif
