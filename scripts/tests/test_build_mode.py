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
import shutil
import subprocess
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent.parent
sys.path.insert(0, str(ROOT / "scripts" / "release"))

import semver  # noqa: E402

SCRIPT = ROOT / "scripts" / "ci" / "build_mode.sh"


def run_script(event: str, ref: str, **extra):
    env = {k: v for k, v in os.environ.items() if not k.startswith("GITHUB_")
           and k not in ("BUILD_MODE_INPUT", "SCREENSHOT_INPUT",
                         "GF_MAIN_BUILD_NUMBER")}
    env.update(GITHUB_EVENT_NAME=event, GITHUB_REF=ref, GITHUB_SHA="0" * 40)
    if event == "workflow_dispatch":
        env["BUILD_MODE_INPUT"] = "release"
    env.update(extra)
    return subprocess.run(["bash", str(SCRIPT)], cwd=ROOT, env=env,
                          capture_output=True, text=True)


def classify(event: str, ref: str, **extra) -> dict:
    out = run_script(event, ref, **extra)
    out.check_returncode()
    return dict(line.split("=", 1) for line in out.stdout.splitlines()
                if "=" in line)


@unittest.skipUnless(shutil.which("cmake") and shutil.which("git"),
                     "needs cmake and git (the version is derived)")
class BuildModeTest(unittest.TestCase):
    CASES = [
        # event, ref, build_mode, publish_kind
        ("push", "refs/heads/main", "nightly", "nightly-main"),
        ("push", "refs/heads/release/2.2", "nightly", "nightly-branch"),
        ("push", "refs/heads/release/2.2-fix", "nightly", "none"),
        ("push", "refs/heads/release/2.2/x", "nightly", "none"),
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

    def test_build_type(self):
        self.assertEqual(classify("pull_request", "refs/pull/7/merge")
                         ["build_type"], "RelWithDebInfo")
        for event, ref in (("push", "refs/heads/main"),
                           ("push", "refs/tags/v2.3.0"),
                           ("workflow_dispatch", "refs/heads/main")):
            with self.subTest(ref=ref):
                self.assertEqual(classify(event, ref)["build_type"], "Release")

    def test_a_dispatch_without_a_mode_fails(self):
        out = run_script("workflow_dispatch", "refs/heads/main",
                         BUILD_MODE_INPUT="")
        self.assertNotEqual(out.returncode, 0)
        self.assertIn("build_mode", out.stderr)

    def test_the_screenshot_build_is_never_a_release(self):
        out = run_script("push", "refs/tags/v2.3.0", SCREENSHOT_INPUT="true")
        self.assertNotEqual(out.returncode, 0)
        self.assertIn("screenshot", out.stderr)
        nightly = classify("push", "refs/heads/main", SCREENSHOT_INPUT="true")
        self.assertIn("SCREENSHOT_MODE=ON", nightly["screenshot_cmake_flag"])

    def test_play_gets_only_what_beats_mains_build_number(self):
        # main builds 2.4.0-alpha.37: Play already holds that code, so only
        # a release above it (2.4.0, or a later series) can be uploaded.
        main = "1204000037"
        for ref, expected in (("refs/heads/main", "true"),
                              ("refs/heads/release/2.3", "false"),
                              ("refs/tags/v2.4.0", "true"),
                              ("refs/tags/v2.5.0", "true"),
                              ("refs/tags/v2.3.1", "false"),
                              ("refs/tags/v2.2.9", "false"),
                              ("refs/tags/v2.4.0-rc.1", "false")):
            with self.subTest(ref=ref):
                self.assertEqual(classify("push", ref,
                                          GF_MAIN_BUILD_NUMBER=main)
                                 ["play_upload"], expected)

    def test_play_matches_the_build_number_the_build_derives(self):
        # The release's number as build_mode.sh computes it from the tag is
        # the one cmake/GitVersion.cmake gives a commit on that tag.
        out = classify("push", "refs/tags/v2.4.0",
                       GF_MAIN_BUILD_NUMBER="1204008999")
        self.assertEqual(out["play_upload"], "true")
        out = classify("push", "refs/tags/v2.4.0",
                       GF_MAIN_BUILD_NUMBER="1204009000")
        self.assertEqual(out["play_upload"], "false")

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
