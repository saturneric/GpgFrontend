#!/usr/bin/env python3
# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
#
# This file is part of GpgFrontend.
#
# GpgFrontend is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# GpgFrontend is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GpgFrontend. If not, see <https://www.gnu.org/licenses/>.
#
# SPDX-License-Identifier: GPL-3.0-or-later
"""Assert a signed bundle carries exactly the entitlements asked for.

Usage: compare_entitlements.py <expected.plist> <actual.plist>
"""

import plistlib
import sys


def main(expected_path, actual_path):
    with open(expected_path, "rb") as f:
        expected = plistlib.load(f)
    with open(actual_path, "rb") as f:
        actual = plistlib.load(f)

    if expected == actual:
        print("entitlements match policy (%d keys)" % len(expected))
        return 0

    for k in sorted(set(expected) - set(actual)):
        print("missing entitlement:    %s = %r" % (k, expected[k]), file=sys.stderr)
    for k in sorted(set(actual) - set(expected)):
        print("unexpected entitlement: %s = %r" % (k, actual[k]), file=sys.stderr)
    for k in sorted(set(expected) & set(actual)):
        if expected[k] != actual[k]:
            print("changed entitlement:    %s: expected %r, got %r"
                  % (k, expected[k], actual[k]), file=sys.stderr)
    return 1


if __name__ == "__main__":
    sys.exit(main(*sys.argv[1:3]))
