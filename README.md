<!---
  Copyright 2018-2023 SECO Mind Srl

  SPDX-License-Identifier: LGPL-2.1-or-later OR Apache-2.0
-->

# Astarte device SDK for ESP32

The Astarte device SDK for ESP32 lets you connect your ESP32 device to an instance of
[Astarte](https://github.com/astarte-platform/astarte).

The SDK exposes a high-level API to publish data from your device, simplifying all the low-level
operations such as credentials generation, pairing, and so on.

This component is available on the
[ESP registry](https://components.espressif.com/components/astarte-platform/astarte-device-sdk-esp32)
where users can find some example applications.

## Documentation

The generated Doxygen documentation is available in the [Astarte Documentation
website](https://docs.astarte-platform.org/device-sdks/esp32/latest/api).

## ESP IDF version compatibility

The SDK is compatibile with the [`esp-idf`](https://github.com/espressif/esp-idf) toolchain from
version `5.1` and superior.
Previous versions of `esp-idf` are not supported.

If you find a problem using a supported version of the `esp-idf`, please
[open an issue](https://github.com/astarte-platform/astarte-device-sdk-esp32/issues).

## Notes on custom certificate bundle

The Astarte device SDK for ESP32 uses the
[ESP x509 Certificate Bundle](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/esp_crt_bundle.html)
to verify the Astarte instance certificates.

If your Astarte instance uses a custom CA you can disable the default certificate bundle and add
your own certificates.
This can be done directly in `esp-idf` using the `menuconfig` command, or adding a
`sdkconfig.defaults` file to your project.

Below an example configuration is presented. It adds a locally stored custom certificate.
```kconfig
#
# Certificate Bundle
#
CONFIG_MBEDTLS_CERTIFICATE_BUNDLE=y
# CONFIG_MBEDTLS_CERTIFICATE_BUNDLE_DEFAULT_FULL is not set
# CONFIG_MBEDTLS_CERTIFICATE_BUNDLE_DEFAULT_CMN is not set
CONFIG_MBEDTLS_CERTIFICATE_BUNDLE_DEFAULT_NONE=y
CONFIG_MBEDTLS_CUSTOM_CERTIFICATE_BUNDLE=y
CONFIG_MBEDTLS_CUSTOM_CERTIFICATE_BUNDLE_PATH="/path/to/certificate/file/astarte_instance.pem"
# end of Certificate Bundle
```
