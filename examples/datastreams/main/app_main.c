#include <esp_idf_version.h>
#include <esp_log.h>
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

#define TAG "ASTARTE_EXAMPLE_APP_MAIN"

/************************************************
 * Main function definition
 ***********************************************/

void app_main()
{
    ESP_LOGI(TAG, "[APP] Startup..");
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    ESP_LOGI(TAG, "[APP] Free memory: %ld bytes", esp_get_free_heap_size());
#else
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
#endif
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);

    nvs_flash_init();
    wifi_init();
    xTaskCreate(astarte_example_task, "astarte_example_task", 6000, NULL, tskIDLE_PRIORITY, NULL);
}
