/*
 * (C) Copyright 2024-2025, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <esp_event.h>
#include <esp_log.h>
#include <esp_netif.h>
#include <inttypes.h>
#include <nvs_flash.h>

#include <freertos/FreeRTOS.h> // NOLINT Circular header file dependencies is an idf problem
#include <freertos/event_groups.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include "astarte_task.h"
#include "protocol_examples_common.h"

/************************************************
 * Constants/Defines
 ***********************************************/

#define TAG "astarte-end-to-end-main"

#define ASTARTE_SAMPLE_TASK_STACK_SIZE 65536

/************************************************
 * Main function definition
 ***********************************************/

void app_main()
{
    esp_log_level_set("astarte-bson-deserializer", ESP_LOG_DEBUG);
    esp_log_level_set("astarte-bson-serializer", ESP_LOG_DEBUG);
    esp_log_level_set("astarte-crypto", ESP_LOG_DEBUG);
    esp_log_level_set("astarte-data-validation", ESP_LOG_DEBUG);
    esp_log_level_set("astarte-data", ESP_LOG_DEBUG);
    esp_log_level_set("astarte-device-caching", ESP_LOG_DEBUG);
    esp_log_level_set("astarte-device-connection", ESP_LOG_DEBUG);
    esp_log_level_set("astarte-device-id", ESP_LOG_DEBUG);
    esp_log_level_set("astarte-device-rx", ESP_LOG_DEBUG);
    esp_log_level_set("astarte-device-tx", ESP_LOG_DEBUG);
    esp_log_level_set("astarte-device", ESP_LOG_DEBUG);
    esp_log_level_set("astarte-dlist", ESP_LOG_DEBUG);
    esp_log_level_set("astarte-http", ESP_LOG_DEBUG);
    esp_log_level_set("astarte-interface", ESP_LOG_DEBUG);
    esp_log_level_set("astarte-introspection", ESP_LOG_DEBUG);
    esp_log_level_set("astarte-kv-storage", ESP_LOG_DEBUG);
    esp_log_level_set("astarte-mapping", ESP_LOG_DEBUG);
    esp_log_level_set("astarte-object", ESP_LOG_DEBUG);
    esp_log_level_set("astarte-pairing", ESP_LOG_DEBUG);
    esp_log_level_set("astarte-uuid", ESP_LOG_DEBUG);

    esp_err_t esp_err = ESP_OK;
    ESP_LOGI(TAG, "Startup..");
    ESP_LOGI(TAG, "Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "IDF version: %s", esp_get_idf_version());

    esp_err = nvs_flash_init();
    if (esp_err != ESP_OK) {
        ESP_LOGE(TAG, "Error initializing default NVS partition: %s.", esp_err_to_name(esp_err));
        goto exit;
    }
    esp_err = esp_netif_init();
    if (esp_err != ESP_OK) {
        ESP_LOGE(TAG, "Error initializing network interfaces: %s.", esp_err_to_name(esp_err));
        goto exit;
    }
    esp_err = esp_event_loop_create_default();
    if (esp_err != ESP_OK) {
        ESP_LOGE(TAG, "Error creating default event loop: %s.", esp_err_to_name(esp_err));
        goto exit;
    }
    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    esp_err = example_connect();
    if (esp_err != ESP_OK) {
        ESP_LOGE(TAG, "Error connecting to the network: %s.", esp_err_to_name(esp_err));
        goto exit;
    }

    esp_err = astarte_task_console_start();
    if (esp_err != ESP_OK) {
        ESP_LOGE(TAG, "Error initializing the console: %s.", esp_err_to_name(esp_err));
        goto exit;
    }

    const configSTACK_DEPTH_TYPE stack_depth = ASTARTE_SAMPLE_TASK_STACK_SIZE;
    xTaskCreate(
        astarte_task_entry, "astarte_task_entry", stack_depth, NULL, tskIDLE_PRIORITY, NULL);

exit:
    vTaskDelete(NULL);
}
