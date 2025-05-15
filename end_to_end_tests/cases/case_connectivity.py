# (C) Copyright 2025, SECO Mind Srl
#
# SPDX-License-Identifier: Apache-2.0

from action import (
    TestActionCheckDeviceStatus,
    TestActionConnect,
    TestActionDisconnect,
    TestActionSleep,
)
from case import TestCase

test_case_connectivity = TestCase(
    "connectivity",
    [
        TestActionConnect(timeout=30),
        TestActionSleep(seconds=2),
        TestActionCheckDeviceStatus(
            connected=True,
            introspection=[
                "org.astarteplatform.end-to-end.DeviceAggregate",
                "org.astarteplatform.end-to-end.DeviceDatastream",
                "org.astarteplatform.end-to-end.DeviceProperty",
                "org.astarteplatform.end-to-end.ServerAggregate",
                "org.astarteplatform.end-to-end.ServerDatastream",
                "org.astarteplatform.end-to-end.ServerProperty",
            ],
        ),
        TestActionSleep(seconds=2),
        TestActionDisconnect(timeout=30),
        TestActionSleep(seconds=2),
    ],
)
