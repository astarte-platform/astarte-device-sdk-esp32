/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "astarte_device_sdk/object.h"
#include "object_private.h"

#include <stdlib.h>
#include <esp_log.h>

#include "bson_types.h"
#include "individual_private.h"
#include "interface_private.h"

#define TAG "ASTARTE_OBJECT"

/************************************************
 *     Global public functions definitions      *
 ***********************************************/

astarte_object_entry_t astarte_object_entry_new(const char *path, astarte_individual_t individual)
{
    return (astarte_object_entry_t) {
        .path = path,
        .individual = individual,
    };
}

astarte_result_t astarte_object_entry_to_path_and_individual(
    astarte_object_entry_t object_entry, const char **path, astarte_individual_t *individual)
{
    if (!path || !individual) {
        ESP_LOGE(TAG, "Conversion from Astarte object entry to path and individual error.");
        return ASTARTE_RESULT_INVALID_PARAM;
    }
    *path = object_entry.path;
    *individual = object_entry.individual;
    return ASTARTE_RESULT_OK;
}

/************************************************
 *     Global private functions definitions     *
 ***********************************************/

astarte_result_t astarte_object_entries_serialize(
    new_ast_bson_serializer_t *bson, astarte_object_entry_t *entries, size_t entries_length)
{
    astarte_result_t ares = ASTARTE_RESULT_OK;
    for (size_t i = 0; i < entries_length; i++) {
        ares = astarte_individual_serialize(bson, entries[i].path, entries[i].individual);
        if (ares != ASTARTE_RESULT_OK) {
            break;
        }
    }

    return ares;
}

astarte_result_t astarte_object_entries_deserialize(astarte_bson_element_t bson_elem,
    const astarte_interface_t *interface, const char *path, astarte_object_entry_t **entries,
    size_t *entries_length)
{
    astarte_object_entry_t *tmp_entries = NULL;
    size_t deserialize_idx = 0;
    astarte_result_t ares = ASTARTE_RESULT_OK;

    // Step 1: extract the document from the BSON and calculate its length
    if (bson_elem.type != ASTARTE_BSON_TYPE_DOCUMENT) {
        ESP_LOGE(TAG, "Received BSON element that is not a document.");
        ares = ASTARTE_RESULT_BSON_DESERIALIZER_ERROR;
        goto failure;
    }
    astarte_bson_document_t bson_doc = astarte_bson_deserializer_element_to_document(bson_elem);

    size_t bson_doc_length = 0;
    ares = astarte_bson_deserializer_doc_count_elements(bson_doc, &bson_doc_length);
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
    astarte_bson_element_t inner_elem = { 0 };
    ares = astarte_bson_deserializer_first_element(bson_doc, &inner_elem);
    if (ares != ASTARTE_RESULT_OK) {
        goto failure;
    }

    const astarte_mapping_t *mapping = NULL;
    while ((ares != ASTARTE_RESULT_NOT_FOUND) && (deserialize_idx < bson_doc_length)) {
        tmp_entries[deserialize_idx].path = inner_elem.name;
        ares = astarte_interface_get_mapping_from_paths(interface, path, inner_elem.name, &mapping);
        if (ares != ASTARTE_RESULT_OK) {
            goto failure;
        }
        ares = astarte_individual_deserialize(
            inner_elem, mapping->type, &(tmp_entries[deserialize_idx].individual));
        if (ares != ASTARTE_RESULT_OK) {
            goto failure;
        }
        deserialize_idx++;
        ares = astarte_bson_deserializer_next_element(bson_doc, inner_elem, &inner_elem);
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
        astarte_individual_destroy_deserialized(tmp_entries[j].individual);
    }
    free(tmp_entries);

    return ares;
}

void astarte_object_entries_destroy_deserialized(
    astarte_object_entry_t *entries, size_t entries_length)
{
    for (size_t i = 0; i < entries_length; i++) {
        astarte_individual_destroy_deserialized(entries[i].individual);
    }
    free(entries);
}
