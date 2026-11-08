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
import openpgp  # noqa: E402
import promote as promote_mod  # noqa: E402
import stage as stage_mod  # noqa: E402
import verify as verify_mod  # noqa: E402
from store import Store  # noqa: E402
from test_release_candidate import FAKE_COSIGN  # noqa: E402

def tools(*names: str) -> bool:
    """Whether every command is installed. With GF_REQUIRE_RELEASE_TOOLS=1
    (CI's setup job) a missing one fails the run instead of skipping the
    tests that need it: a chain test skipped in CI is a chain untested."""
    listed = {line.split()[0] for line in
              (ROOT / "scripts" / "release" / "tools.txt").read_text()
              .splitlines() if line.strip() and not line.startswith("#")}
    unlisted = set(names) - listed - {"gpgconf"}
    if unlisted:
        raise AssertionError(f"not in scripts/release/tools.txt: {unlisted}")
    missing = [n for n in names if not shutil.which(n)]
    if missing and os.environ.get("GF_REQUIRE_RELEASE_TOOLS") == "1":
        raise AssertionError(f"GF_REQUIRE_RELEASE_TOOLS=1, but {missing} "
                             "are not installed")
    return not missing


TOOLS = tools("minisign", "rclone", "git", "cmake")
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
                                 artifact_key=self.art_key,
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
        self.assertEqual(index["channels"], {"stable": None,
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

    def with_retired_artifact_keys(self, retired):
        """This product as release/product.json has it, with @p retired."""
        d = self.tmp / "rotated"
        d.mkdir(exist_ok=True)
        data = json.loads((common.ROOT / "release" / "product.json").read_text())
        data["signing"]["artifact_keys_retired"] = retired
        (d / "product.json").write_text(json.dumps(data))
        shutil.copy(common.ROOT / "release" / "policy.toml", d)
        return common.Product(d / "product.json", d / "policy.toml")

    def test_a_rotated_artifact_key_keeps_earlier_nightlies(self):
        self.build()
        self.promote()
        self.index()
        old = self.art_pub
        self.art_key, self.art_pub = keypair(self.tmp / "keys", "artifact-new")

        # The host signs with the new key. Unless the old one is retired, what
        # it signed is no longer admitted, and no index is published at all.
        self.assert_refused(lambda: self.index(force=True),
                            "not signed with the artifact key")

        self.product = self.with_retired_artifact_keys([old])
        self.commit()
        _, newer = self.build()
        self.promote()
        index = self.index(force=True)
        self.assertEqual(index["channels"]["nightly"], newer)
        listed = {json.loads(self.builds.read(p))["signing"]["key_id"]
                  for p in ("nightly/" + r for r in
                            self.builds.files("nightly", include="*/release.json"))}
        self.assertEqual(listed, {minisig.key_id(old),
                                  minisig.key_id(self.art_pub)})
        self.check()

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

    def test_a_store_that_cannot_be_read_is_an_error_not_an_absence(self):
        # A 403 or an unreachable endpoint must never read as "not there":
        # the immutability check would then upload blind.
        broken = Store("nosuchremote:bucket")
        self.assert_refused(lambda: broken.exists("a/b"), "looking up")
        self.assert_refused(lambda: broken.files("a"), "listing")
        self.assertFalse(self.builds.exists("nightly/none/release.json"))
        self.assertEqual(self.builds.files("nightly"), [])

    # --- robustness ----------------------------------------------------

    def test_a_refused_release_leaves_the_rest_published(self):
        self.commit()
        _, version = self.build()
        self.git("checkout", "-q", "-b", "release/2.2", "v2.2.4")
        self.commit()
        _, branch_version = self.build(ref="refs/heads/release/2.2")
        self.promote()
        # The release branch build's manifest no longer matches its
        # descriptor: that one release is refused, nothing else is.
        sums = (self.tmp / "builds" / self.slug / "nightly" /
                branch_version.replace("+", "-") / common.SUMS)
        sums.write_text(sums.read_text().replace("0", "1", 1))
        with self.assertRaises(index_mod.IndexIncomplete) as caught:
            self.index()
        self.assertIn(branch_version.replace("+", "-"),
                      "\n".join(caught.exception.refused))
        self.assertEqual(caught.exception.index["channels"]["nightly"], version)
        self.assertEqual(index_mod.published(self.update)["serial"], 1)
        self.check()

    def test_one_bad_candidate_does_not_block_the_others(self):
        self.commit()
        _, version = self.build()
        self.git("checkout", "-q", "-b", "release/2.2", "v2.2.4")
        self.commit()
        prefix, _ = self.build(ref="refs/heads/release/2.2")
        (self.tmp / "staging" / prefix / common.STAGED).write_text("{")
        with self.assertRaises(promote_mod.PromoteIncomplete) as caught:
            self.promote()
        self.assertEqual([d["version"] for d in caught.exception.done],
                         [version])
        self.assertIn("not JSON", str(caught.exception))

    def test_a_filesafe_ci_claims_is_never_trusted(self):
        self.commit()
        prefix, version = self.build()
        staged_path = self.tmp / "staging" / prefix / common.STAGED
        staged = json.loads(staged_path.read_text())
        staged["filesafe"] = "9.9.9"
        staged_path.write_text(json.dumps(staged))
        self.assert_refused(self.promote, "staged as 9.9.9")
        self.assertEqual(self.builds.files("nightly"), [])

    def test_a_promotion_cut_short_before_its_marker_completes(self):
        self.commit()
        self.build()
        (descriptor,) = self.promote()
        marker = (self.tmp / "builds" / self.slug / "commits" /
                  descriptor["commit"])
        marker.unlink()
        self.assertEqual(self.promote(), [])
        self.assertTrue(marker.exists())

    def test_a_promotion_cut_short_anywhere_finishes_the_same(self):
        # One clock, so promoted_at is the same in every run compared.
        os.environ["GFR_NOW"] = "2026-11-08T10:00:00Z"
        self.addCleanup(os.environ.pop, "GFR_NOW", None)
        self.commit()
        self.build()
        root = self.tmp / "builds" / self.slug
        self.promote()
        self.index()
        clean = {str(f.relative_to(root)): common.sha256_file(f)
                 for f in sorted(root.rglob("*")) if f.is_file()}

        class Crash(Exception):
            pass

        class CrashingStore(Store):
            def __init__(self, root, at):
                super().__init__(root)
                self.puts, self.at = 0, at

            def put_file(self, local, path, immutable=True):
                self.puts += 1
                if self.puts == self.at:
                    raise Crash(path)
                super().put_file(local, path, immutable)

        n = 1
        while True:
            shutil.rmtree(root)
            shutil.rmtree(self.state, ignore_errors=True)
            shutil.rmtree(self.tmp / "update", ignore_errors=True)
            try:
                promote_mod.promote(self.product, self.staging,
                                    CrashingStore(str(root), n),
                                    git_dir=self.repo, artifact_key=self.art_key,
                                    artifact_pub=self.art_pub,
                                    cosign=str(self.cosign))
                break
            except (Crash, promote_mod.PromoteIncomplete):
                pass
            with self.subTest(cut_at=n):
                self.promote()
                self.index()
                self.assertEqual(
                    {str(f.relative_to(root)): common.sha256_file(f)
                     for f in sorted(root.rglob("*")) if f.is_file()}, clean)
            n += 1
        self.assertGreater(n, 5)

    def test_a_nightly_about_to_expire_is_never_named(self):
        self.commit()
        _, version = self.build()
        self.promote()
        self.assertEqual(self.index()["channels"]["nightly"], version)
        # 23 days on (30-day retention, 7-day margin), no newer main build:
        # the bucket will delete it soon, so the index stops naming it.
        os.environ["GFR_NOW"] = "2099-01-01T00:00:00Z"
        self.addCleanup(os.environ.pop, "GFR_NOW", None)
        err = io.StringIO()
        with contextlib.redirect_stderr(err):
            index = self.index(force=True)
        self.assertIsNone(index["channels"]["nightly"])
        self.assertIsNone(index["nightly"])
        self.assertIn("no main nightly promoted since", err.getvalue())

    def test_a_rerun_workflow_stages_again(self):
        commit = self.commit()
        prefix, version = self.build()
        cand = self.tmp / f"cand-{version.replace('+', '-')}"
        # The re-run's Sigstore bundle has other bytes: still nothing to do.
        (cand / common.BUNDLE).write_text(
            (cand / common.BUNDLE).read_text() + "\n")
        again = stage_mod.stage(self.product, cand, self.staging,
                                ref="refs/heads/main", commit=commit,
                                version=version,
                                filesafe=version.replace("+", "-"),
                                built_at="2026-11-06T13:00:00Z")
        self.assertEqual(again, prefix)
        self.assert_refused(
            lambda: stage_mod.stage(self.product, cand, self.staging,
                                    ref="refs/heads/main", commit="f" * 40,
                                    version=version,
                                    filesafe=version.replace("+", "-"),
                                    built_at="x"),
            "already holds")

    def cli(self):
        import importlib.util
        spec = importlib.util.spec_from_file_location("gfr_cli",
                                                      GFR / "__main__.py")
        cli = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(cli)
        return cli

    def test_a_host_with_keys_clients_do_not_pin_refuses_to_run(self):
        err = io.StringIO()
        with contextlib.redirect_stderr(err):
            status = self.cli().main([
                "index", "--builds", str(self.tmp / "b"),
                "--update", str(self.tmp / "u"),
                "--state-dir", str(self.state),
                "--artifact-pub", self.art_pub,
                "--index-key", str(self.idx_key), "--index-pub", self.idx_pub])
        self.assertEqual(status, 1)
        self.assertIn("not signing.artifact_key", err.getvalue())
        self.assertFalse(self.state.exists())

    def test_a_monitor_warns_before_the_index_expires(self):
        self.commit()
        self.build()
        self.promote()
        self.index()
        args = ["verify", "--update", str(self.tmp / "update" / self.slug),
                "--builds", str(self.tmp / "builds" / self.slug),
                "--index-pub", self.idx_pub, "--artifact-pub", self.art_pub]
        out, err = io.StringIO(), io.StringIO()
        with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
            self.assertEqual(self.cli().main(args + ["--min-days-left", "3"]), 0)
            # 14 days of validity: a 20-day threshold is not met.
            self.assertEqual(self.cli().main(args + ["--min-days-left", "20"]), 1)
        self.assertIn("has not re-signed it", err.getvalue())

    def test_a_dry_run_writes_nothing(self):
        self.commit()
        _, version = self.build()
        self.promote()
        report = index_mod.dry_run(self.product, self.builds, self.update,
                                   artifact_pub=self.art_pub,
                                   state_dir=self.state)
        self.assertEqual(report["nightly"], version)
        self.assertEqual(report["refused"], [])
        self.assertIn("nightly", report["changes"])
        self.assertFalse(self.state.exists())
        self.assertFalse(self.update.exists(common.INDEX))

    def test_the_cli_wants_both_stores_or_neither(self):
        import importlib.util
        spec = importlib.util.spec_from_file_location("gfr_cli",
                                                      GFR / "__main__.py")
        cli = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(cli)
        err = io.StringIO()
        with contextlib.redirect_stderr(err):
            self.assertEqual(cli.main(["verify", "--update", str(self.tmp)]), 1)
        self.assertIn("both --update and --builds", err.getvalue())

    def test_paths_cannot_leave_the_product_root(self):
        for bad in ("../demo/v1/index.json", "/etc/passwd", "a/../b",
                    "https://x/y", "a%2fb", "a//b", ""):
            with self.subTest(path=bad):
                self.assert_refused(lambda: common.check_relpath(bad),
                                    "not a product-relative path")


if __name__ == "__main__":
    unittest.main()


# --------------------------------------------------------------- stable

import authenticode  # noqa: E402
import stable as stable_mod  # noqa: E402


class HelperTest(unittest.TestCase):
    """The shared pieces every role leans on, without any tool."""

    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp(prefix="gfr-helper-"))

    def tearDown(self):
        shutil.rmtree(self.tmp, ignore_errors=True)

    def test_a_clock_without_a_zone_is_utc(self):
        os.environ["GFR_NOW"] = "2026-11-08T10:00:00"
        try:
            self.assertEqual(common.iso(common.utcnow()),
                             "2026-11-08T10:00:00Z")
        finally:
            del os.environ["GFR_NOW"]

    def test_filesafe_folds_build_metadata(self):
        self.assertEqual(common.parse_version("2.3.0-alpha.4+gabc").filesafe,
                         "2.3.0-alpha.4-gabc")
        self.assertEqual(common.parse_version("2.2.4").filesafe, "2.2.4")

    def test_bad_input_is_a_release_error_not_a_traceback(self):
        for data in (b"{", b"[]", b"\xff"):
            with self.assertRaises(common.ReleaseError):
                common.load_json(data, "x")
        for text in ("2.2", None, "v2.2.4.1"):
            with self.assertRaises(common.ReleaseError):
                common.parse_version(text)
        with self.assertRaisesRegex(common.ReleaseError, "malformed line"):
            common.parse_manifest(b"abc  name\n", "SHA256SUMS.txt")

    def test_manifest_round_trips(self):
        hashes = {"b": "1" * 64, "a": "2" * 64}
        data = common.render_manifest(hashes)
        self.assertEqual(data, f"{'2' * 64}  a\n{'1' * 64}  b\n".encode())
        self.assertEqual(common.parse_manifest(data, "m"), hashes)

    def test_atomic_write_leaves_no_partial_file(self):
        target = self.tmp / "state" / "serial"
        common.atomic_write(target, "1\n")
        common.atomic_write(target, b"2\n")
        self.assertEqual(target.read_text(), "2\n")
        self.assertEqual(sorted(p.name for p in target.parent.iterdir()),
                         ["serial"])

    @unittest.skipUnless(shutil.which("git"), "needs git")
    def test_a_branch_never_passes_for_a_tag(self):
        repo = self.tmp / "repo"
        repo.mkdir()
        run = lambda *a: subprocess.run(["git", *a], cwd=repo, env=git_env(),
                                        check=True, capture_output=True)
        run("init", "-q", "-b", "main")
        run("commit", "-q", "--allow-empty", "-m", "c")
        run("branch", "v9.9.9")  # a branch named like a tag
        with self.assertRaisesRegex(common.ReleaseError, "has no tag v9.9.9"):
            common.tagged_commit(repo, "v9.9.9")
        run("tag", "v9.9.8")
        head = subprocess.run(["git", "rev-parse", "HEAD"], cwd=repo,
                              capture_output=True, text=True).stdout.strip()
        self.assertEqual(common.tagged_commit(repo, "v9.9.8", expect=head), head)
        with self.assertRaisesRegex(common.ReleaseError, "not 0+"):
            common.tagged_commit(repo, "v9.9.8", expect="0" * 40)


class OpenPgpStatusTest(unittest.TestCase):
    """What the gate accepts from gpg's status lines, as gpg 2.4 writes
    them for a good, an expired, a revoked and an unpinned signature."""

    PINNED = "C2560D443EE7DEA358920754852C77E84E7E7DBC"
    OTHER = "255A96F33D8EC3547D09B9E9FBE7B7AA7F0E8517"

    def ring(self, expiry=None):
        ring = openpgp.Keyring.__new__(openpgp.Keyring)
        ring.pinned = {self.PINNED}
        ring._key_expiry = lambda fpr: expiry
        return ring

    def status(self, judgement, fpr, created=1794134474):
        return (f"[GNUPG:] NEWSIG x\n[GNUPG:] {judgement} 852C77E84E7E7DBC u\n"
                f"[GNUPG:] VALIDSIG {fpr} 2026-11-08 {created} 0 4 0 22 10 00 "
                f"{fpr}\n")

    def test_good(self):
        self.assertEqual(self.ring()._accept(self.status("GOODSIG", self.PINNED),
                                             "f"), self.PINNED)

    def test_revoked_is_refused(self):
        with self.assertRaisesRegex(common.ReleaseError, "revoked"):
            self.ring()._accept(self.status("REVKEYSIG", self.PINNED), "f")

    def test_expired_counts_only_for_what_it_signed_in_time(self):
        before = self.status("EXPKEYSIG", self.PINNED, created=100)
        self.assertEqual(self.ring(expiry=200)._accept(before, "f"),
                         self.PINNED)
        after = self.status("EXPKEYSIG", self.PINNED, created=300)
        with self.assertRaisesRegex(common.ReleaseError, "after its key"):
            self.ring(expiry=200)._accept(after, "f")

    def test_an_expired_signature_is_refused(self):
        with self.assertRaisesRegex(common.ReleaseError, "EXPSIG"):
            self.ring()._accept(self.status("EXPSIG", self.PINNED), "f")

    def test_every_signature_is_looked_at(self):
        both = self.status("GOODSIG", self.OTHER) + \
            self.status("GOODSIG", self.PINNED)
        self.assertEqual(self.ring()._accept(both, "f"), self.PINNED)

    def test_none_valid(self):
        with self.assertRaisesRegex(common.ReleaseError, "not a pinned"):
            self.ring()._accept(self.status("GOODSIG", self.OTHER), "f")
        with self.assertRaisesRegex(common.ReleaseError, "no valid signature"):
            self.ring()._accept("[GNUPG:] NEWSIG\n[GNUPG:] BADSIG x u\n", "f")


class AuthenticodeSubjectTest(unittest.TestCase):
    """osslsigncode versions print the signer differently; the pin matches
    either form, and nothing else."""

    PIN = "/CN=GpgFrontend Test Signing/O=Rehearsal/C=DE"

    def rdns(self, s):
        return authenticode._subject_rdns(s)

    def test_both_forms_of_one_subject_match(self):
        want = self.rdns(self.PIN)
        self.assertEqual(want, [("CN", "GpgFrontend Test Signing"),
                                ("O", "Rehearsal"), ("C", "DE")])
        self.assertEqual(self.rdns(
            "C=DE,O=Rehearsal,CN=GpgFrontend Test Signing"), want)
        self.assertEqual(self.rdns(
            "C=DE, O=Rehearsal, CN=GpgFrontend Test Signing"), want)

    def test_another_subject_or_order_does_not(self):
        want = self.rdns(self.PIN)
        self.assertNotEqual(self.rdns(
            "C=DE,O=Rehearsal,CN=Someone Else"), want)
        self.assertNotEqual(self.rdns(
            "CN=GpgFrontend Test Signing,O=Rehearsal,C=DE"), want)
        self.assertNotEqual(self.rdns(
            "/CN=GpgFrontend Test Signing/O=Rehearsal"), want)

    def test_escaped_separators_stay_in_the_value(self):
        self.assertEqual(self.rdns(r"CN=A\, B,O=X"), [("O", "X"),
                                                     ("CN", "A, B")])

    def test_both_byte_escapes_and_plain_utf8_are_one_value(self):
        # What osslsigncode prints, what openssl -nameopt RFC2253 and
        # Windows show, and the same subject typed plainly.
        printed = (r"/C=DE/ST=Baden-W\xC3\xBCrttemberg/L=Karlsruhe"
                   "/O=Open Source Developer/CN=Open Source Developer Yu Hu")
        rfc2253 = (r"CN=Open Source Developer Yu Hu,O=Open Source Developer,"
                   r"L=Karlsruhe,ST=Baden-W\C3\BCrttemberg,C=DE")
        plain = ("CN=Open Source Developer Yu Hu,O=Open Source Developer,"
                 "L=Karlsruhe,ST=Baden-Württemberg,C=DE")
        want = self.rdns(plain)
        self.assertIn(("ST", "Baden-Württemberg"), want)
        self.assertEqual(self.rdns(printed), want)
        self.assertEqual(self.rdns(rfc2253), want)
        self.assertNotEqual(self.rdns(rfc2253.replace("BC", "B6")), want)

    def test_an_unreadable_subject_is_refused(self):
        for bad in ("", "/", "nonsense", "/CN=a/=b", r"/CN=a\xFF"):
            with self.assertRaises(common.ReleaseError):
                self.rdns(bad)

STABLE_TOOLS = TOOLS and tools("gpg", "gpgconf", "osslsigncode", "wixl",
                              "openssl")
WXS = """<?xml version="1.0" encoding="utf-8"?>
<Wix xmlns="http://schemas.microsoft.com/wix/2006/wi">
  <Product Id="*" Name="Test" Language="1033" Version="2.3.0" Manufacturer="T"
           UpgradeCode="12345678-1234-1234-1234-123456789012">
    <Package InstallerVersion="200" Compressed="yes"/>
    <Media Id="1" Cabinet="t.cab" EmbedCab="yes"/>
    <Directory Id="TARGETDIR" Name="SourceDir">
      <Directory Id="ProgramFilesFolder"><Directory Id="D" Name="T">
        <Component Id="C" Guid="12345678-1234-1234-1234-123456789013">
          <File Id="F" Source="payload.txt"/></Component>
      </Directory></Directory></Directory>
    <Feature Id="M" Level="1"><ComponentRef Id="C"/></Feature>
  </Product>
</Wix>
"""
SUBJECT = "/CN=GpgFrontend Test Signing/O=Rehearsal/C=DE"


@unittest.skipUnless(STABLE_TOOLS, "needs gpg, osslsigncode, wixl and openssl")
class StableTest(unittest.TestCase):
    """The offline stage and the host's admission of what it made."""

    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp(prefix="gfr-stable-"))
        self.gnupg = self.tmp / "gnupg"
        self.gnupg.mkdir(mode=0o700)
        self.fpr = self.new_key("Test Maintainer <maintainer@example.org>")
        self.product = self.write_product(self.tmp / "product", [self.fpr])

        self.staging = Store(str(self.tmp / "staging"))
        self.builds = Store(str(self.tmp / "builds" / self.product.slug))
        self.update = Store(str(self.tmp / "update" / self.product.slug))
        self.state = self.tmp / "state"
        keys = self.tmp / "keys"
        keys.mkdir()
        self.art_key, self.art_pub = keypair(keys, "artifact")
        self.idx_key, self.idx_pub = keypair(keys, "index")
        self.cosign = self.tmp / "cosign"
        self.cosign.write_text(FAKE_COSIGN.format(python=sys.executable))
        self.cosign.chmod(self.cosign.stat().st_mode | stat.S_IEXEC)

        self.cert, self.cert_key = self.tmp / "cert.pem", self.tmp / "key.pem"
        subprocess.run(["openssl", "req", "-x509", "-newkey", "rsa:2048",
                        "-nodes", "-days", "2", "-subj",
                        "/CN=GpgFrontend Test Signing/O=Rehearsal/C=DE",
                        "-keyout", self.cert_key, "-out", self.cert],
                       check=True, capture_output=True)

        self.repo = self.tmp / "repo"
        self.repo.mkdir()
        self.git("init", "-q", "-b", "main")
        self.git("commit", "-q", "--allow-empty", "-m", "start")
        self.git("tag", "v2.2.4")
        (self.repo / "release" / "notes").mkdir(parents=True)
        (self.repo / "release" / "notes" / "2.3.0.md").write_text(
            "## 2.3.0\n\n- Everything, signed.\n")
        self.git("add", "release")
        self.git("commit", "-q", "-m", "release notes")

    def tearDown(self):
        subprocess.run(["gpgconf", "--homedir", self.gnupg, "--kill", "all"],
                       capture_output=True)
        shutil.rmtree(self.tmp, ignore_errors=True)

    # --- fixtures ------------------------------------------------------

    def gpg(self, *args):
        return subprocess.run(["gpg", "--homedir", self.gnupg, "--batch",
                               *args], check=True, capture_output=True,
                              text=True).stdout

    def new_key(self, uid):
        self.gpg("--passphrase", "", "--quick-gen-key", uid, "ed25519",
                 "sign", "never")
        listing = self.gpg("--with-colons", "--list-keys", uid)
        return [l.split(":")[9] for l in listing.splitlines()
                if l.startswith("fpr:")][0]

    def write_product(self, directory, fingerprints, retired=(),
                      exported=None):
        directory.mkdir(parents=True, exist_ok=True)
        data = json.loads((common.ROOT / "release" / "product.json").read_text())
        data["signing"]["maintainer_fingerprints"] = fingerprints
        data["signing"]["maintainer_fingerprints_retired"] = list(retired)
        data["signing"]["authenticode_subject"] = SUBJECT
        (directory / "product.json").write_text(json.dumps(data))
        shutil.copy(common.ROOT / "release" / "policy.toml", directory)
        (directory / "maintainers.asc").write_text(self.gpg(
            "--armor", "--export",
            *(exported if exported is not None
              else [*fingerprints, *retired])))
        return common.Product(directory / "product.json",
                              directory / "policy.toml")

    def git(self, *args):
        env = git_env()
        env["GNUPGHOME"] = str(self.gnupg)
        return subprocess.run(["git", "-c", f"user.signingkey={self.fpr}",
                               *args], cwd=self.repo, env=env, check=True,
                              capture_output=True, text=True).stdout.strip()

    def tag(self, version="2.3.0", signed=True):
        self.git("tag", "-s" if signed else "-a", f"v{version}", "-m",
                 f"GpgFrontend {version}")

    def build(self, version="2.3.0"):
        """CI's tag build of v<version>, staged."""
        commit = self.git("rev-parse", "HEAD")
        cand = self.tmp / f"cand-{version}"
        cand.mkdir()
        (self.tmp / "payload.txt").write_text("payload\n")
        (self.tmp / "t.wxs").write_text(WXS)
        subprocess.run(["wixl", "-o", cand / f"GpgFrontend-{version}-windows-x86_64.msi",
                        self.tmp / "t.wxs"], cwd=self.tmp, check=True,
                       capture_output=True)
        for leg in ("linux-x86_64.AppImage", "macos-15.dmg"):
            (cand / f"GpgFrontend-{version}-{leg}").write_bytes(
                leg.encode() * 200)
        names = sorted(p.name for p in cand.iterdir())
        (cand / common.SUMS).write_text("".join(
            f"{common.sha256_file(cand / n)}  {n}\n" for n in names))
        ref = f"refs/tags/v{version}"
        subprocess.run([self.cosign, "sign-blob", "--bundle",
                        cand / common.BUNDLE, "--identity",
                        self.product.identity(ref), "--issuer", ISSUER,
                        "--sha", commit, cand / common.SUMS], check=True)
        stage_mod.stage(self.product, cand, self.staging, ref=ref,
                        commit=commit, version=version, filesafe=version,
                        built_at="2026-11-07T08:00:00Z")

    def stage(self, version="2.3.0", **kw):
        args = dict(staging=self.staging, builds=self.builds,
                    git_dir=self.repo, cosign=str(self.cosign),
                    gnupghome=str(self.gnupg),
                    signer=authenticode.Signer(str(self.cert),
                                               str(self.cert_key)),
                    authenticode_ca=str(self.cert))
        args.update(kw)
        return stable_mod.Stage(self.product, version, self.tmp / "work",
                                **args)

    def index(self):
        return index_mod.publish(
            self.product, self.builds, self.update, index_key=self.idx_key,
            artifact_pub=self.art_pub, artifact_key=self.art_key,
            state_dir=self.state, force=True,
            git_dir=self.repo, cosign=str(self.cosign),
            authenticode_ca=str(self.cert))

    def check(self, deep=True):
        return verify_mod.verify(self.product, self.update, self.builds,
                                 index_keys=[self.idx_pub],
                                 artifact_key=self.art_pub, deep=deep)

    def assert_refused(self, fn, pattern):
        with self.assertRaisesRegex(common.ReleaseError, pattern):
            fn()

    # --- the stable path -----------------------------------------------

    def test_a_replaced_certificate_keeps_earlier_releases(self):
        self.tag()
        self.build()
        self.stage().run()
        self.index()

        def pin(subject, retired):
            path = self.tmp / "product" / "product.json"
            data = json.loads(path.read_text())
            data["signing"]["authenticode_subject"] = subject
            data["signing"]["authenticode_subjects_retired"] = retired
            path.write_text(json.dumps(data))
            self.product = common.Product(path, path.with_name("policy.toml"))

        # A new certificate under another name, and a host that checks every
        # release again (a lost state, a new machine).
        shutil.rmtree(self.state / "admitted")
        pin("/CN=GpgFrontend New Signing/O=Rehearsal/C=DE", [])
        self.assert_refused(self.index, "signed by")
        pin("/CN=GpgFrontend New Signing/O=Rehearsal/C=DE", [SUBJECT])
        self.assertEqual(self.index()["releases"]["2.3.0"]["descriptor"],
                         "releases/2.3.0/release.json")

    def test_a_stable_release_goes_from_tag_to_a_verified_index(self):
        self.tag()
        self.build()
        d = self.stage().run()

        msi = next(a for a in d["artifacts"] if a["kind"] == "msi")
        self.assertEqual(msi["transform"], "authenticode")
        self.assertNotEqual(msi["sha256"], msi["build_sha256"])
        for a in d["artifacts"]:
            if a["kind"] != "msi":
                self.assertEqual(a["sha256"], a["build_sha256"])
            self.assertTrue(a["sig"].endswith(".sig"))
        self.assertEqual(d["signing"], {"scheme": "openpgp",
                                        "fingerprint": self.fpr})

        index = self.index()
        self.assertEqual(index["channels"]["stable"], "2.3.0")
        self.assertEqual(index["series"]["2.3"]["latest"], "2.3.0")
        entry = index["releases"]["2.3.0"]
        self.assertTrue(entry["notes"].startswith("notes/2.3.0/"))
        report = self.check()
        self.assertIn({"version": "2.3.0", "kind": "stable", "artifacts": 3},
                      report["checked"])

    def test_a_resumed_upload_sends_the_same_descriptor(self):
        self.tag()
        self.build()
        descriptor = self.stage().run()
        state_path = self.tmp / "work" / "2.3.0" / "state.json"
        # As if the run stopped after the descriptor went up but before the
        # step was recorded: the next run uploads again, byte for byte.
        state = json.loads(state_path.read_text())
        state["done"].remove("upload")
        state_path.write_text(json.dumps(state))
        os.environ["GFR_NOW"] = "2030-01-01T00:00:00Z"
        try:
            again = self.stage().run()
        finally:
            del os.environ["GFR_NOW"]
        self.assertEqual(again["released_at"], descriptor["released_at"])

    def test_signing_again_after_the_upload_is_refused(self):
        self.tag()
        self.build()
        self.stage().run()
        for step in ("pgp", "authenticode", "fetch"):
            self.assert_refused(lambda: self.stage().run(redo=step),
                                "upload has begun")
        self.assertIsNotNone(self.stage().run(redo="upload"))

    def test_another_versions_state_is_refused(self):
        self.tag()
        self.build()
        self.stage().run()
        work = self.tmp / "work"
        shutil.copytree(work / "2.3.0", work / "2.3.1")
        self.assert_refused(lambda: self.stage("2.3.1"), "not 2.3.1")

    # --- the OpenPGP gate and the countersignature ----------------------

    def tree(self):
        """Every object of the artifact store, by path, with its hash."""
        root = self.tmp / "builds" / self.product.slug
        return {str(f.relative_to(root)): common.sha256_file(f)
                for f in sorted(root.rglob("*")) if f.is_file()}

    def minisigned(self, path):
        root = self.tmp / "builds" / self.product.slug
        minisig.verify([self.art_pub], root / path, root / (path + ".minisig"))

    def assert_not_countersigned(self, version="2.3.0"):
        self.assertEqual([p for p in self.tree() if p.endswith(".minisig")],
                         [])
        self.assertFalse(any(p.startswith("commits/") for p in self.tree()))

    def test_a_passing_release_is_countersigned(self):
        self.tag()
        self.build()
        descriptor = self.stage().run()
        entry = self.index()["releases"]["2.3.0"]
        for a in descriptor["artifacts"]:
            self.minisigned(a["path"])
        self.minisigned(descriptor["sums"])
        self.minisigned(entry["notes"])
        self.assertEqual(entry["sums_minisig"], descriptor["sums"] + ".minisig")
        self.assertEqual(entry["notes_minisig"], entry["notes"] + ".minisig")
        self.assertTrue(self.builds.exists(f"commits/{descriptor['commit']}"))
        self.check(deep=True)

    def test_any_missing_or_bad_signature_means_no_minisign_no_listing(self):
        self.tag()
        self.build()
        descriptor = self.stage().run()
        root = self.tmp / "builds" / self.product.slug
        msi = next(a for a in descriptor["artifacts"] if a["kind"] == "msi")
        cases = {
            "a file's .sig missing": lambda: (root / msi["sig"]).unlink(),
            "a file's .sig over other bytes": lambda: (root / msi["sig"])
            .write_bytes((root / descriptor["sums_sig"]).read_bytes()),
            "the manifest's .sig missing":
                lambda: (root / descriptor["sums_sig"]).unlink(),
            "the notes' .sig missing": lambda: [
                p.unlink() for p in (root / "notes").rglob("*.sig")],
        }
        pristine = self.tmp / "pristine"
        shutil.copytree(root, pristine)
        for name, damage in cases.items():
            with self.subTest(name):
                shutil.rmtree(root)
                shutil.copytree(pristine, root)
                shutil.rmtree(self.state, ignore_errors=True)
                damage()
                with self.assertRaises(index_mod.IndexIncomplete) as caught:
                    self.index()
                self.assertNotIn("2.3.0", caught.exception.index["releases"])
                self.assertIsNone(
                    caught.exception.index["channels"]["stable"])
                self.assert_not_countersigned()

    def test_countersigning_again_changes_nothing(self):
        self.tag()
        self.build()
        self.stage().run()
        self.index()
        before = self.tree()
        shutil.rmtree(self.state)  # admitted again from nothing
        self.index()
        self.assertEqual(self.tree(), before)

    def test_a_rotated_artifact_key_keeps_the_countersignatures(self):
        self.tag()
        self.build()
        self.stage().run()
        self.index()
        before = self.tree()
        old = self.art_pub
        self.art_key, self.art_pub = keypair(self.tmp / "keys", "artifact-new")
        data = json.loads((self.tmp / "product" / "product.json").read_text())
        data["signing"]["artifact_keys_retired"] = [old]
        (self.tmp / "product" / "product.json").write_text(json.dumps(data))
        self.product = common.Product(self.tmp / "product" / "product.json",
                                      self.tmp / "product" / "policy.toml")
        self.assertIn("2.3.0", self.index()["releases"])
        self.assertEqual(self.tree(), before)  # nothing signed again

    def test_a_foreign_countersignature_is_never_replaced(self):
        self.tag()
        self.build()
        descriptor = self.stage().run()
        root = self.tmp / "builds" / self.product.slug
        (root / (descriptor["sums"] + ".minisig")).write_text("forged\n")
        with self.assertRaises(index_mod.IndexIncomplete) as caught:
            self.index()
        self.assertIn("never replaced", "\n".join(caught.exception.refused))
        self.assertEqual((root / (descriptor["sums"] + ".minisig"))
                         .read_text(), "forged\n")
        self.assertNotIn("2.3.0", caught.exception.index["releases"])

    def test_a_run_cut_short_anywhere_finishes_the_same(self):
        self.tag()
        self.build()
        self.stage().run()
        root = self.tmp / "builds" / self.product.slug
        pristine = self.tmp / "pristine"
        shutil.copytree(root, pristine)
        self.index()
        clean = self.tree()

        class Crash(Exception):
            pass

        class CrashingStore(Store):
            def __init__(self, root, at):
                super().__init__(root)
                self.puts, self.at = 0, at

            def put_file(self, local, path, immutable=True):
                self.puts += 1
                if self.puts == self.at:
                    raise Crash(path)
                super().put_file(local, path, immutable)

        n = 1
        while True:
            shutil.rmtree(root)
            shutil.copytree(pristine, root)
            shutil.rmtree(self.state, ignore_errors=True)
            shutil.rmtree(self.tmp / "update", ignore_errors=True)
            crashing = CrashingStore(str(root), n)
            try:
                index_mod.publish(
                    self.product, crashing, self.update,
                    index_key=self.idx_key, artifact_pub=self.art_pub,
                    artifact_key=self.art_key, state_dir=self.state,
                    force=True, git_dir=self.repo, cosign=str(self.cosign),
                    authenticode_ca=str(self.cert))
                break  # the run got past its last put: every cut was tried
            except Crash:
                pass
            with self.subTest(cut_at=n):
                # Nothing half-done is listed, and a plain rerun finishes
                # with exactly the bytes of a run never cut short.
                published = index_mod.published(self.update)
                self.assertTrue(published is None or
                                "2.3.0" not in published["releases"])
                self.assertIn("2.3.0", self.index()["releases"])
                self.assertEqual(self.tree(), clean)
            n += 1
        self.assertGreater(n, 5)

    def test_a_client_over_https_sees_the_countersigned_release(self):
        import functools
        import http.server
        import ssl
        import threading
        self.tag()
        self.build()
        self.stage().run()
        self.index()
        ca, key = self.tmp / "ca.pem", self.tmp / "ca.key"
        subprocess.run(["openssl", "req", "-x509", "-newkey", "ec",
                        "-pkeyopt", "ec_paramgen_curve:P-256", "-nodes",
                        "-days", "1", "-subj", "/CN=localhost",
                        "-addext", "subjectAltName=DNS:localhost",
                        "-keyout", key, "-out", ca], check=True,
                       capture_output=True)
        handler = functools.partial(http.server.SimpleHTTPRequestHandler,
                                    directory=str(self.tmp))
        handler.log_message = lambda *a: None
        server = http.server.ThreadingHTTPServer(("127.0.0.1", 0), handler)
        context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        context.load_cert_chain(ca, key)
        server.socket = context.wrap_socket(server.socket, server_side=True)
        threading.Thread(target=server.serve_forever, daemon=True).start()
        self.addCleanup(server.server_close)
        self.addCleanup(server.shutdown)
        base = f"https://localhost:{server.server_address[1]}"
        update = verify_mod.HttpSource([f"{base}/update/"],
                                       self.product.slug, str(ca))
        builds = verify_mod.HttpSource([f"{base}/builds/"],
                                       self.product.slug, str(ca))
        report = verify_mod.verify(self.product, update, builds,
                                   index_keys=[self.idx_pub],
                                   artifact_key=self.art_pub, deep=True)
        self.assertEqual([c["version"] for c in report["checked"]], ["2.3.0"])
        self.assertEqual(report["channels"]["stable"], "2.3.0")

    # --- trust inputs --------------------------------------------------

    def admitted(self):
        self.tag()
        self.build()
        self.stage().run()
        self.assertIn("2.3.0", self.index()["releases"])

    def test_a_key_dropped_from_the_key_file_unlists_its_releases(self):
        self.admitted()
        other = self.new_key("Someone Else <else@example.org>")
        # Still pinned by fingerprint, but no longer in maintainers.asc: the
        # record no longer vouches, and the check from nothing fails.
        self.product = self.write_product(self.tmp / "p2", [self.fpr],
                                          exported=[other])
        with self.assertRaises(index_mod.IndexIncomplete) as caught:
            self.index()
        self.assertNotIn("2.3.0", caught.exception.index["releases"])

    def test_an_unpinned_key_unlists_its_releases(self):
        self.admitted()
        other = self.new_key("Someone Else <else@example.org>")
        self.product = self.write_product(self.tmp / "p2", [other])
        self.assert_refused(self.index, "not signed with a pinned maintainer")

    def test_a_retired_key_keeps_its_releases(self):
        self.admitted()
        other = self.new_key("Next Maintainer <next@example.org>")
        self.product = self.write_product(self.tmp / "p2", [other],
                                          retired=[self.fpr])
        self.assertIn("2.3.0", self.index()["releases"])
        # Never to sign anything new, though.
        self.assert_refused(lambda: self.stage(key=self.fpr),
                            "not a current pinned")

    def test_a_tag_moved_after_admission_is_caught(self):
        self.admitted()
        self.git("commit", "-q", "--allow-empty", "-m", "later")
        self.git("tag", "-f", "-s", "v2.3.0", "-m", "moved")
        self.assert_refused(self.index, r"v2\.3\.0 is [0-9a-f]+, not")

    def test_a_revoked_key_unlists_its_releases(self):
        self.admitted()
        cert = self.gnupg / "openpgp-revocs.d" / f"{self.fpr}.rev"
        revocation = self.tmp / "revoke.asc"
        revocation.write_text(cert.read_text().replace(":-----BEGIN",
                                                       "-----BEGIN"))
        self.gpg("--import", str(revocation))
        # The key file now carries the revocation: everything it signed is
        # checked again, and gpg's REVKEYSIG is refused.
        self.product = self.write_product(self.tmp / "p2", [self.fpr])
        self.assert_refused(self.index, "revoked")

    def test_only_a_pinned_primary_signs(self):
        self.assert_refused(lambda: self.stage(key="0" * 40),
                            "not a current pinned")

    def test_notes_that_do_not_verify_refuse_the_release(self):
        self.admitted()
        (self.repo / "release" / "notes" / "2.3.0.md").write_text("Edited.\n")
        self.git("commit", "-qam", "notes: edited")
        other = self.new_key("Someone Else <else@example.org>")
        digest = common.sha256_bytes(b"Edited.\n")
        local = self.tmp / "notes.md"
        local.write_bytes(b"Edited.\n")
        openpgp.sign(local, other, str(self.gnupg))
        self.builds.put_file(local, f"notes/2.3.0/{digest}.md")
        self.builds.put_file(self.tmp / "notes.md.sig",
                             f"notes/2.3.0/{digest}.md.sig")
        self.assert_refused(self.index, "does not verify|not a pinned")

    def test_an_interrupted_stage_resumes(self):
        self.tag()
        self.build()
        empty = self.tmp / "no-keys"
        empty.mkdir(mode=0o700)
        broken = self.stage(gnupghome=str(empty))  # no secret key: pgp fails
        self.assert_refused(broken.run, "failed")
        done = json.loads((self.tmp / "work" / "2.3.0" / "state.json")
                          .read_text())["done"]
        self.assertEqual(done, ["fetch", "verify", "authenticode"])
        self.assertIsNotNone(self.stage().run())  # picks up at pgp

    def test_a_typo_fix_in_main_reaches_the_index(self):
        self.tag()
        self.build()
        self.stage().run()
        first = self.index()["releases"]["2.3.0"]["notes_sha256"]

        (self.repo / "release" / "notes" / "2.3.0.md").write_text(
            "## 2.3.0\n\n- Everything, signed. Fixed.\n")
        self.git("commit", "-qam", "notes: typo")
        # Not signed yet: the admitted notes stay listed, the release with
        # them, and the unsigned ones are never published.
        kept = self.index()["releases"]["2.3.0"]
        self.assertEqual(kept["notes_sha256"], first)
        # Signed and uploaded by the maintainer, they are published.
        sha = self.stage().publish_notes()
        entry = self.index()["releases"]["2.3.0"]
        self.assertEqual(entry["notes_sha256"], sha)
        self.assertNotEqual(sha, first)
        self.check(deep=False)

    # --- refusals ------------------------------------------------------

    def test_an_unsigned_tag_is_never_signed_for(self):
        self.tag(signed=False)
        self.build()
        self.assert_refused(self.stage().run, "not signed by a maintainer")

    def test_a_tag_signed_by_someone_else_is_refused(self):
        other = self.new_key("Someone Else <else@example.org>")
        self.git("-c", f"user.signingkey={other}", "tag", "-s", "v2.3.0",
                 "-m", "not the maintainer")
        self.build()
        self.assert_refused(self.stage().run, "not .*maintainer")

    def test_a_certificate_with_another_subject_is_refused(self):
        self.tag()
        self.build()
        key, cert = self.tmp / "k2.pem", self.tmp / "c2.pem"
        subprocess.run(["openssl", "req", "-x509", "-newkey", "rsa:2048",
                        "-nodes", "-days", "2", "-subj", "/CN=Not Us",
                        "-keyout", key, "-out", cert], check=True,
                       capture_output=True)
        stage = self.stage(signer=authenticode.Signer(str(cert), str(key)),
                           authenticode_ca=str(cert))
        self.assert_refused(stage.run, "not '/CN=GpgFrontend Test Signing")

    def test_a_prerelease_never_goes_through_the_offline_stage(self):
        self.assert_refused(lambda: self.stage("2.3.0-rc.1"), "final releases")

    def test_the_host_refuses_a_manifest_not_signed_by_a_maintainer(self):
        self.tag()
        self.build()
        d = self.stage().run()
        sums = self.tmp / "builds" / self.product.slug / d["sums"]
        sums.write_text(sums.read_text().replace("0", "1", 1))
        self.assert_refused(self.index, "does not verify")

    def test_the_host_refuses_an_installer_signed_by_another_certificate(self):
        self.tag()
        self.build()
        d = self.stage().run()
        msi = next(a for a in d["artifacts"] if a["kind"] == "msi")
        target = self.tmp / "builds" / self.product.slug / msi["path"]
        key, cert = self.tmp / "k3.pem", self.tmp / "c3.pem"
        subprocess.run(["openssl", "req", "-x509", "-newkey", "rsa:2048",
                        "-nodes", "-days", "2", "-subj", "/CN=Not Us",
                        "-keyout", key, "-out", cert], check=True,
                       capture_output=True)
        resigned = self.tmp / "resigned.msi"
        authenticode.Signer(str(cert), str(key)).sign(
            self.tmp / "work" / "2.3.0" / "build" / msi["name"], resigned)
        target.write_bytes(resigned.read_bytes())
        self.assert_refused(self.index, "not the published bytes|not '/CN")

    def test_the_host_needs_the_notes(self):
        self.tag()
        self.build()
        self.stage().run()
        # Notes gone from both main and the tag's view: nothing to publish.
        self.git("rm", "-q", "release/notes/2.3.0.md")
        self.git("commit", "-qm", "drop notes")
        self.git("tag", "-d", "v2.3.0")
        self.assert_refused(self.index, "tag v2.3.0|notes")


