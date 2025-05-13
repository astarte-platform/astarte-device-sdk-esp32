/*
 * (C) Copyright 2024-2025, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "astarte_task.h"

#include <ctype.h>
#include <stdlib.h>

#include <freertos/FreeRTOS.h> // NOLINT Circular header file dependencies is an idf problem
#include <freertos/task.h>

#include <argtable3/argtable3.h>
#include <esp_console.h>
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
#include "utils.h"

// Those are private funcitons from the device SDK that we use in the unit tests
#include "bson_deserializer.h"
#include "data_private.h"
#include "dlist.h"

/************************************************
 * Constants and defines
 ***********************************************/

#define TAG "astarte-end-to-end-astarte-task"

static astarte_device_handle_t device = NULL;

#define CONSOLE_PROMPT_STR CONFIG_IDF_TARGET
#define CONSOLE_MAX_COMMAND_LINE_LENGTH 1024

/************************************************
 * Static functions declaration
 ***********************************************/

static int send_individual_datastream(const char *interface, const char *path, const uint8_t *data,
    size_t data_size, astarte_mapping_type_t dtype, const int64_t *timestamp);
char *buffer_to_hex_string(const char *buff, size_t buff_size);

/************************************************
 * Astarte device callback handlers
 ***********************************************/

static dlist_t reception_data_dlist = { 0 };
static dlist_t reception_sizes_dlist = { 0 };

