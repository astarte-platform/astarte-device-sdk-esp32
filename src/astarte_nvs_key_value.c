/*
 * (C) Copyright 2018-2023, SECO Mind Srl
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later OR Apache-2.0
 */

/**
 * @file astarte_nvs_key_value.c
 * @brief Wrapper around the NVS library enabling the use of keys longer than 15 chars.
 *
 * @details
 * Each key-value pair is stored as two separate NVS entries:
 * 1. A string entry containing the key
 * 2. A blob entry containing the value
 *
 * The two entries are stored using incremental keys in the format "EntryNx" where x is an
 * incremental identifier.
 *
 * The number of key-value pairs already stored is written to a special 64 bytes NVS entry with a
 * fixed key "next store idx".
 */

#include "astarte_nvs_key_value.h"

#include <string.h>
#include <inttypes.h>

#include <esp_log.h>

/************************************************
 *        Defines, constants and typedef        *
 ***********************************************/

#define TAG "NVS_KEY_VALUE"

typedef enum
{
    NVS_KEY_VALUE_ERR_OK = 0,
    NVS_KEY_VALUE_ERR_INTERNAL,
    NVS_KEY_VALUE_ERR_NOT_FOUND,
} astarte_nvs_key_value_err_t;

/************************************************
 *         Static functions declaration         *
 ***********************************************/

/**
 * @brief Find the key position (store index) for the provided key.
 *
 * @details Loops over all the keys stored in NVS and returns the store index corresponding to it if
 * found.
 *
 * @param[in] handle Handle obtained from nvs_open function.
 * @param[in] key Key to search for.
 * @param[out] store_index Store index corresponding to the provided key.
 * @return An ESP_OK when operation has been successful, an error code otherwise.
 */
static astarte_nvs_key_value_err_t lookup_key_position(
    nvs_handle_t handle, const char *key, uint64_t *store_index);

/************************************************
 *         Global functions definitions         *
 ***********************************************/

esp_err_t astarte_nvs_key_value_set_blob(
    nvs_handle_t handle, const char *key, const void *value, size_t length)
{
    esp_err_t esp_err = ESP_OK;

    // Check if the key is already present and in case set the store index accordingly
    uint64_t key_store_index = 0;
    astarte_nvs_key_value_err_t lookup_err = lookup_key_position(handle, key, &key_store_index);
    if (lookup_err == NVS_KEY_VALUE_ERR_NOT_FOUND) {
        uint64_t next_store_index = 0;
        esp_err = nvs_get_u64(handle, "next store idx", &next_store_index);
        if ((esp_err != ESP_ERR_NVS_NOT_FOUND) && (esp_err != ESP_OK)) {
            ESP_LOGE(TAG, "Error getting the next store index.");
            return esp_err;
        }
        key_store_index = next_store_index;
    } else if (lookup_err != NVS_KEY_VALUE_ERR_OK) {
        return ESP_FAIL;
    }

    // Step 1 store the key
    char key_key[NVS_KEY_NAME_MAX_SIZE] = { 0 };
    int ret = snprintf(key_key, NVS_KEY_NAME_MAX_SIZE, "EntryN%" PRIu64, key_store_index);
    if ((ret < 0) || (ret >= NVS_KEY_NAME_MAX_SIZE)) {
        ESP_LOGE(TAG, "Maximum number of entries exceeded.");
        return ESP_FAIL;
    }
    // Confusing for clang-tidy as second parameter is called 'key'
    // NOLINTNEXTLINE(readability-suspicious-call-argument)
    esp_err = nvs_set_str(handle, key_key, key);
    if (esp_err != ESP_OK) {
        ESP_LOGE(TAG, "Error storing the key.");
        return esp_err;
    }

    // Step 2 store the value
    char value_key[NVS_KEY_NAME_MAX_SIZE] = { 0 };
    ret = snprintf(value_key, NVS_KEY_NAME_MAX_SIZE, "EntryN%" PRIu64, key_store_index + 1);
    if ((ret < 0) || (ret >= NVS_KEY_NAME_MAX_SIZE)) {
        ESP_LOGE(TAG, "Maximum number of entries exceeded.");
        nvs_erase_key(handle, key_key);
        // TODO check return value of erase operation.
        return ESP_FAIL;
    }
    esp_err = nvs_set_blob(handle, value_key, value, length);
    if (esp_err != ESP_OK) {
        ESP_LOGE(TAG, "Error storing the value.");
        nvs_erase_key(handle, key_key);
        // TODO check return value of erase operation.
        return esp_err;
    }

    // Update the next "store" index
    if (lookup_err == NVS_KEY_VALUE_ERR_NOT_FOUND) {
        esp_err = nvs_set_u64(handle, "next store idx", key_store_index + 2);
        if (esp_err != ESP_OK) {
            ESP_LOGE(TAG, "Error updating the next store index.");
            nvs_erase_key(handle, key_key);
            nvs_erase_key(handle, value_key);
            // TODO check return value of erase operation.
            return esp_err;
        }
    }

    return esp_err;
}

