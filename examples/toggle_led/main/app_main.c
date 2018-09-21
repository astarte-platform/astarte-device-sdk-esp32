#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_event_loop.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "esp_vfs.h"
#include "esp_vfs_fat.h"

#include "astarte_credentials.h"
#include "astarte_hwid.h"
#include "astarte_pairing.h"

#define TAG "ASTARTE_TOGGLE_LED"

#define BASE_PATH "/spiflash"

static wl_handle_t s_wl_handle = WL_INVALID_HANDLE;
static EventGroupHandle_t wifi_event_group;
const static int CONNECTED_BIT = BIT0;

static esp_err_t wifi_event_handler(void *ctx, system_event_t *event)
{
    switch (event->event_id) {
        case SYSTEM_EVENT_STA_START:
            esp_wifi_connect();
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);

            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            esp_wifi_connect();
            xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
            break;
        default:
            break;
    }
    return ESP_OK;
}

static void wifi_init(void)
{
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_init(wifi_event_handler, NULL));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_WIFI_SSID,
            .password = CONFIG_WIFI_PASSWORD,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_LOGI(TAG, "start the WIFI SSID:[%s] password:[%s]", CONFIG_WIFI_SSID, "******");
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "Waiting for wifi");
    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
}

static void spiflash_mount()
{
    ESP_LOGI(TAG, "Mounting FAT filesystem");
    const esp_vfs_fat_mount_config_t mount_config = {
            .max_files = 4,
            .format_if_mount_failed = true,
            .allocation_unit_size = CONFIG_WL_SECTOR_SIZE
    };
    esp_err_t err = esp_vfs_fat_spiflash_mount(BASE_PATH, "storage", &mount_config, &s_wl_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount FATFS (%s)", esp_err_to_name(err));
    }
}

static void led_toggle_task(void *ctx)
{
    uint8_t hwid[16];
    astarte_hwid_get_id(hwid);
    char encoded_hwid[256];
    astarte_hwid_encode(encoded_hwid, 256, hwid);
    ESP_LOGI(TAG, "hwid is: %s", encoded_hwid);
    astarte_credentials_init(encoded_hwid);
    struct astarte_pairing_config pairing_config = {
        .base_url = CONFIG_ASTARTE_PAIRING_BASE_URL,
        .jwt = CONFIG_ASTARTE_PAIRING_JWT,
        .realm = CONFIG_ASTARTE_REALM,
        .hw_id = encoded_hwid,
    };
    char credentials_secret[512];
    astarte_err_t err = astarte_pairing_get_credentials_secret(&pairing_config, credentials_secret, 512);
    if (err != ASTARTE_OK) {
        ESP_LOGE(TAG, "Error in get_credentials_secret");
    } else {
        ESP_LOGI(TAG, "credentials_secret is: %s", credentials_secret);
    }
    while (1) {
    }
}

void app_main()
{
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("ASTARTE_PAIRING", ESP_LOG_VERBOSE);

    nvs_flash_init();
    spiflash_mount();
    wifi_init();
    xTaskCreate(led_toggle_task, "led_toggle_task", 16384, NULL, tskIDLE_PRIORITY, NULL);
}
