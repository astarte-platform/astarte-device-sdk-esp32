# (C) Copyright 2025, SECO Mind Srl
#
# SPDX-License-Identifier: Apache-2.0

from datetime import datetime, timezone

from action import (
    TestActionConnect,
    TestActionDisconnect,
    TestActionFetchRESTData,
    TestActionSleep,
    TestActionTransmitMQTTData,
)
from case import TestCase
from qemu_commands import DType

data_binaryblob = (
    "org.astarteplatform.end-to-end.DeviceDatastream",
    "/binaryblob_endpoint",
    b"binblob",
    DType.ASTARTE_MAPPING_TYPE_BINARYBLOB,
    datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
)
data_binaryblobarray = (
    "org.astarteplatform.end-to-end.DeviceDatastream",
    "/binaryblobarray_endpoint",
    [b"bin", b"blob"],
    DType.ASTARTE_MAPPING_TYPE_BINARYBLOBARRAY,
    datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
)
data_boolean = (
    "org.astarteplatform.end-to-end.DeviceDatastream",
    "/boolean_endpoint",
    True,
    DType.ASTARTE_MAPPING_TYPE_BOOLEAN,
    datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
)
data_booleanarray = (
    "org.astarteplatform.end-to-end.DeviceDatastream",
    "/booleanarray_endpoint",
    [True, False, True],
    DType.ASTARTE_MAPPING_TYPE_BOOLEANARRAY,
    datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
)
data_datetime = (
    "org.astarteplatform.end-to-end.DeviceDatastream",
    "/datetime_endpoint",
    datetime(1994, 3, 25, 13, 32, 0, tzinfo=timezone.utc),
    DType.ASTARTE_MAPPING_TYPE_DATETIME,
    datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
)
data_datetimearray = (
    "org.astarteplatform.end-to-end.DeviceDatastream",
    "/datetimearray_endpoint",
    [
        datetime(1994, 3, 25, 13, 32, 0, tzinfo=timezone.utc),
        datetime(2012, 2, 5, 13, 32, 0, tzinfo=timezone.utc),
    ],
    DType.ASTARTE_MAPPING_TYPE_DATETIMEARRAY,
    datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
)
data_double = (
    "org.astarteplatform.end-to-end.DeviceDatastream",
    "/double_endpoint",
    42.5,
    DType.ASTARTE_MAPPING_TYPE_DOUBLE,
    datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
)
data_doublearray = (
    "org.astarteplatform.end-to-end.DeviceDatastream",
    "/doublearray_endpoint",
    [42.5, 32.12, 0.0, 2.0],
    DType.ASTARTE_MAPPING_TYPE_DOUBLEARRAY,
    datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
)
data_integer = (
    "org.astarteplatform.end-to-end.DeviceDatastream",
    "/integer_endpoint",
    10,
    DType.ASTARTE_MAPPING_TYPE_INTEGER,
    datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
)
data_integerarray = (
    "org.astarteplatform.end-to-end.DeviceDatastream",
    "/integerarray_endpoint",
    [10, 133, 2, 0],
    DType.ASTARTE_MAPPING_TYPE_INTEGERARRAY,
    datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
)
data_longinteger = (
    "org.astarteplatform.end-to-end.DeviceDatastream",
    "/longinteger_endpoint",
    2**34,
    DType.ASTARTE_MAPPING_TYPE_LONGINTEGER,
    datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
)
data_longintegerarray = (
    "org.astarteplatform.end-to-end.DeviceDatastream",
    "/longintegerarray_endpoint",
    [2**34, 0, 2**12],
    DType.ASTARTE_MAPPING_TYPE_LONGINTEGERARRAY,
    datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
)
data_string = (
    "org.astarteplatform.end-to-end.DeviceDatastream",
    "/string_endpoint",
    "hello world",
    DType.ASTARTE_MAPPING_TYPE_STRING,
    datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
)
data_stringarray = (
    "org.astarteplatform.end-to-end.DeviceDatastream",
    "/stringarray_endpoint",
    ["hello", " ", "world", "!"],
    DType.ASTARTE_MAPPING_TYPE_STRINGARRAY,
    datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
)

test_case_individual_datastream_transmission = TestCase(
    "data-transmission",
    [
        TestActionConnect(timeout=30),
        TestActionSleep(seconds=2),
        TestActionTransmitMQTTData(*data_binaryblob),
        TestActionTransmitMQTTData(*data_binaryblobarray),
        TestActionTransmitMQTTData(*data_boolean),
        TestActionTransmitMQTTData(*data_booleanarray),
        TestActionTransmitMQTTData(*data_datetime),
        TestActionTransmitMQTTData(*data_datetimearray),
        TestActionTransmitMQTTData(*data_double),
        TestActionTransmitMQTTData(*data_doublearray),
        TestActionTransmitMQTTData(*data_integer),
        TestActionTransmitMQTTData(*data_integerarray),
        TestActionTransmitMQTTData(*data_longinteger),
        TestActionTransmitMQTTData(*data_longintegerarray),
        TestActionTransmitMQTTData(*data_string),
        TestActionTransmitMQTTData(*data_stringarray),
        TestActionSleep(seconds=1),
        TestActionFetchRESTData(*data_binaryblob),
        TestActionFetchRESTData(*data_binaryblobarray),
        TestActionFetchRESTData(*data_boolean),
        TestActionFetchRESTData(*data_booleanarray),
        TestActionFetchRESTData(*data_datetime),
        TestActionFetchRESTData(*data_datetimearray),
        TestActionFetchRESTData(*data_double),
        TestActionFetchRESTData(*data_doublearray),
        TestActionFetchRESTData(*data_integer),
        TestActionFetchRESTData(*data_integerarray),
        TestActionFetchRESTData(*data_longinteger),
        TestActionFetchRESTData(*data_longintegerarray),
        TestActionFetchRESTData(*data_string),
        TestActionFetchRESTData(*data_stringarray),
        TestActionSleep(seconds=2),
        TestActionDisconnect(timeout=30),
        TestActionSleep(seconds=2),
    ],
)
