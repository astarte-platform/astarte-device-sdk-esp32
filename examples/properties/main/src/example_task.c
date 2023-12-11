/**
 * This file is part of Astarte.
 *
 * Copyright 2023 SECO Mind Srl
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later OR Apache-2.0
 *
 **/

#include "example_task.h"

#include <esp_idf_version.h>
#include <esp_log.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "astarte.h"
#include "astarte_bson_types.h"
#include "astarte_credentials.h"
#include "astarte_device.h"
#include "astarte_interface.h"

/************************************************
 * Constants and defines
 ***********************************************/

#define TAG "ASTARTE_EXAMPLE_DATASTREAMS"

// Enable/disable the following defines to personalize the example behaviour
#define SET_DEVICE_PROPERTIES
// #define UNSET_DEVICE_PROPERTIES
// #define SET_DEVICE_PROPERTY_TWICE
#define REPLY_TO_SERVER_PROPERTY_INTERFACE1
#define REPLY_TO_SERVER_PROPERTY_INTERFACE2

static const char cred_sec[] = CONFIG_CREDENTIALS_SECRET;
static const char hwid[] = CONFIG_DEVICE_ID;
static const char realm[] = CONFIG_ASTARTE_REALM;

const static astarte_interface_t device_properties_interface1 = {
    .name = "org.astarteplatform.esp32.examples.DeviceProperties1",
    .major_version = 0,
    .minor_version = 1,
    .ownership = OWNERSHIP_DEVICE,
    .type = TYPE_PROPERTIES,
};

const static astarte_interface_t device_properties_interface2 = {
    .name = "org.astarteplatform.esp32.examples.DeviceProperties2",
    .major_version = 0,
    .minor_version = 1,
    .ownership = OWNERSHIP_DEVICE,
    .type = TYPE_PROPERTIES,
};

const static astarte_interface_t server_properties_interface1 = {
    .name = "org.astarteplatform.esp32.examples.ServerProperties1",
    .major_version = 0,
    .minor_version = 1,
    .ownership = OWNERSHIP_SERVER,
    .type = TYPE_PROPERTIES,
};

