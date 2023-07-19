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

# NOTE: This kind of mocking currently works on Linux targets only.
#       On Espressif chips, too many dependencies are missing at the moment.
message(STATUS "building MBEDTLS MOCKS")

idf_component_get_property(original_mbedtls_dir mbedtls COMPONENT_OVERRIDEN_DIR)

idf_component_mock(
    INCLUDE_DIRS
        "${original_mbedtls_dir}/mbedtls/include"
        "${original_mbedtls_dir}/mbedtls/include/mbedtls"
    REQUIRES esp_hw_support
    MOCK_HEADER_FILES
        "${original_mbedtls_dir}/mbedtls/include/mbedtls/md.h")
