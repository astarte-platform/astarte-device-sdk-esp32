/*
 * (C) Copyright 2024-2025, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "utils.h"

#include <esp_log.h>
#include <time.h>

#include <stdlib.h>

/************************************************
 * Constants, static variables and defines
 ***********************************************/

#define TAG "astarte-sample-utils"

// Maximum size for the datetime string
#define DATETIME_MAX_BUF_SIZE 30

// NOLINTBEGIN(readability-magic-numbers, cppcoreguidelines-avoid-magic-numbers)
const uint8_t utils_binary_blob_data[8] = { 0x53, 0x47, 0x56, 0x73, 0x62, 0x47, 0x38, 0x3d };
static const uint8_t binblob_1[8] = { 0x53, 0x47, 0x56, 0x73, 0x62, 0x47, 0x38, 0x3d };
static const uint8_t binblob_2[5] = { 0x64, 0x32, 0x39, 0x79, 0x62 };
const uint8_t *const utils_binary_blobs_data[2] = { binblob_1, binblob_2 };
const size_t utils_binary_blobs_sizes_data[2] = { ARRAY_SIZE(binblob_1), ARRAY_SIZE(binblob_2) };
const bool utils_boolean_data = true;
const bool utils_boolean_array_data[3] = { true, false, true };
const int64_t utils_unix_time_data = 1710940988984;
const int64_t utils_unix_time_array_data[1] = { 1710940988984 };
const double utils_double_data = 15.42;
const double utils_double_array_data[2] = { 1542.25, 88852.6 };
const int32_t utils_integer_data = 42;
const int32_t utils_integer_array_data[3] = { 4525, 0, 11 };
const int64_t utils_longinteger_data = 8589934592;
const int64_t utils_longinteger_array_data[3] = { 8589930067, 42, 8589934592 };
const char utils_string_data[] = "Hello world!";
const char *const utils_string_array_data[2] = { "Hello ", "world!" };
// NOLINTEND(readability-magic-numbers, cppcoreguidelines-avoid-magic-numbers)

/************************************************
 * Static functions declaration
 ***********************************************/

static size_t datetime_to_string(int64_t datetime, char out[const DATETIME_MAX_BUF_SIZE]);

/************************************************
 * Global functions definition
 ***********************************************/

// NOLINTNEXTLINE(hicpp-function-size)
void utils_log_astarte_data(astarte_data_t data)
{
    char tm_str[DATETIME_MAX_BUF_SIZE] = { 0 };

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
            int64_t datetime = false;
            (void) astarte_data_to_datetime(data, &datetime);
            (void) datetime_to_string(datetime, tm_str);
            ESP_LOGI(TAG, "Astarte datetime: %s", tm_str);
            break;
        case ASTARTE_MAPPING_TYPE_DATETIMEARRAY:
            ESP_LOGI(TAG, "Astarte datetimearray:");
            int64_t *datetimes = NULL;
            size_t datetimes_len = 0;
            (void) astarte_data_to_datetime_array(data, &datetimes, &datetimes_len);
            for (size_t i = 0; i < datetimes_len; i++) {
                (void) datetime_to_string(datetimes[i], tm_str);
                ESP_LOGI(TAG, "    %zi: %s", i, tm_str);
            }
            break;
        case ASTARTE_MAPPING_TYPE_DOUBLE:
            double dbl = false;
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
            int32_t integer = false;
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
            int64_t longinteger = false;
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
            const char *string = false;
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

/************************************************
 * Static functions definitions
 ***********************************************/

static size_t datetime_to_string(int64_t datetime, char out[const DATETIME_MAX_BUF_SIZE])
{
    struct tm *tm_obj = gmtime(&datetime);
    return strftime(out, DATETIME_MAX_BUF_SIZE, "%Y-%m-%dT%H:%M:%S%z", tm_obj);
}
