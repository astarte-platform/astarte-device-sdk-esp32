<!---
  Copyright 2018-2023 SECO Mind Srl

  SPDX-License-Identifier: LGPL-2.1-or-later OR Apache-2.0
-->

# Astarte ESP32 SDK

Astarte ESP32 SDK lets you connect your ESP32 device to
[Astarte](https://github.com/astarte-platform/astarte).

The SDK simplifies all the low-level operations like credentials generation, pairing and so on, and
exposes a high-level API to publish data from your device.

Have a look at the [examples](examples/README.md) for a usage example showing how to send and
receive data.

## Documentation

The generated Doxygen documentation is available in the [Astarte Documentation
website](https://docs.astarte-platform.org/device-sdks/esp32/latest/api).

## `esp-idf` version compatibility

The SDK is tested against these versions of `esp-idf`:
- `v4.4`
- `v5.0`

Previous versions of `esp-idf` are not guaranteed to work. If you find a problem using a later
version of `esp-idf`, please [open an
issue](https://github.com/astarte-platform/astarte-device-sdk-esp32/issues).

## Component Free RTOS APIs usage

The Astarte ESP32 Device interacts internally with the Free RTOS APIs. As such some factors
should be taken into account when integrating this component into a larger system.

The following **tasks** are spawned directly by the Astarte ESP32 Device:
- `credentials_init_task`: Initializes the credentials for the MQTT communication.
This task is created when calling the `astarte_credentials_init()` function.
This should be done before initializing the Astarte ESP32 Device.
It will use `16384` words from the stack and will be deleted before exiting the
`astarte_credentials_init()` function.
- `astarte_device_reinit_task`: Reinitializes the device in case of a TLS error coming from an
expired certificate. This task is created upon device initialization and runs constantly for the
life of the device. It will use `6000` words from the stack.

All of the tasks are spawned with the lowest priority and rely on the time-slicing functionality
of freertos to run concurrently with the main task.

## Notes on non-volatile memory (NVM)

The device's Astarte credentials are always stored in the NVM. This means that credentials will
be preserved in between reboots.

Two storage methods can be used for the credentials. A FAT32 file system or the standard
non-volatile storage (NVS) library provided by the ESP32. The FAT32 is the default storage choice.
If you want to use NVS storage, add the
`astarte_credentials_use_nvs_storage` function before initializing `astarte_credentials`.
```C
astarte_credentials_use_nvs_storage(NVS_PARTITION);
```

However, note that even when FAT32 is used, the device credential secret is always stored using
the NVS library. This will require devices configured as using FAT32 to have two partitions,
one using as name the macro `NVS_DEFAULT_PART_NAME` (NVS) and one named `astarte` (FAT32).

Furthermore, if you whish to use flash encryption for your device the only supported option is
NVS.

### Re-flashing devices

As a side effect of NVM usage, credentials will be preserved also between device flashes using
`idf.py`.

**N.B.** The device will first check if valid credentials are stored in the NVM, and only in case
of a negative result will use the `Astarte SDK` component configuration to obtain fresh credentials.

Most of the time during development the device credentials remain unchanged between firmware
flashes and the old ones stored in the NVM can be reused.
However, when changing elements of the `Astarte SDK` component configuration the old credentials
should be discarded by erasing the NMV.
This can be done using the following command:
| idf.py (ESP-IDF v5.x) | idf.py (ESP-IDF v4.x) |
| ------------- | ------------ |
| `idf.py -p <DEVICE_PORT> erase-flash` | `idf.py -p <DEVICE_PORT> erase_flash` |

## Notes on ignoring TLS certificates

**N.B. Do not ignore TLS certificates errors in production!**

Ignoring TLS certificates errors can be useful when trying out the device libraries in a prototyping
context.
For example when using a local Astarte instance with self signed certificates.

To disable the TLS certificates checks in the device libraries add the following to your
`sdkconfig.defaults` file:
```
#
# ESP-TLS
#
CONFIG_ESP_TLS_INSECURE=y
CONFIG_ESP_TLS_SKIP_SERVER_CERT_VERIFY=y
# end of ESP-TLS

#
# Certificate Bundle
#
CONFIG_MBEDTLS_CERTIFICATE_BUNDLE=n
# end of Certificate Bundle
```

If you are not using an `sdkconfig.defaults` file, run `idf.py menuconfig` and set the three options
listed above manually. The insecure and skip certificate options can be found in the ESP-TLS
component. While the bundle option can be found in the mbedTLS component.

## Notes on custom certificate bundle

The Astarte ESP32 Device uses the
[ESP x509 Certificate Bundle](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/esp_crt_bundle.html)
to verify the Astarte instance certificates.

If your Astarte instance uses a custom CA you can disable the default certificate bundle and add
your own certificates.
This can be done directly in `esp-idf` using the `menuconfig` command, or adding a
`sdkconfig.defaults` file to your project.

Below an example configuration is presented. It adds a locally stored custom certificate.
```
#
# Certificate Bundle
#
CONFIG_MBEDTLS_CERTIFICATE_BUNDLE=y
# CONFIG_MBEDTLS_CERTIFICATE_BUNDLE_DEFAULT_FULL is not set
# CONFIG_MBEDTLS_CERTIFICATE_BUNDLE_DEFAULT_CMN is not set
CONFIG_MBEDTLS_CERTIFICATE_BUNDLE_DEFAULT_NONE=y
CONFIG_MBEDTLS_CUSTOM_CERTIFICATE_BUNDLE=y
CONFIG_MBEDTLS_CUSTOM_CERTIFICATE_BUNDLE_PATH="/path/to/certificate/file/astarte_instance.pem"
# end of Certificate Bundle
```

## Notes on BSON (de)serialization

The data exchange with an Astarte instance is encoded in the
[BSON format](https://bsonspec.org/spec.html).
This library provides utility functions for serializing and deserializing BSONs. Handling BSON
directly is only required when dealing with interfaces with object aggregation. Sending and
receiving on individually aggregated interfaces can be done using standard C types and structures.

Note that not all the element types defined by the BSON 1.1 specification are supported by Astarte.
The subset of the supported element types can be found in the
[Astarte MQTT v1](https://docs.astarte-platform.org/latest/080-mqtt-v1-protocol.html) protocol
specification.

### The serialization utility

Serializing data to a BSON format should be used during transmission of aggregates to Astarte.
The serialization utility is exposed by the `astarte_bson_serializer.h` header.

A BSON object can be created using the `astarte_bson_serializer_new` function. Which will return an
empty BSON document that can be filled by appending new elements using one of the
`astarte_bson_serializer_append_*` functions. Once all the required elements have been added to the
document it must be terminated using the `astarte_bson_serializer_append_end_of_document` function.
The serialized document can then be extracted using the `astarte_bson_serializer_get_document`
function. This function will return a buffer that will live for as long as the BSON object.

Since the memory for the BSON document is dynamically allocated it should be freed once the document
is no longer required. This can be done with the `astarte_bson_serializer_get_document` function.

A simple usage example is reported here.
```C
#include "astarte_bson_serializer.h"

astarte_bson_serializer_handle_t bson = astarte_bson_serializer_new();
astarte_bson_serializer_append_double(bson, "co2", 4.0);
astarte_bson_serializer_append_int64(bson, "temperature", 42);
astarte_bson_serializer_append_string(bson, "identifier", "si383o33dm2");
astarte_bson_serializer_append_end_of_document(bson);

int serialized_doc_size;
const void *serialized_doc = astarte_bson_serializer_get_document(bson, &serialized_doc_size);

// Use the returned serialized document before destroy call
// For example send the buffer to Astarte by calling astarte_device_stream_aggregate()

astarte_bson_serializer_destroy(bson);
```

### The deserialization utility

Deserializing data from the BSON format is required when receiving aggregates from Astarte.
The deserialization utility is exposed by the `astarte_bson_deserializer.h` header.

A BSON object can be initialized from a buffer containing serialized data using the function
`astarte_bson_deserializer_init_doc`. This will return a document object of type
`astarte_bson_document_t`, which provides information regarding the document such as its
total size.

Accessing the list of elements in the BSON document can be performed
in two ways. By looping over all the elements using the `astarte_bson_deserializer_first_element`
and `astarte_bson_deserializer_next_element` functions or, when the required element name is known,
by lookup using the function `astarte_bson_deserializer_element_lookup`. Both functions return an
element object of the type `astarte_bson_element_t`, which provides information regarding the
element such as its type and name.

To extract the data contained in a single element one of the
`astarte_bson_deserializer_element_to_*` functions should be used.

A common use case of the deserializer is in the Astarte `data_event_callback`.
This function exposes the received data though the member `bson_element` of the parameter struct
`event`. The `bson_element` variable is a BSON element object of the type `astarte_bson_element_t`.

The BSON deserializer utility will not perform dynamic allocation of memory but will use directly
the buffer passed during initialization. Make sure the buffer remains valid throughout the
deserialization process.

Two deserialization examples are reported here.
The first one is a very simple deserialization example for a BSON containing a single element of
double type.
```C
#include "astarte_bson_deserializer.h"

astarte_bson_document_t doc = astarte_bson_deserializer_init_doc(input_buffer);

astarte_bson_element_t element;
astarte_err_t astarte_err = astarte_bson_deserializer_element_lookup(doc, "name", &element);
if ((astarte_err != ASTARTE_OK) || (element.type != BSON_TYPE_DOUBLE)) {
    abort();
}
double element_value = astarte_bson_deserializer_element_to_double(element);
```

The second is a more complex example of a document containing an array of elements of type
double and a single boolean element.
```C
#include "astarte_bson_deserializer.h"

astarte_err_t astarte_err;

// Initializing the document from a raw buffer
astarte_bson_document_t doc = astarte_bson_deserializer_init_doc(input_buffer);

// Extracting the element with name `boolean` using lookup
astarte_bson_element_t element_bool;
astarte_err = astarte_bson_deserializer_element_lookup(doc, "boolean", &element_bool);
if ((astarte_err != ASTARTE_OK) || (element_bool.type != BSON_TYPE_BOOLEAN)) {
    abort();
}
bool value_bool = astarte_bson_deserializer_element_to_bool(element_bool);

// Extracting the element with name `array` using lookup
astarte_bson_element_t element_arr;
astarte_err = astarte_bson_deserializer_element_lookup(doc, "array", &element_arr);
if ((astarte_err != ASTARTE_OK) || (element_arr.type != BSON_TYPE_ARRAY)) {
    abort();
}
// Arrays are just special documents in the BSON specification
astarte_bson_document_t arr_doc = astarte_bson_deserializer_element_to_document(element_arr);

// Extract the first element
astarte_bson_element_t element_arr_0;
astarte_err = astarte_bson_deserializer_first_element(arr_doc, &element_arr_0);
if ((astarte_err != ASTARTE_OK) || (element_arr_0.type != BSON_TYPE_DOUBLE)) {
    abort();
}
double element_arr_0_value = astarte_bson_deserializer_element_to_double(element_arr_0);

// Extract each of the successive elements
astarte_bson_element_t element_arr_prev = element_arr_0;
astarte_bson_element_t element_arr_next;
while (1) {
    astarte_err = astarte_bson_deserializer_next_element(
                        arr_doc, element_arr_prev, &element_arr_next);
    if (astarte_err == ASTARTE_ERR_NOT_FOUND) {
        break;
    }
    if ((astarte_err != ASTARTE_OK) || (element_arr_next.type != BSON_TYPE_DOUBLE)) {
        abort();
    }

    double element_arr_X_value = astarte_bson_deserializer_element_to_double(element_arr_next);

    element_arr_prev = element_arr_next;
}
```
