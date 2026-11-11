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
TRUST_FIXTURE = ROOT / "scripts" / "tests" / "fixtures" / "trust" / "root"
sys.path.insert(0, str(RELEASE))

import check_order  # noqa: E402
import gitversion  # noqa: E402
import policy  # noqa: E402
import refs  # noqa: E402
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
        # Only a final vX.Y.Z is a release: nightlies (main) and betas
        # (release/X.Y) are builds, and are never tagged.
        for tag in ("v2.3.0", "v10.20.30", "v0.0.0"):
            self.assertTrue(refs.is_release_tag(tag), tag)
            self.assertTrue(semver.RELEASE_TAG.match(tag), tag)
        for tag in ("v2.3.0-beta.1", "v2.3.0-rc.12", "v2.4.0-alpha.0",
                    "nightly", "v2.3", "2.3.0", "v2.3.0+meta", "v2.3.0.1",
                    # A trailing newline, a leading zero, another script's
                    # digits: none is the tag it resembles.
                    "v2.3.0\n", "v2.08.0", "v02.3.0", "v\u0662.3.0"):
            self.assertFalse(refs.is_release_tag(tag), repr(tag))
            self.assertFalse(semver.RELEASE_TAG.match(tag), repr(tag))

    def test_a_version_is_ascii_and_whole(self):
        for text in ("\u0662.3.0", "2.03.0", "2.3.0-01", "2.3.0\n.1", "2.3",
                     "2.3.0-", "2.3.0+"):
            with self.subTest(text), self.assertRaises(ValueError):
                semver.parse(text)
        # What git prints ends in a newline.
        self.assertEqual(str(semver.parse("v2.3.0-alpha.4+gabc\n")),
                         "2.3.0-alpha.4+gabc")
        self.assertTrue(semver.parse("2.3.0").is_release)
        self.assertFalse(semver.parse("2.3.0+gabc").is_release)
        self.assertTrue(semver.parse("2.3.0+gabc").is_final)
        self.assertFalse(semver.parse("2.3.0-alpha.1").is_final)


class RefsTest(unittest.TestCase):
    def test_which_refs_publish_builds(self):
        for ref, series in (("refs/heads/main", ""),
                            ("refs/heads/release/2.2", "2.2"),
                            ("refs/heads/release/10.0", "10.0")):
            self.assertTrue(refs.publishes_builds(ref), ref)
            self.assertEqual(refs.release_series(ref), series)
        for ref in ("refs/heads/release/2.2-fix", "refs/heads/release/2.2/x",
                    "refs/heads/release/2", "refs/heads/release/2.2\n",
                    "refs/heads/feature/x", "refs/heads/mainline",
                    "refs/tags/v2.3.0", "refs/tags/main", "main",
                    "refs/pull/1/merge", ""):
            self.assertFalse(refs.publishes_builds(ref), repr(ref))
            self.assertEqual(refs.release_series(ref), "")

    def test_refs_take_apart_and_go_together(self):
        self.assertEqual(refs.branch_of("refs/heads/release/2.2"),
                         "release/2.2")
        self.assertEqual(refs.branch_of("refs/tags/v2.3.0"), "")
        self.assertEqual(refs.tag_of("refs/tags/v2.3.0"), "v2.3.0")
        self.assertEqual(refs.tag_of("refs/heads/main"), "")
        self.assertEqual(refs.release_tag_ref("2.3.0"), "refs/tags/v2.3.0")
        self.assertEqual(refs.release_series(
            "refs/heads/" + refs.release_branch("2.3")), "2.3")

    def test_a_commit_is_a_full_lower_case_id(self):
        self.assertTrue(refs.is_commit("a" * 40))
        for text in ("a" * 39, "a" * 41, "A" * 40, "g" * 40, "a" * 40 + "\n",
                     None, 7):
            self.assertFalse(refs.is_commit(text), repr(text))


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

    def test_nightly_retention_outlasts_the_margin(self):
        # Longer than an index lives (14 days) and the margin after it.
        for bad in (7, 15, 0, "30", True):
            product = policy.load_product()
            product["nightly_retention_days"] = bad
            with self.subTest(bad=bad), \
                    self.assertRaisesRegex(policy.PolicyError, "retention"):
                policy.check_product(product)
        product = policy.load_product()
        product["nightly_retention_days"] = policy.NIGHTLY_MARGIN_DAYS + 1
        policy.check_product(product)

    def test_what_identifies_the_build_and_names_its_files_is_checked(self):
        for field, bad in (("artifact_prefix", ""), ("artifact_prefix", "a/b"),
                           ("slug", "gpgfrontend\n"),
                           ("notes_link_hosts", "bktus.com"),
                           ("notes_link_hosts", [7])):
            product = policy.load_product()
            product[field] = bad
            with self.subTest(field=field, bad=bad), \
                    self.assertRaisesRegex(policy.PolicyError, field):
                policy.check_product(product)

    def test_a_file_of_the_wrong_shape_is_refused_not_a_crash(self):
        """Whatever a hand edit made of either file, the check names what
        is wrong; it never stops on a traceback."""
        bad = (None, 5, "x", [], [1], True, {"a": []})

        def mutate(document, check):
            for key in document:
                for value in bad:
                    changed = copy.deepcopy(document)
                    changed[key] = value
                    with self.subTest(key=key, value=value):
                        try:
                            check(changed)
                        except policy.PolicyError:
                            pass
        mutate(POLICY, lambda p: policy.check_policy(p, TODAY))
        mutate(policy.load_product(), policy.check_product)
        for value in bad:
            p = copy.deepcopy(POLICY)
            p["series"]["2.2"] = value
            with self.subTest(series=value):
                self.assert_refused(p, "series")
            p = copy.deepcopy(POLICY)
            p["withdrawn"] = [value]
            with self.subTest(withdrawn=value), \
                    self.assertRaises(policy.PolicyError):
                policy.check_policy(p, TODAY)
            product = policy.load_product()
            product["artifacts"][0] = value
            with self.subTest(rule=value), \
                    self.assertRaises(policy.PolicyError):
                policy.check_product(product)


