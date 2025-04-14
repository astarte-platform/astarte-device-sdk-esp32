/*
 * (C) Copyright 2024-2025, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LOG_H
#define LOG_H

/**
 * @file log.h
 * @brief Wrapper for the log module.
 */

#include <esp_log.h>

/** @brief Defines a logging tag for a module to use. */
#define ASTARTE_LOG_MODULE_REGISTER(name) static const char *log_tag = (name)

/** @brief Defines a logging tag for a module to use. */
#define ASTARTE_LOG_MODULE_DECLARE(name) ASTARTE_LOG_MODULE_REGISTER(name)

/** @brief Wrapper for the ESP_LOGD macro. */
#define ASTARTE_LOG_DBG(...) ESP_LOGD(log_tag, __VA_ARGS__)

/** @brief Wrapper for the ESP_LOGI macro. */
#define ASTARTE_LOG_INF(...) ESP_LOGI(log_tag, __VA_ARGS__)

/** @brief Wrapper for the ESP_LOGW macro. */
#define ASTARTE_LOG_WRN(...) ESP_LOGW(log_tag, __VA_ARGS__)

/** @brief Wrapper for the ESP_LOGE macro. */
#define ASTARTE_LOG_ERR(...) ESP_LOGE(log_tag, __VA_ARGS__)

#endif // LOG_H
