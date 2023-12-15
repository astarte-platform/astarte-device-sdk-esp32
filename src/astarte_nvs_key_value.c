/*
 * (C) Copyright 2023, SECO Mind Srl
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

#include <inttypes.h>
#include <string.h>

#include <esp_log.h>

/************************************************
 *        Defines, constants and typedef        *
 ***********************************************/

#define TAG "NVS_KEY_VALUE"

/************************************************
 *         Static functions declaration         *
 ***********************************************/

/**
 * @brief Find the key position (store index) for the provided key.
 *
 * @details Loops over all the keys stored in NVS and returns the store index corresponding to the
 * matching key, if found.
 *
 * @param[in] handle Handle obtained from nvs_open function.
 * @param[in] key Key to search for.
 * @param[out] store_index Store index corresponding to the provided key, in case of an error this
 * value is left unmodified.
 * @return One of the follwing error codes:
 * - ESP_ERR_NOT_FOUND if the key has not been found,
 * - ESP_FAIL if an internal error has occurred,
 * - ESP_OK if key has been found and store_index is valid
 */
static esp_err_t lookup_key_position(nvs_handle_t handle, const char *key, uint64_t *store_index);

/**
 * @brief Format an entry key in the format 'EntryNx'.
 *
 * @param[in] store_index Store index to use as 'x'.
 * @param[out] entry_name Output buffer where to store the formatted entry name. Should be
 * pre-allocated with a size of at least NVS_KEY_NAME_MAX_SIZE.
 * @return One of the follwing error codes:
 * - ESP_FAIL if an internal error has occurred,
 * - ESP_OK if formatting has been performed successfully
 */
static esp_err_t get_entry_name(uint64_t store_index, char *entry_name);

/************************************************
 *         Global functions definitions         *
 ***********************************************/

esp_err_t astarte_nvs_key_value_set(
    nvs_handle_t handle, const char *key, const void *value, size_t length)
{
    esp_err_t esp_err = ESP_OK;

    // Step 1: Set the store index with the logic:
    // - If the key is already in NVS set it to the store index of the old entry
    // - If the key is not in NVS set it to the next store index
    // - If the key is not in NVS and there is no next store index set it to 0
    uint64_t key_store_index = 0;
    esp_err_t lookup_err = lookup_key_position(handle, key, &key_store_index);
    if (lookup_err == ESP_ERR_NVS_NOT_FOUND) {
        uint64_t next_store_index = 0;
        esp_err = nvs_get_u64(handle, "next store idx", &next_store_index);
        if ((esp_err != ESP_ERR_NVS_NOT_FOUND) && (esp_err != ESP_OK)) {
            ESP_LOGE(TAG, "Error getting the next store index.");
            return esp_err;
        }
        key_store_index = next_store_index;
    } else if (lookup_err != ESP_OK) {
        return ESP_FAIL;
    }

    // Step 2: store the key
    char key_entry_name[NVS_KEY_NAME_MAX_SIZE] = { 0 };
    esp_err = get_entry_name(key_store_index, key_entry_name);
    if (esp_err != ESP_OK) {
        return ESP_FAIL;
    }
    // Confusing for clang-tidy as second parameter is called 'key'
    // NOLINTNEXTLINE(readability-suspicious-call-argument)
    esp_err = nvs_set_str(handle, key_entry_name, key);
    if (esp_err != ESP_OK) {
        ESP_LOGE(TAG, "Error storing the key.");
        return esp_err;
    }

    // Step 3: store the value
    char value_entry_name[NVS_KEY_NAME_MAX_SIZE] = { 0 };
    esp_err = get_entry_name(key_store_index + 1, value_entry_name);
    if (esp_err != ESP_OK) {
        nvs_erase_key(handle, key_entry_name);
        return ESP_FAIL;
    }
    esp_err = nvs_set_blob(handle, value_entry_name, value, length);
    if (esp_err != ESP_OK) {
        ESP_LOGE(TAG, "Error storing the value.");
        nvs_erase_key(handle, key_entry_name);
        return esp_err;
    }

    // Step 4: If the key was not already in NVS increment the next store index
    if (lookup_err == ESP_ERR_NVS_NOT_FOUND) {
        esp_err = nvs_set_u64(handle, "next store idx", key_store_index + 2);
        if (esp_err != ESP_OK) {
            ESP_LOGE(TAG, "Error updating the next store index.");
            nvs_erase_key(handle, key_entry_name);
            nvs_erase_key(handle, value_entry_name);
            return esp_err;
        }
    }

    return esp_err;
}

