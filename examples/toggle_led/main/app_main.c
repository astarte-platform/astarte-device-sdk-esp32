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
#include "freertos/queue.h"
#include "driver/gpio.h"

#include "esp_log.h"
#include "esp_vfs.h"
#include "esp_vfs_fat.h"

#include "astarte_device.h"
#include "astarte_bson.h"
#include "astarte_bson_types.h"

#define TAG "ASTARTE_TOGGLE_LED"

#define BASE_PATH "/spiflash"

#define ESP_INTR_FLAG_DEFAULT 0

static wl_handle_t s_wl_handle = WL_INVALID_HANDLE;
static EventGroupHandle_t wifi_event_group;
const static int CONNECTED_BIT = BIT0;
static xQueueHandle button_evt_queue;

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

static void IRAM_ATTR button_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(button_evt_queue, &gpio_num, NULL);
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

static void button_gpio_init()
{
    // Set GPIO 0 (BOOT button) as input
    gpio_set_direction(GPIO_NUM_0, GPIO_MODE_INPUT);
    // Enable pullup
    gpio_pullup_en(GPIO_NUM_0);
    // Trigger interrupt on negative edge, i.e. on release
    gpio_set_intr_type(GPIO_NUM_0, GPIO_INTR_NEGEDGE);

    button_evt_queue = xQueueCreate(10, sizeof(uint32_t));

    // Install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    // Hook ISR handler for specific gpio pin
    gpio_isr_handler_add(GPIO_NUM_0, button_isr_handler, (void*) GPIO_NUM_0);
}

static void astarte_data_events_handler(astarte_device_data_event_t *event)
{
    ESP_LOGI(TAG, "Got Astarte data event, interface_name: %s, path: %s, bson_type: %d",
        event->interface_name, event->path, event->bson_value_type);

    if (strcmp(event->interface_name, "org.astarteplatform.esp32.ServerDatastream") == 0 &&
            strcmp(event->path, "/led") == 0 &&
            event->bson_value_type == BSON_TYPE_BOOLEAN) {
        int led_state = astarte_bson_value_to_int8(event->bson_value);
        if (led_state) {
            ESP_LOGI(TAG, "Turning led on");
        } else {
            ESP_LOGI(TAG, "Turning led off");
        }
    }
}

static void led_toggle_task(void *ctx)
{
    astarte_device_config_t cfg = {
        .data_event_callback = astarte_data_events_handler,
    };
    astarte_device_handle_t device = astarte_device_init(&cfg);
    if (!device) {
        ESP_LOGE(TAG, "Failed to init astarte device");
        return;
    }

    astarte_device_add_interface(device, "org.astarteplatform.esp32.DeviceDatastream", 0, 2);
    astarte_device_add_interface(device, "org.astarteplatform.esp32.ServerDatastream", 0, 1);
    astarte_device_start(device);

    uint32_t io_num;
    while (1) {
        if (xQueueReceive(button_evt_queue, &io_num, portMAX_DELAY)) {
            if (io_num == 0) {
                // Button pressed, send 1 and current uptime
                astarte_device_stream_boolean(device, "org.astarteplatform.esp32.DeviceDatastream", "/userButton", 1, 0);
                int uptimeSeconds = (xTaskGetTickCount() * portTICK_PERIOD_MS) / 1000;
                astarte_device_stream_integer(device, "org.astarteplatform.esp32.DeviceDatastream", "/uptimeSeconds", uptimeSeconds, 0);
            }
        }
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
    button_gpio_init();

    xTaskCreate(led_toggle_task, "led_toggle_task", 16384, NULL, tskIDLE_PRIORITY, NULL);
}
