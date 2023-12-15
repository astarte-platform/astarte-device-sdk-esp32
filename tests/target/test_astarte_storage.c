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

#include "test_astarte_storage.h"
#include "astarte_storage.h"
#include "unity.h"

#include <esp_log.h>
#include <nvs_flash.h>

#define TAG "ASTARTE STORAGE TEST"

// Some properties to use in the tests
#define I1_INAME "interface 1 name"
#define I1_IMAJOR 2
#define I1_P1_PNAME "/path 1"
#define I1_P1_PAYLOAD_LEN 10
static const uint8_t i1_p1_payload[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
#define I1_P2_PNAME "/path 2"
#define I1_P2_PAYLOAD_LEN 5
static const uint8_t i1_p2_payload[] = { 6, 7, 8, 9, 10 };
#define I2_INAME "interface 2 name"
#define I2_IMAJOR 1
#define I2_P1_PNAME "/path 1"
#define I2_P1_PAYLOAD_LEN 20
static const uint8_t i2_p1_payload[]
    = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20 };
#define I2_P2_PNAME "/path 2"
#define I2_P2_PAYLOAD_LEN 10
static const uint8_t i2_p2_payload[] = { 21, 22, 23, 24, 25, 26, 27, 28, 29, 30 };

void test_astarte_storage_store_delete_cycle(void)
{
    // Prepare device by erasing default nvs partition
    TEST_ASSERT_EQUAL(ESP_OK, nvs_flash_erase());
    // Prepare device by initializing default nvs partition
    TEST_ASSERT_EQUAL(ESP_OK, nvs_flash_init());

    // Variables where to store read payloads
    int32_t i1_major_read = 0;
    size_t i1_p1_payload_read_len = 0;
    uint8_t i1_p1_payload_read[I1_P1_PAYLOAD_LEN] = { 0 };
    size_t i1_p2_payload_read_len = 0;
    uint8_t i1_p2_payload_read[I1_P2_PAYLOAD_LEN] = { 0 };
    int32_t i2_major_read = 0;
    size_t i2_p1_payload_read_len = 0;
    uint8_t i2_p1_payload_read[I2_P1_PAYLOAD_LEN] = { 0 };
    size_t i2_p2_payload_read_len = 0;

    // Open storage
    astarte_storage_handle_t astarte_storage_handle;
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_storage_open(&astarte_storage_handle));

    // Delete returns not found when property is not present
    TEST_ASSERT_EQUAL(ASTARTE_ERR_NOT_FOUND,
        astarte_storage_delete_property(astarte_storage_handle, I1_INAME, I1_P1_PNAME));

    // Load does not work when empty storage
    TEST_ASSERT_EQUAL(ASTARTE_ERR_NOT_FOUND,
        astarte_storage_load_property(
            astarte_storage_handle, I1_INAME, I1_P1_PNAME, NULL, NULL, &i1_p1_payload_read_len));

    TEST_ASSERT_EQUAL(ASTARTE_ERR_NOT_FOUND,
        astarte_storage_load_property(
            astarte_storage_handle, I1_INAME, I1_P2_PNAME, NULL, NULL, &i1_p2_payload_read_len));

    TEST_ASSERT_EQUAL(ASTARTE_ERR_NOT_FOUND,
        astarte_storage_load_property(
            astarte_storage_handle, I2_INAME, I2_P1_PNAME, NULL, NULL, &i2_p1_payload_read_len));

    TEST_ASSERT_EQUAL(ASTARTE_ERR_NOT_FOUND,
        astarte_storage_load_property(
            astarte_storage_handle, I2_INAME, I2_P2_PNAME, NULL, NULL, &i2_p2_payload_read_len));

    // Store property
    TEST_ASSERT_EQUAL(ASTARTE_OK,
        astarte_storage_store_property(astarte_storage_handle, I1_INAME, I1_P1_PNAME, I1_IMAJOR,
            i1_p1_payload, I1_P1_PAYLOAD_LEN));
    TEST_ASSERT_EQUAL(ASTARTE_OK,
        astarte_storage_store_property(astarte_storage_handle, I1_INAME, I1_P2_PNAME, I1_IMAJOR,
            i1_p2_payload, I1_P2_PAYLOAD_LEN));
    TEST_ASSERT_EQUAL(ASTARTE_OK,
        astarte_storage_store_property(astarte_storage_handle, I2_INAME, I2_P1_PNAME, I2_IMAJOR,
            i2_p1_payload, I2_P1_PAYLOAD_LEN));

    // Load property
    TEST_ASSERT_EQUAL(ASTARTE_OK,
        astarte_storage_load_property(
            astarte_storage_handle, I1_INAME, I1_P1_PNAME, NULL, NULL, &i1_p1_payload_read_len));
    TEST_ASSERT_EQUAL(I1_P1_PAYLOAD_LEN, i1_p1_payload_read_len);
    TEST_ASSERT_EQUAL(ASTARTE_OK,
        astarte_storage_load_property(astarte_storage_handle, I1_INAME, I1_P1_PNAME, &i1_major_read,
            i1_p1_payload_read, &i1_p1_payload_read_len));
    TEST_ASSERT_EQUAL(I1_IMAJOR, i1_major_read);
    TEST_ASSERT_EQUAL(I1_P1_PAYLOAD_LEN, i1_p1_payload_read_len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(i1_p1_payload, i1_p1_payload_read, i1_p1_payload_read_len);

    TEST_ASSERT_EQUAL(ASTARTE_OK,
        astarte_storage_load_property(
            astarte_storage_handle, I1_INAME, I1_P2_PNAME, NULL, NULL, &i1_p2_payload_read_len));
    TEST_ASSERT_EQUAL(I1_P2_PAYLOAD_LEN, i1_p2_payload_read_len);
    TEST_ASSERT_EQUAL(ASTARTE_OK,
        astarte_storage_load_property(astarte_storage_handle, I1_INAME, I1_P2_PNAME, &i1_major_read,
            i1_p2_payload_read, &i1_p2_payload_read_len));
    TEST_ASSERT_EQUAL(I1_IMAJOR, i1_major_read);
    TEST_ASSERT_EQUAL(I1_P2_PAYLOAD_LEN, i1_p2_payload_read_len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(i1_p2_payload, i1_p2_payload_read, i1_p2_payload_read_len);

    TEST_ASSERT_EQUAL(ASTARTE_OK,
        astarte_storage_load_property(
            astarte_storage_handle, I2_INAME, I2_P1_PNAME, NULL, NULL, &i2_p1_payload_read_len));
    TEST_ASSERT_EQUAL(I2_P1_PAYLOAD_LEN, i2_p1_payload_read_len);
    TEST_ASSERT_EQUAL(ASTARTE_OK,
        astarte_storage_load_property(astarte_storage_handle, I2_INAME, I2_P1_PNAME, &i2_major_read,
            i2_p1_payload_read, &i2_p1_payload_read_len));
    TEST_ASSERT_EQUAL(I2_P1_PAYLOAD_LEN, i2_p1_payload_read_len);
    TEST_ASSERT_EQUAL(I2_IMAJOR, i2_major_read);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(i2_p1_payload, i2_p1_payload_read, i2_p1_payload_read_len);

    TEST_ASSERT_EQUAL(ASTARTE_ERR_NOT_FOUND,
        astarte_storage_load_property(
            astarte_storage_handle, I2_INAME, I2_P2_PNAME, NULL, NULL, &i2_p2_payload_read_len));

    // Delete property
    TEST_ASSERT_EQUAL(
        ASTARTE_OK, astarte_storage_delete_property(astarte_storage_handle, I1_INAME, I1_P2_PNAME));

    // Load deleted property
    TEST_ASSERT_EQUAL(ASTARTE_OK,
        astarte_storage_load_property(
            astarte_storage_handle, I1_INAME, I1_P1_PNAME, NULL, NULL, &i1_p1_payload_read_len));
    TEST_ASSERT_EQUAL(I1_P1_PAYLOAD_LEN, i1_p1_payload_read_len);
    TEST_ASSERT_EQUAL(ASTARTE_OK,
        astarte_storage_load_property(astarte_storage_handle, I1_INAME, I1_P1_PNAME, &i1_major_read,
            i1_p1_payload_read, &i1_p1_payload_read_len));
    TEST_ASSERT_EQUAL(I1_IMAJOR, i1_major_read);
    TEST_ASSERT_EQUAL(I1_P1_PAYLOAD_LEN, i1_p1_payload_read_len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(i1_p1_payload, i1_p1_payload_read, i1_p1_payload_read_len);

    TEST_ASSERT_EQUAL(ASTARTE_ERR_NOT_FOUND,
        astarte_storage_load_property(
            astarte_storage_handle, I1_INAME, I1_P2_PNAME, NULL, NULL, &i1_p2_payload_read_len));

    TEST_ASSERT_EQUAL(ASTARTE_OK,
        astarte_storage_load_property(
            astarte_storage_handle, I2_INAME, I2_P1_PNAME, NULL, NULL, &i2_p1_payload_read_len));
    TEST_ASSERT_EQUAL(I2_P1_PAYLOAD_LEN, i2_p1_payload_read_len);
    TEST_ASSERT_EQUAL(ASTARTE_OK,
        astarte_storage_load_property(astarte_storage_handle, I2_INAME, I2_P1_PNAME, &i2_major_read,
            i2_p1_payload_read, &i2_p1_payload_read_len));
    TEST_ASSERT_EQUAL(I2_IMAJOR, i2_major_read);
    TEST_ASSERT_EQUAL(I2_P1_PAYLOAD_LEN, i2_p1_payload_read_len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(i2_p1_payload, i2_p1_payload_read, i2_p1_payload_read_len);

    TEST_ASSERT_EQUAL(ASTARTE_ERR_NOT_FOUND,
        astarte_storage_load_property(
            astarte_storage_handle, I2_INAME, I2_P2_PNAME, NULL, NULL, &i2_p2_payload_read_len));

    // Delete property
    TEST_ASSERT_EQUAL(
        ASTARTE_OK, astarte_storage_delete_property(astarte_storage_handle, I1_INAME, I1_P1_PNAME));

    // Load deleted property
    TEST_ASSERT_EQUAL(ASTARTE_ERR_NOT_FOUND,
        astarte_storage_load_property(
            astarte_storage_handle, I1_INAME, I1_P1_PNAME, NULL, NULL, &i1_p1_payload_read_len));

    TEST_ASSERT_EQUAL(ASTARTE_ERR_NOT_FOUND,
        astarte_storage_load_property(
            astarte_storage_handle, I1_INAME, I1_P2_PNAME, NULL, NULL, &i1_p2_payload_read_len));

    TEST_ASSERT_EQUAL(ASTARTE_OK,
        astarte_storage_load_property(
            astarte_storage_handle, I2_INAME, I2_P1_PNAME, NULL, NULL, &i2_p1_payload_read_len));
    TEST_ASSERT_EQUAL(I2_P1_PAYLOAD_LEN, i2_p1_payload_read_len);
    TEST_ASSERT_EQUAL(ASTARTE_OK,
        astarte_storage_load_property(astarte_storage_handle, I2_INAME, I2_P1_PNAME, &i2_major_read,
            i2_p1_payload_read, &i2_p1_payload_read_len));
    TEST_ASSERT_EQUAL(I2_IMAJOR, i2_major_read);
    TEST_ASSERT_EQUAL(I2_P1_PAYLOAD_LEN, i2_p1_payload_read_len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(i2_p1_payload, i2_p1_payload_read, i2_p1_payload_read_len);

    TEST_ASSERT_EQUAL(ASTARTE_ERR_NOT_FOUND,
        astarte_storage_load_property(
            astarte_storage_handle, I2_INAME, I2_P2_PNAME, NULL, NULL, &i2_p2_payload_read_len));

    // Delete property
    TEST_ASSERT_EQUAL(
        ASTARTE_OK, astarte_storage_delete_property(astarte_storage_handle, I2_INAME, I2_P1_PNAME));

    // Load deleted property
    TEST_ASSERT_EQUAL(ASTARTE_ERR_NOT_FOUND,
        astarte_storage_load_property(
            astarte_storage_handle, I1_INAME, I1_P1_PNAME, NULL, NULL, &i1_p1_payload_read_len));

    TEST_ASSERT_EQUAL(ASTARTE_ERR_NOT_FOUND,
        astarte_storage_load_property(
            astarte_storage_handle, I1_INAME, I1_P2_PNAME, NULL, NULL, &i1_p2_payload_read_len));

    TEST_ASSERT_EQUAL(ASTARTE_ERR_NOT_FOUND,
        astarte_storage_load_property(
            astarte_storage_handle, I2_INAME, I2_P1_PNAME, NULL, NULL, &i2_p1_payload_read_len));

    TEST_ASSERT_EQUAL(ASTARTE_ERR_NOT_FOUND,
        astarte_storage_load_property(
            astarte_storage_handle, I2_INAME, I2_P2_PNAME, NULL, NULL, &i2_p2_payload_read_len));

    // Store property
    TEST_ASSERT_EQUAL(ASTARTE_OK,
        astarte_storage_store_property(astarte_storage_handle, I2_INAME, I2_P1_PNAME, I2_IMAJOR,
            i2_p1_payload, I2_P1_PAYLOAD_LEN));

    // Load deleted property
    TEST_ASSERT_EQUAL(ASTARTE_ERR_NOT_FOUND,
        astarte_storage_load_property(
            astarte_storage_handle, I1_INAME, I1_P1_PNAME, NULL, NULL, &i1_p1_payload_read_len));

    TEST_ASSERT_EQUAL(ASTARTE_ERR_NOT_FOUND,
        astarte_storage_load_property(
            astarte_storage_handle, I1_INAME, I1_P2_PNAME, NULL, NULL, &i1_p2_payload_read_len));

    TEST_ASSERT_EQUAL(ASTARTE_OK,
        astarte_storage_load_property(
            astarte_storage_handle, I2_INAME, I2_P1_PNAME, NULL, NULL, &i2_p1_payload_read_len));
    TEST_ASSERT_EQUAL(I2_P1_PAYLOAD_LEN, i2_p1_payload_read_len);
    TEST_ASSERT_EQUAL(ASTARTE_OK,
        astarte_storage_load_property(astarte_storage_handle, I2_INAME, I2_P1_PNAME, &i2_major_read,
            i2_p1_payload_read, &i2_p1_payload_read_len));
    TEST_ASSERT_EQUAL(I2_IMAJOR, i2_major_read);
    TEST_ASSERT_EQUAL(I2_P1_PAYLOAD_LEN, i2_p1_payload_read_len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(i2_p1_payload, i2_p1_payload_read, i2_p1_payload_read_len);

    TEST_ASSERT_EQUAL(ASTARTE_ERR_NOT_FOUND,
        astarte_storage_load_property(
            astarte_storage_handle, I2_INAME, I2_P2_PNAME, NULL, NULL, &i2_p2_payload_read_len));

    // Close storage
    astarte_storage_close(astarte_storage_handle);
}

void test_astarte_storage_contains(void)
{
    // Prepare device by erasing default nvs partition
    TEST_ASSERT_EQUAL(ESP_OK, nvs_flash_erase());
    // Prepare device by initializing default nvs partition
    TEST_ASSERT_EQUAL(ESP_OK, nvs_flash_init());

    // Variable where to store result
    bool result = 0;

    // Open storage
    astarte_storage_handle_t astarte_storage_handle;
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_storage_open(&astarte_storage_handle));

    // Check if property is contained in empty storage
    TEST_ASSERT_EQUAL(ASTARTE_OK,
        astarte_storage_contains_property(astarte_storage_handle, I1_INAME, I1_P1_PNAME, I1_IMAJOR,
            i1_p1_payload, I1_P1_PAYLOAD_LEN, &result));
    TEST_ASSERT_FALSE(result);

    // Store property
    TEST_ASSERT_EQUAL(ASTARTE_OK,
        astarte_storage_store_property(astarte_storage_handle, I1_INAME, I1_P1_PNAME, I1_IMAJOR,
            i1_p1_payload, I1_P1_PAYLOAD_LEN));

    // Check if property is contained in storage
    TEST_ASSERT_EQUAL(ASTARTE_OK,
        astarte_storage_contains_property(astarte_storage_handle, I1_INAME, I1_P1_PNAME, I1_IMAJOR,
            i1_p1_payload, I1_P1_PAYLOAD_LEN, &result));
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(ASTARTE_OK,
        astarte_storage_contains_property(astarte_storage_handle, I1_INAME, I1_P2_PNAME, I1_IMAJOR,
            i1_p2_payload, I1_P2_PAYLOAD_LEN, &result));
    TEST_ASSERT_FALSE(result);

    // Store another property
    TEST_ASSERT_EQUAL(ASTARTE_OK,
        astarte_storage_store_property(astarte_storage_handle, I1_INAME, I1_P1_PNAME, I1_IMAJOR,
            i1_p2_payload, I1_P2_PAYLOAD_LEN));

    // Check if property is contained in storage
    TEST_ASSERT_EQUAL(ASTARTE_OK,
        astarte_storage_contains_property(astarte_storage_handle, I1_INAME, I1_P1_PNAME, I1_IMAJOR,
            i1_p1_payload, I1_P1_PAYLOAD_LEN, &result));
    TEST_ASSERT_FALSE(result);

    // Store property already stored but with different content
    TEST_ASSERT_EQUAL(ASTARTE_OK,
        astarte_storage_store_property(astarte_storage_handle, I1_INAME, I1_P1_PNAME, I1_IMAJOR,
            i2_p2_payload, I2_P2_PAYLOAD_LEN));

    // Check if property is contained in storage with old content
    TEST_ASSERT_EQUAL(ASTARTE_OK,
        astarte_storage_contains_property(astarte_storage_handle, I1_INAME, I1_P1_PNAME, I1_IMAJOR,
            i1_p1_payload, I1_P1_PAYLOAD_LEN, &result));
    TEST_ASSERT_FALSE(result);

    // Check if property is contained in storage with new content
    TEST_ASSERT_EQUAL(ASTARTE_OK,
        astarte_storage_contains_property(astarte_storage_handle, I1_INAME, I1_P1_PNAME, I1_IMAJOR,
            i2_p2_payload, I2_P2_PAYLOAD_LEN, &result));
    TEST_ASSERT_TRUE(result);

    // Check if property is contained in storage with new content also deletes the property
    TEST_ASSERT_EQUAL(ASTARTE_OK,
        astarte_storage_contains_property(astarte_storage_handle, I1_INAME, I1_P1_PNAME,
            I1_IMAJOR + 1, i2_p2_payload, I2_P2_PAYLOAD_LEN, &result));
    TEST_ASSERT_FALSE(result);

    // Property with different major has been deleted
    size_t payload_len = 0;
    TEST_ASSERT_EQUAL(ASTARTE_ERR_NOT_FOUND,
        astarte_storage_load_property(
            astarte_storage_handle, I1_INAME, I1_P1_PNAME, NULL, NULL, &payload_len));

    // Close storage
    astarte_storage_close(astarte_storage_handle);
}

void test_astarte_storage_clear(void)
{
    // Prepare device by erasing default nvs partition
    TEST_ASSERT_EQUAL(ESP_OK, nvs_flash_erase());
    // Prepare device by initializing default nvs partition
    TEST_ASSERT_EQUAL(ESP_OK, nvs_flash_init());

    // Variables where to store read payloads
    int32_t i2_major_read = 0;
    size_t i1_p1_payload_read_len = 0;
    size_t i1_p2_payload_read_len = 0;
    size_t i2_p1_payload_read_len = 0;
    uint8_t i2_p1_payload_read[I2_P1_PAYLOAD_LEN] = { 0 };
    size_t i2_p2_payload_read_len = 0;

    // Open storage
    astarte_storage_handle_t astarte_storage_handle;
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_storage_open(&astarte_storage_handle));

    // Clear storage with no property
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_storage_clear(astarte_storage_handle));

    // Store property
    TEST_ASSERT_EQUAL(ASTARTE_OK,
        astarte_storage_store_property(astarte_storage_handle, I1_INAME, I1_P1_PNAME, I1_IMAJOR,
            i1_p1_payload, I1_P1_PAYLOAD_LEN));
    TEST_ASSERT_EQUAL(ASTARTE_OK,
        astarte_storage_store_property(astarte_storage_handle, I1_INAME, I1_P2_PNAME, I1_IMAJOR,
            i1_p2_payload, I1_P2_PAYLOAD_LEN));
    TEST_ASSERT_EQUAL(ASTARTE_OK,
        astarte_storage_store_property(astarte_storage_handle, I2_INAME, I2_P1_PNAME, I2_IMAJOR,
            i2_p1_payload, I2_P1_PAYLOAD_LEN));

    // Clear storage
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_storage_clear(astarte_storage_handle));

    // Load deleted property
    TEST_ASSERT_EQUAL(ASTARTE_ERR_NOT_FOUND,
        astarte_storage_load_property(
            astarte_storage_handle, I1_INAME, I1_P1_PNAME, NULL, NULL, &i1_p1_payload_read_len));

    TEST_ASSERT_EQUAL(ASTARTE_ERR_NOT_FOUND,
        astarte_storage_load_property(
            astarte_storage_handle, I1_INAME, I1_P2_PNAME, NULL, NULL, &i1_p2_payload_read_len));

    TEST_ASSERT_EQUAL(ASTARTE_ERR_NOT_FOUND,
        astarte_storage_load_property(
            astarte_storage_handle, I2_INAME, I2_P1_PNAME, NULL, NULL, &i2_p1_payload_read_len));

    TEST_ASSERT_EQUAL(ASTARTE_ERR_NOT_FOUND,
        astarte_storage_load_property(
            astarte_storage_handle, I2_INAME, I2_P2_PNAME, NULL, NULL, &i2_p2_payload_read_len));

    // Store property
    TEST_ASSERT_EQUAL(ASTARTE_OK,
        astarte_storage_store_property(astarte_storage_handle, I2_INAME, I2_P1_PNAME, I2_IMAJOR,
            i2_p1_payload, I2_P1_PAYLOAD_LEN));

    // Load deleted property
    TEST_ASSERT_EQUAL(ASTARTE_ERR_NOT_FOUND,
        astarte_storage_load_property(
            astarte_storage_handle, I1_INAME, I1_P1_PNAME, NULL, NULL, &i1_p1_payload_read_len));

    TEST_ASSERT_EQUAL(ASTARTE_ERR_NOT_FOUND,
        astarte_storage_load_property(
            astarte_storage_handle, I1_INAME, I1_P2_PNAME, NULL, NULL, &i1_p2_payload_read_len));

    TEST_ASSERT_EQUAL(ASTARTE_OK,
        astarte_storage_load_property(astarte_storage_handle, I2_INAME, I2_P1_PNAME, &i2_major_read,
            NULL, &i2_p1_payload_read_len));
    TEST_ASSERT_EQUAL(I2_IMAJOR, i2_major_read);
    TEST_ASSERT_EQUAL(I2_P1_PAYLOAD_LEN, i2_p1_payload_read_len);
    TEST_ASSERT_EQUAL(ASTARTE_OK,
        astarte_storage_load_property(astarte_storage_handle, I2_INAME, I2_P1_PNAME, &i2_major_read,
            i2_p1_payload_read, &i2_p1_payload_read_len));
    TEST_ASSERT_EQUAL(I2_IMAJOR, i2_major_read);
    TEST_ASSERT_EQUAL(I2_P1_PAYLOAD_LEN, i2_p1_payload_read_len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(i2_p1_payload, i2_p1_payload_read, i2_p1_payload_read_len);

    TEST_ASSERT_EQUAL(ASTARTE_ERR_NOT_FOUND,
        astarte_storage_load_property(
            astarte_storage_handle, I2_INAME, I2_P2_PNAME, NULL, NULL, &i2_p2_payload_read_len));

    // Close storage
    astarte_storage_close(astarte_storage_handle);
}

void test_astarte_storage_iteration(void)
{
    int32_t major = 0;
    // Prepare device by erasing default nvs partition
    TEST_ASSERT_EQUAL(ESP_OK, nvs_flash_erase());
    // Prepare device by initializing default nvs partition
    TEST_ASSERT_EQUAL(ESP_OK, nvs_flash_init());

    // Open storage
    astarte_storage_handle_t astarte_storage_handle;
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_storage_open(&astarte_storage_handle));

    // Store properties
    TEST_ASSERT_EQUAL(ASTARTE_OK,
        astarte_storage_store_property(astarte_storage_handle, I1_INAME, I1_P1_PNAME, I1_IMAJOR,
            i1_p1_payload, I1_P1_PAYLOAD_LEN));
    TEST_ASSERT_EQUAL(ASTARTE_OK,
        astarte_storage_store_property(astarte_storage_handle, I1_INAME, I1_P2_PNAME, I1_IMAJOR,
            i1_p2_payload, I1_P2_PAYLOAD_LEN));
    TEST_ASSERT_EQUAL(ASTARTE_OK,
        astarte_storage_store_property(astarte_storage_handle, I2_INAME, I2_P1_PNAME, I2_IMAJOR,
            i2_p1_payload, I2_P1_PAYLOAD_LEN));

    // Create iterator
    astarte_storage_iterator_t astarte_storage_iterator;
    TEST_ASSERT_EQUAL(ASTARTE_OK,
        astarte_storage_iterator_create(astarte_storage_handle, &astarte_storage_iterator));

    // Fetch first property
    size_t interface_name_len, path_len, value_len;
    TEST_ASSERT_EQUAL(ASTARTE_OK,
        astarte_storage_iterator_get_property(&astarte_storage_iterator, NULL, &interface_name_len,
            NULL, &path_len, &major, NULL, &value_len));
    TEST_ASSERT_EQUAL(17, interface_name_len);
    TEST_ASSERT_EQUAL(8, path_len);
    TEST_ASSERT_EQUAL(I1_P1_PAYLOAD_LEN, value_len);

    char i1_p1_interface_name_read[17];
    char i1_p1_path_read[8];
    uint8_t i1_p1_payload_read[I1_P1_PAYLOAD_LEN] = { 0 };
    TEST_ASSERT_EQUAL(ASTARTE_OK,
        astarte_storage_iterator_get_property(&astarte_storage_iterator, i1_p1_interface_name_read,
            &interface_name_len, i1_p1_path_read, &path_len, &major, i1_p1_payload_read,
            &value_len));

    // Fetch next property
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_storage_iterator_advance(&astarte_storage_iterator));
    TEST_ASSERT_EQUAL(ASTARTE_OK,
        astarte_storage_iterator_get_property(&astarte_storage_iterator, NULL, &interface_name_len,
            NULL, &path_len, &major, NULL, &value_len));
    TEST_ASSERT_EQUAL(17, interface_name_len);
    TEST_ASSERT_EQUAL(8, path_len);
    TEST_ASSERT_EQUAL(I1_P2_PAYLOAD_LEN, value_len);

    char i1_p2_interface_name_read[17];
    char i1_p2_path_read[8];
    uint8_t i1_p2_payload_read[I1_P2_PAYLOAD_LEN] = { 0 };
    TEST_ASSERT_EQUAL(ASTARTE_OK,
        astarte_storage_iterator_get_property(&astarte_storage_iterator, i1_p2_interface_name_read,
            &interface_name_len, i1_p2_path_read, &path_len, &major, i1_p2_payload_read,
            &value_len));

    // Fetch next property
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_storage_iterator_advance(&astarte_storage_iterator));
    TEST_ASSERT_EQUAL(ASTARTE_OK,
        astarte_storage_iterator_get_property(&astarte_storage_iterator, NULL, &interface_name_len,
            NULL, &path_len, &major, NULL, &value_len));
    TEST_ASSERT_EQUAL(17, interface_name_len);
    TEST_ASSERT_EQUAL(8, path_len);
    TEST_ASSERT_EQUAL(I2_P1_PAYLOAD_LEN, value_len);

    char i2_p1_interface_name_read[17];
    char i2_p1_path_read[8];
    uint8_t i2_p1_payload_read[I2_P1_PAYLOAD_LEN] = { 0 };
    TEST_ASSERT_EQUAL(ASTARTE_OK,
        astarte_storage_iterator_get_property(&astarte_storage_iterator, i2_p1_interface_name_read,
            &interface_name_len, i2_p1_path_read, &path_len, &major, i2_p1_payload_read,
            &value_len));

    // Fetch next property
    TEST_ASSERT_EQUAL(
        ASTARTE_ERR_NOT_FOUND, astarte_storage_iterator_advance(&astarte_storage_iterator));

    // Close storage
    astarte_storage_close(astarte_storage_handle);
}

void test_astarte_storage_iteration_empty_memory(void)
{
    // Prepare device by erasing default nvs partition
    TEST_ASSERT_EQUAL(ESP_OK, nvs_flash_erase());
    // Prepare device by initializing default nvs partition
    TEST_ASSERT_EQUAL(ESP_OK, nvs_flash_init());

    // Open storage
    astarte_storage_handle_t astarte_storage_handle;
    TEST_ASSERT_EQUAL(ASTARTE_OK, astarte_storage_open(&astarte_storage_handle));

    // Create iterator
    astarte_storage_iterator_t astarte_storage_iterator;
    TEST_ASSERT_EQUAL(ASTARTE_ERR_NOT_FOUND,
        astarte_storage_iterator_create(astarte_storage_handle, &astarte_storage_iterator));

    // Close storage
    astarte_storage_close(astarte_storage_handle);
}
