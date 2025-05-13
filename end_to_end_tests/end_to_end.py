# (C) Copyright 2025, SECO Mind Srl
#
# SPDX-License-Identifier: Apache-2.0

import argparse
import logging
import os
import tomllib
from datetime import datetime, timezone
from pathlib import Path

from action import (
    TestActionCheckDeviceStatus,
    TestActionClearReceivedMQTTData,
    TestActionConnect,
    TestActionDisconnect,
    TestActionFetchRESTData,
    TestActionReadReceivedMQTTData,
    TestActionSleep,
    TestActionTransmitMQTTData,
    TestActionTransmitRESTData,
)
from case import TestCase
from orchestrator import ConfigCurl, TestOrchestrator
from qemu_commands import DType

PLATFORM_TARGET = "target"
PLATFORM_HOST = "host"


def init_tmp_directory():
    tmp_dir = Path(".tmp")
    if not os.path.exists(tmp_dir):
        os.makedirs(tmp_dir)
        print(f"Directory '{tmp_dir}' created.")
    else:
        for filename in os.listdir(tmp_dir):
            file_path = os.path.join(tmp_dir, filename)
            if os.path.isfile(file_path):
                os.remove(file_path)
        print(f"Contents of '{tmp_dir}' have been removed.")
    return tmp_dir


def load_configuration():
    with open("./../config.toml", "rb") as f:
        config = tomllib.load(f)

    realm = config.get("REALM", "")
    device_id = config.get("DEVICE_ID", "")
    api_hostname = config.get("API_HOSTNAME", "")
    appengine_token = config.get("APPENGINE_TOKEN", "")

    with open("./../config.priv", "rb") as f:
        priv_config = tomllib.load(f)

    realm = priv_config.get("REALM", realm)
    device_id = priv_config.get("DEVICE_ID", device_id)
    api_hostname = priv_config.get("API_HOSTNAME", api_hostname)
    appengine_token = priv_config.get("APPENGINE_TOKEN", appengine_token)

    return realm, device_id, api_hostname, appengine_token


