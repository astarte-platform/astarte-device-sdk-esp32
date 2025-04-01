/*
 * (C) Copyright 2023-2025, SECO Mind Srl
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later OR Apache-2.0
 */

#include "device_caching.h"

#include <esp_err.h>
#include <esp_log.h>
#include <nvs.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "astarte_device_sdk/astarte.h"
#include "astarte_device_sdk/data.h"
#include "astarte_device_sdk/interface.h"
#include "astarte_device_sdk/mapping.h"
#include "astarte_device_sdk/result.h"
#include "bson_deserializer.h"
#include "bson_serializer.h"
#include "data_private.h"
#include "introspection.h"
#include "kv_storage.h"

/************************************************
 *        Defines, constants and typedef        *
 ***********************************************/

#define TAG "ASTARTE_DEVICE_CACHING"

#define SYNCHRONIZATION_KEY "synchronization_status"
#define INTROSPECTION_KEY "introspection_string"

/************************************************
 *         Static functions declaration         *
 ***********************************************/

/**
 * @brief Parse BSON file used to store a property
 *
 * @param[in] value BSON file
 * @param[out] out_major Pointer to output major version. Might be NULL, in this case the parameter
 * is ignored.
 * @param[out] data Pointer to output Astarte data. May be NULL, in this case
 * the parameter is ignored.
 * @return ASTARTE_RESULT_OK if successful, otherwise an error code.
 */
static astarte_result_t parse_property_bson(
    const char *value, uint32_t *out_major, astarte_data_t *data);
/**
 * @brief Append a property to the end of the string.
 *
 * @note This function will append the property only if it's device owned and if it's present in
 * the introspection.
 *
 * @param[in] introspection Introspection to be used to check ownership for the property.
 * @param[in] interface_name Interface name for the property to append.
 * @param[in] path Path for the property to append.
 * @param[out] str_size Will be set to the size of the extended string.
 * @param[inout] str_buff Buffer containing the string to extend, if NULL it will be ignored.
 * @param[in] str_buff_size Size of the @p str_buff buffer.
 * @return ASTARTE_RESULT_OK if successful, otherwise an error code.
 */
static astarte_result_t append_property_to_string(device_caching_t handle,
    introspection_t *introspection, char *interface_name, char *path, size_t *str_size,
    char *str_buff, size_t str_buff_size);

/************************************************
 *         Global functions definitions         *
 ***********************************************/

astarte_result_t device_caching_open_namespace(device_caching_t *handle, const char *namespace)
{
    esp_err_t esp_err = nvs_open_from_partition(CONFIG_ASTARTE_DEVICE_SDK_NVS_PARTITION_LABEL,
        namespace, NVS_READWRITE, &handle->nvs_handle);
    if (esp_err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS partition: %s.", esp_err_to_name(esp_err));
        return ASTARTE_RESULT_NVS_ERROR;
    }
    return ASTARTE_RESULT_OK;
}

void device_caching_close_namespace(device_caching_t handle)
{
    nvs_close(handle.nvs_handle);
}

astarte_result_t device_caching_synchronization_set(device_caching_t handle, bool sync)
{
    ESP_LOGD(TAG, "Storing synchronization: %s", (sync) ? "synchronized" : "not synchronized");
    ESP_LOGD(TAG, "Inserting pair in storage. Key: %s", SYNCHRONIZATION_KEY);
    esp_err_t esp_err = kv_storage_set(handle.nvs_handle, SYNCHRONIZATION_KEY, &sync, sizeof(sync));
    if (esp_err != ESP_OK) {
        ESP_LOGE(TAG, "Error caching synchronization: %s.", esp_err_to_name(esp_err));
        return ASTARTE_RESULT_NVS_ERROR;
    }
    return ASTARTE_RESULT_OK;
}

astarte_result_t device_caching_synchronization_get(device_caching_t handle, bool *sync)
{
    astarte_result_t ares = ASTARTE_RESULT_OK;

    ESP_LOGD(TAG, "Loading cached synchronization status.");
    ESP_LOGD(TAG, "Searching for pair in storage. Key: '%s'", SYNCHRONIZATION_KEY);
    bool read_sync = false;
    size_t read_sync_size = sizeof(read_sync);
    esp_err_t esp_err
        = kv_storage_get(handle.nvs_handle, SYNCHRONIZATION_KEY, &read_sync, &read_sync_size);
    if (esp_err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "No previous synchronization with Astarte present.");
        ares = ASTARTE_RESULT_NOT_FOUND;
        goto exit;
    }
    if (esp_err != ESP_OK) {
        ares = ASTARTE_RESULT_NVS_ERROR;
        goto exit;
    }

    if (!read_sync) {
        ESP_LOGI(TAG, "No previous synchronization with Astarte present.");
    }
    *sync = read_sync;

