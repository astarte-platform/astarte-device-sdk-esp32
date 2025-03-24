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

#include "dlist.h"
#include "test_dlist.h"

#include <string.h>

#include <esp_log.h>

#define TAG "DLIST TEST"

void test_dlist_is_empty(void)
{
    dlist_t handle = dlist_init();

    TEST_ASSERT_TRUE(dlist_is_empty(&handle));

    char *item_1 = "string 1";
    char *item_2 = "string 2";
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK, dlist_append(&handle, item_1));
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK, dlist_append(&handle, item_2));

    TEST_ASSERT_FALSE(dlist_is_empty(&handle));

    TEST_ASSERT_EQUAL_PTR(item_2, dlist_remove_tail(&handle));
    TEST_ASSERT_EQUAL_PTR(item_1, dlist_remove_tail(&handle));

    TEST_ASSERT_TRUE(dlist_is_empty(&handle));
}

void test_dlist_append_remove_tail(void)
{
    dlist_t handle = dlist_init();

    TEST_ASSERT_NULL(dlist_remove_tail(&handle));

    char *item_1 = "string 1";
    char *item_2 = "string 2";
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK, dlist_append(&handle, item_1));
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK, dlist_append(&handle, item_2));

    TEST_ASSERT_EQUAL_PTR(item_2, dlist_remove_tail(&handle));
    TEST_ASSERT_EQUAL_PTR(item_1, dlist_remove_tail(&handle));
    TEST_ASSERT_NULL(dlist_remove_tail(&handle));

    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK, dlist_append(&handle, item_2));

    TEST_ASSERT_EQUAL_PTR(item_2, dlist_remove_tail(&handle));
    TEST_ASSERT_NULL(dlist_remove_tail(&handle));

    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK, dlist_append(&handle, item_1));
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK, dlist_append(&handle, item_2));
}

void test_dlist_destroy(void)
{
    // Destroy empty list
    dlist_t handle_1 = dlist_init();

    dlist_destroy(&handle_1);

    // Destroy single element list
    dlist_t handle_2 = dlist_init();

    char *item_1 = "string 1";
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK, dlist_append(&handle_2, item_1));

    dlist_destroy(&handle_2);

    // Destroy multiple elements list
    dlist_t handle_3 = dlist_init();

    char *item_2 = "string 2";
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK, dlist_append(&handle_3, item_1));
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK, dlist_append(&handle_3, item_2));

    dlist_destroy(&handle_3);
}

void test_dlist_iterator(void)
{
    // Iterate through empty list
    dlist_t handle = dlist_init();

    dlist_iterator_t iterator_1;
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_NOT_FOUND, dlist_iterator_init(&handle, &iterator_1));

    // Iterate through single element list
    char *item_1 = "string 1";
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK, dlist_append(&handle, item_1));

    dlist_iterator_t iterator_2;
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK, dlist_iterator_init(&handle, &iterator_2));

    TEST_ASSERT_EQUAL_PTR(item_1, dlist_iterator_get_item(&iterator_2));
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_NOT_FOUND, dlist_iterator_advance(&iterator_2));

    // Iterate through a three elements list
    char *item_2 = "string 2";
    char *item_3 = "string 3";
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK, dlist_append(&handle, item_2));
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK, dlist_append(&handle, item_3));

    dlist_iterator_t iterator_3;
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK, dlist_iterator_init(&handle, &iterator_3));

    TEST_ASSERT_EQUAL_PTR(item_1, dlist_iterator_get_item(&iterator_3));
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK, dlist_iterator_advance(&iterator_3));
    TEST_ASSERT_EQUAL_PTR(item_2, dlist_iterator_get_item(&iterator_3));
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK, dlist_iterator_advance(&iterator_3));
    TEST_ASSERT_EQUAL_PTR(item_3, dlist_iterator_get_item(&iterator_3));
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_NOT_FOUND, dlist_iterator_advance(&iterator_3));

    dlist_destroy(&handle);
}

void test_dlist_iterator_replace(void)
{
    // Iterate through empty list
    dlist_t handle = dlist_init();

    // Iterate through a three elements list, and replace the second element content
    char *item_1 = "string 1";
    char *item_2 = "string 2";
    char *item_2_substitute = "string 2 substitute";
    char *item_3 = "string 3";
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK, dlist_append(&handle, item_1));
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK, dlist_append(&handle, item_2));
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK, dlist_append(&handle, item_3));

    dlist_iterator_t iterator;
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK, dlist_iterator_init(&handle, &iterator));

    TEST_ASSERT_EQUAL_PTR(item_1, dlist_iterator_get_item(&iterator));
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK, dlist_iterator_advance(&iterator));

    TEST_ASSERT_EQUAL_PTR(item_2, dlist_iterator_get_item(&iterator));
    TEST_ASSERT_EQUAL_PTR(item_2, dlist_iterator_replace_item(&iterator, item_2_substitute));
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK, dlist_iterator_advance(&iterator));

    TEST_ASSERT_EQUAL_PTR(item_3, dlist_iterator_get_item(&iterator));
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_NOT_FOUND, dlist_iterator_advance(&iterator));

    // Iterate a second time to check if the item has been correctly stored
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK, dlist_iterator_init(&handle, &iterator));

    TEST_ASSERT_EQUAL_PTR(item_1, dlist_iterator_get_item(&iterator));
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK, dlist_iterator_advance(&iterator));
    TEST_ASSERT_EQUAL_PTR(item_2_substitute, dlist_iterator_get_item(&iterator));
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK, dlist_iterator_advance(&iterator));
    TEST_ASSERT_EQUAL_PTR(item_3, dlist_iterator_get_item(&iterator));
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_NOT_FOUND, dlist_iterator_advance(&iterator));

    dlist_destroy(&handle);
}
