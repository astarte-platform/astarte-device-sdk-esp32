/*
 * (C) Copyright 2023-2025, SECO Mind Srl
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later OR Apache-2.0
 */

#include "device_caching.h"

#include <esp_err.h>
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
#include "log.h"

/************************************************
 *        Defines, constants and typedef        *
 ***********************************************/

ASTARTE_LOG_MODULE_REGISTER("Astarte caching");

#define SYNCHRONIZATION_NAMESPACE_NAME "astarte_synch"
#define INTROSPECTION_NAMESPACE_NAME "astarte_intro"
#define PROPERTIES_NAMESPACE_NAME "astarte_props"

#define SYNCHRONIZATION_KEY "synchronization_status"
#define INTROSPECTION_KEY "introspection_string"

/************************************************
 *         Static functions declaration         *
 ***********************************************/

/**
 * @brief Open the underlying NVS partition for the specified namespace.
 *
 * @param[out] handle Device caching instance handle.
 * @param[in] namespace Namespace to open.
 * @return The appropriate return value.
 * @retval ASTARTE_RESULT_INTERNAL_ERROR if NVS opening has failed
 * @retval ASTARTE_RESULT_INVALID_CONFIGURATION when NVS is disabled in kconfig
 * @retval ASTARTE_RESULT_OK if operation has been successful
 */
static astarte_result_t open_namespace(nvs_handle_t *handle, const char *namespace);
/**
 * @brief Close the underlying NVS partition.
 *
 * @param[in] handle Device caching instance to close.
 */
static void close_namespace(nvs_handle_t handle);
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
static astarte_result_t append_property_to_string(introspection_t *introspection,
    char *interface_name, char *path, size_t *str_size, char *str_buff, size_t str_buff_size);

/************************************************
 *         Global functions definitions         *
 ***********************************************/

astarte_result_t device_caching_synchronization_set(bool sync)
{
    ASTARTE_LOG_DBG("Storing synchronization: %s", (sync) ? "synchronized" : "not synchronized");
    astarte_result_t ares = ASTARTE_RESULT_OK;
    nvs_handle_t handle = { 0 };
    ares = open_namespace(&handle, SYNCHRONIZATION_NAMESPACE_NAME);
    if (ares != ASTARTE_RESULT_OK) {
        ASTARTE_LOG_ERR("Failed opening caching %s.", astarte_result_to_name(ares));
        return ares;
    }
    ASTARTE_LOG_DBG("Inserting pair in storage. Key: %s", SYNCHRONIZATION_KEY);
    esp_err_t esp_err = kv_storage_set(handle, SYNCHRONIZATION_KEY, &sync, sizeof(sync));
    if (esp_err != ESP_OK) {
        ASTARTE_LOG_ERR("Error caching synchronization: %s.", esp_err_to_name(esp_err));
        ares = ASTARTE_RESULT_NVS_ERROR;
    }
    close_namespace(handle);
    return ares;
}

astarte_result_t device_caching_synchronization_get(bool *sync)
{
    ASTARTE_LOG_DBG("Loading cached synchronization status.");
    astarte_result_t ares = ASTARTE_RESULT_OK;
    nvs_handle_t handle = { 0 };
    ares = open_namespace(&handle, SYNCHRONIZATION_NAMESPACE_NAME);
    if (ares != ASTARTE_RESULT_OK) {
        ASTARTE_LOG_ERR("Failed opening caching %s.", astarte_result_to_name(ares));
        return ares;
    }
    ASTARTE_LOG_DBG("Searching for pair in storage. Key: '%s'", SYNCHRONIZATION_KEY);
    bool read_sync = false;
    size_t read_sync_size = sizeof(read_sync);
    esp_err_t esp_err = kv_storage_get(handle, SYNCHRONIZATION_KEY, &read_sync, &read_sync_size);
    if (esp_err == ESP_ERR_NVS_NOT_FOUND) {
        ASTARTE_LOG_INF("No previous synchronization with Astarte present.");
        ares = ASTARTE_RESULT_NOT_FOUND;
        goto exit;
    }
    if (esp_err != ESP_OK) {
        ares = ASTARTE_RESULT_NVS_ERROR;
        goto exit;
    }

    if (!read_sync) {
        ASTARTE_LOG_INF("No previous synchronization with Astarte present.");
    }
    *sync = read_sync;

exit:
    close_namespace(handle);
    return ares;
}

