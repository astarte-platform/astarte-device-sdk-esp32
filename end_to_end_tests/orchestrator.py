# (C) Copyright 2025, SECO Mind Srl
#
# SPDX-License-Identifier: Apache-2.0

from pathlib import Path
from typing import List

from case import TestCase


class ConfigCurl:
    def __init__(
        self,
        astarte_local: bool,
        api_hostname: str,
        appengine_token: str,
        realm: str,
        device_id: str,
    ):
        self.astarte_local: bool = astarte_local
        self.api_hostname: str = api_hostname
        self.appengine_token: str = appengine_token
        self.realm: str = realm
        self.device_id: str = device_id


class TestOrchestrator:
    def __init__(self, curl_config: ConfigCurl, is_host: bool):
        self._curl_config: ConfigCurl = curl_config
        self._test_cases: List[TestCase] = []
        self._is_host = is_host

    def add_test_case(self, test_case: TestCase, log_dir: Path):
        test_case.configure_log_dir(log_dir)
        test_case.configure_qemu(self._is_host)
        test_case.configure_curl(
            self._curl_config.astarte_local,
            self._curl_config.api_hostname,
            self._curl_config.appengine_token,
            self._curl_config.realm,
            self._curl_config.device_id,
        )
        self._test_cases.append(test_case)

    def execute_all(self):
        for test_case in self._test_cases:
            test_case.execute()
