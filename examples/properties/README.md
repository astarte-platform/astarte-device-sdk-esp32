<!---
  Copyright 2023 SECO Mind Srl

  SPDX-License-Identifier: LGPL-2.1-or-later OR Apache-2.0
-->

# Astarte properties example

This example will guide you through setting up a simple ESP32 application running the Astarte SDK
and interacting with a local instance of Astarte.

The example device will, by default perform the following actions:
- Connect to a local Astarte instance using a manually generated device ID and credential secret.
- Set some predefined device owned properties to predefined values.
- When the server will set a new value for any of the server-owned properties, the device will react
by setting new values for some of the device-owned properties.

We will assume you already performed the common configuration and satisfy the common prerequisites.
You flashed your device with the example firmware and connection between the device and the Astarte
instance has been established.

## Interaction witt the Astarte instance

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

The command `get-samples` can be also used to check if the device has set correctly its device owned
properties.

## Additional example configuration

One of the main use cases for this example is to observe how property retention works between
different reboots of a device.
For this reason, a set of preprocessor defines has been placed at the beginning of the file.
By enabling or disabling them more complex behaviors can be simulated.

One important consideration for this purpose is that using `idf.py build flash monitor` will not
change the content of the NVS flash partition where the properties are stored. The example can
be built and flashed over the device with different configurations without changing the set of
properties already stored.

Here is a short description of each define and what it does:
- `#define SET_DEVICE_PROPERTIES`: set a few device-owned properties with predefined values
- `#define UNSET_DEVICE_PROPERTIES`: unset a few device-owned properties
- `#define SET_DEVICE_PROPERTY_TWICE`: try to set one device-owned property twice in a row with the
same value
- `#define REPLY_TO_SERVER_PROPERTY_INTERFACE1`: upon reception of a set message for a specific
server property of interface 1, react setting a device property for the same interface.
- `#define REPLY_TO_SERVER_PROPERTY_INTERFACE2`: upon reception of a set message for a specific
server property of interface 2, react setting a device property for the same interface.

One simple behavior can be simulated by first flashing the device with the `SET_DEVICE_PROPERTIES`
define enabled. Observe how the properties are set correctly in the Astarte instance.
Then comment out the preprocessor define, and flash the device once more.
Observe how the device properties remain set even after multiple reboots.
Now enable `UNSET_DEVICE_PROPERTIES`, build and flash the example once more and observe how some
device properties are unset.
If the device is power cycled without flashing the firmware the calls to the unset method will
print a warning as the property is already unset.
