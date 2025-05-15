# (C) Copyright 2025, SECO Mind Srl
#
# SPDX-License-Identifier: Apache-2.0

file(GLOB srcs "src/*.c")
idf_component_register(
    SRCS
        "app_main.c"
        "${srcs}"
        "./../interfaces/generated_interfaces.c"
    INCLUDE_DIRS "./include" "./../interfaces" "./../../../private"
    REQUIRES astarte-device-sdk-esp32 nvs_flash esp_event esp_wifi console)
