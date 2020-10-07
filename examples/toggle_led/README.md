# Astarte LED Toggle example

## Usage
First of all, you need an Astarte instance to send data to.

If you don't have one, you can either follow the [Astarte in 5 minutes](https://docs.astarte-platform.org/latest/010-astarte_in_5_minutes.html)
guide or deploy it to you favorite cloud provider using our [Kubernetes Operator](https://github.com/astarte-platform/astarte-kubernetes-operator).

When you have your realm ready, you should [install the interfaces](https://docs.astarte-platform.org/latest/010-astarte_in_5_minutes.html#install-an-interface)
contained in the `interfaces` folder.

After that, you have to generate a Pairing JWT using the `generate-astarte-credentials` script
contained in [Astarte's main repo](https://github.com/astarte-platform/astarte).

You can do it with:
```
./generate-astarte-credentials -t pairing -p <your realm key>
```

Now you have all that you need to configure and run the example.

Run
```
make menuconfig
```
In the `Astarte toggle LED example` submenu, configure your Wifi and the button and LED GPIO
numbers (check your board schematics).

In the `Component config -> Astarte SDK` submenu configure your realm name, the Astarte Pairing
base URL and the Pairing JWT you've generated before. If you have deployed Astarte through docker-compose,
the Astarte Pairing base URL is http://localhost:4003. On a common, standard installation, the base URL
can be built by adding /pairing to your API base URL.

After configuring the example, run
```
make -j4 flash monitor
```

The device should now register to Astarte, printing its device id.

Now if you press the button, you should be able to see data using
```
curl -X GET https://<your API base URL>/appengine/v1/<your realm>/devices/<your device id>/interfaces/org.astarteplatform.esp32.DeviceDatastream -H "Authorization: Bearer $(./generate-astarte-credentials -t appengine -p <your realm key>)"
```

and you should also be able to switch the LED on and off with
```
curl -X POST https://<your API base URL>/appengine/v1/<your realm>/devices/<your device id>/interfaces/org.astarteplatform.esp32.ServerDatastream/led -H "Authorization: Bearer $(./generate-astarte-credentials -t appengine -p <your realm key>)" -H "Content-Type: application/json" -d '{"data": true}'
```
(use `true` to turn the LED on, `false` to turn the LED off).
