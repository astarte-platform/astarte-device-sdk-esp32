/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _DATA_VALIDATION_H_
#define _DATA_VALIDATION_H_

/**
 * @file data_validation.h
 * @brief Utility functions used to validate data and timestamps in transmission and reception.
 */

#include "astarte_device_sdk/astarte.h"
#include "astarte_device_sdk/individual.h"
#include "astarte_device_sdk/object.h"
#include "astarte_device_sdk/result.h"

#include "interface_private.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Validate data for an individual datastream against the device introspection.
 *
 * @param[in] interface Interface to use for the operation.
 * @param[in] path Path to validate.
 * @param[in] individual Astarte individual value to validate.
 * @param[in] timestamp Timestamp to validate, it might be NULL.
 * @return ASTARTE_RESULT_OK when validation is successful, an error otherwise.
 */
astarte_result_t data_validation_individual_datastream(const astarte_interface_t *interface,
    const char *path, astarte_individual_t individual, const int64_t *timestamp);

/**
 * @brief Validate data for an aggregated datastream against the device introspection.
 *
 * @param[in] interface Interface to use for the operation.
 * @param[in] path Path to validate.
 * @param[in] entries The object entries to validate, organized as an array.
 * @param[in] entries_len The number of element in the @p entries array.
 * @param[in] timestamp Timestamp to validate, it might be NULL.
 * @return ASTARTE_RESULT_OK when validation is successful, an error otherwise.
 */
astarte_result_t data_validation_aggregated_datastream(const astarte_interface_t *interface,
    const char *path, astarte_object_entry_t *entries, size_t entries_len,
    const int64_t *timestamp);

/**
 * @brief Validate data for setting a device property against the device introspection.
 *
 * @param[in] interface Interface to use for the operation.
 * @param[in] path Path to validate.
 * @param[in] individual Astarte individual value to validate.
 * @return ASTARTE_RESULT_OK when validation is successful, an error otherwise.
 */
astarte_result_t data_validation_set_property(
    const astarte_interface_t *interface, const char *path, astarte_individual_t individual);

/**
 * @brief Validate data for unsetting a device property against the device introspection.
 *
 * @param[in] interface Interface to use for the operation.
 * @param[in] path Path to validate.
 * @return ASTARTE_RESULT_OK when validation is successful, an error otherwise.
 */
astarte_result_t data_validation_unset_property(
    const astarte_interface_t *interface, const char *path);

#ifdef __cplusplus
}
#endif

#endif // _DATA_VALIDATION_H_
