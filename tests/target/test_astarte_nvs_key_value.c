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

#include "test_astarte_nvs_key_value.h"
#include "astarte_nvs_key_value.h"
#include "unity.h"

#include <esp_log.h>
#include <nvs_flash.h>

#define TAG "NVS KEY VALUE TEST"

void test_astarte_nvs_key_value_set_get_cycle(void)
{
    // Prepare device by erasing default nvs partition
    TEST_ASSERT_EQUAL(ESP_OK, nvs_flash_erase());
    // Prepare device by initializing default nvs partition
    TEST_ASSERT_EQUAL(ESP_OK, nvs_flash_init());

    // Write three key value pairs in nvs
    const char nvs_namespace[] = "NVS key value";
    const char key1[] = "super long key that would not fit normally 1";
    const char key2[] = "super long key that would not fit normally 2";
    const char key3[] = "super long key that would not fit normally 3";
    uint8_t value1[2] = { 1, 2 };
    uint8_t value2[5] = { 158, 11, 12, 15, 16 };
    uint8_t value3[1] = { 42 };

    nvs_handle_t nvs_handle;
    TEST_ASSERT_EQUAL(ESP_OK, nvs_open(nvs_namespace, NVS_READWRITE, &nvs_handle));

    // Set key value pairs in NVS
    TEST_ASSERT_EQUAL(ESP_OK, astarte_nvs_key_value_set_blob(nvs_handle, key1, (void *) value1, 2));
    TEST_ASSERT_EQUAL(ESP_OK, astarte_nvs_key_value_set_blob(nvs_handle, key2, (void *) value2, 5));
    TEST_ASSERT_EQUAL(ESP_OK, astarte_nvs_key_value_set_blob(nvs_handle, key3, (void *) value3, 1));

    TEST_ASSERT_EQUAL(ESP_OK, nvs_commit(nvs_handle));

    size_t length1, length2, length3;
    uint8_t read_value1[2] = { 0 };
    uint8_t read_value2[5] = { 0 };
    uint8_t read_value3[1] = { 0 };

    // Read length of each value from NVS
    TEST_ASSERT_EQUAL(ESP_OK, astarte_nvs_key_value_get_blob(nvs_handle, key2, NULL, &length2));
    TEST_ASSERT_EQUAL(5, length2);
    TEST_ASSERT_EQUAL(ESP_OK, astarte_nvs_key_value_get_blob(nvs_handle, key3, NULL, &length3));
    TEST_ASSERT_EQUAL(1, length3);
    TEST_ASSERT_EQUAL(ESP_OK, astarte_nvs_key_value_get_blob(nvs_handle, key1, NULL, &length1));
    TEST_ASSERT_EQUAL(2, length1);

    // Read all values from NVS
    TEST_ASSERT_EQUAL(
        ESP_OK, astarte_nvs_key_value_get_blob(nvs_handle, key3, read_value3, &length3));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(value3, read_value3, length3);
    TEST_ASSERT_EQUAL(
        ESP_OK, astarte_nvs_key_value_get_blob(nvs_handle, key1, read_value1, &length1));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(value1, read_value1, length1);
    TEST_ASSERT_EQUAL(
        ESP_OK, astarte_nvs_key_value_get_blob(nvs_handle, key2, read_value2, &length2));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(value2, read_value2, length2);

    nvs_close(nvs_handle);
}

