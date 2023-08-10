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
#include <nvs_flash.h>

#include "astarte.h"
#include "astarte_bson_types.h"
#include "astarte_credentials.h"
#include "astarte_device.h"
#include "astarte_interface.h"

/************************************************
 * Constants and defines
 ***********************************************/

#define TAG "NVS_EXAMPLE_EXAMPLE_TASK"

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
/**
 * @brief Initialize a custom NVS partition.
 *
 * @param name Name of the custom partition to initialize.
 * @return ESP_OK if successful, an appropriate error code otherwise.
 */
static esp_err_t custom_nvs_part_init(const char *name);
/************************************************
 * Global functions definition
 ***********************************************/

void astarte_example_task(void *ctx)
{
    (void) ctx;

    // Can be used to erase the partition when running the example multiple times on the same
    // hardware but different hardware IDs.
    // ESP_ERROR_CHECK(nvs_flash_erase_partition("astarte"));
    esp_err_t ret = custom_nvs_part_init("astarte");
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize astarte partition (%s)", esp_err_to_name(ret));
        return;
    }
    astarte_credentials_use_nvs_storage("astarte");

    if (astarte_credentials_init() != ASTARTE_OK) {
        ESP_LOGE(TAG, "Failed to initialize credentials");
        return;
    }
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

    ESP_LOGI(TAG, "Encoded device ID: %s", astarte_device_get_encoded_id(device));
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

    if (strcmp(event->interface_name, "org.astarteplatform.esp32.examples.ServerDatastream") == 0
        && strcmp(event->path, "/question") == 0 && event->bson_element.type == BSON_TYPE_BOOLEAN) {
        bool answer = !astarte_bson_deserializer_element_to_bool(event->bson_element);
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

static esp_err_t custom_nvs_part_init(const char *name)
{
#if CONFIG_NVS_ENCRYPTION
    esp_err_t ret = ESP_FAIL;
    const esp_partition_t *key_part = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS_KEYS, NULL);
    if (key_part == NULL) {
        ESP_LOGE(TAG,
            "CONFIG_NVS_ENCRYPTION is enabled, but no partition with subtype nvs_keys found in the "
            "partition table.");
        return ret;
    }

    nvs_sec_cfg_t cfg = {};
    ret = nvs_flash_read_security_cfg(key_part, &cfg);
    if (ret != ESP_OK) {
        /* We shall not generate keys here as that must have been done in default NVS partition
         * initialization case */
        ESP_LOGE(TAG, "Failed to read NVS security cfg: [0x%02X] (%s)", ret, esp_err_to_name(ret));
        return ret;
    }

    ret = nvs_flash_secure_init_partition(name, &cfg);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "NVS partition \"%s\" is encrypted.", name);
    }
    return ret;
#else
    return nvs_flash_init_partition(name);
#endif
}
