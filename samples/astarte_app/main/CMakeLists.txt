# (C) Copyright 2025, SECO Mind Srl
#
# SPDX-License-Identifier: Apache-2.0

idf_component_register(
    SRCS
        "app_main.c"
        "./src/wifi_cfg.c"
        "./src/astarte_task.c"
    INCLUDE_DIRS "./include"
    REQUIRES astarte-device-sdk-esp32 nvs_flash esp_event esp_wifi)
