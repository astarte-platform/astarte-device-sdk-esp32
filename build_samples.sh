#!/bin/bash

# (C) Copyright 2025, SECO Mind Srl
#
# SPDX-License-Identifier: Apache-2.0

# Function to display help message
display_help() {
    echo "Usage: $0 [OPTIONS]"
    echo
    echo "Options:"
    echo "  --fresh            Build the samples from scratch."
    echo "  --flash            Also flash the samples."
    echo "  --esp_path         Path to the folder containing the esp-idf installation."
    echo "  --sample           Build a specific sample."
    echo "  -h, --help         Display this help message."
}

# Set the fresh mode to off by default
fresh_mode=false
flash=false
sample=datastreams
esp_path=$HOME/esp

# Check for flags
while [[ "$#" -gt 0 ]]; do
    case $1 in
        --fresh) fresh_mode=true ;;
        --flash) flash=true ;;
        --esp_path) shift; esp_path="$1" ;;
        --sample) shift; sample="$1" ;;
        -h|--help) display_help; exit 0 ;;
        *) echo "Unknown option: $1"; display_help; exit 1 ;;
    esac
    shift
done

if [ ! -d "./examples/$sample" ]; then
    echo "Incorrect sample name: '$sample'"
    exit 1
fi
cd ./examples/$sample

if [ ! -f "$esp_path/esp-idf/export.sh" ]; then
    echo "Could not find the ESP IDF export script: '$esp_path/esp-idf/export.sh'"
    exit 1
fi
. $esp_path/esp-idf/export.sh

if $fresh_mode; then
    rm -r build/ sdkconfig dependencies.lock managed_components/
fi

idf.py build

if $flash; then
    idf.py flash
fi
