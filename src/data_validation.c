/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "data_validation.h"

#include <esp_log.h>

#include "interface_private.h"
#include "mapping_private.h"

#define TAG "ASTARTE_DATA_VALIDATION"

/************************************************
 *         Global functions definitions         *
 ***********************************************/

astarte_result_t data_validation_individual_datastream(const astarte_interface_t *interface,
    const char *path, astarte_individual_t individual, const int64_t *timestamp)
{
    astarte_result_t ares = ASTARTE_RESULT_OK;

    const astarte_mapping_t *mapping = NULL;
    ares = astarte_interface_get_mapping_from_path(interface, path, &mapping);
    if (ares != ASTARTE_RESULT_OK) {
        ESP_LOGE(TAG, "Can't find mapping in interface %s for path %s.", interface->name, path);
        return ares;
    }

    ares = astarte_mapping_check_individual(mapping, individual);
    if (ares != ASTARTE_RESULT_OK) {
        ESP_LOGE(TAG,
            "Individual validation failed, interface/path (%s/%s).", interface->name, path);
        return ares;
    }

    if (mapping->explicit_timestamp && !timestamp) {
        ESP_LOGE(TAG,
            "Explicit timestamp required for interface %s, path %s.", interface->name, path);
        ares = ASTARTE_RESULT_MAPPING_EXPLICIT_TIMESTAMP_REQUIRED;
        return ares;
    }

    if (!mapping->explicit_timestamp && timestamp) {
        ESP_LOGE(TAG,
            "Explicit timestamp not supported for interface %s, path %s.", interface->name, path);
        ares = ASTARTE_RESULT_MAPPING_EXPLICIT_TIMESTAMP_NOT_SUPPORTED;
        return ares;
    }

    return ASTARTE_RESULT_OK;
}

astarte_result_t data_validation_aggregated_datastream(const astarte_interface_t *interface,
    const char *path, astarte_object_entry_t *entries, size_t entries_len, const int64_t *timestamp)
{
    astarte_result_t ares = ASTARTE_RESULT_OK;

    for (size_t i = 0; i < entries_len; i++) {
        const astarte_mapping_t *mapping = NULL;
        astarte_object_entry_t entry = entries[i];
        ares = astarte_interface_get_mapping_from_paths(interface, path, entry.path, &mapping);
        if (ares != ASTARTE_RESULT_OK) {
            ESP_LOGE(TAG, "Can't find mapping in interface %s for path %s/%s.", interface->name,
                path, entry.path);
            return ares;
        }

        ares = astarte_mapping_check_individual(mapping, entry.individual);
        if (ares != ASTARTE_RESULT_OK) {
            ESP_LOGE(TAG, "Individual validation failed, interface/path (%s/%s/%s).",
                interface->name, path, entry.path);
            return ares;
        }

        if (mapping->explicit_timestamp && !timestamp) {
            ESP_LOGE(TAG,
                "Explicit timestamp required for interface %s, path %s.", interface->name, path);
            ares = ASTARTE_RESULT_MAPPING_EXPLICIT_TIMESTAMP_REQUIRED;
            return ares;
        }

        if (!mapping->explicit_timestamp && timestamp) {
            ESP_LOGE(TAG, "Explicit timestamp not supported for interface %s, path %s.",
                interface->name, path);
            ares = ASTARTE_RESULT_MAPPING_EXPLICIT_TIMESTAMP_NOT_SUPPORTED;
            return ares;
        }
    }

    return ASTARTE_RESULT_OK;
}

astarte_result_t data_validation_set_property(
    const astarte_interface_t *interface, const char *path, astarte_individual_t individual)
{
    return data_validation_individual_datastream(interface, path, individual, NULL);
}

astarte_result_t data_validation_unset_property(
    const astarte_interface_t *interface, const char *path)
{
    astarte_result_t ares = ASTARTE_RESULT_OK;

    const astarte_mapping_t *mapping = NULL;
    ares = astarte_interface_get_mapping_from_path(interface, path, &mapping);
    if (ares != ASTARTE_RESULT_OK) {
        ESP_LOGE(TAG, "Can't find mapping in interface %s for path %s.", interface->name, path);
        return ares;
    }

    if (!mapping->allow_unset) {
        ESP_LOGE(TAG, "Unset is not allowed for interface %s, path %s.", interface->name, path);
        ares = ASTARTE_RESULT_MAPPING_UNSET_NOT_ALLOWED;
        return ares;
    }

    return ASTARTE_RESULT_OK;
}
