/*
 * (C) Copyright 2018-2021, Ispirata Srl, info@ispirata.com.
 *
 * SPDX-License-Identifier: LGPL-2.1+ OR Apache-2.0
 */

/**
 * @file astarte.h
 * @brief Astarte types and defines.
 */

#ifndef _ASTARTE_H_
#define _ASTARTE_H_

// Version information
#define ASTARTE_DEVICE_SDK_MAJOR 1
#define ASTARTE_DEVICE_SDK_MINOR 0
#define ASTARTE_DEVICE_SDK_PATCH 2

// clang-format off

/**
 * @brief Astarte return codes.
 *
 * @detail Astarte Device SDK return codes. ASTARTE_OK is always returned when no errors occurred.
 */
enum astarte_err_t
{
    ASTARTE_OK = 0, /**< No errors. */
    ASTARTE_ERR = 1, /**< A generic error occurred. This is usually an internal error in the SDK */
    ASTARTE_ERR_NOT_FOUND = 2, /**< The resource was not found. */
    ASTARTE_ERR_NO_JWT = 3, /**< ASTARTE_PAIRING_JWT is not configured, device can't be registered */
    ASTARTE_ERR_OUT_OF_MEMORY = 4, /**< The operation caused an Out of Memory error */
    ASTARTE_ERR_ESP_SDK = 5, /**< An error caused by the ESP SDK has occurred */
    ASTARTE_ERR_AUTH = 6, /**< An API call returned an authentication or authorization error */
    ASTARTE_ERR_ALREADY_EXISTS = 7, /**< Attempted to perform an operation on an already existing resource */
    ASTARTE_ERR_API = 8, /**< A generic error occurred while calling an Astarte API */
    ASTARTE_ERR_HTTP = 9, /**< An HTTP request could not be processed */
    ASTARTE_ERR_NVS = 10, /**< A generic error occurred when dealing with NVS */
    ASTARTE_ERR_NVS_NOT_INITIALIZED = 11, /**< An error occurred due to an initialization issue with NVS */
    ASTARTE_ERR_PARTITION_SCHEME = 12, /**< An error occurred due to a partitioning scheme incompatible with the SDK */
    ASTARTE_ERR_MBED_TLS = 13, /**< An error occurred during an MBED TLS operation */
    ASTARTE_ERR_IO = 14, /**< An error occurred during a file I/O operation */
    ASTARTE_ERR_INVALID_INTERFACE_PATH = 15, /**< The Interface Path is not valid */
    ASTARTE_ERR_INVALID_QOS = 16, /**< The QoS is not valid */
    ASTARTE_ERR_DEVICE_NOT_READY = 17, /**< Tried to perform an operation on a Device in a non-ready or initialized state */
    ASTARTE_ERR_PUBLISH = 18, /**< An error occurred while publishing data on MQTT */
    ASTARTE_ERR_INVALID_INTROSPECTION = 19 /**< The introspection is not valid or empty */
};

// clang-format on

typedef enum astarte_err_t astarte_err_t;

#endif