void test_astarte_nvs_key_value_erase_key(void)
{
    // Prepare device by erasing default nvs partition
    TEST_ASSERT_EQUAL(ESP_OK, nvs_flash_erase());
    // Prepare device by initializing default nvs partition
    TEST_ASSERT_EQUAL(ESP_OK, nvs_flash_init());

    // Write three key value pairs in nvs
    const char nvs_namespace[] = "NVS key value";
    const char key1[] = "super long key that would not fit normally 1";
    const char key2[] = "super long key that would not fit normally 2";
    const char key3[] = "super long key that would not fit normally 3";
    const char key4[] = "super long key that would not fit normally 4";
    uint8_t value1[2] = { 1, 2 };
    uint8_t value2[5] = { 158, 11, 12, 15, 16 };
    uint8_t value3[1] = { 42 };
    uint8_t value4[4] = { 6, 7, 8, 9 };

    nvs_handle_t nvs_handle;
    TEST_ASSERT_EQUAL(ESP_OK, nvs_open(nvs_namespace, NVS_READWRITE, &nvs_handle));

    // Set key value pairs in NVS
    TEST_ASSERT_EQUAL(ESP_OK, astarte_nvs_key_value_set_blob(nvs_handle, key1, (void *) value1, 2));
    TEST_ASSERT_EQUAL(ESP_OK, astarte_nvs_key_value_set_blob(nvs_handle, key2, (void *) value2, 5));
    TEST_ASSERT_EQUAL(ESP_OK, astarte_nvs_key_value_set_blob(nvs_handle, key3, (void *) value3, 1));
    TEST_ASSERT_EQUAL(ESP_OK, astarte_nvs_key_value_set_blob(nvs_handle, key4, (void *) value4, 4));
    TEST_ASSERT_EQUAL(ESP_OK, nvs_commit(nvs_handle));

    // Remove one of the key pair values stored
    TEST_ASSERT_EQUAL(ESP_OK, astarte_nvs_key_value_erase_key(nvs_handle, key2));
    TEST_ASSERT_EQUAL(ESP_OK, nvs_commit(nvs_handle));

    // Check that all other keys/values are still present
    size_t length1 = 0, length2 = 0, length3 = 0, length4 = 0;
    TEST_ASSERT_EQUAL(
        ESP_ERR_NVS_NOT_FOUND, astarte_nvs_key_value_get_blob(nvs_handle, key2, NULL, &length2));
    TEST_ASSERT_EQUAL(0, length2);
    TEST_ASSERT_EQUAL(ESP_OK, astarte_nvs_key_value_get_blob(nvs_handle, key3, NULL, &length3));
    TEST_ASSERT_EQUAL(1, length3);
    TEST_ASSERT_EQUAL(ESP_OK, astarte_nvs_key_value_get_blob(nvs_handle, key1, NULL, &length1));
    TEST_ASSERT_EQUAL(2, length1);
    TEST_ASSERT_EQUAL(ESP_OK, astarte_nvs_key_value_get_blob(nvs_handle, key4, NULL, &length4));
    TEST_ASSERT_EQUAL(4, length4);
    uint8_t read_value1[2] = { 0 };
    uint8_t read_value3[1] = { 0 };
    uint8_t read_value4[4] = { 0 };
    TEST_ASSERT_EQUAL(
        ESP_OK, astarte_nvs_key_value_get_blob(nvs_handle, key3, read_value3, &length3));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(value3, read_value3, length3);
    TEST_ASSERT_EQUAL(
        ESP_OK, astarte_nvs_key_value_get_blob(nvs_handle, key1, read_value1, &length1));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(value1, read_value1, length1);
    TEST_ASSERT_EQUAL(
        ESP_OK, astarte_nvs_key_value_get_blob(nvs_handle, key4, read_value4, &length4));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(value4, read_value4, length4);

    // Remove one more of the key pair values stored
    TEST_ASSERT_EQUAL(ESP_OK, astarte_nvs_key_value_erase_key(nvs_handle, key4));
    TEST_ASSERT_EQUAL(ESP_OK, nvs_commit(nvs_handle));

    // Check that all other keys/values are still present
    length1 = 0;
    length2 = 0;
    length3 = 0;
    length4 = 0;
    TEST_ASSERT_EQUAL(
        ESP_ERR_NVS_NOT_FOUND, astarte_nvs_key_value_get_blob(nvs_handle, key2, NULL, &length2));
    TEST_ASSERT_EQUAL(ESP_OK, astarte_nvs_key_value_get_blob(nvs_handle, key3, NULL, &length3));
    TEST_ASSERT_EQUAL(1, length3);
    TEST_ASSERT_EQUAL(ESP_OK, astarte_nvs_key_value_get_blob(nvs_handle, key1, NULL, &length1));
    TEST_ASSERT_EQUAL(2, length1);
    TEST_ASSERT_EQUAL(
        ESP_ERR_NVS_NOT_FOUND, astarte_nvs_key_value_get_blob(nvs_handle, key4, NULL, &length4));
    read_value1[0] = 0;
    read_value3[0] = 0;
    TEST_ASSERT_EQUAL(
        ESP_OK, astarte_nvs_key_value_get_blob(nvs_handle, key3, read_value3, &length3));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(value3, read_value3, length3);
    TEST_ASSERT_EQUAL(
        ESP_OK, astarte_nvs_key_value_get_blob(nvs_handle, key1, read_value1, &length1));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(value1, read_value1, length1);

    // Remove one more of the key pair values stored
    TEST_ASSERT_EQUAL(ESP_OK, astarte_nvs_key_value_erase_key(nvs_handle, key1));
    TEST_ASSERT_EQUAL(ESP_OK, nvs_commit(nvs_handle));

    // Check that all other keys/values are still present
    length1 = 0;
    length2 = 0;
    length3 = 0;
    length4 = 0;
    TEST_ASSERT_EQUAL(
        ESP_ERR_NVS_NOT_FOUND, astarte_nvs_key_value_get_blob(nvs_handle, key2, NULL, &length2));
    TEST_ASSERT_EQUAL(ESP_OK, astarte_nvs_key_value_get_blob(nvs_handle, key3, NULL, &length3));
    TEST_ASSERT_EQUAL(1, length3);
    TEST_ASSERT_EQUAL(
        ESP_ERR_NVS_NOT_FOUND, astarte_nvs_key_value_get_blob(nvs_handle, key1, NULL, &length1));
    TEST_ASSERT_EQUAL(
        ESP_ERR_NVS_NOT_FOUND, astarte_nvs_key_value_get_blob(nvs_handle, key4, NULL, &length4));
    read_value3[0] = 0;
    TEST_ASSERT_EQUAL(
        ESP_OK, astarte_nvs_key_value_get_blob(nvs_handle, key3, read_value3, &length3));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(value3, read_value3, length3);

    // Remove one more of the key pair values stored
    TEST_ASSERT_EQUAL(ESP_OK, astarte_nvs_key_value_erase_key(nvs_handle, key3));
    TEST_ASSERT_EQUAL(ESP_OK, nvs_commit(nvs_handle));

    // Check that all other keys/values are still present
    length1 = 0;
    length2 = 0;
    length3 = 0;
    length4 = 0;
    TEST_ASSERT_EQUAL(
        ESP_ERR_NVS_NOT_FOUND, astarte_nvs_key_value_get_blob(nvs_handle, key2, NULL, &length2));
    TEST_ASSERT_EQUAL(
        ESP_ERR_NVS_NOT_FOUND, astarte_nvs_key_value_get_blob(nvs_handle, key3, NULL, &length3));
    TEST_ASSERT_EQUAL(
        ESP_ERR_NVS_NOT_FOUND, astarte_nvs_key_value_get_blob(nvs_handle, key1, NULL, &length1));
    TEST_ASSERT_EQUAL(
        ESP_ERR_NVS_NOT_FOUND, astarte_nvs_key_value_get_blob(nvs_handle, key4, NULL, &length4));

    nvs_close(nvs_handle);
}

