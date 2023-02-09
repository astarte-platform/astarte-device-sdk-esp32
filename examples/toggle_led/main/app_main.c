#include "astarte_interface.h"
#include <driver/gpio.h>
#include <esp_wifi.h>
#include <nvs_flash.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
#include <esp_event.h>
#else
#include <esp_event_loop.h>
#endif

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "astarte_bson.h"
#include "astarte_bson_types.h"
#include "astarte_credentials.h"
#include "astarte_device.h"

#define TAG "ASTARTE_TOGGLE_LED"

// Make sure to configure these according to your board
#define BUTTON_GPIO CONFIG_BUTTON_GPIO
#define LED_GPIO CONFIG_LED_GPIO

#define ESP_INTR_FLAG_DEFAULT 0

static EventGroupHandle_t wifi_event_group;
const static int CONNECTED_BIT = BIT0;
static xQueueHandle button_evt_queue;

const static astarte_interface_t device_datastream_interface = {
    .name = "org.astarteplatform.esp32.DeviceDatastream",
    .major_version = 0,
    .minor_version = 1,
    .ownership = OWNERSHIP_DEVICE,
    .type = TYPE_DATASTREAM,
};

const static astarte_interface_t server_datastream_interface = {
    .name = "org.astarteplatform.esp32.ServerDatastream",
    .major_version = 0,
    .minor_version = 1,
    .ownership = OWNERSHIP_SERVER,
    .type = TYPE_DATASTREAM,
};

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
static void wifi_event_handler(
    void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
    }
}
#else
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
#endif

static void IRAM_ATTR button_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(button_evt_queue, &gpio_num, NULL);
}

static void wifi_init(void)
{
    wifi_event_group = xEventGroupCreate();

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
#else
    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(wifi_event_handler, NULL));
#endif

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, &instance_got_ip));
#endif

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

static void button_gpio_init()
{
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    // zero-initialize the config structure.
    gpio_config_t io_conf = {};
    // interrupt of rising edge
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    // bit mask of the pin
    io_conf.pin_bit_mask = (1ULL << BUTTON_GPIO);
    // set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    // enable pull-up mode
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);
#else
    // Set the pad as GPIO
    gpio_pad_select_gpio(BUTTON_GPIO);
    // Set button GPIO as input
    gpio_set_direction(BUTTON_GPIO, GPIO_MODE_INPUT);
    // Enable pullup
    gpio_pullup_en(BUTTON_GPIO);
    // Trigger interrupt on negative edge, i.e. on release
    gpio_set_intr_type(BUTTON_GPIO, GPIO_INTR_NEGEDGE);
#endif

    button_evt_queue = xQueueCreate(10, sizeof(uint32_t));

    // Install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    // Hook ISR handler for specific gpio pin
    gpio_isr_handler_add(BUTTON_GPIO, button_isr_handler, (void *) BUTTON_GPIO);
}

static void led_init()
{
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    // zero-initialize the config structure.
    gpio_config_t io_conf = {};
    // disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    // set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    // bit mask of the pin to set
    io_conf.pin_bit_mask = (1ULL << LED_GPIO);
    // disable pull-down mode
    io_conf.pull_down_en = 0;
    // disable pull-up mode
    io_conf.pull_up_en = 0;
    // configure GPIO with the given settings
    gpio_config(&io_conf);
#else
    // Set the pad as GPIO
    gpio_pad_select_gpio(LED_GPIO);
    // Set LED GPIO as output
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
#endif
}

static void astarte_data_events_handler(astarte_device_data_event_t *event)
{
    ESP_LOGI(TAG, "Got Astarte data event, interface_name: %s, path: %s, bson_type: %d",
        event->interface_name, event->path, event->bson_value_type);

    if (strcmp(event->interface_name, "org.astarteplatform.esp32.ServerDatastream") == 0
        && strcmp(event->path, "/led") == 0 && event->bson_value_type == BSON_TYPE_BOOLEAN) {
        int led_state = astarte_bson_value_to_int8(event->bson_value);
        if (led_state) {
            ESP_LOGI(TAG, "Turning led on");
            gpio_set_level(LED_GPIO, 1);
        } else {
            ESP_LOGI(TAG, "Turning led off");
            gpio_set_level(LED_GPIO, 0);
        }
    }
}

static void astarte_connection_events_handler(astarte_device_connection_event_t *event)
{
    ESP_LOGI(TAG, "Astarte device connected, session_present: %d", event->session_present);
}

static void astarte_disconnection_events_handler(astarte_device_disconnection_event_t *event)
{
    ESP_LOGI(TAG, "Astarte device disconnected");
}

static void astarte_example_task(void *ctx)
{
    /*
     * For additional ways to define the hwid of your device, see the documentation for
     * the astarte_device_init function in astarte_device.h
     *
     */
    astarte_device_config_t cfg = {
        .data_event_callback = astarte_data_events_handler,
        .connection_event_callback = astarte_connection_events_handler,
        .disconnection_event_callback = astarte_disconnection_events_handler,
    };

    astarte_device_handle_t device = astarte_device_init(&cfg);
    if (!device) {
        ESP_LOGE(TAG, "Failed to init astarte device");
        return;
    }

    astarte_device_add_interface(device, &device_datastream_interface);
    astarte_device_add_interface(device, &server_datastream_interface);
    if (astarte_device_start(device) != ASTARTE_OK) {
        ESP_LOGE(TAG, "Failed to start astarte device");
        return;
    }

    ESP_LOGI(TAG, "[APP] Encoded device ID: %s", astarte_device_get_encoded_id(device));

    uint32_t io_num;
    while (1) {
        if (xQueueReceive(button_evt_queue, &io_num, portMAX_DELAY)) {
            if (io_num == BUTTON_GPIO) {
                // Button pressed, send 1 and current uptime
                astarte_device_stream_boolean(
                    device, "org.astarteplatform.esp32.DeviceDatastream", "/userButton", 1, 0);
                int uptimeSeconds = (xTaskGetTickCount() * portTICK_PERIOD_MS) / 1000;
                astarte_device_stream_integer(device, "org.astarteplatform.esp32.DeviceDatastream",
                    "/uptimeSeconds", uptimeSeconds, 0);
            }
        }
    }
}

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
    esp_log_level_set("ASTARTE_PAIRING", ESP_LOG_VERBOSE);

    nvs_flash_init();
    wifi_init();
    led_init();
    button_gpio_init();
    astarte_credentials_init();
    xTaskCreate(astarte_example_task, "astarte_example_task", 6000, NULL, tskIDLE_PRIORITY, NULL);
}