const static astarte_interface_t server_properties_interface2 = {
    .name = "org.astarteplatform.esp32.examples.ServerProperties2",
    .major_version = 0,
    .minor_version = 1,
    .ownership = OWNERSHIP_SERVER,
    .type = TYPE_PROPERTIES,
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
 * @brief Handler for astarte unset event.
 *
 * @param event Astarte device unset event pointer.
 */
static void astarte_unset_events_handler(astarte_device_unset_event_t *event);
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

    bool invert_reply = true;
    astarte_device_config_t cfg = {
        .data_event_callback = astarte_data_events_handler,
        .unset_event_callback = astarte_unset_events_handler,
        .connection_event_callback = astarte_connection_events_handler,
        .disconnection_event_callback = astarte_disconnection_events_handler,
        .callbacks_user_data = (void *) &invert_reply,
        .credentials_secret = cred_sec,
        .hwid = hwid,
        .realm = realm,
    };

    astarte_device_handle_t device = astarte_device_init(&cfg);
    if (!device) {
        ESP_LOGE(TAG, "Failed to init astarte device");
        return;
    }

    astarte_device_add_interface(device, &device_properties_interface1);
    astarte_device_add_interface(device, &device_properties_interface2);
    astarte_device_add_interface(device, &server_properties_interface1);
    astarte_device_add_interface(device, &server_properties_interface2);
    if (astarte_device_start(device) != ASTARTE_OK) {
        ESP_LOGE(TAG, "Failed to start astarte device");
        return;
    }

    ESP_LOGI(TAG, "Encoded device ID: %s", astarte_device_get_encoded_id(device));

    const TickType_t five_s_ticks = 5000 / portTICK_PERIOD_MS;
    const TickType_t thirty_s_ticks = five_s_ticks * 6U;
    astarte_err_t astarte_err = ASTARTE_OK;
#if defined(SET_DEVICE_PROPERTIES)
    vTaskDelay(five_s_ticks);

    // Set some properties to a standard value
    astarte_err = astarte_device_set_boolean_property(
        device, device_properties_interface1.name, "/sensor1/boolean_endpoint", true);
    ESP_LOGI(TAG, "Transmission result: %s", astarte_err_to_name(astarte_err));
    astarte_err = astarte_device_set_boolean_property(
        device, device_properties_interface1.name, "/sensor2/boolean_endpoint", true);
    ESP_LOGI(TAG, "Transmission result: %s", astarte_err_to_name(astarte_err));
    astarte_err = astarte_device_set_boolean_property(
        device, device_properties_interface1.name, "/sensor3/boolean_endpoint", true);
    ESP_LOGI(TAG, "Transmission result: %s", astarte_err_to_name(astarte_err));
    astarte_err = astarte_device_set_boolean_property(
        device, device_properties_interface2.name, "/sensor4/boolean_endpoint", true);
    ESP_LOGI(TAG, "Transmission result: %s", astarte_err_to_name(astarte_err));
#endif

#if defined(UNSET_DEVICE_PROPERTIES)
    vTaskDelay(five_s_ticks);

    // Unset some properties
    astarte_err = astarte_device_unset_path(
        device, device_properties_interface1.name, "/sensor1/boolean_endpoint");
    ESP_LOGI(TAG, "Transmission result: %s", astarte_err_to_name(astarte_err));
    astarte_err = astarte_device_unset_path(
        device, device_properties_interface1.name, "/sensor2/boolean_endpoint");
    ESP_LOGI(TAG, "Transmission result: %s", astarte_err_to_name(astarte_err));
#endif

#if defined(SET_DEVICE_PROPERTY_TWICE)
    vTaskDelay(five_s_ticks);

    // Set some properties to a standard value
    astarte_err = astarte_device_set_boolean_property(
        device, device_properties_interface1.name, "/sensor1/boolean_endpoint", true);
    ESP_LOGI(TAG, "Transmission result: %s", astarte_err_to_name(astarte_err));
    vTaskDelay(five_s_ticks);
    astarte_err = astarte_device_set_boolean_property(
        device, device_properties_interface1.name, "/sensor1/boolean_endpoint", true);
    ESP_LOGI(TAG, "Transmission result: %s", astarte_err_to_name(astarte_err));
#endif

    vTaskDelay(thirty_s_ticks);
    astarte_device_stop(device);

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

#if defined(REPLY_TO_SERVER_PROPERTY_INTERFACE1)
    if (strcmp(event->interface_name, server_properties_interface1.name) == 0
        && strcmp(event->path, "/42/boolean_endpoint") == 0
        && event->bson_element.type == BSON_TYPE_BOOLEAN) {

        bool received_value = astarte_bson_deserializer_element_to_bool(event->bson_element);
        ESP_LOGI(TAG, "Received: %d", received_value);
        // Set a property to the value received
        astarte_err_t astarte_err = astarte_device_set_boolean_property(event->device,
            device_properties_interface1.name, "/24/boolean_endpoint", received_value);
        ESP_LOGI(TAG, "Transmission result: %s", astarte_err_to_name(astarte_err));
    }
#endif
#if defined(REPLY_TO_SERVER_PROPERTY_INTERFACE2)
    if (strcmp(event->interface_name, server_properties_interface2.name) == 0
        && strcmp(event->path, "/43/boolean_endpoint") == 0
        && event->bson_element.type == BSON_TYPE_BOOLEAN) {

        bool received_value = astarte_bson_deserializer_element_to_bool(event->bson_element);
        ESP_LOGI(TAG, "Received: %d", received_value);
        // Set a property to the value received
        astarte_err_t astarte_err = astarte_device_set_boolean_property(event->device,
            device_properties_interface2.name, "/25/boolean_endpoint", received_value);
        ESP_LOGI(TAG, "Transmission result: %s", astarte_err_to_name(astarte_err));
    }
#endif
}

static void astarte_unset_events_handler(astarte_device_unset_event_t *event)
{
    ESP_LOGI(TAG, "Got Astarte unset event, interface_name: %s, path: %s", event->interface_name,
        event->path);
}

static void astarte_disconnection_events_handler(astarte_device_disconnection_event_t *event)
{
    (void) event;

    ESP_LOGI(TAG, "Astarte device disconnected");
}
