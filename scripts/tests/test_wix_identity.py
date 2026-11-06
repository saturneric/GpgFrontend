#!/usr/bin/env python3
# Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""The MSI's two identities: a nightly ("GpgFrontend Testing") installs beside
a release, never over it.

Checks the preprocessor blocks of resource/wix/Package.wxs, and that the build
hands the identity through to WiX. No WiX toolset is needed: what matters is
which values each identity defines and what is guarded by it.

Run: python3 -m unittest discover -s scripts/tests
"""

import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent.parent
WXS = (ROOT / "resource" / "wix" / "Package.wxs").read_text(encoding="utf-8")
BUILD_MSI = (ROOT / "scripts" / "ci" / "windows" / "build_msi.ps1").read_text(
    encoding="utf-8")
CMAKE = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")

PER_IDENTITY = ("ProductName", "UpgradeCode", "ProfileProgId",
                "GuidProductInformation", "GuidProfileFileAssociation",
                "GuidAppDir")


def defines(identity):
    block = re.search(
        r'<\?(?:else)?if \$\(Identity\) = "' + identity + r'" \?>(.*?)<\?(?:elseif|else)',
        WXS, re.S).group(1)
    return dict(re.findall(r'<\?define (\w+) = "([^"]*)" \?>', block))


def guarded_by_stable():
    return "".join(re.findall(
        r'<\?if \$\(Identity\) = "stable" \?>\n(.*?)<\?endif\?>', WXS, re.S)[1:])


class WixIdentityTest(unittest.TestCase):
    def test_each_identity_defines_everything(self):
        for identity in ("stable", "testing"):
            self.assertEqual(sorted(defines(identity)), sorted(PER_IDENTITY),
                             identity)

    def test_identities_share_nothing(self):
        stable, testing = defines("stable"), defines("testing")
        for name in PER_IDENTITY:
            self.assertNotEqual(stable[name].lower(), testing[name].lower(), name)

    def test_stable_keeps_what_existing_installs_have(self):
        # Changing any of these would orphan every installed release.
        stable = defines("stable")
        self.assertEqual(stable["ProductName"], "GpgFrontend")
        self.assertEqual(stable["UpgradeCode"],
                         "3c642b54-3921-43ce-b744-a2bf0b4fc309")
        self.assertEqual(stable["ProfileProgId"], "GpgFrontend.Profile.1")
        self.assertEqual(stable["GuidAppDir"],
                         "{6F8D11AA-3B0C-4695-8C6A-622C60768DA7}")

    def test_no_literal_where_the_identity_belongs(self):
        body = WXS.split("<Wix ", 1)[1]
        self.assertIn('UpgradeCode="$(UpgradeCode)"', body)
        self.assertIn('Name="$(ProductName)"', body)
        self.assertNotRegex(body, r'Guid="\{[0-9A-F-]+\}"')
        self.assertNotIn("GpgFrontend.Profile.1", body)

    def test_shared_names_are_stable_only(self):
        # .gfp's default handler and Applications\GpgFrontend.exe are keyed by
        # names both identities would share.
        guarded = guarded_by_stable()
        self.assertIn(r'Key="Software\Classes\.gfp"', guarded)
        self.assertIn(r"Applications\GpgFrontend.exe", guarded)
        unguarded = WXS
        for block in re.findall(r'<\?if \$\(Identity\) = "stable" \?>\n.*?<\?endif\?>',
                                WXS, re.S):
            unguarded = unguarded.replace(block, "")
        self.assertNotIn(r"Applications\GpgFrontend.exe", unguarded)
        self.assertNotIn(r'Key="Software\Classes\.gfp">', unguarded)

    def test_the_build_names_the_identity(self):
        self.assertIn('"identity": "$<IF:${GPGFRONTEND_BUILD_STABLE_INT},'
                      'stable,testing>"', CMAKE.replace('\\"', '"'))
        self.assertIn('-d Identity="$identity"', BUILD_MSI)
        self.assertIn("$info.identity", BUILD_MSI)


if __name__ == "__main__":
    unittest.main()
