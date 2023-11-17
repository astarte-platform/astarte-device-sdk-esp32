#
# This file is part of Astarte.
#
# Copyright 2018-2023 SECO Mind Srl
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-License-Identifier: LGPL-2.1-or-later OR Apache-2.0
#

idf_component_register(
    SRCS
        "./src/astarte_bson.c"
        "./src/astarte_bson_deserializer.c"
        "./src/astarte_bson_serializer.c"
        "./src/astarte_credentials.c"
        "./src/astarte_device.c"
        "./src/astarte_hwid.c"
        "./src/astarte_pairing.c"
        "./src/astarte_storage.c"
        "./src/astarte_nvs_key_value.c"
        "./src/uuid.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "private"
    PRIV_REQUIRES vfs mbedtls fatfs mqtt nvs_flash esp_http_client json wpa_supplicant)
