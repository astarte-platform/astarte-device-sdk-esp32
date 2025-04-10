/*
 * (C) Copyright 2024-2025, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <esp_log.h>
#include <inttypes.h>
#include <nvs_flash.h>

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include "astarte_task.h"
#include "wifi_cfg.h"

/************************************************
 * Constants/Defines
 ***********************************************/

#define TAG "ASTARTE SAMPLE APP MAIN"

#define ASTARTE_SAMPLE_TASK_STACK_SIZE 32768

/************************************************
 * Main function definition
 ***********************************************/

void app_main()
{
    esp_err_t esp_err = ESP_OK;
    ESP_LOGI(TAG, "Startup..");
    ESP_LOGI(TAG, "Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);

    esp_err = nvs_flash_init();
    if (esp_err != ESP_OK) {
        ESP_LOGE(TAG, "Error initializing default NVS partition: %s.", esp_err_to_name(esp_err));
        goto exit;
    }
    wifi_init();

    const configSTACK_DEPTH_TYPE stack_depth = ASTARTE_SAMPLE_TASK_STACK_SIZE;
    xTaskCreate(
        astarte_task_entry, "astarte_task_entry", stack_depth, NULL, tskIDLE_PRIORITY, NULL);

exit:
    vTaskDelete(NULL);
}
