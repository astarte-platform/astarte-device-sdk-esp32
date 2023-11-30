/**
 * This file is part of Astarte.
 *
 * Copyright 2023 SECO Mind Srl
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
    astarte_linked_list_handle_t handle = astarte_linked_list_init();

    TEST_ASSERT_TRUE(astarte_linked_list_is_empty(&handle));

    char *item_1 = "string 1";
    char *item_2 = "string 2";
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_linked_list_append(&handle, item_1));
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_linked_list_append(&handle, item_2));

    TEST_ASSERT_FALSE(astarte_linked_list_is_empty(&handle));

    char *ret_item_1 = NULL;
    char *ret_item_2 = NULL;
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_linked_list_remove_tail(&handle, (void **) &ret_item_1));
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_linked_list_remove_tail(&handle, (void **) &ret_item_2));

    TEST_ASSERT_TRUE(astarte_linked_list_is_empty(&handle));
}

void test_astarte_linked_list_append_remove_tail(void)
{
    astarte_linked_list_handle_t handle = astarte_linked_list_init();

    char *ret_item_1 = NULL;
    TEST_ASSERT_EQUAL(
        ASTARTE_ERR_NOT_FOUND, astarte_linked_list_remove_tail(&handle, (void **) &ret_item_1));

    char *item_1 = "string 1";
    char *item_2 = "string 2";
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_linked_list_append(&handle, item_1));
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_linked_list_append(&handle, item_2));

    char *ret_item_2 = NULL;
    char *ret_item_3 = NULL;
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_linked_list_remove_tail(&handle, (void **) &ret_item_1));
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_linked_list_remove_tail(&handle, (void **) &ret_item_2));
    TEST_ASSERT_EQUAL(
        ASTARTE_ERR_NOT_FOUND, astarte_linked_list_remove_tail(&handle, (void **) &ret_item_3));

    TEST_ASSERT_EQUAL_STRING(item_1, ret_item_2);
    TEST_ASSERT_EQUAL_STRING(item_2, ret_item_1);

    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_linked_list_append(&handle, item_2));

    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_linked_list_remove_tail(&handle, (void **) &ret_item_1));
    TEST_ASSERT_EQUAL(
        ASTARTE_ERR_NOT_FOUND, astarte_linked_list_remove_tail(&handle, (void **) &ret_item_2));
    TEST_ASSERT_EQUAL_STRING(item_2, ret_item_1);

    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_linked_list_append(&handle, item_1));
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_linked_list_append(&handle, item_2));
}

void test_astarte_linked_list_destroy(void)
{
    // Destroy empty list
    astarte_linked_list_handle_t handle_1 = astarte_linked_list_init();

    astarte_linked_list_destroy(&handle_1);

    // Destroy single element list
    astarte_linked_list_handle_t handle_2 = astarte_linked_list_init();

    char *item_1 = "string 1";
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_linked_list_append(&handle_2, item_1));

    astarte_linked_list_destroy(&handle_2);

    // Destroy multiple elements list
    astarte_linked_list_handle_t handle_3 = astarte_linked_list_init();

    char *item_2 = "string 2";
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_linked_list_append(&handle_3, item_1));
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_linked_list_append(&handle_3, item_2));

    astarte_linked_list_destroy(&handle_3);
}

void test_astarte_linked_list_iterator(void)
{
    // Iterate through empty list
    astarte_linked_list_handle_t handle = astarte_linked_list_init();

    astarte_linked_list_iterator_t iterator_1;
    TEST_ASSERT_EQUAL(
        ASTARTE_ERR_NOT_FOUND, astarte_linked_list_iterator_init(&handle, &iterator_1));

    // Iterate through single element list
    char *item_1 = "string 1";
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_linked_list_append(&handle, item_1));

    astarte_linked_list_iterator_t iterator_2;
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_linked_list_iterator_init(&handle, &iterator_2));

    char *ret_item_1 = NULL;
    astarte_linked_list_iterator_get_item(&iterator_2, (void **) &ret_item_1);
    TEST_ASSERT_EQUAL(ASTARTE_ERR_NOT_FOUND, astarte_linked_list_iterator_advance(&iterator_2));

    TEST_ASSERT_EQUAL_STRING(item_1, ret_item_1);

    // Iterate through a three elements list
    char *item_2 = "string 2";
    char *item_3 = "string 3";
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_linked_list_append(&handle, item_2));
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_linked_list_append(&handle, item_3));

    astarte_linked_list_iterator_t iterator_3;
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_linked_list_iterator_init(&handle, &iterator_3));

    astarte_linked_list_iterator_get_item(&iterator_3, (void **) &ret_item_1);
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_linked_list_iterator_advance(&iterator_3));
    char *ret_item_2 = NULL;
    astarte_linked_list_iterator_get_item(&iterator_3, (void **) &ret_item_2);
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_linked_list_iterator_advance(&iterator_3));
    char *ret_item_3 = NULL;
    astarte_linked_list_iterator_get_item(&iterator_3, (void **) &ret_item_3);
    TEST_ASSERT_EQUAL(ASTARTE_ERR_NOT_FOUND, astarte_linked_list_iterator_advance(&iterator_3));

    TEST_ASSERT_EQUAL_STRING(item_1, ret_item_1);
    TEST_ASSERT_EQUAL_STRING(item_2, ret_item_2);
    TEST_ASSERT_EQUAL_STRING(item_3, ret_item_3);

    astarte_linked_list_destroy(&handle);
}

void test_astarte_linked_list_iterator_replace(void)
{
    // Iterate through empty list
    astarte_linked_list_handle_t handle = astarte_linked_list_init();

    // Iterate through a three elements list, and replace the second element content
    char *item_1 = "string 1";
    char *item_2 = "string 2";
    char *item_2_substitute = "string 2 substitute";
    char *item_3 = "string 3";
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_linked_list_append(&handle, item_1));
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_linked_list_append(&handle, item_2));
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_linked_list_append(&handle, item_3));

    astarte_linked_list_iterator_t iterator;
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_linked_list_iterator_init(&handle, &iterator));

    char *ret_item_1 = NULL;
    astarte_linked_list_iterator_get_item(&iterator, (void **) &ret_item_1);
    TEST_ASSERT_EQUAL_STRING(item_1, ret_item_1);

    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_linked_list_iterator_advance(&iterator));
    char *ret_item_2 = NULL;
    astarte_linked_list_iterator_get_item(&iterator, (void **) &ret_item_2);
    TEST_ASSERT_EQUAL_STRING(item_2, ret_item_2);

    astarte_linked_list_iterator_replace_item(&iterator, item_2_substitute);

    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_linked_list_iterator_advance(&iterator));
    char *ret_item_3 = NULL;
    astarte_linked_list_iterator_get_item(&iterator, (void **) &ret_item_3);
    TEST_ASSERT_EQUAL_STRING(item_3, ret_item_3);

    TEST_ASSERT_EQUAL(ASTARTE_ERR_NOT_FOUND, astarte_linked_list_iterator_advance(&iterator));

    // Iterate a second time to check if the item has been correctly stored
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_linked_list_iterator_init(&handle, &iterator));

    astarte_linked_list_iterator_get_item(&iterator, (void **) &ret_item_1);
    TEST_ASSERT_EQUAL_STRING(item_1, ret_item_1);

    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_linked_list_iterator_advance(&iterator));
    astarte_linked_list_iterator_get_item(&iterator, (void **) &ret_item_2);
    TEST_ASSERT_EQUAL_STRING(item_2_substitute, ret_item_2);

    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_linked_list_iterator_advance(&iterator));
    astarte_linked_list_iterator_get_item(&iterator, (void **) &ret_item_3);
    TEST_ASSERT_EQUAL_STRING(item_3, ret_item_3);

    TEST_ASSERT_EQUAL(ASTARTE_ERR_NOT_FOUND, astarte_linked_list_iterator_advance(&iterator));

    astarte_linked_list_destroy(&handle);
}
