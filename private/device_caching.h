/*
 * (C) Copyright 2023-2025, SECO Mind Srl
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later OR Apache-2.0
 */

/**
 * @file device_caching.h
 * @brief Implement a storage library for an Astarte device using the NVS libraries.
 */

#ifndef DEVICE_CACHING_H
#define DEVICE_CACHING_H

#include <stddef.h>
#include <stdint.h>

#include "astarte_device_sdk/astarte.h"
#include "astarte_device_sdk/data.h"
#include "astarte_device_sdk/result.h"
#include "introspection.h"
#include "kv_storage.h"

typedef struct
{
    kv_storage_iterator_t nvs_key_value_iterator;
} device_caching_property_iterator_t;

/**
 * @brief Set the synchronization state.
 *
 * @param[in] sync Synchronization state to be stored. Should be set to true if a proper
 * synchronization has been achieved with Astarte.
 * @return ASTARTE_RESULT_OK if successful, otherwise an error code.
 */
astarte_result_t device_caching_synchronization_set(bool sync);

/**
 * @brief Get the synchronization state.
 *
 * @param[out] sync Synchronization state retrieved. Will be set to true if a proper synchronization
 * has been previously achieved with Astarte.
 * @return ASTARTE_RESULT_OK if successful, otherwise an error code.
 */
astarte_result_t device_caching_synchronization_get(bool *sync);

/**
 * @brief Cache the introspection for this device.
 *
 * @param[in] intr Buffer containing the stringified version of the device introspection
 * @param[in] intr_size Size in chars of the @p buffer parameter.
 * @return ASTARTE_RESULT_OK if successful, otherwise an error code.
 */
astarte_result_t device_caching_introspection_set(const char *intr, size_t intr_size);

/**
 * @brief Check if the cached introspection exists and it's identical to the input one.
 *
 * @param[in] intr Buffer containing the stringified version of the device introspection
 * @param[in] intr_size Size in chars of the @p buffer parameter.
 * @return ASTARTE_RESULT_OK if successful, otherwise an error code.
 */
astarte_result_t device_caching_introspection_check(const char *intr, size_t intr_size);

/**
 * @brief Store a property
 *
 * @param[in] interface_name Interface name
 * @param[in] path Property endpoint
 * @param[in] major Major version name
 * @param[in] data Data to store as a generic binary buffer
 * @return ASTARTE_RESULT_OK if successful, otherwise an error code.
 */
astarte_result_t device_caching_property_store(
    const char *interface_name, const char *path, uint32_t major, astarte_data_t data);

/**
 * @brief Load a stored property
 *
 * @warning The @p data parameter should be destroyed using
 * #device_caching_property_destroy_loaded after its usage has ended.
 *
 * @param[in] interface_name Interface name
 * @param[in] path Property endpoint
 * @param[out] out_major Pointer to output major version. Might be NULL, in this case the parameter
 * is ignored.
 * @param[out] data Loaded property value
 * @return ASTARTE_RESULT_OK if successful, otherwise an error code.
 */
astarte_result_t device_caching_property_load(
    const char *interface_name, const char *path, uint32_t *out_major, astarte_data_t *data);

/**
 * @brief Destroy data for a previously loaded property.
 *
 * @details Use this function to free the memory allocated by #device_caching_property_load.
 *
 * @param[out] data Astarte data loaded by #device_caching_property_load.
 */
void device_caching_property_destroy_loaded(astarte_data_t data);

/**
 * @brief Get the device properties string.
 *
 * @details The properties string is a comma separated list of device owned properties full paths.
 * Each property full path is composed by the interface name and path of that property.
 *
 * @param[in] introspection Device introspection used to verify ownership of each property.
 * @param[out] output Buffer where to strore the computed properties string, can be NULL.
 * @param[inout] output_size Size of the @p output buffer. If the @p output parameter is NULL the
 * input value of this parameter will be ignored.
 * The function will store in this variable the size of the computed properties string (including
 * the '\0' terminating char).
 * @return ASTARTE_RESULT_OK if successful, otherwise an error code.
 */
astarte_result_t device_caching_property_get_device_properties_string(
    introspection_t *introspection, char *output, size_t *output_size);

/**
 * @brief Delete a stored property
 *
 * @param[in] interface_name Interface name
 * @param[in] path Property endpoint
 * @return ASTARTE_RESULT_OK if successful, otherwise an error code.
 */
astarte_result_t device_caching_property_delete(const char *interface_name, const char *path);

/**
 * @brief Initialize an iterator to enumerate all stored properties.
 *
 * @param[out] iterator Pointer to the iterator to initialize.
 * @return ASTARTE_RESULT_OK if successful, otherwise an error code.
 */
astarte_result_t device_caching_property_iterator_init(
    device_caching_property_iterator_t *iterator);

/**
 * @brief Destroy an iterator, the iterator won't be usable after this call.
 *
 * @param[out] iterator Pointer to the iterator to terminate.
 */
void device_caching_property_iterator_terminate(device_caching_property_iterator_t *iterator);

/**
 * @brief Advance the iterator to the next element.
 *
 * @param[in] iterator Iterator to advance.
 * @return ASTARTE_RESULT_OK if successful, otherwise an error code.
 */
astarte_result_t device_caching_property_iterator_next(
    device_caching_property_iterator_t *iterator);

/**
 * @brief Retrieve the property pointed to by the iterator.
 *
 * @param[in] iterator Iterator to use.
 * @param[out] out_interface_name Pointer to the output interface name. May be NULL, in this case
 * required length will be returned in out_interface_name_size argument.
 * @param[inout] out_interface_name_size A non-zero pointer to the variable holding the length of
 * out_interface_name. In case out_interface_name is NULL, will be set to the length required to
 * hold the interface name. In case out_interface_name is not NULL, will be set to the actual length
 * of the interface name written.
 * @param[out] out_path Pointer to the output property endpoint. May be NULL, in this case required
 * length will be returned in out_path_size argument.
 * @param[inout] out_path_size A non-zero pointer to the variable holding the length of out_path.
 * In case out_path is NULL, will be set to the length required to hold the property endpoint.
 * In case out_path is not NULL, will be set to the actual length of the property endpoint written.
 * @return ASTARTE_RESULT_OK if successful, otherwise an error code.
 */
astarte_result_t device_caching_property_iterator_get(device_caching_property_iterator_t *iterator,
    char *out_interface_name, size_t *out_interface_name_size, void *out_path,
    size_t *out_path_size);

#endif // DEVICE_CACHING_H
