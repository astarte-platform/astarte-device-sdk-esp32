#include "unity.h"

#include "astarte_bson_serializer.h"
#include "astarte_bson_types.h"
#include "serialized_bsons.h"
#include "test_astarte_bson_serializer.h"

#include <esp_log.h>

#define TAG "BSON SERIALZIER TEST"

void test_astarte_bson_serializer_empty_document(void)
{
    astarte_bson_serializer_handle_t bson = astarte_bson_serializer_new();
    astarte_bson_serializer_append_end_of_document(bson);

    size_t ser_bson_len = 0;
    const void *ser_bson = astarte_bson_serializer_get_document(bson, &ser_bson_len);

    TEST_ASSERT_EQUAL(sizeof(serialized_bson_empty_document), ser_bson_len);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(serialized_bson_empty_document, (const uint8_t *) ser_bson,
        sizeof(serialized_bson_empty_document));

    astarte_bson_serializer_destroy(bson);
}

void test_astarte_bson_serializer_complete_document(void)
{
    astarte_bson_serializer_handle_t bson = astarte_bson_serializer_new();

    astarte_bson_serializer_append_double(bson, "element double", (double) 42.3);
    astarte_bson_serializer_append_string(bson, "element string", "hello world");
    const uint8_t bin[] = { 0x62, 0x69, 0x6e, 0x20, 0x65, 0x6e, 0x63, 0x6f, 0x64, 0x65, 0x64, 0x20,
        0x73, 0x74, 0x72, 0x69, 0x6e, 0x67 };
    astarte_bson_serializer_append_binary(bson, "element binary", bin, sizeof(bin));
    astarte_bson_serializer_append_boolean(bson, "element bool false", false);
    astarte_bson_serializer_append_boolean(bson, "element bool true", true);
    astarte_bson_serializer_append_datetime(bson, "element UTC datetime", 1686304399422);
    astarte_bson_serializer_append_int32(bson, "element int32", 10);
    astarte_bson_serializer_append_int64(bson, "element int64", 17179869184);

    const double arr_d[] = { 10.32, 323.44 };
    astarte_bson_serializer_append_double_array(bson, "element double array", arr_d, 2);
    const char *arr_s[] = { "hello", "world" };
    astarte_bson_serializer_append_string_array(bson, "element string array", arr_s, 2);
    const uint8_t bin_1[] = { 0x61 };
    const uint8_t bin_2[] = { 0x61 };
    const uint8_t bin_3[] = { 0x63, 0x64 };
    const uint8_t *arr_bin[] = { bin_1, bin_2, bin_3 };
    const int arr_sizes[] = { 1, 1, 2 };
    astarte_bson_serializer_append_binary_array(
        bson, "element binary array", (const void *const *) arr_bin, arr_sizes, 3);
    const bool arr_bool[] = { false, true };
    astarte_bson_serializer_append_boolean_array(bson, "element bool array", arr_bool, 2);
    const int64_t arr_dt[] = { 1687252801883 };
    astarte_bson_serializer_append_datetime_array(bson, "element UTC datetime array", arr_dt, 1);
    const int32_t arr_int32[] = { 342, 532, -324, 4323 };
    astarte_bson_serializer_append_int32_array(bson, "element int32 array", arr_int32, 4);
    const int64_t arr_int64[] = { -4294970141, 5149762780, 4294967307, 4294967950 };
    astarte_bson_serializer_append_int64_array(bson, "element int64 array", arr_int64, 4);

    astarte_bson_serializer_append_end_of_document(bson);

    size_t ser_bson_len = 0;
    const void *ser_bson = astarte_bson_serializer_get_document(bson, &ser_bson_len);

    TEST_ASSERT_EQUAL(sizeof(serialized_bson_complete_document), ser_bson_len);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(serialized_bson_complete_document, (const uint8_t *) ser_bson,
        sizeof(serialized_bson_complete_document));

    astarte_bson_serializer_destroy(bson);
}
