<!---
  Copyright 2023 SECO Mind Srl

  SPDX-License-Identifier: LGPL-2.1-or-later OR Apache-2.0
-->

# Astarte encryption example

This example will guide you through setting up a simple ESP32 application running the Astarte SDK
configured to use an NVS partition for persistent storage and with flash encryption enabled.

This example will assume you have already registered manually the device to your Astarte instance.
If you whish to perform on board registration through a JWT token then check out the `registration`
example.

## Partition table

The partition table is the same as the one used in the `non_volatile_storage` example.
The only difference is that encrypted partitions are explicitly marked as such.

## Enabling encryption

Encryption is generally enabled through `idf.py menuconfig`. For this example we provide a
`sdkconfig.defaults` containing a pre-set configuration for encryption. This configuration is
specifically intended for development purposes. It is not safe and should not be used in
production.

## First flash of a previously unencrypted device

Flashing to a previously unencrypted device is the same as flashing non encrypted data.
Use the standard command `idf.py flash`.

## Subsequent flashes of encrypted device

Once a device has been flashed with encrypted firmware the standard `idf.py flash` command will
not work for flashing firmware.
The two commands `idf.py encrypted-app-flash` and `idf.py encrypted-flash` can be used instead.
