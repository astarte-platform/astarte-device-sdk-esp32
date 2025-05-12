/*
 * (C) Copyright 2025, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "unity.h"

#include "data_validation.h"
#include "test_data_validation.h"

#include <esp_log.h>

#define TAG "utest-data-validation"

void test_data_validation_individual_datastream_ok(void)
{
    const astarte_mapping_t mappings[1] = { {
        .endpoint = "/double_endpoint",
        .type = ASTARTE_MAPPING_TYPE_DOUBLE,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = false,
        .allow_unset = false,
    } };

    const astarte_interface_t interface = {
        .name = "org.astarteplatform.esp32.test",
        .major_version = 0,
        .minor_version = 1,
        .type = ASTARTE_INTERFACE_TYPE_DATASTREAM,
        .ownership = ASTARTE_INTERFACE_OWNERSHIP_SERVER,
        .aggregation = ASTARTE_INTERFACE_AGGREGATION_INDIVIDUAL,
        .mappings = mappings,
        .mappings_length = ARRAY_SIZE(mappings),
    };

    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK,
        data_validation_individual_datastream(
            &interface, "/double_endpoint", astarte_data_from_double(11.5), NULL));
}

void test_data_validation_individual_datastream_incorrect_path(void)
{
    const astarte_mapping_t mappings[1] = { {
        .endpoint = "/double_endpoint",
        .type = ASTARTE_MAPPING_TYPE_DOUBLE,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = false,
        .allow_unset = false,
    } };

    const astarte_interface_t interface = {
        .name = "org.astarteplatform.esp32.test",
        .major_version = 0,
        .minor_version = 1,
        .type = ASTARTE_INTERFACE_TYPE_DATASTREAM,
        .ownership = ASTARTE_INTERFACE_OWNERSHIP_SERVER,
        .aggregation = ASTARTE_INTERFACE_AGGREGATION_INDIVIDUAL,
        .mappings = mappings,
        .mappings_length = ARRAY_SIZE(mappings),
    };

    TEST_ASSERT_EQUAL(ASTARTE_RESULT_MAPPING_NOT_IN_INTERFACE,
        data_validation_individual_datastream(
            &interface, "/doubles_endpoint", astarte_data_from_double(11.5), NULL));
}

void test_data_validation_individual_datastream_incorrect_data(void)
{
    const astarte_mapping_t mappings[1] = { {
        .endpoint = "/double_endpoint",
        .type = ASTARTE_MAPPING_TYPE_DOUBLE,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = false,
        .allow_unset = false,
    } };

    const astarte_interface_t interface = {
        .name = "org.astarteplatform.esp32.test",
        .major_version = 0,
        .minor_version = 1,
        .type = ASTARTE_INTERFACE_TYPE_DATASTREAM,
        .ownership = ASTARTE_INTERFACE_OWNERSHIP_SERVER,
        .aggregation = ASTARTE_INTERFACE_AGGREGATION_INDIVIDUAL,
        .mappings = mappings,
        .mappings_length = ARRAY_SIZE(mappings),
    };

    TEST_ASSERT_EQUAL(ASTARTE_RESULT_MAPPING_DATA_INCOMPATIBLE,
        data_validation_individual_datastream(
            &interface, "/double_endpoint", astarte_data_from_boolean(true), NULL));
}

void test_data_validation_individual_datastream_incorrect_timestamp_required(void)
{
    const astarte_mapping_t mappings[1] = { {
        .endpoint = "/double_endpoint",
        .type = ASTARTE_MAPPING_TYPE_DOUBLE,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    } };

    const astarte_interface_t interface = {
        .name = "org.astarteplatform.esp32.test",
        .major_version = 0,
        .minor_version = 1,
        .type = ASTARTE_INTERFACE_TYPE_DATASTREAM,
        .ownership = ASTARTE_INTERFACE_OWNERSHIP_SERVER,
        .aggregation = ASTARTE_INTERFACE_AGGREGATION_INDIVIDUAL,
        .mappings = mappings,
        .mappings_length = ARRAY_SIZE(mappings),
    };

    TEST_ASSERT_EQUAL(ASTARTE_RESULT_MAPPING_EXPLICIT_TIMESTAMP_REQUIRED,
        data_validation_individual_datastream(
            &interface, "/double_endpoint", astarte_data_from_double(11.5), NULL));
}

void test_data_validation_individual_datastream_incorrect_timestamp_not_allowed(void)
{
    const astarte_mapping_t mappings[1] = { {
        .endpoint = "/double_endpoint",
        .type = ASTARTE_MAPPING_TYPE_DOUBLE,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = false,
        .allow_unset = false,
    } };

    const astarte_interface_t interface = {
        .name = "org.astarteplatform.esp32.test",
        .major_version = 0,
        .minor_version = 1,
        .type = ASTARTE_INTERFACE_TYPE_DATASTREAM,
        .ownership = ASTARTE_INTERFACE_OWNERSHIP_SERVER,
        .aggregation = ASTARTE_INTERFACE_AGGREGATION_INDIVIDUAL,
        .mappings = mappings,
        .mappings_length = ARRAY_SIZE(mappings),
    };

    const int64_t timestamp = 1743417057896;
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_MAPPING_EXPLICIT_TIMESTAMP_NOT_SUPPORTED,
        data_validation_individual_datastream(
            &interface, "/double_endpoint", astarte_data_from_double(11.5), &timestamp));
}

void test_data_validation_aggregated_datastream_ok(void)
{
    const astarte_mapping_t mappings[2]
        = { {
                .endpoint = "/root_path/double_endpoint",
                .type = ASTARTE_MAPPING_TYPE_DOUBLE,
                .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
                .explicit_timestamp = false,
                .allow_unset = false,
            },
              {
                  .endpoint = "/root_path/string_endpoint",
                  .type = ASTARTE_MAPPING_TYPE_STRING,
                  .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
                  .explicit_timestamp = false,
                  .allow_unset = false,
              } };

    const astarte_interface_t interface = {
        .name = "org.astarteplatform.esp32.test",
        .major_version = 0,
        .minor_version = 1,
        .type = ASTARTE_INTERFACE_TYPE_DATASTREAM,
        .ownership = ASTARTE_INTERFACE_OWNERSHIP_SERVER,
        .aggregation = ASTARTE_INTERFACE_AGGREGATION_OBJECT,
        .mappings = mappings,
        .mappings_length = ARRAY_SIZE(mappings),
    };

    astarte_object_entry_t entries[] = {
        astarte_object_entry_new("double_endpoint", astarte_data_from_double(54.554)),
        astarte_object_entry_new("string_endpoint", astarte_data_from_string("Test string")),
    };

    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK,
        data_validation_aggregated_datastream(&interface, "/root_path", entries, 2U, NULL));
}

void test_data_validation_aggregated_datastream_incorrect_path(void)
{
    const astarte_mapping_t mappings[2]
        = { {
                .endpoint = "/root_path/double_endpoint",
                .type = ASTARTE_MAPPING_TYPE_DOUBLE,
                .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
                .explicit_timestamp = false,
                .allow_unset = false,
            },
              {
                  .endpoint = "/root_path/string_endpoint",
                  .type = ASTARTE_MAPPING_TYPE_STRING,
                  .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
                  .explicit_timestamp = false,
                  .allow_unset = false,
              } };

    const astarte_interface_t interface = {
        .name = "org.astarteplatform.esp32.test",
        .major_version = 0,
        .minor_version = 1,
        .type = ASTARTE_INTERFACE_TYPE_DATASTREAM,
        .ownership = ASTARTE_INTERFACE_OWNERSHIP_SERVER,
        .aggregation = ASTARTE_INTERFACE_AGGREGATION_OBJECT,
        .mappings = mappings,
        .mappings_length = ARRAY_SIZE(mappings),
    };

    astarte_object_entry_t entries[] = {
        astarte_object_entry_new("double_endpoint", astarte_data_from_double(54.554)),
        astarte_object_entry_new("strings_endpoint", astarte_data_from_string("Test string")),
    };

    TEST_ASSERT_EQUAL(ASTARTE_RESULT_MAPPING_NOT_IN_INTERFACE,
        data_validation_aggregated_datastream(&interface, "/root_path", entries, 2U, NULL));
}

void test_data_validation_aggregated_datastream_incorrect_data(void)
{
    const astarte_mapping_t mappings[2]
        = { {
                .endpoint = "/root_path/double_endpoint",
                .type = ASTARTE_MAPPING_TYPE_DOUBLE,
                .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
                .explicit_timestamp = false,
                .allow_unset = false,
            },
              {
                  .endpoint = "/root_path/string_endpoint",
                  .type = ASTARTE_MAPPING_TYPE_STRING,
                  .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
                  .explicit_timestamp = false,
                  .allow_unset = false,
              } };

    const astarte_interface_t interface = {
        .name = "org.astarteplatform.esp32.test",
        .major_version = 0,
        .minor_version = 1,
        .type = ASTARTE_INTERFACE_TYPE_DATASTREAM,
        .ownership = ASTARTE_INTERFACE_OWNERSHIP_SERVER,
        .aggregation = ASTARTE_INTERFACE_AGGREGATION_OBJECT,
        .mappings = mappings,
        .mappings_length = ARRAY_SIZE(mappings),
    };

    astarte_object_entry_t entries[] = {
        astarte_object_entry_new("double_endpoint", astarte_data_from_boolean(false)),
        astarte_object_entry_new("string_endpoint", astarte_data_from_string("Test string")),
    };

    TEST_ASSERT_EQUAL(ASTARTE_RESULT_MAPPING_DATA_INCOMPATIBLE,
        data_validation_aggregated_datastream(&interface, "/root_path", entries, 2U, NULL));
}

void test_data_validation_aggregated_datastream_incorrect_timestamp_required(void)
{
    const astarte_mapping_t mappings[2]
        = { {
                .endpoint = "/root_path/double_endpoint",
                .type = ASTARTE_MAPPING_TYPE_DOUBLE,
                .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
                .explicit_timestamp = true,
                .allow_unset = false,
            },
              {
                  .endpoint = "/root_path/string_endpoint",
                  .type = ASTARTE_MAPPING_TYPE_STRING,
                  .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
                  .explicit_timestamp = true,
                  .allow_unset = false,
              } };

    const astarte_interface_t interface = {
        .name = "org.astarteplatform.esp32.test",
        .major_version = 0,
        .minor_version = 1,
        .type = ASTARTE_INTERFACE_TYPE_DATASTREAM,
        .ownership = ASTARTE_INTERFACE_OWNERSHIP_SERVER,
        .aggregation = ASTARTE_INTERFACE_AGGREGATION_OBJECT,
        .mappings = mappings,
        .mappings_length = ARRAY_SIZE(mappings),
    };

    astarte_object_entry_t entries[] = {
        astarte_object_entry_new("double_endpoint", astarte_data_from_double(54.554)),
        astarte_object_entry_new("string_endpoint", astarte_data_from_string("Test string")),
    };

    TEST_ASSERT_EQUAL(ASTARTE_RESULT_MAPPING_EXPLICIT_TIMESTAMP_REQUIRED,
        data_validation_aggregated_datastream(&interface, "/root_path", entries, 2U, NULL));
}

void test_data_validation_aggregated_datastream_timestamp_not_allowed(void)
{
    const astarte_mapping_t mappings[2]
        = { {
                .endpoint = "/root_path/double_endpoint",
                .type = ASTARTE_MAPPING_TYPE_DOUBLE,
                .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
                .explicit_timestamp = false,
                .allow_unset = false,
            },
              {
                  .endpoint = "/root_path/string_endpoint",
                  .type = ASTARTE_MAPPING_TYPE_STRING,
                  .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
                  .explicit_timestamp = false,
                  .allow_unset = false,
              } };

    const astarte_interface_t interface = {
        .name = "org.astarteplatform.esp32.test",
        .major_version = 0,
        .minor_version = 1,
        .type = ASTARTE_INTERFACE_TYPE_DATASTREAM,
        .ownership = ASTARTE_INTERFACE_OWNERSHIP_SERVER,
        .aggregation = ASTARTE_INTERFACE_AGGREGATION_OBJECT,
        .mappings = mappings,
        .mappings_length = ARRAY_SIZE(mappings),
    };

    astarte_object_entry_t entries[] = {
        astarte_object_entry_new("double_endpoint", astarte_data_from_double(54.554)),
        astarte_object_entry_new("string_endpoint", astarte_data_from_string("Test string")),
    };

    const int64_t timestamp = 1743417057896;
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_MAPPING_EXPLICIT_TIMESTAMP_NOT_SUPPORTED,
        data_validation_aggregated_datastream(&interface, "/root_path", entries, 2U, &timestamp));
}

void test_data_validation_unset_properties_ok(void)
{
    const astarte_mapping_t mappings[1] = { {
        .endpoint = "/double_endpoint",
        .type = ASTARTE_MAPPING_TYPE_DOUBLE,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = false,
        .allow_unset = true,
    } };

    const astarte_interface_t interface = {
        .name = "org.astarteplatform.esp32.test",
        .major_version = 0,
        .minor_version = 1,
        .type = ASTARTE_INTERFACE_TYPE_PROPERTIES,
        .ownership = ASTARTE_INTERFACE_OWNERSHIP_SERVER,
        .aggregation = ASTARTE_INTERFACE_AGGREGATION_INDIVIDUAL,
        .mappings = mappings,
        .mappings_length = ARRAY_SIZE(mappings),
    };

    TEST_ASSERT_EQUAL(
        ASTARTE_RESULT_OK, data_validation_unset_property(&interface, "/double_endpoint"));
}

void test_data_validation_unset_properties_incorrect_path(void)
{
    const astarte_mapping_t mappings[1] = { {
        .endpoint = "/double_endpoint",
        .type = ASTARTE_MAPPING_TYPE_DOUBLE,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = false,
        .allow_unset = true,
    } };

    const astarte_interface_t interface = {
        .name = "org.astarteplatform.esp32.test",
        .major_version = 0,
        .minor_version = 1,
        .type = ASTARTE_INTERFACE_TYPE_PROPERTIES,
        .ownership = ASTARTE_INTERFACE_OWNERSHIP_SERVER,
        .aggregation = ASTARTE_INTERFACE_AGGREGATION_INDIVIDUAL,
        .mappings = mappings,
        .mappings_length = ARRAY_SIZE(mappings),
    };

    TEST_ASSERT_EQUAL(ASTARTE_RESULT_MAPPING_NOT_IN_INTERFACE,
        data_validation_unset_property(&interface, "/doubles_endpoint"));
}

void test_data_validation_unset_properties_not_allowed(void)
{
    const astarte_mapping_t mappings[1] = { {
        .endpoint = "/double_endpoint",
        .type = ASTARTE_MAPPING_TYPE_DOUBLE,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = false,
        .allow_unset = false,
    } };

    const astarte_interface_t interface = {
        .name = "org.astarteplatform.esp32.test",
        .major_version = 0,
        .minor_version = 1,
        .type = ASTARTE_INTERFACE_TYPE_PROPERTIES,
        .ownership = ASTARTE_INTERFACE_OWNERSHIP_SERVER,
        .aggregation = ASTARTE_INTERFACE_AGGREGATION_INDIVIDUAL,
        .mappings = mappings,
        .mappings_length = ARRAY_SIZE(mappings),
    };

    TEST_ASSERT_EQUAL(ASTARTE_RESULT_MAPPING_UNSET_NOT_ALLOWED,
        data_validation_unset_property(&interface, "/double_endpoint"));
}
