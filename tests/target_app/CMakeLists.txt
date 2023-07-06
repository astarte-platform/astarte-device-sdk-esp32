cmake_minimum_required(VERSION 3.5)

# Include the components directory of the main application:
set(EXTRA_COMPONENT_DIRS "${CMAKE_SOURCE_DIR}/../..")
list(APPEND EXTRA_COMPONENT_DIRS "${CMAKE_SOURCE_DIR}/../common")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(target_app)
