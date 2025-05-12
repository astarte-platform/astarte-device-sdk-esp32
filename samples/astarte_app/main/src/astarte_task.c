/*
 * (C) Copyright 2024-2025, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "astarte_task.h"

#include <freertos/FreeRTOS.h> // NOLINT Circular header file dependencies is an idf problem
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
#include "individual_send.h"
#include "object_send.h"
#include "property_send.h"
#include "utils.h"

/************************************************
 * Constants and defines
 ***********************************************/

#define TAG "astarte-sample-astarte-task"
#define ASTARTE_TRANSMIT_TASK_STACK_SIZE 16384

#define MS_IN_SEC 1000
#define ASTARTE_DISCONNECT_TIMEOUT_MS 2000

static TaskHandle_t device_task_handle = NULL;
static TaskHandle_t transmit_task_handle = NULL;

/************************************************
 * Static functions declaration
 ***********************************************/

/**
 * @brief Entry point for the Astarte device transmission task.
 *
 * @param ctx Task parameters passed during task creation.
 */
static void astarte_task_transmit(void *ctx);
/**
 * @brief Callback handler for Astarte connection events.
 *
 * @param event Astarte device connection event.
 */
static void connection_callback(astarte_device_connection_event_t event);
/**
 * @brief Callback handler for Astarte disconnection events.
 *
 * @param event Astarte device disconnection event.
 */
static void disconnection_callback(astarte_device_disconnection_event_t event);
/**
 * @brief Callback handler for Astarte datastream individual event.
 *
 * @param event Astarte device datastream individual event.
 */
static void datastream_individual_callback(astarte_device_datastream_individual_event_t event);
/**
 * @brief Callback handler for Astarte datastream object event.
 *
 * @param event Astarte device datastream object event.
 */
static void datastream_object_callback(astarte_device_datastream_object_event_t event);
/**
 * @brief Callback handler for Astarte set property event.
 *
 * @param event Astarte device set property event.
 */
static void set_property_callback(astarte_device_property_set_event_t event);
/**
 * @brief Callback handler for Astarte unset property event.
 *
 * @param event Astarte device unset property event.
 */
static void unset_property_callback(astarte_device_data_event_t event);

/************************************************
 * Global functions definition
 ***********************************************/

void astarte_task_entry(void *ctx)
{
    (void) ctx;
    esp_err_t esp_err = ESP_OK;
    astarte_result_t ares = ASTARTE_RESULT_OK;
    astarte_device_handle_t device = NULL;
    device_task_handle = xTaskGetCurrentTaskHandle();

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

#if defined(CONFIG_ASTARTE_DEVICE_SDK_NVS)
    // Initialize the NVS partition dedicated to Astarte
    esp_err = nvs_flash_init_partition(CONFIG_ASTARTE_DEVICE_SDK_NVS_PARTITION_LABEL);
    if (esp_err != ESP_OK) {
        ESP_LOGE(TAG, "Error initializing Astarte NVS partition: %s.", esp_err_to_name(esp_err));
        goto exit;
    }
#endif

    // You shouldn't log a credential secret in a production device
    ESP_LOGI(TAG, "Credential secret: '%s'", cred_secr);

    // Creating a transmission task.
    // This task will stop immediately and be restarted once the device is connected.
    BaseType_t task_create_ret
        = xTaskCreate(astarte_task_transmit, "AstarteTx", ASTARTE_TRANSMIT_TASK_STACK_SIZE,
            (void *) &device, tskIDLE_PRIORITY, &transmit_task_handle);
    if (task_create_ret != pdPASS) {
        ESP_LOGE(TAG, "Failed in transmission task creation.");
        goto exit;
    }

    const astarte_interface_t *interfaces[] = {
        &org_astarteplatform_samples_DeviceAggregate,
        &org_astarteplatform_samples_DeviceDatastream,
        &org_astarteplatform_samples_DeviceProperty,
        &org_astarteplatform_samples_ServerAggregate,
        &org_astarteplatform_samples_ServerDatastream,
        &org_astarteplatform_samples_ServerProperty,
    };

    astarte_device_config_t device_config = { 0 };
    device_config.connection_cbk = connection_callback;
    device_config.disconnection_cbk = disconnection_callback;
    device_config.datastream_individual_cbk = datastream_individual_callback;
    device_config.datastream_object_cbk = datastream_object_callback;
    device_config.property_set_cbk = set_property_callback;
    device_config.property_unset_cbk = unset_property_callback;
    device_config.cbk_user_data = NULL;
    device_config.interfaces = interfaces;
    device_config.interfaces_size = ARRAY_SIZE(interfaces);
    memcpy(device_config.device_id, device_id, sizeof(device_id));
    memcpy(device_config.cred_secr, cred_secr, sizeof(cred_secr));

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

    TickType_t last_wake_time = xTaskGetTickCount();
    while (ulTaskNotifyTake(pdTRUE, 0) == 0) {
        ares = astarte_device_poll(device);
        if (ares != ASTARTE_RESULT_OK) {
            ESP_LOGE(TAG, "Astarte device poll failure.");
            return;
        }
        vTaskDelayUntil(&last_wake_time, CONFIG_DEVICE_POLL_PERIOD_MS / portTICK_PERIOD_MS);
    }

    ESP_LOGI(TAG, "Disconnecting the device.");
    ares = astarte_device_disconnect(device, ASTARTE_DISCONNECT_TIMEOUT_MS);
    if (ares != ASTARTE_RESULT_OK) {
        ESP_LOGE(TAG, "Failed in device disconnection, err: %s", astarte_result_to_name(ares));
        goto exit;
    }

    ESP_LOGI(TAG, "Destroying the device.");
    ares = astarte_device_destroy(device, ASTARTE_DISCONNECT_TIMEOUT_MS);
    if (ares != ASTARTE_RESULT_OK) {
        ESP_LOGE(TAG, "Failed in device destruction, err: %s", astarte_result_to_name(ares));
        goto exit;
    }

    ESP_LOGI(TAG, "Sample completed.");

exit:
    vTaskDelete(NULL);
}

