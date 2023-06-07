#
# This file is part of Astarte.
#
# Copyright 2023 SECO Mind S.r.l.
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
# SPDX-License-Identifier: CC0-1.0

"""
Utility script setting up the environment to run the 'idf.py clang-check' command.

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

from parse_clang_tidy_res import INFO, ERROR


def setup_clang_tidy(prj_dir: str, verbose: bool):
    """
    Setup the environment to run clang-tidy through the ESP IDF clang-check command.

    Parameters
    ----------
    prj_dir : str
        ESP IDF project directory to set up. Should be one of the examples in this repository.
    verbose : bool
        When true, stream to the STDOUT the output of all the commands, hide them otherwise.
    """

    cprint("Setting up the environment.", color=INFO, flush=True)

    run_clang_tidy_py = (
        "https://raw.githubusercontent.com/llvm/llvm-project/main/clang-tools-extra"
        + "/clang-tidy/tool/run-clang-tidy.py"
    )
    cmds = [
        "idf_tools.py install xtensa-clang",
        f"wget -O run-clang-tidy.py {run_clang_tidy_py}",
        "chmod +x run-clang-tidy.py",
    ]

    ret = subprocess.run(
        "&&".join(cmds),
        capture_output=not verbose,
        shell=True,
        cwd=prj_dir,
        check=False,
    )
    if ret.returncode:
        cprint("Failure in the setup.", color=ERROR, flush=True)
        print(ret.stderr, flush=True)
        print(ret.stdout, flush=True)
        sys.exit(1)


# When running as a standalone script
if __name__ == "__main__":
    # Command line arguments are often duplicated.
    # pylint: disable=duplicate-code

    # Parse command line arguments
    DESCTIPTION = (
        "Utility for setting up the ESP IDF clang-check. "
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
        "-v",
        "--verbose",
        dest="verbose",
        action="store_true",
        help="When set, print more information to the stdout.",
    )

    args = parser.parse_args()

    setup_clang_tidy(args.prj_dir, args.verbose)
