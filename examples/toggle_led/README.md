# Astarte LED Toggle example

This example will guide you through setting up a simple ESP32 application running the Astarte SDK
and interacting with a local instance of Astarte.

We will configure the device to perform the following actions:
- Auto-register in the local Astarte instance at the first start-up.
- When a message is received from our local instance of Astarte blink an on board LED.
- When a button is pressed on the device transmit to Astarte the uptime in seconds.

If you want to start using a remote instance of Astarte hosted on Astarte Cloud and register the
device manually you can check out the [`question_answer` example](../question_answer) in this
repository.

## Prerequisites

To run this example you will need:
- An installation of the [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/).
- A local instance of Astarte. We will assume you created you created one following the
[Astarte in 5 minutes](https://docs.astarte-platform.org/latest/010-astarte_in_5_minutes.html) guide.
You can stop right before installing the interfaces step.
- The [astartectl](https://github.com/astarte-platform/astartectl/releases) tool. This tool is a
simple tool to manage local or remote Astarte instances.
- An ESP32 board with at least one LED and one button connected to free GPIOs.

**NB.** This example expects you to run all of the `astartectl` commands from the folder containing
the local Astarte instance. Running `astartectl` from another folder is possible but will require to
specify full paths to all the files used below.

## Installing the interfaces on Astarte

We shall install the required interfaces in our local Astarte instance.

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

Install two interfaces using these two `.json` files:
- `interfaces/org.astarteplatform.esp32.examples.DeviceDatastream.json`
- `interfaces/org.astarteplatform.esp32.examples.ServerDatastream.json`

## Generating the Pairing JWT

After that we will generate a Parining JWT from our Astarte local instance. This JWT will be used
by the device during the first device registration.

The command to generate a Pairing JWT is:
```
astartectl utils gen-jwt --private-key <REALM_NAME>_private.pem pairing --expiry 0
```
This will generate a never expiring token. To generate a token with an expiration date, then change
`--expiry 0` to `--expiry <SEC>` with `<SEC>` the number of seconds your token should last.

Copy the generated token somewhere safe, it will be used in the next steps.

## Configure Astarte to be accessible

We will assume that the machine running the Astarte local instance and the ESP32 are connected
to the same local network.

First identify your local IP address (on linux run `ip addr`).
Then, in the folder of the Astarte installation, locate the file: `compose.env`.
Open it and modify the following line:

`PAIRING_BROKER_URL=mqtts://localhost:8883/` to `PAIRING_BROKER_URL=mqtts://<MY_IP_ADDR>:8883/`

Furthermore, change also the following line:

`VERNEMQ_ENABLE_SSL_LISTENER=true` to `VERNEMQ_ENABLE_SSL_LISTENER=false`

After these changes you will have to restart your astarte instance for them to take effect.

## Configuring the SDK

We will assume you are running the ESP-IDF from command line. If you are using the GUIs provided
with the Visual Studio Code or the Eclipse plugins then the configuration procedure might be
slightly different.

Open the configuration tool using the following command:
| ESP-IDF $\geqslant$ v4.x | ESP-IDF v3.x |
| ------------- | ------------ |
| `idf.py menuconfig` | `make menuconfig` |

In the *Astarte toggle LED example* tab fill in:
- *WiFi SSID* the SSID of your wifi network
- *WiFi Password* the SSID of your wifi network
- *Button GPIO number* the number of the GPIO connected to the button
- *LED GPIO number* the number of the GPIO connected to the LED

In the *Component config ---> Astarte SDK* tab fill in:
- *Astarte realm* place here the name of your Astarte realm
- *Astarte pairing base URL* fill in here the base URL with the correct pairing port
`http://<MY_IP_ADDR>:4003`, and then add at the end `/pairing`.
- *Pairing JWT token* place here the pairing token you generated in the previous step.

You can leave the rest as default.

**NB**: This will place the above mentioned properties in the ESP-IDF sdk project configuration.
Cleaning the configuration will also reset them to their default values. If you want to store them
more permanently you can overwrite the defaults by adding them in the `sdkconfig.defaults` file.

## Building and flashing

Once the project has been configured build and flash it using the ESP-IDF utilities.

You can build, flash and then open a monitor using:
| ESP-IDF $\geqslant$ v4.x | ESP-IDF v3.x |
| ------------- | ------------ |
| `idf.py -p <DEVICE_PORT> build flash monitor` | `make ESPPORT=<DEVICE_PORT> flash monitor` |

When re-using an old device that might have been information stored in persistent flash,
clear the flash. From terminal use the command:
| ESP-IDF $\geqslant$ v4.x | ESP-IDF v3.x |
| ------------- | ------------ |
| `idf.py -p <DEVICE_PORT> erase-flash` | `make ESPPORT=<DEVICE_PORT> erase_flash` |

## Device connection to Astarte

Once the example has been flashed it will automatically attempt to register the device and then
connect the device to the local instance of Astarte.
Verify this by using the monitor functionality of the ESP-IDF to observe the log messages of the
ESP32.
After connection has been accomplished, the following ESP32 log message should be shown:
```
ASTARTE_TOGGLE_LED: Astarte device connected, session_present: 0
```

You can check the device has been correcly registered and is connected from the Astarte instance.
To list the all registered devices use:
```
astartectl appengine --appengine-url http://localhost:4002/ \
    --realm-key <REALM>_private.pem --realm-name <REALM>    \
    devices list
```
Then use the device id printed from this command to check the status of the device with the command:
```
astartectl appengine --appengine-url http://localhost:4002/ \
    --realm-key <REALM>_private.pem --realm-name <REALM>    \
    devices show <DEVICE_ID>
```

## Turning on/off the LED

Turning on and off the LED can be accomplished by sending some data from Astarte to the device.
This can be done using the following command:
```
astartectl appengine --appengine-url http://localhost:4002/       \
    --realm-key <REALM>_private.pem --realm-name <REALM>          \
    devices send-data <DEVICE_ID>                                 \
    org.astarteplatform.esp32.examples.ServerDatastream                    \
    --interface-type individual-datastream --payload-type boolean \
     /led <VALUE>
```
The `<VALUE>` should be in the boolean format. Sending `true` will turn the LED on, sending `false`
will turn it off.

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

This will print the latest message from the device that have been stored in the Astarte Cloud
instance. To print a longer history of the message change the number in `--count 1` to the wanted
history length.
