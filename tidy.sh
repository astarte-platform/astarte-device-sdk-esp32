#!/bin/bash

# (C) Copyright 2025, SECO Mind Srl
#
# SPDX-License-Identifier: Apache-2.0

# Function to display help message
display_help() {
    echo "Usage: $0 [OPTIONS]"
    echo
    echo "Options:"
    echo "  --fresh            Execute analysis from scratch."
    echo "  --esp_path         Path to the folder containing the esp-idf installation."
    echo "  --sample           Run over a specific sample."
    echo "  --file             Only print diagnostics for the a single file."
    echo "  -h, --help         Display this help message."
}

# Set defaults for the command line arguments
fresh_mode=false
esp_path=$HOME/esp/esp-idf
sample=astarte_app
file=""

# Check for flags
while [[ "$#" -gt 0 ]]; do
    case $1 in
        --fresh) fresh_mode=true ;;
        --esp_path) shift; esp_path="$1" ;;
        --sample) shift; sample="$1" ;;
        --file) shift; file="$1" ;;
        -h|--help) display_help; exit 0 ;;
        *) echo "Unknown option: $1"; display_help; exit 1 ;;
    esac
    shift
done

# Check if the sample exists
if [ ! -d "./samples/$sample" ]; then
    echo "Incorrect sample name: '$sample'"
    exit 1
fi
cd ./samples/"$sample" || exit 1

export IDF_TOOLCHAIN="clang"

# Export idf.py
if [ ! -f "$esp_path/export.sh" ]; then
    echo "Could not find the ESP IDF export script: '$esp_path/export.sh'"
    exit 1
fi
. "$esp_path"/export.sh || exit 1

# Install dependencies if required
idf_tools.py install esp-clang

# Re-export idf.py
if [ ! -f "$esp_path/export.sh" ]; then
    echo "Could not find the ESP IDF export script: '$esp_path/export.sh'"
    exit 1
fi
. "$esp_path"/export.sh

# Re-generate the compilation database if fresh is selected
if $fresh_mode; then
    rm -r build/ sdkconfig dependencies.lock managed_components/
fi

idf.py clang-check --run-clang-tidy-options "$PWD"/../../.clang-tidy --include-paths "$PWD"/../.. --exclude-paths "$PWD"/managed_components 1>/dev/null

cd "$PWD"/../.. || exit 1

# Check if the environment and dependencies are ok
if [ ! -d ".venv" ]; then
    python3 -m venv .venv
fi
source .venv/bin/activate
package_name="colored"
installed_version=$( (pip show $package_name | grep Version | awk '{print $2}') || true)
if [ "$installed_version" == "" ]; then
    if ! pip install $package_name; then
        echo "Failed to install $package_name."
        exit 1
    fi
fi

parser_options=""
if [[ -n "$file" ]]; then
    regex=$(echo "$file" | sed 's/\./\\./g' | sed 's/^/.*&/' | sed 's/$/$/')
    parser_options="--ffilter $regex"
fi

python3 python_scripts/parse_clang_tidy_res.py $parser_options
