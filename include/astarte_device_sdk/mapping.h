/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ASTARTE_DEVICE_SDK_MAPPING_H_
#define _ASTARTE_DEVICE_SDK_MAPPING_H_

/**
 * @file mapping.h
 * @brief Astarte interface mapping representation.
 * @details See: https://docs.astarte-platform.org/astarte/latest/040-interface_schema.html#mapping.
 */

/**
 * @defgroup mapping Mapping
 * @brief Astarte interface mapping representation.
 * @details See: https://docs.astarte-platform.org/astarte/latest/040-interface_schema.html#mapping.
 * @ingroup astarte_device_sdk
 * @{
 */

#include "astarte_device_sdk/astarte.h"

/**
 * @brief Allowed types in a mapping on an Astarte interface
 *
 * See more about [Astarte
 * interface mapping
 * types](https://docs.astarte-platform.org/astarte/latest/040-interface_schema.html#astarte-mapping-schema-type).
 */
typedef enum
{
    /** @brief Astarte binaryblob type */
    ASTARTE_MAPPING_TYPE_BINARYBLOB = 1,
    /** @brief Astarte boolean type */
    ASTARTE_MAPPING_TYPE_BOOLEAN = 2,
    /** @brief Astarte datetime type */
    ASTARTE_MAPPING_TYPE_DATETIME = 3,
    /** @brief Astarte double type */
    ASTARTE_MAPPING_TYPE_DOUBLE = 4,
    /** @brief Astarte integer type */
    ASTARTE_MAPPING_TYPE_INTEGER = 5,
    /** @brief Astarte longinteger type */
    ASTARTE_MAPPING_TYPE_LONGINTEGER = 6,
    /** @brief Astarte string type */
    ASTARTE_MAPPING_TYPE_STRING = 7,

    /** @brief Astarte binaryblobarray type */
    ASTARTE_MAPPING_TYPE_BINARYBLOBARRAY = 8,
    /** @brief Astarte booleanarray type */
    ASTARTE_MAPPING_TYPE_BOOLEANARRAY = 9,
    /** @brief Astarte datetimearray type */
    ASTARTE_MAPPING_TYPE_DATETIMEARRAY = 10,
    /** @brief Astarte doublearray type */
    ASTARTE_MAPPING_TYPE_DOUBLEARRAY = 11,
    /** @brief Astarte integerarray type */
    ASTARTE_MAPPING_TYPE_INTEGERARRAY = 12,
    /** @brief Astarte longintegerarray type */
    ASTARTE_MAPPING_TYPE_LONGINTEGERARRAY = 13,
    /** @brief Astarte stringarray type */
    ASTARTE_MAPPING_TYPE_STRINGARRAY = 14,
} astarte_mapping_type_t;

/**
 * @brief mapping reliability definition
 *
 * This enum represents the possible values of the reliability of an astarte mapping
 * https://docs.astarte-platform.org/astarte/latest/040-interface_schema.html#astarte-mapping-schema-reliability
 */
typedef enum
{
    /** @brief Unreliable data transmission QoS: 0 */
    ASTARTE_MAPPING_RELIABILITY_UNRELIABLE = 0,
    /** @brief Guaranteed data transmission QoS: 1 */
    ASTARTE_MAPPING_RELIABILITY_GUARANTEED = 1,
    /** @brief Unique data transmission QoS: 2 */
    ASTARTE_MAPPING_RELIABILITY_UNIQUE = 2,
} astarte_mapping_reliability_t;

/**
 * @brief interface mapping definition
 *
 * This structs represent a subset of the information contained in an Astarte interface mapping
 * https://docs.astarte-platform.org/astarte/latest/040-interface_schema.html#mapping
 */
typedef struct
{
    /** @brief Mapping endpoint */
    const char *endpoint;
    /**
     * @brief Modified mapping endpoint for regex matching.
     *
     * @details The original mapping endpoint can contain parameters in the form
     * `%{([a-zA-Z_][a-zA-Z0-9_]*)}`. In this modified endpoint all the parameters have been
     * substituted by the regex pattern: `[a-zA-Z_]+[a-zA-Z0-9_]*` to ease pattern matching.
     */
    const char *regex_endpoint;
    /** @brief Mapping type */
    astarte_mapping_type_t type;
    /** @brief Mapping reliability */
    astarte_mapping_reliability_t reliability;
    /** @brief Explicit timestamp flag */
    bool explicit_timestamp;
    /** @brief Allow unset flag */
    bool allow_unset;
} astarte_mapping_t;

/**
 * @}
 */

#endif // _ASTARTE_DEVICE_SDK_MAPPING_H_
