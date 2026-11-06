#!/usr/bin/env python3
# Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""cmake/GitVersion.cmake, the one version derivation, on throwaway repos.

Every case builds a real git history and asks the script what a build of it
would call itself, so the rules are tested through git describe itself, not a
copy of its output.

Run: python3 -m unittest discover -s scripts/tests
"""

import json
import os
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent.parent
SCRIPT = ROOT / "cmake" / "GitVersion.cmake"
sys.path.insert(0, str(ROOT / "scripts" / "release"))

import semver  # noqa: E402

CMAKE = shutil.which("cmake")
GIT = shutil.which("git")


def clean_env(**extra):
    env = {k: v for k, v in os.environ.items()
           if not k.startswith("GITHUB_") and k != "CI"}
    env.update({
        "GIT_AUTHOR_NAME": "t", "GIT_AUTHOR_EMAIL": "t@example.org",
        "GIT_COMMITTER_NAME": "t", "GIT_COMMITTER_EMAIL": "t@example.org",
        "GIT_CONFIG_GLOBAL": os.devnull, "GIT_CONFIG_NOSYSTEM": "1",
    })
    env.update(extra)
    return env


@unittest.skipUnless(CMAKE and GIT, "needs cmake and git")
class GitVersionTest(unittest.TestCase):

    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp(prefix="gfver-"))
        self.repo = self.tmp / "repo"
        self.repo.mkdir()
        self.git("init", "-q", "-b", "main")
        self.n = 0

    def tearDown(self):
        shutil.rmtree(self.tmp, ignore_errors=True)

    def git(self, *args):
        return subprocess.run([GIT, *args], cwd=self.repo, env=clean_env(),
                              check=True, capture_output=True,
                              text=True).stdout.strip()

    def commit(self, count=1):
        for _ in range(count):
            self.n += 1
            (self.repo / "f").write_text(str(self.n))
            self.git("add", "f")
            self.git("commit", "-q", "-m", f"c{self.n}")

    def derive(self, src=None, env=None, defines=()):
        cmd = [CMAKE, f"-DGF_SOURCE_DIR={src or self.repo}", *defines,
               "-P", str(SCRIPT)]
        out = subprocess.run(cmd, env=clean_env(**(env or {})),
                             capture_output=True, text=True)
        if out.returncode != 0:
            raise AssertionError(out.stderr)
        return json.loads(out.stdout)

    def test_exact_tag_is_the_tag(self):
        self.commit()
        self.git("tag", "v2.3.0")
        v = self.derive()
        self.assertEqual(v["version"], "2.3.0")
        self.assertEqual(v["tag"], "v2.3.0")
        self.assertEqual(v["build_number"], 1_203_009_000)
        self.assertEqual(v["filesafe"], "2.3.0")

    def test_prerelease_tag(self):
        self.commit()
        self.git("tag", "v2.4.0-rc.2")
        v = self.derive()
        self.assertEqual(v["version"], "2.4.0-rc.2")
        self.assertEqual(v["prerelease"], "rc.2")
        self.assertEqual(v["build_number"], 1_204_007_200)

    def test_main_after_release_bumps_minor(self):
        self.commit()
        self.git("tag", "v2.2.4")
        self.commit(3)
        sha = self.git("rev-parse", "HEAD")
        v = self.derive()
        self.assertEqual(v["version"], f"2.3.0-alpha.3+g{sha[:12]}")
        self.assertEqual(v["core"], "2.3.0")
        self.assertEqual(v["series"], "2.3")
        self.assertEqual(v["commit"], sha)
        self.assertNotIn("+", v["filesafe"])
        self.assertEqual(v["build_number"], 1_203_000_003)

    def test_release_branch_bumps_patch(self):
        self.commit()
        self.git("tag", "v2.2.4")
        self.git("checkout", "-q", "-b", "release/2.2")
        self.commit(2)
        self.assertTrue(self.derive()["version"].startswith("2.2.5-alpha.2+g"))

    def test_detached_ci_checkout_reads_the_runner_ref(self):
        self.commit()
        self.git("tag", "v2.2.4")
        self.commit()
        self.git("checkout", "-q", "--detach")
        v = self.derive(env={"GITHUB_REF": "refs/heads/release/2.2", "CI": "true"})
        self.assertTrue(v["version"].startswith("2.2.5-alpha.1+g"))
        v = self.derive(env={"GITHUB_REF": "refs/pull/7/merge", "CI": "true"})
        self.assertTrue(v["version"].startswith("2.3.0-alpha.1+g"))

    def test_after_prerelease_stays_below_the_next(self):
        self.commit()
        self.git("tag", "v2.4.0-rc.1")
        self.commit(5)
        v = self.derive()
        self.assertTrue(v["version"].startswith("2.4.0-rc.1.dev.5+g"))
        self.assertGreater(semver.parse(v["version"]), semver.parse("2.4.0-rc.1"))
        self.assertLess(semver.parse(v["version"]), semver.parse("2.4.0-rc.2"))
        self.assertEqual(v["build_number"], 1_204_007_105)

    def test_build_numbers_order_like_semver(self):
        order = ["2.3.0-alpha.400", "2.4.0-alpha.0", "2.4.0-alpha.1",
                 "2.4.0-beta.1", "2.4.0-beta.1.dev.3", "2.4.0-beta.2",
                 "2.4.0-rc.1", "2.4.0-rc.1.dev.7", "2.4.0", "2.4.1-alpha.1",
                 "2.4.1"]
        # The list is in SemVer order to begin with ...
        parsed = [semver.parse(v) for v in order]
        self.assertEqual(parsed, sorted(parsed))
        self.assertEqual(len(set(parsed)), len(parsed))
        # ... and the build numbers keep it.
        numbers = [self.derive(defines=[f"-DGPGFRONTEND_VERSION_OVERRIDE={v}"])
                   ["build_number"] for v in order]
        self.assertEqual(numbers, sorted(numbers))
        self.assertEqual(len(set(numbers)), len(numbers))
        self.assertGreater(numbers[0], 263_000_000)  # last timestamp scheme

    def test_nightly_tag_is_ignored(self):
        self.commit()
        self.git("tag", "v2.2.4")
        self.commit()
        self.git("tag", "nightly")
        self.assertTrue(self.derive()["version"].startswith("2.3.0-alpha.1+g"))

    def test_base_tag_counts_from_itself(self):
        self.commit()
        self.git("tag", "v2.4.0-alpha.0")
        v = self.derive()
        self.assertEqual(v["version"], "2.4.0-alpha.0")
        self.commit(4)
        self.assertTrue(self.derive()["version"].startswith("2.4.0-alpha.4+g"))

    def test_dev_after_beta_sorts_between_betas(self):
        self.commit()
        self.git("tag", "v2.4.0-beta.1")
        self.commit(3)
        v = semver.parse(self.derive()["version"])
        self.assertGreater(v, semver.parse("2.4.0-beta.1"))
        self.assertLess(v, semver.parse("2.4.0-beta.2"))

    def main_versions_across_a_cut(self, tag_final_on_branch):
        """Walks a whole release cycle and returns every version main had."""
        seen = []

        def on_main():
            self.git("checkout", "-q", "main")
            seen.append(semver.parse(self.derive()["version"]))

        self.commit()
        self.git("tag", "v2.2.4")
        self.commit(3)
        on_main()                                     # 2.3.0-alpha.3
        self.git("tag", "v2.3.0-beta.1")
        on_main()                                     # 2.3.0-beta.1
        self.commit(2)
        on_main()                                     # 2.3.0-beta.1.dev.2

        if tag_final_on_branch:
            # Cut first, stabilise on the branch, tag .0 there.
            self.git("checkout", "-q", "-b", "release/2.3")
            self.commit()
            self.git("tag", "v2.3.0")
        else:
            # The planned flow: tag .0 on main, cut the branch at it.
            self.git("tag", "v2.3.0")
            on_main()                                 # 2.3.0
            self.git("branch", "release/2.3")

        # scripts/release/cut.sh: an empty commit on main carries the base
        # tag, so the release branch can never reach it.
        self.git("checkout", "-q", "main")
        self.git("commit", "-q", "--allow-empty", "-m",
                 "chore(release): open 2.4 development")
        self.git("tag", "v2.4.0-alpha.0")
        on_main()                                     # 2.4.0-alpha.0
        self.commit(2)
        on_main()                                     # 2.4.0-alpha.2

        # A fix on the release branch, and its tag, must not move main.
        self.git("checkout", "-q", "release/2.3")
        self.commit()
        branch = semver.parse(self.derive()["version"])
        self.assertEqual(branch.series, "2.3")
        self.assertEqual(branch.prerelease[0], "alpha")
        self.assertGreater(branch, semver.parse("2.3.0"))
        self.git("tag", "v2.3.1")
        self.commit()
        self.assertTrue(
            self.derive()["version"].startswith("2.3.2-alpha.1+g"))

        self.git("checkout", "-q", "main")
        self.commit()
        on_main()                                     # 2.4.0-alpha.3
        return seen

    def assert_strictly_increasing(self, seen):
        for before, after in zip(seen, seen[1:]):
            self.assertLess(before, after, f"{before} !< {after}")
        for tag in self.git("tag").split():
            if semver.RELEASE_TAG.match(tag):
                self.assertGreater(seen[-1], semver.parse(tag), tag)

    def test_cut_with_final_tagged_on_main(self):
        self.assert_strictly_increasing(self.main_versions_across_a_cut(False))

    def test_cut_with_final_tagged_on_release_branch(self):
        seen = self.main_versions_across_a_cut(True)
        self.assert_strictly_increasing(seen)
        self.assertEqual(seen[-1].series, "2.4")

    def test_dirty_tree_is_build_metadata(self):
        self.commit()
        self.git("tag", "v2.3.0")
        (self.repo / "f").write_text("changed")
        v = self.derive()
        self.assertEqual(v["version"], "2.3.0+dirty")
        self.assertEqual(v["build_metadata"], "dirty")

    def test_shallow_ci_checkout_is_an_error(self):
        self.commit()
        out = subprocess.run(
            [CMAKE, f"-DGF_SOURCE_DIR={self.repo}", "-P", str(SCRIPT)],
            env=clean_env(CI="true"), capture_output=True, text=True)
        self.assertNotEqual(out.returncode, 0)
        self.assertIn("fetch-depth", out.stderr)

    def test_source_archive_reads_version_file(self):
        self.commit()
        (self.repo / "VERSION").write_text(
            "$Format:%(describe:tags=true,abbrev=12,match=v[0-9]*)$\n"
            "$Format:%H$\n")
        (self.repo / ".gitattributes").write_text("VERSION export-subst\n")
        self.git("add", "VERSION", ".gitattributes")
        self.git("commit", "-q", "-m", "version file")
        self.git("tag", "v2.2.4")
        self.commit(2)
        sha = self.git("rev-parse", "HEAD")

        export = self.tmp / "export"
        export.mkdir()
        archive = subprocess.run([GIT, "archive", "HEAD"], cwd=self.repo,
                                 env=clean_env(), check=True,
                                 capture_output=True).stdout
        subprocess.run(["tar", "x", "-C", str(export)], input=archive,
                       check=True)
        v = self.derive(src=export)
        self.assertEqual(v["source"], "archive")
        self.assertEqual(v["version"], f"2.3.0-alpha.2+g{sha[:12]}")
        self.assertEqual(v["commit"], sha)

    def test_unexpanded_version_file_falls_back(self):
        export = self.tmp / "plain"
        export.mkdir()
        (export / "VERSION").write_text("$Format:%(describe)$\n$Format:%H$\n")
        v = self.derive(src=export)
        self.assertEqual(v["version"], "0.0.0-unknown")
        self.assertEqual(v["source"], "unknown")

    def test_a_given_commit_of_a_bare_mirror(self):
        self.commit()
        self.git("tag", "v2.2.4")
        self.commit(2)
        old = self.git("rev-parse", "HEAD")
        self.commit(3)  # the mirror has moved on; the commit asked about has not
        bare = self.tmp / "mirror.git"
        subprocess.run([GIT, "clone", "-q", "--mirror", str(self.repo), str(bare)],
                       env=clean_env(), check=True)
        v = self.derive(src=bare, defines=[f"-DGPGFRONTEND_VERSION_REV={old}",
                                          "-DGPGFRONTEND_VERSION_REF=main"])
        self.assertEqual(v["version"], f"2.3.0-alpha.2+g{old[:12]}")
        self.assertEqual(v["commit"], old)
        # The work tree's dirt never reaches a version asked of a commit.
        (self.repo / "f").write_text("dirty")
        v = self.derive(defines=[f"-DGPGFRONTEND_VERSION_REV={old}"])
        self.assertNotIn("dirty", v["version"])

    def test_override_wins(self):
        self.commit()
        self.git("tag", "v2.2.4")
        v = self.derive(defines=["-DGPGFRONTEND_VERSION_OVERRIDE=2.2.4-1"])
        self.assertEqual(v["version"], "2.2.4-1")
        self.assertEqual(v["source"], "override")

    def test_repository_version_file_is_wired_for_export(self):
        lines = (ROOT / "VERSION").read_text().splitlines()
        self.assertTrue(lines[0].startswith("$Format:%(describe:"))
        self.assertEqual(lines[1], "$Format:%H$")
        self.assertIn("VERSION export-subst",
                      (ROOT / ".gitattributes").read_text())


if __name__ == "__main__":
    unittest.main()
