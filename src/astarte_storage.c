/*
 * (C) Copyright 2023, SECO Mind Srl
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later OR Apache-2.0
 */

#include "astarte_storage.h"

#include <esp_log.h>
#include <nvs.h>
#include <stdlib.h>
#include <string.h>

/************************************************
 *        Defines, constants and typedef        *
 ***********************************************/

#define TAG "ASTARTE_STORAGE"

#define NVS_NAMESPACE "ASTARTE_STORAGE"

/************************************************
 *         Global functions definitions         *
 ***********************************************/

astarte_err_t astarte_storage_open(astarte_storage_handle_t *handle)
{
    esp_err_t esp_err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle->nvs_handle);
    if (esp_err != ESP_OK) {
        return ASTARTE_ERR;
    }
    return ASTARTE_OK;
}

void astarte_storage_close(astarte_storage_handle_t handle)
{
    nvs_close(handle.nvs_handle);
}

astarte_err_t astarte_storage_store_property(astarte_storage_handle_t handle,
    const char *interface_name, const char *path, int32_t major, const void *data, size_t data_len)
{
    // Get the full key interface_name + path
    size_t key_len = strlen(interface_name) + strlen(path) + 1;
    char *key = calloc(key_len, sizeof(char));
    if (!key) {
        ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
        return ASTARTE_ERR;
    }
    strncat(strncpy(key, interface_name, key_len), path, key_len - strlen(interface_name) - 1);

    // Allocate memory for major version + data
    size_t value_len = sizeof(int32_t) + data_len;
    void *value = malloc(value_len);
    if (!value) {
        ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
        free(key);
        return ASTARTE_ERR;
    }
    memcpy(value, &major, sizeof(int32_t));
    memcpy(value + sizeof(int32_t), data, data_len);

    // Set the property value in NVS
    esp_err_t esp_err = astarte_nvs_key_value_set(handle.nvs_handle, key, value, value_len);
    if (esp_err != ESP_OK) {
        free(key);
        free(value);
        return ASTARTE_ERR;
    }
    free(key);
    free(value);

    // Commit the changes
    esp_err = nvs_commit(handle.nvs_handle);
    if (esp_err != ESP_OK) {
        return ASTARTE_ERR;
    }

    return ASTARTE_OK;
}

astarte_err_t astarte_storage_contains_property(astarte_storage_handle_t handle,
    const char *interface_name, const char *path, int32_t major, const void *data, size_t data_len,
    bool *res)
{
    // Set result as false as default
    *res = false;

    // Get the full key interface_name + path
    size_t key_len = strlen(interface_name) + strlen(path) + 1;
    char *key = calloc(key_len, sizeof(char));
    if (!key) {
        ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
        return ASTARTE_ERR;
    }
    strncat(strncpy(key, interface_name, key_len), path, key_len - strlen(interface_name) - 1);

    // Get length of data
    size_t value_len = 0;
    esp_err_t esp_err = astarte_nvs_key_value_get(handle.nvs_handle, key, NULL, &value_len);
    if ((esp_err != ESP_ERR_NVS_NOT_FOUND) && (esp_err != ESP_OK)) {
        free(key);
        return ASTARTE_ERR;
    }
    if ((esp_err == ESP_ERR_NVS_NOT_FOUND) || (value_len != (sizeof(int32_t) + data_len))) {
        free(key);
        return ASTARTE_OK;
    }

    // Allocate temporary data
    char *value = calloc(value_len, sizeof(char));
    if (!value) {
        ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
        free(key);
        return ASTARTE_ERR;
    }

    // Get stored data
    astarte_nvs_key_value_get(handle.nvs_handle, key, value, &value_len);
    free(key);
    if (esp_err != ESP_OK) {
        free(value);
        return ASTARTE_ERR;
    }

    // Check version
    if ((*(int32_t *) value) != major) {
        free(value);
        esp_err = astarte_storage_delete_property(handle, interface_name, path);
        if (esp_err != ESP_OK) {
            return ASTARTE_ERR;
        }
        return ASTARTE_OK;
    }

    // Check stored data
    if (memcmp(value + sizeof(int32_t), data, data_len) == 0) {
        *res = true;
    }

    free(value);
    return ASTARTE_OK;
}