exit:
    return ares;
}

astarte_result_t device_caching_introspection_set(
    device_caching_t handle, const char *intr, size_t intr_size)
{
    ESP_LOGD(TAG, "Storing introspection in key-value storage: '%s' (%d).", intr, intr_size);
    ESP_LOGD(TAG, "Inserting pair in storage. Key: %s", INTROSPECTION_KEY);
    esp_err_t esp_err = kv_storage_set(handle.nvs_handle, INTROSPECTION_KEY, intr, intr_size);
    if (esp_err != ESP_OK) {
        ESP_LOGE(TAG, "Error setting introspection: %s.", esp_err_to_name(esp_err));
        return ASTARTE_RESULT_NVS_ERROR;
    }
    return ASTARTE_RESULT_OK;
}

astarte_result_t device_caching_introspection_check(
    device_caching_t handle, const char *intr, size_t intr_size)
{
    astarte_result_t ares = ASTARTE_RESULT_OK;
    char *read_intr = NULL;
    size_t read_intr_size = 0;

    ESP_LOGD(TAG, "Checking stored introspection against new one: '%s' (%d).", intr, intr_size);
    ESP_LOGD(TAG, "Searching for pair in storage. Key: '%s'", INTROSPECTION_KEY);
    esp_err_t esp_err = kv_storage_get(handle.nvs_handle, INTROSPECTION_KEY, NULL, &read_intr_size);
    if (esp_err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "No previous device introspection present.");
        ares = ASTARTE_RESULT_DEVICE_CACHING_OUTDATED_INTROSPECTION;
        goto exit;
    }
    if (esp_err != ESP_OK) {
        ESP_LOGE(TAG, "Error caching previous introspection size: %s.", esp_err_to_name(esp_err));
        ares = ASTARTE_RESULT_NVS_ERROR;
        goto exit;
    }

    if (read_intr_size != intr_size) {
        ares = ASTARTE_RESULT_DEVICE_CACHING_OUTDATED_INTROSPECTION;
        goto exit;
    }

    read_intr = calloc(read_intr_size, sizeof(char));
    if (!read_intr) {
        ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
        ares = ASTARTE_RESULT_OUT_OF_MEMORY;
        goto exit;
    }

    ESP_LOGD(TAG, "Searching for pair in storage. Key: '%s'", INTROSPECTION_KEY);
    esp_err = kv_storage_get(handle.nvs_handle, INTROSPECTION_KEY, read_intr, &read_intr_size);
    if (esp_err != ESP_OK) {
        ESP_LOGE(TAG, "Error caching previous introspection: %s.", esp_err_to_name(esp_err));
        ares = ASTARTE_RESULT_NVS_ERROR;
        goto exit;
    }

    if (memcmp(intr, read_intr, MIN(read_intr_size, intr_size)) != 0) {
        ESP_LOGI(TAG, "Found outdated introspection: '%s' (%d).", read_intr, read_intr_size);
        ares = ASTARTE_RESULT_DEVICE_CACHING_OUTDATED_INTROSPECTION;
        goto exit;
    }

exit:
    free(read_intr);
    return ares;
}

