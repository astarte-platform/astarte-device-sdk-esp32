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
    echo "  --astarte-local    Connect to a local instance of Astarte."
    echo "  --platform         Choice between: host and target."
    echo "  --flash            Also flash the tests, only meaningful for target."
    echo "  --esp_path         Path to the folder containing the esp-idf installation."
    echo "  -h, --help         Display this help message."
}

# Set the fresh mode to off by default
fresh_mode=false
astarte_local=false
platform=host
flash=false
esp_path=$HOME/esp/esp-idf

# Check for flags
while [[ "$#" -gt 0 ]]; do
    case $1 in
        --fresh) fresh_mode=true ;;
        --astarte-local) astarte_local=true ;;
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
        --flash) flash=true ;;
        --esp_path) shift; esp_path="$1" ;;
        -h|--help) display_help; exit 0 ;;
        *) echo "Unknown option: $1"; display_help; exit 1 ;;
    esac
    shift
done

cd ./end_to_end_tests/app

if [ ! -f "$esp_path/export.sh" ]; then
    echo "Could not find the ESP IDF export script: '$esp_path/export.sh'"
    exit 1
fi
. $esp_path/export.sh


if $fresh_mode; then
    rm -r build/ sdkconfig dependencies.lock managed_components/
fi

package_name="tqdm"
if ! pip show $package_name > /dev/null 2>&1; then
    if ! pip install $package_name; then
        echo "Failed to install $package_name."
        exit 1
    fi
fi
package_name="bson"
if ! pip show $package_name > /dev/null 2>&1; then
    if ! pip install $package_name; then
        echo "Failed to install $package_name."
        exit 1
    fi
fi
package_name="requests"
if ! pip show $package_name > /dev/null 2>&1; then
    if ! pip install $package_name; then
        echo "Failed to install $package_name."
        exit 1
    fi
fi

python_flags="--platform $platform"
if $astarte_local; then
    python_flags="--astarte-local $python_flags"
fi

if [ "$platform" = "host" ]; then

tee sdkconfig.tmp << END
CONFIG_EXAMPLE_CONNECT_WIFI=n
CONFIG_EXAMPLE_CONNECT_ETHERNET=y
CONFIG_EXAMPLE_USE_OPENETH=y
END

    python ./../end_to_end.py $python_flags
else

tee sdkconfig.tmp << END
CONFIG_EXAMPLE_CONNECT_WIFI=y
END

    idf.py build

    if $flash; then
        python ./../end_to_end.py $python_flags
    fi
fi
