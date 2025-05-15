# (C) Copyright 2025, SECO Mind Srl
#
# SPDX-License-Identifier: Apache-2.0


class TimeoutException(Exception):
    pass


class MismatchException(Exception):
    pass


class HttpException(Exception):
    pass


class ShellException(Exception):
    pass