/************************************************
 * Static functions definitions
 ***********************************************/

static void astarte_task_transmit(void *ctx)
{
    vTaskSuspend(NULL);
    astarte_device_handle_t device = *((astarte_device_handle_t *) ctx);

#if defined(CONFIG_DEVICE_INDIVIDUAL_TRANSMISSION)
    ESP_LOGI(TAG, "Waiting %d seconds to send individuals.",
        CONFIG_DEVICE_INDIVIDUAL_TRANSMISSION_DELAY_SECONDS);
    vTaskDelay(
        CONFIG_DEVICE_INDIVIDUAL_TRANSMISSION_DELAY_SECONDS * MS_IN_SEC / portTICK_PERIOD_MS);
    sample_individual_transmission(device);
#endif
#if defined(CONFIG_DEVICE_OBJECT_TRANSMISSION)
    ESP_LOGI(TAG, "Waiting %d seconds to send objects.",
        CONFIG_DEVICE_OBJECT_TRANSMISSION_DELAY_SECONDS);
    vTaskDelay(CONFIG_DEVICE_OBJECT_TRANSMISSION_DELAY_SECONDS * MS_IN_SEC / portTICK_PERIOD_MS);
    sample_object_transmission(device);
#endif
#if defined(CONFIG_DEVICE_PROPERTY_SET_TRANSMISSION)
    ESP_LOGI(TAG, "Waiting %d seconds to set properties.",
        CONFIG_DEVICE_PROPERTY_SET_TRANSMISSION_DELAY_SECONDS);
    vTaskDelay(
        CONFIG_DEVICE_PROPERTY_SET_TRANSMISSION_DELAY_SECONDS * MS_IN_SEC / portTICK_PERIOD_MS);
    sample_property_set_transmission(device);
#endif
#if defined(CONFIG_DEVICE_PROPERTY_UNSET_TRANSMISSION)
    ESP_LOGI(TAG, "Waiting %d seconds to unset properties.",
        CONFIG_DEVICE_PROPERTY_UNSET_TRANSMISSION_DELAY_SECONDS);
    vTaskDelay(
        CONFIG_DEVICE_PROPERTY_UNSET_TRANSMISSION_DELAY_SECONDS * MS_IN_SEC / portTICK_PERIOD_MS);
    sample_property_unset_transmission(device);
#endif

#if !defined(CONFIG_DEVICE_INDIVIDUAL_TRANSMISSION) && !defined(CONFIG_DEVICE_OBJECT_TRANSMISSION) \
    && !defined(CONFIG_DEVICE_PROPERTY_SET_TRANSMISSION)                                           \
    && !defined(CONFIG_DEVICE_PROPERTY_UNSET_TRANSMISSION)
    ESP_LOGI(TAG, "No trasmission required, waiting for operational timeout.");
    vTaskDelay(CONFIG_DEVICE_OPERATIONAL_TIMEOUT * MS_IN_SEC / portTICK_PERIOD_MS);
#endif

    vTaskDelay(MS_IN_SEC / portTICK_PERIOD_MS);
    ESP_LOGI(TAG, "Transmission task completed signaling the main task to terminate.");
    xTaskNotifyGive(device_task_handle);
    vTaskDelete(NULL);
}

static void connection_callback(astarte_device_connection_event_t event)
{
    (void) event;
    ESP_LOGI(TAG, "Astarte device connected.");
    vTaskResume(transmit_task_handle);
}

static void disconnection_callback(astarte_device_disconnection_event_t event)
{
    (void) event;
    ESP_LOGI(TAG, "Astarte device disconnected");
}

static void datastream_individual_callback(astarte_device_datastream_individual_event_t event)
{
    const char *interface_name = event.base_event.interface_name;
    const char *path = event.base_event.path;
    astarte_data_t individual = event.data;

    ESP_LOGI(TAG, "Datastream individual event, interface: %s, path: %s", interface_name, path);

    utils_log_astarte_data(individual);
}

static void datastream_object_callback(astarte_device_datastream_object_event_t event)
{
    const char *interface_name = event.base_event.interface_name;
    const char *path = event.base_event.path;
    astarte_object_entry_t *entries = event.entries;
    size_t entries_length = event.entries_len;

    ESP_LOGI(TAG, "Datastream object event, interface: %s, path: %s", interface_name, path);

    utils_log_astarte_object(entries, entries_length);
}

static void set_property_callback(astarte_device_property_set_event_t event)
{
    const char *interface_name = event.base_event.interface_name;
    const char *path = event.base_event.path;
    astarte_data_t individual = event.data;

    ESP_LOGI(TAG, "Property set event, interface: %s, path: %s", interface_name, path);

    utils_log_astarte_data(individual);
}

static void unset_property_callback(astarte_device_data_event_t event)
{
    const char *interface_name = event.interface_name;
    const char *path = event.path;
    ESP_LOGI(TAG, "Property unset event, interface: %s, path: %s", interface_name, path);
}
