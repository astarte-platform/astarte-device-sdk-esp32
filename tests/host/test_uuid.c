/**
 * This file is part of Astarte.
 *
 * Copyright 2018-2023 SECO Mind Srl
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later OR Apache-2.0
 *
 **/

#include "unity.h"

#include "test_uuid.h"
#include "uuid.h"

#include <esp_log.h>

#include <string.h>

#define TAG "UUID TEST"

#include "Mockesp_random.h"
#include "Mockmd.h"

void test_uuid_from_string(void)
{
    const char *first_uuid_v4_string = "44b35f73-cfbd-43b4-8fef-ca7baea1375f";
    uuid_t first_uuid_v4;
    int res = uuid_from_string(first_uuid_v4_string, first_uuid_v4);
    TEST_ASSERT_EQUAL(0, res);
    uint8_t expected_first_uuid_v4_byte_array[16u] = { 0x44, 0xb3, 0x5f, 0x73, 0xcf, 0xbd, 0x43,
        0xb4, 0x8f, 0xef, 0xca, 0x7b, 0xae, 0xa1, 0x37, 0x5f };
    for (size_t i = 0; i < 16; i++)
        TEST_ASSERT_EQUAL_HEX8(expected_first_uuid_v4_byte_array[i], first_uuid_v4[i]);

    const char *second_uuid_v4_string = "6f2fd4cb-94a0-41c7-8d27-864c6b13b8c0";
    uuid_t second_uuid_v4;
    res = uuid_from_string(second_uuid_v4_string, second_uuid_v4);
    TEST_ASSERT_EQUAL(0, res);
    uint8_t expected_second_uuid_v4_byte_array[16u] = { 0x6f, 0x2f, 0xd4, 0xcb, 0x94, 0xa0, 0x41,
        0xc7, 0x8d, 0x27, 0x86, 0x4c, 0x6b, 0x13, 0xb8, 0xc0 };
    for (size_t i = 0; i < 16; i++)
        TEST_ASSERT_EQUAL_HEX8(expected_second_uuid_v4_byte_array[i], second_uuid_v4[i]);

    const char *third_uuid_v4_string = "8f65dbbc-5868-4015-8523-891cc0bffa58";
    uuid_t third_uuid_v4;
    res = uuid_from_string(third_uuid_v4_string, third_uuid_v4);
    TEST_ASSERT_EQUAL(0, res);
    uint8_t expected_third_uuid_v4_byte_array[16u] = { 0x8f, 0x65, 0xdb, 0xbc, 0x58, 0x68, 0x40,
        0x15, 0x85, 0x23, 0x89, 0x1c, 0xc0, 0xbf, 0xfa, 0x58 };
    for (size_t i = 0; i < 16; i++)
        TEST_ASSERT_EQUAL_HEX8(expected_third_uuid_v4_byte_array[i], third_uuid_v4[i]);

    const char *first_uuid_v5_string = "0575a569-51eb-575c-afe4-ce7fc03bcdc5";
    uuid_t first_uuid_v5;
    res = uuid_from_string(first_uuid_v5_string, first_uuid_v5);
    TEST_ASSERT_EQUAL(0, res);
    uint8_t expected_first_uuid_v5_byte_array[16u] = { 0x05, 0x75, 0xa5, 0x69, 0x51, 0xeb, 0x57,
        0x5c, 0xaf, 0xe4, 0xce, 0x7f, 0xc0, 0x3b, 0xcd, 0xc5 };
    for (size_t i = 0; i < 16; i++)
        TEST_ASSERT_EQUAL_HEX8(expected_first_uuid_v5_byte_array[i], first_uuid_v5[i]);
}

