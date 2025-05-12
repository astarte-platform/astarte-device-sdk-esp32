/*
 * (C) Copyright 2025, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "unity.h"

#include "astarte_device_sdk/object.h"
#include "object_private.h"
#include "test_object.h"

#include <esp_log.h>

#define TAG "utest-object"

static const char test_data_double_path[] = "double_endpoint";
static const double test_data_double = 32.1;
static const char test_data_integer_path[] = "integer_endpoint";
static const double test_data_integer = 42;
static const char test_data_stringarray_path[] = "stringarray_endpoint";
static const char *test_data_stringarray[] = { "hello, world" };
static const uint8_t test_data_serialized[] = { 0x6b, 0x00, 0x00, 0x00, 0x03, 0x76, 0x00, 0x63,
    0x00, 0x00, 0x00, 0x01, 0x64, 0x6f, 0x75, 0x62, 0x6c, 0x65, 0x5f, 0x65, 0x6e, 0x64, 0x70, 0x6f,
    0x69, 0x6e, 0x74, 0x00, 0xcd, 0xcc, 0xcc, 0xcc, 0xcc, 0x0c, 0x40, 0x40, 0x10, 0x69, 0x6e, 0x74,
    0x65, 0x67, 0x65, 0x72, 0x5f, 0x65, 0x6e, 0x64, 0x70, 0x6f, 0x69, 0x6e, 0x74, 0x00, 0x2a, 0x00,
    0x00, 0x00, 0x04, 0x73, 0x74, 0x72, 0x69, 0x6e, 0x67, 0x61, 0x72, 0x72, 0x61, 0x79, 0x5f, 0x65,
    0x6e, 0x64, 0x70, 0x6f, 0x69, 0x6e, 0x74, 0x00, 0x19, 0x00, 0x00, 0x00, 0x02, 0x30, 0x00, 0x0d,
    0x00, 0x00, 0x00, 0x68, 0x65, 0x6c, 0x6c, 0x6f, 0x2c, 0x20, 0x77, 0x6f, 0x72, 0x6c, 0x64, 0x00,
    0x00, 0x00, 0x00 };

static const uint8_t test_data_serialized_empty[]
    = { 0x0d, 0x00, 0x00, 0x00, 0x03, 0x76, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00 };

void test_object_deserialize_astarte_object_from_aggregate(void)
{
    const astarte_mapping_t mappings[3]
        = { {
                .endpoint = "/%{sensor_id}/double_endpoint",
                .type = ASTARTE_MAPPING_TYPE_DOUBLE,
                .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
                .explicit_timestamp = false,
                .allow_unset = false,
            },
              {
                  .endpoint = "/%{sensor_id}/integer_endpoint",
                  .type = ASTARTE_MAPPING_TYPE_INTEGER,
                  .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
                  .explicit_timestamp = false,
                  .allow_unset = false,
              },
              {
                  .endpoint = "/%{sensor_id}/stringarray_endpoint",
                  .type = ASTARTE_MAPPING_TYPE_STRINGARRAY,
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

    bson_document_t full_document = bson_deserializer_init_doc(test_data_serialized);
    bson_element_t v_elem;
    bson_deserializer_element_lookup(full_document, "v", &v_elem);

    astarte_object_entry_t *entries = NULL;
    size_t entries_length = 0;
    astarte_result_t res
        = object_entries_deserialize(v_elem, &interface, "/sensor33", &entries, &entries_length);
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK, res);
    TEST_ASSERT_EQUAL(3, entries_length); // The bson contains two pairs

    astarte_object_entry_t entry_double = entries[0];
    TEST_ASSERT_EQUAL_STRING(test_data_double_path, entry_double.path);
    astarte_data_t data_double = entry_double.data;
    TEST_ASSERT_EQUAL(ASTARTE_MAPPING_TYPE_DOUBLE, data_double.tag);
    TEST_ASSERT_EQUAL(test_data_double, data_double.data.dbl);

    astarte_object_entry_t entry_integer = entries[1];
    TEST_ASSERT_EQUAL_STRING(test_data_integer_path, entry_integer.path);
    astarte_data_t data_integer = entry_integer.data;
    TEST_ASSERT_EQUAL(ASTARTE_MAPPING_TYPE_INTEGER, data_integer.tag);
    TEST_ASSERT_EQUAL(test_data_integer, data_integer.data.integer);

    astarte_object_entry_t object_string = entries[2];
    TEST_ASSERT_EQUAL_STRING(test_data_stringarray_path, object_string.path);
    astarte_data_t data_string = object_string.data;
    TEST_ASSERT_EQUAL(ASTARTE_MAPPING_TYPE_STRINGARRAY, data_string.tag);
    TEST_ASSERT_EQUAL(ARRAY_SIZE(test_data_stringarray), data_string.data.string_array.len);
    for (size_t i = 0; i < ARRAY_SIZE(test_data_stringarray); i++) {
        TEST_ASSERT_EQUAL_STRING(test_data_stringarray[i], data_string.data.string_array.buf[i]);
    }

    object_entries_destroy_deserialized(entries, entries_length);
}

void test_object_deserialize_astarte_object_from_empty_aggregate(void)
{
    bson_document_t full_document = bson_deserializer_init_doc(test_data_serialized_empty);
    bson_element_t v_elem;
    bson_deserializer_element_lookup(full_document, "v", &v_elem);

    astarte_object_entry_t *entries = NULL;
    size_t entries_length = 0;
    astarte_result_t res
        = object_entries_deserialize(v_elem, NULL, NULL, &entries, &entries_length);
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_BSON_EMPTY_DOCUMENT_ERROR, res);
}
