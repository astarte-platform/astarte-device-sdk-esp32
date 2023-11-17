/*
 * (C) Copyright 2018-2023, SECO Mind Srl
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later OR Apache-2.0
 */

/**
 * @file astarte_nvs_key_value.h
 * @brief Wrapper around the NVS library enabling the use of keys longer than 15 chars.
 *
 * @details The use of longer keys is made possible by storing both key and value as separate
 * NVS values. This leads to overhead in terms of computational complexity and lookup speed.
 */

#ifndef _ASTARTE_NVS_KEY_VALUE_H_
#define _ASTARTE_NVS_KEY_VALUE_H_

#include <nvs.h>

typedef struct
{
    uint64_t next_iter_index;
    nvs_handle_t handle;
} astarte_nvs_key_value_iterator_t;

/**
 * @brief Set variable length binary value for given key.
 *
 * @details This function API behaves as similar as possible to the nvs_set_blob function found in
 * nvs_flash.
 *
 * @param[in] handle Handle obtained from nvs_open function. Read only handles cannot be used.
 * @param[in] key Key name. Maximum length is 4000 bytes, including the terminating char.
 * @param[in] value Buffer containing the value to set.
 * @param[in] length Length of binary value to set, in bytes.
 * @return An ESP_OK when operation has been successful, an error code otherwise.
 */
esp_err_t astarte_nvs_key_value_set_blob(
    nvs_handle_t handle, const char *key, const void *value, size_t length);

/**
 * @brief Get blob value for given key.
 *
 * @details This function API behaves as similar as possible to the nvs_get_blob function found in
 * nvs_flash.
 *
 * @param[in] handle Handle obtained from nvs_open function.
 * @param[in] key Key name. Maximum length is 4000 bytes, including the terminating char.
 * @param[out] out_value Pointer to the output value. May be NULL, in this case required length will
 * be returned in length argument.
 * @param[inout] length A non-zero pointer to the variable holding the length of out_value. In case
 * out_value is NULL, will be set to the length required to hold the value. In case out_value is
 * not NULL, will be set to the actual length of the value written.
 * @return An ESP_OK when operation has been successful, an error code otherwise.
 */
esp_err_t astarte_nvs_key_value_get_blob(
    nvs_handle_t handle, const char *key, void *out_value, size_t *length);

/**
 * @brief Erase key-value pair with given key name.
 *
 * @details This function API behaves as similar as possible to the nvs_erase_key function found in
 * nvs_flash. However, its efficiency is incomparably lower as this function may require a large
 * amount of write and read operations.
 *
 * @param[in] handle Handle obtained from nvs_open function. Read only handles cannot be used.
 * @param[in] key Key name. Maximum length is 4000 bytes, including the terminating char.
 * @return An ESP_OK when operation has been successful, an error code otherwise.
 */
esp_err_t astarte_nvs_key_value_erase_key(nvs_handle_t handle, const char *key);

/**
 * @brief Create an iterator to enumerate NVS entries.
 *
 * @details When the return value is ESP_OK the iterator is already pointing to the first entry.
 * The `*_get_element()` function should be called before advancing the iterator with `*_next()`.
 *
 * @note During iteration over an NVS namespace the NVS content is assumed to remain unchanged.
 * Meaning that if while iterating key/value pairs are added/removed, then the iterator should be
 * considered invalid. A new iterator should be created and iteration should re-start from the
 * beginning.
 * There is a single exception to this rule. If while iterating, the key/value pair pointed to by
 * the iterator is deleted, then the iterator remains valid and will automatically point to the
 * next item.
 * Note that in this scenario after deleting the key/value pair the 'get_element' function should
 * be called as the iterator has been already advanced by the call to the `erase_key` function.
 *
 * @param[in] handle Handle obtained from nvs_open function.
 * @param[in] type One of nvs_type_t values. Should be NVS_TYPE_BLOB as its the only type supported
 * by this driver at the moment.
 * @param[out] output_iterator Initialized iterator.
 * @return An ESP_OK when operation has been successful, an error code otherwise.
 */
esp_err_t astarte_nvs_key_value_iterator_init(
    nvs_handle_t handle, nvs_type_t type, astarte_nvs_key_value_iterator_t *output_iterator);

/**
 * @brief Advances the iterator to next item.
 *
 * @param[inout] iterator Iterator obtained from astarte_nvs_key_value_iterator_init.
 * @return An ESP_OK when operation has been successful, an error code otherwise.
 */
esp_err_t astarte_nvs_key_value_iterator_next(astarte_nvs_key_value_iterator_t *iterator);

/**
 * @brief Get the entry currently pointed to by the iterator.
 *
 * @details This function can be used in a similar manner as any other getter function from nvs.
 * The function can be called first with the buffer pointers set to NULL to obtained the required
 * buffer sizes. Then called a second time with a pair of pre-allocated buffers.
 *
 * @param[inout] iterator Iterator obtained from astarte_nvs_key_value_iterator_init.
 * @param[out] out_key Pointer to the output key. May be NULL, in this case required length will
 * be returned in out_key_len argument.
 * @param[inout] out_key_len A non-zero pointer to the variable holding the length of out_key.
 * In case out_key is NULL, will be set to the length required to hold the key. In case out_key is
 * not NULL, will be set to the actual length of the key written.
 * @param[out] out_value Pointer to the output value. May be NULL, in this case required length will
 * be returned in out_value_len argument.
 * @param[inout] out_value_len A non-zero pointer to the variable holding the length of out_value.
 * In case out_value is NULL, will be set to the length required to hold the value. In case
 * out_value is not NULL, will be set to the actual length of the value written.
 * @return An ESP_OK when operation has been successful, an error code otherwise.
 */
esp_err_t astarte_nvs_key_value_iterator_get_element(astarte_nvs_key_value_iterator_t *iterator,
    char *out_key, size_t *out_key_len, void *out_value, size_t *out_value_len);

#endif /* _ASTARTE_NVS_KEY_VALUE_H_ */