astarte_result_t device_caching_property_store(device_caching_t handle, const char *interface_name,
    const char *path, uint32_t major, astarte_data_t data)
{
    astarte_result_t ares = ASTARTE_RESULT_OK;
    char *key = NULL;
    new_ast_bson_serializer_t bson = { 0 };

    ESP_LOGD(TAG, "Caching property ('%s' - '%s').", interface_name, path);

    // Get the full key interface_name + ';' + path
    size_t key_len = strlen(interface_name) + 1 + strlen(path) + 1;
    key = calloc(key_len, sizeof(char));
    if (!key) {
        ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
        ares = ASTARTE_RESULT_OUT_OF_MEMORY;
        goto exit;
    }
    int snprintf_rc = snprintf(key, key_len, "%s;%s", interface_name, path);
    if (snprintf_rc != key_len - 1) {
        ESP_LOGE(TAG, "Could not create the property key-value storage key.");
        ares = ASTARTE_RESULT_INTERNAL_ERROR;
        goto exit;
    }

    // Serialize the Astarte data
    ares = new_ast_bson_serializer_init(&bson);
    if (ares != ASTARTE_RESULT_OK) {
        ESP_LOGE(TAG, "Could not initialize the bson serializer");
        goto exit;
    }
    new_ast_bson_serializer_append_int32(&bson, "major", *(int32_t *) &major);
    new_ast_bson_serializer_append_int64(&bson, "type", (int64_t) data.tag);
    ares = astarte_data_serialize(&bson, "data", data);
    if (ares != ASTARTE_RESULT_OK) {
        goto exit;
    }
    new_ast_bson_serializer_append_end_of_document(&bson);

    int data_ser_len = 0;
    void *data_ser = (void *) new_ast_bson_serializer_get_serialized(bson, &data_ser_len);
    if (!data_ser) {
        ESP_LOGE(TAG, "Error during BSON serialization.");
        ares = ASTARTE_RESULT_BSON_SERIALIZER_ERROR;
        goto exit;
    }
    if (data_ser_len < 0) {
        ESP_LOGE(TAG, "BSON document is too long to be cached.");
        ares = ASTARTE_RESULT_BSON_SERIALIZER_ERROR;
        goto exit;
    }

    ESP_LOGD(TAG, "Inserting pair in storage. Key: %s", key);
    esp_err_t esp_err = kv_storage_set(handle.nvs_handle, key, data_ser, data_ser_len);
    if (esp_err != ESP_OK) {
        ESP_LOGE(TAG, "Error caching property: %s.", astarte_result_to_name(ares));
        ares = ASTARTE_RESULT_NVS_ERROR;
        goto exit;
    }

    esp_err = nvs_commit(handle.nvs_handle);
    if (esp_err != ESP_OK) {
        ares = ASTARTE_RESULT_NVS_ERROR;
    }

exit:
    free(key);
    new_ast_bson_serializer_destroy(&bson);
    return ares;
}

astarte_result_t device_caching_property_load(device_caching_t handle, const char *interface_name,
    const char *path, uint32_t *out_major, astarte_data_t *data)
{
    astarte_result_t ares = ASTARTE_RESULT_OK;
    char *key = NULL;
    char *value = NULL;

    ESP_LOGD(TAG, "Loading cached property ('%s' - '%s').", interface_name, path);

    // Get the full key interface_name + ';' + path
    size_t key_len = strlen(interface_name) + 1 + strlen(path) + 1;
    key = calloc(key_len, sizeof(char));
    if (!key) {
        ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
        ares = ASTARTE_RESULT_OUT_OF_MEMORY;
        goto exit;
    }
    int snprintf_rc = snprintf(key, key_len, "%s;%s", interface_name, path);
    if (snprintf_rc != key_len - 1) {
        ESP_LOGE(TAG, "Could not create the property key-value storage key.");
        ares = ASTARTE_RESULT_OUT_OF_MEMORY;
        goto exit;
    }

    ESP_LOGD(TAG, "Searching for pair in storage. Key: '%s'", key);
    size_t value_len = 0;
    esp_err_t esp_err = kv_storage_get(handle.nvs_handle, key, NULL, &value_len);
    if (esp_err == ESP_ERR_NVS_NOT_FOUND) {
        ares = ASTARTE_RESULT_NOT_FOUND;
        goto exit;
    }
    if (esp_err != ESP_OK) {
        ares = ASTARTE_RESULT_NVS_ERROR;
        goto exit;
    }

    // Allocate memory for BSON file to read
    value = calloc(value_len, sizeof(char));
    if (!value) {
        ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
        ares = ASTARTE_RESULT_OUT_OF_MEMORY;
        goto exit;
    }

    // Get the data from NVS
    ESP_LOGD(TAG, "Getting pair in storage. Key: '%s'", key);
    esp_err = kv_storage_get(handle.nvs_handle, key, value, &value_len);
    if (esp_err != ESP_OK) {
        ares = ASTARTE_RESULT_NVS_ERROR;
        goto exit;
    }

    // Parse property from the BSON
    ares = parse_property_bson(value, out_major, data);
    if (ares != ASTARTE_RESULT_OK) {
        ESP_LOGE(TAG, "Could not parse data from storage: %s.", astarte_result_to_name(ares));
    }

exit:
    free(key);
    free(value);
    return ares;
}

void device_caching_property_destroy_loaded(astarte_data_t data)
{
    astarte_data_destroy_deserialized(data);
}

