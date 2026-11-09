#!/usr/bin/env python3
# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""scripts/ci/build_mode.sh: which runs build what, and which publish it.

Every signing, provenance, staging and upload job in build.yml gates on
`publish_kind`, so this table is the whole answer to "does a push of this ref
reach users": main's nightlies, release/X.Y's betas and vX.Y.Z releases. A
base tag or a prerelease tag publishes nothing.
"""

import os
import shutil
import subprocess
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent.parent
sys.path.insert(0, str(ROOT / "scripts" / "release"))

import refs  # noqa: E402
import semver  # noqa: E402

SCRIPT = ROOT / "scripts" / "ci" / "build_mode.sh"


def run_script(event: str, ref: str, **extra):
    env = {k: v for k, v in os.environ.items() if not k.startswith("GITHUB_")
           and k not in ("BUILD_MODE_INPUT", "SCREENSHOT_INPUT")}
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
        ("push", "refs/heads/release/2.2", "nightly", "beta"),
        ("push", "refs/heads/release/2.2-fix", "nightly", "none"),
        ("push", "refs/heads/release/2.2/x", "nightly", "none"),
        ("push", "refs/heads/feature/x", "nightly", "none"),
        ("push", "refs/tags/v2.3.0", "release", "final"),
        ("push", "refs/tags/v2.4.0-alpha.0", "nightly", "none"),
        ("push", "refs/tags/v2.3.0-beta.1", "nightly", "none"),
        ("push", "refs/tags/v2.3.0-rc.1", "nightly", "none"),
        ("push", "refs/tags/v2.08.0", "nightly", "none"),
        ("push", "refs/tags/v2.3.0.1", "nightly", "none"),
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

    def test_a_dispatch_without_a_known_mode_fails(self):
        for mode in ("", "beta", "release\n"):
            with self.subTest(mode=mode):
                out = run_script("workflow_dispatch", "refs/heads/main",
                                 BUILD_MODE_INPUT=mode)
                self.assertNotEqual(out.returncode, 0)
                self.assertIn("build_mode", out.stderr)
        self.assertEqual(classify("workflow_dispatch", "refs/heads/main",
                                  BUILD_MODE_INPUT="nightly")["build_mode"],
                         "nightly")

    def test_what_the_ref_is_whatever_the_event(self):
        # mas-sandbox.yml signs on this, on a manual run as on a push.
        for ref, kind in (("refs/heads/main", "main"),
                          ("refs/heads/release/2.2", "release-branch"),
                          ("refs/tags/v2.3.0", "release-tag"),
                          ("refs/tags/v2.3", "other"),
                          ("refs/tags/v2.3.0-rc.1", "other"),
                          ("refs/heads/release/2.2-fix", "other")):
            for event in ("push", "workflow_dispatch"):
                with self.subTest(ref=ref, event=event):
                    self.assertEqual(classify(event, ref)["ref_kind"], kind)

    def test_the_screenshot_build_is_never_a_release(self):
        out = run_script("push", "refs/tags/v2.3.0", SCREENSHOT_INPUT="true")
        self.assertNotEqual(out.returncode, 0)
        self.assertIn("screenshot", out.stderr)
        nightly = classify("push", "refs/heads/main", SCREENSHOT_INPUT="true")
        self.assertIn("SCREENSHOT_MODE=ON", nightly["screenshot_cmake_flag"])

    def test_play_gets_mains_nightlies_and_the_finals(self):
        # Each to its own app; whether a final is newer than what its app
        # holds is Play's to say (play_upload.py --only-if-newer).
        for event, ref, expected in (
                ("push", "refs/heads/main", "true"),
                ("push", "refs/heads/release/2.3", "false"),
                ("push", "refs/tags/v2.4.0", "true"),
                ("push", "refs/tags/v2.3.1", "true"),
                ("push", "refs/tags/v2.4.0-rc.1", "false"),
                ("push", "refs/tags/v2.08.0", "false"),
                ("pull_request", "refs/pull/7/merge", "false"),
                ("workflow_dispatch", "refs/tags/v2.4.0", "false")):
            with self.subTest(event=event, ref=ref):
                self.assertEqual(classify(event, ref)["play_upload"], expected)

    def test_a_nightly_is_its_own_android_application(self):
        for event, ref, package in (
                ("push", "refs/heads/main", "com.bktus.gpgfrontend.nightly"),
                ("push", "refs/heads/release/2.3",
                 "com.bktus.gpgfrontend.nightly"),
                ("push", "refs/tags/v2.4.0", "com.bktus.gpgfrontend"),
                ("pull_request", "refs/pull/7/merge",
                 "com.bktus.gpgfrontend.nightly")):
            with self.subTest(ref=ref):
                out = classify(event, ref)
                self.assertEqual(out["android_package"], package)
                self.assertEqual(out["gpgfrontend_build_mode"] == "release",
                                 package == "com.bktus.gpgfrontend")

    def test_the_ref_shapes_match_the_tooling(self):
        # The shell patterns and scripts/release/refs.py must agree, or CI
        # stages a build the release server then refuses (or skips one it
        # would take).
        for tag in ("v2.3.0", "v10.0.1", "v0.0.0", "v2.3.0-beta.1",
                    "v2.4.0-alpha.0", "v2.3", "v2.3.0+meta", "nightly",
                    "v2.08.0", "v02.3.0", "v2.3.0.1"):
            with self.subTest(tag=tag):
                published = classify("push", f"refs/tags/{tag}")[
                    "publish_kind"] == "final"
                self.assertEqual(published, refs.is_release_tag(tag))
                self.assertEqual(published,
                                 bool(semver.RELEASE_TAG.match(tag)))
        for branch in ("main", "release/2.2", "release/10.11",
                       "release/2.2-fix", "release/2", "release/2.2/x",
                       "mainline", "feature/release/2.2"):
            ref = f"refs/heads/{branch}"
            with self.subTest(ref=ref):
                published = classify("push", ref)["publish_kind"] != "none"
                self.assertEqual(published, refs.publishes_builds(ref))


if __name__ == "__main__":
    unittest.main()