esp_err_t astarte_nvs_key_value_get(
    nvs_handle_t handle, const char *key, void *out_value, size_t *length)
{
    // Step 1: Find the store index for the key
    uint64_t key_store_index = 0;
    esp_err_t esp_err = lookup_key_position(handle, key, &key_store_index);
    if (esp_err != ESP_OK) {
        return esp_err;
    }

    // Step 2: Get the value using the store index
    char value_entry_name[NVS_KEY_NAME_MAX_SIZE] = { 0 };
    esp_err = get_entry_name(key_store_index + 1, value_entry_name);
    if (esp_err != ESP_OK) {
        return ESP_FAIL;
    }
    esp_err = nvs_get_blob(handle, value_entry_name, out_value, length);
    if (esp_err != ESP_OK) {
        ESP_LOGE(TAG, "Error fetching the value.");
    }
    return esp_err;
}

esp_err_t astarte_nvs_key_value_erase_key(nvs_handle_t handle, const char *key)
{
    // Step 1: Find the store index for the key
    uint64_t key_store_index = 0;
    esp_err_t esp_err = lookup_key_position(handle, key, &key_store_index);
    if (esp_err != ESP_OK) {
        return esp_err;
    }

    // Step 2: Find the next store index
    uint64_t next_store_index = 0;
    esp_err = nvs_get_u64(handle, "next store idx", &next_store_index);
    if (esp_err != ESP_OK) {
        ESP_LOGE(TAG, "Error getting the next store index.");
        return esp_err;
    }

    // Step 3: Shift back all the keys of one position (erasing the entry in the process)
    for (uint64_t i = key_store_index + 2; i < next_store_index; i += 2) {
        // Get the key to shift using the store index
        char tmp_key_entry_name[NVS_KEY_NAME_MAX_SIZE] = { 0 };
        esp_err = get_entry_name(i, tmp_key_entry_name);
        if (esp_err != ESP_OK) {
            return ESP_FAIL;
        }
        size_t tmp_key_len = 0;
        esp_err = nvs_get_str(handle, tmp_key_entry_name, NULL, &tmp_key_len);
        if (esp_err != ESP_OK) {
            ESP_LOGE(TAG, "Error fetching key from nvs during erase operation.");
            return esp_err;
        }
        char *tmp_key = calloc(tmp_key_len, sizeof(char));
        if (!tmp_key) {
            ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
            return ESP_FAIL;
        }
        // Confusing for clang-tidy as second parameter is called 'key'
        // NOLINTNEXTLINE(readability-suspicious-call-argument)
        esp_err = nvs_get_str(handle, tmp_key_entry_name, tmp_key, &tmp_key_len);
        if (esp_err != ESP_OK) {
            ESP_LOGE(TAG, "Error fetching key from nvs during erase operation.");
            free(tmp_key);
            return esp_err;
        }
        // Get the value to shift using the store index
        char tmp_value_entry_name[NVS_KEY_NAME_MAX_SIZE] = { 0 };
        esp_err = get_entry_name(i + 1, tmp_value_entry_name);
        if (esp_err != ESP_OK) {
            free(tmp_key);
            return ESP_FAIL;
        }
        size_t tmp_value_len = 0;
        esp_err = nvs_get_blob(handle, tmp_value_entry_name, NULL, &tmp_value_len);
        if (esp_err != ESP_OK) {
            ESP_LOGE(TAG, "Error fetching value from nvs during erase operation.");
            free(tmp_key);
            return esp_err;
        }
        char *tmp_value = calloc(tmp_value_len, sizeof(char));
        if (!tmp_value) {
            ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
            free(tmp_key);
            return ESP_FAIL;
        }
        esp_err = nvs_get_blob(handle, tmp_value_entry_name, tmp_value, &tmp_value_len);
        if (esp_err != ESP_OK) {
            ESP_LOGE(TAG, "Error fetching value from nvs during erase operation.");
            free(tmp_key);
            free(tmp_value);
            return esp_err;
        }
        // Store the key in the new position
        char new_key_entry_name[NVS_KEY_NAME_MAX_SIZE] = { 0 };
        esp_err = get_entry_name(i - 2, new_key_entry_name);
        if (esp_err != ESP_OK) {
            free(tmp_key);
            free(tmp_value);
            return ESP_FAIL;
        }
        // Confusing for clang-tidy as second parameter is called 'key'
        // NOLINTNEXTLINE(readability-suspicious-call-argument)
        esp_err = nvs_set_str(handle, new_key_entry_name, tmp_key);
        free(tmp_key);
        if (esp_err != ESP_OK) {
            ESP_LOGE(TAG, "Error storing the key.");
            free(tmp_value);
            return esp_err;
        }
        // Store the value in the new position
        char new_value_entry_name[NVS_KEY_NAME_MAX_SIZE] = { 0 };
        esp_err = get_entry_name(i - 1, new_value_entry_name);
        if (esp_err != ESP_OK) {
            free(tmp_value);
            return ESP_FAIL;
        }
        esp_err = nvs_set_blob(handle, new_value_entry_name, tmp_value, tmp_value_len);
        free(tmp_value);
        if (esp_err != ESP_OK) {
            ESP_LOGE(TAG, "Error storing the value.");
            return esp_err;
        }
    }

    // Step 4: Erase the last two entries that are dangling at this point
    char key_to_erase_entry_name[NVS_KEY_NAME_MAX_SIZE] = { 0 };
    esp_err = get_entry_name(next_store_index - 2, key_to_erase_entry_name);
    if (esp_err != ESP_OK) {
        return ESP_FAIL;
    }
    esp_err = nvs_erase_key(handle, key_to_erase_entry_name);
    if (esp_err != ESP_OK) {
        ESP_LOGE(TAG, "Failed erasing a key.");
        return esp_err;
    }
    char value_to_erase_entry_name[NVS_KEY_NAME_MAX_SIZE] = { 0 };
    esp_err = get_entry_name(next_store_index - 1, value_to_erase_entry_name);
    if (esp_err != ESP_OK) {
        return ESP_FAIL;
    }
    esp_err = nvs_erase_key(handle, value_to_erase_entry_name);
    if (esp_err != ESP_OK) {
        ESP_LOGE(TAG, "Failed erasing a key.");
        return esp_err;
    }

    // Step 5: Update the next store index
    esp_err = nvs_set_u64(handle, "next store idx", next_store_index - 2);
    if (esp_err != ESP_OK) {
        ESP_LOGE(TAG, "Error updating the next store index.");
        return esp_err;
    }

    return esp_err;
}

