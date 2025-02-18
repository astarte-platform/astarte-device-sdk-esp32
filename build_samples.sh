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
    echo "  --sample           Build a specific sample."
    echo "  -h, --help         Display this help message."
}

# Set the fresh mode to off by default
fresh_mode=false
flash=false
sample=datastreams

# Check for flags
while [[ "$#" -gt 0 ]]; do
    case $1 in
        --fresh) fresh_mode=true ;;
        --flash) flash=true ;;
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
get_idf
if $fresh_mode; then
    rm -r build/ sdkconfig dependencies.lock managed_components/
fi
idf.py build
if $flash; then
    idf.py flash
fi