astarte_result_t device_caching_property_get_device_properties_string(
    device_caching_t handle, introspection_t *introspection, char *output, size_t *output_size)
{
    astarte_result_t ares = ASTARTE_RESULT_OK;
    device_caching_iterator_t iter = { 0 };
    size_t string_size = 0U;
    char *interface_name = NULL;
    char *path = NULL;

    ares = device_caching_property_iterator_init(handle, &iter);
    if ((ares != ASTARTE_RESULT_OK) && (ares != ASTARTE_RESULT_NOT_FOUND)) {
        ESP_LOGE(TAG, "Properties iterator init failed: %s", astarte_result_to_name(ares));
        goto error;
    }

    if (output) {
        *output = '\0';
    }

    while (ares != ASTARTE_RESULT_NOT_FOUND) {
        size_t interface_name_size = 0U;
        size_t path_size = 0U;
        ares = device_caching_property_iterator_get(
            &iter, NULL, &interface_name_size, NULL, &path_size);
        if (ares != ASTARTE_RESULT_OK) {
            ESP_LOGE(TAG, "Properties iterator get error: %s", astarte_result_to_name(ares));
            goto error;
        }

        interface_name = calloc(interface_name_size, sizeof(char));
        path = calloc(path_size, sizeof(char));
        if (!interface_name || !path) {
            ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
            goto error;
        }

        ares = device_caching_property_iterator_get(
            &iter, interface_name, &interface_name_size, path, &path_size);
        if (ares != ASTARTE_RESULT_OK) {
            ESP_LOGE(TAG, "Properties iterator get error: %s", astarte_result_to_name(ares));
            goto error;
        }

        ares = append_property_to_string(
            handle, introspection, interface_name, path, &string_size, output, *output_size);
        if ((ares != ASTARTE_RESULT_OK) && (ares != ASTARTE_RESULT_NOT_FOUND)) {
            if (ares != ASTARTE_RESULT_OK) {
                ESP_LOGE(TAG, "Failed adding property to string: %s", astarte_result_to_name(ares));
            }
            goto error;
        }

        free(interface_name);
        interface_name = NULL;
        free(path);
        path = NULL;

        ares = device_caching_property_iterator_next(&iter);
        if ((ares != ASTARTE_RESULT_OK) && (ares != ASTARTE_RESULT_NOT_FOUND)) {
            ESP_LOGE(TAG, "Iterator next error: %s", astarte_result_to_name(ares));
            goto error;
        }
    }

    *output_size = string_size;
    return ASTARTE_RESULT_OK;

error:
    free(interface_name);
    free(path);
    return ares;
}

astarte_result_t device_caching_property_delete(
    device_caching_t handle, const char *interface_name, const char *path)
{
    astarte_result_t ares = ASTARTE_RESULT_OK;
    char *key = NULL;

    ESP_LOGD(TAG, "Deleting cached property ('%s' - '%s').", interface_name, path);

    // Get the full key interface_name + ';' + path
    size_t key_len = strlen(interface_name) + 1 + strlen(path) + 1;
    key = calloc(key_len, sizeof(char));
    if (!key) {
        ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
        ares = ASTARTE_RESULT_OUT_OF_MEMORY;
        goto exit;
    }
    int snprintf_rc = snprintf(key, key_len, "%s;%s", interface_name, path);
    if (snprintf_rc != key_len - 1) {
        ESP_LOGE(TAG, "Could not create the property key-value storage key.");
        ares = ASTARTE_RESULT_OUT_OF_MEMORY;
        goto exit;
    }

    // Erase the property value using the full key
    ESP_LOGD(TAG, "Deleting pair from storage. Key: %s", key);
    esp_err_t esp_err = kv_storage_erase_entry(handle.nvs_handle, key);
    if (esp_err == ESP_ERR_NVS_NOT_FOUND) {
        ares = ASTARTE_RESULT_NOT_FOUND;
        goto exit;
    }
    if (esp_err != ESP_OK) {
        ares = ASTARTE_RESULT_NVS_ERROR;
        goto exit;
    }

    // Commit the changes to NVS
    esp_err = nvs_commit(handle.nvs_handle);
    if (esp_err != ESP_OK) {
        ares = ASTARTE_RESULT_NVS_ERROR;
        goto exit;
    }

exit:
    ESP_LOGD(TAG, "Destroying the key value storage instance.");
    free(key);
    return ares;
}

