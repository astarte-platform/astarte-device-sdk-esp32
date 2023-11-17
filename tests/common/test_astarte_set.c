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

#include "astarte_set.h"
#include "test_astarte_set.h"

#include <string.h>

#include <esp_log.h>

#define TAG "SET TEST"

void test_astarte_set_is_empty(void)
{
    astarte_set_handle_t handle;
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_set_init(&handle));
    TEST_ASSERT_TRUE(astarte_set_is_empty(handle));

    astarte_set_element_t element_1 = {.value = "string 1", .value_len = strlen("string 1") + 1};
    astarte_set_element_t element_2 = {.value = "string 2", .value_len = strlen("string 2") + 1};

    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_set_add(handle, element_1));
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_set_add(handle, element_2));

    TEST_ASSERT_FALSE(astarte_set_is_empty(handle));
}

void test_astarte_set_add_pop(void)
{
    astarte_set_handle_t astarte_set_handle;
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_set_init(&astarte_set_handle));

    astarte_set_element_t element_1 = {.value = "string 1", .value_len = strlen("string 1") + 1};
    astarte_set_element_t element_2 = {.value = "string 2", .value_len = strlen("string 2") + 1};
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_set_add(astarte_set_handle, element_1));
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_set_add(astarte_set_handle, element_2));

    astarte_set_element_t ret_element_1, ret_element_2, ret_element_3;
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_set_pop(astarte_set_handle, &ret_element_1));
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_set_pop(astarte_set_handle, &ret_element_2));
    TEST_ASSERT_EQUAL(ASTARTE_ERR_NOT_FOUND, astarte_set_pop(astarte_set_handle, &ret_element_3));

    TEST_ASSERT_EQUAL_INT(element_1.value_len, ret_element_2.value_len);
    TEST_ASSERT_EQUAL_STRING(element_1.value, ret_element_2.value);
    TEST_ASSERT_EQUAL_INT(element_2.value_len, ret_element_1.value_len);
    TEST_ASSERT_EQUAL_STRING(element_2.value, ret_element_1.value);

    TEST_ASSERT_TRUE(astarte_set_is_empty(astarte_set_handle));

    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_set_add(astarte_set_handle, element_2));

    TEST_ASSERT_FALSE(astarte_set_is_empty(astarte_set_handle));

    astarte_set_destroy(astarte_set_handle);
}
