/*
 * (C) Copyright 2018-2023, SECO Mind Srl
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later OR Apache-2.0
 */

/**
 * @file astarte_storage.h
 * @brief Implement a storage library for Astarte properties using the NVS libraries.
 */

#ifndef _ASTARTE_STORAGE_H_
#define _ASTARTE_STORAGE_H_

#include <stddef.h>
#include <stdint.h>

#include "astarte_nvs_key_value.h"

typedef enum
{
    ASTARTE_STORAGE_ERR_OK = 0,
    ASTARTE_STORAGE_ERR_INTERNAL,
    ASTARTE_STORAGE_ERR_DATA_TOO_SMALL,
    ASTARTE_STORAGE_ERR_NOT_FOUND,
} astarte_storage_err_t;

typedef struct
{
    nvs_handle_t nvs_handle;
} astarte_storage_handle_t;

typedef struct
{
    nvs_handle_t handle;
    astarte_nvs_key_value_iterator_t nvs_key_value_iterator;
} astarte_storage_iterator_t;

/**
 * @brief Open the underlying NVS memory.
 *
 * @param[out] handle Handle to initialize.
 * @return ASTARTE_STORAGE_ERR_OK when operation has been successful, an error code otherwise.
 */
astarte_storage_err_t astarte_storage_open(astarte_storage_handle_t *handle);

/**
 * @brief Close the underlying NVS memory.
 *
 * @param[in] handle Handle to close.
 */
void astarte_storage_close(astarte_storage_handle_t handle);

/**
 * @brief Store a property.
 *
 * @note Passing NULL to the data parameter will delete a previously stored property.
 *
 * @param[in] handle Handle to astarte storage instance.
 * @param[in] interface_name Interface name.
 * @param[in] path Property endpoint.
 * @param[in] data Data to store. This can be a generic binary buffer.
 * @param[in] data_len Length of the binary buffer to store.
 * @return ASTARTE_STORAGE_ERR_OK when operation has been successful, an error code otherwise.
 */
astarte_storage_err_t astarte_storage_store_property(astarte_storage_handle_t handle,
    const char *interface_name, const char *path, const void *data, size_t data_len);

/**
 * @brief Load a stored property.
 *
 * @param[in] handle Handle to astarte storage instance.
 * @param[in] interface_name Interface name.
 * @param[in] path Property endpoint.
 * @param[out] out_data Pointer to the output value. May be NULL, in this case required length will
 * be returned in out_data_len argument.
 * @param[in] out_data_len A non-zero pointer to the variable holding the length of out_data. In
 * case out_data is NULL, will be set to the length required to hold the value. In case out_data is
 * not NULL, will be set to the actual length of the value written.
 * @return ASTARTE_STORAGE_ERR_OK when operation has been successful, an error code otherwise.
 */
astarte_storage_err_t astarte_storage_load_property(astarte_storage_handle_t handle,
    const char *interface_name, const char *path, void *out_data, size_t *out_data_len);

/**
 * @brief Delete a stored property.
 *
 * @param[in] handle Handle to astarte storage instance.
 * @param[in] interface_name Interface name.
 * @param[in] path Property endpoint.
 * @return ASTARTE_STORAGE_ERR_OK when operation has been successful, an error code otherwise.
 */
astarte_storage_err_t astarte_storage_delete_property(astarte_storage_handle_t handle,
    const char *interface_name, const char *path);

/**
 * @brief Clear the storage.
 *
 * @details Delete all the properties stored in non volatile memory.
 *
 * @param[in] handle Handle to astarte storage instance.
 * @return ASTARTE_STORAGE_ERR_OK when operation has been successful, an error code otherwise.
 */
astarte_storage_err_t astarte_storage_clear(astarte_storage_handle_t handle);

/**
 * @brief Create an iterator to enumerate all stored properties.
 *
 * @note This function will call nvs_open for a specific namespace. Make sure to destroy the
 * iterator (will close the NVS namespace) before performing potentially conflicting NVS operations.
 *
 * @param[in] handle Handle to astarte storage instance.
 * @param[out] iterator Pointer to the created iterator.
 * @return ASTARTE_STORAGE_ERR_OK when operation has been successful, an error code otherwise.
 */
astarte_storage_err_t astarte_storage_iterator_create(astarte_storage_handle_t handle,
    astarte_storage_iterator_t *iterator);

/**
 * @brief Advance the iterator.
 *
 * @param[in] iterator Iterator to advance.
 * @return ASTARTE_STORAGE_ERR_OK when operation has been successful, an error code otherwise.
 */
astarte_storage_err_t astarte_storage_iterator_advance(astarte_storage_iterator_t *iterator);

/**
 * @brief Retrieve the property pointed to by the iterator.
 *
 * @param[in] iterator Iterator to use.
 * @param[out] out_interface_name Pointer to the output interface name. May be NULL, in this case
 * required length will be returned in out_interface_name_len argument.
 * @param[inout] out_interface_name_len A non-zero pointer to the variable holding the length of
 * out_interface_name. In case out_interface_name is NULL, will be set to the length required to
 * hold the interface name. In case out_interface_name is not NULL, will be set to the actual length
 * of the interface name written.
 * @param[out] out_path Pointer to the output property endpoint. May be NULL, in this case required
 * length will be returned in out_path_len argument.
 * @param[inout] out_path_len A non-zero pointer to the variable holding the length of out_path.
 * In case out_path is NULL, will be set to the length required to hold the property endpoint.
 * In case out_path is not NULL, will be set to the actual length of the property endpoint written.
 * @param[out] out_value Pointer to the output value. May be NULL, in this case required length will
 * be returned in out_value_len argument.
 * @param[inout] out_value_len A non-zero pointer to the variable holding the length of out_value.
 * In case out_value is NULL, will be set to the length required to hold the property value.
 * In case out_value is not NULL, will be set to the actual length of the property value written.
 * @return ASTARTE_STORAGE_ERR_OK when operation has been successful, an error code otherwise.
 */
astarte_storage_err_t astarte_storage_iterator_get_property(astarte_storage_iterator_t *iterator,
    char *out_interface_name, size_t *out_interface_name_len, void *out_path, size_t *out_path_len,
    void *out_value, size_t *out_value_len);

#endif /* _ASTARTE_STORAGE_H_ */