astarte_result_t device_caching_property_iterator_init(
    device_caching_t handle, device_caching_iterator_t *iterator)
{
    esp_err_t esp_err = kv_storage_iterator_init(
        handle.nvs_handle, NVS_TYPE_BLOB, &iterator->nvs_key_value_iterator);
    if (esp_err == ESP_ERR_NVS_NOT_FOUND) {
        return ASTARTE_RESULT_NOT_FOUND;
    }
    if (esp_err != ESP_OK) {
        return ASTARTE_RESULT_NVS_ERROR;
    }
    return ASTARTE_RESULT_OK;
}

astarte_result_t device_caching_property_iterator_next(device_caching_iterator_t *iterator)
{
    esp_err_t esp_err = kv_storage_iterator_next(&iterator->nvs_key_value_iterator);
    if (esp_err == ESP_ERR_NVS_NOT_FOUND) {
        return ASTARTE_RESULT_NOT_FOUND;
    }
    if (esp_err != ESP_OK) {
        return ASTARTE_RESULT_NVS_ERROR;
    }
    return ASTARTE_RESULT_OK;
}

astarte_result_t device_caching_property_iterator_get(device_caching_iterator_t *iterator,
    char *out_interface_name, size_t *out_interface_name_size, void *out_path,
    size_t *out_path_size)
{
    astarte_result_t ares = ASTARTE_RESULT_OK;
    char *key = NULL;

    // Check input parameters
    if (!out_interface_name_size || !out_path_size) {
        ESP_LOGE(TAG, "Interface name and path sizes can't be NULL.");
        ares = ASTARTE_RESULT_INVALID_PARAM;
        goto exit;
    }
    if ((!out_interface_name && out_path) || (out_interface_name && !out_path)) {
        ESP_LOGE(TAG, "Parameters interface_name and path can only be NULL at the same time.");
        ares = ASTARTE_RESULT_INVALID_PARAM;
        goto exit;
    }

    // Get size of item key
    size_t key_size = 0U;
    size_t value_size = 0U;
    ESP_LOGD(TAG, "Getting the key size for the pair pointer by the storage iterator.");
    esp_err_t esp_err = kv_storage_iterator_get_element(
        &iterator->nvs_key_value_iterator, NULL, &key_size, NULL, &value_size);
    if (esp_err != ESP_OK) {
        ares = ASTARTE_RESULT_NVS_ERROR;
        goto exit;
    }

    // Allocate required space
    key = calloc(key_size, sizeof(char));
    if (!key) {
        ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
        ares = ASTARTE_RESULT_OUT_OF_MEMORY;
        goto exit;
    }

    // Get the key
    value_size = 0U;
    ESP_LOGD(TAG, "Getting the key data for the pair pointer by the storage iterator.");
    esp_err = kv_storage_iterator_get_element(
        &iterator->nvs_key_value_iterator, key, &key_size, NULL, &value_size);
    if (esp_err != ESP_OK) {
        ares = ASTARTE_RESULT_NVS_ERROR;
        goto exit;
    }

    // Split interface name and path
    char *read_interface_name = strtok(key, ";");
    size_t read_interface_name_size = strlen(read_interface_name) + 1; // Including the NULL term
    char *read_path = strtok(NULL, "\0");
    size_t read_path_size = strlen(read_path) + 1;

    if (!out_interface_name && !out_path) {
        *out_interface_name_size = read_interface_name_size;
        *out_path_size = read_path_size;
        goto exit;
    }

    if ((*out_interface_name_size < read_interface_name_size)
        || (*out_path_size < read_path_size)) {
        ESP_LOGE(TAG, "Insufficient buff size in for device caching iterator property get.");
        ares = ASTARTE_RESULT_INVALID_PARAM;
        goto exit;
    }

    *out_interface_name_size = read_interface_name_size;
    *out_path_size = read_path_size;

    int snprintf_rc
        = snprintf(out_interface_name, *out_interface_name_size, "%s", read_interface_name);
    if (snprintf_rc != read_interface_name_size - 1) {
        ESP_LOGE(TAG, "Could not create the property interface name.");
        ares = ASTARTE_RESULT_INTERNAL_ERROR;
        goto exit;
    }

    snprintf_rc = snprintf(out_path, *out_path_size, "%s", read_path);
    if (snprintf_rc != read_path_size - 1) {
        ESP_LOGE(TAG, "Could not create the property path.");
        ares = ASTARTE_RESULT_INTERNAL_ERROR;
        goto exit;
    }

exit:
    free(key);
    return ares;
}