astarte_result_t device_caching_introspection_set(const char *intr, size_t intr_size)
{
    ASTARTE_LOG_DBG("Storing introspection in key-value storage: '%s' (%d).", intr, intr_size);
    astarte_result_t ares = ASTARTE_RESULT_OK;
    nvs_handle_t handle = { 0 };
    ares = open_namespace(&handle, INTROSPECTION_NAMESPACE_NAME);
    if (ares != ASTARTE_RESULT_OK) {
        ASTARTE_LOG_ERR("Failed opening caching %s.", astarte_result_to_name(ares));
        return ares;
    }
    ASTARTE_LOG_DBG("Inserting pair in storage. Key: %s", INTROSPECTION_KEY);
    esp_err_t esp_err = kv_storage_set(handle, INTROSPECTION_KEY, intr, intr_size);
    if (esp_err != ESP_OK) {
        ASTARTE_LOG_ERR("Error setting introspection: %s.", esp_err_to_name(esp_err));
        ares = ASTARTE_RESULT_NVS_ERROR;
    }
    close_namespace(handle);
    return ares;
}

astarte_result_t device_caching_introspection_check(const char *intr, size_t intr_size)
{
    ASTARTE_LOG_DBG("Checking stored introspection against new one: '%s' (%d).", intr, intr_size);

    astarte_result_t ares = ASTARTE_RESULT_OK;
    char *read_intr = NULL;
    size_t read_intr_size = 0;

    nvs_handle_t handle = { 0 };
    ares = open_namespace(&handle, INTROSPECTION_NAMESPACE_NAME);
    if (ares != ASTARTE_RESULT_OK) {
        ASTARTE_LOG_ERR("Failed opening caching %s.", astarte_result_to_name(ares));
        return ares;
    }

    ASTARTE_LOG_DBG("Searching for pair in storage. Key: '%s'", INTROSPECTION_KEY);
    esp_err_t esp_err = kv_storage_get(handle, INTROSPECTION_KEY, NULL, &read_intr_size);
    if (esp_err == ESP_ERR_NVS_NOT_FOUND) {
        ASTARTE_LOG_INF("No previous device introspection present.");
        ares = ASTARTE_RESULT_DEVICE_CACHING_OUTDATED_INTROSPECTION;
        goto exit;
    }
    if (esp_err != ESP_OK) {
        ASTARTE_LOG_ERR("Error caching previous introspection size: %s.", esp_err_to_name(esp_err));
        ares = ASTARTE_RESULT_NVS_ERROR;
        goto exit;
    }

    if (read_intr_size != intr_size) {
        ares = ASTARTE_RESULT_DEVICE_CACHING_OUTDATED_INTROSPECTION;
        goto exit;
    }

    read_intr = calloc(read_intr_size, sizeof(char));
    if (!read_intr) {
        ASTARTE_LOG_ERR("Out of memory %s: %d", __FILE__, __LINE__);
        ares = ASTARTE_RESULT_OUT_OF_MEMORY;
        goto exit;
    }

    ASTARTE_LOG_DBG("Searching for pair in storage. Key: '%s'", INTROSPECTION_KEY);
    esp_err = kv_storage_get(handle, INTROSPECTION_KEY, read_intr, &read_intr_size);
    if (esp_err != ESP_OK) {
        ASTARTE_LOG_ERR("Error caching previous introspection: %s.", esp_err_to_name(esp_err));
        ares = ASTARTE_RESULT_NVS_ERROR;
        goto exit;
    }

    if (memcmp(intr, read_intr, MIN(read_intr_size, intr_size)) != 0) {
        ASTARTE_LOG_INF("Found outdated introspection: '%s' (%d).", read_intr, read_intr_size);
        ares = ASTARTE_RESULT_DEVICE_CACHING_OUTDATED_INTROSPECTION;
        goto exit;
    }

exit:
    close_namespace(handle);
    free(read_intr);
    return ares;
}