astarte_err_t astarte_storage_load_property(astarte_storage_handle_t handle,
    const char *interface_name, const char *path, int32_t *out_major, void *out_data,
    size_t *out_data_len)
{
    // Get the full key interface_name + path
    size_t key_len = strlen(interface_name) + strlen(path) + 1;
    char *key = calloc(key_len, sizeof(char));
    if (!key) {
        ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
        return ASTARTE_ERR;
    }
    strncat(strncpy(key, interface_name, key_len), path, key_len - strlen(interface_name) - 1);

    // Get length of data
    size_t value_len = 0;
    esp_err_t esp_err = astarte_nvs_key_value_get(handle.nvs_handle, key, NULL, &value_len);
    if (esp_err == ESP_ERR_NVS_NOT_FOUND) {
        free(key);
        return ASTARTE_ERR_NOT_FOUND;
    }
    if (esp_err != ESP_OK) {
        free(key);
        return ASTARTE_ERR;
    }

    // If out data and out major are NULL, return the required length
    size_t data_len = value_len - sizeof(int32_t);
    if (!out_data && !out_major) {
        *out_data_len = data_len;
        free(key);
        return ASTARTE_OK;
    }

    // Allocate memory for major version + data
    void *value = malloc(value_len);
    if (!value) {
        ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
        free(key);
        return ASTARTE_ERR;
    }

    // Get the data from NVS
    astarte_nvs_key_value_get(handle.nvs_handle, key, value, &value_len);
    free(key);
    if (esp_err != ESP_OK) {
        free(value);
        return ASTARTE_ERR;
    }

    // If out major is non null, store it
    if (out_major) {
        *out_major = *(int32_t *) value;
    }

    // If out data is non null, store it
    if (out_data) {
        // Check if out data has sufficient length
        if (data_len > *out_data_len) {
            free(value);
            return ASTARTE_ERR_INVALID_SIZE;
        }
        memcpy(out_data, value + sizeof(int32_t), data_len);
    }
    *out_data_len = data_len;

    free(value);
    return ASTARTE_OK;
}

astarte_err_t astarte_storage_delete_property(
    astarte_storage_handle_t handle, const char *interface_name, const char *path)
{
    // Get the full key interface_name + path
    size_t key_len = strlen(interface_name) + strlen(path) + 1;
    char *key = calloc(key_len, sizeof(char));
    if (!key) {
        ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
        return ASTARTE_ERR;
    }
    strncat(strncpy(key, interface_name, key_len), path, key_len - strlen(interface_name) - 1);

    // Erase the property value using the full key
    esp_err_t esp_err = astarte_nvs_key_value_erase_key(handle.nvs_handle, key);
    free(key);
    if (esp_err == ESP_ERR_NVS_NOT_FOUND) {
        return ASTARTE_ERR_NOT_FOUND;
    }
    if (esp_err != ESP_OK) {
        return ASTARTE_ERR;
    }

    // Commit the changes to NVS
    esp_err = nvs_commit(handle.nvs_handle);
    if (esp_err != ESP_OK) {
        return ASTARTE_ERR;
    }

    return ASTARTE_OK;
}

astarte_err_t astarte_storage_clear(astarte_storage_handle_t handle)
{
    esp_err_t esp_err = nvs_erase_all(handle.nvs_handle);
    if (esp_err != ESP_OK) {
        return ASTARTE_ERR;
    }
    esp_err = nvs_commit(handle.nvs_handle);
    if (esp_err != ESP_OK) {
        return ASTARTE_ERR;
    }
    return ASTARTE_OK;
}

astarte_err_t astarte_storage_iterator_create(
    astarte_storage_handle_t handle, astarte_storage_iterator_t *iterator)
{
    esp_err_t esp_err = astarte_nvs_key_value_iterator_init(
        handle.nvs_handle, NVS_TYPE_BLOB, &iterator->nvs_key_value_iterator);
    if (esp_err == ESP_ERR_NVS_NOT_FOUND) {
        return ASTARTE_ERR_NOT_FOUND;
    }
    if (esp_err != ESP_OK) {
        return ASTARTE_ERR;
    }

    return ASTARTE_OK;
}

astarte_err_t astarte_storage_iterator_peek(astarte_storage_iterator_t *iterator, bool *has_next)
{
    bool tmp_has_next = false;
    esp_err_t esp_err
        = astarte_nvs_key_value_iterator_peek(&iterator->nvs_key_value_iterator, &tmp_has_next);
    if (esp_err != ESP_OK) {
        return ASTARTE_ERR;
    }
    *has_next = tmp_has_next;
    return ASTARTE_OK;
}

