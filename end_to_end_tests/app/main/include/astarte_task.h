/*
 * (C) Copyright 2024-2025, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ASTARTE_TASK_H
#define ASTARTE_TASK_H

#include <esp_err.h>

/**
 * @brief Configure the console to be used to control the test execution.
 */
esp_err_t astarte_task_console_start(void);

/**
 * @brief Astarte sample task entry point.
 *
 * @param ctx ESP 32 task context.
 */
void astarte_task_entry(void *ctx);

#endif // ASTARTE_TASK_H
