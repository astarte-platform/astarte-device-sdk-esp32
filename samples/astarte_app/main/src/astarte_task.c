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

#include "astarte_device_sdk/data.h"
#include "astarte_device_sdk/device_id.h"
#include "astarte_device_sdk/interface.h"
#include "astarte_device_sdk/mapping.h"
#include "astarte_device_sdk/object.h"
#include "astarte_device_sdk/pairing.h"

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
    char cred_secr[NEW_AST_PAIRING_CRED_SECR_LEN + 1] = { 0 };
    size_t cred_secr_len = sizeof(cred_secr);
    esp_err = nvs_get_str(nvs_handle, "cred secret", cred_secr, &cred_secr_len);
    if (esp_err == ESP_ERR_NVS_NOT_FOUND) {

        // If NVS does not contain a credential secret, register the device using the JWT
        ESP_LOGI(TAG, "Performing a new device registration with Astarte");
        astarte_result_t ares = new_ast_pairing_register_device(device_id, cred_secr);
        if (ares != ASTARTE_RESULT_OK) {
            ESP_LOGE(TAG, "Device registration failure, err: %s", astarte_result_to_name(ares));
            goto exit;
        }

        // Store received credential secret in NVS
        esp_err = nvs_set_str(nvs_handle, "cred secret", cred_secr);
        if (ares != ASTARTE_RESULT_OK) {
            ESP_LOGE(TAG, "Error storing credential secret in NVS: %s.", esp_err_to_name(esp_err));
            goto exit;
        }

    } else if (esp_err != ESP_OK) {
        ESP_LOGE(TAG, "Error reading credential secret from NVS: %s.", esp_err_to_name(esp_err));
        goto exit;
    }
#else
    char cred_secr[NEW_AST_PAIRING_CRED_SECR_LEN + 1] = CONFIG_CREDENTIAL_SECRET;
#endif

    // Initialize the NVS partition dedicated to Astarte
    esp_err = nvs_flash_init_partition(CONFIG_ASTARTE_DEVICE_SDK_NVS_PARTITION_LABEL);
    if (esp_err != ESP_OK) {
        ESP_LOGE(TAG, "Error initializing Astarte NVS partition: %s.", esp_err_to_name(esp_err));
        goto exit;
    }

    // You shouldn't log a credential secret in a production device
    ESP_LOGI(TAG, "Credential secret: '%s'", cred_secr);

    while (1) {
    }

exit:
    vTaskDelete(NULL);
}

/************************************************
 * Static functions definitions
 ***********************************************/
