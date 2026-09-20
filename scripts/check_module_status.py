#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Check a `gpgfrontend --module-status` report against what the build made.

The smoke tests on all three platforms ask one question: did the thing we ship
load its modules. They used to answer it by grepping a log line for
``loaded 4 module(s), refused 0``, which was wrong three ways:

* it matched a **sentence**, so improving the wording broke the test;
* it hardcoded a **count**, so a fifth module would break every platform at
  once and the obvious repair is to bump the number until it passes again;
* it had to **find the log**, whose location depends on the flavour -- a
  portable build keeps its profile beside the executable -- which reported a
  healthy build as broken.

The application writes a status report on request instead. This reads it.

Usage: check_module_status.py STATUS_JSON EXPECTED_COUNT

EXPECTED_COUNT is what the build actually produced, never a literal: the module
list CMake writes at configure time, or the `modules` array of the build
record, depending on which is at hand.
"""

import json
import sys


def main() -> int:
    if len(sys.argv) != 3:
        print(__doc__, file=sys.stderr)
        return 2

    path, expected_raw = sys.argv[1], sys.argv[2]

    try:
        report = json.load(open(path, encoding="utf-8"))
    except (OSError, ValueError) as exc:
        print(f"the status report could not be read: {exc}", file=sys.stderr)
        return 1

    try:
        expected = int(expected_raw)
    except ValueError:
        print(f"expected count is not a number: {expected_raw!r}",
              file=sys.stderr)
        return 2
    if expected <= 0:
        print("expected count is zero; the build produced no modules and "
              "this check would pass vacuously", file=sys.stderr)
        return 2

    for key in ("discovered", "loaded", "refused", "loaded_modules"):
        if key not in report:
            print(f"the status report has no {key!r}", file=sys.stderr)
            return 1

    problems = []

    if report["refused"]:
        problems.append(f"{report['refused']} module(s) refused")

    # Discovered, not just loaded: a module that never shipped is invisible to
    # a check that only counts what loaded, because nothing refused it either.
    if report["discovered"] != expected:
        problems.append(
            f"found {report['discovered']} module(s), "
            f"and this build produced {expected}")

    if report["loaded"] != expected:
        problems.append(f"loaded {report['loaded']} of {expected}")

    # The counts come from the loader and the list from the registry; they are
    # two views of one thing and a disagreement means one of them is stale.
    if len(report["loaded_modules"]) != report["loaded"]:
        problems.append(
            f"the identifier list names {len(report['loaded_modules'])} "
            f"module(s) and the count says {report['loaded']}")

    if problems:
        print("the shipped build did not load its modules:", file=sys.stderr)
        for problem in problems:
            print(f"  {problem}", file=sys.stderr)
        print(f"  searched: {report.get('integrated_module_path', '?')}",
              file=sys.stderr)
        return 1

    print(f"all {expected} module(s) loaded, none refused")
    for identifier in report["loaded_modules"]:
        print(f"  {identifier}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