void test_astarte_nvs_key_value_iterator(void)
{
    // Prepare device by erasing default nvs partition
    TEST_ASSERT_EQUAL(ESP_OK, nvs_flash_erase());
    // Prepare device by initializing default nvs partition
    TEST_ASSERT_EQUAL(ESP_OK, nvs_flash_init());

    // Write three key value pairs in nvs
    const char nvs_namespace[] = "NVS key value";
    const char key1[] = "super long key that would not fit normally 1";
    const char key2[] = "super long key that would not fit normally 2";
    const char key3[] = "super long key that would not fit normally 3";
    const char key4[] = "super long key that would not fit normally 4";
    uint8_t value1[2] = { 1, 2 };
    uint8_t value2[5] = { 158, 11, 12, 15, 16 };
    uint8_t value3[1] = { 42 };
    uint8_t value4[4] = { 6, 7, 8, 9 };

    nvs_handle_t nvs_handle;
    TEST_ASSERT_EQUAL(ESP_OK, nvs_open(nvs_namespace, NVS_READWRITE, &nvs_handle));

    // Set key value pairs in NVS
    TEST_ASSERT_EQUAL(ESP_OK, astarte_nvs_key_value_set_blob(nvs_handle, key1, (void *) value1, 2));
    TEST_ASSERT_EQUAL(ESP_OK, astarte_nvs_key_value_set_blob(nvs_handle, key3, (void *) value3, 1));
    TEST_ASSERT_EQUAL(ESP_OK, astarte_nvs_key_value_set_blob(nvs_handle, key2, (void *) value2, 5));
    TEST_ASSERT_EQUAL(ESP_OK, astarte_nvs_key_value_set_blob(nvs_handle, key4, (void *) value4, 4));
    TEST_ASSERT_EQUAL(ESP_OK, nvs_commit(nvs_handle));

    // Initialize an iterator for the NVS
    astarte_nvs_key_value_iterator_t nvs_key_value_iterator;
    TEST_ASSERT_EQUAL(ESP_OK,
        astarte_nvs_key_value_iterator_init(nvs_handle, NVS_TYPE_BLOB, &nvs_key_value_iterator));

    // Check element
    size_t key_len, value_len;
    TEST_ASSERT_EQUAL(ESP_OK,
        astarte_nvs_key_value_iterator_get_element(
            &nvs_key_value_iterator, NULL, &key_len, NULL, &value_len));
    TEST_ASSERT_EQUAL(45, key_len);
    TEST_ASSERT_EQUAL(2, value_len);
    char out_key1[45];
    uint8_t out_value1[2];
    TEST_ASSERT_EQUAL(ESP_OK,
        astarte_nvs_key_value_iterator_get_element(
            &nvs_key_value_iterator, out_key1, &key_len, out_value1, &value_len));
    TEST_ASSERT_EQUAL_STRING(key1, out_key1);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(value1, out_value1, 2);

    // Advance iterator
    TEST_ASSERT_EQUAL(ESP_OK, astarte_nvs_key_value_iterator_next(&nvs_key_value_iterator));

    // Check element
    TEST_ASSERT_EQUAL(ESP_OK,
        astarte_nvs_key_value_iterator_get_element(
            &nvs_key_value_iterator, NULL, &key_len, NULL, &value_len));
    TEST_ASSERT_EQUAL(45, key_len);
    TEST_ASSERT_EQUAL(1, value_len);
    char out_key3[45];
    uint8_t out_value3[1];
    TEST_ASSERT_EQUAL(ESP_OK,
        astarte_nvs_key_value_iterator_get_element(
            &nvs_key_value_iterator, out_key3, &key_len, out_value3, &value_len));
    TEST_ASSERT_EQUAL_STRING(key3, out_key3);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(value3, out_value3, 1);

    // Advance iterator
    TEST_ASSERT_EQUAL(ESP_OK, astarte_nvs_key_value_iterator_next(&nvs_key_value_iterator));

    // Check element
    TEST_ASSERT_EQUAL(ESP_OK,
        astarte_nvs_key_value_iterator_get_element(
            &nvs_key_value_iterator, NULL, &key_len, NULL, &value_len));
    TEST_ASSERT_EQUAL(45, key_len);
    TEST_ASSERT_EQUAL(5, value_len);
    char out_key2[45];
    uint8_t out_value2[5];
    TEST_ASSERT_EQUAL(ESP_OK,
        astarte_nvs_key_value_iterator_get_element(
            &nvs_key_value_iterator, out_key2, &key_len, out_value2, &value_len));
    TEST_ASSERT_EQUAL_STRING(key2, out_key2);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(value2, out_value2, 5);

    // Advance iterator
    TEST_ASSERT_EQUAL(ESP_OK, astarte_nvs_key_value_iterator_next(&nvs_key_value_iterator));

    // Check element
    TEST_ASSERT_EQUAL(ESP_OK,
        astarte_nvs_key_value_iterator_get_element(
            &nvs_key_value_iterator, NULL, &key_len, NULL, &value_len));
    TEST_ASSERT_EQUAL(45, key_len);
    TEST_ASSERT_EQUAL(4, value_len);
    char out_key4[45];
    uint8_t out_value4[4];
    TEST_ASSERT_EQUAL(ESP_OK,
        astarte_nvs_key_value_iterator_get_element(
            &nvs_key_value_iterator, out_key4, &key_len, out_value4, &value_len));
    TEST_ASSERT_EQUAL_STRING(key4, out_key4);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(value4, out_value4, 4);

    // Advance iterator
    TEST_ASSERT_EQUAL(
        ESP_ERR_NVS_NOT_FOUND, astarte_nvs_key_value_iterator_next(&nvs_key_value_iterator));

    nvs_close(nvs_handle);
}

