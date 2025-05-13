#!/bin/bash

# (C) Copyright 2025, SECO Mind Srl
#
# SPDX-License-Identifier: Apache-2.0

check_only=false

# Function to display help
display_help() {
    echo "Usage: $0 [--check-only] [--help]"
    echo
    echo "Options:"
    echo "  --check-only    Run clang-format in check mode without making changes."
    echo "  --help          Display this help message."
    exit 0
}

# Parse flags
for arg in "$@"
do
    case $arg in
        --check-only)
            check_only=true
            ;;
        --help)
            display_help
            ;;
        *)
            echo "Unknown option: $arg"
            echo "Use --help to display usage information."
            exit 1
            ;;
    esac
done

# Check if the environment and dependencies are ok
if [ ! -d ".venv" ]; then
    python3 -m venv .venv
fi
source .venv/bin/activate
package_name="clang-format"
package_version="20.1.0"
installed_version=$( (pip show $package_name | grep Version | awk '{print $2}') || true)
if [ "$installed_version" != "$package_version" ]; then
    if ! pip install $package_name==$package_version; then
        echo "Failed to install $package_name version $package_version."
        exit 1
    fi
fi
package_name="black"
package_version="25.1.0"
installed_version=$( (pip show $package_name | grep Version | awk '{print $2}') || true)
if [ "$installed_version" != "$package_version" ]; then
    if ! pip install $package_name==$package_version; then
        echo "Failed to install $package_name version $package_version."
        exit 1
    fi
fi
package_name="isort"
package_version="6.0.1"
installed_version=$( (pip show $package_name | grep Version | awk '{print $2}') || true)
if [ "$installed_version" != "$package_version" ]; then
    if ! pip install $package_name==$package_version; then
        echo "Failed to install $package_name version $package_version."
        exit 1
    fi
fi

# Run clang-format
format_files=("src/*.c" "include/astarte_device_sdk/*.h" "private/*.h"
              "end_to_end_tests/app/main/*.c" "end_to_end_tests/app/main/src/*.c"
              "end_to_end_tests/app/main/include/*.h" "samples/**/main/*.c"
              "samples/**/main/src/*.c" "samples/**/main/include/*.h" "tests/host/*.c"
              "tests/host/*.h" "tests/common/*.h" "tests/common/*.c" "tests/target/*.c"
              "tests/target/*.h" "tests/host_app/main/*.c" "tests/target_app/main/*.c")
if [ "$check_only" = true ]; then
    command="--dry-run -Werror"
else
    command="-i"
fi
for file_pattern in "${format_files[@]}"; do
    if ! clang-format --style=file $command $file_pattern; then
        exit 1
    fi
done

# Run python formatter
format_files=("./python_scripts/*.py" "./end_to_end_tests/*.py")
if [ "$check_only" = true ]; then
    command="--diff --check"
else
    command=""
fi
for file_pattern in "${format_files[@]}"; do
    if ! black --line-length 100 $command $file_pattern; then
        exit 1
    fi
done

# Run isort formatter
format_files=("./python_scripts/*.py" "./end_to_end_tests/*.py")
if [ "$check_only" = true ]; then
    command="--check-only"
else
    command=""
fi
for file_pattern in "${format_files[@]}"; do
    if ! isort --profile black $command $file_pattern; then
        exit 1
    fi
done
