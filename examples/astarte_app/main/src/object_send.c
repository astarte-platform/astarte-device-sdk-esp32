/*
 * (C) Copyright 2024-2025, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "object_send.h"

#include <esp_log.h>

#include "astarte_device_sdk/data.h"
#include "astarte_device_sdk/result.h"
#include "generated_interfaces.h"
#include "utils.h"

/************************************************
 * Constants, static variables and defines
 ***********************************************/

#define TAG "astarte-sample-objects"

/************************************************
 * Global functions definition
 ***********************************************/

void sample_object_transmission(astarte_device_handle_t device)
{
    ESP_LOGI(TAG, "Sending an object using the Astarte device."); // NOLINT
    const char *interface_name = org_astarteplatform_samples_DeviceAggregate.name;

    astarte_object_entry_t entries[] = {
        astarte_object_entry_new("binaryblob_endpoint",
            astarte_data_from_binaryblob(
                (void *) utils_binary_blob_data, ARRAY_SIZE(utils_binary_blob_data))),
        astarte_object_entry_new("binaryblobarray_endpoint",
            astarte_data_from_binaryblob_array((const void **) utils_binary_blobs_data,
                (size_t *) utils_binary_blobs_sizes_data, ARRAY_SIZE(utils_binary_blobs_data))),
        astarte_object_entry_new("boolean_endpoint", astarte_data_from_boolean(utils_boolean_data)),
        astarte_object_entry_new("booleanarray_endpoint",
            astarte_data_from_boolean_array(
                (bool *) utils_boolean_array_data, ARRAY_SIZE(utils_boolean_array_data))),
        astarte_object_entry_new(
            "datetime_endpoint", astarte_data_from_datetime(utils_unix_time_data)),
        astarte_object_entry_new("datetimearray_endpoint",
            astarte_data_from_datetime_array(
                (int64_t *) utils_unix_time_array_data, ARRAY_SIZE(utils_unix_time_array_data))),
        astarte_object_entry_new("double_endpoint", astarte_data_from_double(utils_double_data)),
        astarte_object_entry_new("doublearray_endpoint",
            astarte_data_from_double_array(
                (double *) utils_double_array_data, ARRAY_SIZE(utils_double_array_data))),
        astarte_object_entry_new("integer_endpoint", astarte_data_from_integer(utils_integer_data)),
        astarte_object_entry_new("integerarray_endpoint",
            astarte_data_from_integer_array(
                (int32_t *) utils_integer_array_data, ARRAY_SIZE(utils_integer_array_data))),
        astarte_object_entry_new(
            "longinteger_endpoint", astarte_data_from_longinteger(utils_longinteger_data)),
        astarte_object_entry_new("longintegerarray_endpoint",
            astarte_data_from_longinteger_array((int64_t *) utils_longinteger_array_data,
                ARRAY_SIZE(utils_longinteger_array_data))),
        astarte_object_entry_new("string_endpoint", astarte_data_from_string(utils_string_data)),
        astarte_object_entry_new("stringarray_endpoint",
            astarte_data_from_string_array(
                (const char **) utils_string_array_data, ARRAY_SIZE(utils_string_array_data))),
    };

    astarte_result_t res = astarte_device_send_object(
        device, interface_name, "/sensor24", entries, ARRAY_SIZE(entries), NULL);
    if (res != ASTARTE_RESULT_OK) {
        ESP_LOGI(TAG, "Astarte device transmission failure."); // NOLINT
    }

    ESP_LOGI(TAG, "Object transmission completed."); // NOLINT
}