static void connection_callback(astarte_device_connection_event_t event)
{
    (void) event;
    ESP_LOGI(TAG, "Astarte device connected.");
}
static void disconnection_callback(astarte_device_disconnection_event_t event)
{
    (void) event;
    ESP_LOGI(TAG, "Astarte device disconnected.");
}
static void datastream_individual_callback(astarte_device_datastream_individual_event_t event)
{
    astarte_result_t ares = ASTARTE_RESULT_OK;
    bson_serializer_t bson = { 0 };
    const char *interface_name = event.base_event.interface_name;
    const char *path = event.base_event.path;
    astarte_data_t data = event.data;

    ESP_LOGI(TAG, "Datastream individual event, interface: %s, path: %s", interface_name, path);
    utils_log_astarte_data(data);

    ares = bson_serializer_init(&bson);
    if (ares != ASTARTE_RESULT_OK) {
        ESP_LOGE(TAG, "Could not initialize the bson serializer");
        goto failure;
    }
    ares = data_serialize(&bson, "data", data);
    if (ares != ASTARTE_RESULT_OK) {
        goto failure;
    }
    bson_serializer_append_string(&bson, "interface-name", interface_name);
    bson_serializer_append_string(&bson, "path", path);
    bson_serializer_append_end_of_document(&bson);

    int serialized_bson_size = 0;
    const void *serialized_bson = bson_serializer_get_serialized(bson, &serialized_bson_size);
    ares = dlist_append(&reception_data_dlist, (void *) serialized_bson);
    if (ares != ASTARTE_RESULT_OK) {
        goto failure;
    }
    ares = dlist_append_int(&reception_sizes_dlist, serialized_bson_size);
    if (ares != ASTARTE_RESULT_OK) {
        dlist_remove_tail(&reception_data_dlist);
        goto failure;
    }

    return;

failure:
    bson_serializer_destroy(&bson);
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

/************************************************
 * Shell commands handlers
 ***********************************************/

static struct
{
    struct arg_lit *object;
    struct arg_int *timestamp;
    struct arg_str *interface;
    struct arg_str *path;
    struct arg_str *data;
    struct arg_int *dtype;
    struct arg_end *end;
} cmd_send_data_args;

static int cmd_connect_handler(int argc, char **argv)
{
    ESP_LOGI(TAG, "Connect a device to Astarte.");

    astarte_result_t ares = astarte_device_connect(device);
    if (ares != ASTARTE_RESULT_OK) {
        ESP_LOGE(TAG, "Failed in device connection, err: %s", astarte_result_to_name(ares));
        return -1;
    }

    return 0;
}
static int cmd_disconnect_handler(int argc, char **argv)
{
    ESP_LOGI(TAG, "Disconnect a device from Astarte.");

    astarte_result_t ares = astarte_device_disconnect(device, 2000);
    if (ares != ASTARTE_RESULT_OK) {
        ESP_LOGE(TAG, "Failed in device disconnection, err: %s", astarte_result_to_name(ares));
        return -1;
    }

    return 0;
}
static int cmd_send_data_handler(int argc, char **argv)
{
    int res = 0;
    uint8_t *data_parsed = NULL;
    int64_t *timestamp = NULL;

    ESP_LOGI(TAG, "Received send-data command.");
    int nerrors = arg_parse(argc, argv, (void **) &cmd_send_data_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, cmd_send_data_args.end, argv[0]);
        return 1;
    }

    bool object = (cmd_send_data_args.object->count == 0) ? false : true;
    const char *interface = cmd_send_data_args.interface->sval[0];
    const char *path = cmd_send_data_args.path->sval[0];
    const char *data = cmd_send_data_args.data->sval[0];
    if (cmd_send_data_args.timestamp->count != 0) {
        timestamp = calloc(1, sizeof(int64_t));
        if (!timestamp) {
            ESP_LOGE(TAG, "Out of memory %s: %d", __FILE__, __LINE__);
            res = 1;
            goto exit;
        }
        *timestamp = ((int64_t) cmd_send_data_args.timestamp->ival[0]) * 1000;
    }
    const int dtype = cmd_send_data_args.dtype->ival[0];

    size_t data_parsed_size = 0U;
    data_parsed = utils_hexstr_to_bytes(data, &data_parsed_size);
    if (!data_parsed) {
        ESP_LOGE(TAG, "Could not parse the data string '%s'", data);
        res = 1;
        goto exit;
    }
    ESP_LOGI(TAG, "Size of parsed: %zu.", data_parsed_size);

    if (!object) {
        ESP_LOGI(TAG, "Sending individual datastream to Astarte.");
        ESP_LOGI(TAG, "Interface: %s.", interface);
        ESP_LOGI(TAG, "Path: %s.", path);
        ESP_LOGI(TAG, "Data: %s.", data);
        ESP_LOGI(TAG, "Timestamp: %" PRIi64, (timestamp) ? *timestamp : -1);

        int res = send_individual_datastream(interface, path, data_parsed, data_parsed_size,
            (astarte_mapping_type_t) dtype, (timestamp) ? timestamp : NULL);
        if (res != 0) {
            ESP_LOGE(TAG, "Could not transmit the data.");
            res = 1;
            goto exit;
        }
    } else {
        ESP_LOGI(TAG, "Sending object datastream to Astarte.");
        ESP_LOGI(TAG, "Interface: %s.", interface);
        ESP_LOGI(TAG, "Path: %s.", path);
        ESP_LOGI(TAG, "Data: %s.", data);
    }

    ESP_LOGI(TAG, "Transmission to Astarte completed '%s' - '%s' - '%s'", interface, path, data);

exit:
    free(data_parsed);
    free(timestamp);
    return res;
}
static int cmd_get_data_handler(int argc, char **argv)
{
    astarte_result_t ares_data = ASTARTE_RESULT_OK;
    astarte_result_t ares_sizes = ASTARTE_RESULT_OK;
    ESP_LOGI(TAG, "Getting data received from Astarte.");

    dlist_iterator_t data_iterator = { 0 };
    dlist_iterator_t sizes_iterator = { 0 };
    ares_data = dlist_iterator_init(&reception_data_dlist, &data_iterator);
    ares_sizes = dlist_iterator_init(&reception_sizes_dlist, &sizes_iterator);
    while ((ares_data == ASTARTE_RESULT_OK) && (ares_sizes == ASTARTE_RESULT_OK)) {
        const char *data = (const char *) dlist_iterator_get_item(&data_iterator);
        const int *data_size = dlist_iterator_get_item(&sizes_iterator);
        ESP_LOGI(TAG, "Received data:");
        char *payload_str = buffer_to_hex_string(data, *data_size);
        if (payload_str) {
            ESP_LOGI(TAG, "\n-- BEGIN --\n%s\n-- END --", payload_str);
            free(payload_str);
        } else {
            ESP_LOGE(TAG, "Error while converting the bson to a string.");
        }
        ares_data = dlist_iterator_advance(&data_iterator);
        ares_sizes = dlist_iterator_advance(&sizes_iterator);
    }

    ESP_LOGI(TAG, "List of received data emptied.");

    return 0;
}
static int cmd_clear_receive_queue_handler(int argc, char **argv)
{
    ESP_LOGI(TAG, "Clearing the receive queue.");

    dlist_destroy_and_release(&reception_data_dlist);
    dlist_destroy_and_release(&reception_sizes_dlist);

    ESP_LOGI(TAG, "Receive queue cleared.");

    return 0;
}

