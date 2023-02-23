# Astarte Aggregates example

This example will guide you through setting up a simple ESP32 application running the Astarte SDK
and interacting with a local instance of Astarte.

The example device will perform the following actions:
- Connect to a local Astarte instance using a manually generated Device ID and credential secret.
- When new aggregated object values are published on a server-owned interface, then the device will
publish new aggregated object values on the device-owned interface.

We will assume you already performed the common configuration and satisfy the common prerequisites.
You flashed your device with the example firmware and connection between the device and the Astarte
instance has been established.

## Send and receive data from the device

Now that our device is connected to Astarte it's time to start publishing data.

We will use two sub-commands of `astartectl` in this example, `send-data`, and `get-samples`.
With `send-data` we can publish new values on a server-owned interface.
The syntax is the following:

```
astartectl appengine                                                                 \
--appengine-url http://localhost:4002/ --realm-management-url http://localhost:4000/ \
--realm-key <REALM_NAME>_private.pem --realm-name <REALM_NAME>                       \
devices send-data <DEVICE_NAME>                                                      \
org.astarteplatform.esp32.examples.ServerAggregate /11                               \
'{"longinteger_endpoint":45993543534, "booleanarray_endpoint":[false,false,true,true]}'
```
Where `<REALM_NAME>` is your realm name,`<DEVICE_NAME>` is the device ID to send the data to.
The last argument is the data to publish.

The device will react to this event by publishing some values on its interface.

We can check if the publish from the device has been correctly received on the server using
`get-samples` as follows:

```
astartectl appengine                                                                 \
--appengine-url http://localhost:4002/ --realm-management-url http://localhost:4000/ \
--realm-key <REALM_NAME>_private.pem --realm-name <REALM_NAME>                       \
devices get-samples <DEVICE_NAME>                                                    \
org.astarteplatform.esp32.examples.DeviceAggregate /24 -c 1
```
This will print the latest published data from the device that has been stored in the Astarte Cloud
instance.
To print a longer history of the published data change the number in `-c 1` to the history length
of your liking.