void test_uuid_to_string(void)
{
    char first_uuid_v4_byte_array[37];
    const uuid_t first_uuid_v4 = { 0x44, 0xb3, 0x5f, 0x73, 0xcf, 0xbd, 0x43, 0xb4, 0x8f, 0xef, 0xca,
        0x7b, 0xae, 0xa1, 0x37, 0x5f };
    astarte_err_t err = uuid_to_string(first_uuid_v4, first_uuid_v4_byte_array);
    TEST_ASSERT_EQUAL(ASTARTE_OK, err);
    const char *expected_first_uuid_v4_string = "44b35f73-cfbd-43b4-8fef-ca7baea1375f";
    TEST_ASSERT_EQUAL_STRING(expected_first_uuid_v4_string, first_uuid_v4_byte_array);

    char second_uuid_v4_byte_array[37];
    const uuid_t second_uuid_v4 = { 0x6f, 0x2f, 0xd4, 0xcb, 0x94, 0xa0, 0x41, 0xc7, 0x8d, 0x27,
        0x86, 0x4c, 0x6b, 0x13, 0xb8, 0xc0 };
    err = uuid_to_string(second_uuid_v4, second_uuid_v4_byte_array);
    TEST_ASSERT_EQUAL(ASTARTE_OK, err);
    const char *expected_second_uuid_v4_string = "6f2fd4cb-94a0-41c7-8d27-864c6b13b8c0";
    TEST_ASSERT_EQUAL_STRING(expected_second_uuid_v4_string, second_uuid_v4_byte_array);

    char first_uuid_v5_byte_array[37];
    const uuid_t first_uuid_v5 = { 0x05, 0x75, 0xa5, 0x69, 0x51, 0xeb, 0x57, 0x5c, 0xaf, 0xe4, 0xce,
        0x7f, 0xc0, 0x3b, 0xcd, 0xc5 };
    err = uuid_to_string(first_uuid_v5, first_uuid_v5_byte_array);
    TEST_ASSERT_EQUAL(ASTARTE_OK, err);
    const char *expected_first_uuid_v5_string = "0575a569-51eb-575c-afe4-ce7fc03bcdc5";
    TEST_ASSERT_EQUAL_STRING(expected_first_uuid_v5_string, first_uuid_v5_byte_array);
}

void test_uuid_generate_v5(void)
{
    uuid_t namespace = { 0x6f, 0x2f, 0xd4, 0xcb, 0x94, 0xa0, 0x41, 0xc7, 0x8d, 0x27, 0x86, 0x4c,
        0x6b, 0x13, 0xb8, 0xc0 };
    const char *unique_data = "some unique data";

    mbedtls_md_init_ExpectAnyArgs();
    mbedtls_md_info_from_type_ExpectAnyArgsAndReturn(NULL);
    mbedtls_md_setup_ExpectAnyArgsAndReturn(0);
    mbedtls_md_starts_ExpectAnyArgsAndReturn(0);
    mbedtls_md_update_ExpectAnyArgsAndReturn(0);
    mbedtls_md_update_ExpectAnyArgsAndReturn(0);
    mbedtls_md_finish_ExpectAnyArgsAndReturn(0);
    uint8_t sha_result[32] = { 0x3e, 0x73, 0xe1, 0xf2, 0x03, 0x63, 0x04, 0xe5, 0x35, 0x96, 0x23,
        0x66, 0x17, 0x0f, 0xa3, 0xe5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    mbedtls_md_finish_ReturnArrayThruPtr_output(sha_result, 32);
    mbedtls_md_free_ExpectAnyArgs();

    uuid_t out;
    uuid_generate_v5(namespace, unique_data, strlen(unique_data), out);

    uuid_t expected_out = { 0x3e, 0x73, 0xe1, 0xf2, 0x03, 0x63, 0x54, 0xe5, 0xb5, 0x96, 0x23, 0x66,
        0x17, 0x0f, 0xa3, 0xe5 };
    for (size_t i = 0; i < 16; i++)
        TEST_ASSERT_EQUAL_HEX8(expected_out[i], out[i]);
}

void test_uuid_generate_v4(void)
{
    esp_fill_random_Expect(NULL, 16);
    esp_fill_random_IgnoreArg_buf();
    uuid_t returned_random = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };
    esp_fill_random_ReturnArrayThruPtr_buf(returned_random, 16);

    uuid_t out;
    uuid_generate_v4(out);

    uuid_t expected_out = { 1, 2, 3, 4, 5, 6, 0x40 + 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected_out, out, 16);

    esp_fill_random_Expect(NULL, 16);
    esp_fill_random_IgnoreArg_buf();
    uuid_t returned_random_empty = { 0 };
    esp_fill_random_ReturnArrayThruPtr_buf(returned_random_empty, 16);

    uuid_generate_v4(out);

    uuid_t expected_out_empty = { 0, 0, 0, 0, 0, 0, 0x40, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected_out_empty, out, 16);
}
