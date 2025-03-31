/*
 * (C) Copyright 2025, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "unity.h"

#include "astarte_device_sdk/interface.h"
#include "interface_private.h"
#include "test_interface.h"

#include <esp_log.h>

#define TAG "INTERFACE TEST"

void test_interface_get_mapping(void)
{
    astarte_result_t res = ASTARTE_RESULT_OK;
    const astarte_mapping_t *mapping = NULL;

    const astarte_mapping_t mappings[3]
        = { {
                .endpoint = "/binaryblob_endpoint",
                .type = ASTARTE_MAPPING_TYPE_BINARYBLOB,
                .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
                .explicit_timestamp = true,
                .allow_unset = false,
            },
              {
                  .endpoint = "/binaryblobarray_endpoint",
                  .type = ASTARTE_MAPPING_TYPE_BINARYBLOBARRAY,
                  .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
                  .explicit_timestamp = true,
                  .allow_unset = false,
              },
              {
                  .endpoint = "/boolean_endpoint",
                  .type = ASTARTE_MAPPING_TYPE_BOOLEAN,
                  .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
                  .explicit_timestamp = true,
                  .allow_unset = false,
              } };

    const astarte_interface_t interface = {
        .name = "org.astarteplatform.esp32.test",
        .major_version = 0,
        .minor_version = 1,
        .type = ASTARTE_INTERFACE_TYPE_DATASTREAM,
        .ownership = ASTARTE_INTERFACE_OWNERSHIP_DEVICE,
        .aggregation = ASTARTE_INTERFACE_AGGREGATION_INDIVIDUAL,
        .mappings = mappings,
        .mappings_length = ARRAY_SIZE(mappings),
    };

    const char path_first_endpoint[] = "/binaryblob_endpoint";
    mapping = NULL;
    res = astarte_interface_get_mapping_from_path(&interface, path_first_endpoint, &mapping);
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK, res);
    TEST_ASSERT_EQUAL_PTR(&mappings[0], mapping);

    const char path_second_endpoint[] = "/binaryblobarray_endpoint";
    mapping = NULL;
    res = astarte_interface_get_mapping_from_path(&interface, path_second_endpoint, &mapping);
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK, res);
    TEST_ASSERT_EQUAL_PTR(&mappings[1], mapping);

    const char path_third_endpoint[] = "/boolean_endpoint";
    mapping = NULL;
    res = astarte_interface_get_mapping_from_path(&interface, path_third_endpoint, &mapping);
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK, res);
    TEST_ASSERT_EQUAL_PTR(&mappings[2], mapping);

    const char path_missing_endpoint[] = "/missing_endpoint";
    mapping = NULL;
    res = astarte_interface_get_mapping_from_path(&interface, path_missing_endpoint, &mapping);
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_MAPPING_NOT_IN_INTERFACE, res);
}
