/*
 * (C) Copyright 2024-2025, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "utils.h"

#include <esp_log.h>
#include <time.h>

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

/************************************************
 * Constants, static variables and defines
 ***********************************************/

#define TAG "astarte-end-to-end-test"

// Maximum size for the datetime string
#define DATETIME_MAX_STR_LEN 30

/************************************************
 * Global functions definition
 ***********************************************/

// NOLINTNEXTLINE(hicpp-function-size)
void utils_log_astarte_data(astarte_data_t data)
{
    struct tm *tm_obj = NULL;
    char tm_str[DATETIME_MAX_STR_LEN] = { 0 };

    switch (astarte_data_get_type(data)) {
        case ASTARTE_MAPPING_TYPE_BINARYBLOB:
            void *blob = NULL;
            size_t blob_len = 0;
            (void) astarte_data_to_binaryblob(data, &blob, &blob_len);
            ESP_LOGI(TAG, "Astarte binaryblob:");
            ESP_LOG_BUFFER_HEX_LEVEL(TAG, blob, blob_len, ESP_LOG_INFO);
            break;
        case ASTARTE_MAPPING_TYPE_BINARYBLOBARRAY:
            ESP_LOGI(TAG, "Astarte binaryblobarray:");
            const void **blobs = NULL;
            size_t *sizes = NULL;
            size_t count = 0;
            (void) astarte_data_to_binaryblob_array(data, &blobs, &sizes, &count);
            for (size_t i = 0; i < count; i++) {
                ESP_LOG_BUFFER_HEX_LEVEL(TAG, blobs[i], sizes[i], ESP_LOG_INFO);
            }
            break;
        case ASTARTE_MAPPING_TYPE_BOOLEAN:
            bool boolean = false;
            (void) astarte_data_to_boolean(data, &boolean);
            ESP_LOGI(TAG, "Astarte boolean: %s", (boolean) ? "true" : "false");
            break;
        case ASTARTE_MAPPING_TYPE_BOOLEANARRAY:
            ESP_LOGI(TAG, "Astarte booleanarray:");
            bool *bools = NULL;
            size_t bools_len = 0;
            (void) astarte_data_to_boolean_array(data, &bools, &bools_len);
            for (size_t i = 0; i < bools_len; i++) {
                ESP_LOGI(TAG, "    %zi: %s", i, (bools[i]) ? "true" : "false");
            }
            break;
        case ASTARTE_MAPPING_TYPE_DATETIME:
            int64_t datetime = 0;
            (void) astarte_data_to_datetime(data, &datetime);
            tm_obj = gmtime(&datetime);
            (void) strftime(tm_str, DATETIME_MAX_STR_LEN, "%Y-%m-%dT%H:%M:%S%z", tm_obj);
            ESP_LOGI(TAG, "Astarte datetime: %s", tm_str);
            break;
        case ASTARTE_MAPPING_TYPE_DATETIMEARRAY:
            ESP_LOGI(TAG, "Astarte datetimearray:");
            int64_t *datetimes = NULL;
            size_t datetimes_len = 0;
            (void) astarte_data_to_datetime_array(data, &datetimes, &datetimes_len);
            for (size_t i = 0; i < datetimes_len; i++) {
                tm_obj = gmtime(&datetimes[i]);
                (void) strftime(tm_str, DATETIME_MAX_STR_LEN, "%Y-%m-%dT%H:%M:%S%z", tm_obj);
                ESP_LOGI(TAG, "    %zi: %s", i, tm_str);
            }
            break;
        case ASTARTE_MAPPING_TYPE_DOUBLE:
            double dbl = 0.0;
            (void) astarte_data_to_double(data, &dbl);
            ESP_LOGI(TAG, "Astarte double: %f", dbl);
            break;
        case ASTARTE_MAPPING_TYPE_DOUBLEARRAY:
            ESP_LOGI(TAG, "Astarte doublearray:");
            double *doubles = NULL;
            size_t doubles_len = 0;
            (void) astarte_data_to_double_array(data, &doubles, &doubles_len);
            for (size_t i = 0; i < doubles_len; i++) {
                ESP_LOGI(TAG, "    %zi: %f", i, doubles[i]);
            }
            break;
        case ASTARTE_MAPPING_TYPE_INTEGER:
            int32_t integer = 0;
            (void) astarte_data_to_integer(data, &integer);
            ESP_LOGI(TAG, "Astarte integer: %" PRIi32, integer);
            break;
        case ASTARTE_MAPPING_TYPE_INTEGERARRAY:
            ESP_LOGI(TAG, "Astarte integerarray:");
            int32_t *integers = NULL;
            size_t integers_len = 0;
            (void) astarte_data_to_integer_array(data, &integers, &integers_len);
            for (size_t i = 0; i < integers_len; i++) {
                ESP_LOGI(TAG, "    %zi: %" PRIi32, i, integers[i]);
            }
            break;
        case ASTARTE_MAPPING_TYPE_LONGINTEGER:
            int64_t longinteger = 0;
            (void) astarte_data_to_longinteger(data, &longinteger);
            ESP_LOGI(TAG, "Astarte longinteger: %lli", longinteger);
            break;
        case ASTARTE_MAPPING_TYPE_LONGINTEGERARRAY:
            ESP_LOGI(TAG, "Astarte longintegerarray:");
            int64_t *longintegers = NULL;
            size_t longintegers_len = 0;
            (void) astarte_data_to_longinteger_array(data, &longintegers, &longintegers_len);
            for (size_t i = 0; i < longintegers_len; i++) {
                ESP_LOGI(TAG, "    %zi: %lli", i, longintegers[i]);
            }
            break;
        case ASTARTE_MAPPING_TYPE_STRING:
            const char *string = NULL;
            (void) astarte_data_to_string(data, &string);
            ESP_LOGI(TAG, "Astarte string: %s", string);
            break;
        case ASTARTE_MAPPING_TYPE_STRINGARRAY:
            ESP_LOGI(TAG, "Astarte stringarray:");
            const char **strings = NULL;
            size_t strings_len = 0;
            (void) astarte_data_to_string_array(data, &strings, &strings_len);
            for (size_t i = 0; i < strings_len; i++) {
                ESP_LOGI(TAG, "    %zi: %s", i, strings[i]);
            }
            break;
        default:
            ESP_LOGE(TAG, "Astarte data has invalid tag!");
            break;
    }
}