esp_err_t astarte_nvs_key_value_get_blob(
    nvs_handle_t handle, const char *key, void *out_value, size_t *length)
{
    esp_err_t esp_err = ESP_OK;

    // Find the "store" index for the key
    uint64_t key_store_index = 0;
    astarte_nvs_key_value_err_t lookup_err = lookup_key_position(handle, key, &key_store_index);
    if (lookup_err == NVS_KEY_VALUE_ERR_NOT_FOUND) {
        return ESP_ERR_NVS_NOT_FOUND;
    }
    if (lookup_err != NVS_KEY_VALUE_ERR_OK) {
        return ESP_FAIL;
    }

    // Get the value using the store index
    char value_key[NVS_KEY_NAME_MAX_SIZE] = { 0 };
    int ret = snprintf(value_key, NVS_KEY_NAME_MAX_SIZE, "EntryN%" PRIu64, key_store_index + 1);
    if ((ret < 0) || (ret >= NVS_KEY_NAME_MAX_SIZE)) {
        ESP_LOGE(TAG, "Maximum number of entries exceeded.");
        return ESP_FAIL;
    }
    esp_err = nvs_get_blob(handle, value_key, out_value, length);
    if (esp_err != ESP_OK) {
        ESP_LOGE(TAG, "Error fetching the value.");
        return esp_err;
    }

    return esp_err;
}

