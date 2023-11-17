/*
 * (C) Copyright 2018-2023, SECO Mind Srl
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

astarte_storage_err_t astarte_storage_open(astarte_storage_handle_t *handle)
{
    esp_err_t esp_err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle->nvs_handle);
    if (esp_err != ESP_OK) {
        return ASTARTE_STORAGE_ERR_INTERNAL;
    }
    return ASTARTE_STORAGE_ERR_OK;
}

void astarte_storage_close(astarte_storage_handle_t handle)
{
    nvs_close(handle.nvs_handle);
}

astarte_storage_err_t astarte_storage_store_property(astarte_storage_handle_t handle,
    const char *interface_name, const char *path, const void *data, size_t data_len)
{
    if (!data) {
        // Delete returns ok when property is not present
        return astarte_storage_delete_property(handle, interface_name, path);
    }

    // Get the full key interface_name + path
    size_t interface_name_len = strlen(interface_name);
    size_t path_len = strlen(path);
    char *key = calloc(interface_name_len + path_len + 1, sizeof(char));
    if (!key) {
        return ASTARTE_STORAGE_ERR_INTERNAL;
    }

    strncat(strncat(key, interface_name, interface_name_len), path, path_len);

    esp_err_t esp_err = astarte_nvs_key_value_set_blob(handle.nvs_handle, key, data, data_len);
    free(key);
    if (esp_err != ESP_OK) {
        return ASTARTE_STORAGE_ERR_INTERNAL;
    }

    esp_err = nvs_commit(handle.nvs_handle);
    if (esp_err != ESP_OK) {
        return ASTARTE_STORAGE_ERR_INTERNAL;
    }

    return ASTARTE_STORAGE_ERR_OK;
}

astarte_storage_err_t astarte_storage_load_property(astarte_storage_handle_t handle,
    const char *interface_name, const char *path, void *out_data, size_t *out_data_len)
{
    // Get the full key interface_name + path
    size_t interface_name_len = strlen(interface_name);
    size_t path_len = strlen(path);
    char *key = calloc(interface_name_len + path_len + 1, sizeof(char));
    if (!key) {
        return ASTARTE_STORAGE_ERR_INTERNAL;
    }
    strncat(strncat(key, interface_name, interface_name_len), path, path_len);

    // Get length of data
    size_t length = 0;
    esp_err_t esp_err = astarte_nvs_key_value_get_blob(handle.nvs_handle, key, NULL, &length);
    if (esp_err == ESP_ERR_NVS_NOT_FOUND) {
        free(key);
        return ASTARTE_STORAGE_ERR_NOT_FOUND;
    }
    if (esp_err != ESP_OK) {
        free(key);
        return ASTARTE_STORAGE_ERR_INTERNAL;
    }

    // If out data is NULL, return the required length
    if (!out_data) {
        *out_data_len = length;
        free(key);
        return ASTARTE_STORAGE_ERR_OK;
    }

    // Check of out data has insufficient length
    if (length > *out_data_len) {
        free(key);
        return ASTARTE_STORAGE_ERR_DATA_TOO_SMALL;
    }

    // Get actual data
    astarte_nvs_key_value_get_blob(handle.nvs_handle, key, out_data, out_data_len);
    free(key);
    if (esp_err != ESP_OK) {
        return ASTARTE_STORAGE_ERR_INTERNAL;
    }

    return ASTARTE_STORAGE_ERR_OK;
}

astarte_storage_err_t astarte_storage_delete_property(astarte_storage_handle_t handle,
    const char *interface_name, const char *path)
{
    size_t interface_name_len = strlen(interface_name);
    size_t path_len = strlen(path);
    char *key = calloc(interface_name_len + path_len + 1, sizeof(char));
    if (!key) {
        return ASTARTE_STORAGE_ERR_INTERNAL;
    }
    strncat(strncat(key, interface_name, interface_name_len), path, path_len);

    esp_err_t esp_err = astarte_nvs_key_value_erase_key(handle.nvs_handle, key);
    free(key);
    if (esp_err == ESP_ERR_NVS_NOT_FOUND) {
        return ASTARTE_STORAGE_ERR_OK;
    }
    if (esp_err != ESP_OK) {
        return ASTARTE_STORAGE_ERR_INTERNAL;
    }

    esp_err = nvs_commit(handle.nvs_handle);
    if (esp_err != ESP_OK) {
        return ASTARTE_STORAGE_ERR_INTERNAL;
    }

    return ASTARTE_STORAGE_ERR_OK;
}

astarte_storage_err_t astarte_storage_clear(astarte_storage_handle_t handle)
{
    esp_err_t esp_err = nvs_erase_all(handle.nvs_handle);
    if (esp_err != ESP_OK) {
        return ASTARTE_STORAGE_ERR_INTERNAL;
    }
    esp_err = nvs_commit(handle.nvs_handle);
    if (esp_err != ESP_OK) {
        return ASTARTE_STORAGE_ERR_INTERNAL;
    }
    return ASTARTE_STORAGE_ERR_OK;
}

astarte_storage_err_t astarte_storage_iterator_create(astarte_storage_handle_t handle,
    astarte_storage_iterator_t *iterator)
{
    iterator->handle = handle.nvs_handle;

    esp_err_t esp_err = astarte_nvs_key_value_iterator_init(
        iterator->handle, NVS_TYPE_BLOB, &iterator->nvs_key_value_iterator);
    if (esp_err == ESP_ERR_NVS_NOT_FOUND) {
        return ASTARTE_STORAGE_ERR_NOT_FOUND;
    }
    if (esp_err != ESP_OK) {
        return ASTARTE_STORAGE_ERR_INTERNAL;
    }

    return ASTARTE_STORAGE_ERR_OK;
}

astarte_storage_err_t astarte_storage_iterator_advance(astarte_storage_iterator_t *iterator)
{
    esp_err_t esp_err = astarte_nvs_key_value_iterator_next(&iterator->nvs_key_value_iterator);
    if (esp_err == ESP_ERR_NVS_NOT_FOUND) {
        return ASTARTE_STORAGE_ERR_NOT_FOUND;
    }
    if (esp_err != ESP_OK) {
        return ASTARTE_STORAGE_ERR_INTERNAL;
    }
    return ASTARTE_STORAGE_ERR_OK;
}

astarte_storage_err_t astarte_storage_iterator_get_property(astarte_storage_iterator_t *iterator,
    char *out_interface_name, size_t *out_interface_name_len, void *out_path, size_t *out_path_len,
    void *out_value, size_t *out_value_len)
{
    astarte_storage_err_t astarte_storage_err = ASTARTE_STORAGE_ERR_OK;
    char *key = NULL;
    char *value = NULL;
    char *interface_name = NULL;
    char *path = NULL;

    size_t key_len = 0;
    size_t value_len = 0;
    esp_err_t esp_err = astarte_nvs_key_value_iterator_get_element(
        &iterator->nvs_key_value_iterator, NULL, &key_len, NULL, &value_len);
    if (esp_err != ESP_OK) {
        astarte_storage_err = ASTARTE_STORAGE_ERR_INTERNAL;
        goto end;
    }

    key = calloc(key_len, sizeof(char));
    if (!key) {
        ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
        astarte_storage_err = ASTARTE_STORAGE_ERR_INTERNAL;
        goto end;
    }
    value = calloc(value_len, sizeof(char));
    if (!value) {
        ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
        astarte_storage_err = ASTARTE_STORAGE_ERR_INTERNAL;
        goto end;
    }

    esp_err = astarte_nvs_key_value_iterator_get_element(
        &iterator->nvs_key_value_iterator, key, &key_len, value, &value_len);
    if (esp_err != ESP_OK) {
        astarte_storage_err = ASTARTE_STORAGE_ERR_INTERNAL;
        goto end;
    }

    // Split interface name and path
    char *interface_name_p = strtok(key, "/");
    size_t interface_name_len = strlen(interface_name_p) + 1; // Including the \0 terminating char
    interface_name = calloc(interface_name_len, sizeof(char));
    if (!interface_name) {
        ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
        astarte_storage_err = ASTARTE_STORAGE_ERR_INTERNAL;
        goto end;
    }
    strcpy(interface_name, interface_name_p);

    char *path_p = strtok(NULL, "\0");

    size_t path_len = strlen(path_p) + 2; // +1 because the '/' char is removed by strtok, +1 for \0
    path = calloc(path_len, sizeof(char));
    if (!path) {
        ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
        astarte_storage_err = ASTARTE_STORAGE_ERR_INTERNAL;
        goto end;
    }
    strncat(strcpy(path, "/"), path_p, strlen(path_p));

    // Free as content has already been stored in interface_name and path
    free(key);
    key = NULL;

    // Copy property information in the outputs
    if (out_interface_name == NULL) {
        *out_interface_name_len = interface_name_len;
    } else if (interface_name_len > *out_interface_name_len) {
        ESP_LOGE(TAG, "Output buffer out_interface_name is of insufficient size.");
        astarte_storage_err = ASTARTE_STORAGE_ERR_DATA_TOO_SMALL;
        goto end;
    } else {
        strcpy(out_interface_name, interface_name);
        *out_interface_name_len = interface_name_len;
    }

    if (out_path == NULL) {
        *out_path_len = path_len;
    } else if (path_len > *out_path_len) {
        ESP_LOGE(TAG, "Output buffer out_path is of insufficient size.");
        astarte_storage_err = ASTARTE_STORAGE_ERR_DATA_TOO_SMALL;
        goto end;
    } else {
        strcpy(out_path, path);
        *out_path_len = path_len;
    }

    if (out_value == NULL) {
        *out_value_len = value_len;
    } else if (path_len > *out_path_len) {
        ESP_LOGE(TAG, "Output buffer out_path is of insufficient size.");
        astarte_storage_err = ASTARTE_STORAGE_ERR_DATA_TOO_SMALL;
        goto end;
    } else {
        memcpy(out_value, value, value_len);
        *out_value_len = value_len;
    }

end:
    // Free all data
    free(key);
    free(value);
    free(interface_name);
    free(path);

    return astarte_storage_err;
}