astarte_result_t device_caching_property_store(
    const char *interface_name, const char *path, uint32_t major, astarte_data_t data)
{
    ASTARTE_LOG_DBG("Caching property ('%s' - '%s').", interface_name, path);

    astarte_result_t ares = ASTARTE_RESULT_OK;
    char *key = NULL;
    bson_serializer_t bson = { 0 };

    nvs_handle_t handle = { 0 };
    ares = open_namespace(&handle, PROPERTIES_NAMESPACE_NAME);
    if (ares != ASTARTE_RESULT_OK) {
        ASTARTE_LOG_ERR("Failed opening caching %s.", astarte_result_to_name(ares));
        return ares;
    }

    // Get the full key interface_name + ';' + path
    size_t key_len = strlen(interface_name) + 1 + strlen(path) + 1;
    key = calloc(key_len, sizeof(char));
    if (!key) {
        ASTARTE_LOG_ERR("Out of memory %s: %d", __FILE__, __LINE__);
        ares = ASTARTE_RESULT_OUT_OF_MEMORY;
        goto exit;
    }
    int snprintf_rc = snprintf(key, key_len, "%s;%s", interface_name, path);
    if (snprintf_rc != key_len - 1) {
        ASTARTE_LOG_ERR("Could not create the property key-value storage key.");
        ares = ASTARTE_RESULT_INTERNAL_ERROR;
        goto exit;
    }

    // Serialize the Astarte data
    ares = bson_serializer_init(&bson);
    if (ares != ASTARTE_RESULT_OK) {
        ASTARTE_LOG_ERR("Could not initialize the bson serializer");
        goto exit;
    }
    bson_serializer_append_int32(&bson, "major", *(int32_t *) &major);
    bson_serializer_append_int64(&bson, "type", (int64_t) data.tag);
    ares = data_serialize(&bson, "data", data);
    if (ares != ASTARTE_RESULT_OK) {
        goto exit;
    }
    bson_serializer_append_end_of_document(&bson);

    int data_ser_len = 0;
    void *data_ser = (void *) bson_serializer_get_serialized(bson, &data_ser_len);
    if (!data_ser) {
        ASTARTE_LOG_ERR("Error during BSON serialization.");
        ares = ASTARTE_RESULT_BSON_SERIALIZER_ERROR;
        goto exit;
    }
    if (data_ser_len < 0) {
        ASTARTE_LOG_ERR("BSON document is too long to be cached.");
        ares = ASTARTE_RESULT_BSON_SERIALIZER_ERROR;
        goto exit;
    }

    ASTARTE_LOG_DBG("Inserting pair in storage. Key: %s", key);
    esp_err_t esp_err = kv_storage_set(handle, key, data_ser, data_ser_len);
    if (esp_err != ESP_OK) {
        ASTARTE_LOG_ERR("Error caching property: %s.", astarte_result_to_name(ares));
        ares = ASTARTE_RESULT_NVS_ERROR;
        goto exit;
    }

    esp_err = nvs_commit(handle);
    if (esp_err != ESP_OK) {
        ares = ASTARTE_RESULT_NVS_ERROR;
    }

exit:
    close_namespace(handle);
    free(key);
    bson_serializer_destroy(&bson);
    return ares;
}

