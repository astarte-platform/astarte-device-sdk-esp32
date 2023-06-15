<!--
Copyright 2023 SECO Mind Srl

SPDX-License-Identifier: Apache-2.0
-->

# Get started using the ESP32 Astarte Device SDK

The following examples are available to get you started with the ESP32 Astarte Device SDK:
- [Datastreams](./datastreams/README.md): shows how to connect a manually registered device to a
 local instance of Astarte and how to send/receive individual datastreams on the device.
- [Aggregates](./aggregates/README.md): shows how to connect a manually registered device to a
 local instance of Astarte and how to send/receive aggregated datastreams on the device.
- [Toggle led](./toggle_led/README.md): shows how to auto-register a device using the provided API
 to a local instance of Astarte and how to control the integrated LED on a device from Astarte.

# Common prerequisites

All the examples above have some common prerequisites:
- An up-to-date version of the
[ESP IDF SDK](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/index.html).
- A local instance of Astarte. See
[Astarte in 5 minutes](https://docs.astarte-platform.org/latest/010-astarte_in_5_minutes.html) for a
quick way to set up Astarte on your machine.
- The [astartectl](https://github.com/astarte-platform/astartectl/releases) tool. We will use
`astartectl` to manage the Astarte instance.

**N.B.** When installing Astarte using *Astarte in 5 minutes* perform all the installation steps
until right before the *installing the interfaces* step.

# Common configuration

Some common configuration is required for all the examples.

## Installing the interfaces on Astarte

An interface can be installed by running the following command:

```
astartectl realm-management                       \
    --realm-management-url http://localhost:4000/ \
    --realm-key <REALM>_private.pem               \
    --realm-name <REALM>                          \
    interfaces install <INTERFACE_FILE_PATH>
```
Where `<REALM>` is the name of the realm, and `<INTERFACE_FILE_PATH>` is the path name to the
`.json` file containing the interface description.
We assume you are running this command from the Astarte installation folder. If you would like to
run it from another location provide the full path to the realm key.

Each example contains an `/interfaces` folder. To run that example install all the interfaces
contained in the `.json` files in that folder.

## Registering a new device on Astarte (only when manually registering a device)

To manually register the device on the Astarte instance you can use the following `astartectl`
command:
```
astartectl pairing                       \
    --pairing-url http://localhost:4003/ \
    --realm-key <REALM>_private.pem      \
    --realm-name <REALM>                 \
    agent register <DEVICE_ID>
```
**NB**: The credential secret is only shown once during the device registration procedure.

## Generating the Pairing JWT (only when auto registering a device)

We will now generate a Parining JWT from our Astarte local instance. This JWT will be used
by the device during the initial device registration.

The command to generate a Pairing JWT is:
```
astartectl utils gen-jwt --private-key <REALM_NAME>_private.pem pairing --expiry 0
```
This will generate a never expiring token. To generate a token with an expiration date, then change
`--expiry 0` to `--expiry <SEC>` with `<SEC>` the number of seconds your token should last.

## Configure Astarte to be accessible

We will assume that the machine running the Astarte local instance and the ESP32 are connected
to the same local network.

First, identify your local IP address (on Linux run `ip addr`).
Then, in the folder of the Astarte installation, locate the file: `compose.env`.
Open it and modify the following line:

`PAIRING_BROKER_URL=mqtts://localhost:8883/` to `PAIRING_BROKER_URL=mqtts://<MY_IP_ADDR>:8883/`

Also, change the following line:

`VERNEMQ_ENABLE_SSL_LISTENER=true` to `VERNEMQ_ENABLE_SSL_LISTENER=false`

After these changes, you will have to restart your astarte instance for them to take effect.

## Configuring the SDK

We will assume you are running the ESP-IDF from the command line. If you are using the GUIs provided
with the Visual Studio Code or the Eclipse plugins then the configuration procedure might be
slightly different.

Open the configuration tool using the following command:
| idf.py | make |
| ------------- | ------------ |
| `idf.py menuconfig` | `make menuconfig` |

In the *Astarte [..] example* tab fill in:
- *WiFi SSID* - the SSID of your wifi network.
- *WiFi Password* - the SSID of your wifi network.
- *Credentials secret* - (only for manual device registration) the device-specific credential secret
generated in the previous step.
- *Device hardware ID* - (only for manual device registration) the device hardware ID generated in
the previous step.
- *Button GPIO number* - (only for toggle led example) the number of the GPIO connected to the
button.
- *LED GPIO number* - (only for toggle led example) the number of the GPIO connected to the LED.

In the *Component config ---> Astarte SDK* tab fill in:
- *Astarte realm* - place here the name of your Astarte realm.
- *Pairing JWT token* - (only for automatic device registration) the pairing token generated in the
previous step.
- *Astarte pairing base URL* - use the Base API URL for your local Astarte instance.
(e.g.: `http://<MY_IP_ADDR>:4003`).

**NB**: This will place the above-mentioned properties in the ESP-IDF sdk project configuration.
Cleaning the configuration will also reset them to their default values. If you want to store them
more permanently you can overwrite the defaults by adding them to the `sdkconfig.defaults` file.

# Wiping clean the device

When using a non-brand new device, some certificates might be already stored in data flash. Make
sure you correctly wipe clean the device by running:
| idf.py (ESP-IDF v5.x) | idf.py (ESP-IDF v4.x) | make |
| ------------- | ------------ | ------------ |
| `idf.py -p <DEVICE_PORT> erase-flash` | `idf.py -p <DEVICE_PORT> erase_flash` | `make ESPPORT=<DEVICE_PORT> erase_flash` |

# Building and flashing

Now build and flash the device using the ESP-IDF utilities. You can build, flash and then open a
monitor using:
| idf.py | make |
| ------------- | ------------ |
| `idf.py -p <DEVICE_PORT> build flash monitor` | `make ESPPORT=<DEVICE_PORT> flash monitor` |

## Device connection to Astarte

Once the example has been flashed it will automatically attempt to connect the device to the local
instance of Astarte.
Verify this by using the monitor functionality of the ESP-IDF to observe the log messages of the
ESP32.
After the connection has been accomplished, the following ESP32 log message should be shown:
```
ASTARTE_EXAMPLE_<EXAMPLE_NAME>: Astarte device connected, session_present: 0
```

You can check the device has been correctly registered and is connected to the Astarte instance
using `astartectl`.
To list the all registered devices run:
```
astartectl appengine --appengine-url http://localhost:4002/ \
    --realm-key <REALM>_private.pem --realm-name <REALM>    \
    devices list
```
You can check the status of a specific device with the command:
```
astartectl appengine --appengine-url http://localhost:4002/ \
    --realm-key <REALM>_private.pem --realm-name <REALM>    \
    devices show <DEVICE_ID>
```
