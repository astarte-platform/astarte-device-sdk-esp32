/*
 * (C) Copyright 2024-2025, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ASTARTE_DEVICE_SDK_ASTARTE_H
#define ASTARTE_DEVICE_SDK_ASTARTE_H

/**
 * @file astarte.h
 * @brief Global Astarte includes and defines.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "sdkconfig.h"

/** @brief Major version number */
#define ASTARTE_DEVICE_SDK_MAJOR 1
/** @brief Minor version number */
#define ASTARTE_DEVICE_SDK_MINOR 99
/** @brief Patch version number */
#define ASTARTE_DEVICE_SDK_PATCH 99

#ifndef ARRAY_SIZE
/** @brief Macro to calculate the size of an array */
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif

#ifndef MAX
/** @brief Macro to calculate maximum between two values */
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
/** @brief Macro to calculate minimum between two values */
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#endif // ASTARTE_DEVICE_SDK_ASTARTE_H