/************************************************
 * Global functions definition
 ***********************************************/

esp_err_t astarte_task_console_start(void)
{
    esp_err_t esp_err = ESP_OK;
    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.prompt = CONSOLE_PROMPT_STR ">";
    repl_config.max_cmdline_length = CONSOLE_MAX_COMMAND_LINE_LENGTH;

#if defined(CONFIG_ESP_CONSOLE_UART_DEFAULT) || defined(CONFIG_ESP_CONSOLE_UART_CUSTOM)
    esp_console_dev_uart_config_t hw_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    esp_err = esp_console_new_repl_uart(&hw_config, &repl_config, &repl);
    if (esp_err != ESP_OK) {
        ESP_LOGE(TAG, "Error establishing UART REPL environment: %s.", esp_err_to_name(esp_err));
        goto exit;
    }
#else
#error Unsupported console type
#endif

    // Register all the required commands
    const esp_console_cmd_t connect_cmd = { .command = "connect",
        .help = "Connect the device to Astarte",
        .hint = NULL,
        .func = &cmd_connect_handler,
        .argtable = NULL };
    esp_err = esp_console_cmd_register(&connect_cmd);
    if (esp_err != ESP_OK) {
        ESP_LOGE(TAG, "Error registering connect command: %s.", esp_err_to_name(esp_err));
        goto exit;
    }
    const esp_console_cmd_t disconnect_cmd = { .command = "disconnect",
        .help = "Disconnect the device from Astarte",
        .hint = NULL,
        .func = &cmd_disconnect_handler,
        .argtable = NULL };
    esp_err = esp_console_cmd_register(&disconnect_cmd);
    if (esp_err != ESP_OK) {
        ESP_LOGE(TAG, "Error registering disconnect command: %s.", esp_err_to_name(esp_err));
        goto exit;
    }
    cmd_send_data_args.object = arg_lit0("o", NULL, "The data is of a object interface");
    cmd_send_data_args.timestamp = arg_int0("t", NULL, "<int>", "Timesamp to add to the data");
    cmd_send_data_args.interface = arg_str1(NULL, NULL, "<str>", "The interface name");
    cmd_send_data_args.path = arg_str1(NULL, NULL, "<str>", "The path");
    cmd_send_data_args.data = arg_str1(NULL, NULL, "<str>", "The data to transmit");
    cmd_send_data_args.dtype = arg_int1(NULL, "dtype", "<int>", "Type of the data to transmit");
    cmd_send_data_args.end = arg_end(10);
    const esp_console_cmd_t send_data_cmd = { .command = "send-data",
        .help = "Send data to Astarte",
        .hint = NULL,
        .func = &cmd_send_data_handler,
        .argtable = &cmd_send_data_args };
    esp_err = esp_console_cmd_register(&send_data_cmd);
    if (esp_err != ESP_OK) {
        ESP_LOGE(TAG, "Error registering send data command: %s.", esp_err_to_name(esp_err));
        goto exit;
    }
    const esp_console_cmd_t get_data_cmd = { .command = "get-data",
        .help = "Get data received from Astarte",
        .hint = NULL,
        .func = &cmd_get_data_handler,
        .argtable = NULL };
    esp_err = esp_console_cmd_register(&get_data_cmd);
    if (esp_err != ESP_OK) {
        ESP_LOGE(TAG, "Error registering get data command: %s.", esp_err_to_name(esp_err));
        goto exit;
    }
    const esp_console_cmd_t clear_receive_queue_cmd = { .command = "clear-receive-queue",
        .help = "Clear data receive queue",
        .hint = NULL,
        .func = &cmd_clear_receive_queue_handler,
        .argtable = NULL };
    esp_err = esp_console_cmd_register(&clear_receive_queue_cmd);
    if (esp_err != ESP_OK) {
        ESP_LOGE(TAG, "Error registering clear rx queue command: %s.", esp_err_to_name(esp_err));
        goto exit;
    }

    // Start the console context
    esp_err = esp_console_start_repl(repl);
    if (esp_err != ESP_OK) {
        ESP_LOGE(TAG, "Error registering : %s.", esp_err_to_name(esp_err));
        goto exit;
    }

exit:
    return esp_err;
}