astarte_result_t device_caching_property_load(
    const char *interface_name, const char *path, uint32_t *out_major, astarte_data_t *data)
{
    ASTARTE_LOG_DBG("Loading cached property ('%s' - '%s').", interface_name, path);

    astarte_result_t ares = ASTARTE_RESULT_OK;
    char *key = NULL;
    char *value = NULL;

    nvs_handle_t handle = { 0 };
    ares = open_namespace(&handle, PROPERTIES_NAMESPACE_NAME);
    if (ares != ASTARTE_RESULT_OK) {
        ASTARTE_LOG_ERR("Failed opening caching %s.", astarte_result_to_name(ares));
        return ares;
    }

    // Get the full key interface_name + ';' + path
    size_t key_len = strlen(interface_name) + 1 + strlen(path) + 1;
    key = calloc(key_len, sizeof(char));
    if (!key) {
        ASTARTE_LOG_ERR("Out of memory %s: %d", __FILE__, __LINE__);
        ares = ASTARTE_RESULT_OUT_OF_MEMORY;
        goto exit;
    }
    int snprintf_rc = snprintf(key, key_len, "%s;%s", interface_name, path);
    if (snprintf_rc != key_len - 1) {
        ASTARTE_LOG_ERR("Could not create the property key-value storage key.");
        ares = ASTARTE_RESULT_OUT_OF_MEMORY;
        goto exit;
    }

    ASTARTE_LOG_DBG("Searching for pair in storage. Key: '%s'", key);
    size_t value_len = 0;
    esp_err_t esp_err = kv_storage_get(handle, key, NULL, &value_len);
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
        ASTARTE_LOG_ERR("Out of memory %s: %d", __FILE__, __LINE__);
        ares = ASTARTE_RESULT_OUT_OF_MEMORY;
        goto exit;
    }

    // Get the data from NVS
    ASTARTE_LOG_DBG("Getting pair in storage. Key: '%s'", key);
    esp_err = kv_storage_get(handle, key, value, &value_len);
    if (esp_err != ESP_OK) {
        ares = ASTARTE_RESULT_NVS_ERROR;
        goto exit;
    }

    // Parse property from the BSON
    ares = parse_property_bson(value, out_major, data);
    if (ares != ASTARTE_RESULT_OK) {
        ASTARTE_LOG_ERR("Could not parse data from storage: %s.", astarte_result_to_name(ares));
    }

exit:
    close_namespace(handle);
    free(key);
    free(value);
    return ares;
}

void device_caching_property_destroy_loaded(astarte_data_t data)
{
    data_destroy_deserialized(data);
}

astarte_result_t device_caching_property_get_device_properties_string(
    introspection_t *introspection, char *output, size_t *output_size)
{
    astarte_result_t ares = ASTARTE_RESULT_OK;
    device_caching_property_iterator_t iter = { 0 };
    size_t string_size = 0U;
    char *interface_name = NULL;
    char *path = NULL;

    ares = device_caching_property_iterator_init(&iter);
    if ((ares != ASTARTE_RESULT_OK) && (ares != ASTARTE_RESULT_NOT_FOUND)) {
        ASTARTE_LOG_ERR("Properties iterator init failed: %s", astarte_result_to_name(ares));
        return ares;
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
            ASTARTE_LOG_ERR("Properties iterator get error: %s", astarte_result_to_name(ares));
            goto error;
        }

        interface_name = calloc(interface_name_size, sizeof(char));
        path = calloc(path_size, sizeof(char));
        if (!interface_name || !path) {
            ASTARTE_LOG_ERR("Out of memory %s: %d", __FILE__, __LINE__);
            goto error;
        }

        ares = device_caching_property_iterator_get(
            &iter, interface_name, &interface_name_size, path, &path_size);
        if (ares != ASTARTE_RESULT_OK) {
            ASTARTE_LOG_ERR("Properties iterator get error: %s", astarte_result_to_name(ares));
            goto error;
        }

        ares = append_property_to_string(
            introspection, interface_name, path, &string_size, output, *output_size);
        if ((ares != ASTARTE_RESULT_OK) && (ares != ASTARTE_RESULT_NOT_FOUND)) {
            if (ares != ASTARTE_RESULT_OK) {
                ASTARTE_LOG_ERR(
                    "Failed adding property to string: %s", astarte_result_to_name(ares));
            }
            goto error;
        }

        free(interface_name);
        interface_name = NULL;
        free(path);
        path = NULL;

        ares = device_caching_property_iterator_next(&iter);
        if ((ares != ASTARTE_RESULT_OK) && (ares != ASTARTE_RESULT_NOT_FOUND)) {
            ASTARTE_LOG_ERR("Iterator next error: %s", astarte_result_to_name(ares));
            goto error;
        }
    }

    *output_size = string_size;
    return ASTARTE_RESULT_OK;

