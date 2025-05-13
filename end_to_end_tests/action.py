# (C) Copyright 2025, SECO Mind Srl
#
# SPDX-License-Identifier: Apache-2.0

import base64
import json
import logging
import time
from abc import ABC, abstractmethod
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, List

import requests
from exceptions import HttpException, MismatchException, ShellException
from qemu_commands import (
    DType,
    qemu_cmd_clear_receive_queue,
    qemu_cmd_connect,
    qemu_cmd_disconnect,
    qemu_cmd_get_data,
    qemu_cmd_send_data,
)
from tqdm import tqdm


class TestAction(ABC):
    def configure_qemu(self, pty_master_fd: int, log_file: Path):
        self._pty_master_fd: int = pty_master_fd
        self._log_file: Path = log_file

    def configure_curl(
        self,
        astarte_local: bool,
        api_hostname: str,
        appengine_token: str,
        realm: str,
        device_id: str,
    ):
        self._astarte_local: bool = astarte_local
        self._api_hostname: str = api_hostname
        self._appengine_token: str = appengine_token
        self._realm: str = realm
        self._device_id: str = device_id

    @abstractmethod
    def execute(self, case_name: str):
        pass


class TestActionSleep(TestAction):
    def __init__(self, milliseconds: int = 0, seconds: int = 0):
        self._delay_ms = milliseconds + (seconds * 1000)

    def execute(self, case_name: str):
        logging.info(f"[{case_name}] Sleeping {self._delay_ms}ms"),
        for _ in tqdm(range(self._delay_ms)):
            time.sleep(0.001)


class TestActionConnect(TestAction):
    def __init__(self, timeout: int = 60):
        self._timeout = timeout

    def execute(self, case_name: str):
        logging.info(f"[{case_name}] Connecting...")
        qemu_cmd_connect(self._pty_master_fd, self._log_file, timeout=self._timeout)


class TestActionDisconnect(TestAction):
    def __init__(self, timeout: int = 60):
        self._timeout = timeout

    def execute(self, case_name: str):
        logging.info(f"[{case_name}] Disconnecting...")
        qemu_cmd_disconnect(self._pty_master_fd, self._log_file, timeout=self._timeout)


class TestActionCheckDeviceStatus(TestAction):
    def __init__(self, connected: bool, introspection: List[str], http_timeout=1):
        self._connected = connected
        self._introspection = introspection
        self._http_timeout = http_timeout

    def execute(self, case_name: str):
        logging.info(f"[{case_name}] Checking device status...")
        request_url = (
            "https://"
            + ("127.0.0.1" if self._astarte_local else self._api_hostname)
            + "/appengine/v1/"
            + self._realm
            + "/devices/"
            + self._device_id
        )
        headers = {
            "Authorization": "Bearer " + self._appengine_token,
            "Content-Type": "application/json",
            "Host": self._api_hostname,
        }
        logging.debug(f"HTTPs GET: {request_url}")
        res = requests.get(
            url=request_url, headers=headers, verify=False, timeout=self._http_timeout
        )
        if res.status_code != 200:
            logging.error(res.text)
            raise requests.HTTPError("Fetching device status through REST API failed.")

        response_json = res.json()
        connected = response_json.get("data", {}).get("connected")
        if connected != self._connected:
            logging.error("Expected: " + ("connected" if self._connected else "disconnected"))
            logging.error("Actual: " + ("connected" if connected else "disconnected"))
            raise MismatchException("Mismatch in connection status.")

        introspection = response_json.get("data", {}).get("introspection")
        for interface in self._introspection:
            logging.debug(f"Searching for interface {interface} in introspection.")
            if interface not in introspection:
                logging.error(f"Device introspection is missing interface: {interface}")
                raise MismatchException("Device introspection is missing one interface.")


class TestActionTransmitMQTTData(TestAction):
    def __init__(
        self,
        interface: str,
        path: str,
        data: Any,
        dtype: DType,
        datetime: datetime,
        timeout: int = 60,
    ):
        self._interface: str = interface
        self._path: str = path
        self._data: str = data
        self._dtype: DType = dtype
        self._timestamp_s: float = datetime.timestamp()
        self._timeout = timeout

    def execute(self, case_name: str):
        logging.info(f"[{case_name}] Transmitting MQTT data...")
        qemu_cmd_send_data(
            self._pty_master_fd,
            self._log_file,
            self._interface,
            self._path,
            self._data,
            self._dtype,
            timestamp=self._timestamp_s,
            timeout=self._timeout,
        )


