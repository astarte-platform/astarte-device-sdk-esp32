<!---
  Copyright 2018-2023 SECO Mind Srl

  SPDX-License-Identifier: LGPL-2.1-or-later OR Apache-2.0
-->

# Astarte datastreams example

This example will guide you through setting up a simple ESP32 application running the Astarte SDK
and interacting with a local instance of Astarte.

The example device will perform the following actions:
- Connect to a local Astarte instance using a manually generated device ID and credential secret.
- When the server will publish new values for the server-owned interface, the device will react
by publishing new values on the device-owned interface.
Specifically, the device will invert the payload of the `/question` endpoint and publish it to the
`/answer` endpoint.

We will assume you already performed the common configuration and satisfy the common prerequisites.
You flashed your device with the example firmware and connection between the device and the Astarte
instance has been established.

## Send and receive data

Now that our device is connected to Astarte it's time to start publishing data.

We will use two sub-commands of `astartectl` in this example, `send-data`, and `get-samples`.
With `send-data` we can publish new values on a server-owned interface.
The syntax is the following:
```
astartectl appengine --appengine-url http://localhost:4002/ --realm-key <REALM>_private.pem \
    --realm-name <REALM> devices send-data <DEVICE_ID>                                      \
    --interface-type individual-datastream --payload-type boolean                           \
    org.astarteplatform.esp32.examples.ServerDatastream /question <VALUE>
```
Where `<TOKEN>` is the shell variable containing the token, `<REALM_NAME>` is your realm name, and
`<API_URL>` is the API URL as found on the astarte cloud console. `<DEVICE_NAME>` is the device ID
to send the data to, and `<VALUE>` is the value to send. In this example the data to send is in
boolean format, so substitute `<VALUE>` with `true` or `false`.

The device will react to this event by publishing some values on its interface. The value
published in this example will correspond to the negation of the received value.

We can check if the publish from the device has been correctly received on the server using
`get-samples` as follows:
```
astartectl appengine --appengine-url http://localhost:4002/ --realm-key <REALM>_private.pem \
    --realm-name <REALM> devices get-samples <DEVICE_ID>                                    \
    org.astarteplatform.esp32.examples.DeviceDatastream /answer -c 1
```
This will print the latest published data from the device that has been stored in the Astarte Cloud
instance.
To print a longer history of the published data change the number in `-c 1` to the history length
of your liking.
