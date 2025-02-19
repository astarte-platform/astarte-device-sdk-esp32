#
# This file is part of Astarte.
#
# Copyright 2023 SECO Mind Srl
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

cmake_minimum_required(VERSION 3.5)

# Get the absolute path of the current directory
get_filename_component(CURRENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)
# Construct the absolute path to sdkconfig.priv
set(SDKCONFIG_PRIV_PATH "${CURRENT_DIR}/sdkconfig.priv")
# Construct the absolute path to sdkconfig.defaults
set(SDKCONFIG_DEFAULTS_PATH "${CURRENT_DIR}/sdkconfig.defaults")
# Check if sdkconfig.priv exists and add it to SDKCONFIG_DEFAULTS
if(EXISTS "${SDKCONFIG_PRIV_PATH}")
    if(EXISTS "${SDKCONFIG_DEFAULTS_PATH}")
        set(SDKCONFIG_DEFAULTS "sdkconfig.defaults;sdkconfig.priv")
    else()
        set(SDKCONFIG_DEFAULTS "sdkconfig.priv")
    endif()
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(encryption)
