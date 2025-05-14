# (C) Copyright 2025, SECO Mind Srl
#
# SPDX-License-Identifier: Apache-2.0

import pty
from pathlib import Path
from typing import List

from action import TestAction
from qemu_commands import qemu_cmd_start, qemu_cmd_stop


class TestCase:
    def __init__(self, name: str, actions: List[TestAction]):
        self._name: str = name
        self._actions: List[TestAction] = actions
        self._pty_master_fd, self._pty_slave_fd = pty.openpty()

    def configure_log_dir(self, log_dir: Path):
        self._log_file: Path = log_dir / f"{self._name}_qemu_build_stdout.log"
        self._err_file: Path = log_dir / f"{self._name}_qemu_build_stderr.log"

    def configure_qemu(self, is_host: bool):
        self._is_host: bool = is_host
        for action in self._actions:
            action.configure_qemu(self._pty_master_fd, self._log_file)

    def configure_curl(
        self,
        astarte_local: bool,
        api_hostname: str,
        appengine_token: str,
        realm: str,
        device_id: str,
    ):
        for action in self._actions:
            action.configure_curl(astarte_local, api_hostname, appengine_token, realm, device_id)

    def execute(self):
        process = qemu_cmd_start(
            self._pty_master_fd,
            self._pty_slave_fd,
            self._is_host,
            self._log_file,
            self._err_file,
            timeout=120,
        )
        try:
            for action in self._actions:
                action.execute(self._name)
        finally:
            qemu_cmd_stop(process, self._pty_master_fd, timeout=10)
