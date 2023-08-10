<!---
  Copyright 2023 SECO Mind Srl

  SPDX-License-Identifier: LGPL-2.1-or-later OR Apache-2.0
-->

# Astarte non volatile storage example

This example will guide you through setting up a simple ESP32 application running the Astarte SDK
configured to use an NVS partition for persistent storage.

This example will perform the device registration starting from a provided JWT token.

## Partition table

The partition table used by this example is contained in `partitions_example.csv`.
The only change from a regarding the SDK from a standard basic table is the addition of the
`astarte` partition.

Note that this table contains two NVS partitions. A generic `nvs` labelled partition and
the `astarte` labelled partition. The `nvs` partition can be used by some ESP drivers such as the
WIFI driver. We suggest creating a separate partition for the SDK data not to be shared with other
components. This will prevent issues related memory sharing.

Note that the `astarte` partition minimum size is not determined by the SDK memory requirements
but by the minimum NVS partition sizes. Data partitions of NVS type are recommended to have a size
of at least 12kB (0x3000) due to some NVS library limitations.

## Enabling NVS in the example

The following two lines of code are sufficient to instruct the SDK to use the NVS partition
specified.
```C
nvs_flash_init_partition("astarte");
astarte_credentials_use_nvs_storage("astarte");
```
Make sure you call `astarte_credentials_use_nvs_storage` before calling any other credentials
related funcion.
