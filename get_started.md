<!--
Copyright 2024 SECO Mind Srl

SPDX-License-Identifier: Apache-2.0
-->

# Get started with ESP32

Follow this guide to start with the Astarte device SDK for the ESP32 device. We will guide you
through setting up a very basic application creating a device, connecting it to a local Astarte
instance and transmitting some data.

## Before you begin

### Local Astarte instance

This get started will focus on creating a device and connecting it to an Astarte instance.
If you don't have access to an Astarte instance you can easily set up one following our
[Astarte quick instance guide](https://docs.astarte-platform.org/device-sdks/common/astarte_quick_instance.html).

From here on we will assume you have access to an Astarte instance, remote or on a host machine
connected to the same LAN where your device will be connected.
Furthermore, we will assume you have access to the Astarte dashboard for a realm.
The next steps will install the required interfaces and register a new device on Astarte using the
dashboard. The same operations could be performed using `astartectl` and the access token generated
in the Astarte quick instance guide.

### Installing the required interfaces

The interfaces that our device will use must first be installed within the Astarte instance.
In this guide we will show how to stream individual and aggregated data as well as how to set and
unset properties. As a consequence we will need three separated interfaces, one for each data type.

The following is the definition of the individually aggregated interface:
```json
{
    "interface_name": "org.astarte-platform.esp32.get-started.Individual",
    "version_major": 0,
    "version_minor": 1,
    "type": "datastream",
    "ownership": "device",
    "description": "Individual interface for the get-started of the Astarte device SDK for ESP32.",
    "mappings": [
        {
            "endpoint": "/double_endpoint",
            "type": "double",
            "explicit_timestamp": false
        }
    ]
}
```
Next is the definition of the aggregated interface:
```json
{
    "interface_name": "org.astarte-platform.esp32.get-started.Aggregated",
    "version_major": 0,
    "version_minor": 1,
    "type": "datastream",
    "aggregation": "object",
    "ownership": "device",
    "description": "Aggregated interface for the get-started of the Astarte device SDK for ESP32.",
    "mappings": [
        {
            "endpoint": "/group_data/double_endpoint",
            "type": "double",
            "explicit_timestamp": false
        },
        {
            "endpoint": "/group_data/string_endpoint",
            "type": "string",
            "explicit_timestamp": false
        }
    ]
}
```
And finally the definition of the property interface:
```json
{
    "interface_name": "org.astarte-platform.esp32.get-started.Property",
    "version_major": 0,
    "version_minor": 1,
    "type": "properties",
    "ownership": "device",
    "description": "Property interface for the get-started of the Astarte device SDK for ESP32.",
    "mappings": [
        {
            "endpoint": "/double_endpoint",
            "type": "double",
            "allow_unset": true
        }
    ]
}
```

To install the three interfaces in the Astarte instance, open the Astarte dashboard, navigate to the
interfaces tab and click on install new interface.
You can copy and paste the JSON files for each interface in the right box overwriting the default
template.

### Registering the device

Devices should be pre-registered to Astarte before their first connection.
With the Astarte device SDK for ESP32 this can be achieved in two ways:
- By registering the device on Astarte manually, obtaining a credentials secret and transfering it
  on the device.
- By using the included registration utilities provided by the SDK. Those utilities can make use of
  a registration JWT issued by Astarte and register the device automatically before the first
  connection.

To keep this guide as simple as possible we will use the first method, as a device can be registered
using the Astarte dashboard with a couple of clicks.

To install a new device start by opening the dashboard and navigate to the devices tab.
Click on register a new device, there you can input your own device ID or generate a random one.
For example you could use the device ID `MFVgjjP2Tt6RbT_wKOI0VA`.
Click on register device, this will register the device an give you a credentials secret.
The credentials secret will be used by the device to authenticate with Astarte.
Copy it somewhere safe as it will be used in the next steps.

## ESP IDF toolchain

This library is dependent on the
[ESP IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/index.html) project.
We will assume you have already followed the
[getting started guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html)
from Espressif and are familiar with the ESP IDF toolchain.

## Create a new application for ESP IDF

We leave to the user to set up a simple basic application for this get started.
The application should configure networking an connect the ESP32 to the internet.
Furthermore it should create a new task for the Astarte device with a stack size of at least 65kB.

## Adding minimal configuration options for the Astarte device

The following configuration options should be set in the `sdkconfig.defaults` file.
```kconfig
CONFIG_ASTARTE_DEVICE_SDK_HOSTNAME=
CONFIG_ASTARTE_DEVICE_SDK_REALM_NAME=
ASTARTE_DEVICE_SDK_NVS-n
```
If you used the Astarte quick instance to set up Astarte the hostname and realm nanme will be
`api.astarte.<HOST IP ADDRESS>.nip.io` and `test`. We disable the NVS support in the Astarte
device SDK for simplicity.

## Generating the interfaces headers

We previously added three interfaces to our Astarte instance. We will now do something similar on
the device.
Start by saving the three interfaces in JSON files in a `interfaces` folder in your working
directory. You can then convert them to a set of C files containing all the same structures by
running a Python script.

```sh
curl -sSL https://raw.githubusercontent.com/astarte-platform/astarte-device-sdk-esp32/v2.0.0-alpha.1/python_scripts/generate_interfaces.py
python generate_interfaces.py ./interfaces
```

This command will generate a source and header files. In order for ESP IDF to include them in the
build process we should add a couple of lines to the `CMakeLists.txt` of our application.
```
idf_component_register(
    SRCS
        ... other sources ...
        "./../interfaces/generated_interfaces.c"
    INCLUDE_DIRS
        ... other includes ...
        "./../interfaces"
    REQUIRES astarte-device-sdk-esp32 ... other requires ...)

```

## Instantiating a device

Finally, we can start with the source code of our device application. We will first create a new
device using the device ID and credentials secret we obtained in the previous steps.

The Astarte device SDK uses callbacks to communicate to the user that some event has occurred.
Such callbacks should be configured before instantiating the device and connecting to Astarte.
Several callbacks can be optionally configured. A connection callback, an individual data received
callback, an object data received callback, and set property received callback, an unset property
received callback, and a disconnection callback.

```C
#include "astarte_task.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <esp_log.h>
#include <string.h>

#include "astarte_device_sdk/data.h"
#include "astarte_device_sdk/device.h"
#include "astarte_device_sdk/device_id.h"
#include "astarte_device_sdk/interface.h"
#include "astarte_device_sdk/mapping.h"
#include "astarte_device_sdk/object.h"
#include "astarte_device_sdk/pairing.h"

#include "generated_interfaces.h"

#define TAG "astarte-app"

static void connection_callback(astarte_device_connection_event_t event)
{
    (void) event;
    ESP_LOGI(TAG, "Astarte device connected.");
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
    ESP_LOGI(TAG, "Datastream individual event, interface: %s, path: %s", interface_name, path);
}
static void datastream_object_callback(astarte_device_datastream_object_event_t event)
{
    const char *interface_name = event.base_event.interface_name;
    const char *path = event.base_event.path;
    ESP_LOGI(TAG, "Datastream object event, interface: %s, path: %s", interface_name, path);
}
static void set_property_callback(astarte_device_property_set_event_t event)
{
    const char *interface_name = event.base_event.interface_name;
    const char *path = event.base_event.path;
    ESP_LOGI(TAG, "Property set event, interface: %s, path: %s", interface_name, path);
}
static void unset_property_callback(astarte_device_data_event_t event)
{
    const char *interface_name = event.interface_name;
    const char *path = event.path;
    ESP_LOGI(TAG, "Property unset event, interface: %s, path: %s", interface_name, path);
}

void astarte_task_entry(void *ctx)
{
    (void) ctx;
    esp_err_t esp_err = ESP_OK;
    astarte_result_t ares = ASTARTE_RESULT_OK;
    astarte_device_handle_t device = NULL;

    char device_id[ASTARTE_DEVICE_ID_LEN + 1] = "";
    char cred_secr[ASTARTE_PAIRING_CRED_SECR_LEN + 1] = "";

    const astarte_interface_t *interfaces[] = {
        &org_astarte_platform_esp32_get_started_Individual,
        &org_astarte_platform_esp32_get_started_Aggregated,
        &org_astarte_platform_esp32_get_started_Property
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

    // ... do stuff with the device ...

exit:
    vTaskDelete(NULL);
}
```

## Connecting and polling the device

After device initialization, the device will need to be connected explicitly and polled regularly
to ensure the processing of MQTT messages.
Ideally, two separate threads should be used for polling and transmission.
```C
// ... imports and callbacks ...

void astarte_task_entry(void *ctx)
{
    // ... device instantiation ...

    ESP_LOGI(TAG, "Connecting the device.");
    ares = astarte_device_connect(device);
    if (ares != ASTARTE_RESULT_OK) {
        ESP_LOGE(TAG, "Failed in device connection, err: %s", astarte_result_to_name(ares));
        goto exit;
    }

    TickType_t last_wake_time = xTaskGetTickCount();
    while (true) {
        ares = astarte_device_poll(device);
        if (ares != ASTARTE_RESULT_OK) {
            ESP_LOGE(TAG, "Astarte device poll failure.");
            return;
        }
        vTaskDelayUntil(&last_wake_time, 100 / portTICK_PERIOD_MS);
    }

exit:
    vTaskDelete(NULL);
}
```

## Streaming data

Streaming of data could be performed for device owned interfaces of `individual` or `object`
aggregation type.
Since the main Astarte application thread is occupied with polling the Astarte device transmission
should be performed in a different thread.
Insert a new thread creation right before the connect function. We will use it for transmission.

```C
// ... imports and callbacks ...

#define ASTARTE_TRANSMIT_TASK_STACK_SIZE 16384
static TaskHandle_t transmit_task_handle = NULL;

static void connection_callback(astarte_device_connection_event_t event)
{
    (void) event;
    ESP_LOGI(TAG, "Astarte device connected.");
    vTaskResume(transmit_task_handle);
}

static void astarte_task_transmit(void *ctx)
{
    vTaskSuspend(NULL);
    astarte_device_handle_t device = *((astarte_device_handle_t *) ctx);

    // ... data transmission ...

    vTaskDelete(NULL);
}

void astarte_task_entry(void *ctx)
{
    // ... device instantiation ...

    BaseType_t task_create_ret
        = xTaskCreate(astarte_task_transmit, "AstarteTx", ASTARTE_TRANSMIT_TASK_STACK_SIZE,
            (void *) &device, tskIDLE_PRIORITY, &transmit_task_handle);
    if (task_create_ret != pdPASS) {
        ESP_LOGE(TAG, "Failed in transmission task creation.");
        goto exit;
    }

    // ... device connection and polling ...

exit:
    vTaskDelete(NULL);
}
```

We will add some data transmission function calls in `astarte_task_transmit` in the next steps.
Notice how the task is suspended untill the connection callback triggers.

### Streaming individual data

In Astarte interfaces with `individual` aggregation, each mapping is treated as an independent value
and is managed individually.

The snippet below shows how to send a value that will be inserted into the `"/double_endpoint"`
datastream, that is part of the `"org.astarte-platform.esp32.get-started.Individual"` datastream
interface.

```C
// ... imports and callbacks ...

static void astarte_task_transmit(void *ctx)
{
    vTaskSuspend(NULL);
    astarte_device_handle_t device = *((astarte_device_handle_t *) ctx);

    {
        const char *interface_name = org_astarte_platform_esp32_get_started_Individual.name;
        const char *path = "/double_endpoint";
        astarte_data_t data = astarte_data_from_double(25.4);
        astarte_result_t res = astarte_device_send_individual(device, interface_name, path, data, NULL);
        if (res != ASTARTE_RESULT_OK) {
            ESP_LOGI(TAG, "Astarte device transmission failure.");
        }
    }

    // ... more data transmission ...

    vTaskDelete(NULL);
}

// ... Astarte device main task function (astarte_task_entry) ...
```

### Streaming aggregated data

In Astarte interfaces with `object` aggregation, Astarte expects the owner to send all of the
interface's mappings at the same time, packed in a single message.

The following snippet shows how to send a value for an object-aggregated interface. In this example,
two different data types will be sent together and will be inserted into the `"/group_data"`
datastream, which is part of the `"org.astarte-platform.esp32.get-started.Aggregated"` datastream
interface.

```C
// ... imports and callbacks ...

static void astarte_task_transmit(void *ctx)
{
    vTaskSuspend(NULL);
    astarte_device_handle_t device = *((astarte_device_handle_t *) ctx);

    // ... more data transmission ...

    {
        const char *interface_name = org_astarte_platform_esp32_get_started_Aggregated.name;
        const char *path = "/group_data";

        astarte_object_entry_t entries[] = {
            astarte_object_entry_new("double_endpoint", astarte_data_from_double(utils_double_data)),
            astarte_object_entry_new("string_endpoint", astarte_data_from_string(utils_string_data)),
        };
        astarte_result_t res = astarte_device_send_object(
            device, interface_name, path, entries, ARRAY_SIZE(entries), NULL);
        if (res != ASTARTE_RESULT_OK) {
            ESP_LOGI(TAG, "Astarte device transmission failure.");
        }
    }

    vTaskDelete(NULL);
}

// ... Astarte device main task function (astarte_task_entry) ...
```

## Setting and unsetting properties

Interfaces of the `property` type represent a persistent, stateful, synchronized state with no
concept of history or timestamping. From a programming point of view, setting and unsetting
properties of device-owned interfaces is rather similar to sending messages on datastream
interfaces.

In this get started guide persistency for the properties has been disabled. As such, setting and
unsetting properties from the device would make little sense. See the more complete code samples in
the [GitHub repository](https://github.com/astarte-platform/astarte-device-sdk-zephyr) of the
ESP32 Astarte device SDK for more information on how to set up the appropriate flash partition.

## Disconnecting the device

The Astarte device can be gracefully disconnected by Astarte using the disconnect function.
This function will trigger a callback to one of the user-defined functions.

To disconnect the device we should first exit the infinite polling loop we wrote earlier.
We can use another task notification as done for the transmission task.

```C
// ... imports and callbacks ...

#define ASTARTE_DISCONNECT_TIMEOUT_MS 2000

static TaskHandle_t device_task_handle = NULL;

static void astarte_task_transmit(void *ctx)
{
    vTaskSuspend(NULL);
    astarte_device_handle_t device = *((astarte_device_handle_t *) ctx);

    // ... data transmission ...

    ESP_LOGI(TAG, "Transmission task completed signaling the main task to terminate.");
    xTaskNotifyGive(device_task_handle);
    vTaskDelete(NULL);
}

void astarte_task_entry(void *ctx)
{
    device_task_handle = xTaskGetCurrentTaskHandle();

    // ... device instantiation and connection, transmission task creation ...

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
```
