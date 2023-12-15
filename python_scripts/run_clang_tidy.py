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
Utility script running the 'idf.py clang-check' command and then parsing and printing the result in
a human readable format.

Before the first run install the requirements.txt
Before the first new run in a fresh terminal run the export script from the ESP IDF toolchain.

Checked using pylint with the following command:
python3.8 -m pylint --rcfile=./python_scripts/.pylintrc ./python_scripts/*.py
Formatted using black with the following command:
python3 -m black --line-length 100 ./python_scripts/*.py

"""

import sys
import os
import argparse
import subprocess
from termcolor import cprint

from setup_clang_tidy import setup_clang_tidy
from parse_clang_tidy_res import warn_txt_parse, INFO, ERROR


def run_clang_tidy(prj_dir: str, verbose: bool, clang_tidy_cfg_dir: str):
    """
    Run clang-tidy through the ESP IDF clang-check command.

    Parameters
    ----------
    prj_dir : str
        ESP IDF project directory configured for clang-check. Should be one of the examples in this
        repository.
    verbose : bool
        When true, stream to the STDOUT the output of all the commands, hide them otherwise.
    clang_tidy_cfg_dir : str
        Absolute path to the .clang-tidy configuration file to pass to clang-tidy.
    """
    cprint("Running the checker.", color=INFO, flush=True)

    run_clang_tidy_py_args = f"-config-file={clang_tidy_cfg_dir}"
    cmds = [
        'export PATH="$PWD:$PATH"',
        " ".join(
            [
                "idf.py",
                "clang-check",
                "--run-clang-tidy-options",
                f'"{run_clang_tidy_py_args}"',
                "--include-paths $PWD/../..",
                "--exclude-paths $PWD/managed_components",
            ]
        ),
    ]
    ret = subprocess.run(
        "&&".join(cmds),
        capture_output=not verbose,
        shell=True,
        cwd=prj_dir,
        check=False,
    )
    if ret.returncode:
        cprint("Failure in running the checker.", color=ERROR, flush=True)
        print(ret.stderr, flush=True)
        print(ret.stdout, flush=True)
        sys.exit(1)

    # Check if resulting report is not empty.
    with open(os.path.join(prj_dir, "warnings.txt"), encoding="utf-8") as warnings_txt_fp:
        if len(warnings_txt_fp.readlines()) < 2:
            cprint("Clang tidy did not generate the correct output.", color=ERROR, flush=True)
            sys.exit(1)


# When running as a standalone script
if __name__ == "__main__":
    # Command line arguments are often duplicated.
    # pylint: disable=duplicate-code

    # Parse command line arguments
    DESCTIPTION = (
        "Utility for running ESP IDF clang-check and analyze the result. "
        r"The export.sh\export.bat\export.fish script should be run before this script. "
        "The default values of --project-dir and --clang-tidy-dir assume you are running this"
        " script from the main folder in this repository."
    )
    parser = argparse.ArgumentParser(description=DESCTIPTION)
    parser.add_argument(
        "-p",
        "--project-dir",
        dest="prj_dir",
        default=os.path.join(os.getcwd(), "examples", "datastreams"),
        help=(
            "ESP IDF project directory in which to clang-run should be run. "
            "Defaults to the datastreams example folder."
        ),
    )
    parser.add_argument(
        "-c",
        "--clang-tidy-dir",
        dest="clang_tidy_cfg_dir",
        default=os.path.join(os.getcwd(), ".clang-tidy"),
        help="The absolute path to the .clang-tidy configuration file.",
    )
    parser.add_argument(
        "-a",
        "--also_install",
        dest="also_install",
        action="store_true",
        help="Setup clang-tidy before running it.",
    )
    parser.add_argument(
        "-n",
        "--no-run",
        dest="no_run",
        action="store_true",
        help="When set, do not run. Use the result of the last run.",
    )
    parser.add_argument(
        "-t",
        "--trim",
        dest="trim",
        action="store_true",
        help="When set, trim the report printed to stdout.",
    )
    parser.add_argument(
        "-v",
        "--verbose",
        dest="verbose",
        action="store_true",
        help="When set, print more information to the stdout.",
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
        help="A regex used for filtering the clang-tidy error codes. Defaults to '*.'",
    )

    args = parser.parse_args()

    if args.also_install and not args.no_run:
        setup_clang_tidy(args.prj_dir, args.verbose)
    else:
        cprint("Skipping the environment configuration.", color=INFO, flush=True)

    if not args.no_run:
        run_clang_tidy(args.prj_dir, args.verbose, args.clang_tidy_cfg_dir)
    else:
        cprint("Using an old analysis result.", color=INFO, flush=True)

    if warn_txt_parse(
        os.path.join(args.prj_dir, "warnings.txt"),
        args.trim,
        args.code_filter,
        args.remove_clang_diag,
    ):
        sys.exit(1)
