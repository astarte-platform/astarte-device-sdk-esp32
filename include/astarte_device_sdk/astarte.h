/*
 * (C) Copyright 2024-2025, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ASTARTE_DEVICE_SDK_ASTARTE_H
#define ASTARTE_DEVICE_SDK_ASTARTE_H

/**
 * @file astarte.h
 * @brief Astarte types and defines.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "sdkconfig.h"

/** @brief Major version number */
#define ASTARTE_DEVICE_SDK_MAJOR 1
/** @brief Minor version number */
#define ASTARTE_DEVICE_SDK_MINOR 9
/** @brief Patch version number */
#define ASTARTE_DEVICE_SDK_PATCH 9

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#endif // ASTARTE_DEVICE_SDK_ASTARTE_H
