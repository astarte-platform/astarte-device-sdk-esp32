# (C) Copyright 2024, SECO Mind Srl
#
# SPDX-License-Identifier: Apache-2.0

"""docs.py

West extension that can be used to generate interfaces definitions.

Checked using pylint with the following command (pip install pylint):
python -m pylint --rcfile=./python_scripts/.pylintrc ./python_scripts/*.py
Formatted using black with the following command (pip install black):
python -m black --line-length 100 ./python_scripts/*.py
"""

import argparse
import json
import os
import sys
from pathlib import Path
from string import Template

from astarte.device import Interface
from colored import fore, stylize

reliability_lookup = {
    0: "UNRELIABLE",
    1: "GUARANTEED",
    2: "UNIQUE",
}

interface_header_template = Template(
    r"""/**
 * @file ${output_filename}.h
 * @brief Contains automatically generated interfaces.
 *
 * @warning Do not modify this file manually.
 *
 * @details The generated structures contain all information regarding each interface.
 * and are automatically generated from the json interfaces definitions.
 */

// NOLINTNEXTLINE This guard is clear enough.
#ifndef ${output_filename_cap}_H
#define ${output_filename_cap}_H

#include <astarte_device_sdk/interface.h>
#include <astarte_device_sdk/mapping.h>

// Interface names should resemble as closely as possible their respective .json file names.
// NOLINTBEGIN(readability-identifier-naming)
${interfaces_declarations}
// NOLINTEND(readability-identifier-naming)

#endif /* ${output_filename_cap}_H */
"""
)

interface_source_template = Template(
    r"""/**
 * @file ${output_filename}.c
 * @brief Contains automatically generated interfaces.
 *
 * @warning Do not modify this file manually.
 */

#include "${output_filename}.h"

// Interface names should resemble as closely as possible their respective .json file names.
// NOLINTBEGIN(readability-identifier-naming)
${interfaces_declarations}

// NOLINTEND(readability-identifier-naming)
"""
)

interface_declaration_template = Template(
    r"""extern const astarte_interface_t ${interface_name_sc};"""
)

interface_definition_template = Template(
    r"""
static const astarte_mapping_t ${interface_name_sc}_mappings[${mappings_number}] = {
${mappings}
};

const astarte_interface_t ${interface_name_sc} = {
    .name = "${interface_name}",
    .major_version = ${version_major},
    .minor_version = ${version_minor},
    .type = ${type},
    .ownership = ${ownership},
    .aggregation = ${aggregation},
    .mappings = ${interface_name_sc}_mappings,
    .mappings_length = ${mappings_number}U,
};"""
)

mapping_definition_template = Template(
    r"""
    {
        .endpoint = "${endpoint}",
        .type = ${type},
        .reliability = ${reliability},
        .explicit_timestamp = ${explicit_timestamp},
        .allow_unset = ${allow_unset},
    },"""
)


