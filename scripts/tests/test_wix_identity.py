#!/usr/bin/env python3
# Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""The MSI release/2.1 builds is the product every earlier GpgFrontend MSI
installed, 2.1.13's included, so it upgrades one in place.

On release/2.1 there is one identity (no nightly "GpgFrontend Testing"
product) and no .gfp association. Checks resource/wix/Package.wxs and its
build script; no WiX toolset is needed. The expected values are those of
GpgFrontend-2.1.13-windows-x86_64.msi (its Property, Upgrade and Component
tables).

Run: python3 -m unittest discover -s scripts/tests
"""

import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent.parent
WXS = (ROOT / "resource" / "wix" / "Package.wxs").read_text(encoding="utf-8")
BUILD_MSI = (ROOT / "scripts" / "ci" / "windows" / "build_msi.ps1").read_text(
    encoding="utf-8")


def defines():
    return dict(re.findall(r'<\?define (\w+) = "([^"]*)" \?>', WXS))


class WixIdentityTest(unittest.TestCase):
    def test_keeps_what_existing_installs_have(self):
        # Changing any of these would orphan every installed release.
        d = defines()
        self.assertEqual(d["ProductName"], "GpgFrontend")
        self.assertEqual(d["UpgradeCode"].lower(),
                         "3c642b54-3921-43ce-b744-a2bf0b4fc309")
        self.assertEqual(d["GuidProductInformation"],
                         "{B5718CF9-F3A3-43B0-BE3A-DEF48CA19E0A}")
        self.assertEqual(d["GuidAppDir"],
                         "{6F8D11AA-3B0C-4695-8C6A-622C60768DA7}")
        self.assertIn('<StandardDirectory Id="AppDataFolder">', WXS)
        self.assertIn('Scope="perUser"', WXS)
        self.assertIn(r'Key="Software\Saturneric\$(ProductName)"', WXS)

    def test_no_literal_where_the_identity_belongs(self):
        body = WXS.split("<Wix ", 1)[1]
        self.assertIn('UpgradeCode="$(UpgradeCode)"', body)
        self.assertIn('Name="$(ProductName)"', body)
        self.assertNotRegex(body, r'Guid="\{[0-9A-F-]+\}"')

    def test_one_identity_and_no_profile_association(self):
        body = WXS.split("<Wix ", 1)[1]
        self.assertNotIn("$(Identity)", WXS)
        self.assertNotIn("Testing", body)
        self.assertNotIn(".gfp", body)

    def test_upgrades_replace_never_stack(self):
        self.assertRegex(WXS, r'<MajorUpgrade[^>]*AllowDowngrades="no"')
        self.assertRegex(WXS, r'<MajorUpgrade[^>]*Schedule="afterInstallValidate"')

    def test_the_cabinet_is_one_mszip(self):
        # The offline signing stage repacks the cabinet and reads MSZIP only.
        self.assertRegex(WXS, r'<MediaTemplate EmbedCab="yes" '
                              r'CompressionLevel="mszip"')

    def test_the_build_installs_no_portable_payload(self):
        self.assertIn("bin/ENV.ini", BUILD_MSI)
        self.assertIn("throw", BUILD_MSI)
        self.assertIn("$info.version_core", BUILD_MSI)
        self.assertIn("$wixVersion = '5.0.2'", BUILD_MSI)


if __name__ == "__main__":
    unittest.main()