error:
    device_caching_property_iterator_terminate(&iter);
    free(interface_name);
    free(path);
    return ares;
}

astarte_result_t device_caching_property_delete(const char *interface_name, const char *path)
{
    ASTARTE_LOG_DBG("Deleting cached property ('%s' - '%s').", interface_name, path);

    astarte_result_t ares = ASTARTE_RESULT_OK;
    char *key = NULL;

    nvs_handle_t handle = { 0 };
    ares = open_namespace(&handle, PROPERTIES_NAMESPACE_NAME);
    if (ares != ASTARTE_RESULT_OK) {
        ASTARTE_LOG_ERR("Failed opening caching %s.", astarte_result_to_name(ares));
        return ares;
    }

    // Get the full key interface_name + ';' + path
    size_t key_len = strlen(interface_name) + 1 + strlen(path) + 1;
    key = calloc(key_len, sizeof(char));
    if (!key) {
        ASTARTE_LOG_ERR("Out of memory %s: %d", __FILE__, __LINE__);
        ares = ASTARTE_RESULT_OUT_OF_MEMORY;
        goto exit;
    }
    int snprintf_rc = snprintf(key, key_len, "%s;%s", interface_name, path);
    if (snprintf_rc != key_len - 1) {
        ASTARTE_LOG_ERR("Could not create the property key-value storage key.");
        ares = ASTARTE_RESULT_OUT_OF_MEMORY;
        goto exit;
    }

    // Erase the property value using the full key
    ASTARTE_LOG_DBG("Deleting pair from storage. Key: %s", key);
    esp_err_t esp_err = kv_storage_erase_entry(handle, key);
    if (esp_err == ESP_ERR_NVS_NOT_FOUND) {
        ares = ASTARTE_RESULT_NOT_FOUND;
        goto exit;
    }
    if (esp_err != ESP_OK) {
        ares = ASTARTE_RESULT_NVS_ERROR;
        goto exit;
    }

    // Commit the changes to NVS
    esp_err = nvs_commit(handle);
    if (esp_err != ESP_OK) {
        ares = ASTARTE_RESULT_NVS_ERROR;
        goto exit;
    }

exit:
    close_namespace(handle);
    free(key);
    return ares;
}

astarte_result_t device_caching_property_iterator_init(device_caching_property_iterator_t *iterator)
{
    astarte_result_t ares = ASTARTE_RESULT_OK;
    nvs_handle_t handle = { 0 };
    ares = open_namespace(&handle, PROPERTIES_NAMESPACE_NAME);
    if (ares != ASTARTE_RESULT_OK) {
        ASTARTE_LOG_ERR("Failed opening caching %s.", astarte_result_to_name(ares));
        return ares;
    }

    esp_err_t esp_err
        = kv_storage_iterator_init(handle, NVS_TYPE_BLOB, &iterator->nvs_key_value_iterator);
    if (esp_err == ESP_ERR_NVS_NOT_FOUND) {
        ares = ASTARTE_RESULT_NOT_FOUND;
        goto error;
    }
    if (esp_err != ESP_OK) {
        ares = ASTARTE_RESULT_NVS_ERROR;
        goto error;
    }
    return ares;

error:
    close_namespace(handle);
    return ares;
}

