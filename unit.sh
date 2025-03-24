#!/bin/bash

# (C) Copyright 2025, SECO Mind Srl
#
# SPDX-License-Identifier: Apache-2.0

# Function to display help message
display_help() {
    echo "Usage: $0 [OPTIONS]"
    echo
    echo "Options:"
    echo "  --fresh            Build the tests from scratch."
    echo "  --platform         Choice between: host and target."
    echo "  --esp_path         Path to the folder containing the esp-idf installation."
    echo "  -h, --help         Display this help message."
}

# Set the fresh mode to off by default
fresh_mode=false
platform=host
esp_path=$HOME/esp

# Check for flags
while [[ "$#" -gt 0 ]]; do
    case $1 in
        --fresh) fresh_mode=true ;;
        --platform)
            shift
            case "$1" in
                "host"|"target")
                    platform="$1"
                    ;;
                *)
                    echo "Invalid argument for --platform. Use host or target."
                    exit 1
                    ;;
            esac
            ;;
        --esp_path) shift; esp_path="$1" ;;
        -h|--help) display_help; exit 0 ;;
        *) echo "Unknown option: $1"; display_help; exit 1 ;;
    esac
    shift
done

cd ./tests/"$platform"_app || exit 1

if [ ! -f "$esp_path/esp-idf/export.sh" ]; then
    echo "Could not find the ESP IDF export script: '$esp_path/esp-idf/export.sh'"
    exit 1
fi
. $esp_path/esp-idf/export.sh

if $fresh_mode; then
    rm -r build/ sdkconfig
fi

idf.py build

if [ "$platform" = "host" ]; then
    ./build/host_app.elf
else
    idf.py flash monitor
fi
