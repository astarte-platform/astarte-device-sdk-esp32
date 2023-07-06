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
