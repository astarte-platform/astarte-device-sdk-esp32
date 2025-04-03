/*
 * (C) Copyright 2025, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "unity.h"

#include "astarte_device_sdk/data.h"
#include "data_private.h"
#include "test_data.h"

#include "bson_deserializer.h"
#include "bson_serializer.h"

#include <string.h>

#include <esp_log.h>

#define TAG "DATA TEST"

static const uint8_t test_data_binaryblob[] = { 0x68, 0x65, 0x6c, 0x6c, 0x6f };
static const uint8_t test_data_serialized_binaryblob[] = { 0x12, 0x00, 0x00, 0x00, 0x05, 0x76, 0x00,
    0x05, 0x00, 0x00, 0x00, 0x00, 0x68, 0x65, 0x6c, 0x6c, 0x6f, 0x00 };

static const uint8_t test_data_binaryblob_array_blob_1[]
    = { 0x41, 0x53, 0x54, 0x41, 0x52, 0x54, 0x45 };
static const uint8_t test_data_binaryblob_array_blob_2[] = { 0x49, 0x53 };
static const uint8_t test_data_binaryblob_array_blob_3[] = { 0x43, 0x4F, 0x4F, 0x4C };
static const void *test_data_binaryblob_array[] = { test_data_binaryblob_array_blob_1,
    test_data_binaryblob_array_blob_2, test_data_binaryblob_array_blob_3 };
static const size_t test_data_binaryblob_sizes[] = {
    sizeof(test_data_binaryblob_array_blob_1) / sizeof(uint8_t),
    sizeof(test_data_binaryblob_array_blob_2) / sizeof(uint8_t),
    sizeof(test_data_binaryblob_array_blob_3) / sizeof(uint8_t),
};
static const uint8_t test_data_serialized_binaryblob_array[] = { 0x32, 0x00, 0x00, 0x00, 0x04, 0x76,
    0x00, 0x2a, 0x00, 0x00, 0x00, 0x05, 0x30, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x41, 0x53, 0x54,
    0x41, 0x52, 0x54, 0x45, 0x05, 0x31, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x49, 0x53, 0x05, 0x32,
    0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x43, 0x4f, 0x4f, 0x4c, 0x00, 0x00 };

static const bool test_data_boolean = true;
static const uint8_t test_data_serialized_boolean[]
    = { 0x09, 0x00, 0x00, 0x00, 0x08, 0x76, 0x00, 0x01, 0x00 };

static const bool test_data_boolean_array[] = { true, false, true, true };
static const uint8_t test_data_serialized_boolean_array[]
    = { 0x1d, 0x00, 0x00, 0x00, 0x04, 0x76, 0x00, 0x15, 0x00, 0x00, 0x00, 0x08, 0x30, 0x00, 0x01,
          0x08, 0x31, 0x00, 0x00, 0x08, 0x32, 0x00, 0x01, 0x08, 0x33, 0x00, 0x01, 0x00, 0x00 };

static const int64_t test_data_datetime = 1669111881000;
static const uint8_t test_data_serialized_datetime[] = { 0x10, 0x00, 0x00, 0x00, 0x09, 0x76, 0x00,
    0x28, 0x1d, 0xd2, 0x9e, 0x84, 0x01, 0x00, 0x00, 0x00 };

static const int64_t test_data_datetime_array[] = { 1669111881000, 1669111881000 };
static const uint8_t test_data_serialized_datetime_array[] = { 0x23, 0x00, 0x00, 0x00, 0x04, 0x76,
    0x00, 0x1b, 0x00, 0x00, 0x00, 0x09, 0x30, 0x00, 0x28, 0x1d, 0xd2, 0x9e, 0x84, 0x01, 0x00, 0x00,
    0x09, 0x31, 0x00, 0x28, 0x1d, 0xd2, 0x9e, 0x84, 0x01, 0x00, 0x00, 0x00, 0x00 };

static const double test_data_double = 432.4324;
static const uint8_t test_data_serialized_double[] = { 0x10, 0x00, 0x00, 0x00, 0x01, 0x76, 0x00,
    0xa5, 0x2c, 0x43, 0x1c, 0xeb, 0x06, 0x7b, 0x40, 0x00 };

static const double test_data_double_array[] = { 21.0, 11.5, 0.0, 44.5 };
static const uint8_t test_data_serialized_double_array[]
    = { 0x39, 0x00, 0x00, 0x00, 0x04, 0x76, 0x00, 0x31, 0x00, 0x00, 0x00, 0x01, 0x30, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x35, 0x40, 0x01, 0x31, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x27, 0x40, 0x01, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
          0x33, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x46, 0x40, 0x00, 0x00 };

static const int32_t test_data_integer = 42;
static const uint8_t test_data_serialized_integer[]
    = { 0x0C, 0x00, 0x00, 0x00, 0x10, 0x76, 0x00, 0x2a, 0x00, 0x00, 0x00, 0x00 };

static const int32_t test_data_integer_array[] = { 42, 10, 128, 9, 256 };
static const uint8_t test_data_serialized_integer_array[] = { 0x30, 0x00, 0x00, 0x00, 0x04, 0x76,
    0x00, 0x28, 0x00, 0x00, 0x00, 0x10, 0x30, 0x00, 0x2a, 0x00, 0x00, 0x00, 0x10, 0x31, 0x00, 0x0a,
    0x00, 0x00, 0x00, 0x10, 0x32, 0x00, 0x80, 0x00, 0x00, 0x00, 0x10, 0x33, 0x00, 0x09, 0x00, 0x00,
    0x00, 0x10, 0x34, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00 };

static const int64_t test_data_longinteger = 3147483647;
static const uint8_t test_data_serialized_longinteger[] = { 0x10, 0x00, 0x00, 0x00, 0x12, 0x76,
    0x00, 0xff, 0xc9, 0x9a, 0xbb, 0x00, 0x00, 0x00, 0x00, 0x00 };

static const int64_t test_data_longinteger_array[] = { 68719476736 };
static const uint8_t test_data_serialized_longinteger_array[]
    = { 0x18, 0x00, 0x00, 0x00, 0x04, 0x76, 0x00, 0x10, 0x00, 0x00, 0x00, 0x12, 0x30, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00 };

static const char *test_data_string = "this is a test string";
static const uint8_t test_data_serialized_string[] = { 0x22, 0x00, 0x00, 0x00, 0x02, 0x76, 0x00,
    0x16, 0x00, 0x00, 0x00, 0x74, 0x68, 0x69, 0x73, 0x20, 0x69, 0x73, 0x20, 0x61, 0x20, 0x74, 0x65,
    0x73, 0x74, 0x20, 0x73, 0x74, 0x72, 0x69, 0x6e, 0x67, 0x00, 0x00 };

static const char *test_data_string_array[] = { "this", "is", "a", "test", "string_array" };
static const uint8_t test_data_serialized_string_array[] = { 0x4c, 0x00, 0x00, 0x00, 0x04, 0x76,
    0x00, 0x44, 0x00, 0x00, 0x00, 0x02, 0x30, 0x00, 0x05, 0x00, 0x00, 0x00, 0x74, 0x68, 0x69, 0x73,
    0x00, 0x02, 0x31, 0x00, 0x03, 0x00, 0x00, 0x00, 0x69, 0x73, 0x00, 0x02, 0x32, 0x00, 0x02, 0x00,
    0x00, 0x00, 0x61, 0x00, 0x02, 0x33, 0x00, 0x05, 0x00, 0x00, 0x00, 0x74, 0x65, 0x73, 0x74, 0x00,
    0x02, 0x34, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x73, 0x74, 0x72, 0x69, 0x6e, 0x67, 0x5f, 0x61, 0x72,
    0x72, 0x61, 0x79, 0x00, 0x00, 0x00 };

static const uint8_t test_data_serialized_empty_array[]
    = { 0x0d, 0x00, 0x00, 0x00, 0x04, 0x76, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00 };

static const uint8_t test_data_serialized_mismatched_array_initial[] = { 0x32, 0x00, 0x00, 0x00,
    0x04, 0x76, 0x00, 0x2a, 0x00, 0x00, 0x00, 0x01, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x35, 0x40, 0x02, 0x31, 0x00, 0x06, 0x00, 0x00, 0x00, 0x68, 0x65, 0x6c, 0x6c, 0x6f, 0x00, 0x02,
    0x32, 0x00, 0x06, 0x00, 0x00, 0x00, 0x77, 0x6f, 0x72, 0x6c, 0x64, 0x00, 0x00, 0x00 };

static const uint8_t test_data_serialized_mismatched_array_final[] = { 0x2e, 0x00, 0x00, 0x00, 0x04,
    0x76, 0x00, 0x26, 0x00, 0x00, 0x00, 0x02, 0x30, 0x00, 0x06, 0x00, 0x00, 0x00, 0x68, 0x65, 0x6c,
    0x6c, 0x6f, 0x00, 0x02, 0x31, 0x00, 0x06, 0x00, 0x00, 0x00, 0x77, 0x6f, 0x72, 0x6c, 0x64, 0x00,
    0x10, 0x32, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x00, 0x00 };

void test_data_serialize_integer(void)
{
    astarte_data_t data = astarte_data_from_integer(test_data_integer);
    new_ast_bson_serializer_t bson = { 0 };
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK, new_ast_bson_serializer_init(&bson));
    astarte_data_serialize(&bson, "v", data);
    new_ast_bson_serializer_append_end_of_document(&bson);
    int len = 0;
    const void *data_ser = new_ast_bson_serializer_get_serialized(bson, &len);
    TEST_ASSERT_EQUAL(sizeof(test_data_serialized_integer), len);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(test_data_serialized_integer, data_ser, len);
}

void test_data_serialize_longinteger(void)
{
    astarte_data_t data = astarte_data_from_longinteger(test_data_longinteger);
    new_ast_bson_serializer_t bson = { 0 };
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK, new_ast_bson_serializer_init(&bson));
    astarte_data_serialize(&bson, "v", data);
    new_ast_bson_serializer_append_end_of_document(&bson);
    int len = 0;
    const void *data_ser = new_ast_bson_serializer_get_serialized(bson, &len);
    TEST_ASSERT_EQUAL(sizeof(test_data_serialized_longinteger), len);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(test_data_serialized_longinteger, data_ser, len);
}

void test_data_serialize_double(void)
{
    astarte_data_t data = astarte_data_from_double(test_data_double);
    new_ast_bson_serializer_t bson = { 0 };
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK, new_ast_bson_serializer_init(&bson));
    astarte_data_serialize(&bson, "v", data);
    new_ast_bson_serializer_append_end_of_document(&bson);
    int len = 0;
    const void *data_ser = new_ast_bson_serializer_get_serialized(bson, &len);
    TEST_ASSERT_EQUAL(sizeof(test_data_serialized_double), len);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(test_data_serialized_double, data_ser, len);
}

void test_data_serialize_boolean(void)
{
    astarte_data_t data = astarte_data_from_boolean(test_data_boolean);
    new_ast_bson_serializer_t bson = { 0 };
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK, new_ast_bson_serializer_init(&bson));
    astarte_data_serialize(&bson, "v", data);
    new_ast_bson_serializer_append_end_of_document(&bson);
    int len = 0;
    const void *data_ser = new_ast_bson_serializer_get_serialized(bson, &len);
    TEST_ASSERT_EQUAL(sizeof(test_data_serialized_boolean), len);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(test_data_serialized_boolean, data_ser, len);
}

void test_data_serialize_string(void)
{
    astarte_data_t data = astarte_data_from_string(test_data_string);
    new_ast_bson_serializer_t bson = { 0 };
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK, new_ast_bson_serializer_init(&bson));
    astarte_data_serialize(&bson, "v", data);
    new_ast_bson_serializer_append_end_of_document(&bson);
    int len = 0;
    const void *data_ser = new_ast_bson_serializer_get_serialized(bson, &len);
    TEST_ASSERT_EQUAL(sizeof(test_data_serialized_string), len);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(test_data_serialized_string, data_ser, len);
}

void test_data_serialize_integer_array(void)
{
    astarte_data_t data = astarte_data_from_integer_array(
        (int32_t *) &(test_data_integer_array), sizeof(test_data_integer_array) / sizeof(int32_t));
    new_ast_bson_serializer_t bson = { 0 };
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK, new_ast_bson_serializer_init(&bson));
    astarte_data_serialize(&bson, "v", data);
    new_ast_bson_serializer_append_end_of_document(&bson);
    int len = 0;
    const void *data_ser = new_ast_bson_serializer_get_serialized(bson, &len);
    TEST_ASSERT_EQUAL(sizeof(test_data_serialized_integer_array), len);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(test_data_serialized_integer_array, data_ser, len);
}

void test_data_serialize_string_array(void)
{
    astarte_data_t data = astarte_data_from_string_array((const char **) &(test_data_string_array),
        sizeof(test_data_string_array) / sizeof(const char *const));
    new_ast_bson_serializer_t bson = { 0 };
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK, new_ast_bson_serializer_init(&bson));
    astarte_data_serialize(&bson, "v", data);
    new_ast_bson_serializer_append_end_of_document(&bson);
    int len = 0;
    const void *data_ser = new_ast_bson_serializer_get_serialized(bson, &len);
    TEST_ASSERT_EQUAL(sizeof(test_data_serialized_string_array), len);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(test_data_serialized_string_array, data_ser, len);
}

void test_data_serialize_binaryblob_array(void)
{
    astarte_data_t data = astarte_data_from_binaryblob_array(test_data_binaryblob_array,
        (size_t *) test_data_binaryblob_sizes,
        sizeof(test_data_binaryblob_array) / sizeof(uint8_t *));

    new_ast_bson_serializer_t bson = { 0 };
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK, new_ast_bson_serializer_init(&bson));
    astarte_data_serialize(&bson, "v", data);
    new_ast_bson_serializer_append_end_of_document(&bson);
    int len = 0;
    const void *data_ser = new_ast_bson_serializer_get_serialized(bson, &len);

    TEST_ASSERT_EQUAL(sizeof(test_data_serialized_binaryblob_array), len);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(test_data_serialized_binaryblob_array, data_ser, len);
}

void test_data_deserialize_astarte_data_from_incorrect_type(void)
{
    new_ast_bson_document_t full_document
        = new_ast_bson_deserializer_init_doc(test_data_serialized_binaryblob);
    new_ast_bson_element_t v_elem;
    new_ast_bson_deserializer_element_lookup(full_document, "v", &v_elem);

    astarte_data_t data = { 0 };
    astarte_result_t res
        = astarte_data_deserialize(v_elem, ASTARTE_MAPPING_TYPE_DATETIMEARRAY, &data);
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_BSON_DESERIALIZER_TYPES_ERROR, res);
}

void test_data_deserialize_astarte_data_from_binblob(void)
{
    new_ast_bson_document_t full_document
        = new_ast_bson_deserializer_init_doc(test_data_serialized_binaryblob);
    new_ast_bson_element_t v_elem;
    new_ast_bson_deserializer_element_lookup(full_document, "v", &v_elem);

    astarte_data_t data = { 0 };
    astarte_result_t res = astarte_data_deserialize(v_elem, ASTARTE_MAPPING_TYPE_BINARYBLOB, &data);
    TEST_ASSERT_EQUAL(res, ASTARTE_RESULT_OK);
    TEST_ASSERT_EQUAL(data.tag, ASTARTE_MAPPING_TYPE_BINARYBLOB);
    TEST_ASSERT_EQUAL(data.data.binaryblob.len, 5);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(
        data.data.binaryblob.buf, test_data_binaryblob, data.data.binaryblob.len);
    astarte_data_destroy_deserialized(data);
}

void test_data_deserialize_astarte_data_from_boolean(void)
{
    new_ast_bson_document_t full_document
        = new_ast_bson_deserializer_init_doc(test_data_serialized_boolean);
    new_ast_bson_element_t v_elem;
    new_ast_bson_deserializer_element_lookup(full_document, "v", &v_elem);

    astarte_data_t data = { 0 };
    astarte_result_t res = astarte_data_deserialize(v_elem, ASTARTE_MAPPING_TYPE_BOOLEAN, &data);
    TEST_ASSERT_EQUAL(res, ASTARTE_RESULT_OK);
    TEST_ASSERT_EQUAL(data.tag, ASTARTE_MAPPING_TYPE_BOOLEAN);
    TEST_ASSERT_EQUAL(data.data.boolean, test_data_boolean);
    astarte_data_destroy_deserialized(data);
}

void test_data_deserialize_astarte_data_from_datetime(void)
{
    new_ast_bson_document_t full_document
        = new_ast_bson_deserializer_init_doc(test_data_serialized_datetime);
    new_ast_bson_element_t v_elem;
    new_ast_bson_deserializer_element_lookup(full_document, "v", &v_elem);

    astarte_data_t data = { 0 };
    astarte_result_t res = astarte_data_deserialize(v_elem, ASTARTE_MAPPING_TYPE_DATETIME, &data);
    TEST_ASSERT_EQUAL(res, ASTARTE_RESULT_OK);
    TEST_ASSERT_EQUAL(data.tag, ASTARTE_MAPPING_TYPE_DATETIME);
    TEST_ASSERT_EQUAL(data.data.datetime, test_data_datetime);
    astarte_data_destroy_deserialized(data);
}

void test_data_deserialize_astarte_data_from_double(void)
{
    new_ast_bson_document_t full_document
        = new_ast_bson_deserializer_init_doc(test_data_serialized_double);
    new_ast_bson_element_t v_elem;
    new_ast_bson_deserializer_element_lookup(full_document, "v", &v_elem);

    astarte_data_t data = { 0 };
    astarte_result_t res = astarte_data_deserialize(v_elem, ASTARTE_MAPPING_TYPE_DOUBLE, &data);
    TEST_ASSERT_EQUAL(res, ASTARTE_RESULT_OK);
    TEST_ASSERT_EQUAL(data.tag, ASTARTE_MAPPING_TYPE_DOUBLE);
    TEST_ASSERT_EQUAL(data.data.dbl, test_data_double);
    astarte_data_destroy_deserialized(data);
}

void test_data_deserialize_astarte_data_from_integer(void)
{
    new_ast_bson_document_t full_document
        = new_ast_bson_deserializer_init_doc(test_data_serialized_integer);
    new_ast_bson_element_t v_elem;
    new_ast_bson_deserializer_element_lookup(full_document, "v", &v_elem);

    astarte_data_t data = { 0 };
    astarte_result_t res = astarte_data_deserialize(v_elem, ASTARTE_MAPPING_TYPE_INTEGER, &data);
    TEST_ASSERT_EQUAL(res, ASTARTE_RESULT_OK);
    TEST_ASSERT_EQUAL(data.tag, ASTARTE_MAPPING_TYPE_INTEGER);
    TEST_ASSERT_EQUAL(data.data.integer, test_data_integer);
    astarte_data_destroy_deserialized(data);
}

void test_data_deserialize_astarte_data_from_longinteger(void)
{
    new_ast_bson_document_t full_document
        = new_ast_bson_deserializer_init_doc(test_data_serialized_longinteger);
    new_ast_bson_element_t v_elem;
    new_ast_bson_deserializer_element_lookup(full_document, "v", &v_elem);

    astarte_data_t data = { 0 };
    astarte_result_t res
        = astarte_data_deserialize(v_elem, ASTARTE_MAPPING_TYPE_LONGINTEGER, &data);
    TEST_ASSERT_EQUAL(res, ASTARTE_RESULT_OK);
    TEST_ASSERT_EQUAL(data.tag, ASTARTE_MAPPING_TYPE_LONGINTEGER);
    TEST_ASSERT_EQUAL(data.data.longinteger, test_data_longinteger);
    astarte_data_destroy_deserialized(data);
}

void test_data_deserialize_astarte_data_from_string(void)
{
    new_ast_bson_document_t full_document
        = new_ast_bson_deserializer_init_doc(test_data_serialized_string);
    new_ast_bson_element_t v_elem;
    new_ast_bson_deserializer_element_lookup(full_document, "v", &v_elem);

    astarte_data_t data = { 0 };
    astarte_result_t res = astarte_data_deserialize(v_elem, ASTARTE_MAPPING_TYPE_STRING, &data);
    TEST_ASSERT_EQUAL(res, ASTARTE_RESULT_OK);
    TEST_ASSERT_EQUAL(data.tag, ASTARTE_MAPPING_TYPE_STRING);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(data.data.string, test_data_string, strlen(test_data_string) + 1);
    astarte_data_destroy_deserialized(data);
}

void test_data_deserialize_astarte_data_from_binblob_array(void)
{
    new_ast_bson_document_t full_document
        = new_ast_bson_deserializer_init_doc(test_data_serialized_binaryblob_array);
    new_ast_bson_element_t v_elem;
    new_ast_bson_deserializer_element_lookup(full_document, "v", &v_elem);

    astarte_data_t data = { 0 };
    astarte_result_t res
        = astarte_data_deserialize(v_elem, ASTARTE_MAPPING_TYPE_BINARYBLOBARRAY, &data);
    TEST_ASSERT_EQUAL(res, ASTARTE_RESULT_OK);
    TEST_ASSERT_EQUAL(data.tag, ASTARTE_MAPPING_TYPE_BINARYBLOBARRAY);
    TEST_ASSERT_EQUAL(data.data.binaryblob_array.count, ARRAY_SIZE(test_data_binaryblob_array));
    for (size_t i = 0; i < ARRAY_SIZE(test_data_binaryblob_array); i++) {
        TEST_ASSERT_EQUAL(data.data.binaryblob_array.sizes[i], test_data_binaryblob_sizes[i]);
        TEST_ASSERT_EQUAL_HEX8_ARRAY(data.data.binaryblob_array.blobs[i],
            test_data_binaryblob_array[i], test_data_binaryblob_sizes[i]);
    }
    astarte_data_destroy_deserialized(data);
}

void test_data_deserialize_astarte_data_from_boolean_array(void)
{
    new_ast_bson_document_t full_document
        = new_ast_bson_deserializer_init_doc(test_data_serialized_boolean_array);
    new_ast_bson_element_t v_elem;
    new_ast_bson_deserializer_element_lookup(full_document, "v", &v_elem);

    astarte_data_t data = { 0 };
    astarte_result_t res
        = astarte_data_deserialize(v_elem, ASTARTE_MAPPING_TYPE_BOOLEANARRAY, &data);
    TEST_ASSERT_EQUAL(res, ASTARTE_RESULT_OK);
    TEST_ASSERT_EQUAL(data.tag, ASTARTE_MAPPING_TYPE_BOOLEANARRAY);
    TEST_ASSERT_EQUAL(data.data.boolean_array.len, ARRAY_SIZE(test_data_boolean_array));
    TEST_ASSERT_EQUAL_HEX8_ARRAY(
        data.data.boolean_array.buf, test_data_boolean_array, ARRAY_SIZE(test_data_boolean_array));
    astarte_data_destroy_deserialized(data);
}

void test_data_deserialize_astarte_data_from_double_array(void)
{
    new_ast_bson_document_t full_document
        = new_ast_bson_deserializer_init_doc(test_data_serialized_double_array);
    new_ast_bson_element_t v_elem;
    new_ast_bson_deserializer_element_lookup(full_document, "v", &v_elem);

    astarte_data_t data = { 0 };
    astarte_result_t res
        = astarte_data_deserialize(v_elem, ASTARTE_MAPPING_TYPE_DOUBLEARRAY, &data);
    TEST_ASSERT_EQUAL(res, ASTARTE_RESULT_OK);
    TEST_ASSERT_EQUAL(res, ASTARTE_RESULT_OK);
    TEST_ASSERT_EQUAL(data.tag, ASTARTE_MAPPING_TYPE_DOUBLEARRAY);
    TEST_ASSERT_EQUAL(data.data.double_array.len, ARRAY_SIZE(test_data_double_array));
    for (size_t i = 0; i < ARRAY_SIZE(test_data_double_array); i++) {
        TEST_ASSERT_EQUAL_FLOAT(data.data.double_array.buf[i], test_data_double_array[i]);
    }
    astarte_data_destroy_deserialized(data);
}

void test_data_deserialize_astarte_data_from_datetime_array(void)
{
    new_ast_bson_document_t full_document
        = new_ast_bson_deserializer_init_doc(test_data_serialized_datetime_array);
    new_ast_bson_element_t v_elem;
    new_ast_bson_deserializer_element_lookup(full_document, "v", &v_elem);

    astarte_data_t data = { 0 };
    astarte_result_t res
        = astarte_data_deserialize(v_elem, ASTARTE_MAPPING_TYPE_DATETIMEARRAY, &data);
    TEST_ASSERT_EQUAL(res, ASTARTE_RESULT_OK);
    TEST_ASSERT_EQUAL(data.tag, ASTARTE_MAPPING_TYPE_DATETIMEARRAY);
    TEST_ASSERT_EQUAL(data.data.datetime_array.len, ARRAY_SIZE(test_data_datetime_array));
    for (size_t i = 0; i < ARRAY_SIZE(test_data_datetime_array); i++) {
        TEST_ASSERT_EQUAL(data.data.datetime_array.buf[i], test_data_datetime_array[i]);
    }
    astarte_data_destroy_deserialized(data);
}

void test_data_deserialize_astarte_data_from_integer_array(void)
{
    new_ast_bson_document_t full_document
        = new_ast_bson_deserializer_init_doc(test_data_serialized_integer_array);
    new_ast_bson_element_t v_elem;
    new_ast_bson_deserializer_element_lookup(full_document, "v", &v_elem);

    astarte_data_t data = { 0 };
    astarte_result_t res
        = astarte_data_deserialize(v_elem, ASTARTE_MAPPING_TYPE_INTEGERARRAY, &data);
    TEST_ASSERT_EQUAL(res, ASTARTE_RESULT_OK);
    TEST_ASSERT_EQUAL(data.tag, ASTARTE_MAPPING_TYPE_INTEGERARRAY);
    TEST_ASSERT_EQUAL(data.data.integer_array.len, ARRAY_SIZE(test_data_integer_array));
    for (size_t i = 0; i < ARRAY_SIZE(test_data_integer_array); i++) {
        TEST_ASSERT_EQUAL(data.data.integer_array.buf[i], test_data_integer_array[i]);
    }
    astarte_data_destroy_deserialized(data);
}

void test_data_deserialize_astarte_data_from_longinteger_array(void)
{
    new_ast_bson_document_t full_document
        = new_ast_bson_deserializer_init_doc(test_data_serialized_longinteger_array);
    new_ast_bson_element_t v_elem;
    new_ast_bson_deserializer_element_lookup(full_document, "v", &v_elem);

    astarte_data_t data = { 0 };
    astarte_result_t res
        = astarte_data_deserialize(v_elem, ASTARTE_MAPPING_TYPE_LONGINTEGERARRAY, &data);
    TEST_ASSERT_EQUAL(res, ASTARTE_RESULT_OK);
    TEST_ASSERT_EQUAL(data.tag, ASTARTE_MAPPING_TYPE_LONGINTEGERARRAY);
    TEST_ASSERT_EQUAL(data.data.longinteger_array.len, ARRAY_SIZE(test_data_longinteger_array));
    for (size_t i = 0; i < ARRAY_SIZE(test_data_longinteger_array); i++) {
        TEST_ASSERT_EQUAL(data.data.longinteger_array.buf[i], test_data_longinteger_array[i]);
    }
    astarte_data_destroy_deserialized(data);
}

void test_data_deserialize_astarte_data_from_string_array(void)
{
    new_ast_bson_document_t full_document
        = new_ast_bson_deserializer_init_doc(test_data_serialized_string_array);
    new_ast_bson_element_t v_elem;
    new_ast_bson_deserializer_element_lookup(full_document, "v", &v_elem);

    astarte_data_t data = { 0 };
    astarte_result_t res
        = astarte_data_deserialize(v_elem, ASTARTE_MAPPING_TYPE_STRINGARRAY, &data);
    TEST_ASSERT_EQUAL(res, ASTARTE_RESULT_OK);
    TEST_ASSERT_EQUAL(data.tag, ASTARTE_MAPPING_TYPE_STRINGARRAY);
    TEST_ASSERT_EQUAL(data.data.string_array.len, ARRAY_SIZE(test_data_string_array));
    for (size_t i = 0; i < ARRAY_SIZE(test_data_string_array); i++) {
        TEST_ASSERT_EQUAL(strcmp(data.data.string_array.buf[i], test_data_string_array[i]), 0);
    }
    astarte_data_destroy_deserialized(data);
}

void test_data_deserialize_astarte_data_from_empty_array(void)
{
    new_ast_bson_document_t full_document
        = new_ast_bson_deserializer_init_doc(test_data_serialized_empty_array);
    new_ast_bson_element_t v_elem;
    new_ast_bson_deserializer_element_lookup(full_document, "v", &v_elem);

    astarte_data_t data = { 0 };
    astarte_result_t res
        = astarte_data_deserialize(v_elem, ASTARTE_MAPPING_TYPE_DOUBLEARRAY, &data);
    TEST_ASSERT_EQUAL(res, ASTARTE_RESULT_OK);
    TEST_ASSERT_EQUAL(data.tag, ASTARTE_MAPPING_TYPE_DOUBLEARRAY);
    TEST_ASSERT_EQUAL(data.data.double_array.len, 0);
}

void test_data_deserialize_astarte_data_from_mismatched_array_initial(void)
{
    new_ast_bson_document_t full_document
        = new_ast_bson_deserializer_init_doc(test_data_serialized_mismatched_array_initial);
    new_ast_bson_element_t v_elem;
    new_ast_bson_deserializer_element_lookup(full_document, "v", &v_elem);

    astarte_data_t data = { 0 };
    astarte_result_t res
        = astarte_data_deserialize(v_elem, ASTARTE_MAPPING_TYPE_STRINGARRAY, &data);
    TEST_ASSERT_EQUAL(res, ASTARTE_RESULT_BSON_DESERIALIZER_TYPES_ERROR);
}

void test_data_deserialize_astarte_data_from_mismatched_array_final(void)
{
    new_ast_bson_document_t full_document
        = new_ast_bson_deserializer_init_doc(test_data_serialized_mismatched_array_final);
    new_ast_bson_element_t v_elem;
    new_ast_bson_deserializer_element_lookup(full_document, "v", &v_elem);

    astarte_data_t data = { 0 };
    astarte_result_t res
        = astarte_data_deserialize(v_elem, ASTARTE_MAPPING_TYPE_STRINGARRAY, &data);
    TEST_ASSERT_EQUAL(res, ASTARTE_RESULT_BSON_DESERIALIZER_TYPES_ERROR);
}