/************************************************
 *         Static functions definitions         *
 ***********************************************/

static astarte_result_t parse_property_bson(
    const char *value, uint32_t *out_major, astarte_data_t *data)
{
    astarte_result_t ares = ASTARTE_RESULT_OK;
    astarte_bson_document_t full_document = astarte_bson_deserializer_init_doc(value);
    if (out_major) {
        astarte_bson_element_t major_elem = { 0 };
        ares = astarte_bson_deserializer_element_lookup(full_document, "major", &major_elem);
        if (ares != ASTARTE_RESULT_OK) {
            ESP_LOGE(TAG, "Cannot parse BSON element for major version.");
            return ares;
        }
        int32_t major = astarte_bson_deserializer_element_to_int32(major_elem);
        *out_major = *(uint32_t *) &major;
    }
    if (data) {
        astarte_bson_element_t type_elem = { 0 };
        ares = astarte_bson_deserializer_element_lookup(full_document, "type", &type_elem);
        if (ares != ASTARTE_RESULT_OK) {
            ESP_LOGE(TAG, "Cannot parse BSON element for type.");
            return ares;
        }
        astarte_mapping_type_t type
            = (astarte_mapping_type_t) astarte_bson_deserializer_element_to_int64(type_elem);

        astarte_bson_element_t data_elem = { 0 };
        ares = astarte_bson_deserializer_element_lookup(full_document, "data", &data_elem);
        if (ares != ASTARTE_RESULT_OK) {
            ESP_LOGE(TAG, "Cannot parse BSON element for data.");
            return ares;
        }
        ares = astarte_data_deserialize(data_elem, type, data);
        if (ares != ASTARTE_RESULT_OK) {
            ESP_LOGE(TAG, "Failed in deserializing BSON file.");
            return ares;
        }
    }
    return ares;
}

static astarte_result_t append_property_to_string(device_caching_t handle,
    introspection_t *introspection, char *interface_name, char *path, size_t *str_size,
    char *str_buff, size_t str_buff_size)
{
    astarte_result_t ares = ASTARTE_RESULT_OK;
    // Check if property is device owned
    const astarte_interface_t *interface = introspection_get(introspection, interface_name);
    if (!interface) {
        ESP_LOGD(TAG, "Purge property from unknown interface: '%s%s'", interface_name, path);
        ares = device_caching_property_delete(handle, interface_name, path);
        if ((ares != ASTARTE_RESULT_OK) && (ares != ASTARTE_RESULT_NOT_FOUND)) {
            if (ares != ASTARTE_RESULT_OK) {
                ESP_LOGE(
                    TAG, "Failed deleting the cached property: %s", astarte_result_to_name(ares));
            }
        }
        return ASTARTE_RESULT_NOT_FOUND;
    }

    if (interface->ownership != ASTARTE_INTERFACE_OWNERSHIP_DEVICE) {
        return ares;
    }

    // Update the formed string size
    *str_size = *str_size + strlen(interface_name) + strlen(path) + 1;

    if (!str_buff) {
        return ares;
    }

    if (str_buff_size < *str_size) {
        ESP_LOGE(TAG, "Insufficient size to extend the string.");
        return ASTARTE_RESULT_INVALID_PARAM;
    }
    // Points to the NULL terminator char in the string
    char *str_buff_end = str_buff + strlen(str_buff);
    // Available number of chars in the buffer (including the NULL terminator)
    size_t str_buff_avail_size = str_buff_size - strlen(str_buff);
    if (strlen(str_buff) != 0) {
        int snprintf_rc = snprintf(str_buff_end, str_buff_avail_size, ";");
        if (snprintf_rc != strlen(";")) {
            ESP_LOGE(TAG, "Couldn't append ';' to the property string. Err %d", snprintf_rc);
            ares = ASTARTE_RESULT_INTERNAL_ERROR;
        }
        str_buff_end += snprintf_rc;
        str_buff_avail_size -= snprintf_rc;
    }
    int snprintf_rc = snprintf(str_buff_end, str_buff_avail_size, "%s%s", interface_name, path);
    if (snprintf_rc != strlen(interface_name) + strlen(path)) {
        ESP_LOGE(TAG,
            "Couldn't append encoding interface name '%s' and path '%s' to the property string. "
            "Err %d",
            interface_name, path, snprintf_rc);
        ares = ASTARTE_RESULT_INTERNAL_ERROR;
    }

    return ares;
}