esp_err_t astarte_nvs_key_value_erase_key(nvs_handle_t handle, const char *key)
{

    esp_err_t esp_err = ESP_OK;

    // Find the "store" index for the key
    uint64_t key_store_index = 0;
    astarte_nvs_key_value_err_t lookup_err = lookup_key_position(handle, key, &key_store_index);
    if (lookup_err == NVS_KEY_VALUE_ERR_NOT_FOUND) {
        return ESP_ERR_NVS_NOT_FOUND;
    }
    if (lookup_err != NVS_KEY_VALUE_ERR_OK) {
        return ESP_FAIL;
    }

    // Find the maximum "store" index (TODO make this part of lookup_key_position())
    uint64_t next_store_index = 0;
    esp_err = nvs_get_u64(handle, "next store idx", &next_store_index);
    if (esp_err != ESP_OK) {
        ESP_LOGE(TAG, "Error getting the next store index.");
        return esp_err;
    }

    // Shift all keys of one position
    for (uint64_t i = key_store_index + 2; i < next_store_index; i += 2) {
        // Get the key to shift using the store index
        char tmp_key_key[NVS_KEY_NAME_MAX_SIZE] = { 0 };
        int snprintf_ret = snprintf(tmp_key_key, NVS_KEY_NAME_MAX_SIZE, "EntryN%" PRIu64, i);
        if ((snprintf_ret < 0) || (snprintf_ret >= NVS_KEY_NAME_MAX_SIZE)) {
            ESP_LOGE(TAG, "Maximum number of entries exceeded.");
            return ESP_FAIL;
        }
        size_t key_length = 0;
        esp_err = nvs_get_str(handle, tmp_key_key, NULL, &key_length);
        if (esp_err != ESP_OK) {
            ESP_LOGE(TAG, "Error fetching key from nvs during erase operation.");
            return esp_err;
        }
        char *tmp_key = calloc(key_length, sizeof(char));
        if (!tmp_key) {
            ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
            return ESP_FAIL;
        }
        esp_err = nvs_get_str(handle, tmp_key_key, tmp_key, &key_length);
        if (esp_err != ESP_OK) {
            ESP_LOGE(TAG, "Error fetching key from nvs during erase operation.");
            return esp_err;
        }
        // Get the value to shift using the store index
        char tmp_value_key[NVS_KEY_NAME_MAX_SIZE] = { 0 };
        snprintf_ret = snprintf(tmp_value_key, NVS_KEY_NAME_MAX_SIZE, "EntryN%" PRIu64, i + 1);
        if ((snprintf_ret < 0) || (snprintf_ret >= NVS_KEY_NAME_MAX_SIZE)) {
            ESP_LOGE(TAG, "Maximum number of entries exceeded.");
            free(tmp_key);
            return ESP_FAIL;
        }
        size_t value_length = 0;
        esp_err = nvs_get_blob(handle, tmp_value_key, NULL, &value_length);
        if (esp_err != ESP_OK) {
            ESP_LOGE(TAG, "Error fetching value from nvs during erase operation.");
            free(tmp_key);
            return esp_err;
        }
        char *tmp_value = calloc(value_length, sizeof(char));
        if (!tmp_value) {
            ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
            free(tmp_key);
            return ESP_FAIL;
        }
        esp_err = nvs_get_blob(handle, tmp_value_key, tmp_value, &value_length);
        if (esp_err != ESP_OK) {
            ESP_LOGE(TAG, "Error fetching value from nvs during erase operation.");
            free(tmp_key);
            return esp_err;
        }
        // Store the key in the new position
        char new_key_key[NVS_KEY_NAME_MAX_SIZE] = { 0 };
        snprintf_ret = snprintf(new_key_key, NVS_KEY_NAME_MAX_SIZE, "EntryN%" PRIu64, i - 2);
        if ((snprintf_ret < 0) || (snprintf_ret >= NVS_KEY_NAME_MAX_SIZE)) {
            ESP_LOGE(TAG, "Maximum number of entries exceeded.");
            free(tmp_key);
            free(tmp_value);
            return ESP_FAIL;
        }
        esp_err = nvs_set_str(handle, new_key_key, tmp_key);
        free(tmp_key);
        if (esp_err != ESP_OK) {
            ESP_LOGE(TAG, "Error storing the key.");
            free(tmp_value);
            return esp_err;
        }
        // Store the value in the new position
        char new_value_key[NVS_KEY_NAME_MAX_SIZE] = { 0 };
        snprintf_ret = snprintf(new_value_key, NVS_KEY_NAME_MAX_SIZE, "EntryN%" PRIu64, i - 1);
        if ((snprintf_ret < 0) || (snprintf_ret >= NVS_KEY_NAME_MAX_SIZE)) {
            ESP_LOGE(TAG, "Maximum number of entries exceeded.");
            free(tmp_value);
            return ESP_FAIL;
        }
        esp_err = nvs_set_blob(handle, new_value_key, tmp_value, value_length);
        free(tmp_value);
        if (esp_err != ESP_OK) {
            ESP_LOGE(TAG, "Error storing the value.");
            return esp_err;
        }
    }

    // Erase the last two entries that are dangling at this point
    char key_key_to_erase[NVS_KEY_NAME_MAX_SIZE] = { 0 };
    int snprintf_ret
        = snprintf(key_key_to_erase, NVS_KEY_NAME_MAX_SIZE, "EntryN%" PRIu64, next_store_index - 2);
    if ((snprintf_ret < 0) || (snprintf_ret >= NVS_KEY_NAME_MAX_SIZE)) {
        ESP_LOGE(TAG, "Maximum number of entries exceeded.");
        return ESP_FAIL;
    }
    nvs_erase_key(handle, key_key_to_erase);
    char value_key_to_erase[NVS_KEY_NAME_MAX_SIZE] = { 0 };
    snprintf_ret = snprintf(
        value_key_to_erase, NVS_KEY_NAME_MAX_SIZE, "EntryN%" PRIu64, next_store_index - 1);
    if ((snprintf_ret < 0) || (snprintf_ret >= NVS_KEY_NAME_MAX_SIZE)) {
        ESP_LOGE(TAG, "Maximum number of entries exceeded.");
        return ESP_FAIL;
    }
    nvs_erase_key(handle, value_key_to_erase);

    // Update the maximum "store" index
    esp_err = nvs_set_u64(handle, "next store idx", next_store_index - 2);
    if (esp_err != ESP_OK) {
        ESP_LOGE(TAG, "Error updating the next store index.");
        return esp_err;
    }

    return esp_err;
}

