/*
 * (C) Copyright 2025, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "unity.h"

#include "backoff.h"
#include "test_backoff.h"

#include <esp_log.h>

#define TAG "utest-backoff"

void test_backoff_no_jitter(void)
{
    struct backoff_context ctx = { 0 };
    uint32_t next_backoff_ms = 0U;

    const uint32_t base_backoff_ms = 500;
    const uint32_t maximum_backoff_ms = 5000;
    backoff_context_init(&ctx, base_backoff_ms, maximum_backoff_ms, false);

    backoff_get_next(&ctx, &next_backoff_ms);
    TEST_ASSERT_EQUAL(base_backoff_ms, next_backoff_ms);

    backoff_get_next(&ctx, &next_backoff_ms);
    TEST_ASSERT_EQUAL(base_backoff_ms * 2, next_backoff_ms);

    backoff_get_next(&ctx, &next_backoff_ms);
    TEST_ASSERT_EQUAL(base_backoff_ms * 4, next_backoff_ms);

    backoff_get_next(&ctx, &next_backoff_ms);
    TEST_ASSERT_EQUAL(base_backoff_ms * 8, next_backoff_ms);

    backoff_get_next(&ctx, &next_backoff_ms);
    TEST_ASSERT_EQUAL(maximum_backoff_ms, next_backoff_ms);

    backoff_get_next(&ctx, &next_backoff_ms);
    TEST_ASSERT_EQUAL(maximum_backoff_ms, next_backoff_ms);
}
