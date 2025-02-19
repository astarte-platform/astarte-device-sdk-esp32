/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "astarte_device_sdk/mapping.h"
#include "mapping_private.h"

#include <math.h>
#include <string.h>

#include <esp_log.h>

#define TAG "ASTARTE_MAPPING"

/************************************************
 *         Static functions declaration         *
 ***********************************************/

/**
 * @brief Check a single path segment against an endpoint segment.
 *
 * @param[in] endpoint_start The start of the endpoint segment.
 * @param[in] endpoint_end The end of the endpoint segment. This points to the char after the last
 * valid char of the segment.
 * @param[in] path_start The start of the path segment.
 * @param[in] path_end The end of the path segment. This points to the char after the last valid
 * char of the segment.
 * @return True if the segments match, false otherwise.
 */
static bool check_path_segment(const char *endpoint_start, const char *endpoint_end,
    const char *path_start, const char *path_end);

/************************************************
 *         Global functions definitions         *
 ***********************************************/

astarte_result_t astarte_mapping_array_to_scalar_type(
    astarte_mapping_type_t array_type, astarte_mapping_type_t *scalar_type)
{
    astarte_result_t ares = ASTARTE_RESULT_OK;
    switch (array_type) {
        case ASTARTE_MAPPING_TYPE_BINARYBLOBARRAY:
            *scalar_type = ASTARTE_MAPPING_TYPE_BINARYBLOB;
            break;
        case ASTARTE_MAPPING_TYPE_BOOLEANARRAY:
            *scalar_type = ASTARTE_MAPPING_TYPE_BOOLEAN;
            break;
        case ASTARTE_MAPPING_TYPE_DATETIMEARRAY:
            *scalar_type = ASTARTE_MAPPING_TYPE_DATETIME;
            break;
        case ASTARTE_MAPPING_TYPE_DOUBLEARRAY:
            *scalar_type = ASTARTE_MAPPING_TYPE_DOUBLE;
            break;
        case ASTARTE_MAPPING_TYPE_INTEGERARRAY:
            *scalar_type = ASTARTE_MAPPING_TYPE_INTEGER;
            break;
        case ASTARTE_MAPPING_TYPE_LONGINTEGERARRAY:
            *scalar_type = ASTARTE_MAPPING_TYPE_LONGINTEGER;
            break;
        case ASTARTE_MAPPING_TYPE_STRINGARRAY:
            *scalar_type = ASTARTE_MAPPING_TYPE_STRING;
            break;
        default:
            ESP_LOGE(TAG, "Attempting to conversion array->scalar on non array type.");
            ares = ASTARTE_RESULT_INTERNAL_ERROR;
            break;
    }
    return ares;
}

astarte_result_t astarte_mapping_check_path(astarte_mapping_t mapping, const char *path)
{
    // The endpoint is in the format "/segment1/%{param1}/%{param2}/segment2/segment3/..."
    const char *endpoint_segment_start = mapping.endpoint;
    const char *endpoint_segment_end = NULL;

    const char *path_segment_start = path;
    const char *path_segment_end = NULL;

    // Check minimum path length
    size_t path_len = strlen(path);
    if (path_len < 2) {
        return ASTARTE_RESULT_MAPPING_PATH_MISMATCH;
    }

    // Check that the path does not terminate with a slash
    if (path[path_len - 1] == '/') {
        return ASTARTE_RESULT_MAPPING_PATH_MISMATCH;
    }

    // Check the first "/" matches
    if (*endpoint_segment_start != *path_segment_start) {
        return ASTARTE_RESULT_MAPPING_PATH_MISMATCH;
    }

    // Skip the leading slash
    endpoint_segment_start++;
    path_segment_start++;

    // Iterate over each segment
    while ((*endpoint_segment_start != '\0') && (*path_segment_start != '\0')) {
        endpoint_segment_end = strchr(endpoint_segment_start, '/');
        if (endpoint_segment_end == NULL) {
            endpoint_segment_end = endpoint_segment_start + strlen(endpoint_segment_start);
        }
        path_segment_end = strchr(path_segment_start, '/');
        if (path_segment_end == NULL) {
            path_segment_end = path_segment_start + strlen(path_segment_start);
        }

        if (!check_path_segment(endpoint_segment_start, endpoint_segment_end, path_segment_start,
                path_segment_end)) {
            return ASTARTE_RESULT_MAPPING_PATH_MISMATCH;
        }

        // Move to the start of the next segment
        endpoint_segment_start = endpoint_segment_end;
        if (*endpoint_segment_start == '/') {
            endpoint_segment_start++;
        }
        path_segment_start = path_segment_end;
        if (*path_segment_start == '/') {
            path_segment_start++;
        }
    }

    if ((*endpoint_segment_start != '\0') || (*path_segment_start != '\0')) {
        return ASTARTE_RESULT_MAPPING_PATH_MISMATCH;
    }
    return ASTARTE_RESULT_OK;
}

astarte_result_t astarte_mapping_check_individual(
    const astarte_mapping_t *mapping, astarte_individual_t individual)
{
    if (mapping->type != individual.tag) {
        ESP_LOGE(TAG, "Astarte individual type and mapping type do not match.");
        return ASTARTE_RESULT_MAPPING_INDIVIDUAL_INCOMPATIBLE;
    }

    if ((mapping->type == ASTARTE_MAPPING_TYPE_DOUBLE) && (isfinite(individual.data.dbl) == 0)) {
        ESP_LOGE(TAG, "Astarte individual double is not a number.");
        return ASTARTE_RESULT_MAPPING_INDIVIDUAL_INCOMPATIBLE;
    }

    if (mapping->type == ASTARTE_MAPPING_TYPE_DOUBLEARRAY) {
        for (size_t i = 0; i < individual.data.double_array.len; i++) {
            if (isfinite(individual.data.double_array.buf[i]) == 0) {
                ESP_LOGE(TAG, "Astarte individual double is not a number.");
                return ASTARTE_RESULT_MAPPING_INDIVIDUAL_INCOMPATIBLE;
            }
        }
    }

    return ASTARTE_RESULT_OK;
}

/************************************************
 *         Static functions definitions         *
 ***********************************************/

static bool check_path_segment(const char *endpoint_start, const char *endpoint_end,
    const char *path_start, const char *path_end)
{
    size_t endpoint_segment_len = endpoint_end - endpoint_start;
    size_t path_segment_len = path_end - path_start;
    // The check will differ is the segment is a parameter or not
    if ((strncmp(endpoint_start, "%{", 2) == 0)
        && (endpoint_start[endpoint_segment_len - 1] == '}')) {
        if (path_segment_len == 0) {
            return false;
        }
        for (size_t i = 0; i < path_segment_len; i++) {
            if (path_start[i] == '#' || path_start[i] == '+') {
                return false;
            }
        }
    } else if ((endpoint_segment_len != path_segment_len)
        || (memcmp(endpoint_start, path_start, path_segment_len) != 0)) {
        return false;
    }
    return true;
}