if __name__ == "__main__":
    logging.basicConfig(level=logging.DEBUG)
    parser = argparse.ArgumentParser(description="End to end test runner.")

    parser.add_argument(
        "--platform",
        choices=[PLATFORM_TARGET, PLATFORM_HOST],
        type=str,
        required=False,
        default="host",
        help=f"Specify the platform type: {PLATFORM_TARGET} or {PLATFORM_HOST}",
    )
    parser.add_argument(
        "--astarte-local",
        action="store_true",
        help="When set, assumes that Astarte is running locally on the host.",
    )

    args = parser.parse_args()

    tmp_dir = init_tmp_directory()
    realm, device_id, api_hostname, appengine_token = load_configuration()

    connectivity_test = TestCase(
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
        tmp_dir,
    )
    data_tx_test = TestCase(
        "data-transmission",
        [
            TestActionConnect(timeout=30),
            TestActionSleep(seconds=2),
            TestActionTransmitMQTTData(
                "org.astarteplatform.end-to-end.DeviceDatastream",
                "/binaryblob_endpoint",
                b"binblob",
                DType.ASTARTE_MAPPING_TYPE_BINARYBLOB,
                datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
            ),
            TestActionTransmitMQTTData(
                "org.astarteplatform.end-to-end.DeviceDatastream",
                "/binaryblobarray_endpoint",
                [b"bin", b"blob"],
                DType.ASTARTE_MAPPING_TYPE_BINARYBLOBARRAY,
                datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
            ),
            TestActionTransmitMQTTData(
                "org.astarteplatform.end-to-end.DeviceDatastream",
                "/boolean_endpoint",
                True,
                DType.ASTARTE_MAPPING_TYPE_BOOLEAN,
                datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
            ),
            TestActionTransmitMQTTData(
                "org.astarteplatform.end-to-end.DeviceDatastream",
                "/booleanarray_endpoint",
                [True, False, True],
                DType.ASTARTE_MAPPING_TYPE_BOOLEANARRAY,
                datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
            ),
            TestActionTransmitMQTTData(
                "org.astarteplatform.end-to-end.DeviceDatastream",
                "/datetime_endpoint",
                datetime(1994, 3, 25, 13, 32, 0, tzinfo=timezone.utc),
                DType.ASTARTE_MAPPING_TYPE_DATETIME,
                datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
            ),
            TestActionTransmitMQTTData(
                "org.astarteplatform.end-to-end.DeviceDatastream",
                "/datetimearray_endpoint",
                [
                    datetime(1994, 3, 25, 13, 32, 0, tzinfo=timezone.utc),
                    datetime(2012, 2, 5, 13, 32, 0, tzinfo=timezone.utc),
                ],
                DType.ASTARTE_MAPPING_TYPE_DATETIMEARRAY,
                datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
            ),
            TestActionTransmitMQTTData(
                "org.astarteplatform.end-to-end.DeviceDatastream",
                "/double_endpoint",
                42.5,
                DType.ASTARTE_MAPPING_TYPE_DOUBLE,
                datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
            ),
            TestActionTransmitMQTTData(
                "org.astarteplatform.end-to-end.DeviceDatastream",
                "/doublearray_endpoint",
                [42.5, 32.12, 0.0, 2.0],
                DType.ASTARTE_MAPPING_TYPE_DOUBLEARRAY,
                datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
            ),
            TestActionTransmitMQTTData(
                "org.astarteplatform.end-to-end.DeviceDatastream",
                "/integer_endpoint",
                10,
                DType.ASTARTE_MAPPING_TYPE_INTEGER,
                datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
            ),
            TestActionTransmitMQTTData(
                "org.astarteplatform.end-to-end.DeviceDatastream",
                "/integerarray_endpoint",
                [10, 133, 2, 0],
                DType.ASTARTE_MAPPING_TYPE_INTEGERARRAY,
                datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
            ),
            TestActionTransmitMQTTData(
                "org.astarteplatform.end-to-end.DeviceDatastream",
                "/longinteger_endpoint",
                2**34,
                DType.ASTARTE_MAPPING_TYPE_LONGINTEGER,
                datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
            ),
            TestActionTransmitMQTTData(
                "org.astarteplatform.end-to-end.DeviceDatastream",
                "/longintegerarray_endpoint",
                [2**34, 0, 2**12],
                DType.ASTARTE_MAPPING_TYPE_LONGINTEGERARRAY,
                datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
            ),
            TestActionTransmitMQTTData(
                "org.astarteplatform.end-to-end.DeviceDatastream",
                "/string_endpoint",
                "hello world",
                DType.ASTARTE_MAPPING_TYPE_STRING,
                datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
            ),
            TestActionTransmitMQTTData(
                "org.astarteplatform.end-to-end.DeviceDatastream",
                "/stringarray_endpoint",
                ["hello", " ", "world", "!"],
                DType.ASTARTE_MAPPING_TYPE_STRINGARRAY,
                datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
            ),
            TestActionSleep(seconds=1),
            TestActionFetchRESTData(
                "org.astarteplatform.end-to-end.DeviceDatastream",
                "binaryblob_endpoint",
                b"binblob",
                DType.ASTARTE_MAPPING_TYPE_BINARYBLOB,
                datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
            ),
            TestActionFetchRESTData(
                "org.astarteplatform.end-to-end.DeviceDatastream",
                "binaryblobarray_endpoint",
                [b"bin", b"blob"],
                DType.ASTARTE_MAPPING_TYPE_BINARYBLOBARRAY,
                datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
            ),
            TestActionFetchRESTData(
                "org.astarteplatform.end-to-end.DeviceDatastream",
                "boolean_endpoint",
                True,
                DType.ASTARTE_MAPPING_TYPE_BOOLEAN,
                datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
            ),
            TestActionFetchRESTData(
                "org.astarteplatform.end-to-end.DeviceDatastream",
                "booleanarray_endpoint",
                [True, False, True],
                DType.ASTARTE_MAPPING_TYPE_BOOLEANARRAY,
                datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
            ),
            TestActionFetchRESTData(
                "org.astarteplatform.end-to-end.DeviceDatastream",
                "datetime_endpoint",
                datetime(1994, 3, 25, 13, 32, 0, tzinfo=timezone.utc),
                DType.ASTARTE_MAPPING_TYPE_DATETIME,
                datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
            ),
            TestActionFetchRESTData(
                "org.astarteplatform.end-to-end.DeviceDatastream",
                "datetimearray_endpoint",
                [
                    datetime(1994, 3, 25, 13, 32, 0, tzinfo=timezone.utc),
                    datetime(2012, 2, 5, 13, 32, 0, tzinfo=timezone.utc),
                ],
                DType.ASTARTE_MAPPING_TYPE_DATETIMEARRAY,
                datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
            ),
            TestActionFetchRESTData(
                "org.astarteplatform.end-to-end.DeviceDatastream",
                "double_endpoint",
                42.5,
                DType.ASTARTE_MAPPING_TYPE_DOUBLE,
                datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
            ),
            TestActionFetchRESTData(
                "org.astarteplatform.end-to-end.DeviceDatastream",
                "doublearray_endpoint",
                [42.5, 32.12, 0.0, 2.0],
                DType.ASTARTE_MAPPING_TYPE_DOUBLEARRAY,
                datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
            ),
            TestActionFetchRESTData(
                "org.astarteplatform.end-to-end.DeviceDatastream",
                "integer_endpoint",
                10,
                DType.ASTARTE_MAPPING_TYPE_INTEGER,
                datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
            ),
            TestActionFetchRESTData(
                "org.astarteplatform.end-to-end.DeviceDatastream",
                "integerarray_endpoint",
                [10, 133, 2, 0],
                DType.ASTARTE_MAPPING_TYPE_INTEGERARRAY,
                datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
            ),
            TestActionFetchRESTData(
                "org.astarteplatform.end-to-end.DeviceDatastream",
                "longinteger_endpoint",
                2**34,
                DType.ASTARTE_MAPPING_TYPE_LONGINTEGER,
                datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
            ),
            TestActionFetchRESTData(
                "org.astarteplatform.end-to-end.DeviceDatastream",
                "longintegerarray_endpoint",
                [2**34, 0, 2**12],
                DType.ASTARTE_MAPPING_TYPE_LONGINTEGERARRAY,
                datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
            ),
            TestActionFetchRESTData(
                "org.astarteplatform.end-to-end.DeviceDatastream",
                "string_endpoint",
                "hello world",
                DType.ASTARTE_MAPPING_TYPE_STRING,
                datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
            ),
            TestActionFetchRESTData(
                "org.astarteplatform.end-to-end.DeviceDatastream",
                "stringarray_endpoint",
                ["hello", " ", "world", "!"],
                DType.ASTARTE_MAPPING_TYPE_STRINGARRAY,
                datetime(2025, 4, 26, 13, 32, 0, tzinfo=timezone.utc),
            ),
            TestActionSleep(seconds=2),
            TestActionDisconnect(timeout=30),
            TestActionSleep(seconds=2),
        ],
        tmp_dir,
    )
    data_rx_test = TestCase(
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
        tmp_dir,
    )

    orchestrator = TestOrchestrator(
        ConfigCurl(
            astarte_local=args.astarte_local,
            realm=realm,
            device_id=device_id,
            api_hostname=api_hostname,
            appengine_token=appengine_token,
        ),
        is_host=(args.platform == PLATFORM_HOST),
    )
    orchestrator.add_test_case(connectivity_test)
    orchestrator.add_test_case(data_tx_test)
    orchestrator.add_test_case(data_rx_test)
    orchestrator.execute_all()
