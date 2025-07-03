<!--
Copyright 2025 SECO Mind Srl

SPDX-License-Identifier: Apache-2.0
-->

# Astarte application sample

The sample app in this folder contains code to send all data types supported by Astarte.

The Astarte interfaces for this sample are defined in JSON files contained in the `interfaces`
folder.
In addition to the JSON version of the interfaces, an auto-generated version of the same interfaces
is contained in the `generated_interfaces` header/source files. Those files have been generated
running the `generate_interfaces.py` script contained in the
[`python_scripts`](https://github.com/astarte-platform/astarte-device-sdk-esp32/tree/api-restructuring/python_scripts)
folder and should not be modified manually.

## Configuration

This sample can be configured using the standard `esp-idf` menuconfig tool, or using the
`stdkconfig.defaults` file.

The following fields should be updated to values that match an existing Astarte instance and
a WiFi access point:
```kconfig
CONFIG_WIFI_SSID=
CONFIG_WIFI_PASSWORD=
CONFIG_ASTARTE_DEVICE_SDK_HOSTNAME=
CONFIG_ASTARTE_DEVICE_SDK_REALM_NAME=

CONFIG_DEVICE_ID=
# Disable the credential secret if using the JWT to perform device pairing on the device
CONFIG_CREDENTIAL_SECRET=
# Enable the following line if performing device pairing on the device
# CONFIG_ASTARTE_DEVICE_SDK_PAIRING_JWT=
```
