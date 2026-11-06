#!/usr/bin/env python3
# Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""release/policy.toml and the SECURITY.md it renders.

Run: python3 -m unittest discover -s scripts/tests
"""

import copy
import datetime as dt
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent.parent
RELEASE = ROOT / "scripts" / "release"
sys.path.insert(0, str(RELEASE))

import policy  # noqa: E402
import semver  # noqa: E402

POLICY = {
    "schema": 1,
    "signing": {"legacy_unsigned_before": "2.2.5"},
    "series": {
        "2.3": {"status": "mainline", "branch": "main",
                "platforms": {"windows": "10.0.17763", "android_api": 28}},
        "2.2": {"status": "stable", "branch": "release/2.2",
                "platforms": {"macos": "15.0"}},
        "2.1": {"status": "legacy", "branch": "release/2.1",
                "eol": "2027-12-31", "successor": "2.2"},
        "1": {"status": "eol", "successor": "2.2"},
    },
}
TODAY = dt.date(2026, 11, 6)


def broken(**change):
    p = copy.deepcopy(POLICY)
    for path, value in change.items():
        node = p
        keys = path.split("__")
        for k in keys[:-1]:
            node = node[k.replace("_", ".")] if k[0].isdigit() else node[k]
        last = keys[-1]
        if value is None:
            del node[last]
        else:
            node[last] = value
    return p


class SemverTest(unittest.TestCase):
    def test_precedence(self):
        order = ["1.0.0-alpha", "1.0.0-alpha.1", "1.0.0-alpha.beta",
                 "1.0.0-beta", "1.0.0-beta.2", "1.0.0-beta.11",
                 "1.0.0-rc.1", "1.0.0"]  # the SemVer spec's own example
        parsed = [semver.parse(v) for v in order]
        self.assertEqual(parsed, sorted(reversed(parsed)))

    def test_build_metadata_does_not_order(self):
        self.assertEqual(semver.parse("2.4.0-alpha.3+gabc"),
                         semver.parse("2.4.0-alpha.3+gdef"))

    def test_dev_would_have_sorted_above_beta(self):
        # Why untagged builds are alpha.N.
        self.assertGreater(semver.parse("2.4.0-dev.1"),
                           semver.parse("2.4.0-beta.1"))
        self.assertLess(semver.parse("2.4.0-alpha.99"),
                        semver.parse("2.4.0-beta.1"))

    def test_release_tag_shapes(self):
        for tag in ("v2.3.0", "v2.3.0-beta.1", "v2.3.0-rc.12"):
            self.assertTrue(semver.RELEASE_TAG.match(tag), tag)
        for tag in ("v2.4.0-alpha.0", "nightly", "v2.3", "2.3.0"):
            self.assertFalse(semver.RELEASE_TAG.match(tag), tag)
        self.assertTrue(semver.BASE_TAG.match("v2.4.0-alpha.0"))


class PolicyCheckTest(unittest.TestCase):
    def test_the_repository_policy_holds(self):
        policy.check_policy(policy.load_policy())
        policy.check_product(policy.load_product())
        self.assertEqual(policy.mainline(policy.load_policy()), "2.3")

    def test_fixture_holds(self):
        self.assertEqual(policy.check_policy(POLICY, TODAY), [])

    def assert_refused(self, p, needle):
        with self.assertRaises(policy.PolicyError) as e:
            policy.check_policy(p, TODAY)
        self.assertIn(needle, str(e.exception))

    def test_two_mainlines(self):
        p = copy.deepcopy(POLICY)
        p["series"]["2.4"] = {"status": "mainline", "branch": "main",
                              "platforms": {}}
        self.assert_refused(p, "more than one mainline")

    def test_mainline_older_than_stable(self):
        p = copy.deepcopy(POLICY)
        p["series"]["2.3"]["status"] = "stable"
        p["series"]["2.3"]["branch"] = "release/2.3"
        p["series"]["2.2"]["status"] = "mainline"
        p["series"]["2.2"]["branch"] = "main"
        self.assert_refused(p, "mainline series must be newer")

    def test_legacy_needs_eol_and_successor(self):
        p = copy.deepcopy(POLICY)
        del p["series"]["2.1"]["eol"]
        self.assert_refused(p, "needs eol")
        p = copy.deepcopy(POLICY)
        p["series"]["2.1"]["successor"] = "9.9"
        self.assert_refused(p, "needs a successor")

    def test_supported_series_need_platforms(self):
        p = copy.deepcopy(POLICY)
        del p["series"]["2.2"]["platforms"]
        self.assert_refused(p, "needs a platforms table")
        p = copy.deepcopy(POLICY)
        p["series"]["2.2"]["platforms"]["android_api"] = "28"
        self.assert_refused(p, "must be a int")

    def test_branches_follow_status(self):
        p = copy.deepcopy(POLICY)
        p["series"]["2.2"]["branch"] = "main"
        self.assert_refused(p, 'lives on "release/2.2"')

    def test_withdrawal_needs_reason(self):
        p = copy.deepcopy(POLICY)
        p["withdrawn"] = [{"version": "2.2.3"}]
        self.assert_refused(p, "needs a reason")

    def test_passed_eol_is_a_warning(self):
        warnings = policy.check_policy(POLICY, dt.date(2028, 1, 1))
        self.assertTrue(any("has passed" in w for w in warnings))

    def test_product_bases_are_https(self):
        product = policy.load_product()
        product["bases"]["update"].append("http://example.org/")
        with self.assertRaises(policy.PolicyError):
            policy.check_product(product)


class SecurityRenderTest(unittest.TestCase):
    def test_repository_security_md_is_current(self):
        current = policy.SECURITY.read_text(encoding="utf-8")
        rendered = policy.render_security(current, policy.load_policy(),
                                          policy.load_product())
        self.assertEqual(current, rendered,
                         "run scripts/release/policy.py render-security")

    def test_table_follows_the_policy(self):
        table = policy.render_versions(POLICY)
        self.assertIn("| 2.1.x | Security fixes only; move to 2.2.x | "
                      ":white_check_mark: | 2027-12-31 |", table)
        self.assertIn("| 1.x.x | End of life; move to 2.2.x | :x: |", table)
        lines = [l for l in table.splitlines() if l.startswith("| 2")]
        self.assertTrue(lines[0].startswith("| 2.3.x"))

    def test_withdrawn_versions_are_listed(self):
        p = copy.deepcopy(POLICY)
        p["withdrawn"] = [{"version": "2.2.3", "reason": "broken key import"}]
        self.assertIn("- 2.2.3: broken key import", policy.render_versions(p))

    def test_render_needs_markers(self):
        with self.assertRaises(policy.PolicyError):
            policy.render_security("# no markers\n", POLICY,
                                   policy.load_product())


class ToolingIsProductAgnosticTest(unittest.TestCase):
    def test_no_product_name_outside_the_product_config(self):
        # The lower-case slug is what paths, URLs and keys would spell; the
        # build's own GPGFRONTEND_* CMake inputs are not a product reference.
        slug = policy.load_product()["slug"]
        for path in sorted(RELEASE.rglob("*")):
            if path.is_file() and path.suffix in (".py", ".sh"):
                self.assertNotIn(slug, path.read_text(encoding="utf-8"),
                                 f"{path} names the product; read "
                                 "release/product.json instead")


if __name__ == "__main__":
    unittest.main()