void astarte_task_entry(void *ctx)
{
    (void) ctx;
    esp_err_t esp_err = ESP_OK;
    astarte_result_t ares = ASTARTE_RESULT_OK;

    reception_data_dlist = dlist_init();
    reception_sizes_dlist = dlist_init();

    char device_id[ASTARTE_DEVICE_ID_LEN + 1] = CONFIG_DEVICE_ID;
    char cred_secr[ASTARTE_PAIRING_CRED_SECR_LEN + 1] = CONFIG_CREDENTIAL_SECRET;

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

    const astarte_interface_t *interfaces[] = {
        &org_astarteplatform_end_to_end_DeviceAggregate,
        &org_astarteplatform_end_to_end_DeviceDatastream,
        &org_astarteplatform_end_to_end_DeviceProperty,
        &org_astarteplatform_end_to_end_ServerAggregate,
        &org_astarteplatform_end_to_end_ServerDatastream,
        &org_astarteplatform_end_to_end_ServerProperty,
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
    ESP_LOGI(TAG, "Device created.");

    TickType_t last_wake_time = xTaskGetTickCount();
    while (true) {
        ares = astarte_device_poll(device);
        if (ares != ASTARTE_RESULT_OK) {
            ESP_LOGE(TAG, "Astarte device poll failure.");
            goto exit;
        }
        vTaskDelayUntil(&last_wake_time, CONFIG_DEVICE_POLL_PERIOD_MS / portTICK_PERIOD_MS);
    }

    ESP_LOGI(TAG, "Destroying the device.");
    ares = astarte_device_destroy(device, 2000);
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

static int send_individual_datastream(const char *interface, const char *path, const uint8_t *data,
    size_t data_size, astarte_mapping_type_t dtype, const int64_t *timestamp)
{
    if (!bson_deserializer_check_validity(data, data_size)) {
        ESP_LOGE(TAG, "Invalid BSON document");
        return -1;
    }

    bson_document_t full_document = bson_deserializer_init_doc(data);
    bson_element_t v_elem = { 0 };
    if (bson_deserializer_element_lookup(full_document, "v", &v_elem) != ASTARTE_RESULT_OK) {
        ESP_LOGE(TAG, "Cannot retrieve BSON value from data");
        return -1;
    }

    astarte_data_t data_deserialized = { 0 };
    astarte_result_t ares = data_deserialize(v_elem, dtype, &data_deserialized);
    if (ares != ASTARTE_RESULT_OK) {
        ESP_LOGE(TAG, "Failed in parsing BSON file.");
        return -1;
    }

    ESP_LOGI(TAG, "Sending data on path '%s':", path);
    utils_log_astarte_data(data_deserialized);
    ares = astarte_device_send_individual(device, interface, path, data_deserialized, timestamp);
    if (ares != ASTARTE_RESULT_OK) {
        ESP_LOGI(TAG, "Astarte device transmission failure.");
    }

    data_destroy_deserialized(data_deserialized);
    return (ares == ASTARTE_RESULT_OK) ? 0 : -1;
}

char *buffer_to_hex_string(const char *buff, size_t buff_size)
{
    if (buff == NULL || buff_size == 0) {
        return NULL;
    }

    // Calculate the required string size. Each byte becomes 2 hex characters, and we need
    // (buff_size - 1) hyphens, plus 1 for the null terminator.
    size_t hex_string_size = (buff_size * 2) + (buff_size - 1) + 1;
    char *hex_string = (char *) calloc(hex_string_size, sizeof(char));
    if (hex_string == NULL) {
        return NULL;
    }

    // Use snprintf to convert each byte to its hexadecimal representation, with a hyphen separator.
    int offset = 0;
    for (size_t i = 0; i < buff_size; i++) {
        if (i > 0) {
            offset += snprintf(hex_string + offset, hex_string_size - offset, "-");
        }
        offset += snprintf(hex_string + offset, hex_string_size - offset, "%02X", buff[i]);
    }
    // Ensure null termination
    hex_string[hex_string_size - 1] = '\0';

    return hex_string;
}
