<!---
  Copyright 2018-2023 SECO Mind Srl

  SPDX-License-Identifier: LGPL-2.1-or-later OR Apache-2.0
-->

# Astarte properties example

This example will guide you through setting up a simple ESP32 application running the Astarte SDK
and interacting with a local instance of Astarte.

The example device will perform the following actions:
- Connect to a local Astarte instance using a manually generated device ID and credential secret.
- When the server will set a new value for any of the server-owned properties, the device will react
by setting new values for some of the device-owned properties.

We will assume you already performed the common configuration and satisfy the common prerequisites.
You flashed your device with the example firmware and connection between the device and the Astarte
instance has been established.

## Send and receive data

We will use two sub-commands of `astartectl` in this example, `send-data`, and `get-samples`.
With `send-data` we can publish new values on a server-owned interface.
The syntax is the following:
```
astartectl appengine --appengine-url http://localhost:4002/ --realm-key <REALM>_private.pem \
    --realm-name <REALM> devices send-data <DEVICE_ID>                                      \
    --interface-type individual-properties --payload-type boolean                           \
    org.astarteplatform.esp32.examples.ServerProperties1 /42/boolean_endpoint <VALUE>
```
Where `<TOKEN>` is the shell variable containing the token, `<REALM_NAME>` is your realm name, and
`<API_URL>` is the API URL as found on the astarte cloud console. `<DEVICE_NAME>` is the device ID
to send the data to, and `<VALUE>` is the value to send. In this example the data to send is in
boolean format, so substitute `<VALUE>` with `true` or `false`.

The device will react to this event by mirroring the received value on a device owned interface.

We can check if the publish from the device has been correctly received on the server using
`get-samples` as follows:
```
astartectl appengine --appengine-url http://localhost:4002/ --realm-key <REALM>_private.pem \
    --realm-name <REALM> devices get-samples <DEVICE_ID>                                    \
    org.astarteplatform.esp32.examples.DeviceProperties1 /24/boolean_endpoint -c 1
```
This will print the latest published data from the device that has been stored in the Astarte Cloud
instance.
To print a longer history of the published data change the number in `-c 1` to the history length
of your liking.

## Check properties persistency

Checking that the set propery has been correctly stored can be done by commenting out the
following line in `example_task.c`, then re-building and flashing the project using:
`idf.py build flash monitor`.
```C
#define REPLY_TO_PROPERTY
```

This will disable the set function in the source code without erasing the NVS.
Power cycling the device will cause it to send the device owned property to Astarte.