esp_err_t astarte_nvs_key_value_iterator_init(
    nvs_handle_t handle, nvs_type_t type, astarte_nvs_key_value_iterator_t *output_iterator)
{
    if (type != NVS_TYPE_BLOB) {
        ESP_LOGE(TAG, "Only blob type is supported by astarte_nvs_key_value driver.");
        return ESP_FAIL;
    }

    // Get the store index (it is not used here, but it ensures that there is at least one pair)
    uint64_t next_store_index = 0;
    esp_err_t esp_err = nvs_get_u64(handle, "next store idx", &next_store_index);
    if ((esp_err == ESP_ERR_NVS_NOT_FOUND) || ((esp_err == ESP_OK) && (next_store_index == 0))) {
        return ESP_ERR_NVS_NOT_FOUND;
    }
    if (esp_err != ESP_OK) {
        ESP_LOGE(TAG, "Error getting the next store index.");
        return esp_err;
    }

    // Fill in the iterator
    output_iterator->next_iter_index = 0;
    output_iterator->handle = handle;

    return esp_err;
}

esp_err_t astarte_nvs_key_value_iterator_next(astarte_nvs_key_value_iterator_t *iterator)
{
    // Get the store index (might have changed from last call of this function)
    uint64_t next_store_index = 0;
    esp_err_t esp_err = nvs_get_u64(iterator->handle, "next store idx", &next_store_index);
    if ((esp_err == ESP_ERR_NVS_NOT_FOUND) || ((esp_err == ESP_OK) && (next_store_index == 0))) {
        return ESP_ERR_NVS_NOT_FOUND;
    }
    if (esp_err != ESP_OK) {
        ESP_LOGE(TAG, "Error getting the next store index.");
        return esp_err;
    }

    // Check if the current pair is the last one
    if (iterator->next_iter_index + 2 >= next_store_index) {
        return ESP_ERR_NVS_NOT_FOUND;
    }

    // Advance the iterator
    iterator->next_iter_index += 2;

    return ESP_OK;
}

