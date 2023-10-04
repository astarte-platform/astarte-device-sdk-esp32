<!---
  Copyright 2023 SECO Mind Srl

  SPDX-License-Identifier: LGPL-2.1-or-later OR Apache-2.0
-->

# Astarte device registration example

This example will guide you through setting up a simple ESP32 application running the Astarte SDK
and interacting with a local instance of Astarte.

We will configure the device to auto-register in the local Astarte instance at the first start-up.

## Checking the connection status of the device in Astarte

`astartectl` can be used to check the connection status of the device on the local Astarte instance:
```
astartectl appengine devices show <DEVICE ID> --appengine-url http://localhost:4002
    -r test -k test_private.pem
```
