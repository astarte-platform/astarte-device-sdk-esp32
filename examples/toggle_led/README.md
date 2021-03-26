# Astarte LED Toggle example

## Usage
First of all, you need an Astarte instance to send data to.

If you don't have one, you can either follow the [Astarte in 5
minutes](https://docs.astarte-platform.org/latest/010-astarte_in_5_minutes.html) guide or deploy it
to you favorite cloud provider using our [Kubernetes
Operator](https://github.com/astarte-platform/astarte-kubernetes-operator).

When you have your realm ready, you should [install the
interfaces](https://docs.astarte-platform.org/latest/010-astarte_in_5_minutes.html#install-an-interface)
contained in the `interfaces` folder.

After that, you have to generate a Pairing JWT using
[`astartectl`](https://github.com/astarte-platform/astartectl).

You can do it with:

```
astartectl utils gen-jwt pairing -e0 -k <private-key-pem>
```

Now you have all that you need to configure and run the example.

Run

```
make menuconfig
```

In the `Astarte toggle LED example` submenu, configure your Wifi and the button and LED GPIO
numbers (check your board schematics).

In the `Component config -> Astarte SDK` submenu configure your realm name, the Astarte Pairing base
URL and the Pairing JWT you've generated before. If you have deployed Astarte through
docker-compose, the Astarte Pairing base URL is http://<your-machine-url>:4003. On a common,
standard installation, the base URL can be built by adding `/pairing` to your API base URL, e.g.
`https://api.astarte.example.com/pairing`.

After configuring the example, run

```
make -j4 flash monitor
```

The device should now register to Astarte, printing its device id, and you can verify its connection
status using:

```
astartectl appengine devices show <device-id> \
  -r <realm> -k <private-key> -u <astarte-base-url>
```

Now if you press the button, you should be able to see data using

```
astartectl appengine devices get-samples <device-id> \
  org.astarteplatform.esp32.DeviceDatastream /button \
  -r <realm> -k <private-key> -u <astarte-base-url>

astartectl appengine devices get-samples <device-id> \
  org.astarteplatform.esp32.DeviceDatastream /uptimeSeconds \
  -r <realm> -k <private-key> -u <astarte-base-url>
```

and you should also be able to switch the LED on and off with

```
astartectl appengine devices send-data <device-id> \
  org.astarteplatform.esp32.ServerDatastream /led true \
  -r <realm> -k <private-key> -u <astarte-base-url>
```

(use `true` to turn the LED on, `false` to turn the LED off).
