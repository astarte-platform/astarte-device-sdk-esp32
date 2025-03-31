/*
 * (C) Copyright 2025, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_device_id.h"
#include "astarte_device_sdk/device_id.h"
#include "unity.h"

#include <esp_log.h>

#define TAG "DEVICE ID TEST"

void test_device_id_generate_deterministic(void)
{
    const uint8_t namespace[] = { 0x05, 0x75, 0xa5, 0x69, 0x51, 0xeb, 0x57, 0x5c, 0xaf, 0xe4, 0xce,
        0x7f, 0xc0, 0x3b, 0xcd, 0xc5 };
    const char name[] = "Hello world";
    char out[ASTARTE_DEVICE_ID_LEN + 1] = { 0 };
    TEST_ASSERT_EQUAL(ASTARTE_RESULT_OK,
        astarte_device_id_generate_deterministic(
            namespace, (const uint8_t *) name, sizeof(name), out));
    TEST_ASSERT_EQUAL_STRING("3lkQQ8UyWIakgHOqOoCzwA", out);
}
