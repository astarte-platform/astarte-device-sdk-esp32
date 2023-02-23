# Astarte LED Toggle example

This example will guide you through setting up a simple ESP32 application running the Astarte SDK
and interacting with a local instance of Astarte.

We will configure the device to perform the following actions:
- Auto-register in the local Astarte instance at the first start-up.
- Blink an onboard LED when a message is received from our local instance of Astarte.
- When a button is pressed on the device transmits to Astarte the uptime in seconds.

## Turning on/off the LED

Turning on and off the LED can be accomplished by sending some data from Astarte to the device.
This can be done using the following command:
```
astartectl appengine --appengine-url http://localhost:4002/       \
    --realm-key <REALM>_private.pem --realm-name <REALM>          \
    devices send-data <DEVICE_ID>                                 \
    org.astarteplatform.esp32.examples.ServerDatastream           \
    --interface-type individual-datastream --payload-type boolean \
     /led <VALUE>
```
The `<VALUE>` should be in the boolean format. Sending `true` will turn the LED on, and sending
`false` will turn it off.

## Checking out the stored messages in Astarte

Each time the button is pressed, the device will send its uptime in seconds to the local Astarte
instance.
You can verify the transmitted data has been successfully stored in Astarte by running:
```
astartectl appengine --appengine-url http://localhost:4002/ \
    --realm-key <REALM>_private.pem --realm-name <REALM>    \
    devices get-samples <DEVICE_ID>                         \
    org.astarteplatform.esp32.examples.DeviceDatastream /uptimeSeconds --count 1
```

This will print the latest message from the device that has been stored in the Astarte Cloud
instance. To print a longer history of messages change the number in `--count 1` to the history
length of your liking.