void device_caching_property_iterator_terminate(device_caching_property_iterator_t *iterator)
{
    if (iterator->nvs_key_value_iterator.handle) {
        close_namespace(iterator->nvs_key_value_iterator.handle);
    }
}

astarte_result_t device_caching_property_iterator_next(device_caching_property_iterator_t *iterator)
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

astarte_result_t device_caching_property_iterator_get(device_caching_property_iterator_t *iterator,
    char *out_interface_name, size_t *out_interface_name_size, void *out_path,
    size_t *out_path_size)
{
    astarte_result_t ares = ASTARTE_RESULT_OK;
    char *key = NULL;

    // Check input parameters
    if (!out_interface_name_size || !out_path_size) {
        ASTARTE_LOG_ERR("Interface name and path sizes can't be NULL.");
        ares = ASTARTE_RESULT_INVALID_PARAM;
        goto exit;
    }
    if ((!out_interface_name && out_path) || (out_interface_name && !out_path)) {
        ASTARTE_LOG_ERR("Parameters interface_name and path can only be NULL at the same time.");
        ares = ASTARTE_RESULT_INVALID_PARAM;
        goto exit;
    }

    // Get size of item key
    size_t key_size = 0U;
    size_t value_size = 0U;
    ASTARTE_LOG_DBG("Getting the key size for the pair pointer by the storage iterator.");
    esp_err_t esp_err = kv_storage_iterator_get_element(
        &iterator->nvs_key_value_iterator, NULL, &key_size, NULL, &value_size);
    if (esp_err != ESP_OK) {
        ares = ASTARTE_RESULT_NVS_ERROR;
        goto exit;
    }

    // Allocate required space
    key = calloc(key_size, sizeof(char));
    if (!key) {
        ASTARTE_LOG_ERR("Out of memory %s: %d", __FILE__, __LINE__);
        ares = ASTARTE_RESULT_OUT_OF_MEMORY;
        goto exit;
    }

    // Get the key
    value_size = 0U;
    ASTARTE_LOG_DBG("Getting the key data for the pair pointer by the storage iterator.");
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
        ASTARTE_LOG_ERR("Insufficient buff size in for device caching iterator property get.");
        ares = ASTARTE_RESULT_INVALID_PARAM;
        goto exit;
    }

    *out_interface_name_size = read_interface_name_size;
    *out_path_size = read_path_size;

    int snprintf_rc
        = snprintf(out_interface_name, *out_interface_name_size, "%s", read_interface_name);
    if (snprintf_rc != read_interface_name_size - 1) {
        ASTARTE_LOG_ERR("Could not create the property interface name.");
        ares = ASTARTE_RESULT_INTERNAL_ERROR;
        goto exit;
    }

    snprintf_rc = snprintf(out_path, *out_path_size, "%s", read_path);
    if (snprintf_rc != read_path_size - 1) {
        ASTARTE_LOG_ERR("Could not create the property path.");
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

static astarte_result_t open_namespace(nvs_handle_t *handle, const char *namespace)
{
#if defined(CONFIG_ASTARTE_DEVICE_SDK_NVS)
    esp_err_t esp_err = nvs_open_from_partition(
        CONFIG_ASTARTE_DEVICE_SDK_NVS_PARTITION_LABEL, namespace, NVS_READWRITE, handle);
    if (esp_err != ESP_OK) {
        ASTARTE_LOG_ERR("Error opening NVS partition: %s.", esp_err_to_name(esp_err));
        return ASTARTE_RESULT_NVS_ERROR;
    }
    return ASTARTE_RESULT_OK;
#else
    ASTARTE_LOG_ERR("Attempting to open an NVS namespace when NVS is disabled");
    return ASTARTE_RESULT_INVALID_CONFIGURATION;
#endif
}

static void close_namespace(nvs_handle_t handle)
{
    nvs_close(handle);
}

static astarte_result_t parse_property_bson(
    const char *value, uint32_t *out_major, astarte_data_t *data)
{
    astarte_result_t ares = ASTARTE_RESULT_OK;
    bson_document_t full_document = bson_deserializer_init_doc(value);
    if (out_major) {
        bson_element_t major_elem = { 0 };
        ares = bson_deserializer_element_lookup(full_document, "major", &major_elem);
        if (ares != ASTARTE_RESULT_OK) {
            ASTARTE_LOG_ERR("Cannot parse BSON element for major version.");
            return ares;
        }
        int32_t major = bson_deserializer_element_to_int32(major_elem);
        *out_major = *(uint32_t *) &major;
    }
    if (data) {
        bson_element_t type_elem = { 0 };
        ares = bson_deserializer_element_lookup(full_document, "type", &type_elem);
        if (ares != ASTARTE_RESULT_OK) {
            ASTARTE_LOG_ERR("Cannot parse BSON element for type.");
            return ares;
        }
        astarte_mapping_type_t type
            = (astarte_mapping_type_t) bson_deserializer_element_to_int64(type_elem);

        bson_element_t data_elem = { 0 };
        ares = bson_deserializer_element_lookup(full_document, "data", &data_elem);
        if (ares != ASTARTE_RESULT_OK) {
            ASTARTE_LOG_ERR("Cannot parse BSON element for data.");
            return ares;
        }
        ares = data_deserialize(data_elem, type, data);
        if (ares != ASTARTE_RESULT_OK) {
            ASTARTE_LOG_ERR("Failed in deserializing BSON file.");
            return ares;
        }
    }
    return ares;
}

static astarte_result_t append_property_to_string(introspection_t *introspection,
    char *interface_name, char *path, size_t *str_size, char *str_buff, size_t str_buff_size)
{
    astarte_result_t ares = ASTARTE_RESULT_OK;
    // Check if property is device owned
    const astarte_interface_t *interface = introspection_get(introspection, interface_name);
    if (!interface) {
        ASTARTE_LOG_DBG("Purge property from unknown interface: '%s%s'", interface_name, path);
        ares = device_caching_property_delete(interface_name, path);
        if ((ares != ASTARTE_RESULT_OK) && (ares != ASTARTE_RESULT_NOT_FOUND)) {
            if (ares != ASTARTE_RESULT_OK) {
                ASTARTE_LOG_ERR(
                    "Failed deleting the cached property: %s", astarte_result_to_name(ares));
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
        ASTARTE_LOG_ERR("Insufficient size to extend the string.");
        return ASTARTE_RESULT_INVALID_PARAM;
    }
    // Points to the NULL terminator char in the string
    char *str_buff_end = str_buff + strlen(str_buff);
    // Available number of chars in the buffer (including the NULL terminator)
    size_t str_buff_avail_size = str_buff_size - strlen(str_buff);
    if (strlen(str_buff) != 0) {
        int snprintf_rc = snprintf(str_buff_end, str_buff_avail_size, ";");
        if (snprintf_rc != strlen(";")) {
            ASTARTE_LOG_ERR("Couldn't append ';' to the property string. Err %d", snprintf_rc);
            ares = ASTARTE_RESULT_INTERNAL_ERROR;
        }
        str_buff_end += snprintf_rc;
        str_buff_avail_size -= snprintf_rc;
    }
    int snprintf_rc = snprintf(str_buff_end, str_buff_avail_size, "%s%s", interface_name, path);
    if (snprintf_rc != strlen(interface_name) + strlen(path)) {
        ASTARTE_LOG_ERR("Couldn't append encoding interface name '%s' and path '%s' to the "
                        "property string. Err %d",
            interface_name, path, snprintf_rc);
        ares = ASTARTE_RESULT_INTERNAL_ERROR;
    }

    return ares;
}
