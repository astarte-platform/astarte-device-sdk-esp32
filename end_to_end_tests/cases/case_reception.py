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

test_case_individual_datastream_reception = TestCase(
    "data-reception",
    [
        TestActionConnect(timeout=30),
        TestActionSleep(seconds=2),
        TestActionTransmitRESTData(
            "org.astarteplatform.end-to-end.ServerDatastream",
            "/binaryblob_endpoint",
            b"binblob",
            DType.ASTARTE_MAPPING_TYPE_BINARYBLOB,
            datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
        ),
        TestActionTransmitRESTData(
            "org.astarteplatform.end-to-end.ServerDatastream",
            "/binaryblobarray_endpoint",
            [b"bin", b"blob"],
            DType.ASTARTE_MAPPING_TYPE_BINARYBLOBARRAY,
            datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
        ),
        TestActionTransmitRESTData(
            "org.astarteplatform.end-to-end.ServerDatastream",
            "/boolean_endpoint",
            True,
            DType.ASTARTE_MAPPING_TYPE_BOOLEAN,
            datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
        ),
        TestActionTransmitRESTData(
            "org.astarteplatform.end-to-end.ServerDatastream",
            "/booleanarray_endpoint",
            [True, False, True],
            DType.ASTARTE_MAPPING_TYPE_BOOLEANARRAY,
            datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
        ),
        TestActionTransmitRESTData(
            "org.astarteplatform.end-to-end.ServerDatastream",
            "/datetime_endpoint",
            datetime(1994, 3, 25, 13, 32, 0, tzinfo=timezone.utc),
            DType.ASTARTE_MAPPING_TYPE_DATETIME,
            datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
        ),
        TestActionTransmitRESTData(
            "org.astarteplatform.end-to-end.ServerDatastream",
            "/datetimearray_endpoint",
            [
                datetime(1994, 3, 25, 13, 32, 0, tzinfo=timezone.utc),
                datetime(2012, 2, 5, 13, 32, 0, tzinfo=timezone.utc),
            ],
            DType.ASTARTE_MAPPING_TYPE_DATETIMEARRAY,
            datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
        ),
        TestActionTransmitRESTData(
            "org.astarteplatform.end-to-end.ServerDatastream",
            "/double_endpoint",
            42.5,
            DType.ASTARTE_MAPPING_TYPE_DOUBLE,
            datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
        ),
        TestActionTransmitRESTData(
            "org.astarteplatform.end-to-end.ServerDatastream",
            "/doublearray_endpoint",
            [42.5, 32.12, 0.0, 2.0],
            DType.ASTARTE_MAPPING_TYPE_DOUBLEARRAY,
            datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
        ),
        TestActionTransmitRESTData(
            "org.astarteplatform.end-to-end.ServerDatastream",
            "/integer_endpoint",
            10,
            DType.ASTARTE_MAPPING_TYPE_INTEGER,
            datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
        ),
        TestActionTransmitRESTData(
            "org.astarteplatform.end-to-end.ServerDatastream",
            "/integerarray_endpoint",
            [10, 133, 2, 0],
            DType.ASTARTE_MAPPING_TYPE_INTEGERARRAY,
            datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
        ),
        TestActionTransmitRESTData(
            "org.astarteplatform.end-to-end.ServerDatastream",
            "/longinteger_endpoint",
            2**34,
            DType.ASTARTE_MAPPING_TYPE_LONGINTEGER,
            datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
        ),
        TestActionTransmitRESTData(
            "org.astarteplatform.end-to-end.ServerDatastream",
            "/longintegerarray_endpoint",
            [2**34, 0, 2**12],
            DType.ASTARTE_MAPPING_TYPE_LONGINTEGERARRAY,
            datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
        ),
        TestActionTransmitRESTData(
            "org.astarteplatform.end-to-end.ServerDatastream",
            "/string_endpoint",
            "hello world",
            DType.ASTARTE_MAPPING_TYPE_STRING,
            datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
        ),
        TestActionTransmitRESTData(
            "org.astarteplatform.end-to-end.ServerDatastream",
            "/stringarray_endpoint",
            ["hello", " ", "world", "!"],
            DType.ASTARTE_MAPPING_TYPE_STRINGARRAY,
            datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
        ),
        TestActionSleep(seconds=2),
        TestActionReadReceivedMQTTData(
            "org.astarteplatform.end-to-end.ServerDatastream",
            "/binaryblob_endpoint",
            b"binblob",
        ),
        TestActionReadReceivedMQTTData(
            "org.astarteplatform.end-to-end.ServerDatastream",
            "/binaryblobarray_endpoint",
            [b"bin", b"blob"],
        ),
        TestActionReadReceivedMQTTData(
            "org.astarteplatform.end-to-end.ServerDatastream",
            "/boolean_endpoint",
            True,
        ),
        TestActionReadReceivedMQTTData(
            "org.astarteplatform.end-to-end.ServerDatastream",
            "/booleanarray_endpoint",
            [True, False, True],
        ),
        TestActionReadReceivedMQTTData(
            "org.astarteplatform.end-to-end.ServerDatastream",
            "/datetime_endpoint",
            datetime(1994, 3, 25, 13, 32, 0, tzinfo=timezone.utc),
        ),
        TestActionReadReceivedMQTTData(
            "org.astarteplatform.end-to-end.ServerDatastream",
            "/datetimearray_endpoint",
            [
                datetime(1994, 3, 25, 13, 32, 0, tzinfo=timezone.utc),
                datetime(2012, 2, 5, 13, 32, 0, tzinfo=timezone.utc),
            ],
        ),
        TestActionReadReceivedMQTTData(
            "org.astarteplatform.end-to-end.ServerDatastream",
            "/double_endpoint",
            42.5,
        ),
        TestActionReadReceivedMQTTData(
            "org.astarteplatform.end-to-end.ServerDatastream",
            "/doublearray_endpoint",
            [42.5, 32.12, 0.0, 2.0],
        ),
        TestActionReadReceivedMQTTData(
            "org.astarteplatform.end-to-end.ServerDatastream",
            "/integer_endpoint",
            10,
        ),
        TestActionReadReceivedMQTTData(
            "org.astarteplatform.end-to-end.ServerDatastream",
            "/integerarray_endpoint",
            [10, 133, 2, 0],
        ),
        TestActionReadReceivedMQTTData(
            "org.astarteplatform.end-to-end.ServerDatastream",
            "/longinteger_endpoint",
            2**34,
        ),
        TestActionReadReceivedMQTTData(
            "org.astarteplatform.end-to-end.ServerDatastream",
            "/longintegerarray_endpoint",
            [2**34, 0, 2**12],
        ),
        TestActionReadReceivedMQTTData(
            "org.astarteplatform.end-to-end.ServerDatastream",
            "/string_endpoint",
            "hello world",
        ),
        TestActionReadReceivedMQTTData(
            "org.astarteplatform.end-to-end.ServerDatastream",
            "/stringarray_endpoint",
            ["hello", " ", "world", "!"],
        ),
        TestActionSleep(seconds=2),
        TestActionClearReceivedMQTTData(timeout=30),
        TestActionDisconnect(timeout=30),
        TestActionSleep(seconds=2),
    ],
)