# pylint: disable-next=too-many-locals
def generate_interfaces(interfaces_dir: Path, output_dir: Path, output_fn: str, check: bool):
    """
    Generate the C files defining a set of interfaces starting from .json definitions.

    Parameters
    ----------
    interfaces_dir : Path
        Folder in which to search for .json files.
    output_dir : Path
        Folder where the generated files will be saved.
    output_fn : str
        Output file name without extension.
    check : bool
        Check if previously generated interfaces are up to date.
    """

    # Iterate over all the interfaces
    interfaces_declarations = []
    interfaces_structs = []
    for interface_file in sorted([i for i in interfaces_dir.iterdir() if i.suffix == ".json"]):
        with open(interface_file, "r", encoding="utf-8") as interface_fp:
            interface_json = json.load(interface_fp)
            interface = Interface(interface_json)

            # Iterate over each mapping
            mappings_struct = []
            for mapping in interface.mappings:
                # Fill in the mapping information in the template
                mapping_struct = mapping_definition_template.substitute(
                    endpoint=mapping.endpoint,
                    type="ASTARTE_MAPPING_TYPE_" + mapping.type.upper(),
                    reliability="ASTARTE_MAPPING_RELIABILITY_"
                    + reliability_lookup[mapping.reliability],
                    explicit_timestamp="true" if mapping.explicit_timestamp else "false",
                    allow_unset="true" if mapping.allow_unset else "false",
                )
                mappings_struct.append(mapping_struct)

            # Fill in the interface information in the template
            itype = "ASTARTE_INTERFACE_" + (
                "TYPE_PROPERTIES" if interface.is_type_properties() else "TYPE_DATASTREAM"
            )
            iownership = "ASTARTE_INTERFACE_" + (
                "OWNERSHIP_SERVER" if interface.is_server_owned() else "OWNERSHIP_DEVICE"
            )
            iaggregation = "ASTARTE_INTERFACE_" + (
                "AGGREGATION_OBJECT"
                if interface.is_aggregation_object()
                else "AGGREGATION_INDIVIDUAL"
            )
            interface_struct = interface_definition_template.substitute(
                mappings_number=len(interface.mappings),
                interface_name_sc=interface.name.replace(".", "_").replace("-", "_"),
                interface_name=interface.name,
                version_major=interface.version_major,
                version_minor=interface.version_minor,
                type=itype,
                ownership=iownership,
                aggregation=iaggregation,
                mappings="".join(mappings_struct),
            )
            interfaces_structs.append(interface_struct)

            # Fill in the extern definition
            interface_declaration = interface_declaration_template.substitute(
                interface_name_sc=interface.name.replace(".", "_").replace("-", "_")
            )
            interfaces_declarations.append(interface_declaration)

    # Fill in the header
    interfaces_header = interface_header_template.substitute(
        output_filename=output_fn,
        output_filename_cap=output_fn.upper(),
        interfaces_declarations="\n".join(interfaces_declarations),
    )

    interfaces_source = interface_source_template.substitute(
        output_filename=output_fn, interfaces_declarations="\n".join(interfaces_structs)
    )

    # Write the output files if required
    generated_header = output_dir.joinpath(f"{output_fn}.h")
    generated_source = output_dir.joinpath(f"{output_fn}.c")
    if check:
        if not output_dir.exists():
            print(stylize("Check failed: non existant output directory", fore("yellow")))
            sys.exit(1)

        with open(generated_header, "r", encoding="utf-8") as generated_fp:
            if generated_fp.read() != interfaces_header:
                print(stylize("Check failed: header is not up to date", fore("yellow")))
                sys.exit(1)

        with open(generated_source, "r", encoding="utf-8") as generated_fp:
            if generated_fp.read() != interfaces_source:
                print(stylize("Check failed: source is not up to date", fore("yellow")))
                sys.exit(1)
    else:
        # Generate directory
        if not output_dir.exists():
            os.makedirs(output_dir)

        # Generate files
        with open(generated_header, "w", encoding="utf-8") as generated_fp:
            generated_fp.write(interfaces_header)

        with open(generated_source, "w", encoding="utf-8") as generated_fp:
            generated_fp.write(interfaces_source)

# When running as a standalone script
if __name__ == "__main__":
    # Command line arguments are often duplicated.
    # pylint: disable=duplicate-code

    # Parse command line arguments
    parser = argparse.ArgumentParser(
        description="Generates C interfaces definitions from .json definitions."
    )
    parser.add_argument("json_dir", help="Directory containing the interfaces .json file(s).")
    parser.add_argument(
        "-p",
        "--output_prefix",
        help="Prefix to add to all generated sources.",
        type=str,
        default="",
    )
    parser.add_argument(
        "-d",
        "--output_dir",
        help="Directory where the generated headers will be stored.",
        default=None,
    )
    parser.add_argument(
        "-c",
        "--check",
        help="Check if previously generated interfaces are up to date.",
        action="store_true",
    )

    args = parser.parse_args()

    interfaces_dir = Path(args.json_dir).absolute()
    output_dir = Path(args.output_dir).absolute() if args.output_dir else interfaces_dir
    output_filename = args.output_prefix + "generated_interfaces"

    generate_interfaces(interfaces_dir, output_dir, output_filename, args.check)