import imported as imported_mod  # noqa: E402

IMPORT_TOOLS = TOOLS and tools("gpg", "gpgconf")


class FakeGitHub:
    """GitHub Releases, from a directory per tag: every file in it is an
    asset; `drafts` and `missing` shape the answers."""

    def __init__(self, root: Path):
        self.root = root

    def release(self, tag):
        d = self.root / tag
        if not d.is_dir():
            raise common.ReleaseError(f"GitHub release {tag}: 404")
        return {"tag_name": tag, "draft": False,
                "published_at": "2026-10-28T12:00:00Z",
                "assets": [{"name": p.name, "size": p.stat().st_size,
                            "browser_download_url": str(p)}
                           for p in sorted(d.iterdir())]}

    def download(self, url, dst):
        shutil.copy(url, dst)

    def page(self, tag):
        return f"https://github.example/releases/tag/{tag}"


@unittest.skipUnless(IMPORT_TOOLS, "needs gpg, minisign, rclone, git and cmake")
class ImportTest(unittest.TestCase):
    """Releases from before the pipeline: brought over on their original
    signatures, admitted by the host only when every file still verifies."""

    gpg = StableTest.gpg
    new_key = StableTest.new_key
    write_product = StableTest.write_product
    git = StableTest.git
    assert_refused = StableTest.assert_refused

    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp(prefix="gfr-import-"))
        self.gnupg = self.tmp / "gnupg"
        self.gnupg.mkdir(mode=0o700)
        self.fpr = self.new_key("Test Maintainer <maintainer@example.org>")
        self.product = self.write_product(self.tmp / "product", [self.fpr])
        self.builds = Store(str(self.tmp / "builds" / self.product.slug))
        self.update = Store(str(self.tmp / "update" / self.product.slug))
        self.state = self.tmp / "state"
        keys = self.tmp / "keys"
        keys.mkdir()
        self.art_key, self.art_pub = keypair(keys, "artifact")
        self.idx_key, self.idx_pub = keypair(keys, "index")
        self.github = FakeGitHub(self.tmp / "github")

        self.repo = self.tmp / "repo"
        self.repo.mkdir()
        self.git("init", "-q", "-b", "main")
        self.git("commit", "-q", "--allow-empty", "-m", "2.1.13")
        self.git("tag", "v2.1.13")
        self.git("commit", "-q", "--allow-empty", "-m", "2.2.4")
        self.git("tag", "v2.2.4")

    def tearDown(self):
        subprocess.run(["gpgconf", "--homedir", self.gnupg, "--kill", "all"],
                       capture_output=True)
        shutil.rmtree(self.tmp, ignore_errors=True)

    def publish_on_github(self, version, legs, *, signer=None, unsigned=()):
        """A hand-made release: each file with a binary detached `.sig`."""
        d = self.tmp / "github" / f"v{version}"
        d.mkdir(parents=True)
        for leg in legs:
            f = d / f"GpgFrontend-{version}-{leg}"
            f.write_bytes(f"{version} {leg}\n".encode() * 100)
            if leg not in unsigned:
                self.gpg("--local-user", f"{signer or self.fpr}!",
                         "--detach-sign", "--output", f"{f}.sig", f)
        return d

    def run_import(self, version, builds=True):
        job = imported_mod.Import(self.product, version, self.tmp / "work",
                                  builds=self.builds if builds else None,
                                  git_dir=self.repo, source=self.github)
        return job, job.run()

    def index(self):
        return index_mod.publish(
            self.product, self.builds, self.update, index_key=self.idx_key,
            artifact_pub=self.art_pub, artifact_key=self.art_key,
            state_dir=self.state, force=True, git_dir=self.repo)

    def check(self):
        return verify_mod.verify(self.product, self.update, self.builds,
                                 index_keys=[self.idx_pub],
                                 artifact_key=self.art_pub, deep=True)

    LEGS = ("linux-x86_64.AppImage", "windows-x86_64.msi",
            "windows-x86_64.msix", "macos-15.dmg")

    def test_old_releases_reach_a_verified_index(self):
        self.publish_on_github("2.2.4", self.LEGS)
        self.publish_on_github("2.1.13", ("qt5-linux-x86_64.AppImage",
                                          "macos-13.dmg",
                                          "windows-x86_64.wixpdb"))
        _, d = self.run_import("2.2.4")
        job, old = self.run_import("2.1.13")

        self.assertEqual(d["kind"], "imported")
        self.assertEqual(d["signing"], {"scheme": "openpgp",
                                        "fingerprint": self.fpr})
        self.assertEqual(d["commit"], self.git("rev-parse", "v2.2.4"))
        self.assertEqual(d["released_at"], "2026-10-28T12:00:00Z")
        self.assertEqual(len(d["artifacts"]), 4)
        self.assertEqual(job.skipped,
                         ["GpgFrontend-2.1.13-windows-x86_64.wixpdb "
                          "(no artifact rule)"])
        kinds = {(a["name"].split("2.1.13-")[1]): (a["arch"], a["flavor"])
                 for a in old["artifacts"]}
        self.assertEqual(kinds, {"qt5-linux-x86_64.AppImage":
                                 ("x86_64", "qt5"),
                                 "macos-13.dmg": ("x86_64", "installed")})
        # The published signature is the original one, byte for byte.
        name = "GpgFrontend-2.2.4-macos-15.dmg"
        original = (self.tmp / "github" / "v2.2.4" / f"{name}.sig").read_bytes()
        self.assertEqual(self.builds.read(f"releases/2.2.4/{name}.sig"),
                         original)

        index = self.index()
        self.assertEqual(index["channels"]["stable"], "2.2.4")
        self.assertEqual(index["series"]["2.2"]["latest"], "2.2.4")
        self.assertEqual(index["series"]["2.1"]["latest"], "2.1.13")
        self.assertNotIn("notes", index["releases"]["2.2.4"])
        report = self.check()
        self.assertEqual(sorted((c["version"], c["kind"], c["artifacts"])
                                for c in report["checked"]),
                         [("2.1.13", "imported", 2), ("2.2.4", "imported", 4)])

    def test_a_rerun_changes_nothing(self):
        self.publish_on_github("2.2.4", self.LEGS)
        _, first = self.run_import("2.2.4")
        _, again = self.run_import("2.2.4")
        self.assertEqual(first, again)

    def test_check_only_uploads_nothing(self):
        self.publish_on_github("2.2.4", self.LEGS)
        self.run_import("2.2.4", builds=False)
        self.assertEqual(self.builds.files(""), [])

    def test_what_the_signatures_do_not_cover_is_refused(self):
        d = self.publish_on_github("2.2.4", self.LEGS)
        (d / "GpgFrontend-2.2.4-macos-15.dmg").write_bytes(b"swapped")
        self.assert_refused(lambda: self.run_import("2.2.4"),
                            "macos-15.dmg: the signature does not verify")
        self.assertEqual(self.builds.files(""), [])

    def test_another_key_is_refused(self):
        other = self.new_key("Someone Else <else@example.org>")
        self.publish_on_github("2.2.4", self.LEGS, signer=other)
        # Not in maintainers.asc at all, so gpg cannot even check it.
        self.assert_refused(lambda: self.run_import("2.2.4"),
                            "signature does not verify|not a pinned")

    def test_an_unsigned_file_is_left_out(self):
        self.publish_on_github("2.2.4", self.LEGS,
                               unsigned=("windows-x86_64.msix",))
        job, d = self.run_import("2.2.4")
        self.assertEqual(len(d["artifacts"]), 3)
        self.assertEqual(job.skipped, ["GpgFrontend-2.2.4-windows-x86_64.msix "
                                       "(no signature)"])

    def test_releases_before_the_floor_and_prereleases_are_not_imported(self):
        self.assert_refused(lambda: self.run_import("2.1.4"), "older than 2.1.5")
        self.assert_refused(lambda: self.run_import("2.2.0-beta.1"),
                            "only final releases")

    def test_the_host_checks_every_file_again(self):
        self.publish_on_github("2.2.4", self.LEGS)
        self.run_import("2.2.4")
        # A file replaced behind the importer's back.
        stored = (self.tmp / "builds" / self.product.slug / "releases" /
                  "2.2.4" / "GpgFrontend-2.2.4-macos-15.dmg")
        stored.write_bytes(b"replaced")
        self.assert_refused(self.index, "not the descriptor's bytes")

    def test_the_host_refuses_a_moved_tag(self):
        self.publish_on_github("2.2.4", self.LEGS)
        self.run_import("2.2.4")
        self.git("commit", "-q", "--allow-empty", "-m", "later")
        self.git("tag", "-f", "v2.2.4")
        self.assert_refused(self.index, r"v2\.2\.4 is [0-9a-f]+, not")

    def notes_on_main(self, version, text):
        notes = self.repo / "release" / "notes"
        notes.mkdir(parents=True, exist_ok=True)
        (notes / f"{version}.md").write_text(text)
        self.git("add", "release")
        self.git("commit", "-q", "-m", f"notes for {version}")

    def sign_notes(self, version):
        return stable_mod.Stage(self.product, version, self.tmp / "work",
                                builds=self.builds, git_dir=self.repo,
                                gnupghome=str(self.gnupg)).publish_notes()

    def test_an_imported_release_is_countersigned_too(self):
        self.publish_on_github("2.2.4", self.LEGS)
        _, d = self.run_import("2.2.4")
        entry = self.index()["releases"]["2.2.4"]
        root = self.tmp / "builds" / self.product.slug
        sums = "releases/2.2.4/SHA256SUMS.txt"
        self.assertEqual(entry["sums_minisig"], sums + ".minisig")
        self.assertEqual(common.parse_manifest((root / sums).read_bytes(), sums),
                         {a["name"]: a["sha256"] for a in d["artifacts"]})
        minisig.verify([self.art_pub], root / sums, root / (sums + ".minisig"))
        for a in d["artifacts"]:
            minisig.verify([self.art_pub], root / a["path"],
                           root / (a["path"] + ".minisig"))
        self.assertTrue(self.builds.exists(f"commits/{d['commit']}"))
        self.check()

    def test_an_imported_file_without_its_sig_is_not_listed(self):
        self.publish_on_github("2.2.4", self.LEGS)
        _, d = self.run_import("2.2.4")
        root = self.tmp / "builds" / self.product.slug
        (root / d["artifacts"][0]["sig"]).unlink()
        with self.assertRaises(index_mod.IndexIncomplete) as caught:
            self.index()
        self.assertNotIn("2.2.4", caught.exception.index["releases"])
        self.assertEqual([p for p in root.rglob("*.minisig")], [])

    def test_notes_are_listed_once_a_maintainer_signed_them(self):
        self.publish_on_github("2.2.4", self.LEGS)
        self.run_import("2.2.4")
        self.notes_on_main("2.2.4", "Fixes.\n")

        # On main but not signed yet: listed without them, and said so.
        err = io.StringIO()
        with contextlib.redirect_stderr(err):
            index = self.index()
        self.assertNotIn("notes", index["releases"]["2.2.4"])
        self.assertIn("2.2.4: listed without notes", err.getvalue())

        digest = self.sign_notes("2.2.4")
        # The files were checked once; only the notes are checked now.
        stored = (self.tmp / "builds" / self.product.slug / "releases" /
                  "2.2.4" / "GpgFrontend-2.2.4-macos-15.dmg")
        original = stored.read_bytes()
        stored.write_bytes(b"replaced")
        entry = self.index()["releases"]["2.2.4"]
        stored.write_bytes(original)
        self.assertEqual(entry["notes"], f"notes/2.2.4/{digest}.md")
        self.assertEqual(entry["notes_sig"], f"notes/2.2.4/{digest}.md.sig")
        self.check()

        # A fix on main waits for its own signature; until then the signed
        # notes stay listed, and nothing is replaced.
        self.notes_on_main("2.2.4", "Fixes, spelled right.\n")
        with contextlib.redirect_stderr(io.StringIO()):
            self.assertEqual(self.index()["releases"]["2.2.4"]["notes"],
                             f"notes/2.2.4/{digest}.md")
        self.assertNotEqual(self.sign_notes("2.2.4"), digest)
        self.assertNotEqual(self.index()["releases"]["2.2.4"]["notes"],
                            f"notes/2.2.4/{digest}.md")

    def test_notes_signed_by_someone_else_are_refused(self):
        self.publish_on_github("2.2.4", self.LEGS)
        self.run_import("2.2.4")
        self.notes_on_main("2.2.4", "Fixes.\n")
        other = self.new_key("Someone Else <else@example.org>")
        digest = common.sha256_bytes(b"Fixes.\n")
        local = self.tmp / "notes.md"
        local.write_bytes(b"Fixes.\n")
        openpgp.sign(local, other, str(self.gnupg))
        self.builds.put_file(local, f"notes/2.2.4/{digest}.md")
        self.builds.put_file(self.tmp / "notes.md.sig",
                             f"notes/2.2.4/{digest}.md.sig")
        self.assert_refused(self.index, "signature does not verify|not a pinned")

    def test_the_host_admits_once(self):
        self.publish_on_github("2.2.4", self.LEGS)
        self.run_import("2.2.4")
        self.index()
        # Admitted and recorded: the next round does not download again.
        stored = (self.tmp / "builds" / self.product.slug / "releases" /
                  "2.2.4" / "GpgFrontend-2.2.4-macos-15.dmg")
        stored.write_bytes(b"replaced")
        self.index()

