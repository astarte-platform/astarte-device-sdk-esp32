/*
 * (C) Copyright 2024-2025, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "individual_send.h"

#include <esp_log.h>

#include "astarte_device_sdk/data.h"
#include "astarte_device_sdk/result.h"
#include "generated_interfaces.h"
#include "utils.h"

/************************************************
 * Constants, static variables and defines
 ***********************************************/

#define TAG "Astarte sample"

/************************************************
 * Global functions definition
 ***********************************************/

void sample_individual_transmission(astarte_device_handle_t device)
{
    ESP_LOGI(TAG, "Sending some individuals using the Astarte device."); // NOLINT
    const char *interface_name = org_astarteplatform_samples_DeviceDatastream.name;

    astarte_data_t individuals[] = { astarte_data_from_binaryblob((void *) utils_binary_blob_data,
                                         ARRAY_SIZE(utils_binary_blob_data)),
        astarte_data_from_binaryblob_array((const void **) utils_binary_blobs_data,
            (size_t *) utils_binary_blobs_sizes_data, ARRAY_SIZE(utils_binary_blobs_data)),
        astarte_data_from_boolean(utils_boolean_data),
        astarte_data_from_boolean_array(
            (bool *) utils_boolean_array_data, ARRAY_SIZE(utils_boolean_array_data)),
        astarte_data_from_datetime(utils_unix_time_data),
        astarte_data_from_datetime_array(
            (int64_t *) utils_unix_time_array_data, ARRAY_SIZE(utils_unix_time_array_data)),
        astarte_data_from_double(utils_double_data),
        astarte_data_from_double_array(
            (double *) utils_double_array_data, ARRAY_SIZE(utils_double_array_data)),
        astarte_data_from_integer(utils_integer_data),
        astarte_data_from_integer_array(
            (int32_t *) utils_integer_array_data, ARRAY_SIZE(utils_integer_array_data)),
        astarte_data_from_longinteger(utils_longinteger_data),
        astarte_data_from_longinteger_array(
            (int64_t *) utils_longinteger_array_data, ARRAY_SIZE(utils_longinteger_array_data)),
        astarte_data_from_string(utils_string_data),
        astarte_data_from_string_array(
            (const char **) utils_string_array_data, ARRAY_SIZE(utils_string_array_data)) };

    const char *paths[] = {
        "/binaryblob_endpoint",
        "/binaryblobarray_endpoint",
        "/boolean_endpoint",
        "/booleanarray_endpoint",
        "/datetime_endpoint",
        "/datetimearray_endpoint",
        "/double_endpoint",
        "/doublearray_endpoint",
        "/integer_endpoint",
        "/integerarray_endpoint",
        "/longinteger_endpoint",
        "/longintegerarray_endpoint",
        "/string_endpoint",
        "/stringarray_endpoint",
    };

    const int64_t tms = 1714748755;

    for (size_t i = 0; i < ARRAY_SIZE(individuals); i++) {
        ESP_LOGI(TAG, "Stream on %s:", paths[i]);
        utils_log_astarte_data(individuals[i]);
        astarte_result_t res = astarte_device_send_individual(
            device, interface_name, paths[i], individuals[i], &tms);
        if (res != ASTARTE_RESULT_OK) {
            ESP_LOGI(TAG, "Astarte device transmission failure.");
        }
    }

    ESP_LOGI(TAG, "Individual transmission completed.");
}
