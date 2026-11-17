#!/usr/bin/env python3
# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""Every file CI publishes is named by scripts/ci/artifact_name.sh, and
every name it makes is one release/product.json's artifact rules read
back: gfrelease refuses to stage a file no rule describes, so a leg named
otherwise would fail the release, not a test.

Run: python3 -m unittest discover -s scripts/tests
"""

import json
import os
import re
import subprocess
import unittest

import _paths
from gfrelease.app.product import Product
from gfrelease.paths import POLICY_FILE, PRODUCT_FILE

ROOT = _paths.ROOT
HELPER = ROOT / "scripts" / "ci" / "artifact_name.sh"
WORKFLOWS = ROOT / ".github" / "workflows"

# Each producer of a published file, and the legs it names.
LEGS = {
    "scripts/ci/linux/package.sh": [
        f"linux-{arch}{flavor}.AppImage" for arch in ("x86_64", "aarch64")
        for flavor in ("", "-portable")],
    ".github/workflows/build-windows.yml": [
        "windows-x86_64.msi", "windows-x86_64-portable.zip"],
    "scripts/ci/android/sign.sh": [
        "android-arm64-v8a.apk", "android-x86_64.apk"],
}


def macos_legs() -> list[str]:
    """The disk images: one per macOS runner build.yml names, after it."""
    text = (WORKFLOWS / "build.yml").read_text(encoding="utf-8")
    runners = json.loads(re.search(r"macos_runners=(\[.*?\])", text).group(1))
    return [f"{label}.dmg" for label in runners]


class ArtifactNamesTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.product = Product(PRODUCT_FILE, POLICY_FILE)
        cls.legs = dict(LEGS)
        cls.legs[".github/workflows/sign-apple.yml"] = macos_legs()

    def test_every_leg_ci_makes_is_one_the_rules_describe(self):
        version = "2.3.0-alpha.5-gabcdef012345"
        prefix = self.product.data["artifact_prefix"]
        for producer, legs in self.legs.items():
            for leg in legs:
                with self.subTest(producer=producer, leg=leg):
                    self.product.classify(f"{prefix}{version}-{leg}", version)

    def test_each_producer_names_by_the_helper_only(self):
        for producer in self.legs:
            text = (ROOT / producer).read_text(encoding="utf-8")
            with self.subTest(producer=producer):
                self.assertIn("artifact_name.sh", text)
                # Never a name spelled by hand beside it.
                self.assertIsNone(re.search(
                    r"GpgFrontend-\$(\{?ARTIFACT_VERSION|env:|\{\{)", text))

    @unittest.skipIf(os.name == "nt", "CI's helper, run by sh")
    def test_the_helper_spells_the_products_prefix(self):
        out = subprocess.run(
            ["sh", str(HELPER), "windows-x86_64.msi"], capture_output=True,
            text=True, env=dict(os.environ, ARTIFACT_VERSION="2.3.0"),
            check=True).stdout.strip()
        self.assertEqual(
            out, f"{self.product.data['artifact_prefix']}2.3.0-"
            "windows-x86_64.msi")
        missing = subprocess.run(["sh", str(HELPER), "x"],
                                 capture_output=True, text=True,
                                 env={"PATH": os.environ.get("PATH", "")})
        self.assertNotEqual(missing.returncode, 0)


if __name__ == "__main__":
    unittest.main()
