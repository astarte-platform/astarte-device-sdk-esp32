# (C) Copyright 2025, SECO Mind Srl
#
# SPDX-License-Identifier: Apache-2.0

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
project(astarte_app)
