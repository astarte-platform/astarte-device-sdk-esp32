/*
 * (C) Copyright 2025, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BACKOFF_H
#define BACKOFF_H

/**
 * @file backoff.h
 * @brief Utility to be used to generate an exponential backoff.
 *
 * @details Each backoff context can be utilized to generate a series of backoff periods.
 * Use #backoff_context_init to initialize a new context and repeatedly call
 * #backoff_get_next to get the next period. Each context can be optionally configured to use
 * jitter.
 *
 * See
 * [exponential-backoff-and-jitter](https://aws.amazon.com/blogs/architecture/exponential-backoff-and-jitter/)
 * for more details.
 */

#include "astarte_device_sdk/astarte.h"

/**
 * @brief Backoff context.
 */
struct backoff_context
{
    /** @brief Maximum backoff for the next attempt. */
    uint32_t attempt_max_backoff_ms;
    /** @brief Maximum overall backoff. */
    uint32_t max_backoff_ms;
    /** @brief Enables jitter for the backoff context. */
    bool enable_jitter;
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize a backoff context.
 *
 * @param[out] ctx Backoff context to intialize.
 * @param[in] base_backoff_ms First backoff duration.
 * @param[in] max_backoff_ms Maximum backoff duration.
 * @param[in] enable_jitter Enable jitter.
 */
void backoff_context_init(struct backoff_context *ctx, uint32_t base_backoff_ms,
    uint32_t max_backoff_ms, bool enable_jitter);

/**
 * @brief Get next backoff duration.
 *
 * @param[in] ctx Backoff context to use.
 * @param[out] next_backoff_ms Next backoff period.
 */
void backoff_get_next(struct backoff_context *ctx, uint32_t *next_backoff_ms);

#ifdef __cplusplus
}
#endif

#endif // BACKOFF_H
