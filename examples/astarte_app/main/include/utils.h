/*
 * (C) Copyright 2024-2025, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef UTILS_H
#define UTILS_H

/**
 * @file utils.h
 * @brief Contains some common utilities for the samples.
 *
 * @details This file contains a function to "pretty print" an Astarte individual and a set of
 * standard data to use when streaming datastreams or setting properties in the samples.
 * The standard data contains an element for each possible Astarte type. This allows the samples
 * to demonstrate transmission of all existing Astarte types.
 * Each data element is paired to a string representing a mapping endpoint for the interfaces to
 * be used in the samples.
 */

#include "astarte_device_sdk/data.h"
#include "astarte_device_sdk/object.h"

#define UTILS_DATA_ELEMENTS 14

extern const uint8_t utils_binary_blob_data[8];
extern const uint8_t *const utils_binary_blobs_data[2];
extern const size_t utils_binary_blobs_sizes_data[2];
extern const bool utils_boolean_data;
extern const bool utils_boolean_array_data[3];
extern const int64_t utils_unix_time_data;
extern const int64_t utils_unix_time_array_data[1];
extern const double utils_double_data;
extern const double utils_double_array_data[2];
extern const int32_t utils_integer_data;
extern const int32_t utils_integer_array_data[3];
extern const int64_t utils_longinteger_data;
extern const int64_t utils_longinteger_array_data[3];
extern const char utils_string_data[];
extern const char *const utils_string_array_data[2];

/**
 * @brief Pretty print to the log output an Astarte data.
 *
 * @param[in] data The data to log
 */
void utils_log_astarte_data(astarte_data_t data);

void utils_log_astarte_object(astarte_object_entry_t *entries, size_t entries_len);

#endif /* UTILS_H */
