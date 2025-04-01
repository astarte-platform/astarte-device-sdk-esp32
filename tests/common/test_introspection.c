/*
 * (C) Copyright 2025, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "unity.h"

#include "introspection.h"
#include "test_introspection.h"

#include "interfaces.h"

#include <esp_log.h>

#define TAG "INTROSPECTION TEST"

void test_introspection_creation(void)
{
    introspection_t introspection = introspection_new();
    introspection_free(introspection);
}

void test_introspection_add_get_update(void)
{
    introspection_t introspection = introspection_new();

    TEST_ASSERT_NULL(introspection_get(&introspection, test_interface1.name));
    TEST_ASSERT_NULL(introspection_get(&introspection, test_interface2.name));
    TEST_ASSERT_NULL(introspection_get(&introspection, test_interface3.name));

    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK, introspection_add(&introspection, &test_interface1));

    TEST_ASSERT_EQUAL_PTR(
        &test_interface1, introspection_get(&introspection, test_interface1.name));
    TEST_ASSERT_NULL(introspection_get(&introspection, test_interface2.name));
    TEST_ASSERT_NULL(introspection_get(&introspection, test_interface3.name));

    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK, introspection_add(&introspection, &test_interface2));
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK, introspection_add(&introspection, &test_interface3));

    TEST_ASSERT_EQUAL_PTR(
        &test_interface1, introspection_get(&introspection, test_interface1.name));
    TEST_ASSERT_EQUAL_PTR(
        &test_interface2, introspection_get(&introspection, test_interface2.name));
    TEST_ASSERT_EQUAL_PTR(
        &test_interface3, introspection_get(&introspection, test_interface3.name));

    TEST_ASSERT_EQUAL(ASTARTE_RESULT_INTERFACE_ALREADY_PRESENT,
        introspection_add(&introspection, &test_interface2));

    TEST_ASSERT_EQUAL_PTR(
        &test_interface2, introspection_get(&introspection, test_interface2.name));

    TEST_ASSERT_EQUAL(
        ASTARTE_RESULT_OK, introspection_update(&introspection, &test_interface2_substitute));

    TEST_ASSERT_EQUAL_PTR(&test_interface2_substitute,
        introspection_get(&introspection, test_interface2_substitute.name));

    introspection_free(introspection);
}

void test_introspection_get_string(void)
{
    char expected_string[] = "test.interface1:0:1;test.interface2:0:1;test.interface3:0:1";
    introspection_t introspection = introspection_new();

    TEST_ASSERT_EQUAL(1U, introspection_get_string_size(&introspection));

    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK, introspection_add(&introspection, &test_interface1));
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK, introspection_add(&introspection, &test_interface2));
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK, introspection_add(&introspection, &test_interface3));

    TEST_ASSERT_EQUAL(sizeof(expected_string), introspection_get_string_size(&introspection));

    char string[sizeof(expected_string)] = { 0U };
    introspection_fill_string(&introspection, string, sizeof(expected_string));
    TEST_ASSERT_EQUAL_STRING(expected_string, string);

    introspection_free(introspection);
}

void test_introspection_iterator(void)
{
    introspection_t introspection = introspection_new();

    introspection_iterator_t iterator_1;
    TEST_ASSERT_EQUAL(
        ASTARTE_RESULT_NOT_FOUND, introspection_iterator_init(&introspection, &iterator_1));

    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK, introspection_add(&introspection, &test_interface1));
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK, introspection_add(&introspection, &test_interface2));
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK, introspection_add(&introspection, &test_interface3));

    introspection_iterator_t iterator_2;
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK, introspection_iterator_init(&introspection, &iterator_2));

    TEST_ASSERT_EQUAL_PTR(&test_interface1, introspection_iterator_get_interface(&iterator_2));
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK, introspection_iterator_advance(&iterator_2));
    TEST_ASSERT_EQUAL_PTR(&test_interface2, introspection_iterator_get_interface(&iterator_2));
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK, introspection_iterator_advance(&iterator_2));
    TEST_ASSERT_EQUAL_PTR(&test_interface3, introspection_iterator_get_interface(&iterator_2));
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_NOT_FOUND, introspection_iterator_advance(&iterator_2));

    introspection_free(introspection);
}