esp_err_t astarte_nvs_key_value_iterator_init(
    nvs_handle_t handle, nvs_type_t type, astarte_nvs_key_value_iterator_t *iterator)
{
    if ((type != NVS_TYPE_BLOB) && (type != NVS_TYPE_I32)) {
        ESP_LOGE(TAG, "Only blob type and int32 are supported by astarte_nvs_key_value driver.");
        return ESP_FAIL;
    }

    // Check that there is at least one key/value pair in NVS by reading the next store index
    uint64_t next_store_index = 0;
    esp_err_t esp_err = nvs_get_u64(handle, "next store idx", &next_store_index);
    if ((esp_err == ESP_ERR_NVS_NOT_FOUND) || ((esp_err == ESP_OK) && (next_store_index == 0))) {
        return ESP_ERR_NVS_NOT_FOUND;
    }
    if (esp_err != ESP_OK) {
        ESP_LOGE(TAG, "Error getting the next store index.");
        return esp_err;
    }

    // Initialize the iterator
    iterator->store_index = 0;
    iterator->handle = handle;
    iterator->type = type;

    return esp_err;
}

esp_err_t astarte_nvs_key_value_iterator_peek(
    astarte_nvs_key_value_iterator_t *iterator, bool *has_next)
{
    // Get the next store index
    uint64_t next_store_index = 0;
    esp_err_t esp_err = nvs_get_u64(iterator->handle, "next store idx", &next_store_index);
    if ((esp_err == ESP_ERR_NVS_NOT_FOUND) || ((esp_err == ESP_OK) && (next_store_index == 0))) {
        *has_next = false;
        return ESP_OK;
    }
    if (esp_err != ESP_OK) {
        ESP_LOGE(TAG, "Error getting the next store index.");
        return esp_err;
    }

    // Check if the current pair is the last one
    if (iterator->store_index + 2 >= next_store_index) {
        *has_next = false;
    } else {
        *has_next = true;
    }
    return ESP_OK;
}

esp_err_t astarte_nvs_key_value_iterator_next(astarte_nvs_key_value_iterator_t *iterator)
{
    bool has_next = false;
    esp_err_t esp_err = astarte_nvs_key_value_iterator_peek(iterator, &has_next);
    if (esp_err != ESP_OK) {
        return esp_err;
    }

    if (!has_next) {
        return ESP_ERR_NVS_NOT_FOUND;
    }

    // Advance the iterator
    iterator->store_index += 2;

    return ESP_OK;
}

