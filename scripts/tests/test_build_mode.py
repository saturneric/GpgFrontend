#!/usr/bin/env python3
# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""scripts/ci/build_mode.sh: which runs build what, and which publish it.

Every signing, provenance, staging and upload job in build.yml gates on
`publish_kind`, so this table is the whole answer to "does a push of this ref
reach users". Two lines, nightly and stable: a base tag or a prerelease tag
publishes nothing.
"""

import os
import subprocess
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent.parent
sys.path.insert(0, str(ROOT / "scripts" / "release"))

import semver  # noqa: E402

SCRIPT = ROOT / "scripts" / "ci" / "build_mode.sh"


def classify(event: str, ref: str) -> dict:
    env = {k: v for k, v in os.environ.items() if not k.startswith("GITHUB_")}
    env.update(GITHUB_EVENT_NAME=event, GITHUB_REF=ref, GITHUB_SHA="0" * 40)
    if event == "workflow_dispatch":
        env["BUILD_MODE_INPUT"] = "release"
    out = subprocess.run(["bash", str(SCRIPT)], cwd=ROOT, env=env, check=True,
                         capture_output=True, text=True).stdout
    return dict(line.split("=", 1) for line in out.splitlines() if "=" in line)


class BuildModeTest(unittest.TestCase):
    CASES = [
        # event, ref, build_mode, publish_kind
        ("push", "refs/heads/main", "nightly", "nightly-main"),
        ("push", "refs/heads/release/2.2", "nightly", "nightly-branch"),
        ("push", "refs/heads/release/2.2-fix", "nightly", "none"),
        ("push", "refs/heads/feature/x", "nightly", "none"),
        ("push", "refs/tags/v2.3.0", "release", "final"),
        ("push", "refs/tags/v2.4.0-alpha.0", "nightly", "none"),
        ("push", "refs/tags/v2.3.0-beta.1", "nightly", "none"),
        ("push", "refs/tags/v2.3.0-rc.1", "nightly", "none"),
        ("push", "refs/tags/nightly", "nightly", "none"),
        ("pull_request", "refs/pull/7/merge", "pr", "none"),
        ("workflow_dispatch", "refs/heads/main", "release", "none"),
    ]

    def test_each_ref(self):
        for event, ref, mode, kind in self.CASES:
            with self.subTest(event=event, ref=ref):
                out = classify(event, ref)
                self.assertEqual(out["build_mode"], mode)
                self.assertEqual(out["publish_kind"], kind)
                self.assertEqual(out["publishes"],
                                 "false" if kind == "none" else "true")

    def test_the_release_tag_shape_matches_the_tooling(self):
        # The shell pattern and semver.RELEASE_TAG must agree, or CI stages a
        # tag the release server then refuses (or skips one it would take).
        for tag in ("v2.3.0", "v10.0.1", "v2.3.0-beta.1", "v2.4.0-alpha.0",
                    "v2.3", "v2.3.0+meta", "nightly"):
            with self.subTest(tag=tag):
                published = classify("push", f"refs/tags/{tag}")[
                    "publish_kind"] == "final"
                self.assertEqual(published,
                                 bool(semver.RELEASE_TAG.match(tag)))


if __name__ == "__main__":
    unittest.main()