esp_err_t astarte_nvs_key_value_iterator_get_element(astarte_nvs_key_value_iterator_t *iterator,
    char *out_key, size_t *out_key_len, void *out_value, size_t *out_value_len)
{
    // Get element key NVS key
    char key_key[NVS_KEY_NAME_MAX_SIZE] = { 0 };
    int ret = snprintf(key_key, NVS_KEY_NAME_MAX_SIZE, "EntryN%" PRIu64, iterator->next_iter_index);
    if ((ret < 0) || (ret >= NVS_KEY_NAME_MAX_SIZE)) {
        ESP_LOGE(TAG, "Maximum number of entries exceeded.");
        return ESP_FAIL;
    }

    // Get key length
    size_t key_length = 0;
    esp_err_t esp_err = nvs_get_str(iterator->handle, key_key, NULL, &key_length);
    if (esp_err != ESP_OK) {
        ESP_LOGE(TAG, "Error getting the key length for %s", key_key);
        return ESP_FAIL;
    }

    // Get key, only if out_key is not null
    if (out_key == NULL) {
        *out_key_len = key_length;
    } else if (key_length > *out_key_len) {
        ESP_LOGE(TAG, "Output buffer out_key is insufficient to store the key.");
        return ESP_ERR_INVALID_SIZE;
    } else {
        esp_err = nvs_get_str(iterator->handle, key_key, out_key, &key_length);
        if (esp_err != ESP_OK) {
            ESP_LOGE(TAG, "Error fetching key from nvs during iteration.");
            return esp_err;
        }
    }

    // Get element value NVS key
    char key_value[NVS_KEY_NAME_MAX_SIZE] = { 0 };
    ret = snprintf(
        key_value, NVS_KEY_NAME_MAX_SIZE, "EntryN%" PRIu64, iterator->next_iter_index + 1);
    if ((ret < 0) || (ret >= NVS_KEY_NAME_MAX_SIZE)) {
        ESP_LOGE(TAG, "Maximum number of entries exceeded.");
        return ESP_FAIL;
    }

    // Get value length
    size_t value_length = 0;
    esp_err = nvs_get_blob(iterator->handle, key_value, NULL, &value_length);
    if (esp_err != ESP_OK) {
        ESP_LOGE(TAG, "Error fetching value from nvs during erase operation.");
        return esp_err;
    }

    // If out value is NULL, return the required length
    if (out_value == NULL) {
        *out_value_len = value_length;
        return ESP_OK;
    }

    // Check of out value has insufficient length
    if (value_length > *out_value_len) {
        ESP_LOGE(TAG, "Output buffer out_value is insufficient to store the value.");
        return ESP_ERR_INVALID_SIZE;
    }

    // Get actual value
    esp_err = nvs_get_blob(iterator->handle, key_value, out_value, &value_length);
    if (esp_err != ESP_OK) {
        ESP_LOGE(TAG, "Error fetching key from nvs during iteration.");
        return esp_err;
    }

    return esp_err;
}

/************************************************
 *         Static functions definitions         *
 ***********************************************/

static astarte_nvs_key_value_err_t lookup_key_position(
    nvs_handle_t handle, const char *key, uint64_t *store_index)
{
    // Find the maximum "store" index
    uint64_t next_store_index = 0;
    esp_err_t esp_err = nvs_get_u64(handle, "next store idx", &next_store_index);
    if (esp_err == ESP_ERR_NVS_NOT_FOUND) {
        return NVS_KEY_VALUE_ERR_NOT_FOUND;
    }
    if (esp_err != ESP_OK) {
        ESP_LOGE(TAG, "Error getting the next store index.");
        return NVS_KEY_VALUE_ERR_INTERNAL;
    }

    // Loop over all the stored entries using the "store" index
    for (uint64_t i = 0; i < next_store_index; i += 2) {
        char key_key[NVS_KEY_NAME_MAX_SIZE] = { 0 };
        int ret = snprintf(key_key, NVS_KEY_NAME_MAX_SIZE, "EntryN%" PRIu64, i);
        if ((ret < 0) || (ret >= NVS_KEY_NAME_MAX_SIZE)) {
            ESP_LOGE(TAG, "Maximum number of entries exceeded.");
            return NVS_KEY_VALUE_ERR_INTERNAL;
        }

        size_t str_length = 0;
        esp_err = nvs_get_str(handle, key_key, NULL, &str_length);
        if (esp_err != ESP_OK) {
            ESP_LOGE(TAG, "Error getting the key length for %s", key_key);
            return NVS_KEY_VALUE_ERR_INTERNAL;
        }

        char *read_key = calloc(str_length, sizeof(char));
        esp_err = nvs_get_str(handle, key_key, read_key, &str_length);
        if (esp_err != ESP_OK) {
            ESP_LOGE(TAG, "Error getting the key for %s", key_key);
            free(read_key);
            return NVS_KEY_VALUE_ERR_INTERNAL;
        }

        if (strcmp(key, read_key) == 0) {
            *store_index = i;
            free(read_key);
            return NVS_KEY_VALUE_ERR_OK;
        }
        free(read_key);
    }
    return NVS_KEY_VALUE_ERR_NOT_FOUND;
}
