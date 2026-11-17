#!/usr/bin/env python3
# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""Every file CI publishes is named by scripts/ci/artifact_name.sh, and
every name it makes is one release/product.json's artifact rules read
back: gfrelease refuses to stage a file no rule describes, so a leg named
otherwise would fail the release, not a test.

Run: python3 -m unittest discover -s scripts/tests
"""

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

# On release/2.1 one workflow makes every published file, Qt5's included;
# the legs it names besides the disk images.
BUILD = ".github/workflows/build.yml"
LEGS = [
    "linux-x86_64.AppImage", "linux-aarch64.AppImage",
    "qt5-linux-x86_64.AppImage",
    "windows-x86_64.msi", "windows-x86_64-portable.zip",
    "qt5-windows-x86_64-portable.zip",
]


def macos_legs() -> list[str]:
    """The disk images: one per macOS leg build.yml's matrix names."""
    text = (WORKFLOWS / "build.yml").read_text(encoding="utf-8")
    labels = re.findall(r'leg: "(macos-[^"]+)"', text)
    if not labels:
        raise AssertionError("build.yml names no macOS leg")
    return [f"{label}.dmg" for label in labels]


class ArtifactNamesTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.product = Product(PRODUCT_FILE, POLICY_FILE)
        cls.legs = {BUILD: LEGS + macos_legs()}

    def test_every_leg_is_named_by_the_workflow(self):
        """Each leg above is one build.yml really names: the Qt6 AppImages
        by the runner's architecture, everything else literally."""
        text = (ROOT / BUILD).read_text(encoding="utf-8")
        self.assertIn('artifact_name.sh "linux-${ARCH}.AppImage"', text)
        for leg in LEGS:
            if leg.startswith("linux-"):
                continue
            with self.subTest(leg=leg):
                self.assertRegex(
                    text, rf'artifact_name\.sh"? {re.escape(leg)}\b')

    def test_qt5_files_are_their_own_flavour(self):
        version = "2.1.14"
        prefix = self.product.data["artifact_prefix"]
        for leg in ("qt5-linux-x86_64.AppImage",
                    "qt5-windows-x86_64-portable.zip"):
            with self.subTest(leg=leg):
                self.assertEqual(self.product.classify(
                    f"{prefix}{version}-{leg}", version)["flavor"], "qt5")

    def test_every_leg_ci_makes_is_one_the_rules_describe(self):
        version = "2.1.14-alpha.5-gabcdef012345"
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
