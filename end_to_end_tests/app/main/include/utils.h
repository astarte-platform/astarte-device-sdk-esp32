/*
 * (C) Copyright 2025, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef UTILS_H
#define UTILS_H

/**
 * @file utils.h
 * @brief Contains some common utilities.
 */

#include "astarte_device_sdk/data.h"
#include "astarte_device_sdk/object.h"

/**
 * @brief Pretty print to the log output an Astarte data.
 *
 * @param[in] data The data to log
 */
void utils_log_astarte_data(astarte_data_t data);

void utils_log_astarte_object(astarte_object_entry_t *entries, size_t entries_len);

uint8_t *utils_hexstr_to_bytes(const char *hexstr, size_t *num_bytes);

#endif /* UTILS_H */