// This will test iterating over key/value pairs and at the same time deleting some of the found
// key/pairs.
// This is the only supported use case of dynamically changing NVS while iterating.
void test_astarte_nvs_key_value_iterator_on_changing_memory_remove_first(void)
{
    // Prepare device by erasing default nvs partition
    TEST_ASSERT_EQUAL(ESP_OK, nvs_flash_erase());
    // Prepare device by initializing default nvs partition
    TEST_ASSERT_EQUAL(ESP_OK, nvs_flash_init());

    // Write three key value pairs in nvs
    const char nvs_namespace[] = "NVS key value";
    const char key1[] = "super long key that would not fit normally 1";
    const char key2[] = "super long key that would not fit normally 2";
    const char key3[] = "super long key that would not fit normally 3";
    const char key4[] = "super long key that would not fit normally 4";
    uint8_t value1[2] = { 1, 2 };
    uint8_t value2[5] = { 158, 11, 12, 15, 16 };
    uint8_t value3[1] = { 42 };
    uint8_t value4[4] = { 6, 7, 8, 9 };

    nvs_handle_t nvs_handle;
    TEST_ASSERT_EQUAL(ESP_OK, nvs_open(nvs_namespace, NVS_READWRITE, &nvs_handle));

    // Set key value pairs in NVS
    TEST_ASSERT_EQUAL(ESP_OK, astarte_nvs_key_value_set_blob(nvs_handle, key1, (void *) value1, 2));
    TEST_ASSERT_EQUAL(ESP_OK, astarte_nvs_key_value_set_blob(nvs_handle, key2, (void *) value2, 5));
    TEST_ASSERT_EQUAL(ESP_OK, astarte_nvs_key_value_set_blob(nvs_handle, key3, (void *) value3, 1));
    TEST_ASSERT_EQUAL(ESP_OK, astarte_nvs_key_value_set_blob(nvs_handle, key4, (void *) value4, 4));
    TEST_ASSERT_EQUAL(ESP_OK, nvs_commit(nvs_handle));

    // Initialize an iterator for the NVS
    astarte_nvs_key_value_iterator_t nvs_key_value_iterator;
    TEST_ASSERT_EQUAL(ESP_OK,
        astarte_nvs_key_value_iterator_init(nvs_handle, NVS_TYPE_BLOB, &nvs_key_value_iterator));

    // Check first element
    size_t key_len, value_len;
    TEST_ASSERT_EQUAL(ESP_OK,
        astarte_nvs_key_value_iterator_get_element(
            &nvs_key_value_iterator, NULL, &key_len, NULL, &value_len));
    TEST_ASSERT_EQUAL(45, key_len);
    TEST_ASSERT_EQUAL(2, value_len);
    char out_key1[45];
    uint8_t out_value1[2];
    TEST_ASSERT_EQUAL(ESP_OK,
        astarte_nvs_key_value_iterator_get_element(
            &nvs_key_value_iterator, out_key1, &key_len, out_value1, &value_len));
    TEST_ASSERT_EQUAL_STRING(key1, out_key1);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(value1, out_value1, 2);

    // Remove the element just fetched
    TEST_ASSERT_EQUAL(ESP_OK, astarte_nvs_key_value_erase_key(nvs_handle, key1));
    TEST_ASSERT_EQUAL(ESP_OK, nvs_commit(nvs_handle));

    // Check the element the iterator is pointing to. Now it's the next element
    TEST_ASSERT_EQUAL(ESP_OK,
        astarte_nvs_key_value_iterator_get_element(
            &nvs_key_value_iterator, NULL, &key_len, NULL, &value_len));
    TEST_ASSERT_EQUAL(45, key_len);
    TEST_ASSERT_EQUAL(5, value_len);
    char out_key2[45];
    uint8_t out_value2[5];
    TEST_ASSERT_EQUAL(ESP_OK,
        astarte_nvs_key_value_iterator_get_element(
            &nvs_key_value_iterator, out_key2, &key_len, out_value2, &value_len));
    TEST_ASSERT_EQUAL_STRING(key2, out_key2);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(value2, out_value2, 2);

    // Advance iterator
    TEST_ASSERT_EQUAL(ESP_OK, astarte_nvs_key_value_iterator_next(&nvs_key_value_iterator));

    // Check element
    TEST_ASSERT_EQUAL(ESP_OK,
        astarte_nvs_key_value_iterator_get_element(
            &nvs_key_value_iterator, NULL, &key_len, NULL, &value_len));
    TEST_ASSERT_EQUAL(45, key_len);
    TEST_ASSERT_EQUAL(1, value_len);
    char out_key3[45];
    uint8_t out_value3[1];
    TEST_ASSERT_EQUAL(ESP_OK,
        astarte_nvs_key_value_iterator_get_element(
            &nvs_key_value_iterator, out_key3, &key_len, out_value3, &value_len));
    TEST_ASSERT_EQUAL_STRING(key3, out_key3);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(value3, out_value3, 1);

    // Advance iterator
    TEST_ASSERT_EQUAL(ESP_OK, astarte_nvs_key_value_iterator_next(&nvs_key_value_iterator));

    // Check element
    TEST_ASSERT_EQUAL(ESP_OK,
        astarte_nvs_key_value_iterator_get_element(
            &nvs_key_value_iterator, NULL, &key_len, NULL, &value_len));
    TEST_ASSERT_EQUAL(45, key_len);
    TEST_ASSERT_EQUAL(4, value_len);
    char out_key4[45];
    uint8_t out_value4[4];
    TEST_ASSERT_EQUAL(ESP_OK,
        astarte_nvs_key_value_iterator_get_element(
            &nvs_key_value_iterator, out_key4, &key_len, out_value4, &value_len));
    TEST_ASSERT_EQUAL_STRING(key4, out_key4);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(value4, out_value4, 4);

    // Advance iterator
    TEST_ASSERT_EQUAL(
        ESP_ERR_NVS_NOT_FOUND, astarte_nvs_key_value_iterator_next(&nvs_key_value_iterator));

    nvs_close(nvs_handle);
}