esp_err_t astarte_nvs_key_value_iterator_get_element(astarte_nvs_key_value_iterator_t *iterator,
    char *out_key, size_t *out_key_len, void *out_value, size_t *out_value_len)
{
    if (iterator->type != NVS_TYPE_BLOB) {
        ESP_LOGE(TAG, "Calling getter function for binary blob over an iterator of other type.");
        return ESP_FAIL;
    }

    // Step 1: Get the key using the store index contained in the iterator
    char key_entry_name[NVS_KEY_NAME_MAX_SIZE] = { 0 };
    esp_err_t esp_err = get_entry_name(iterator->store_index, key_entry_name);
    if (esp_err != ESP_OK) {
        return ESP_FAIL;
    }
    size_t key_len = 0;
    esp_err = nvs_get_str(iterator->handle, key_entry_name, NULL, &key_len);
    if (esp_err != ESP_OK) {
        ESP_LOGE(TAG, "Error getting the key length for %s", key_entry_name);
        return ESP_FAIL;
    }
    // If out_key is null return only the required size for out_key to be stored
    if (!out_key) {
        *out_key_len = key_len;
    } else if (key_len > *out_key_len) {
        ESP_LOGE(TAG, "Output buffer out_key is insufficient to store the key.");
        return ESP_ERR_INVALID_SIZE;
    } else {
        // Confusing for clang-tidy as second parameter is called 'key'
        // NOLINTNEXTLINE(readability-suspicious-call-argument)
        esp_err = nvs_get_str(iterator->handle, key_entry_name, out_key, &key_len);
        if (esp_err != ESP_OK) {
            ESP_LOGE(TAG, "Error fetching key from nvs during iteration.");
            return esp_err;
        }
    }

    // Step 2: Get the value using the store index contained in the iterator
    char value_entry_name[NVS_KEY_NAME_MAX_SIZE] = { 0 };
    esp_err = get_entry_name(iterator->store_index + 1, value_entry_name);
    if (esp_err != ESP_OK) {
        return ESP_FAIL;
    }
    size_t value_len = 0;
    esp_err = nvs_get_blob(iterator->handle, value_entry_name, NULL, &value_len);
    if (esp_err != ESP_OK) {
        ESP_LOGE(TAG, "Error fetching value from nvs during erase operation.");
        return esp_err;
    }
    // If out_value is null return only the required size for out_value to be stored
    if (!out_value) {
        *out_value_len = value_len;
        return ESP_OK;
    }
    if (value_len > *out_value_len) {
        ESP_LOGE(TAG, "Output buffer out_value is insufficient to store the value.");
        return ESP_ERR_INVALID_SIZE;
    }
    esp_err = nvs_get_blob(iterator->handle, value_entry_name, out_value, &value_len);
    if (esp_err != ESP_OK) {
        ESP_LOGE(TAG, "Error fetching key from nvs during iteration.");
        return esp_err;
    }

    return esp_err;
}

/************************************************
 *         Static functions definitions         *
 ***********************************************/

static esp_err_t get_entry_name(uint64_t store_index, char *entry_name)
{
    int ret = snprintf(entry_name, NVS_KEY_NAME_MAX_SIZE, "EntryN%" PRIu64, store_index);
    if ((ret < 0) || (ret >= NVS_KEY_NAME_MAX_SIZE)) {
        ESP_LOGE(TAG, "Maximum number of entries exceeded.");
        return ESP_FAIL;
    }
    return ESP_OK;
}

static esp_err_t lookup_key_position(nvs_handle_t handle, const char *key, uint64_t *store_index)
{
    // Read the next "store" index
    uint64_t next_store_index = 0;
    esp_err_t esp_err = nvs_get_u64(handle, "next store idx", &next_store_index);
    if (esp_err == ESP_ERR_NVS_NOT_FOUND) {
        return ESP_ERR_NVS_NOT_FOUND;
    }
    if (esp_err != ESP_OK) {
        ESP_LOGE(TAG, "Error getting the next store index.");
        return ESP_FAIL;
    }

    // Loop over all the stored entries using the "store" index
    for (uint64_t i = 0; i < next_store_index; i += 2) {
        char tmp_key_entry_name[NVS_KEY_NAME_MAX_SIZE] = { 0 };
        esp_err = get_entry_name(i, tmp_key_entry_name);
        if (esp_err != ESP_OK) {
            return ESP_FAIL;
        }
        size_t tmp_key_len = 0;
        esp_err = nvs_get_str(handle, tmp_key_entry_name, NULL, &tmp_key_len);
        if (esp_err != ESP_OK) {
            ESP_LOGE(TAG, "Error getting the key length for %s", tmp_key_entry_name);
            return ESP_FAIL;
        }
        char *tmp_key = calloc(tmp_key_len, sizeof(char));
        if (!tmp_key) {
            ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
            return ESP_FAIL;
        }
        // Confusing for clang-tidy as second parameter is called 'key'
        // NOLINTNEXTLINE(readability-suspicious-call-argument)
        esp_err = nvs_get_str(handle, tmp_key_entry_name, tmp_key, &tmp_key_len);
        if (esp_err != ESP_OK) {
            ESP_LOGE(TAG, "Error getting the key for %s", tmp_key_entry_name);
            free(tmp_key);
            return ESP_FAIL;
        }
        if (strcmp(key, tmp_key) == 0) {
            *store_index = i;
            free(tmp_key);
            return ESP_OK;
        }
        free(tmp_key);
    }
    return ESP_ERR_NVS_NOT_FOUND;
}
