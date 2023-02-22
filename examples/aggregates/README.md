# Astarte Aggregates example

This example will guide you through setting up a simple ESP32 application running the Astarte SDK
and interacting with a remote instance of Astarte.

We will configure the device to perform the following actions:
- Connect to a remote Astarte instance using a manually generated Device ID and credential secret.
- When new aggregated object values are published for a server owned interface, then the device will
publish new aggregated object values on the device owned interface.

If you want to start using a local instance of Astarte hosted on your machine and register the
device automatically you can check out the [`toggle_led` example](../toggle_led) in this repository.

## Prerequisites

Three elements are required to run this example:
- An installation of the [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/).
- A remote instance of Astarte. We will assume you created your own realm on
[Astarte Cloud](https://astarte.cloud/) and you will be managing it through the
[Astarte Dashboard](https://github.com/astarte-platform/astarte-dashboard).
- The [astartectl](https://github.com/astarte-platform/astartectl/releases) tool. This tool is an
alternative to Astarte Dashboard to manage your remote Astarte instance. It includes some additional
functionality used in this example.

## Installing the interfaces on Astarte

We shall start by installing the required interfaces on the remote Astarte instance.

To install a new interface using the Astarte Dashboard follow this sequence of steps:
1. In the *Interfaces* tab click on *Install a new interface ...*
2. In the interface editor fill in the required information for your interface. If you already have
a interface definition in the `.json` format you can paste that directly in the editor at the
right of the page.
3. Finish by clicking on install interface.
4. Refresh the page to check that the interface has been installed correctly.

Install two interfaces using these two `.json` files:
- `interfaces/org.astarteplatform.esp32.examples.DeviceAggregate.json`
- `interfaces/org.astarteplatform.esp32.examples.ServerAggregate.json`

## Manually registering a new device on Astarte

Next, let's register a new device on the remote Astarte instance.

The following procedure shows how to do this:
1. In the *Devices* tab click on *Register a new device*
2. Click *Generate random ID* and then on *Register device*
3. Copy the device credential secret somewhere local

**NB**: While the device ID can always be recovered from the Astarte Dashboard, the
credential secred is only shown once during the device registration procedure.
Take care of storing it somewhere local as it will be required in the next
step.

## Configuring the SDK

We will assume you are running the ESP-IDF from command line. If you are using the GUIs provided
with the Visual Studio Code or the Eclipse plugins then the configuration procedure might be
slightly different.

Open the configuration tool using the following command:
| ESP-IDF $\geqslant$ v4.x | ESP-IDF v3.x |
| ------------- | ------------ |
| `idf.py menuconfig` | `make menuconfig` |

In the *Astarte question/answer example* tab fill in:
- *WiFi SSID* the SSID of your wifi network
- *WiFi Password* the SSID of your wifi network
- *Credentials secret* the device specific credential secret generated in the previous step
- *Device hardware ID* the device hardware ID generated in the previous step

In the *Component config ---> Astarte SDK* tab fill in:
- *Astarte realm* place here the name of your Astarte realm
- *Astarte pairing base URL* use the Base API URL for your remote Astarte instance adding at the
end `/pairing` (e.g.: if you are using Astarte Cloud: https://api.eu1.astarte.cloud/pairing).

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

Once the example has been flashed it will automatically attempt to connect the device to the remote
instance of Astarte.
Verify this by using the monitor functionality of the ESP-IDF to observe the log messages of the
ESP32.
After connection has been accomplished, the following ESP32 log message should be shown:
```
ASTARTE_EXAMPLE_AGGREGATE: Astarte device connected, session_present: 0
```
You can see how the connection has been achieved also from the Astarte Dashboard by opening the
newly created device and observing the status that should have changed to *Connected*.

## Send and receive data from the device

Now that our device is connected to Astarte is time to start publishing data.
We can interact with the remote Astarte instance using the `astartectl` tool.

**NB** `astartectl` needs an access token to interact with the remote Astarte instance. We can find
this token in the *Astarte Cloud Console* (the page on Astarte Cloud where all your realms are
shown) in the *Realm Information* page. The token is presented as a command that will store it in a
shell variable. Something similar to: `TOKEN="token_content"`.

We will use two commands in this example, `send-data` and `get-samples`.
With `send-data` we can publish new values for a server owned interface.
The syntax is the following:
```
astartectl --astarte-url "<API_URL>" --token "<TOKEN>" appengine --realm-name "<REALM_NAME>" \
    devices send-data <DEVICE_NAME>                                                          \
    org.astarteplatform.esp32.examples.ServerAggregate /11                                   \
    '{"longinteger_endpoint":45993543534, "booleanarray_endpoint":[false,false,true,true]}'
```
Where `<TOKEN>` is the shell variable containing the token, `<REALM_NAME>` is your realm's name and
`<API_URL>` is the API's url as found on astarte cloud console. `<DEVICE_NAME>` is the device ID to
send the data to. The last argument is the data to publish.

The device will react to this publish by publishing some values on its interface. The value
published in this example will correspond to the negation of the received value.

We can check if the publish from the device has been correctly received on the server using
`get-samples` as follows:
```
astartectl --astarte-url "<API_URL>" --token "<TOKEN>" appengine --realm-name "<REALM_NAME>" \
    devices get-samples <DEVICE_NAME>                                                        \
    org.astarteplatform.esp32.examples.DeviceAggregate /24 -c 1
```
This will print the latest published data from the device that has been stored in the Astarte Cloud
instance.
To print a longer history of the published data change the number in `-c 1` to the wanted history
length.
