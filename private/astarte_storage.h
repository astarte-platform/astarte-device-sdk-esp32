/*
 * (C) Copyright 2023, SECO Mind Srl
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

#include "astarte.h"
#include "astarte_nvs_key_value.h"

typedef struct
{
    nvs_handle_t nvs_handle;
} astarte_storage_handle_t;

typedef struct
{
    astarte_nvs_key_value_iterator_t nvs_key_value_iterator;
} astarte_storage_iterator_t;

/**
 * @brief Opens the underlying NVS memory
 *
 * @param[out] handle Handle to initialize
 * @return One of the follwing error codes:
 * - ASTARTE_ERR if operation failed,
 * - ASTARTE_OK if operation has been successful
 */
astarte_err_t astarte_storage_open(astarte_storage_handle_t *handle);

/**
 * @brief Closes the underlying NVS memory
 *
 * @param[in] handle Handle to close
 */
void astarte_storage_close(astarte_storage_handle_t handle);

/**
 * @brief Stores a property
 *
 * @param[in] handle Handle to astarte storage instance
 * @param[in] interface_name Interface name
 * @param[in] path Property endpoint
 * @param[in] major Major version name
 * @param[in] data Data to store as a generic binary buffer
 * @param[in] data_len Length of the binary buffer to store
 * @return One of the follwing error codes:
 * - ASTARTE_ERR if operation failed,
 * - ASTARTE_OK if operation has been successful
 */
astarte_err_t astarte_storage_store_property(astarte_storage_handle_t handle,
    const char *interface_name, const char *path, int32_t major, const void *data, size_t data_len);

/**
 * @brief Checks if a property is contained in storage with an exact value
 *
 * @note If a property exists in storage with same 'interface_name' and 'path' but with different
 * version, the stored property will be erased.
 *
 * @param[in] handle Handle to astarte storage instance
 * @param[in] interface_name Interface name
 * @param[in] path Property endpoint
 * @param[in] major Major version name
 * @param[in] data Data to compare to the stored one
 * @param[in] data_len Length, in bytes, of the data to check
 * @param[out] res Result for the check: true if the property is present with the same value
 * @return One of the follwing error codes:
 * - ASTARTE_ERR if the operation failed for other reasons,
 * - ASTARTE_OK if the operation has been successful
 */
astarte_err_t astarte_storage_contains_property(astarte_storage_handle_t handle,
    const char *interface_name, const char *path, int32_t major, const void *data, size_t data_len,
    bool *res);

/**
 * @brief Loads a stored property
 *
 * @param[in] handle Handle to astarte storage instance
 * @param[in] interface_name Interface name
 * @param[in] path Property endpoint
 * @param[out] out_major Pointer to output major version. Might be NULL, in this case the parameter
 * is ignored.
 * @param[out] out_data Pointer to the output value. May be NULL, in this case required length will
 * be returned in out_data_len argument
 * @param[in] out_data_len A non-zero pointer to the variable holding the length of out_data. In
 * case out_data is NULL, will be set to the length required to hold the value. In case out_data is
 * not NULL, will be set to the actual length of the value written
 * @return One of the follwing error codes:
 * - ASTARTE_ERR_NOT_FOUND is no such property is present in storage,
 * - ASTARTE_ERR_INVALID_SIZE if the provided out_data buffer is not sufficiently large,
 * - ASTARTE_ERR if the operation failed for other reasons,
 * - ASTARTE_OK if the operation has been successful
 */
astarte_err_t astarte_storage_load_property(astarte_storage_handle_t handle,
    const char *interface_name, const char *path, int32_t *out_major, void *out_data,
    size_t *out_data_len);

/**
 * @brief Deletes a stored property
 *
 * @param[in] handle Handle to astarte storage instance
 * @param[in] interface_name Interface name
 * @param[in] path Property endpoint
 * @return One of the follwing error codes:
 * - ASTARTE_ERR_NOT_FOUND is no such property is present in storage,
 * - ASTARTE_ERR if operation failed,
 * - ASTARTE_OK if operation has been successful
 */
astarte_err_t astarte_storage_delete_property(
    astarte_storage_handle_t handle, const char *interface_name, const char *path);

/**
 * @brief Clears the storage
 *
 * @details Delete all the properties stored in non volatile memory.
 *
 * @param[in] handle Handle to astarte storage instance
 * @return One of the follwing error codes:
 * - ASTARTE_ERR if operation failed,
 * - ASTARTE_OK if operation has been successful
 */
astarte_err_t astarte_storage_clear(astarte_storage_handle_t handle);

/**
 * @brief Create an iterator to enumerate all stored properties.
 *
 * @param[in] handle Handle to astarte storage instance.
 * @param[out] iterator Pointer to the created iterator.
 * @return One of the follwing error codes:
 * - ASTARTE_ERR_NOT_FOUND if no property is present in storage,
 * - ASTARTE_ERR if operation failed,
 * - ASTARTE_OK if operation has been successful
 */
astarte_err_t astarte_storage_iterator_create(
    astarte_storage_handle_t handle, astarte_storage_iterator_t *iterator);

/**
 * @brief Check if the iterator has one more item without advancing it.
 *
 * @param[in] iterator Iterator to peek.
 * @return One of the follwing error codes:
 * - ASTARTE_ERR if operation failed,
 * - ASTARTE_OK if operation has been successful
 */
astarte_err_t astarte_storage_iterator_peek(astarte_storage_iterator_t *iterator, bool *has_next);

/**
 * @brief Advance the iterator.
 *
 * @param[in] iterator Iterator to advance.
 * @return One of the follwing error codes:
 * - ASTARTE_ERR_NOT_FOUND if no other property is present in storage,
 * - ASTARTE_ERR if operation failed,
 * - ASTARTE_OK if operation has been successful
 */
astarte_err_t astarte_storage_iterator_advance(astarte_storage_iterator_t *iterator);

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
 * @param[out] out_major Pointer to output major version. Might be NULL, in this case the parameter
 * is ignored.
 * @param[out] out_data Pointer to the output value. May be NULL, in this case required length will
 * be returned in out_data_len argument.
 * @param[inout] out_data_len A non-zero pointer to the variable holding the length of out_value.
 * In case out_value is NULL, will be set to the length required to hold the property value.
 * In case out_value is not NULL, will be set to the actual length of the property value written.
 * @return One of the follwing error codes:
 * - ASTARTE_ERR_INVALID_SIZE if one of the provided out_* buffers is too small,
 * - ASTARTE_ERR if the operation failed for other reasons,
 * - ASTARTE_OK if the operation has been successful
 */
astarte_err_t astarte_storage_iterator_get_property(astarte_storage_iterator_t *iterator,
    char *out_interface_name, size_t *out_interface_name_len, void *out_path, size_t *out_path_len,
    int32_t *out_major, void *out_data, size_t *out_data_len);

#endif /* _ASTARTE_STORAGE_H_ */
