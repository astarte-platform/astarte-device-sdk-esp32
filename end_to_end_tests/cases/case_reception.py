# (C) Copyright 2025, SECO Mind Srl
#
# SPDX-License-Identifier: Apache-2.0

from datetime import datetime, timezone

from action import (
    TestActionClearReceivedMQTTData,
    TestActionConnect,
    TestActionDisconnect,
    TestActionReadReceivedMQTTData,
    TestActionSleep,
    TestActionTransmitRESTData,
)
from case import TestCase
from qemu_commands import DType

data_binaryblob = (
    "org.astarteplatform.end-to-end.ServerDatastream",
    "/binaryblob_endpoint",
    b"binblob",
    DType.ASTARTE_MAPPING_TYPE_BINARYBLOB,
    datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
)
data_binaryblobarray = (
    "org.astarteplatform.end-to-end.ServerDatastream",
    "/binaryblobarray_endpoint",
    [b"bin", b"blob"],
    DType.ASTARTE_MAPPING_TYPE_BINARYBLOBARRAY,
    datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
)
data_boolean = (
    "org.astarteplatform.end-to-end.ServerDatastream",
    "/boolean_endpoint",
    True,
    DType.ASTARTE_MAPPING_TYPE_BOOLEAN,
    datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
)
data_booleanarray = (
    "org.astarteplatform.end-to-end.ServerDatastream",
    "/booleanarray_endpoint",
    [True, False, True],
    DType.ASTARTE_MAPPING_TYPE_BOOLEANARRAY,
    datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
)
data_datetime = (
    "org.astarteplatform.end-to-end.ServerDatastream",
    "/datetime_endpoint",
    datetime(1994, 3, 25, 13, 32, 0, tzinfo=timezone.utc),
    DType.ASTARTE_MAPPING_TYPE_DATETIME,
    datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
)
data_datetimearray = (
    "org.astarteplatform.end-to-end.ServerDatastream",
    "/datetimearray_endpoint",
    [
        datetime(1994, 3, 25, 13, 32, 0, tzinfo=timezone.utc),
        datetime(2012, 2, 5, 13, 32, 0, tzinfo=timezone.utc),
    ],
    DType.ASTARTE_MAPPING_TYPE_DATETIMEARRAY,
    datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
)
data_double = (
    "org.astarteplatform.end-to-end.ServerDatastream",
    "/double_endpoint",
    42.5,
    DType.ASTARTE_MAPPING_TYPE_DOUBLE,
    datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
)
data_doublearray = (
    "org.astarteplatform.end-to-end.ServerDatastream",
    "/doublearray_endpoint",
    [42.5, 32.12, 0.0, 2.0],
    DType.ASTARTE_MAPPING_TYPE_DOUBLEARRAY,
    datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
)
data_integer = (
    "org.astarteplatform.end-to-end.ServerDatastream",
    "/integer_endpoint",
    10,
    DType.ASTARTE_MAPPING_TYPE_INTEGER,
    datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
)
data_integerarray = (
    "org.astarteplatform.end-to-end.ServerDatastream",
    "/integerarray_endpoint",
    [10, 133, 2, 0],
    DType.ASTARTE_MAPPING_TYPE_INTEGERARRAY,
    datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
)
data_longinteger = (
    "org.astarteplatform.end-to-end.ServerDatastream",
    "/longinteger_endpoint",
    2**34,
    DType.ASTARTE_MAPPING_TYPE_LONGINTEGER,
    datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
)
data_longintegerarray = (
    "org.astarteplatform.end-to-end.ServerDatastream",
    "/longintegerarray_endpoint",
    [2**34, 0, 2**12],
    DType.ASTARTE_MAPPING_TYPE_LONGINTEGERARRAY,
    datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
)
data_string = (
    "org.astarteplatform.end-to-end.ServerDatastream",
    "/string_endpoint",
    "hello world",
    DType.ASTARTE_MAPPING_TYPE_STRING,
    datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
)
data_stringarray = (
    "org.astarteplatform.end-to-end.ServerDatastream",
    "/stringarray_endpoint",
    ["hello", " ", "world", "!"],
    DType.ASTARTE_MAPPING_TYPE_STRINGARRAY,
    datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
)

test_case_individual_datastream_reception = TestCase(
    "data-reception",
    [
        TestActionConnect(timeout=30),
        TestActionSleep(seconds=2),
        TestActionTransmitRESTData(*data_binaryblob),
        TestActionTransmitRESTData(*data_binaryblobarray),
        TestActionTransmitRESTData(*data_boolean),
        TestActionTransmitRESTData(*data_booleanarray),
        TestActionTransmitRESTData(*data_datetime),
        TestActionTransmitRESTData(*data_datetimearray),
        TestActionTransmitRESTData(*data_double),
        TestActionTransmitRESTData(*data_doublearray),
        TestActionTransmitRESTData(*data_integer),
        TestActionTransmitRESTData(*data_integerarray),
        TestActionTransmitRESTData(*data_longinteger),
        TestActionTransmitRESTData(*data_longintegerarray),
        TestActionTransmitRESTData(*data_string),
        TestActionTransmitRESTData(*data_stringarray),
        TestActionSleep(seconds=2),
        TestActionReadReceivedMQTTData(*(data_binaryblob[:3])),
        TestActionReadReceivedMQTTData(*(data_binaryblobarray[:3])),
        TestActionReadReceivedMQTTData(*(data_boolean[:3])),
        TestActionReadReceivedMQTTData(*(data_booleanarray[:3])),
        TestActionReadReceivedMQTTData(*(data_datetime[:3])),
        TestActionReadReceivedMQTTData(*(data_datetimearray[:3])),
        TestActionReadReceivedMQTTData(*(data_double[:3])),
        TestActionReadReceivedMQTTData(*(data_doublearray[:3])),
        TestActionReadReceivedMQTTData(*(data_integer[:3])),
        TestActionReadReceivedMQTTData(*(data_integerarray[:3])),
        TestActionReadReceivedMQTTData(*(data_longinteger[:3])),
        TestActionReadReceivedMQTTData(*(data_longintegerarray[:3])),
        TestActionReadReceivedMQTTData(*(data_string[:3])),
        TestActionReadReceivedMQTTData(*(data_stringarray[:3])),
        TestActionSleep(seconds=2),
        TestActionClearReceivedMQTTData(timeout=30),
        TestActionDisconnect(timeout=30),
        TestActionSleep(seconds=2),
    ],
)
