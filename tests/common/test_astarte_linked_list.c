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

#include "astarte_linked_list.h"
#include "test_astarte_linked_list.h"

#include <string.h>

#include <esp_log.h>

#define TAG "LINKED LIST TEST"

void test_astarte_linked_list_is_empty(void)
{
    astarte_linked_list_handle_t handle;
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_linked_list_init(&handle));

    TEST_ASSERT_TRUE(astarte_linked_list_is_empty(handle));

    astarte_linked_list_item_t item_1 = {.value = "string 1", .value_len = strlen("string 1") + 1};
    astarte_linked_list_item_t item_2 = {.value = "string 2", .value_len = strlen("string 2") + 1};

    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_linked_list_append(handle, item_1));
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_linked_list_append(handle, item_2));

    TEST_ASSERT_FALSE(astarte_linked_list_is_empty(handle));

    astarte_linked_list_item_t ret_item_1, ret_item_2;
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_linked_list_remove_tail(handle, &ret_item_1));
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_linked_list_remove_tail(handle, &ret_item_2));

    TEST_ASSERT_TRUE(astarte_linked_list_is_empty(handle));
}

void test_astarte_linked_list_append_remove_tail(void)
{
    astarte_linked_list_handle_t handle;
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_linked_list_init(&handle));

    astarte_linked_list_item_t ret_item;
    TEST_ASSERT_EQUAL(ASTARTE_ERR_NOT_FOUND, astarte_linked_list_remove_tail(handle, &ret_item));

    astarte_linked_list_item_t item_1 = {.value = "string 1", .value_len = strlen("string 1") + 1};
    astarte_linked_list_item_t item_2 = {.value = "string 2", .value_len = strlen("string 2") + 1};
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_linked_list_append(handle, item_1));
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_linked_list_append(handle, item_2));

    astarte_linked_list_item_t ret_item_1, ret_item_2, ret_item_3;
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_linked_list_remove_tail(handle, &ret_item_1));
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_linked_list_remove_tail(handle, &ret_item_2));
    TEST_ASSERT_EQUAL(ASTARTE_ERR_NOT_FOUND, astarte_linked_list_remove_tail(handle, &ret_item_3));

    TEST_ASSERT_EQUAL_INT(item_1.value_len, ret_item_2.value_len);
    TEST_ASSERT_EQUAL_STRING(item_1.value, ret_item_2.value);
    TEST_ASSERT_EQUAL_INT(item_2.value_len, ret_item_1.value_len);
    TEST_ASSERT_EQUAL_STRING(item_2.value, ret_item_1.value);

    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_linked_list_append(handle, item_2));

    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_linked_list_remove_tail(handle, &ret_item_1));
    TEST_ASSERT_EQUAL(ASTARTE_ERR_NOT_FOUND, astarte_linked_list_remove_tail(handle, &ret_item_2));

    TEST_ASSERT_EQUAL_INT(item_2.value_len, ret_item_1.value_len);
    TEST_ASSERT_EQUAL_STRING(item_2.value, ret_item_1.value);

    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_linked_list_append(handle, item_1));
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_linked_list_append(handle, item_2));
}

void test_astarte_linked_list_find(void)
{
    astarte_linked_list_handle_t handle;
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_linked_list_init(&handle));

    bool result;
    astarte_linked_list_item_t item = {.value = "string", .value_len = strlen("string") + 1};
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_linked_list_find(handle, item, &result));
    TEST_ASSERT_FALSE(result);

    astarte_linked_list_item_t item_1 = {.value = "string 1", .value_len = strlen("string 1") + 1};
    astarte_linked_list_item_t item_2 = {.value = "string 2", .value_len = strlen("string 2") + 1};
    astarte_linked_list_item_t item_3 = {.value = "string 3", .value_len = strlen("string 3") + 1};
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_linked_list_append(handle, item_1));
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_linked_list_append(handle, item_2));

    bool result_1, result_2, result_3;
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_linked_list_find(handle, item_1, &result_1));
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_linked_list_find(handle, item_2, &result_2));
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_linked_list_find(handle, item_3, &result_3));

    TEST_ASSERT_TRUE(result_1);
    TEST_ASSERT_TRUE(result_2);
    TEST_ASSERT_FALSE(result_3);

    astarte_linked_list_destroy(handle);
}

