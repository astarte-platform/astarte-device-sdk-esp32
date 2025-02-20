/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MAPPING_PRIVATE_H_
#define _MAPPING_PRIVATE_H_

/**
 * @file mapping_private.h
 * @brief Private methods for the Astarte interfaces.
 */

#include "astarte_device_sdk/individual.h"
#include "astarte_device_sdk/mapping.h"
#include "astarte_device_sdk/result.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Convert an array mapping type to its corresponding scalar mapping type.
 *
 * @param[in] array_type Input mapping type. Has to be an array type.
 * @param[out] scalar_type The resulting scalar type.
 * @return ASTARTE_RESULT_OK on success, otherwise an error code.
 */
astarte_result_t astarte_mapping_array_to_scalar_type(
    astarte_mapping_type_t array_type, astarte_mapping_type_t *scalar_type);

/**
 * @brief Check if a path corresponds to the endpoint of a mapping.
 *
 * @note A path is an endpoint with all parametrization substituted by real values.
 *
 * @param[in] mapping Mapping to use for the comparison.
 * @param[in] path Path to use for comparison.
 * @return ASTARTE_RESULT_OK on success, otherwise an error code.
 */
astarte_result_t astarte_mapping_check_path(astarte_mapping_t mapping, const char *path);

/**
 * @brief Check if a individual is compatible to the type of a mapping.
 *
 * @param[in] mapping Mapping to use for the comparison.
 * @param[in] individual Astarte individual value to use for comparison.
 * @return ASTARTE_RESULT_OK on success, otherwise an error code.
 */
astarte_result_t astarte_mapping_check_individual(
    const astarte_mapping_t *mapping, astarte_individual_t individual);

#ifdef __cplusplus
}
#endif

#endif // _MAPPING_PRIVATE_H_
