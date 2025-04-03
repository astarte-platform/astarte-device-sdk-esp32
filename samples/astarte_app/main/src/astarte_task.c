/*
 * (C) Copyright 2024=5, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "astarte_task.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <esp_log.h>
#include <nvs_flash.h>
#include <string.h>

#include "astarte_device_sdk/data.h"
#include "astarte_device_sdk/device.h"
#include "astarte_device_sdk/device_id.h"
#include "astarte_device_sdk/interface.h"
#include "astarte_device_sdk/mapping.h"
#include "astarte_device_sdk/object.h"
#include "astarte_device_sdk/pairing.h"

#include "generated_interfaces.h"

/************************************************
 * Constants and defines
 ***********************************************/

#define TAG "ASTARTE SAMPLE ASTARTE TASK"

/************************************************
 * Static functions declaration
 ***********************************************/

/************************************************
 * Global functions definition
 ***********************************************/

void astarte_task_entry(void *ctx)
{
    (void) ctx;
    esp_err_t esp_err = ESP_OK;
    astarte_result_t ares = ASTARTE_RESULT_OK;

    char device_id[ASTARTE_DEVICE_ID_LEN + 1] = CONFIG_DEVICE_ID;
#if defined(CONFIG_DEVICE_REGISTRATION)
    // Open NVS to search for a previosuly stored credential secret
    nvs_handle_t nvs_handle = 0U;
    esp_err = nvs_open("sample app", NVS_READWRITE, &nvs_handle);
    if (esp_err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS partition: %s.", esp_err_to_name(esp_err));
        goto exit;
    }

    // Fetch credential secret from NVS
    char cred_secr[ASTARTE_PAIRING_CRED_SECR_LEN + 1] = { 0 };
    size_t cred_secr_len = sizeof(cred_secr);
    esp_err = nvs_get_str(nvs_handle, "cred secret", cred_secr, &cred_secr_len);
    if (esp_err == ESP_ERR_NVS_NOT_FOUND) {

        // If NVS does not contain a credential secret, register the device using the JWT
        ESP_LOGI(TAG, "Performing a new device registration with Astarte");
        ares = astarte_pairing_register_device(device_id, cred_secr);
        if (ares != ASTARTE_RESULT_OK) {
            ESP_LOGE(TAG, "Device registration failure, err: %s", astarte_result_to_name(ares));
            goto exit;
        }

        // Store received credential secret in NVS
        esp_err = nvs_set_str(nvs_handle, "cred secret", cred_secr);
        if (esp_err != ESP_OK) {
            ESP_LOGE(TAG, "Error storing credential secret in NVS: %s.", esp_err_to_name(esp_err));
            goto exit;
        }

    } else if (esp_err != ESP_OK) {
        ESP_LOGE(TAG, "Error reading credential secret from NVS: %s.", esp_err_to_name(esp_err));
        goto exit;
    }
#else
    char cred_secr[ASTARTE_PAIRING_CRED_SECR_LEN + 1] = CONFIG_CREDENTIAL_SECRET;
#endif

    // Initialize the NVS partition dedicated to Astarte
    esp_err = nvs_flash_init_partition(CONFIG_ASTARTE_DEVICE_SDK_NVS_PARTITION_LABEL);
    if (esp_err != ESP_OK) {
        ESP_LOGE(TAG, "Error initializing Astarte NVS partition: %s.", esp_err_to_name(esp_err));
        goto exit;
    }

    // You shouldn't log a credential secret in a production device
    ESP_LOGI(TAG, "Credential secret: '%s'", cred_secr);

    const astarte_interface_t *interfaces[] = {
        &org_astarteplatform_esp32_examples_DeviceAggregate,
        &org_astarteplatform_esp32_examples_DeviceDatastream,
        &org_astarteplatform_esp32_examples_DeviceProperty,
        &org_astarteplatform_esp32_examples_ServerAggregate,
        &org_astarteplatform_esp32_examples_ServerDatastream,
        &org_astarteplatform_esp32_examples_ServerProperty,
    };

    astarte_device_config_t device_config = { 0 };
    // device_config.connection_cbk = connection_callback;
    // device_config.disconnection_cbk = disconnection_callback;
    // device_config.datastream_individual_cbk = datastream_individual_callback;
    // device_config.datastream_object_cbk = datastream_object_callback;
    // device_config.property_set_cbk = set_property_callback;
    // device_config.property_unset_cbk = unset_property_callback;
    device_config.interfaces = interfaces;
    device_config.interfaces_size = ARRAY_SIZE(interfaces);
    memcpy(device_config.device_id, device_id, sizeof(device_id));
    memcpy(device_config.cred_secr, cred_secr, sizeof(cred_secr));

    astarte_device_handle_t device = NULL;
    ares = astarte_device_new(&device_config, &device);
    if (ares != ASTARTE_RESULT_OK) {
        ESP_LOGE(TAG, "Failed in device creation, err: %s", astarte_result_to_name(ares));
        goto exit;
    }

    ESP_LOGI(TAG, "Connecting the device.");
    ares = astarte_device_connect(device);
    if (ares != ASTARTE_RESULT_OK) {
        ESP_LOGE(TAG, "Failed in device connection, err: %s", astarte_result_to_name(ares));
        goto exit;
    }

    // TODO wait and check for connectivity

#if defined(CONFIG_DEVICE_INDIVIDUAL_TRANSMISSION) || \
    defined(CONFIG_DEVICE_OBJECT_TRANSMISSION) || \
    defined(CONFIG_DEVICE_PROPERTY_SET_TRANSMISSION)  || \
    defined(CONFIG_DEVICE_PROPERTY_UNSET_TRANSMISSION)
    // TODO: transmit something
#else
    ESP_LOGI(TAG, "No trasmission required, waiting for operational timeout.");
    vTaskDelay(CONFIG_DEVICE_OPERATIONAL_TIMEOUT * 1000 / portTICK_PERIOD_MS);
#endif

    ESP_LOGI(TAG, "Disconnecting the device.");
    ares = astarte_device_disconnect(device);
    if (ares != ASTARTE_RESULT_OK) {
        ESP_LOGE(TAG, "Failed in device disconnection, err: %s", astarte_result_to_name(ares));
        goto exit;
    }

    ESP_LOGI(TAG, "Destroying the device.");
    ares = astarte_device_destroy(device);
    if (ares != ASTARTE_RESULT_OK) {
        ESP_LOGE(TAG, "Failed in device destruction, err: %s", astarte_result_to_name(ares));
        goto exit;
    }

exit:
    vTaskDelete(NULL);
}

/************************************************
 * Static functions definitions
 ***********************************************/