void test_astarte_linked_list_destroy(void)
{
    // Destroy empty list
    astarte_linked_list_handle_t handle_1;
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_linked_list_init(&handle_1));

    astarte_linked_list_destroy(handle_1);

    // Destroy single element list
    astarte_linked_list_handle_t handle_2;
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_linked_list_init(&handle_2));

    astarte_linked_list_item_t item_1 = {.value = "string 1", .value_len = strlen("string 1") + 1};
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_linked_list_append(handle_2, item_1));

    astarte_linked_list_destroy(handle_2);

    // Destroy multiple elements list
    astarte_linked_list_handle_t handle_3;
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_linked_list_init(&handle_3));

    astarte_linked_list_item_t item_2 = {.value = "string 2", .value_len = strlen("string 2") + 1};
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_linked_list_append(handle_3, item_1));
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_linked_list_append(handle_3, item_2));

    astarte_linked_list_destroy(handle_3);
}

void test_astarte_linked_list_iterator(void)
{
    // Iterate through empty list
    astarte_linked_list_handle_t handle;
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_linked_list_init(&handle));

    astarte_linked_list_iterator_t iterator_1;
    TEST_ASSERT_EQUAL(ASTARTE_ERR_NOT_FOUND, astarte_linked_list_iterator_init(handle, &iterator_1));

    // Iterate through single element list
    astarte_linked_list_item_t item = {.value = "string 1", .value_len = strlen("string 1") + 1};
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_linked_list_append(handle, item));

    astarte_linked_list_iterator_t iterator_2;
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_linked_list_iterator_init(handle, &iterator_2));

    astarte_linked_list_item_t ret_item;
    astarte_linked_list_iterator_get_item(iterator_2, &ret_item);
    TEST_ASSERT_EQUAL(ASTARTE_ERR_NOT_FOUND, astarte_linked_list_iterator_advance(iterator_2));

    TEST_ASSERT_EQUAL_INT(item.value_len, ret_item.value_len);
    TEST_ASSERT_EQUAL_STRING(item.value, ret_item.value);

    astarte_linked_list_iterator_destroy(iterator_2);

    // Iterate through a three elements list
    astarte_linked_list_item_t item_2 = {.value = "string 2", .value_len = strlen("string 2") + 1};
    astarte_linked_list_item_t item_3 = {.value = "string 3", .value_len = strlen("string 3") + 1};
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_linked_list_append(handle, item_2));
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_linked_list_append(handle, item_3));

    astarte_linked_list_iterator_t iterator_3;
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_linked_list_iterator_init(handle, &iterator_3));

    astarte_linked_list_item_t ret_item_2, ret_item_3;
    astarte_linked_list_iterator_get_item(iterator_3, &ret_item);
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_linked_list_iterator_advance(iterator_3));
    astarte_linked_list_iterator_get_item(iterator_3, &ret_item_2);
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_linked_list_iterator_advance(iterator_3));
    astarte_linked_list_iterator_get_item(iterator_3, &ret_item_3);
    TEST_ASSERT_EQUAL(ASTARTE_ERR_NOT_FOUND, astarte_linked_list_iterator_advance(iterator_3));

    TEST_ASSERT_EQUAL_INT(item.value_len, ret_item.value_len);
    TEST_ASSERT_EQUAL_STRING(item.value, ret_item.value);
    TEST_ASSERT_EQUAL_INT(item_2.value_len, ret_item_2.value_len);
    TEST_ASSERT_EQUAL_STRING(item_2.value, ret_item_2.value);
    TEST_ASSERT_EQUAL_INT(item_3.value_len, ret_item_3.value_len);
    TEST_ASSERT_EQUAL_STRING(item_3.value, ret_item_3.value);

    astarte_linked_list_iterator_destroy(iterator_3);

    astarte_linked_list_destroy(handle);
}
