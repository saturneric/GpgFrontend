#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Merge the per-shard GoogleTest XML reports written by scripts/run_tests.sh
# into a single report, so <phase>.xml keeps meaning the whole phase even when
# the phase was sharded across processes.
#
# Shards are disjoint by construction (GTEST_SHARD_INDEX assigns test case i to
# shard i % N), so merging is a concatenation: group <testsuite> elements by
# name, concatenate their <testcase> children, and recompute the counters.
#
# Usage: merge_gtest_xml.py -o merged.xml shard0.xml shard1.xml ...

import argparse
import sys
import xml.etree.ElementTree as ET

COUNTERS = ("tests", "failures", "disabled", "errors", "skipped")


def _int(elem, name):
    try:
        return int(elem.get(name) or 0)
    except ValueError:
        return 0


def _float(elem, name):
    try:
        return float(elem.get(name) or 0)
    except ValueError:
        return 0.0


def _recount(suite):
    """Derive a suite's counters from the cases it actually holds.

    The shard reports are trusted for per-case results but not for their own
    totals: a suite split across shards has a partial count in each.
    """
    cases = suite.findall("testcase")
    suite.set("tests", str(len(cases)))
    for name, tag in (("failures", "failure"), ("errors", "error")):
        suite.set(name, str(sum(len(c.findall(tag)) for c in cases)))
    suite.set(
        "skipped", str(sum(1 for c in cases if c.get("result") == "skipped"))
    )
    suite.set(
        "disabled",
        str(sum(1 for c in cases if c.get("status") == "notrun")),
    )
    suite.set("time", "%.3f" % sum(_float(c, "time") for c in cases))


def merge(paths):
    merged = None
    suites = {}   # name -> element, in first-seen order
    order = []
    timestamps = []

    for path in paths:
        root = ET.parse(path).getroot()
        if merged is None:
            merged = ET.Element("testsuites", dict(root.attrib))
        if root.get("timestamp"):
            timestamps.append(root.get("timestamp"))
        for suite in root.findall("testsuite"):
            name = suite.get("name", "")
            if name not in suites:
                copy = ET.SubElement(merged, "testsuite", dict(suite.attrib))
                suites[name] = copy
                order.append(name)
            target = suites[name]
            for case in suite.findall("testcase"):
                target.append(case)

    if merged is None:
        raise SystemExit("no input files")

    for name in order:
        _recount(suites[name])

    for counter in COUNTERS:
        merged.set(
            counter, str(sum(_int(suites[n], counter) for n in order))
        )
    # Sum of case times, matching what a serial run reports.
    merged.set(
        "time", "%.3f" % sum(_float(suites[n], "time") for n in order)
    )
    if timestamps:
        merged.set("timestamp", min(timestamps))
    return merged


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("-o", "--output", required=True)
    ap.add_argument("inputs", nargs="+")
    args = ap.parse_args()

    merged = merge(args.inputs)
    tree = ET.ElementTree(merged)
    ET.indent(tree, space="  ")
    tree.write(args.output, encoding="UTF-8", xml_declaration=True)
    print(
        "merged %d shard report(s) -> %s (%s tests)"
        % (len(args.inputs), args.output, merged.get("tests")),
        file=sys.stderr,
    )


if __name__ == "__main__":
    main()
