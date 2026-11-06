#!/usr/bin/env python3
# Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""The nightly chain end to end: stage (CI) -> promote -> index -> verify.

Everything real but the network and Sigstore: minisign with throwaway keys,
rclone over local directories standing in for the buckets, git for the
version, and the fake cosign of test_release_candidate, which binds a bundle
to exactly one identity, issuer, commit and manifest.

Run: python3 -m unittest discover -s scripts/tests
"""

import contextlib
import io
import json
import os
import shutil
import stat
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent.parent
GFR = ROOT / "scripts" / "release" / "gfrelease"
sys.path.insert(0, str(GFR))
sys.path.insert(0, str(ROOT / "scripts" / "tests"))

import common  # noqa: E402
import index as index_mod  # noqa: E402
import minisig  # noqa: E402
import promote as promote_mod  # noqa: E402
import stage as stage_mod  # noqa: E402
import verify as verify_mod  # noqa: E402
from store import Store  # noqa: E402
from test_release_candidate import FAKE_COSIGN  # noqa: E402

TOOLS = all(shutil.which(t) for t in ("minisign", "rclone", "git", "cmake"))
ISSUER = "https://token.actions.githubusercontent.com"


def git_env():
    env = {k: v for k, v in os.environ.items()
           if not k.startswith("GITHUB_") and k != "CI"}
    env.update({"GIT_AUTHOR_NAME": "t", "GIT_AUTHOR_EMAIL": "t@example.org",
                "GIT_COMMITTER_NAME": "t", "GIT_COMMITTER_EMAIL": "t@example.org",
                "GIT_CONFIG_GLOBAL": os.devnull, "GIT_CONFIG_NOSYSTEM": "1"})
    return env


def keypair(directory: Path, name: str) -> tuple[Path, str]:
    sec, pub = directory / f"{name}.key", directory / f"{name}.pub"
    subprocess.run(["minisign", "-G", "-W", "-s", sec, "-p", pub], check=True,
                   capture_output=True)
    return sec, minisig.public_key_of(pub)


@unittest.skipUnless(TOOLS, "needs minisign, rclone, git and cmake")
class ChainTest(unittest.TestCase):

    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp(prefix="gfr-test-"))
        self.product = common.default_product()
        self.slug = self.product.slug
        self.staging = Store(str(self.tmp / "staging"))
        self.builds = Store(str(self.tmp / "builds" / self.slug))
        self.update = Store(str(self.tmp / "update" / self.slug))
        self.state = self.tmp / "state"
        keys = self.tmp / "keys"
        keys.mkdir()
        self.art_key, self.art_pub = keypair(keys, "artifact")
        self.idx_key, self.idx_pub = keypair(keys, "index")
        self.cosign = self.tmp / "cosign"
        self.cosign.write_text(FAKE_COSIGN.format(python=sys.executable))
        self.cosign.chmod(self.cosign.stat().st_mode | stat.S_IEXEC)

        self.repo = self.tmp / "repo"
        self.repo.mkdir()
        self.git("init", "-q", "-b", "main")
        self.commit()
        self.git("tag", "v2.2.4")

    def tearDown(self):
        shutil.rmtree(self.tmp, ignore_errors=True)

    def git(self, *args):
        return subprocess.run(["git", *args], cwd=self.repo, env=git_env(),
                              check=True, capture_output=True,
                              text=True).stdout.strip()

    def commit(self, n=1):
        for _ in range(n):
            self.git("commit", "-q", "--allow-empty", "-m", "c")
        return self.git("rev-parse", "HEAD")

    # --- what CI does --------------------------------------------------

    def build(self, ref="refs/heads/main", claim=None, extra=None):
        """A signed candidate of HEAD, staged as CI would."""
        commit = self.git("rev-parse", "HEAD")
        version = claim or common.git_version(self.repo, commit, ref.split("/", 2)[2])
        filesafe = version.replace("+", "-")
        cand = self.tmp / f"cand-{filesafe}"
        cand.mkdir()
        names = [f"GpgFrontend-{filesafe}-linux-x86_64.AppImage",
                 f"GpgFrontend-{filesafe}-windows-x86_64.msi",
                 f"GpgFrontend-{filesafe}-macos-15.dmg"] + (extra or [])
        for name in names:
            (cand / name).write_bytes(f"{name}\n".encode() * 100)
        lines = "".join(f"{common.sha256_file(cand / n)}  {n}\n"
                        for n in sorted(names))
        (cand / common.SUMS).write_text(lines)
        subprocess.run([self.cosign, "sign-blob", "--bundle", cand / common.BUNDLE,
                        "--identity", self.product.identity(ref),
                        "--issuer", ISSUER, "--sha", commit, cand / common.SUMS],
                       check=True)
        return stage_mod.stage(self.product, cand, self.staging, ref=ref,
                               commit=commit, version=version, filesafe=filesafe,
                               built_at="2026-11-06T12:00:00Z"), version

    # --- what the signing host does ------------------------------------

    def promote(self):
        return promote_mod.promote(self.product, self.staging, self.builds,
                                   git_dir=self.repo, artifact_key=self.art_key,
                                   artifact_pub=self.art_pub, cosign=str(self.cosign))

    def index(self, force=False):
        return index_mod.publish(self.product, self.builds, self.update,
                                 index_key=self.idx_key, artifact_pub=self.art_pub,
                                 state_dir=self.state, force=force)

    def check(self, deep=True):
        return verify_mod.verify(self.product, self.update, self.builds,
                                 index_keys=[self.idx_pub],
                                 artifact_key=self.art_pub, deep=deep)

    # --- the chain -----------------------------------------------------

    def test_a_nightly_goes_from_ci_to_a_verified_index(self):
        self.commit(3)
        _, version = self.build()
        self.assertTrue(version.startswith("2.3.0-alpha.3+g"))

        (descriptor,) = self.promote()
        self.assertEqual(descriptor["version"], version)
        self.assertEqual({a["kind"] for a in descriptor["artifacts"]},
                         {"appimage", "msi", "dmg"})
        dmg = next(a for a in descriptor["artifacts"] if a["kind"] == "dmg")
        self.assertEqual((dmg["arch"], dmg["min_os"]), ("arm64", {"macos": "15.0"}))
        self.assertTrue(self.builds.exists(f"commits/{descriptor['commit']}"))

        index = self.index()
        self.assertEqual(index["serial"], 1)
        self.assertEqual(index["channels"], {"stable": None, "beta": None,
                                             "nightly": version})
        self.assertEqual(index["series"]["2.1"]["eol"], "2027-12-31")

        report = self.check()
        self.assertEqual(report["checked"],
                         [{"version": version, "kind": "nightly", "artifacts": 3}])

    def test_only_the_newest_build_per_branch_is_promoted(self):
        self.commit()
        self.build()
        self.commit()
        _, newest = self.build()
        promoted = self.promote()
        self.assertEqual([d["version"] for d in promoted], [newest])
        self.assertEqual(self.promote(), [])  # a rerun does nothing

    def test_a_newer_nightly_moves_the_channel_and_the_serial(self):
        self.commit()
        self.build()
        self.promote()
        first = self.index()
        self.assertIsNone(self.index())  # nothing new: nothing published
        self.commit()
        _, newer = self.build()
        self.promote()
        second = self.index()
        self.assertEqual(second["channels"]["nightly"], newer)
        self.assertEqual(second["serial"], first["serial"] + 1)

    def test_resign_pushes_expiry_with_a_new_serial(self):
        self.commit()
        self.build()
        self.promote()
        first = self.index()
        later = self.index(force=True)
        self.assertEqual(later["serial"], first["serial"] + 1)
        self.check(deep=False)

    def test_serial_survives_a_lost_state_dir(self):
        self.commit()
        self.build()
        self.promote()
        self.index()
        self.index(force=True)
        shutil.rmtree(self.state)
        self.assertEqual(self.index(force=True)["serial"], 3)

    def test_release_branch_nightlies_are_published_without_a_channel(self):
        self.git("checkout", "-q", "-b", "release/2.2", "v2.2.4")
        self.commit()
        _, version = self.build(ref="refs/heads/release/2.2")
        self.assertTrue(version.startswith("2.2.5-alpha.1"))
        self.promote()
        self.assertIsNone(self.index()["channels"]["nightly"])

    # --- refusals ------------------------------------------------------

    def assert_refused(self, fn, pattern):
        with self.assertRaisesRegex(common.ReleaseError, pattern):
            fn()

    def test_a_version_git_does_not_confirm_is_refused(self):
        self.commit()
        self.build(claim="9.9.9")
        self.assert_refused(self.promote, "git says")

    def test_a_tampered_staged_file_is_refused(self):
        self.commit()
        prefix, version = self.build()
        target = self.tmp / "staging" / prefix / \
            f"GpgFrontend-{version.replace('+', '-')}-windows-x86_64.msi"
        target.write_bytes(b"evil")
        self.assert_refused(self.promote, "sha256 does not match")

    def test_a_candidate_signed_for_another_ref_is_refused(self):
        self.commit()
        prefix, _ = self.build()
        staged = self.tmp / "staging" / prefix / common.STAGED
        data = json.loads(staged.read_text())
        data["ref"] = "refs/heads/release/2.3"
        staged.write_text(json.dumps(data))
        with contextlib.redirect_stderr(io.StringIO()):
            self.assert_refused(self.promote, "does not verify")

    def test_an_unknown_file_never_reaches_staging(self):
        self.commit()
        self.assert_refused(lambda: self.build(extra=["GpgFrontend-x-evil.exe"]),
                            "no artifact rule|not named")

    def test_a_promoted_object_is_never_replaced(self):
        self.commit()
        self.build()
        (descriptor,) = self.promote()
        msi = next(a for a in descriptor["artifacts"] if a["kind"] == "msi")
        self.assert_refused(lambda: self.builds.put_bytes(b"other", msi["path"]),
                            "exists with other bytes")
        # The same bytes again are no error: what makes a rerun safe.
        same = self.tmp / "same"
        same.write_bytes(self.builds.read(msi["path"]))
        self.builds.put_file(same, msi["path"])

    def test_index_refuses_a_descriptor_whose_manifest_was_swapped(self):
        self.commit()
        self.build()
        (d,) = self.promote()
        sums = self.tmp / "builds" / self.slug / d["sums"]
        sums.chmod(0o644)
        sums.write_text(sums.read_text().replace("0", "1", 1))
        self.assert_refused(self.index, "no pinned key verifies")

    def test_verify_rejects_a_tampered_index(self):
        self.commit()
        self.build()
        self.promote()
        self.index()
        path = self.tmp / "update" / self.slug / common.INDEX
        data = json.loads(path.read_text())
        data["channels"]["nightly"] = "9.9.9"
        path.write_bytes(common.canonical_json(data))
        self.assert_refused(lambda: self.check(deep=False), "no pinned key")

    def test_verify_rejects_another_products_index(self):
        self.commit()
        self.build()
        self.promote()
        self.index()
        other = common.default_product()
        other.slug = "demo"
        self.assert_refused(
            lambda: verify_mod.verify(other, self.update, self.builds,
                                      index_keys=[self.idx_pub],
                                      artifact_key=self.art_pub),
            "is for 'gpgfrontend'|not 'demo'")

    def test_verify_rejects_the_wrong_key(self):
        self.commit()
        self.build()
        self.promote()
        self.index()
        _, other = keypair(self.tmp / "keys", "other")
        self.assert_refused(
            lambda: verify_mod.verify(self.product, self.update, self.builds,
                                      index_keys=[other], artifact_key=self.art_pub),
            "no pinned key")

    def test_paths_cannot_leave_the_product_root(self):
        for bad in ("../demo/v1/index.json", "/etc/passwd", "a/../b",
                    "https://x/y", "a%2fb", "a//b", ""):
            with self.subTest(path=bad):
                self.assert_refused(lambda: common.check_relpath(bad),
                                    "not a product-relative path")


if __name__ == "__main__":
    unittest.main()
