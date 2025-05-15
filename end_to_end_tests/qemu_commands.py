# (C) Copyright 2025, SECO Mind Srl
#
# SPDX-License-Identifier: Apache-2.0

import os
import signal
import subprocess
import time
from enum import IntEnum
from typing import List

import bson
from exceptions import TimeoutException


class DType(IntEnum):
    ASTARTE_MAPPING_TYPE_BINARYBLOB = 1
    ASTARTE_MAPPING_TYPE_BOOLEAN = 2
    ASTARTE_MAPPING_TYPE_DATETIME = 3
    ASTARTE_MAPPING_TYPE_DOUBLE = 4
    ASTARTE_MAPPING_TYPE_INTEGER = 5
    ASTARTE_MAPPING_TYPE_LONGINTEGER = 6
    ASTARTE_MAPPING_TYPE_STRING = 7
    ASTARTE_MAPPING_TYPE_BINARYBLOBARRAY = 8
    ASTARTE_MAPPING_TYPE_BOOLEANARRAY = 9
    ASTARTE_MAPPING_TYPE_DATETIMEARRAY = 10
    ASTARTE_MAPPING_TYPE_DOUBLEARRAY = 11
    ASTARTE_MAPPING_TYPE_INTEGERARRAY = 12
    ASTARTE_MAPPING_TYPE_LONGINTEGERARRAY = 13
    ASTARTE_MAPPING_TYPE_STRINGARRAY = 14


def send_qemu_command(pty_master_fd: int, cmd: List[str], write_interval=1):
    cmd_encoded = (" ".join(cmd) + "\n").encode()
    offset = 0
    while offset < len(cmd_encoded):
        offset += os.write(pty_master_fd, cmd_encoded[offset : offset + 128])
        time.sleep(write_interval)


def init_log_position(log_file):
    try:
        with open(log_file, "r") as f:
            f.seek(0, 2)
            return f.tell()
    except FileNotFoundError:
        return 0


def wait_for_qemu_log(search_string, log_file, initial_position, timeout, check_interval=1):
    start_time = time.time()
    current_position = initial_position
    with open(log_file, "r") as logfile:
        logfile.seek(current_position)
        while time.time() - start_time < timeout:
            new_lines = logfile.readlines()
            if new_lines:
                for line in new_lines:
                    if search_string in line:
                        return
            time.sleep(check_interval)
    raise TimeoutException("QEMU wait for log timed out!")


def qemu_cmd_start(pty_master_fd, pty_slave_fd, is_host, log_file, err_file, timeout):

    log_file_position = init_log_position(log_file)

    with open(log_file, "w+") as logfile:
        with open(err_file, "w+") as errfile:
            process = subprocess.Popen(
                ["idf.py"] + (["qemu"] if is_host else ["flash", "monitor"]),
                stdin=pty_slave_fd,
                stdout=logfile,
                stderr=errfile,
                close_fds=True,
                start_new_session=True,
            )
            print(f"QEMU subprocess started with PID: {process.pid}.")

    os.close(pty_slave_fd)  # Not used in the current process

    try:
        wait_for_qemu_log("Device created.", log_file, log_file_position, timeout)
    except TimeoutException as e:
        print("No reply from the start message.")
        qemu_cmd_stop(process, pty_master_fd, timeout=10)
        raise e

    return process


def qemu_cmd_stop(process, pty_master_fd, timeout):
    print("Closing pty master fd")
    os.close(pty_master_fd)
    print(f"Stopping QEMU process group with PGID: {os.getpgid(process.pid)}")
    try:
        os.killpg(os.getpgid(process.pid), signal.SIGTERM)
        process.wait(timeout=timeout)
    except subprocess.TimeoutExpired:
        print("Process group did not exit, killing directly.")
        os.killpg(os.getpgid(process.pid), signal.SIGKILL)
    print("QEMU stopped.")


def qemu_cmd_connect(pty_master_fd, log_file, timeout):
    log_file_position = init_log_position(log_file)
    os.write(pty_master_fd, b"connect\n")
    try:
        wait_for_qemu_log("Astarte device connected.", log_file, log_file_position, timeout)
    except TimeoutException as e:
        print("No reply from connect message.")
        raise e


def qemu_cmd_disconnect(pty_master_fd, log_file, timeout):
    log_file_position = init_log_position(log_file)
    os.write(pty_master_fd, b"disconnect\n")
    try:
        wait_for_qemu_log("Astarte device disconnected.", log_file, log_file_position, timeout)
    except TimeoutException as e:
        print("No reply from disconnect message.")
        raise e


def qemu_cmd_send_data(
    pty_master_fd, log_file, interface, path, data, dtype, timeout, object=False, timestamp=None
):
    log_file_position = init_log_position(log_file)

    bson_payload = b""
    payload = {"v": data}
    bson_payload = bson.dumps(payload)
    bson_payload_str = "-".join(f"{byte:02X}" for byte in bson_payload)

    cmd = [
        o
        for o in [
            "send-data",
            f"--dtype {int(dtype)}",
            "-o" if object else None,
            f"-t {int(timestamp)}" if timestamp else None,
            f'"{interface}"',
            f'"{path}"',
            f'"{bson_payload_str}"',
        ]
        if o
    ]
    send_qemu_command(pty_master_fd, cmd)

    try:
        wait_for_qemu_log(
            f"Transmission to Astarte completed '{interface}' - '{path}' - '{bson_payload_str}'",
            log_file,
            log_file_position,
            timeout,
        )
    except TimeoutException as e:
        print("No reply from send data message.")
        raise e


def parse_hex_block(block):
    return bytes.fromhex(block.replace("-", ""))


def qemu_cmd_get_data(pty_master_fd, log_file, timeout):
    log_file_position = init_log_position(log_file)

    cmd = ["get-data"]
    send_qemu_command(pty_master_fd, cmd)

    # Wait for the writing to complete
    try:
        wait_for_qemu_log(
            "List of received data emptied.",
            log_file,
            log_file_position,
            timeout,
        )
    except TimeoutException as e:
        print("No reply from get data message.")
        raise e

    # Parse the data
    received_data = []
    with open(log_file, "r") as logfile:
        logfile.seek(log_file_position)
        log_lines = logfile.readlines()
        for idx, line in enumerate(log_lines):
            if "-- BEGIN --" in line:
                received_data.append(log_lines[idx + 1].strip())
    return [bson.loads(bytes.fromhex(d.replace("-", ""))) for d in received_data]


def qemu_cmd_clear_receive_queue(pty_master_fd, log_file, timeout):
    log_file_position = init_log_position(log_file)
    send_qemu_command(pty_master_fd, ["clear-receive-queue"])
    try:
        wait_for_qemu_log("Receive queue cleared.", log_file, log_file_position, timeout)
    except TimeoutException as e:
        print("No reply from connect message.")
        raise e
