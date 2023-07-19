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

cmake_minimum_required(VERSION 3.16)

set(COMPONENTS main)

list(APPEND EXTRA_COMPONENT_DIRS "${CMAKE_SOURCE_DIR}/../common")
list(APPEND EXTRA_COMPONENT_DIRS "${CMAKE_SOURCE_DIR}/../host")
list(APPEND EXTRA_COMPONENT_DIRS "$ENV{IDF_PATH}/tools/mocks/freertos/")
list(APPEND EXTRA_COMPONENT_DIRS "$ENV{IDF_PATH}/tools/mocks/esp_hw_support/")
list(APPEND EXTRA_COMPONENT_DIRS "${CMAKE_SOURCE_DIR}/../mocks/esp_system")
list(APPEND EXTRA_COMPONENT_DIRS "${CMAKE_SOURCE_DIR}/../mocks/mbedtls")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

add_compile_definitions(UNIT_TEST)

idf_build_set_property(COMPILE_DEFINITIONS "-DNO_DEBUG_STORAGE" APPEND)
project(host_app)