class SecurityRenderTest(unittest.TestCase):
    def test_repository_security_md_is_current(self):
        current = policy.SECURITY.read_text(encoding="utf-8")
        # SECURITY.md's keys are the newest signed root's.
        rendered = policy.render_security(current, policy.load_policy(),
                                          policy.newest_root())
        self.assertEqual(current, rendered,
                         "run scripts/release/policy.py render-security")

    def test_table_follows_the_policy(self):
        table = policy.render_versions(POLICY)
        rows = {cells[0]: cells for cells in
                ([c.strip() for c in line.strip("|").split("|")]
                 for line in table.splitlines() if line.startswith("| "))}
        self.assertEqual(rows["2.1.x"], ["2.1.x",
                                         "Security fixes only; move to 2.2.x",
                                         "Yes", "2027-12-31"])
        self.assertEqual(rows["1.x.x"], ["1.x.x", "End of life; move to 2.2.x",
                                         "No", ""])
        lines = [l for l in table.splitlines() if l.startswith("| 2")]
        self.assertTrue(lines[0].startswith("| 2.3.x"))

    def test_security_md_reads_as_plain_text(self):
        # Read in a terminal as often as rendered: no emoji shortcodes, the
        # table's columns line up, and nothing generated is wider than 79.
        p = copy.deepcopy(POLICY)
        p["series"]["2.1"]["note"] = "the last series that builds with Qt5"
        p["withdrawn"] = [{"version": "2.2.3", "reason": "broken key " * 12}]
        text = policy.render_versions(p) + "\n" + policy.render_keys(
            p, policy.newest_root(TRUST_FIXTURE))
        self.assertNotIn(":white_check_mark:", text)
        self.assertNotRegex(text, r"(?m)\| :x: ")
        self.assertIn("2.1.x: the last series that builds with Qt5.", text)
        for line in text.splitlines():
            # A URL in a code block is the one thing never broken.
            if not line.startswith("    https://"):
                self.assertLessEqual(len(line), policy.WIDTH, line)
        table = [l for l in text.splitlines() if l.startswith("| ")]
        self.assertEqual(len({len(l) for l in table}), 1, table)

    def test_withdrawn_versions_are_listed(self):
        p = copy.deepcopy(POLICY)
        p["withdrawn"] = [{"version": "2.2.3", "reason": "broken key import"}]
        self.assertIn("- 2.2.3: broken key import", policy.render_versions(p))

    def test_render_needs_markers(self):
        with self.assertRaises(policy.PolicyError):
            policy.render_security("# no markers\n", POLICY,
                                   policy.newest_root(TRUST_FIXTURE))

    def test_the_keys_are_the_newest_roots(self):
        keys = policy.render_keys(POLICY, policy.newest_root(TRUST_FIXTURE))
        self.assertIn("\n    12F7E8858CF15BEC9975FF3C5CA3DA246843FD03\n", keys)
        self.assertIn("saturneric/GpgFrontend/.github/workflows/build.yml",
                      keys)


    def test_an_unsigned_draft_root_is_not_shown(self):
        # `gfrelease root new` writes the next root before anyone signed
        # it: SECURITY.md keeps showing the newest one that is signed.
        with tempfile.TemporaryDirectory() as tmp:
            roots = Path(tmp) / "root"
            shutil.copytree(TRUST_FIXTURE, roots)
            draft = json.loads((roots / "3.root.json").read_text())
            draft["version"] = 4
            draft["host_policy"]["repository"] = "someone/else"
            (roots / "4.root.json").write_text(json.dumps(draft))
            (roots / "4.root.json.new").write_text("left behind")
            self.assertEqual(policy.newest_root(roots)["version"], 3)

    def test_a_root_without_host_policy_is_an_error_not_a_traceback(self):
        with tempfile.TemporaryDirectory() as tmp:
            roots = Path(tmp)
            doc = json.loads((TRUST_FIXTURE / "1.root.json").read_text())
            del doc["host_policy"]
            (roots / "1.root.json").write_text(json.dumps(doc))
            (roots / "1.root.json.0123456789ABCDEF.minisig").write_text("x")
            with self.assertRaisesRegex(policy.PolicyError, "host_policy"):
                policy.newest_root(roots)
            with self.assertRaisesRegex(policy.PolicyError, "no signed root"):
                policy.newest_root(roots / "missing")


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
    # Releases only: the base tag vX.Y.0-alpha.0 and anything else that is
    # not a final vX.Y.Z is no release (check_order.release_tags).
    TAGS = [semver.parse(v) for v in ("2.1.13", "2.2.4", "2.2.5")]

    def test_main_newer_than_everything(self):
        self.assertEqual(check_order.check("2.3.0-alpha.4+gabc", "main",
                                           self.TAGS, "2.3"), [])

    def test_main_behind_a_release(self):
        tags = self.TAGS + [semver.parse("2.3.0")]
        problems = check_order.check("2.3.0-alpha.9+gabc", "main", tags, "2.3")
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
        for ref in ("feature/x", "release/2.2-fix", "release/2.2/x"):
            self.assertEqual(check_order.check("1.0.0", ref, self.TAGS,
                                               "2.3"), [], ref)


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
        shutil.copytree(TRUST_FIXTURE, self.repo / "release" / "root")
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
            version = gitversion.derive(self.repo, ref=ref)
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
