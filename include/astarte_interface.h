/*
 * (C) Copyright 2018-2023, SECO Mind Srl
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later OR Apache-2.0
 */

/**
 * @file astarte_interface.h
 * @brief Astarte interface functions.
 */

#ifndef _ASTARTE_INTERFACE_H_
#define _ASTARTE_INTERFACE_H_

/**
 * @brief interface ownership
 *
 * This enum represents the possible ownerhips of an Astarte interface
 */
typedef enum
{
    OWNERSHIP_DEVICE = 1, /**< Device owned interface */
    OWNERSHIP_SERVER, /**< Server owned interface*/
} astarte_interface_ownership_t;

typedef enum
{
    TYPE_DATASTREAM = 1, /**< Datastream interface */
    TYPE_PROPERTIES, /**< Properties interface */
} astarte_interface_type_t;

/**
 * @brief Astarte interface definition
 *
 * This struct represents a subset of the information contained in an Astarte interface, and can be
 * used to specify some details about a specific interface.
 *
 */
typedef struct
{
    const char *name; /**< Interface name */
    int major_version; /**< Major version */
    int minor_version; /**< Minor version */
    astarte_interface_ownership_t ownership; /**< Ownership, see #astarte_interface_ownership_t */
    astarte_interface_type_t type; /**< Type, see #astarte_interface_type_t */
} astarte_interface_t;

#endif
