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
            shift # Remove --check-only from processing
            ;;
        --help)
            display_help
            shift # Remove --help from processing
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
package_version="19.1.6"
installed_version=$((pip show $package_name | grep Version | awk '{print $2}') || true)
if [ "$installed_version" != "$package_version" ]; then
    pip install $package_name==$package_version
    if [ $? -ne 0 ]; then
        echo "Failed to install $package_name version $package_version."
        exit 1
    fi
fi

# Run clang-format
format_files=("src/*.c" "include/*.h" "private/*.h" "examples/**/main/*.c" "examples/**/main/src/*.c"
              "examples/**/main/include/*.h" "tests/host/*.c" "tests/host/*.h" "tests/common/*.h"
              "tests/common/*.c" "tests/target/*.c" "tests/target/*.h" "tests/host_app/main/*.c"
              "tests/target_app/main/*.c")
if [ "$check_only" = true ]; then
    command="--dry-run -Werror"
else
    command="-i"
fi
for file_pattern in "${format_files[@]}"; do
    clang-format --style=file $command $file_pattern
    if [ $? -ne 0 ]; then
        exit 1
    fi
done
