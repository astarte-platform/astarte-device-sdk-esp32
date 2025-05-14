# (C) Copyright 2025, SECO Mind Srl
#
# SPDX-License-Identifier: Apache-2.0

import argparse
import logging
import os
import tomllib
from pathlib import Path

from cases.case_connectivity import test_case_connectivity
from cases.case_reception import test_case_individual_datastream_reception
from cases.case_transmission import test_case_individual_datastream_transmission
from orchestrator import ConfigCurl, TestOrchestrator

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
    orchestrator.add_test_case(test_case_connectivity, tmp_dir)
    orchestrator.add_test_case(test_case_individual_datastream_transmission, tmp_dir)
    orchestrator.add_test_case(test_case_individual_datastream_reception, tmp_dir)
    orchestrator.execute_all()