astarte_err_t astarte_storage_iterator_advance(astarte_storage_iterator_t *iterator)
{
    esp_err_t esp_err = astarte_nvs_key_value_iterator_next(&iterator->nvs_key_value_iterator);
    if (esp_err == ESP_ERR_NVS_NOT_FOUND) {
        return ASTARTE_ERR_NOT_FOUND;
    }
    if (esp_err != ESP_OK) {
        return ASTARTE_ERR;
    }
    return ASTARTE_OK;
}

astarte_err_t astarte_storage_iterator_get_property(astarte_storage_iterator_t *iterator,
    char *out_interface_name, size_t *out_interface_name_len, void *out_path, size_t *out_path_len,
    int32_t *out_major, void *out_data, size_t *out_data_len)
{
    astarte_err_t astarte_storage_err = ASTARTE_OK;
    char *key = NULL;
    char *value = NULL;
    char *interface_name = NULL;
    char *path = NULL;

    // Get the sizes required for NVS key and value
    size_t key_len = 0;
    size_t value_len = 0;
    esp_err_t esp_err = astarte_nvs_key_value_iterator_get_element(
        &iterator->nvs_key_value_iterator, NULL, &key_len, NULL, &value_len);
    if (esp_err != ESP_OK) {
        astarte_storage_err = ASTARTE_ERR;
        goto end;
    }
    // Allocate required space
    key = calloc(key_len, sizeof(char));
    if (!key) {
        ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
        astarte_storage_err = ASTARTE_ERR;
        goto end;
    }
    value = calloc(value_len, sizeof(char));
    if (!value) {
        ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
        astarte_storage_err = ASTARTE_ERR;
        goto end;
    }
    // Get key and value
    esp_err = astarte_nvs_key_value_iterator_get_element(
        &iterator->nvs_key_value_iterator, key, &key_len, value, &value_len);
    if (esp_err != ESP_OK) {
        astarte_storage_err = ASTARTE_ERR;
        goto end;
    }

    // Split interface name and path
    char *interface_name_p = strtok(key, "/");
    size_t interface_name_len = strlen(interface_name_p) + 1; // Including the \0 terminating char
    interface_name = calloc(interface_name_len, sizeof(char));
    if (!interface_name) {
        ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
        astarte_storage_err = ASTARTE_ERR;
        goto end;
    }
    strncpy(interface_name, interface_name_p, interface_name_len);

    char *path_p = strtok(NULL, "\0");
    size_t path_len = strlen(path_p) + 2; // +1 because the '/' char is removed by strtok, +1 for \0
    path = calloc(path_len, sizeof(char));
    if (!path) {
        ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
        astarte_storage_err = ASTARTE_ERR;
        goto end;
    }
    strncat(strncpy(path, "/", path_len), path_p, path_len - strlen("/") - 1);

    // Free as content has already been stored in interface_name and path
    free(key);
    key = NULL;

    // Copy property information in the outputs
    if (!out_interface_name) {
        *out_interface_name_len = interface_name_len;
    } else if (interface_name_len > *out_interface_name_len) {
        ESP_LOGE(TAG, "Output buffer out_interface_name is of insufficient size.");
        astarte_storage_err = ASTARTE_ERR_INVALID_SIZE;
        goto end;
    } else {
        strncpy(out_interface_name, interface_name, *out_interface_name_len);
        *out_interface_name_len = interface_name_len;
    }

    if (!out_path) {
        *out_path_len = path_len;
    } else if (path_len > *out_path_len) {
        ESP_LOGE(TAG, "Output buffer out_path is of insufficient size.");
        astarte_storage_err = ASTARTE_ERR_INVALID_SIZE;
        goto end;
    } else {
        strncpy(out_path, path, *out_path_len);
        *out_path_len = path_len;
    }

    if (out_major) {
        *out_major = *(int32_t *) value;
    }

    size_t data_len = value_len - sizeof(int32_t);
    if (!out_data) {
        *out_data_len = data_len;
    } else if (data_len > *out_data_len) {
        ESP_LOGE(TAG, "Output buffer out_data is of insufficient size.");
        astarte_storage_err = ASTARTE_ERR_INVALID_SIZE;
        goto end;
    } else {
        memcpy(out_data, value + sizeof(int32_t), data_len);
        *out_data_len = data_len;
    }

end:
    // Free all data
    free(key);
    free(value);
    free(interface_name);
    free(path);

    return astarte_storage_err;
}