// This will test iterating over key/value pairs and at the same time deleting some of the found
// key/pairs.
// This is the only supported use case of dynamically changing NVS while iterating.
void test_astarte_nvs_key_value_iterator_on_changing_memory_remove_last(void)
{
    // Prepare device by erasing default nvs partition
    TEST_ASSERT_EQUAL(ESP_OK, nvs_flash_erase());
    // Prepare device by initializing default nvs partition
    TEST_ASSERT_EQUAL(ESP_OK, nvs_flash_init());

    // Write three key value pairs in nvs
    const char nvs_namespace[] = "NVS key value";
    const char key1[] = "super long key that would not fit normally 1";
    const char key2[] = "super long key that would not fit normally 2";
    const char key3[] = "super long key that would not fit normally 3";
    const char key4[] = "super long key that would not fit normally 4";
    uint8_t value1[2] = { 1, 2 };
    uint8_t value2[5] = { 158, 11, 12, 15, 16 };
    uint8_t value3[1] = { 42 };
    uint8_t value4[4] = { 6, 7, 8, 9 };

    nvs_handle_t nvs_handle;
    TEST_ASSERT_EQUAL(ESP_OK, nvs_open(nvs_namespace, NVS_READWRITE, &nvs_handle));

    // Set key value pairs in NVS
    TEST_ASSERT_EQUAL(ESP_OK, astarte_nvs_key_value_set_blob(nvs_handle, key1, (void *) value1, 2));
    TEST_ASSERT_EQUAL(ESP_OK, astarte_nvs_key_value_set_blob(nvs_handle, key2, (void *) value2, 5));
    TEST_ASSERT_EQUAL(ESP_OK, astarte_nvs_key_value_set_blob(nvs_handle, key3, (void *) value3, 1));
    TEST_ASSERT_EQUAL(ESP_OK, astarte_nvs_key_value_set_blob(nvs_handle, key4, (void *) value4, 4));
    TEST_ASSERT_EQUAL(ESP_OK, nvs_commit(nvs_handle));

    // Initialize an iterator for the NVS
    astarte_nvs_key_value_iterator_t nvs_key_value_iterator;
    TEST_ASSERT_EQUAL(ESP_OK,
        astarte_nvs_key_value_iterator_init(nvs_handle, NVS_TYPE_BLOB, &nvs_key_value_iterator));

    // Check first element
    size_t key_len, value_len;
    TEST_ASSERT_EQUAL(ESP_OK,
        astarte_nvs_key_value_iterator_get_element(
            &nvs_key_value_iterator, NULL, &key_len, NULL, &value_len));
    TEST_ASSERT_EQUAL(45, key_len);
    TEST_ASSERT_EQUAL(2, value_len);
    char out_key1[45];
    uint8_t out_value1[2];
    TEST_ASSERT_EQUAL(ESP_OK,
        astarte_nvs_key_value_iterator_get_element(
            &nvs_key_value_iterator, out_key1, &key_len, out_value1, &value_len));
    TEST_ASSERT_EQUAL_STRING(key1, out_key1);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(value1, out_value1, 2);

    // Advance iterator
    TEST_ASSERT_EQUAL(ESP_OK, astarte_nvs_key_value_iterator_next(&nvs_key_value_iterator));

    // Check the element the iterator is pointing to. Now it's the next element
    TEST_ASSERT_EQUAL(ESP_OK,
        astarte_nvs_key_value_iterator_get_element(
            &nvs_key_value_iterator, NULL, &key_len, NULL, &value_len));
    TEST_ASSERT_EQUAL(45, key_len);
    TEST_ASSERT_EQUAL(5, value_len);
    char out_key2[45];
    uint8_t out_value2[5];
    TEST_ASSERT_EQUAL(ESP_OK,
        astarte_nvs_key_value_iterator_get_element(
            &nvs_key_value_iterator, out_key2, &key_len, out_value2, &value_len));
    TEST_ASSERT_EQUAL_STRING(key2, out_key2);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(value2, out_value2, 2);

    // Advance iterator
    TEST_ASSERT_EQUAL(ESP_OK, astarte_nvs_key_value_iterator_next(&nvs_key_value_iterator));

    // Check element
    TEST_ASSERT_EQUAL(ESP_OK,
        astarte_nvs_key_value_iterator_get_element(
            &nvs_key_value_iterator, NULL, &key_len, NULL, &value_len));
    TEST_ASSERT_EQUAL(45, key_len);
    TEST_ASSERT_EQUAL(1, value_len);
    char out_key3[45];
    uint8_t out_value3[1];
    TEST_ASSERT_EQUAL(ESP_OK,
        astarte_nvs_key_value_iterator_get_element(
            &nvs_key_value_iterator, out_key3, &key_len, out_value3, &value_len));
    TEST_ASSERT_EQUAL_STRING(key3, out_key3);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(value3, out_value3, 1);

    // Advance iterator
    TEST_ASSERT_EQUAL(ESP_OK, astarte_nvs_key_value_iterator_next(&nvs_key_value_iterator));

    // Check element
    TEST_ASSERT_EQUAL(ESP_OK,
        astarte_nvs_key_value_iterator_get_element(
            &nvs_key_value_iterator, NULL, &key_len, NULL, &value_len));
    TEST_ASSERT_EQUAL(45, key_len);
    TEST_ASSERT_EQUAL(4, value_len);
    char out_key4[45];
    uint8_t out_value4[4];
    TEST_ASSERT_EQUAL(ESP_OK,
        astarte_nvs_key_value_iterator_get_element(
            &nvs_key_value_iterator, out_key4, &key_len, out_value4, &value_len));
    TEST_ASSERT_EQUAL_STRING(key4, out_key4);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(value4, out_value4, 4);

    // Advance iterator
    TEST_ASSERT_EQUAL(
        ESP_ERR_NVS_NOT_FOUND, astarte_nvs_key_value_iterator_next(&nvs_key_value_iterator));

    // Remove the element just fetched
    TEST_ASSERT_EQUAL(ESP_OK, astarte_nvs_key_value_erase_key(nvs_handle, key4));
    TEST_ASSERT_EQUAL(ESP_OK, nvs_commit(nvs_handle));

    // Advance iterator
    TEST_ASSERT_EQUAL(
        ESP_ERR_NVS_NOT_FOUND, astarte_nvs_key_value_iterator_next(&nvs_key_value_iterator));

    nvs_close(nvs_handle);
}

