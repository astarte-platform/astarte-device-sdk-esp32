#
# This file is part of Astarte.
#
# Copyright 2018-2023 SECO Mind Srl
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-License-Identifier: LGPL-2.1-or-later OR Apache-2.0
#

"""
Parser script for the warnings.txt file generated running the 'idf.py clang-check' command.
Prints the content in a humand readable format.

Before the first run install the requirements.txt
Before the first new run in a fresh terminal run the export script from the ESP IDF toolchain.

Checked using pylint with the following command:
python3.8 -m pylint --rcfile=./python_scripts/.pylintrc ./python_scripts/*.py
Formatted using black with the following command:
python3 -m black --line-length 100 ./python_scripts/*.py

"""
from __future__ import annotations

import sys
import os
import argparse

import textwrap
import re
from collections import namedtuple
import colored
from colored import stylize
from termcolor import cprint

INFO = "cyan"
WARNING = "yellow"
ERROR = "red"

SEVERITY_COLOUR = {"warning": "yellow", "error": "red", "note": "magenta"}
Warn = namedtuple("Warn", ["severity", "code", "message", "path", "line", "col"])

# Group 1: file path (can contain only alphanumeric chars and '/', '.', '-', ' ', '+')
# Group 2: line
# Group 3: column
# Group 4: error type
# Group 5: error message
# Group 6: error identifier
warn_header_pattern = re.compile(
    r"^([\w\/\.\-\ \+]+):(\d+):(\d+): (warning|error|note): (.+) \[([\w\-,\.]+)\]$"
)
clang_tidy_cmd_pattern = re.compile(
    r"^([\w\/\.\-\ \+]+clang-tidy) -p=build "
    r"--config-file=([\w\/\.\-\ \+]+\.clang-tidy) ([\w\/\.\-\ \+]+\.[ch])$"
)
clang_comp_err_pattern = re.compile(r"^clang-diagnostic-.*")


def warn_txt_remove_garbage_lines(warn_txt: str) -> list[str]:
    """
    Remove all the unecessary lines from the warnings.txt file.

    Two types of elements are filtered, using different regexes:
    - Enabled checks for clang-tidy. Positioned at the beginning of the warnings.txt file.
    - Warnings from clang tidy regarding compiler flags. Those warnings are filtered by matching
      against the compiler call and then removing all the following lines untill an acceptable line
      is found.

    Parameters
    ----------
    warn_txt : str
        Content of the warnings.txt file as a single string.

    Returns
    -------
    list[str]
        The remaining lines of the warnings.txt file after cleaning.
    """
    enabled_checks_pattern = r"^Enabled checks:\n(    [\w\-\.]*\n)*\n"

    enabled_checks_m = re.match(enabled_checks_pattern, warn_txt)
    if enabled_checks_m:
        warn_txt = warn_txt[enabled_checks_m.end() :]

    warn_txt = warn_txt.splitlines()

    lines_keep = []
    ignore_next = False
    for warn_txt_line in warn_txt:
        if clang_tidy_cmd_pattern.match(warn_txt_line):
            ignore_next = True
        elif warn_header_pattern.match(warn_txt_line):
            lines_keep.append(warn_txt_line)
            ignore_next = False
        elif not ignore_next:
            lines_keep.append(warn_txt_line)
    return lines_keep


def warn_txt_parse_header(warn_header, trim: bool = False) -> str:
    """
    Parse the first line of each warning of clang-tidy.

    Parameters
    ----------
    warn_header : str
        The first line of each warning as included in reports.txt.
    trim : bool
        When true, parse a compact version of the error header.

    Returns
    -------
    str
        The parsed warning header.
    """
    # Format the severity and error code
    pretty_warn_header = (
        "\n"
        + stylize(
            f"{warn_header.severity.capitalize()}[{warn_header.code}]:",
            colored.fg(SEVERITY_COLOUR[warn_header.severity]),
        )
        + "\n"
    )

    # Format the message
    if trim:
        message = textwrap.shorten(text=warn_header.message, width=80, initial_indent="Message: ")
    else:
        wrapper = textwrap.TextWrapper(
            width=100, initial_indent="Message: ", subsequent_indent=" " * 9
        )
        message = "\n".join(wrapper.wrap(text=warn_header.message)) + "."
    pretty_warn_header += message + "\n"

    # Format the location of the error
    if not trim:
        pretty_warn_header += stylize("--> ", colored.fg("blue"))
    pretty_warn_header += f"{warn_header.path}:{warn_header.line}:{warn_header.col}" + "\n"

    return pretty_warn_header


