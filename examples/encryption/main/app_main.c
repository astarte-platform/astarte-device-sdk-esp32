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

#include <esp_log.h>
#include <inttypes.h>
#include <nvs_flash.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "example_task.h"
#include "wifi_cfg.h"

/************************************************
 * Constants/Defines
 ***********************************************/

#define TAG "NVS_EXAMPLE_APP_MAIN"

/************************************************
 * Main function definition
 ***********************************************/

void app_main()
{
    ESP_LOGI(TAG, "Startup..");
    ESP_LOGI(TAG, "Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);

    nvs_flash_init();
    wifi_init();

    const configSTACK_DEPTH_TYPE stack_depth = 6000;
    xTaskCreate(
        astarte_example_task, "astarte_example_task", stack_depth, NULL, tskIDLE_PRIORITY, NULL);
}
