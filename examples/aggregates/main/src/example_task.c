#include "example_task.h"

#include <esp_idf_version.h>
#include <esp_log.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>

#include "astarte.h"
#include "astarte_bson_serializer.h"
#include "astarte_bson_types.h"
#include "astarte_credentials.h"
#include "astarte_device.h"
#include "astarte_interface.h"

/************************************************
 * Constants and defines
 ***********************************************/

#define TAG "ASTARTE_EXAMPLE_AGGREGATE"

static const char cred_sec[] = CONFIG_CREDENTIALS_SECRET;
static const char hwid[] = CONFIG_DEVICE_ID;
static const char realm[] = CONFIG_ASTARTE_REALM;

const static astarte_interface_t device_datastream_interface = {
    .name = "org.astarteplatform.esp32.examples.DeviceAggregate",
    .major_version = 0,
    .minor_version = 1,
    .ownership = OWNERSHIP_DEVICE,
    .type = TYPE_DATASTREAM,
};

const static astarte_interface_t server_datastream_interface = {
    .name = "org.astarteplatform.esp32.examples.ServerAggregate",
    .major_version = 0,
    .minor_version = 1,
    .ownership = OWNERSHIP_SERVER,
    .type = TYPE_DATASTREAM,
};

/************************************************
 * Structs declarations
 ***********************************************/

struct rx_aggregate
{
    bool booleans[4];
    int64_t longinteger;
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
/**
 * @brief Parse the received BSON data for this example.
 *
 * @param bson_element The bson element to be parsed
 * @param the resulting parsed data
 * @return 0 when successful, 1 otherwise
 */
static uint8_t parse_received_bson(
    astarte_bson_element_t bson_element, struct rx_aggregate *rx_data);

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
        event->interface_name, event->path, event->bson_element.type);

    uint8_t parsing_error = 0;
    struct rx_aggregate rx_data;

    if ((strcmp(event->interface_name, server_datastream_interface.name) == 0)
        && (strcmp(event->path, "/11") == 0)) {

        parsing_error = parse_received_bson(event->bson_element, &rx_data);

    } else {
        parsing_error = 1U;
    }

    if (parsing_error == 0U) {
        ESP_LOGI(TAG, "Server aggregate received with the following content:");
        ESP_LOGI(TAG, "longinteger_endpoint: %lli", rx_data.longinteger);
        ESP_LOGI(TAG, "booleanarray_endpoint: {%d, %d, %d, %d}", rx_data.booleans[0],
            rx_data.booleans[1], rx_data.booleans[2], rx_data.booleans[3]);

        double double_endpoint = 43.2;
        int32_t integer_endpoint = 54;
        bool boolean_endpoint = true;
        double doubleanarray_endpoint[] = { 11.2, 2.2, 99.9, 421.1 };
        ESP_LOGI(TAG, "Sending device aggregate with the following content:");
        ESP_LOGI(TAG, "double_endpoint: %lf", double_endpoint);
        ESP_LOGI(TAG, "integer_endpoint: %" PRIu32, integer_endpoint);
        ESP_LOGI(TAG, "boolean_endpoint: %d", boolean_endpoint);
        ESP_LOGI(TAG, "doubleanarray_endpoint: {%lf, %lf, %lf, %lf}", doubleanarray_endpoint[0],
            doubleanarray_endpoint[1], doubleanarray_endpoint[2], doubleanarray_endpoint[3]);

        astarte_bson_serializer_handle_t aggregate_bson = astarte_bson_serializer_new();
        astarte_bson_serializer_append_double(aggregate_bson, "double_endpoint", double_endpoint);
        astarte_bson_serializer_append_int32(aggregate_bson, "integer_endpoint", integer_endpoint);
        astarte_bson_serializer_append_boolean(
            aggregate_bson, "boolean_endpoint", boolean_endpoint);
        astarte_bson_serializer_append_double_array(
            aggregate_bson, "doublearray_endpoint", doubleanarray_endpoint, 4);
        astarte_bson_serializer_append_end_of_document(aggregate_bson);

        const void *doc = astarte_bson_serializer_get_document(aggregate_bson, NULL);
        astarte_err_t res = astarte_device_stream_aggregate(
            event->device, device_datastream_interface.name, "/24", doc, 0);
        if (res != ASTARTE_OK) {
            ESP_LOGE(TAG, "Error streaming the aggregate");
        }

        astarte_bson_serializer_destroy(aggregate_bson);
        ESP_LOGI(TAG, "Device aggregate sent, using sensor_id: 24.");
    } else {
        ESP_LOGE(TAG, "Server aggregate incorrectly received.");
    }
}

static void astarte_disconnection_events_handler(astarte_device_disconnection_event_t *event)
{
    (void) event;

    ESP_LOGI(TAG, "Astarte device disconnected");
}

static uint8_t parse_received_bson(
    astarte_bson_element_t bson_element, struct rx_aggregate *rx_data)
{
    if (bson_element.type != BSON_TYPE_DOCUMENT) {
        return 1U;
    }

    astarte_bson_document_t doc = astarte_bson_deserializer_element_to_document(bson_element);
    if (doc.size != 0x4F) {
        return 1U;
    }

    astarte_bson_element_t elem_longinteger_endpoint;
    if ((astarte_bson_deserializer_element_lookup(
             doc, "longinteger_endpoint", &elem_longinteger_endpoint)
            != ASTARTE_OK)
        || (elem_longinteger_endpoint.type != BSON_TYPE_INT64)) {
        return 1U;
    }
    rx_data->longinteger = astarte_bson_deserializer_element_to_int64(elem_longinteger_endpoint);

    astarte_bson_element_t elem_boolean_array;
    if ((astarte_bson_deserializer_element_lookup(doc, "booleanarray_endpoint", &elem_boolean_array)
            != ASTARTE_OK)
        || (elem_boolean_array.type != BSON_TYPE_ARRAY)) {
        return 1U;
    }

    astarte_bson_document_t arr = astarte_bson_deserializer_element_to_array(elem_boolean_array);
    if (arr.size != 0x15) {
        return 1U;
    }

    astarte_bson_element_t elem_boolean;
    if ((astarte_bson_deserializer_first_element(arr, &elem_boolean) != ASTARTE_OK)
        || (elem_boolean.type != BSON_TYPE_BOOLEAN)) {
        return 1U;
    }
    rx_data->booleans[0] = astarte_bson_deserializer_element_to_bool(elem_boolean);

    for (size_t i = 1; i < 4; i++) {
        if ((astarte_bson_deserializer_next_element(arr, elem_boolean, &elem_boolean) != ASTARTE_OK)
            || (elem_boolean.type != BSON_TYPE_BOOLEAN)) {
            return 1U;
        }
        rx_data->booleans[i] = astarte_bson_deserializer_element_to_bool(elem_boolean);
    }

    return 0U;
}
