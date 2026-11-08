#!/usr/bin/env python3
# Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""release/policy.toml, the SECURITY.md it renders, the CI order check and
scripts/release/cut.sh.

Run: python3 -m unittest discover -s scripts/tests
"""

import copy
import datetime as dt
import json
import os
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent.parent
RELEASE = ROOT / "scripts" / "release"
sys.path.insert(0, str(RELEASE))

import check_order  # noqa: E402
import policy  # noqa: E402
import semver  # noqa: E402

GIT = shutil.which("git")
CMAKE = shutil.which("cmake")

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
        # Two lines, nightly and stable: only a final vX.Y.Z is a release.
        for tag in ("v2.3.0", "v10.20.30"):
            self.assertTrue(semver.RELEASE_TAG.match(tag), tag)
        for tag in ("v2.3.0-beta.1", "v2.3.0-rc.12", "v2.4.0-alpha.0",
                    "nightly", "v2.3", "2.3.0", "v2.3.0+meta"):
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

    def test_artifact_rules_name_known_kinds_and_compile(self):
        product = policy.load_product()
        product["artifacts"][0]["kind"] = "deb"
        with self.assertRaisesRegex(policy.PolicyError, "not one of"):
            policy.check_product(product)
        product = policy.load_product()
        product["artifacts"][0]["match"] = "^(unclosed"
        with self.assertRaisesRegex(policy.PolicyError, "does not compile"):
            policy.check_product(product)
        product = policy.load_product()
        del product["artifacts"][0]["flavor"]
        with self.assertRaisesRegex(policy.PolicyError, "lacks flavor"):
            policy.check_product(product)

    def test_keys_are_minisign_keys(self):
        for field in ("artifact_key", "index_keys", "artifact_keys_retired"):
            product = policy.load_product()
            product["signing"][field] = "RWQ=" if field == "artifact_key" \
                else ["RWQ="]
            with self.subTest(field=field), \
                    self.assertRaisesRegex(policy.PolicyError, "not a minisign"):
                policy.check_product(product)

    def test_nightly_retention_outlasts_the_margin(self):
        for bad in (7, 0, "30", True):
            product = policy.load_product()
            product["nightly_retention_days"] = bad
            with self.subTest(bad=bad), \
                    self.assertRaisesRegex(policy.PolicyError, "retention"):
                policy.check_product(product)

    def test_the_current_key_or_certificate_is_never_retired(self):
        for field, current in (("artifact_keys_retired", "artifact_key"),
                               ("authenticode_subjects_retired",
                                "authenticode_subject")):
            product = policy.load_product()
            product["signing"][field] = [product["signing"][current]]
            with self.assertRaisesRegex(policy.PolicyError, "current"):
                policy.check_product(product)
            product["signing"][field] = "not a list"
            with self.assertRaisesRegex(policy.PolicyError, "list"):
                policy.check_product(product)
            # Replaced: another key (here the index key stands in for one),
            # or another subject.
            product["signing"][field] = [
                product["signing"]["index_keys"][0]
                if field == "artifact_keys_retired" else "something replaced"]
            policy.check_product(product)

    def test_a_current_maintainer_is_never_retired(self):
        product = policy.load_product()
        current = product["signing"]["maintainer_fingerprints"][0]
        product["signing"]["maintainer_fingerprints_retired"] = [current]
        with self.assertRaisesRegex(policy.PolicyError, "lists a current"):
            policy.check_product(product)
        product["signing"]["maintainer_fingerprints_retired"] = ["abc"]
        with self.assertRaisesRegex(policy.PolicyError, "upper-case"):
            policy.check_product(product)
        product["signing"]["maintainer_fingerprints_retired"] = ["A" * 40]
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


class CheckOrderTest(unittest.TestCase):
    TAGS = [semver.parse(v) for v in ("2.1.13", "2.2.4", "2.2.5", "2.3.0-beta.1")]

    def test_main_newer_than_everything(self):
        self.assertEqual(check_order.check("2.3.0-beta.1.dev.4+gabc", "main",
                                           self.TAGS, "2.3"), [])

    def test_main_behind_a_release(self):
        tags = self.TAGS + [semver.parse("2.3.0")]
        problems = check_order.check("2.3.0-beta.1.dev.9", "main", tags, "2.3")
        self.assertTrue(any("base tag" in p for p in problems))

    def test_main_in_the_wrong_series(self):
        problems = check_order.check("2.4.0-alpha.1", "main", self.TAGS, "2.3")
        self.assertTrue(any("policy" in p for p in problems))

    def test_release_branch(self):
        self.assertEqual(check_order.check("2.2.6-alpha.1", "release/2.2",
                                           self.TAGS, "2.3"), [])
        self.assertTrue(check_order.check("2.4.0-alpha.1", "release/2.2",
                                          self.TAGS, "2.3"))
        # Exactly on the newest tag: that release itself.
        self.assertEqual(check_order.check("2.2.5", "release/2.2",
                                           self.TAGS, "2.3"), [])
        self.assertTrue(check_order.check("2.2.4-alpha.3+gabc", "release/2.2",
                                          self.TAGS, "2.3"))

    def test_release_tags_need_notes(self):
        with tempfile.TemporaryDirectory() as tmp:
            repo = Path(tmp)
            self.assertTrue(check_order.check_tag("v2.3.0", repo))
            (repo / "release" / "notes").mkdir(parents=True)
            (repo / "release" / "notes" / "2.3.0.md").write_text("  \n")
            self.assertTrue(check_order.check_tag("v2.3.0", repo))
            (repo / "release" / "notes" / "2.3.0.md").write_text("## 2.3.0\n")
            self.assertEqual(check_order.check_tag("v2.3.0", repo), [])
            # A base tag is no release and needs none; nor is a prerelease.
            self.assertEqual(check_order.check_tag("v2.4.0-alpha.0", repo), [])
            self.assertEqual(check_order.check_tag("v2.4.0-beta.1", repo), [])
            self.assertEqual(check_order.check_tag("v2.4.0-rc.1", repo), [])

    def test_other_refs_are_not_checked(self):
        self.assertEqual(check_order.check("1.0.0", "feature/x", self.TAGS,
                                           "2.3"), [])


def clean_env():
    env = {k: v for k, v in os.environ.items()
           if not k.startswith("GITHUB_") and k != "CI"}
    env.update({"GIT_AUTHOR_NAME": "t", "GIT_AUTHOR_EMAIL": "t@example.org",
                "GIT_COMMITTER_NAME": "t", "GIT_COMMITTER_EMAIL": "t@example.org",
                "GIT_CONFIG_GLOBAL": os.devnull, "GIT_CONFIG_NOSYSTEM": "1"})
    return env


@unittest.skipUnless(GIT and CMAKE, "needs git and cmake")
class CutTest(unittest.TestCase):
    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp(prefix="gfcut-"))
        self.repo = self.tmp / "repo"
        (self.repo / "release").mkdir(parents=True)
        shutil.copy(ROOT / "release" / "product.json", self.repo / "release")
        self.write_policy(POLICY)
        (self.repo / "SECURITY.md").write_text(
            "# Security\n<!-- policy:versions:begin -->\n"
            "<!-- policy:versions:end -->\n<!-- policy:keys:begin -->\n"
            "<!-- policy:keys:end -->\n")
        self.git("init", "-q", "-b", "main")
        self.git("add", ".")
        self.git("commit", "-q", "-m", "init")
        self.git("tag", "v2.2.4")
        self.git("commit", "-q", "--allow-empty", "-m", "work")
        self.git("tag", "v2.3.0")

    def tearDown(self):
        shutil.rmtree(self.tmp, ignore_errors=True)

    def git(self, *args):
        return subprocess.run(["git", *args], cwd=self.repo, env=clean_env(),
                              check=True, capture_output=True,
                              text=True).stdout.strip()

    def write_policy(self, p):
        lines = ["schema = 1", "", "[signing]",
                 f'legacy_unsigned_before = "{p["signing"]["legacy_unsigned_before"]}"']
        for key, entry in p["series"].items():
            lines += ["", f'[series."{key}"]']
            for k, v in entry.items():
                if k == "platforms":
                    continue
                lines.append(f'{k} = "{v}"')
            if "platforms" in entry:
                lines += ["", f'[series."{key}".platforms]']
                for k, v in entry["platforms"].items():
                    lines.append(f"{k} = {json.dumps(v)}")
        (self.repo / "release" / "policy.toml").write_text("\n".join(lines) + "\n")

    def cut(self, *args):
        return subprocess.run(["bash", str(RELEASE / "cut.sh"), *args],
                              cwd=self.repo, env=clean_env(),
                              capture_output=True, text=True)

    def flipped(self):
        p = copy.deepcopy(POLICY)
        p["series"]["2.4"] = {"status": "mainline", "branch": "main",
                              "platforms": {"windows": "10.0.17763"}}
        p["series"]["2.3"] = {"status": "stable", "branch": "release/2.3",
                              "platforms": {"windows": "10.0.17763"}}
        p["series"]["2.2"] = {"status": "legacy", "branch": "release/2.2",
                              "eol": "2027-06-30", "successor": "2.3"}
        return p

    def test_refuses_without_the_policy_flip(self):
        out = self.cut("2.3", "--no-sign")
        self.assertNotEqual(out.returncode, 0)
        self.assertIn("edit release/policy.toml", out.stderr)

    def test_cut_moves_main_on(self):
        self.write_policy(self.flipped())
        out = self.cut("2.3", "--no-sign")
        self.assertEqual(out.returncode, 0, out.stderr)

        self.assertEqual(self.git("rev-parse", "release/2.3"),
                         self.git("rev-parse", "v2.3.0^{commit}"))
        self.assertEqual(self.git("describe", "--tags", "--exact-match", "main"),
                         "v2.4.0-alpha.0")
        # The branch cannot reach the base tag.
        self.assertEqual(
            self.git("tag", "--merged", "release/2.3", "--list", "v2.4*"), "")
        self.assertIn("2.4.x", (self.repo / "SECURITY.md").read_text())
        self.assertIn("git push origin release/2.3 main v2.4.0-alpha.0",
                      out.stdout)

        for ref, want in (("main", "2.4"), ("release/2.3", "2.3")):
            self.git("checkout", "-q", ref)
            version = check_order.derived_version(self.repo, ref)
            self.assertEqual(semver.parse(version).series, want, version)
            problems = check_order.check(
                version, ref, check_order.release_tags(self.repo),
                policy.mainline(policy.load_policy(
                    self.repo / "release" / "policy.toml")))
            self.assertEqual(problems, [], ref)

    def test_refuses_twice(self):
        self.write_policy(self.flipped())
        self.assertEqual(self.cut("2.3", "--no-sign").returncode, 0)
        self.write_policy(self.flipped())
        out = self.cut("2.3", "--no-sign")
        self.assertNotEqual(out.returncode, 0)
        self.assertIn("already exists", out.stderr)

    def test_refuses_other_changes(self):
        self.write_policy(self.flipped())
        (self.repo / "release" / "product.json").write_text("{}\n")
        out = self.cut("2.3", "--no-sign")
        self.assertNotEqual(out.returncode, 0)
        self.assertIn("commit or stash", out.stderr)


if __name__ == "__main__":
    unittest.main()
