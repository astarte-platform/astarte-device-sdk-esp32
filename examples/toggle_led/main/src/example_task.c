#include "example_task.h"

#include <esp_idf_version.h>
#include <esp_log.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
#include "freertos/task.h"
#endif

#include "astarte_bson.h"
#include "astarte_bson_types.h"
#include "astarte_credentials.h"
#include "astarte_device.h"

#include "button.h"
#include "led.h"

/************************************************
 * Constants and defines
 ***********************************************/

#define TAG "ASTARTE_EXAMPLE_TOGGLE_LED"

const static astarte_interface_t device_datastream_interface = {
    .name = "org.astarteplatform.esp32.examples.DeviceDatastream",
    .major_version = 0,
    .minor_version = 1,
    .ownership = OWNERSHIP_DEVICE,
    .type = TYPE_DATASTREAM,
};

const static astarte_interface_t server_datastream_interface = {
    .name = "org.astarteplatform.esp32.examples.ServerDatastream",
    .major_version = 0,
    .minor_version = 1,
    .ownership = OWNERSHIP_SERVER,
    .type = TYPE_DATASTREAM,
};

/************************************************
 * Static functions declaration
 ***********************************************/

/**
 * @brief Handler for astarte connection events.
 *
 * @param event Astarte device connection event pointer.
 */
static void astarte_connection_events_handler(astarte_device_connection_event_t *event);
/**
 * @brief Handler for astarte data event.
 *
 * @param event Astarte device data event pointer.
 */
static void astarte_data_events_handler(astarte_device_data_event_t *event);
/**
 * @brief Handler for astarte disconnection events.
 *
 * @param event Astarte device disconnection event pointer.
 */
static void astarte_disconnection_events_handler(astarte_device_disconnection_event_t *event);

/************************************************
 * Global functions definition
 ***********************************************/

void astarte_example_task(void *ctx)
{
    led_init();
    button_gpio_init();
    if (astarte_credentials_init() != ASTARTE_OK) {
        ESP_LOGE(TAG, "Failed to initialize credentials");
        return;
    }
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

    while (1) {
        if (poll_button_event()) {
            // Button pressed, send 1 and current uptime
            astarte_device_stream_boolean(
                device, "org.astarteplatform.esp32.examples.DeviceDatastream", "/userButton", 1, 0);
            int uptimeSeconds = (xTaskGetTickCount() * portTICK_PERIOD_MS) / 1000;
            astarte_device_stream_integer(device,
                "org.astarteplatform.esp32.examples.DeviceDatastream", "/uptimeSeconds",
                uptimeSeconds, 0);
        }
    }
}

/************************************************
 * Static functions definitions
 ***********************************************/

static void astarte_connection_events_handler(astarte_device_connection_event_t *event)
{
    ESP_LOGI(TAG, "Astarte device connected, session_present: %d", event->session_present);
}

static void astarte_data_events_handler(astarte_device_data_event_t *event)
{
    ESP_LOGI(TAG, "Got Astarte data event, interface_name: %s, path: %s, bson_type: %d",
        event->interface_name, event->path, event->bson_value_type);

    if (strcmp(event->interface_name, "org.astarteplatform.esp32.examples.ServerDatastream") == 0
        && strcmp(event->path, "/led") == 0 && event->bson_value_type == BSON_TYPE_BOOLEAN) {
        if (astarte_bson_value_to_int8(event->bson_value) != 0) {
            led_set_level(1);
        } else {
            led_set_level(0);
        }
    }
}

static void astarte_disconnection_events_handler(astarte_device_disconnection_event_t *event)
{
    ESP_LOGI(TAG, "Astarte device disconnected");
}
