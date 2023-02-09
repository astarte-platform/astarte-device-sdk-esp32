# Astarte ESP32 SDK

Astarte ESP32 SDK lets you connect your ESP32 device to
[Astarte](https://github.com/astarte-platform/astarte).

The SDK simplifies all the low level operations like credentials generation, pairing and so on, and
exposes an high-level API to publish data from your device.

Have a look at the [`toggle_led` example](examples/toggle_led) for a usage example showing how to
send and receive data.

## Documentation

The generated Doxygen documentation is available in the [Astarte Documentation
website](https://docs.astarte-platform.org/1.0/device-sdks/esp32).

## `esp-idf` version compatibility

The SDK is tested against these versions of `esp-idf`:
- `v3.3.4` (only `make` supported)
- `v4.2` (`make` and `idf.py` supported)
- `v5.0` (`idf.py` supported)

Previous versions of `esp-idf` are not guaranteed to work. If you find a problem using a later
version of `esp-idf`, please [open an
issue](https://github.com/astarte-platform/astarte-device-sdk-esp32/issues).