void utils_log_astarte_object(astarte_object_entry_t *entries, size_t entries_length)
{
    ESP_LOGI(TAG, "Astarte object:");

    for (size_t i = 0; i < entries_length; i++) {
        const char *mapping_path = NULL;
        astarte_data_t data = { 0 };
        astarte_result_t astarte_rc
            = astarte_object_entry_to_path_and_data(entries[i], &mapping_path, &data);
        if (astarte_rc == ASTARTE_RESULT_OK) {
            ESP_LOGI(TAG, "Mapping path: %s", mapping_path);
            utils_log_astarte_data(data);
        }
    }
}

/**
 * @brief Convert a hex string to a dynamically allocated array of bytes
 * @param hexstr The string to convert. Should always be in the form "01-ab-8f".
 * @param num_bytes The resulting size of the returned array, only valid if the return is non NULL.
 * @return The dynamically allocated and filled bytes array, or NULL upon error.
 */
uint8_t *utils_hexstr_to_bytes(const char *hexstr, size_t *num_bytes)
{
    if (!hexstr) {
        return NULL;
    }

    size_t len = strlen(hexstr);
    if (len < 2) {
        return NULL;
    }

    // Calculate the number of bytes, equal to the number of '-' + 1
    size_t count = 1;
    for (size_t i = 0; i < len; i++) {
        if (hexstr[i] == '-') {
            count++;
        }
    }

    uint8_t *buf = calloc(count, sizeof(uint8_t));
    if (!buf) {
        return NULL;
    }

    size_t j = 0;
    for (size_t i = 0; i < len;) {
        if (!isxdigit((unsigned char) hexstr[i]) || !isxdigit((unsigned char) hexstr[i + 1])) {
            free(buf);
            return NULL;
        }
        const char byte_str[3] = { hexstr[i], hexstr[i + 1], '\0' };
        buf[j++] = (uint8_t) strtol(byte_str, NULL, 16);
        i += 2;
        if (hexstr[i] == '-') {
            i++;
        }
    }

    *num_bytes = j;
    return buf;
}