def warn_txt_parse(
    warn_txt_path: str, trim: bool = False, code_filter: str = r".*", skip_clang_diag: bool = False
) -> int:
    """
    Parse the all the warnings contained in the warnings.txt file produced by running clang-tidy
    through the ESP IDF clang-check command.

    Parameters
    ----------
    warn_txt_path : str
        Path to the generated reports.txt.
    trim : bool
        When true, parse a compact version of the error header.
    code_filter : str
        A regex pattern used to filter only a subset of the warnings.
    skip_clang_diag : bool
        Do not show clang compilation diagnostic warnings. Those warnings are not always appropriate
        for the ESP architecture.

    Returns
    -------
    int
        The number of detected warnings.
    """
    # Load the txt report
    with open(warn_txt_path, encoding="utf-8") as warn_txt_fp:
        warn_txt = warn_txt_fp.read()

        # Filter out garbage lines
        warn_txt_lines = warn_txt_remove_garbage_lines(warn_txt)

        # Loop over the good lines
        skip_warning = True
        comp_warn_count = 0
        good_warn_count = 0
        for warn_txt_line in warn_txt_lines:
            warn_header_m = warn_header_pattern.match(warn_txt_line)
            if warn_header_m:
                warn_header = Warn(
                    warn_header_m.group(4),
                    warn_header_m.group(6),
                    warn_header_m.group(5),
                    warn_header_m.group(1),
                    int(warn_header_m.group(2)),
                    int(warn_header_m.group(3)),
                )
                # Filter warnings using the provided filter
                if skip_clang_diag and clang_comp_err_pattern.match(warn_header.code):
                    skip_warning = True
                    comp_warn_count += 1
                elif re.match(code_filter, warn_header.code):
                    skip_warning = False
                    good_warn_count += 1
                    parsed_warn_header = warn_txt_parse_header(warn_header, trim)
                    print(parsed_warn_header, end="")
                else:
                    skip_warning = True
            elif (not skip_warning) and (not trim):
                print(stylize(" | ", colored.fg("blue")) + warn_txt_line)

        if skip_clang_diag:
            cprint(f"\nRemoved {comp_warn_count} compiler errors.", color=WARNING)

        cprint(f"\nFound {good_warn_count} errors/warnings/notes.", color=INFO, end="\n\n")

        return good_warn_count


# When running as a standalone script
if __name__ == "__main__":
    # Command line arguments are often duplicated.
    # pylint: disable=duplicate-code

    # Parse command line arguments
    parser = argparse.ArgumentParser(
        description="Parse the warnings.txt generated by the ESP IDF clang-check."
    )
    parser.add_argument(
        "-d",
        "--warnings-txt-dir",
        dest="warnings_txt_dir",
        default=os.path.join(os.getcwd(), "examples", "datastreams"),
        help="Location for the warnings.txt file.",
    )
    parser.add_argument(
        "-r",
        "--remove-clang-diag",
        dest="remove_clang_diag",
        action="store_true",
        help="Remove warnings and errors generated by the clang compiler diagnostic.",
    )
    parser.add_argument(
        "-f",
        "--filter",
        dest="code_filter",
        default=r".*",
        help="A regex used for filtering the clang-tidy error codes.",
    )
    parser.add_argument(
        "-t",
        "--trim",
        dest="trim",
        action="store_true",
        help="When set, trim the report printed to stdout.",
    )

    args = parser.parse_args()

    warnings_txt_path = os.path.join(args.warnings_txt_dir, "warnings.txt")
    if warn_txt_parse(warnings_txt_path, args.trim, args.code_filter, args.remove_clang_diag):
        sys.exit(1)
