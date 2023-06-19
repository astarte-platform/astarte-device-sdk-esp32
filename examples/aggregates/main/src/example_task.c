#include "example_task.h"

#include <esp_idf_version.h>
#include <esp_log.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>

#include "astarte.h"
#include "astarte_bson.h"
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
    uint8_t booleans[4];
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

    uint8_t parsing_error = 0;
    struct rx_aggregate rx_data;

    if ((strcmp(event->interface_name, server_datastream_interface.name) == 0)
        && (strcmp(event->path, "/11") == 0) && (event->bson_value_type == BSON_TYPE_DOCUMENT)
        && (astarte_bson_document_size(event->bson_value) == 0x4F)) {
        /*  event->bson_value should be containing:
         *  0x4F 0x00 0x00 0x00 {
         *      0x04 { "booleanarray_endpoint" }
         *          0x15 0x00 0x00 0x00 {
         *              0x08 { "0" } 0x00
         *              0x08 { "1" } 0x00
         *              0x08 { "2" } 0x00
         *              0x08 { "3" } 0x01
         *          } 0x00
         *      0x12 { "longinteger_endpoint" } int64
         *  } 0x00
         */

        char booleanarray_key[] = "booleanarray_endpoint";
        uint8_t booleanarray_type = 0U;
        const void *booleanarray = astarte_bson_key_lookup(
            (char *) &booleanarray_key, event->bson_value, &booleanarray_type);
        if ((booleanarray != NULL) && (booleanarray_type == BSON_TYPE_ARRAY)
            && (astarte_bson_document_size(booleanarray) == 0x15)) {

            uint8_t boolean_type = 0U;
            char boolean_key[5];
            for (uint32_t i = 0; i < 4; i++) {
                (void) snprintf(boolean_key, 5, "%" PRIu32 "", i); // Can't fail (i is known)
                const void *boolean
                    = astarte_bson_key_lookup((char *) &boolean_key, booleanarray, &boolean_type);
                if ((boolean != NULL) && (boolean_type == BSON_TYPE_BOOLEAN)) {
                    rx_data.booleans[i] = astarte_bson_value_to_int8(boolean);
                } else {
                    parsing_error = 1U;
                    break;
                }
            }
        } else {
            parsing_error = 1U;
        }

        char longinteger_key[] = "longinteger_endpoint";
        uint8_t longinteger_type = 0U;
        const void *longinteger = astarte_bson_key_lookup(
            (char *) &longinteger_key, event->bson_value, &longinteger_type);
        if ((longinteger != NULL) && (longinteger_type == BSON_TYPE_INT64)) {
            rx_data.longinteger = astarte_bson_value_to_int64(longinteger);
        } else {
            parsing_error = 1U;
        }
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

        struct astarte_bson_serializer aggregate_bson;
        astarte_bson_serializer_init(&aggregate_bson);
        astarte_bson_serializer_append_double(&aggregate_bson, "double_endpoint", double_endpoint);
        astarte_bson_serializer_append_int32(&aggregate_bson, "integer_endpoint", integer_endpoint);
        astarte_bson_serializer_append_boolean(
            &aggregate_bson, "boolean_endpoint", boolean_endpoint);
        astarte_bson_serializer_append_double_array(
            &aggregate_bson, "doublearray_endpoint", doubleanarray_endpoint, 4);
        astarte_bson_serializer_append_end_of_document(&aggregate_bson);

        size_t doc_len = 0;
        const void *doc = astarte_bson_serializer_get_document(&aggregate_bson, &doc_len);
        astarte_err_t res = astarte_device_stream_aggregate(
            event->device, device_datastream_interface.name, "/24", doc, 0);
        if (res != ASTARTE_OK) {
            ESP_LOGE(TAG, "Error streaming the aggregate");
        }

        astarte_bson_serializer_destroy(&aggregate_bson);
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