// This will test iterating over key/value pairs and at the same time deleting some of the found
// key/pairs.
// This is the only supported use case of dynamically changing NVS while iterating.
void test_astarte_nvs_key_value_iterator_on_changing_memory_remove_middle(void)
{
    // Prepare device by erasing default nvs partition
    TEST_ASSERT_EQUAL(ESP_OK, nvs_flash_erase());
    // Prepare device by initializing default nvs partition
    TEST_ASSERT_EQUAL(ESP_OK, nvs_flash_init());

    // Write three key value pairs in nvs
    const char nvs_namespace[] = "NVS key value";
    const char key1[] = "super long key that would not fit normally 1";
    const char key2[] = "super long key that would not fit normally 2";
    const char key3[] = "super long key that would not fit normally 3";
    const char key4[] = "super long key that would not fit normally 4";
    uint8_t value1[2] = { 1, 2 };
    uint8_t value2[5] = { 158, 11, 12, 15, 16 };
    uint8_t value3[1] = { 42 };
    uint8_t value4[4] = { 6, 7, 8, 9 };

    nvs_handle_t nvs_handle;
    TEST_ASSERT_EQUAL(ESP_OK, nvs_open(nvs_namespace, NVS_READWRITE, &nvs_handle));

    // Set key value pairs in NVS
    TEST_ASSERT_EQUAL(ESP_OK, astarte_nvs_key_value_set_blob(nvs_handle, key1, (void *) value1, 2));
    TEST_ASSERT_EQUAL(ESP_OK, astarte_nvs_key_value_set_blob(nvs_handle, key2, (void *) value2, 5));
    TEST_ASSERT_EQUAL(ESP_OK, astarte_nvs_key_value_set_blob(nvs_handle, key3, (void *) value3, 1));
    TEST_ASSERT_EQUAL(ESP_OK, astarte_nvs_key_value_set_blob(nvs_handle, key4, (void *) value4, 4));
    TEST_ASSERT_EQUAL(ESP_OK, nvs_commit(nvs_handle));

    // Initialize an iterator for the NVS
    astarte_nvs_key_value_iterator_t nvs_key_value_iterator;
    TEST_ASSERT_EQUAL(ESP_OK,
        astarte_nvs_key_value_iterator_init(nvs_handle, NVS_TYPE_BLOB, &nvs_key_value_iterator));

    // Check first element
    size_t key_len, value_len;
    TEST_ASSERT_EQUAL(ESP_OK,
        astarte_nvs_key_value_iterator_get_element(
            &nvs_key_value_iterator, NULL, &key_len, NULL, &value_len));
    TEST_ASSERT_EQUAL(45, key_len);
    TEST_ASSERT_EQUAL(2, value_len);
    char out_key1[45];
    uint8_t out_value1[2];
    TEST_ASSERT_EQUAL(ESP_OK,
        astarte_nvs_key_value_iterator_get_element(
            &nvs_key_value_iterator, out_key1, &key_len, out_value1, &value_len));
    TEST_ASSERT_EQUAL_STRING(key1, out_key1);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(value1, out_value1, 2);

    // Advance iterator
    TEST_ASSERT_EQUAL(ESP_OK, astarte_nvs_key_value_iterator_next(&nvs_key_value_iterator));

    // Check the element the iterator is pointing to. Now it's the next element
    TEST_ASSERT_EQUAL(ESP_OK,
        astarte_nvs_key_value_iterator_get_element(
            &nvs_key_value_iterator, NULL, &key_len, NULL, &value_len));
    TEST_ASSERT_EQUAL(45, key_len);
    TEST_ASSERT_EQUAL(5, value_len);
    char out_key2[45];
    uint8_t out_value2[5];
    TEST_ASSERT_EQUAL(ESP_OK,
        astarte_nvs_key_value_iterator_get_element(
            &nvs_key_value_iterator, out_key2, &key_len, out_value2, &value_len));
    TEST_ASSERT_EQUAL_STRING(key2, out_key2);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(value2, out_value2, 2);

    // Remove the element just fetched
    TEST_ASSERT_EQUAL(ESP_OK, astarte_nvs_key_value_erase_key(nvs_handle, key2));
    TEST_ASSERT_EQUAL(ESP_OK, nvs_commit(nvs_handle));

    // Check element
    TEST_ASSERT_EQUAL(ESP_OK,
        astarte_nvs_key_value_iterator_get_element(
            &nvs_key_value_iterator, NULL, &key_len, NULL, &value_len));
    TEST_ASSERT_EQUAL(45, key_len);
    TEST_ASSERT_EQUAL(1, value_len);
    char out_key3[45];
    uint8_t out_value3[1];
    TEST_ASSERT_EQUAL(ESP_OK,
        astarte_nvs_key_value_iterator_get_element(
            &nvs_key_value_iterator, out_key3, &key_len, out_value3, &value_len));
    TEST_ASSERT_EQUAL_STRING(key3, out_key3);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(value3, out_value3, 1);

    // Advance iterator
    TEST_ASSERT_EQUAL(ESP_OK, astarte_nvs_key_value_iterator_next(&nvs_key_value_iterator));

    // Check element
    TEST_ASSERT_EQUAL(ESP_OK,
        astarte_nvs_key_value_iterator_get_element(
            &nvs_key_value_iterator, NULL, &key_len, NULL, &value_len));
    TEST_ASSERT_EQUAL(45, key_len);
    TEST_ASSERT_EQUAL(4, value_len);
    char out_key4[45];
    uint8_t out_value4[4];
    TEST_ASSERT_EQUAL(ESP_OK,
        astarte_nvs_key_value_iterator_get_element(
            &nvs_key_value_iterator, out_key4, &key_len, out_value4, &value_len));
    TEST_ASSERT_EQUAL_STRING(key4, out_key4);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(value4, out_value4, 4);

    // Advance iterator
    TEST_ASSERT_EQUAL(
        ESP_ERR_NVS_NOT_FOUND, astarte_nvs_key_value_iterator_next(&nvs_key_value_iterator));

    // Advance iterator
    TEST_ASSERT_EQUAL(
        ESP_ERR_NVS_NOT_FOUND, astarte_nvs_key_value_iterator_next(&nvs_key_value_iterator));

    nvs_close(nvs_handle);
}
