/*
 * (C) Copyright 2024-2025, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "astarte_device_sdk/object.h"
#include "object_private.h"

#include <esp_log.h>
#include <stdlib.h>

#include "bson_types.h"
#include "data_private.h"
#include "interface_private.h"

#define TAG "ASTARTE_OBJECT"

/************************************************
 *     Global public functions definitions      *
 ***********************************************/

astarte_object_entry_t astarte_object_entry_new(const char *path, astarte_data_t data)
{
    return (astarte_object_entry_t) {
        .path = path,
        .data = data,
    };
}

astarte_result_t astarte_object_entry_to_path_and_data(
    astarte_object_entry_t object_entry, const char **path, astarte_data_t *data)
{
    if (!path || !data) {
        ESP_LOGE(TAG, "Conversion from Astarte object entry to path and data error.");
        return ASTARTE_RESULT_INVALID_PARAM;
    }
    *path = object_entry.path;
    *data = object_entry.data;
    return ASTARTE_RESULT_OK;
}

/************************************************
 *     Global private functions definitions     *
 ***********************************************/

astarte_result_t object_entries_serialize(
    bson_serializer_t *bson, astarte_object_entry_t *entries, size_t entries_length)
{
    astarte_result_t ares = ASTARTE_RESULT_OK;
    for (size_t i = 0; i < entries_length; i++) {
        ares = data_serialize(bson, entries[i].path, entries[i].data);
        if (ares != ASTARTE_RESULT_OK) {
            break;
        }
    }

    return ares;
}

astarte_result_t object_entries_deserialize(bson_element_t bson_elem,
    const astarte_interface_t *interface, const char *path, astarte_object_entry_t **entries,
    size_t *entries_length)
{
    astarte_object_entry_t *tmp_entries = NULL;
    size_t deserialize_idx = 0;
    astarte_result_t ares = ASTARTE_RESULT_OK;

    // Step 1: extract the document from the BSON and calculate its length
    if (bson_elem.type != BSON_TYPE_DOCUMENT) {
        ESP_LOGE(TAG, "Received BSON element that is not a document.");
        ares = ASTARTE_RESULT_BSON_DESERIALIZER_ERROR;
        goto failure;
    }
    bson_document_t bson_doc = bson_deserializer_element_to_document(bson_elem);

    size_t bson_doc_length = 0;
    ares = bson_deserializer_doc_count_elements(bson_doc, &bson_doc_length);
    if (ares != ASTARTE_RESULT_OK) {
        goto failure;
    }
    if (bson_doc_length == 0) {
        ESP_LOGE(TAG, "BSON document can't be empty.");
        ares = ASTARTE_RESULT_BSON_EMPTY_DOCUMENT_ERROR;
        goto failure;
    }

    // Step 2: Allocate sufficient memory for all the astarte object entries
    tmp_entries = calloc(bson_doc_length, sizeof(astarte_object_entry_t));
    if (!tmp_entries) {
        ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
        ares = ASTARTE_RESULT_OUT_OF_MEMORY;
        goto failure;
    }

    // Step 3: Fill the allocated memory
    bson_element_t inner_elem = { 0 };
    ares = bson_deserializer_first_element(bson_doc, &inner_elem);
    if (ares != ASTARTE_RESULT_OK) {
        goto failure;
    }

    const astarte_mapping_t *mapping = NULL;
    while ((ares != ASTARTE_RESULT_NOT_FOUND) && (deserialize_idx < bson_doc_length)) {
        tmp_entries[deserialize_idx].path = inner_elem.name;
        ares = interface_get_mapping_from_paths(interface, path, inner_elem.name, &mapping);
        if (ares != ASTARTE_RESULT_OK) {
            goto failure;
        }
        ares = data_deserialize(inner_elem, mapping->type, &(tmp_entries[deserialize_idx].data));
        if (ares != ASTARTE_RESULT_OK) {
            goto failure;
        }
        deserialize_idx++;
        ares = bson_deserializer_next_element(bson_doc, inner_elem, &inner_elem);
        if ((ares != ASTARTE_RESULT_OK) && (ares != ASTARTE_RESULT_NOT_FOUND)) {
            goto failure;
        }
    }

    // Step 4: fill in the output variables
    *entries = tmp_entries;
    *entries_length = bson_doc_length;

    return ASTARTE_RESULT_OK;

failure:
    for (size_t j = 0; j < deserialize_idx; j++) {
        data_destroy_deserialized(tmp_entries[j].data);
    }
    free(tmp_entries);

    return ares;
}

void object_entries_destroy_deserialized(astarte_object_entry_t *entries, size_t entries_length)
{
    for (size_t i = 0; i < entries_length; i++) {
        data_destroy_deserialized(entries[i].data);
    }
    free(entries);
}
