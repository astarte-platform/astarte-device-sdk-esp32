/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef OBJECT_PRIVATE_H
#define OBJECT_PRIVATE_H

/**
 * @file object_private.h
 * @brief Private definitions for Astarte object data types
 */

#include "astarte_device_sdk/object.h"

#include "astarte_device_sdk/astarte.h"
#include "astarte_device_sdk/interface.h"
#include "bson_deserializer.h"
#include "bson_serializer.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Appends the content of an array of #astarte_object_entry_t to a BSON document.
 *
 * @param[in,out] bson a valid handle for the serializer instance.
 * @param[in] entries Array of object entries.
 * @param[in] entries_length Number of elements for the @p entries array.
 * @return ASTARTE_RESULT_OK if successful, otherwise an error code.
 */
astarte_result_t astarte_object_entries_serialize(
    astarte_bson_serializer_t *bson, astarte_object_entry_t *entries, size_t entries_length);

/**
 * @brief Deserialize a BSON element to an array of #astarte_object_entry_t.
 *
 * @warning This function might perform dynamic allocation, as such any individual deserialized
 * with this function should be destroyed calling #astarte_object_entries_destroy_deserialized.
 *
 * @note The BSON element should respect a predefined structure. It should contain a document
 * with all its element deserializable with #astarte_individual_deserialize.
 *
 * @param[in] bson_elem The BSON element containing the data to deserialize.
 * @param[in] interface The interface corresponding the the Astarte entries to deserialize.
 * @param[in] path The path corresponding to the BSON element to deserialize.
 * @param[out] entries Reference where to store the deserialized object entries array.
 * @param[out] entries_length The deserialized number of elements for the @p entries array.
 * @return ASTARTE_RESULT_OK if successful, otherwise an error code.
 */
astarte_result_t astarte_object_entries_deserialize(astarte_bson_element_t bson_elem,
    const astarte_interface_t *interface, const char *path, astarte_object_entry_t **entries,
    size_t *entries_length);

/**
 * @brief Destroy the data serialized with #astarte_object_entries_deserialize.
 *
 * @warning This function will free all dynamically allocated memory allocated by
 * #astarte_object_entries_deserialize. As so it should only be called on #astarte_object_entry_t
 * that have been created with #astarte_object_entries_deserialize.
 *
 * @param[inout] entries The Astarte object entries array to destroy, must have been deserialized
 * with #astarte_object_entries_deserialize.
 * @param[inout] entries_length The length of the @p entries array.
 */
void astarte_object_entries_destroy_deserialized(
    astarte_object_entry_t *entries, size_t entries_length);

#ifdef __cplusplus
}
#endif

#endif /* OBJECT_PRIVATE_H */
