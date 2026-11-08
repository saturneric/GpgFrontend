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

STABLE_TOOLS = TOOLS and all(shutil.which(t) for t in
                             ("gpg", "gpgconf", "osslsigncode", "wixl",
                              "openssl"))
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

    def write_product(self, directory, fingerprints):
        directory.mkdir(parents=True, exist_ok=True)
        data = json.loads((common.ROOT / "release" / "product.json").read_text())
        data["signing"]["maintainer_fingerprints"] = fingerprints
        data["signing"]["authenticode_subject"] = SUBJECT
        (directory / "product.json").write_text(json.dumps(data))
        shutil.copy(common.ROOT / "release" / "policy.toml", directory)
        (directory / "maintainers.asc").write_text(
            self.gpg("--armor", "--export", *fingerprints))
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
            artifact_pub=self.art_pub, state_dir=self.state, force=True,
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

    def test_an_interrupted_stage_resumes(self):
        self.tag()
        self.build()
        broken = self.stage(key="0" * 40)  # no such key: pgp fails
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
        # Not signed yet: the host refuses rather than publish unsigned notes.
        self.assert_refused(self.index, "notes")
        # Signed and uploaded by the maintainer, it is published.
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

    def test_a_beta_never_goes_through_the_offline_stage(self):
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

IMPORT_TOOLS = TOOLS and all(shutil.which(t) for t in ("gpg", "gpgconf"))


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
            artifact_pub=self.art_pub, state_dir=self.state, force=True,
            git_dir=self.repo)

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

        # A fix on main waits for its own signature; nothing is replaced.
        self.notes_on_main("2.2.4", "Fixes, spelled right.\n")
        with contextlib.redirect_stderr(io.StringIO()):
            self.assertNotIn("notes", self.index()["releases"]["2.2.4"])
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