class TestActionFetchRESTData(TestAction):
    def __init__(
        self,
        interface: str,
        path: str,
        exp_data: Any,
        exp_dtype: DType,
        exp_datetime: datetime | None,
        http_timeout=1,
    ):
        self._interface: str = interface
        self._path: str = path
        self._exp_data: str = exp_data
        self._exp_dtype: DType = exp_dtype
        self._exp_datetime: datetime | None = exp_datetime
        self._http_timeout = http_timeout

    def execute(self, case_name: str):
        logging.info(f"[{case_name}] Fetching REST data...")
        request_url = (
            "https://"
            + ("127.0.0.1" if self._astarte_local else self._api_hostname)
            + "/appengine/v1/"
            + self._realm
            + "/devices/"
            + self._device_id
            + "/interfaces/"
            + self._interface
        )
        headers = {
            "Authorization": "Bearer " + self._appengine_token,
            "Content-Type": "application/json",
            "Host": self._api_hostname,
        }
        logging.debug(f"HTTPs GET: {request_url}")
        res = requests.get(
            url=request_url, headers=headers, verify=False, timeout=self._http_timeout
        )
        if res.status_code != 200:
            logging.error(res.text)
            raise requests.HTTPError("Fetching device data through REST API failed.")

        response_json = res.json()["data"]
        try:
            fetched_data = response_json[self._path]["value"]
            fetched_datetime_str = response_json[self._path].get("timestamp", None)
            fetched_datetime = datetime.strptime(
                fetched_datetime_str, "%Y-%m-%dT%H:%M:%S.%fZ"
            ).replace(tzinfo=timezone.utc)
        except:
            logging.error(f"Missing entry '{self._path}' in REST data.")
            logging.info(f"Fetched data: {json.dumps(response_json, indent=4)}")
            raise HttpException("Fetching of data through REST API failed.")

        if self._exp_dtype is DType.ASTARTE_MAPPING_TYPE_BINARYBLOB:
            fetched_data = base64.b64decode(fetched_data)
        if self._exp_dtype is DType.ASTARTE_MAPPING_TYPE_BINARYBLOBARRAY:
            fetched_data = [base64.b64decode(e) for e in fetched_data]
        if self._exp_dtype is DType.ASTARTE_MAPPING_TYPE_DATETIME:
            fetched_data = datetime.strptime(fetched_data, "%Y-%m-%dT%H:%M:%S.%fZ").replace(
                tzinfo=timezone.utc
            )
        if self._exp_dtype is DType.ASTARTE_MAPPING_TYPE_DATETIMEARRAY:
            fetched_data = [
                datetime.strptime(e, "%Y-%m-%dT%H:%M:%S.%fZ").replace(tzinfo=timezone.utc)
                for e in fetched_data
            ]

        if self._exp_data != fetched_data:
            logging.error(f"Fetched data: {fetched_data}")
            logging.error(f"Expected data: {self._exp_data}")
            raise MismatchException("Fetched REST API data differs from expected data.")

        if self._exp_datetime != fetched_datetime:
            logging.error(f"Fetched timestamp: {fetched_datetime}")
            logging.error(f"Expected timestamp: {self._exp_datetime}")
            raise MismatchException("Fetched REST API timestamp differs from expected timestamp.")


class TestActionTransmitRESTData(TestAction):
    def __init__(
        self,
        interface: str,
        path: str,
        data: Any,
        dtype: DType,
        datetime: datetime,
        http_timeout=1,
    ):
        self._interface: str = interface
        self._path: str = path
        self._data: Any = data
        self._dtype: DType = dtype
        self._timestamp_s: float = datetime.timestamp()
        self._http_timeout = http_timeout

    def execute(self, case_name: str):
        logging.info(f"[{case_name}] Transmitting REST data...")
        request_url = (
            "https://"
            + ("127.0.0.1" if self._astarte_local else self._api_hostname)
            + "/appengine/v1/"
            + self._realm
            + "/devices/"
            + self._device_id
            + "/interfaces/"
            + self._interface
            + self._path
        )
        headers = {
            "Authorization": "Bearer " + self._appengine_token,
            "Content-Type": "application/json",
            "Host": self._api_hostname,
        }

        data = self._data
        if self._dtype is DType.ASTARTE_MAPPING_TYPE_BINARYBLOB:
            data = base64.b64encode(data).decode("utf-8")
        if self._dtype is DType.ASTARTE_MAPPING_TYPE_BINARYBLOBARRAY:
            data = [base64.b64encode(d).decode("utf-8") for d in data]

        json_data = json.dumps({"data": data}, default=str)
        logging.debug(f"HTTPs POST: {request_url} {json_data}")
        res = requests.post(
            url=request_url,
            data=json_data,
            headers=headers,
            verify=False,
            timeout=self._http_timeout,
        )
        if res.status_code != 200:
            logging.error(res.text)
            raise requests.HTTPError("Transmitting device data through REST API failed.")


class TestActionReadReceivedMQTTData(TestAction):
    def __init__(
        self,
        interface: str,
        path: str,
        data: Any,
        timeout: int = 60,
    ):
        self._interface: str = interface
        self._path: str = path
        self._data: Any = data
        self._timeout = timeout

    def execute(self, case_name: str):
        logging.info(f"[{case_name}] Reading received MQTT data...")
        all_received_data = qemu_cmd_get_data(
            self._pty_master_fd,
            self._log_file,
            timeout=self._timeout,
        )

        received_data = [
            d
            for d in all_received_data
            if (
                (d["interface-name"] == self._interface)
                and (d["path"] == self._path)
                and (d["data"] == self._data)
            )
        ]
        if len(received_data) != 1:
            logging.error(f"Received data: {all_received_data}")
            logging.error(f"Expected interface: {self._interface}")
            logging.error(f"Expected path: {self._path}")
            logging.error(f"Expected data: {self._data}")
            raise MismatchException("Could not find expected received data.")


class TestActionClearReceivedMQTTData(TestAction):
    def __init__(
        self,
        timeout: int = 60,
    ):
        self._timeout = timeout

    def execute(self, case_name: str):
        logging.info(f"[{case_name}] Clearing received MQTT data...")
        qemu_cmd_clear_receive_queue(
            self._pty_master_fd,
            self._log_file,
            timeout=self._timeout,
        )
