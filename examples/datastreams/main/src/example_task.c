#include "example_task.h"

#include <esp_idf_version.h>
#include <esp_log.h>

#include "astarte.h"
#include "astarte_bson.h"
#include "astarte_bson_types.h"
#include "astarte_credentials.h"
#include "astarte_device.h"
#include "astarte_interface.h"

/************************************************
 * Constants and defines
 ***********************************************/

#define TAG "ASTARTE_EXAMPLE_DATASTREAMS"

static const char cred_sec[] = CONFIG_CREDENTIALS_SECRET;
static const char hwid[] = CONFIG_DEVICE_ID;
static const char realm[] = CONFIG_ASTARTE_REALM;

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
    (void) ctx;

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
        .credentials_secret = cred_sec,
        .hwid = hwid,
        .realm = realm,
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
        && strcmp(event->path, "/question") == 0 && event->bson_value_type == BSON_TYPE_BOOLEAN) {
        bool answer = !*(char *) event->bson_value;
        ESP_LOGI(TAG, "Sending answer %d.", answer);
        astarte_device_stream_boolean(event->device,
            "org.astarteplatform.esp32.examples.DeviceDatastream", "/answer", answer, 0);
    }
}

static void astarte_disconnection_events_handler(astarte_device_disconnection_event_t *event)
{
    (void) event;

    ESP_LOGI(TAG, "Astarte device disconnected");
}
