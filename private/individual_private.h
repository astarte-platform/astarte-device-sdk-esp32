/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef INDIVIDUAL_PRIVATE_H
#define INDIVIDUAL_PRIVATE_H

/**
 * @file individual_private.h
 * @brief Private definitions for Astarte individual data types
 */
#include "astarte_device_sdk/individual.h"

#include "astarte_device_sdk/astarte.h"
#include "astarte_device_sdk/interface.h"
#include "astarte_device_sdk/object.h"
#include "bson_deserializer.h"
#include "bson_serializer.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Appends the content of an #astarte_individual_t to a BSON document.
 *
 * @param[in,out] bson a valid handle for the serializer instance.
 * @param[in] key BSON key name, which is a C string.
 * @param[in] individual the #astarte_individual_t to serialize to the bson.
 * @return ASTARTE_RESULT_OK if successful, otherwise an error code.
 */
astarte_result_t astarte_individual_serialize(
    astarte_bson_serializer_t *bson, const char *key, astarte_individual_t individual);

/**
 * @brief Deserialize a BSON element to an #astarte_individual_t.
 *
 * @warning This function might perform dynamic allocation, as such any individual deserialized with
 * this function should be destroyed calling #astarte_individual_destroy_deserialized.
 *
 * @note No encapsulation is permitted in the BSON element, the individual should be directy
 * accessible.
 *
 * @param[in] bson_elem The BSON element containing the data to deserialize.
 * @param[in] type An expected mapping type for the Astarte individual.
 * @param[out] individual The resulting individual.
 * @return ASTARTE_RESULT_OK if successful, otherwise an error code.
 */
astarte_result_t astarte_individual_deserialize(astarte_bson_element_t bson_elem,
    astarte_mapping_type_t type, astarte_individual_t *individual);

/**
 * @brief Destroy the data serialized with #astarte_individual_deserialize.
 *
 * @warning This function will free all dynamically allocated memory allocated by
 * #astarte_individual_deserialize. As so it should only be called on #astarte_individual_t
 * that have been created with #astarte_individual_deserialize.
 *
 * @note This function will have no effect on scalar #astarte_individual_t, but only act on array
 * types.
 *
 * @param[in] individual Individual of which the content will be destroyed.
 */
void astarte_individual_destroy_deserialized(astarte_individual_t individual);

#ifdef __cplusplus
}
#endif

#endif /* INDIVIDUAL_PRIVATE_H */
