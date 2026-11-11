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
import datetime as dt
import fnmatch
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
from unittest import mock

ROOT = Path(__file__).resolve().parent.parent.parent
GFR = ROOT / "scripts" / "release" / "gfrelease"
sys.path.insert(0, str(GFR))
sys.path.insert(0, str(ROOT / "scripts" / "tests"))

import common  # noqa: E402
import host as host_mod  # noqa: E402
import index as index_mod  # noqa: E402
import layout  # noqa: E402
import minisig  # noqa: E402
import openpgp  # noqa: E402
import promote as promote_mod  # noqa: E402
import stage as stage_mod  # noqa: E402
import trust  # noqa: E402
import verify as verify_mod  # noqa: E402
from store import Store, StoreUnavailable  # noqa: E402
from test_release_candidate import FAKE_COSIGN  # noqa: E402
from trustutil import git_env, keypair, tools, write_root  # noqa: E402


TOOLS = tools("minisign", "rclone", "git", "cmake")
ISSUER = "https://token.actions.githubusercontent.com"


TRUST_KEYS = Path(__file__).resolve().parent / "fixtures" / "trust" / "keys"


def published(update: Store) -> dict:
    """The index in @p update as it stands, unverified: what a test looks
    at after the run that published it."""
    return common.load_json(update.read(common.INDEX), common.INDEX)


def make_product(directory: Path, index_pubs, after=None,
                 **host_policy) -> common.Product:
    """This product as release/product.json has it, published under a
    trust root of its own (fixtures/trust/keys: root keys a and b sign it,
    g1 signs the index-keys list) that lists @p index_pubs, and whose
    host_policy is @p host_policy over a plain one. With @p after (a
    product made so before), its trust goes on: the policy comes as the
    next root, as it would in the real world, where a published root
    never changes."""
    directory.mkdir(parents=True, exist_ok=True)
    for name in ("product.json", "policy.toml"):
        shutil.copy(common.ROOT / "release" / name, directory)
    if after is not None:
        shutil.copytree(after.root_dir, directory / "root")
        shutil.copytree(after.index_keys_dir, directory / "index-keys")
        product = common.Product(directory / "product.json",
                                 directory / "policy.toml",
                                 root_dir=directory / "root",
                                 index_keys_dir=directory / "index-keys")
        root = product.trust[0]
        next_root(product, host_policy=dict(root["host_policy"],
                                            **host_policy))
        return product
    pub = {n: minisig.public_key_of(TRUST_KEYS / f"{n}.pub")
           for n in ("root-a", "root-b", "g1")}
    ids = {n: minisig.key_id(k) for n, k in pub.items()}
    head = {"schema": 1, "product": "gpgfrontend", "version": 1,
            "expires": "2099-12-31T00:00:00Z"}
    root = dict(head, type="root", keys={ids[n]: pub[n] for n in pub},
                roles={"root": {"keyids": [ids["root-a"], ids["root-b"]],
                                "threshold": 2},
                       "index_list": {"keyids": [ids["g1"]],
                                      "threshold": 1}},
                host_policy={"repository": "saturneric/GpgFrontend",
                             "workflow": ".github/workflows/build.yml",
                             "stable_pgp": {"fingerprints": ["A" * 40]},
                             **host_policy})
    keys = dict(head, type="index-keys",
                keys={minisig.key_id(k): k for k in index_pubs})
    (directory / "root").mkdir(exist_ok=True)
    (directory / "index-keys").mkdir(exist_ok=True)
    path = directory / "index-keys" / layout.index_keys_name(1)
    path.write_bytes(trust.dump(keys))
    minisig.sign(TRUST_KEYS / "g1.key", path,
                 layout.index_keys_comment("gpgfrontend", 1))
    product = common.Product(directory / "product.json",
                             directory / "policy.toml",
                             root_dir=directory / "root",
                             index_keys_dir=directory / "index-keys")
    sign_root(product, root)
    return product


def sign_root(product: common.Product, doc: dict) -> None:
    """@p doc as the next root of @p product's trust, signed by root keys
    a and b."""
    write_root(product.root_dir, doc,
               [TRUST_KEYS / f"{name}.key" for name in ("root-a", "root-b")],
               "gpgfrontend")
    product._trust = None  # read again


def next_root(product: common.Product, **changes) -> dict:
    """The next root of @p product's trust, with @p changes."""
    doc = dict(trust.renewed(product.trust[0], trust.ROOT_LIFETIME),
               **changes)
    sign_root(product, doc)
    return doc


def trust_args(product: common.Product) -> list[str]:
    """The command line that names @p product and its trust."""
    return ["--product", str(product.keyring_path.with_name("product.json")),
            "--root-dir", str(product.root_dir),
            "--index-keys-dir", str(product.index_keys_dir)]


def flaky_rclone(directory: Path, marker: str, verb: str = "") -> str:
    """An rclone that fails like an outage (not "not found") for any path
    containing @p marker (when running @p verb, if given), and is rclone for
    everything else."""
    script = directory / f"rclone-down-{abs(hash((marker, verb)))}"
    only = f'[ "$1" = "{verb}" ] || exec rclone "$@"\n' if verb else ""
    script.write_text("#!/bin/sh\n" + only +
                      "for a in \"$@\"; do case \"$a\" in "
                      f"*{marker}*) echo 'simulated outage' >&2; exit 1;; "
                      "esac; done\n"
                      "exec rclone \"$@\"\n")
    script.chmod(0o755)
    return str(script)


BAD_VALUES = (None, 5, "x", [], {}, True, 1.5)


def mutations(d: dict, values=BAD_VALUES):
    """(label, copy of @p d) with one field, at any depth a descriptor has,
    replaced by each of @p values."""
    import copy
    paths = [(k,) for k in d]
    for k in ("signing", "build"):
        if isinstance(d.get(k), dict):
            paths += [(k, sub) for sub in d[k]]
    if d.get("artifacts"):
        paths += [("artifacts", 0, sub) for sub in d["artifacts"][0]]
    for path in paths:
        for value in values:
            m = copy.deepcopy(d)
            target = m
            for step in path[:-1]:
                target = target[step]
            target[path[-1]] = value
            yield f"{'.'.join(map(str, path))}={value!r}", m


def assert_only_release_errors(test, admit, d: dict, values=BAD_VALUES):
    """Whatever shape a descriptor has, admitting it either passes or
    raises ReleaseError: never a traceback that stops the whole run."""
    for label, m in mutations(d, values):
        with test.subTest(label):
            try:
                admit(json.dumps(m).encode())
            except common.ReleaseError:
                pass


@unittest.skipUnless(TOOLS, "needs minisign, rclone, git and cmake")
class ChainTest(unittest.TestCase):

    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp(prefix="gfr-test-"))
        keys = self.tmp / "keys"
        keys.mkdir()
        self.idx_key, self.idx_pub = keypair(keys, "index")
        self.product = make_product(self.tmp / "trusted", [self.idx_pub])
        self.slug = self.product.slug
        self.staging = Store(str(self.tmp / "staging"))
        self.builds = Store(str(self.tmp / "builds" / self.slug))
        self.update = Store(str(self.tmp / "update" / self.slug))
        self.state = self.tmp / "state"
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
                                   git_dir=self.repo, cosign=str(self.cosign))

    def index(self, force=False):
        return index_mod.publish(self.product, self.builds, self.update,
                                 index_key=self.idx_key,
                                 state_dir=self.state, force=force,
                                 git_dir=self.repo, cosign=str(self.cosign),
                                 index_pub=[self.idx_pub])

    def check(self, deep=True):
        return verify_mod.verify(self.product, self.update, self.builds,
                                 deep=deep)

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

    def test_a_release_branch_build_is_the_beta_channel(self):
        self.commit()
        _, nightly = self.build()
        self.git("checkout", "-q", "-b", "release/2.2", "v2.2.4")
        self.commit()
        _, version = self.build(ref="refs/heads/release/2.2")
        self.assertTrue(version.startswith("2.2.5-alpha.1+g"))
        self.promote()
        index = self.index()
        # Never main's nightly; the beta channel, signed the same way.
        self.assertEqual(index["channels"]["nightly"], nightly)
        self.assertEqual(index["channels"]["beta"], version)
        self.assertEqual(index["beta"]["version"], version)
        self.assertEqual(index["beta"]["descriptor"],
                         f"nightly/{version.replace('+', '-')}/"
                         f"{common.DESCRIPTOR}")
        self.check()
        # A series users are no longer meant to be on offers no beta.
        self.product.policy["series"]["2.2"]["status"] = "legacy"
        index = self.index(force=True)
        self.assertIsNone(index["channels"]["beta"])
        self.assertIsNone(index["beta"])
        self.check()

    def test_a_tampered_beta_entry_is_caught(self):
        self.git("checkout", "-q", "-b", "release/2.2", "v2.2.4")
        self.commit()
        self.build(ref="refs/heads/release/2.2")
        self.promote()
        self.index()
        path = self.tmp / "update" / self.slug / common.INDEX
        tampered = json.loads(path.read_text())
        tampered["channels"]["beta"] = "2.2.9-beta.1"
        path.write_bytes(common.canonical_json(tampered))
        minisig.sign(self.idx_key, path,
                     f"product={self.slug} serial={tampered['serial']}")
        self.assert_refused(self.check, "channels.beta")

    def resign(self, index: dict) -> None:
        """@p index published as it is, signed with the index key: what
        whoever holds that key could publish."""
        path = self.tmp / "update" / self.slug / common.INDEX
        path.write_bytes(common.canonical_json(index))
        minisig.sign(self.idx_key, path,
                     layout.index_comment(self.slug, index["serial"]))

    def test_each_series_offers_only_its_own_beta(self):
        self.git("checkout", "-q", "-b", "release/2.2", "v2.2.4")
        self.commit()
        _, older = self.build(ref="refs/heads/release/2.2")
        self.commit()
        _, newer = self.build(ref="refs/heads/release/2.2")
        self.assertEqual([d["version"] for d in self.promote_all()],
                         [older, newer])
        index = self.index()
        self.assertEqual(index["series"]["2.2"]["beta"]["version"], newer)
        # The mainline series has no release branch build: no beta.
        self.assertIsNone(index["series"]["2.3"]["beta"])
        # The top-level entry, for clients that predate the per-series
        # one, is the newest of them.
        self.assertEqual(index["beta"], index["series"]["2.2"]["beta"])
        self.check()

    def promote_all(self) -> list[dict]:
        """Every staged branch candidate promoted, oldest first, as rounds
        of a host that ran after each push would have."""
        done = []
        for tried in promote_mod.by_ref_newest_first(
                promote_mod.candidates(self.staging)):
            for prefix, staged in reversed(tried):
                dest = promote_mod.final_prefix(staged)
                if self.builds.exists(layout.descriptor_of(dest)):
                    continue
                done.append(promote_mod.promote_one(
                    self.product, self.staging, self.builds, prefix, staged,
                    git_dir=self.repo, cosign=str(self.cosign)))
        return done

    def test_a_beta_another_series_claims_is_caught(self):
        self.git("checkout", "-q", "-b", "release/2.2", "v2.2.4")
        self.commit()
        _, version = self.build(ref="refs/heads/release/2.2")
        self.promote()
        index = self.index()
        # Offered as another series' beta, by whoever holds the index key.
        moved = json.loads(json.dumps(index))
        moved["series"]["2.3"]["beta"] = moved["series"]["2.2"]["beta"]
        moved["series"]["2.2"]["beta"] = None
        self.resign(moved)
        self.assert_refused(self.check, "series 2.3 offers")
        # A beta of a series no longer offered one.
        legacy = json.loads(json.dumps(index))
        legacy["series"]["2.2"]["status"] = "legacy"
        self.resign(legacy)
        self.assert_refused(self.check, "offers no beta")
        # The top-level beta, not the newest of the series' ones.
        stray = json.loads(json.dumps(index))
        stray["beta"] = stray["nightly"] = None
        stray["channels"]["beta"] = stray["channels"]["nightly"] = None
        self.resign(stray)
        self.assert_refused(self.check, "not the newest|does not list")

    def test_a_beta_no_series_offers_is_refused(self):
        # The shape of an index from before each series named its own beta:
        # none is live any more, and the top-level beta alone is not one.
        self.git("checkout", "-q", "-b", "release/2.2", "v2.2.4")
        self.commit()
        self.build(ref="refs/heads/release/2.2")
        self.promote()
        old = self.index()
        for entry in old["series"].values():
            del entry["beta"]
        self.resign(old)
        self.assert_refused(self.check, "not the newest")

    def test_a_published_index_the_host_did_not_sign_is_not_believed(self):
        self.commit()
        self.build()
        self.promote()
        first = self.index()
        # Someone with write access to the bucket, not the key, publishes a
        # serial far ahead (clients would refuse the host's next index as a
        # replay) and drops the nightly.
        forged = dict(first, serial=10 ** 9, nightly=None)
        forged["channels"] = dict(first["channels"], nightly=None)
        path = self.tmp / "update" / self.slug / common.INDEX
        path.write_bytes(common.canonical_json(forged))
        _, other = keypair(self.tmp / "keys", "intruder")
        minisig.sign(self.tmp / "keys" / "intruder.key", path,
                     layout.index_comment(self.slug, forged["serial"]))
        with contextlib.redirect_stderr(io.StringIO()) as err:
            again = self.index()
        self.assertIn("not one this host signed", err.getvalue())
        self.assertEqual(again["serial"], first["serial"] + 1)
        self.assertEqual(again["channels"]["nightly"],
                         first["channels"]["nightly"])
        self.check()

    def test_a_lost_state_and_an_unverifiable_index_publish_nothing(self):
        self.commit()
        self.build()
        self.promote()
        self.index()
        shutil.rmtree(self.state)
        sig = self.tmp / "update" / self.slug / (common.INDEX + ".minisig")
        sig.write_text("cut short\n")
        # Any serial picked now could be below one clients have seen.
        self.assert_refused(self.index, "next serial is not known")
        self.state.mkdir(exist_ok=True)
        (self.state / "serial").write_text("1\n")
        self.assertEqual(self.index()["serial"], 2)
        self.check()

    def test_a_serial_whose_history_exists_is_never_signed_again(self):
        self.commit()
        self.build()
        self.promote()
        self.index()
        # A run that died after writing the history of serial 2 and before
        # the live index, on a host that then lost its state.
        self.update.put_bytes(b"{}", layout.history_path(2))
        shutil.rmtree(self.state)
        self.assertEqual(self.index(force=True)["serial"], 3)
        self.check()

    def test_history_goes_up_before_the_index_goes_live(self):
        self.commit()
        self.build()
        self.promote()
        self.index()
        down = Store(str(self.tmp / "update" / self.slug),
                     rclone=flaky_rclone(self.tmp, "history", "copyto"))
        self.commit()
        self.build()
        self.promote()
        with self.assertRaises(StoreUnavailable):
            index_mod.publish(self.product, self.builds, down,
                              index_pub=[self.idx_pub],
                              index_key=self.idx_key, state_dir=self.state,
                              git_dir=self.repo, cosign=str(self.cosign))
        # The live index is the first one still, whole and verified.
        self.assertEqual(self.check(deep=False)["serial"], 1)

    def host(self, **kw) -> host_mod.Host:
        kw.setdefault("cosign", str(self.cosign))
        return host_mod.Host(
            self.product, self.builds, self.state,
            staging=self.staging, update=self.update, git_dir=self.repo,
            index_key=self.idx_key,
            index_pub=[self.idx_pub], **kw)

    def round(self, host: host_mod.Host | None = None):
        out, err = io.StringIO(), io.StringIO()
        with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
            status = (host or self.host()).round(promote=True, index=True)
        return status, out.getvalue(), err.getvalue()

    def test_a_round_says_done_partial_or_failed(self):
        self.commit()
        prefix, version = self.build()
        status, out, _ = self.round()
        self.assertEqual(status, host_mod.DONE)
        self.assertIn(f"{host_mod.PUBLISHED} 1, stable None, beta None, "
                      f"nightly {version}", out)
        status, out, _ = self.round()
        self.assertEqual((status, out.strip()),
                         (host_mod.DONE, host_mod.UNCHANGED))

        # A forged candidate: refused for what it is, every round alike.
        staged = json.loads(self.staging.read(f"{prefix}/{common.STAGED}"))
        self.forge_staged("branches/main/9.9.9-alpha.1", staged,
                          version="9.9.9-alpha.1", filesafe="9.9.9-alpha.1")
        status, out, err = self.round()
        self.assertEqual(status, host_mod.PARTIAL)
        self.assertIn("not promoted", err)
        self.assertIn(host_mod.UNCHANGED, out)

        # A store that does not answer: worth another round as it is.
        down = self.host()
        down.staging = Store(self.staging.root,
                             rclone=flaky_rclone(self.tmp, "staging"))
        status, out, err = self.round(down)
        self.assertEqual(status, host_mod.FAILED)
        # The index is kept all the same.
        self.assertIn(host_mod.UNCHANGED, out)

    def test_a_tool_that_does_not_answer_is_retried_not_refused(self):
        self.commit()
        self.build()
        hung = self.tmp / "hung-cosign"
        hung.write_text("#!/bin/sh\nsleep 30\n")
        hung.chmod(0o755)
        saved = common.rc.COSIGN_TIMEOUT
        common.rc.COSIGN_TIMEOUT = 1
        try:
            status, _, err = self.round(self.host(cosign=str(hung)))
        finally:
            common.rc.COSIGN_TIMEOUT = saved
        # Nothing was decided about the candidate: the next round asks
        # again, rather than counting it refused until something changes.
        self.assertEqual(status, host_mod.FAILED, err)
        self.assertIn("did not finish", err)
        # A git that cannot be run is no verdict either.
        self.assertTrue(common.is_passing(common.ToolFailed("git failed")))
        self.assertFalse(common.is_passing(common.ReleaseError("forged")))

    def test_a_build_that_could_not_be_checked_keeps_its_listing(self):
        self.commit()
        _, version = self.build()
        self.promote()
        self.index()
        previous = index_mod.trusted_published(self.product, self.update,
                                               [self.idx_pub])

        def admit(*_args, **_kw):
            raise common.ToolFailed("cosign cannot be run")
        saved = index_mod.admit_nightly
        index_mod.admit_nightly = admit
        try:
            refused = common.Problems()
            content = index_mod.build(self.product, self.builds,
                                      refused=refused, previous=previous)
            # Not checked is not refused: still listed, the run still fails.
            self.assertEqual(content["channels"]["nightly"], version)
            self.assertTrue(refused and refused.passing)

            def forged(*_args, **_kw):
                raise common.ReleaseError("not what CI built")
            index_mod.admit_nightly = forged
            refused = common.Problems()
            content = index_mod.build(self.product, self.builds,
                                      refused=refused, previous=previous)
            # Refused for what it is: off the channel.
            self.assertIsNone(content["channels"]["nightly"])
            self.assertFalse(refused.passing)
        finally:
            index_mod.admit_nightly = saved

    def test_the_cli_exits_with_the_rounds_status(self):
        self.commit()
        prefix, _ = self.build()
        staged = json.loads(self.staging.read(f"{prefix}/{common.STAGED}"))
        self.forge_staged("branches/main/9.9.9-alpha.1", staged,
                          version="9.9.9-alpha.1", filesafe="9.9.9-alpha.1")
        out, err = io.StringIO(), io.StringIO()
        with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
            status = self.cli().main([
                *trust_args(self.product), "run",
                "--staging", self.staging.root, "--builds", self.builds.root,
                "--update", self.update.root, "--git-dir", str(self.repo),
                "--state-dir", str(self.state),
                "--index-key", str(self.idx_key), "--index-pub", self.idx_pub,
                "--cosign", str(self.cosign)])
        self.assertEqual(status, host_mod.PARTIAL, err.getvalue())

    def test_the_signing_host_lists_what_index_reads(self):
        """The Release-Server container tells whether anything changed by
        listing these paths, and runs gfrelease only when something did:
        whatever index and promote read must be under one of them."""
        listed = ("branches/**/staged.json", "/releases/**", "/notes/**",
                  "/nightly/*/release.json")
        self.assertEqual(layout.BRANCHES, "branches")
        self.assertEqual(common.STAGED, "staged.json")
        self.assertEqual(common.DESCRIPTOR, "release.json")
        for path in (layout.release_dir("2.3.0") + "/x",
                     layout.notes_path("2.3.0", "a" * 64),
                     layout.descriptor_of(layout.nightly_dir("2.3.0-alpha.1"))):
            self.assertTrue(any(fnmatch.fnmatch("/" + path, pattern.replace(
                "**", "*")) for pattern in listed), path)

    # --- refusals ------------------------------------------------------

    def assert_refused(self, fn, pattern):
        with self.assertRaisesRegex(common.ReleaseError, pattern):
            fn()

    def test_a_version_git_does_not_confirm_is_refused(self):
        self.commit()
        self.build(claim="9.9.9-alpha.1")
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
        sums = self.tmp / "builds" / self.slug / d["build"]["sums"]
        sums.chmod(0o644)
        sums.write_text(sums.read_text().replace("0", "1", 1))
        self.assert_refused(self.index, "does not verify")

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
        other = make_product(self.tmp / "other", [self.idx_pub])
        other.slug = "demo"
        self.assert_refused(
            lambda: verify_mod.verify(other, self.update, self.builds),
            "is for 'gpgfrontend'|not 'demo'")

    def test_verify_rejects_the_wrong_key(self):
        self.commit()
        self.build()
        self.promote()
        self.index()
        # Signed by a key the index-keys list does not hold.
        secret, _ = keypair(self.tmp / "keys", "other")
        path = self.tmp / "update" / self.slug / common.INDEX
        index = json.loads(path.read_text())
        minisig.sign(secret, path,
                     layout.index_comment(self.slug, index["serial"]))
        self.assert_refused(lambda: self.check(deep=False), "no pinned key")

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
                branch_version.replace("+", "-") / "build" / common.SUMS)
        sums.write_text(sums.read_text().replace("0", "1", 1))
        with self.assertRaises(index_mod.IndexIncomplete) as caught:
            self.index()
        self.assertIn(branch_version.replace("+", "-"),
                      "\n".join(caught.exception.refused))
        self.assertEqual(caught.exception.index["channels"]["nightly"], version)
        self.assertEqual(published(self.update)["serial"], 1)
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
                                    git_dir=self.repo,
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

    def at(self, when):
        os.environ["GFR_NOW"] = when
        self.addCleanup(os.environ.pop, "GFR_NOW", None)

    def test_a_nightly_about_to_expire_is_never_named(self):
        # 30-day retention: named while the index it is in (14 days) cannot
        # outlive the nightly, never after.
        self.at("2026-11-01T00:00:00Z")
        self.commit()
        _, version = self.build()
        self.promote()
        self.assertEqual(self.index()["channels"]["nightly"], version)
        self.at("2026-11-15T00:00:00Z")
        self.assertEqual(self.index(force=True)["channels"]["nightly"],
                         version)
        self.at("2026-11-17T00:00:00Z")
        err = io.StringIO()
        with contextlib.redirect_stderr(err):
            index = self.index(force=True)
        self.assertIsNone(index["channels"]["nightly"])
        self.assertIsNone(index["nightly"])
        self.assertIn("no main nightly promoted since", err.getvalue())
        self.assertGreater(
            common.parse_iso(index["expires"]) + index_mod.NIGHTLY_MARGIN
            - index_mod.LIFETIME, common.parse_iso(index["generated_at"]))

    def test_a_nightly_the_bucket_is_deleting_is_no_refusal(self):
        self.at("2026-11-01T00:00:00Z")
        self.commit()
        self.build()
        (d,) = self.promote()
        self.index()
        # The lifecycle rule has started on it: its manifest is gone.
        self.at("2026-11-29T00:00:00Z")
        (self.tmp / "builds" / self.slug / d["build"]["sums"]).unlink()
        with contextlib.redirect_stderr(io.StringIO()):
            index = self.index(force=True)
        self.assertIsNone(index["nightly"])

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
        _, other = keypair(self.tmp / "keys", "other")
        listed = make_product(self.tmp / "other", [other])
        err = io.StringIO()
        with contextlib.redirect_stderr(err):
            status = self.cli().main([
                *trust_args(listed),
                "index", "--builds", str(self.tmp / "b"),
                "--update", str(self.tmp / "u"),
                "--state-dir", str(self.state),
                "--index-key", str(self.idx_key), "--index-pub", self.idx_pub])
        self.assertEqual(status, 1)
        self.assertIn("not in index-keys 1", err.getvalue())
        self.assertFalse(self.state.exists())

    def test_a_monitor_warns_before_the_index_expires(self):
        self.commit()
        self.build()
        self.promote()
        self.index()
        args = [*trust_args(self.product), "verify",
                "--update", str(self.tmp / "update" / self.slug),
                "--builds", str(self.tmp / "builds" / self.slug)]
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
                                   index_pub=[self.idx_pub],
                                   state_dir=self.state, git_dir=self.repo,
                                   cosign=str(self.cosign))
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
                    "https://x/y", "a%2fb", "a//b", "", "a/b\n/c",
                    "a/b\n", ".", "a/.hidden", None):
            with self.subTest(path=bad):
                self.assert_refused(lambda: common.check_relpath(bad),
                                    "not a product-relative path")

    def test_every_published_index_is_kept_in_the_history(self):
        published = []
        for _ in range(2):
            self.commit()
            self.build()
            self.promote()
            published.append(self.index())
        for index in published:
            path = f"v1/history/{index['serial']}.json"
            data = self.update.read(path)
            self.assertEqual(json.loads(data), index)
            local = self.tmp / f"history-{index['serial']}.json"
            local.write_bytes(data)
            Path(f"{local}.minisig").write_bytes(
                self.update.read(path + ".minisig"))
            minisig.verify([self.idx_pub], local, Path(f"{local}.minisig"))

    def test_a_broken_signature_on_the_published_index_is_repaired(self):
        self.commit()
        self.build()
        self.promote()
        first = self.index()
        sig = self.tmp / "update" / self.slug / (common.INDEX + ".minisig")
        sig.write_text("cut short\n")
        again = self.index()
        self.assertIsNotNone(again)
        self.assertGreater(again["serial"], first["serial"])
        self.check()

    def test_the_trust_an_index_names_goes_up_before_it(self):
        order = []

        class Recording(Store):
            def put_file(self, local, path, immutable=True):
                order.append(path)
                super().put_file(local, path, immutable)

        self.update = Recording(self.update.root)
        # What a crash can leave beside the trust files: never published.
        for name in ("1.root.json.new", "1.root.json.new.minisig",
                     "01.root.json"):
            (self.product.root_dir / name).write_text("left behind\n")
        self.commit()
        self.build()
        self.promote()
        index = self.index()
        self.assertEqual((index["root_version"],
                          index["index_keys_version"]), (1, 1))
        trust_paths = [path for _, path in index_mod.trust_files(self.product)]
        self.assertEqual(len(trust_paths), 5)  # a root, 2 sigs, a list, 1
        (live,) = [i for i, p in enumerate(order) if p == common.INDEX]
        for path in trust_paths:
            self.assertIn(path, order[:live])
            self.assertTrue(self.update.exists(path), path)
        self.check()

    def test_verify_walks_to_the_root_an_index_names(self):
        self.commit()
        self.build()
        self.promote()
        # The host trusts root 2 and publishes under it; a reader of this
        # tree's root 1 walks there through v1/root/ as a client would.
        host = make_product(self.tmp / "host", [self.idx_pub])
        next_root(host)
        index_mod.publish(host, self.builds, self.update,
                          index_key=self.idx_key, state_dir=self.state,
                          git_dir=self.repo, cosign=str(self.cosign),
                          index_pub=[self.idx_pub])
        self.assertEqual(self.product.trust[0]["version"], 1)
        self.assertEqual(self.check(deep=False)["serial"], 1)
        # A root the index names, not published: a client gets nowhere.
        (self.tmp / "update" / self.slug / "v1" / "root" /
         "2.root.json").unlink()
        self.assert_refused(lambda: self.check(deep=False),
                            "names root 2")

    def test_a_candidate_that_cannot_be_read_still_lets_the_index_go(self):
        # A candidate the staging lifecycle deleted while it was being
        # fetched (or a disk that filled up): an OSError, no verdict on it.
        self.commit()
        _, version = self.build()
        self.promote()
        self.commit()
        self.build()

        def vanished(*_args, **_kw):
            raise FileNotFoundError("staged.json")
        with mock.patch.object(promote_mod, "promote_one", vanished):
            status, out, err = self.round()
        self.assertEqual(status, host_mod.FAILED, err)
        self.assertIn("staged.json", err)
        # The index was still built from what is there.
        self.assertIn(f"{host_mod.PUBLISHED} 1", out)
        self.assertEqual(published(self.update)["channels"]["nightly"],
                         version)

    def test_a_minisign_that_does_not_run_never_replaces_the_index(self):
        # Taking it for "not an index this host signed" would build the
        # next one without what the published one lists.
        self.commit()
        self.build()
        self.promote()
        self.index()
        with mock.patch.object(minisig, "run",
                               side_effect=common.ToolFailed("no minisign")):
            with self.assertRaisesRegex(common.ToolFailed, "no minisign"):
                index_mod.trusted_published(self.product, self.update,
                                            [self.idx_pub])

    def test_verify_reads_the_index_as_a_client_does(self):
        self.commit()
        self.build()
        self.promote()
        self.index()
        index = published(self.update)
        comment = layout.index_comment(self.slug, index["serial"])
        verify_mod.check_index(self.product, index, comment)
        expires = common.parse_iso(index["expires"])
        for generated, why in (
                (common.iso(expires - verify_mod.MAX_INDEX_LIFETIME
                            - dt.timedelta(seconds=1)),
                 "longer than any client takes"),
                (None, "UTC time"), ("2026-1-1T00:00:00Z", "UTC time")):
            with self.subTest(generated):
                self.assert_refused(lambda: verify_mod.check_index(
                    self.product, dict(index, generated_at=generated),
                    comment), why)
        # A day past its expiry is still a clock that is off, as for clients.
        later = common.iso(expires + dt.timedelta(hours=12))
        with mock.patch.dict(os.environ, {"GFR_NOW": later}):
            verify_mod.check_index(self.product, index, comment)

    def test_notes_no_client_would_read_are_refused(self):
        notes = self.repo / "release" / "notes" / "2.3.0.md"
        notes.parent.mkdir(parents=True)
        notes.write_bytes(b"x" * (common.NOTES_LIMIT + 1))
        self.git("add", "release")
        self.commit()
        self.assert_refused(lambda: common.notes_at(
            self.repo, common.refs.MAIN, "2.3.0"), "no client reads")

    def forge_staged(self, prefix, staged, **changes):
        """A staged.json in the untrusted staging bucket, written by anyone."""
        self.staging.put_bytes(common.canonical_json(dict(staged, **changes)),
                               f"{prefix}/{common.STAGED}")

    def test_an_interrupted_stage_upload_finishes_on_a_rerun(self):
        self.commit()
        prefix, version = self.build()
        # As if the earlier attempt died before staged.json, with a bundle
        # of other bytes (a re-run signs again) already up.
        root = self.tmp / "staging" / prefix
        (root / common.STAGED).unlink()
        (root / common.BUNDLE).write_text("an earlier attempt's bundle")
        cand = self.tmp / f"cand-{version.replace('+', '-')}"
        commit = self.git("rev-parse", "HEAD")
        stage_mod.stage(self.product, cand, self.staging,
                        ref="refs/heads/main", commit=commit, version=version,
                        filesafe=version.replace("+", "-"),
                        built_at="2026-11-06T12:00:00Z")
        self.assertEqual((root / common.BUNDLE).read_bytes(),
                         (cand / common.BUNDLE).read_bytes())
        self.assertEqual([d["version"] for d in self.promote()], [version])

    def test_a_forged_candidate_never_writes_a_commit_marker(self):
        self.commit()
        prefix, _ = self.build()
        (d,) = self.promote()
        staged = json.loads(self.staging.read(f"{prefix}/{common.STAGED}"))
        forged = "f" * 40
        self.forge_staged("branches/main/0-forged", staged, commit=forged)
        try:
            self.promote()
        except promote_mod.PromoteIncomplete:
            pass
        self.assertFalse(self.builds.exists(f"commits/{forged}"))
        self.assertTrue(self.builds.exists(f"commits/{d['commit']}"))

    def test_a_forged_higher_candidate_does_not_block_the_real_one(self):
        self.commit()
        prefix, version = self.build()
        staged = json.loads(self.staging.read(f"{prefix}/{common.STAGED}"))
        self.forge_staged("branches/main/9.9.9-alpha.1", staged,
                          version="9.9.9-alpha.1", filesafe="9.9.9-alpha.1")
        with self.assertRaises(promote_mod.PromoteIncomplete) as caught:
            self.promote()
        self.assertEqual([d["version"] for d in caught.exception.done],
                         [version])

    def test_a_branch_build_at_its_release_tag_is_never_a_nightly(self):
        self.git("checkout", "-q", "-b", "release/2.2", "v2.2.4")
        _, version = self.build(ref="refs/heads/release/2.2")
        self.assertEqual(version, "2.2.4")
        err = io.StringIO()
        with contextlib.redirect_stderr(err):
            self.assertEqual(self.promote(), [])
        self.assertIn("final version", err.getvalue())
        self.assertFalse(self.builds.exists(f"nightly/2.2.4/{common.DESCRIPTOR}"))

    def test_a_withdrawn_nightly_is_never_named(self):
        self.commit()
        _, version = self.build()
        self.promote()
        self.index()
        # The index as a signing host that missed the withdrawal would sign
        # it: verify refuses a channel naming a withdrawn version.
        path = self.tmp / "update" / self.slug / common.INDEX
        tampered = json.loads(path.read_text())
        tampered["withdrawn"] = [{"version": version, "reason": "broken"}]
        path.write_bytes(common.canonical_json(tampered))
        sig = minisig.sign(self.idx_key, path,
                           f"product={self.slug} serial={tampered['serial']}")
        self.assertEqual(sig, Path(f"{path}.minisig"))
        self.assert_refused(self.check, "withdrawn")
        # And the host itself never names it.
        self.product.policy.setdefault("withdrawn", []).append(
            {"version": version, "reason": "broken"})
        self.assertIsNone(self.index(force=True)["channels"]["nightly"])
        self.check()

    def test_no_shape_of_a_nightly_descriptor_is_a_traceback(self):
        self.commit()
        self.build()
        (d,) = self.promote()
        path = layout.descriptor_of(layout.nightly_dir(d["version"]))
        work = self.tmp / "admit"
        work.mkdir()
        assert_only_release_errors(
            self, lambda raw: index_mod.admit_nightly(
                self.product, self.builds, path, raw, work,
                git_dir=self.repo, cosign=str(self.cosign)), d)

    def test_poisoned_descriptors_leave_the_good_nightly_published(self):
        self.commit()
        _, old = self.build()
        (first,) = self.promote()
        self.commit()
        _, version = self.build()
        self.promote()
        # The older nightly's promotion time garbled, and two descriptors
        # whose fields have the wrong types.
        prefix = layout.nightly_dir(first["version"])
        self.rewrite(dict(first, promoted_at="yesterday"),
                     f"{prefix}/{common.DESCRIPTOR}")
        base = {"schema": 1, "product": self.slug, "kind": "nightly",
                "version": "9.9.9-alpha.1", "ref": "refs/heads/main",
                "artifacts": []}
        for name, d in (("9.9.9-alpha.1", dict(base, signing="x")),
                        ("9.9.9-alpha.2", dict(base, ref=5))):
            self.builds.put_bytes(json.dumps(d).encode(),
                                  f"nightly/{name}/{common.DESCRIPTOR}")
        with self.assertRaises(index_mod.IndexIncomplete) as caught:
            self.index()
        self.assertEqual(len(caught.exception.refused), 3)
        self.assertEqual(caught.exception.index["channels"]["nightly"],
                         version)
        self.assertNotEqual(old, version)

    # --- what admitting a nightly checks again -------------------------

    def stored(self, path):
        f = self.tmp / "builds" / self.slug / path
        f.chmod(0o644)
        return f

    def rewrite(self, descriptor, path):
        self.stored(path).write_bytes(common.canonical_json(descriptor))

    def assert_no_nightly(self):
        with self.assertRaises(index_mod.IndexIncomplete) as caught:
            self.index(force=True)
        self.assertIsNone(caught.exception.index["nightly"])
        return caught.exception

    def test_a_descriptor_other_than_the_signed_manifest_is_refused(self):
        self.commit()
        self.build()
        (d,) = self.promote()
        prefix = layout.nightly_dir(d["version"])
        first = d["artifacts"][0]
        self.stored(first["path"]).write_bytes(b"replaced")
        digest = common.sha256_bytes(b"replaced")
        changed = [dict(first, sha256=digest, build_sha256=digest)] + \
            d["artifacts"][1:]
        self.rewrite(dict(d, artifacts=changed),
                     f"{prefix}/{common.DESCRIPTOR}")
        self.assertIn("signed manifest", str(self.assert_no_nightly()))

    def test_a_name_listed_twice_is_refused(self):
        self.commit()
        self.build()
        (d,) = self.promote()
        prefix = layout.nightly_dir(d["version"])
        first = dict(d["artifacts"][0], sha256="0" * 64, build_sha256="0" * 64,
                     path=f"{prefix}/elsewhere.bin")
        self.rewrite(dict(d, artifacts=[first] + d["artifacts"]),
                     f"{prefix}/{common.DESCRIPTOR}")
        self.assertIn("twice", str(self.assert_no_nightly()))

    def test_an_artifact_the_rules_call_otherwise_is_refused(self):
        self.commit()
        self.build()
        (d,) = self.promote()
        prefix = layout.nightly_dir(d["version"])
        lied = [dict(a, os="linux") if a["kind"] == "msi" else a
                for a in d["artifacts"]]
        self.rewrite(dict(d, artifacts=lied), f"{prefix}/{common.DESCRIPTOR}")
        self.assertIn("artifact rules", str(self.assert_no_nightly()))

    def test_a_nightly_forged_in_the_builds_bucket_is_never_listed(self):
        """Whoever can write the builds bucket, but is not the host: a
        descriptor of a higher version, with a genuine build's Sigstore
        bundle and manifest, and with files of its own."""
        self.commit()
        self.build()
        (d,) = self.promote()
        version = "2.9.0-alpha.1+g" + "a" * 12
        prefix = layout.nightly_dir(version)
        # The genuine bundle and manifest, relabelled: Sigstore still holds,
        # git does not call that commit 2.9.0-alpha.1.
        build = {k: f"{prefix}/build/{k}"
                 for k in (common.SUMS, common.BUNDLE)}
        for name, path in build.items():
            self.builds.put_bytes(self.builds.read(d["build"][
                "sums" if name == common.SUMS else "bundle"]), path)
        artifacts = []
        for a in d["artifacts"]:
            name = a["name"].replace(
                common.parse_version(d["version"]).filesafe,
                common.parse_version(version).filesafe)
            artifacts.append(dict(a, name=name, path=f"{prefix}/{name}"))
            self.builds.put_bytes(self.builds.read(a["path"]),
                                  f"{prefix}/{name}")
        forged = dict(d, version=version, artifacts=artifacts,
                      build=dict(d["build"], sums=build[common.SUMS],
                                 bundle=build[common.BUNDLE]))
        self.builds.put_bytes(common.canonical_json(forged),
                              f"{prefix}/{common.DESCRIPTOR}")
        with self.assertRaises(index_mod.IndexIncomplete) as caught:
            self.index()
        self.assertEqual(caught.exception.index["nightly"]["version"],
                         d["version"])
        self.assertIn(prefix, "\n".join(caught.exception.refused))


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

    def test_output_that_is_not_utf8_is_no_traceback(self):
        out = common.run([sys.executable, "-c",
                          "import sys; sys.stdout.buffer.write(b'ok \\xff')"])
        self.assertTrue(out.stdout.startswith("ok "))

    def test_a_minisig_with_a_garbled_untrusted_comment_is_read(self):
        sig = (b"untrusted comment: \xff\xfe\nRWQ=\n"
               b"trusted comment: product=x file=y version=1\nAAAA\n")
        self.assertEqual(minisig.trusted_comment(sig),
                         "product=x file=y version=1")
        self.assertRaises(common.ReleaseError, minisig.trusted_comment,
                          b"untrusted comment: x\nRWQ=\n"
                          b"trusted comment: \xff\nAAAA\n")

    def test_no_shape_of_staged_json_is_a_traceback(self):
        staged = {"schema": 1, "product": "gpgfrontend",
                  "ref": "refs/heads/main", "commit": "a" * 40,
                  "version": "2.3.0-alpha.1+gaaaaaaaaaaaa",
                  "filesafe": "2.3.0-alpha.1-gaaaaaaaaaaaa",
                  "built_at": "2026-11-06T12:00:00Z",
                  "files": ["GpgFrontend-2.3.0-alpha.1-gaaaaaaaaaaaa-macos-15.dmg"]}
        stage_mod.read_staged(json.dumps(staged).encode())
        for label, m in mutations(staged):
            with self.subTest(label):
                try:
                    staged_ok = stage_mod.read_staged(json.dumps(m).encode())
                except common.ReleaseError:
                    continue
                # What passes is what promote then relies on.
                promote_mod.by_ref_newest_first([("p", staged_ok)])
                promote_mod.final_prefix(staged_ok)
                self.assertTrue(staged_ok["ref"].startswith("refs/"))

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

    def ring(self, expiry=None, revoked=False):
        ring = openpgp.Keyring.__new__(openpgp.Keyring)
        ring.pinned = {self.PINNED}
        ring._key_state = lambda fpr: (expiry, revoked)
        return ring

    def accept(self, ring, status):
        return ring._accept(status, "f")["primary"]

    def status(self, judgement, fpr, created=1794134474):
        return (f"[GNUPG:] NEWSIG x\n[GNUPG:] {judgement} 852C77E84E7E7DBC u\n"
                f"[GNUPG:] VALIDSIG {fpr} 2026-11-08 {created} 0 4 0 22 10 00 "
                f"{fpr}\n")

    def test_good(self):
        self.assertEqual(self.accept(self.ring(),
                                     self.status("GOODSIG", self.PINNED)),
                         self.PINNED)

    def test_revoked_is_refused(self):
        with self.assertRaisesRegex(common.ReleaseError, "revoked"):
            self.ring()._accept(self.status("REVKEYSIG", self.PINNED), "f")

    def test_expired_counts_only_for_what_it_signed_in_time(self):
        before = self.status("EXPKEYSIG", self.PINNED, created=100)
        self.assertEqual(self.accept(self.ring(expiry=200), before),
                         self.PINNED)
        with self.assertRaisesRegex(common.ReleaseError, "revoked"):
            self.ring(expiry=200, revoked=True)._accept(before, "f")
        after = self.status("EXPKEYSIG", self.PINNED, created=300)
        with self.assertRaisesRegex(common.ReleaseError, "after its key"):
            self.ring(expiry=200)._accept(after, "f")

    def test_an_expired_signature_is_refused(self):
        with self.assertRaisesRegex(common.ReleaseError, "EXPSIG"):
            self.ring()._accept(self.status("EXPSIG", self.PINNED), "f")

    def test_every_signature_is_looked_at(self):
        both = self.status("GOODSIG", self.OTHER) + \
            self.status("GOODSIG", self.PINNED)
        self.assertEqual(self.accept(self.ring(), both), self.PINNED)

    def test_none_valid(self):
        with self.assertRaisesRegex(common.ReleaseError, "not a pinned"):
            self.ring()._accept(self.status("GOODSIG", self.OTHER), "f")
        with self.assertRaisesRegex(common.ReleaseError, "no valid signature"):
            self.ring()._accept("[GNUPG:] NEWSIG\n[GNUPG:] BADSIG x u\n", "f")


@unittest.skipUnless(tools("gpg", "git"), "needs gpg and git")
class OpenPgpKeyLifecycleTest(unittest.TestCase):
    """Real keys whose life is played out on a faked clock: made in 2020,
    expired in 2021, signing in between."""

    MADE, SIGNED = "20200101T000000", "20200601T000000"

    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp(prefix="gfr-life-"))
        self.home = self.tmp / "g"
        self.home.mkdir(mode=0o700)
        self.gpg("--faked-system-time", self.MADE, "--passphrase", "",
                 "--quick-gen-key", "Old Maintainer <old@example.org>",
                 "ed25519", "sign", "1y")
        listing = self.gpg("--with-colons", "--list-keys", "old@example.org")
        self.fpr = [l.split(":")[9] for l in listing.splitlines()
                    if l.startswith("fpr:")][0]
        self.keys = self.tmp / "maintainers.asc"
        self.export()
        # git signs through gpg on the faked clock.
        self.wrapper = self.tmp / "gpg-then"
        self.wrapper.write_text(f"#!/bin/sh\nexec gpg --homedir {self.home} "
                                f"--faked-system-time {self.SIGNED} \"$@\"\n")
        self.wrapper.chmod(0o755)
        self.repo = self.tmp / "repo"
        self.repo.mkdir()
        self.git("init", "-q", "-b", "main")
        self.git("commit", "-q", "--allow-empty", "-m", "c")
        self.git("-c", f"gpg.program={self.wrapper}", "-c",
                 f"user.signingkey={self.fpr}", "tag", "-s", "v1.0.0",
                 "-m", "signed in time")
        self.data = self.tmp / "data"
        self.data.write_bytes(b"payload\n")
        self.gpg("--faked-system-time", self.SIGNED, "--local-user",
                 f"{self.fpr}!", "--detach-sign", "--output",
                 f"{self.data}.sig", self.data)

    def tearDown(self):
        subprocess.run(["gpgconf", "--homedir", self.home, "--kill", "all"],
                       capture_output=True)
        shutil.rmtree(self.tmp, ignore_errors=True)

    def gpg(self, *args):
        return subprocess.run(["gpg", "--homedir", self.home, "--batch",
                               *args], check=True, capture_output=True,
                              text=True).stdout

    def git(self, *args):
        return subprocess.run(["git", *args], cwd=self.repo, env=git_env(),
                              check=True, capture_output=True, text=True)

    def export(self):
        self.keys.write_text(self.gpg("--armor", "--export", self.fpr))

    def revoke(self):
        cert = self.home / "openpgp-revocs.d" / f"{self.fpr}.rev"
        revocation = self.tmp / "revoke.asc"
        revocation.write_text(cert.read_text().replace(":-----BEGIN",
                                                       "-----BEGIN"))
        self.gpg("--import", str(revocation))
        self.export()

    def test_a_file_signed_before_the_key_expired_is_accepted(self):
        with openpgp.Keyring(self.keys, [self.fpr]) as ring:
            self.assertEqual(ring.verify(self.data, Path(f"{self.data}.sig")),
                             self.fpr)

    def test_a_tag_signed_before_the_key_expired_is_accepted(self):
        with openpgp.Keyring(self.keys, [self.fpr]) as ring:
            self.assertEqual(ring.verify_tag(self.repo, "v1.0.0"), self.fpr)

    def test_an_expired_and_revoked_key_signs_nothing(self):
        self.revoke()
        with openpgp.Keyring(self.keys, [self.fpr]) as ring:
            with self.assertRaisesRegex(common.ReleaseError, "revoked"):
                ring.verify(self.data, Path(f"{self.data}.sig"))
            with self.assertRaisesRegex(common.ReleaseError, "revoked"):
                ring.verify_tag(self.repo, "v1.0.0")

    def test_the_callers_git_config_has_no_say(self):
        hostile = self.tmp / "gitconfig"
        hostile.write_text("[gpg]\n\tminTrustLevel = ultimate\n"
                           "\tprogram = /bin/false\n")
        saved = os.environ.get("GIT_CONFIG_GLOBAL")
        os.environ["GIT_CONFIG_GLOBAL"] = str(hostile)
        try:
            # A key that has not expired: only the config could refuse it.
            self.gpg("--quick-set-expire", self.fpr, "0")
            self.export()
            with openpgp.Keyring(self.keys, [self.fpr]) as ring:
                self.assertEqual(ring.verify_tag(self.repo, "v1.0.0"),
                                 self.fpr)
        finally:
            if saved is None:
                del os.environ["GIT_CONFIG_GLOBAL"]
            else:
                os.environ["GIT_CONFIG_GLOBAL"] = saved


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
        keys = self.tmp / "keys"
        keys.mkdir()
        self.idx_key, self.idx_pub = keypair(keys, "index")
        self.product = self.write_product(self.tmp / "product", [self.fpr])

        self.staging = Store(str(self.tmp / "staging"))
        self.builds = Store(str(self.tmp / "builds" / self.product.slug))
        self.update = Store(str(self.tmp / "update" / self.product.slug))
        self.state = self.tmp / "state"
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
                      exported=None, **policy):
        """A product whose root pins @p fingerprints (and @p retired), with
        @p policy over this test's host policy, and a key file holding
        @p exported (by default every one of them)."""
        product = make_product(directory, [self.idx_pub],
                               after=getattr(self, "product", None), **{
            "stable_pgp": {"fingerprints": list(fingerprints),
                           "fingerprints_retired": list(retired)},
            "authenticode_subject": SUBJECT,
            # A rehearsal certificate has no time-stamping service to use.
            "authenticode_timestamp": "optional", **policy})
        (directory / "maintainers.asc").write_text(self.gpg(
            "--armor", "--export",
            *(exported if exported is not None
              else [*fingerprints, *retired])))
        return product

    def git(self, *args):
        env = git_env()
        env["GNUPGHOME"] = str(self.gnupg)
        return subprocess.run(["git", "-c", f"user.signingkey={self.fpr}",
                               *args], cwd=self.repo, env=env, check=True,
                              capture_output=True, text=True).stdout.strip()

    def retag_later(self, version, signed):
        """v<version> made again on the same commit, dated long after every
        signature: what an older release's signatures look like to it."""
        env = git_env()
        env["GNUPGHOME"] = str(self.gnupg)
        env["GIT_COMMITTER_DATE"] = "2099-01-01T00:00:00+0000"
        subprocess.run(["git", "-c", f"user.signingkey={self.fpr}", "tag",
                        "-f", "-s" if signed else "-a", f"v{version}",
                        f"v{version}^{{commit}}", "-m", "dated later"],
                       cwd=self.repo, env=env, check=True,
                       capture_output=True)

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
            self.product, self.builds, self.update,
            index_pub=[self.idx_pub], index_key=self.idx_key,
            state_dir=self.state, force=True,
            git_dir=self.repo, cosign=str(self.cosign),
            authenticode_ca=str(self.cert))

    def check(self, deep=True):
        return verify_mod.verify(self.product, self.update, self.builds,
                                 deep=deep)

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
            self.product = self.write_product(
                self.tmp / f"p{len(retired)}", [self.fpr],
                authenticode_subject=subject,
                authenticode_subjects_retired=retired)

        # A new certificate under another name, and a host that checks every
        # release again (a lost state, a new machine): the old one retired,
        # the release the published index lists stays.
        shutil.rmtree(self.state / "admitted")
        pin("/CN=GpgFrontend New Signing/O=Rehearsal/C=DE", [SUBJECT])
        self.assertEqual(self.index()["releases"]["2.3.0"]["descriptor"],
                         "releases/2.3.0/release.json")
        # Not retired but dropped: it signed nothing that counts any more.
        pin("/CN=GpgFrontend New Signing/O=Rehearsal/C=DE", [])
        self.assert_refused(self.index, "signed by")

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

    # --- the OpenPGP gate ----------------------------------------------

    def tree(self):
        """Every object of the artifact store, by path, with its hash."""
        root = self.tmp / "builds" / self.product.slug
        return {str(f.relative_to(root)): common.sha256_file(f)
                for f in sorted(root.rglob("*")) if f.is_file()}

    def assert_no_marker(self):
        self.assertFalse(any(p.startswith("commits/") for p in self.tree()))

    def test_a_passing_release_gets_its_commit_marker(self):
        self.tag()
        self.build()
        descriptor = self.stage().run()
        self.index()
        self.assertTrue(self.builds.exists(f"commits/{descriptor['commit']}"))
        self.assertEqual([p for p in self.tree() if p.endswith(".minisig")],
                         [])
        self.check(deep=True)

    def test_any_missing_or_bad_signature_means_no_listing(self):
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
                self.assert_no_marker()

    def test_admitting_again_changes_nothing(self):
        self.tag()
        self.build()
        self.stage().run()
        self.index()
        before = self.tree()
        shutil.rmtree(self.state)  # admitted again from nothing
        self.index()
        self.assertEqual(self.tree(), before)

    def test_a_client_over_https_sees_the_stable_release(self):
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
                                   deep=True)
        self.assertEqual([c["version"] for c in report["checked"]], ["2.3.0"])
        self.assertEqual(report["channels"]["stable"], "2.3.0")

    # --- trust inputs --------------------------------------------------

    def admitted(self):
        self.tag()
        self.build()
        self.stage().run()
        self.assertIn("2.3.0", self.index()["releases"])

    def test_no_shape_of_a_stable_descriptor_is_a_traceback(self):
        self.tag()
        self.build()
        d = self.stage().run()
        path = f"releases/2.3.0/{common.DESCRIPTOR}"
        work = self.tmp / "admit"

        def admit(raw):
            shutil.rmtree(work, ignore_errors=True)
            work.mkdir()
            index_mod.admit_stable(
                self.product, self.builds, path, raw, git_dir=self.repo,
                state_dir=work / "state", cosign=str(self.cosign),
                tmp=work, authenticode_ca=str(self.cert))
        assert_only_release_errors(self, admit, d, values=(None, 5, {}))

    def test_signatures_made_before_the_tag_are_refused(self):
        self.admitted()
        entry = published(self.update)["releases"]["2.3.0"]
        self.assertEqual(set(entry["signatures"]),
                         {a["name"] for a in json.loads(self.builds.read(
                             entry["descriptor"]))["artifacts"]})
        self.retag_later("2.3.0", signed=True)
        self.assert_refused(self.index, "before its tag")

    def test_a_stable_release_relabelled_imported_is_refused(self):
        self.tag()
        self.build()
        d = self.stage().run()
        self.index()
        relabelled = {k: v for k, v in d.items()
                      if k not in ("sums", "sums_sig", "build")}
        relabelled["kind"] = "imported"
        path = self.tmp / "builds" / self.product.slug / "releases" / \
            "2.3.0" / common.DESCRIPTOR
        path.chmod(0o644)
        path.write_bytes(common.canonical_json(relabelled))
        self.assert_refused(self.index, "admitted as stable")

    def test_an_outage_keeps_a_listed_release_listed(self):
        self.admitted()
        before = published(self.update)
        flaky = Store(str(self.tmp / "builds" / self.product.slug),
                      rclone=flaky_rclone(self.tmp, "releases/2.3.0"))
        with self.assertRaises(index_mod.IndexIncomplete) as caught:
            index_mod.publish(
                self.product, flaky, self.update,
                index_pub=[self.idx_pub], index_key=self.idx_key,
                state_dir=self.tmp / "fresh-state", force=True,
                git_dir=self.repo, cosign=str(self.cosign),
                authenticode_ca=str(self.cert))
        self.assertIn("simulated outage", "\n".join(caught.exception.refused))
        index = caught.exception.index
        self.assertEqual(index["releases"]["2.3.0"],
                         before["releases"]["2.3.0"])
        self.assertEqual(index["channels"]["stable"], "2.3.0")

    def test_notes_only_with_unchanged_notes_is_a_no_op(self):
        self.tag()
        self.build()
        self.stage().run()
        # main's notes are still the tag's: nothing new to sign or upload,
        # and the stage's own signed copy is left as it is.
        sig = next((self.tmp / "work" / "2.3.0" / "notes").glob("*.md.sig"))
        before = sig.read_bytes()
        digest = self.stage().publish_notes()
        self.assertEqual(sig.read_bytes(), before)
        self.assertTrue(self.builds.exists(f"notes/2.3.0/{digest}.md.sig"))

    def rotate_maintainer(self):
        other = self.new_key("Next Maintainer <next@example.org>")
        self.product = self.write_product(self.tmp / "p2", [other],
                                          retired=[self.fpr])
        return other

    def test_a_resumed_upload_names_the_key_that_signed(self):
        self.tag()
        self.build()
        self.stage().run()
        state_path = self.tmp / "work" / "2.3.0" / "state.json"
        state = json.loads(state_path.read_text())
        state["done"].remove("upload")
        state_path.write_text(json.dumps(state))
        other = self.rotate_maintainer()
        # The default key is now the new maintainer's; the signatures are
        # the old one's, and the descriptor must say so.
        d = self.stage().run()
        self.assertEqual(d["signing"]["fingerprint"], self.fpr)
        # Never admitted before the key retired: a retired key's signatures
        # count only for releases that were.
        self.assert_refused(self.index, "not a pinned maintainer")
        # Naming another key than the one that signed is an error.
        self.assert_refused(lambda: self.stage(key=other).run(redo="upload"),
                            "signed with")

    def test_a_timestamp_is_required_where_the_product_says_so(self):
        required = self.write_product(self.tmp / "required", [self.fpr],
                                      authenticode_timestamp="required")
        self.tag()
        self.build()
        # The stage will not sign without a time-stamping service.
        self.assert_refused(
            lambda: stable_mod.Stage(required, "2.3.0", self.tmp / "work",
                                     staging=self.staging, builds=self.builds,
                                     git_dir=self.repo,
                                     cosign=str(self.cosign),
                                     gnupghome=str(self.gnupg),
                                     signer=authenticode.Signer(
                                         str(self.cert), str(self.cert_key)),
                                     authenticode_ca=str(self.cert)).run(),
            "timestamp")
        # Made without one (the product said optional), the host that
        # requires one refuses it.
        shutil.rmtree(self.tmp / "work")
        self.stage().run()
        self.product = required
        self.assert_refused(self.index, "time-stamp")

    def test_a_trust_change_keeps_the_admitted_notes(self):
        """main's notes edited and not signed yet when the key file changes
        (an expiry extended, a key added): the release stays listed with
        the notes it had, once they verify again."""
        self.admitted()
        first = published(self.update)["releases"]["2.3.0"]
        (self.repo / "release" / "notes" / "2.3.0.md").write_text(
            "## 2.3.0\n\n- Everything, signed. Fixed.\n")
        self.git("commit", "-qam", "notes: typo")
        other = self.new_key("Another Key <another@example.org>")
        self.product = self.write_product(self.tmp / "p2", [self.fpr],
                                          exported=[self.fpr, other])
        with contextlib.redirect_stderr(io.StringIO()):
            entry = self.index()["releases"]["2.3.0"]
        self.assertEqual(entry["notes_sha256"], first["notes_sha256"])
        self.check()

    def test_editing_the_key_file_never_unlists_a_release(self):
        """maintainers.asc is read from main unsigned: re-exported with a
        later expiry, with someone else's key added, or gone, it changes
        nothing for what was admitted, nor for its notes."""
        self.admitted()
        before = self.index()["releases"]["2.3.0"]
        other = self.new_key("Someone Else <else@example.org>")
        for name, exported in (("an unpinned key added", [self.fpr, other]),
                               ("the pinned key dropped", [other]),
                               ("nothing left", [])):
            with self.subTest(name):
                product = self.tmp / "product"
                (product / "maintainers.asc").write_text(
                    self.gpg("--armor", "--export", *exported)
                    if exported else "")
                self.assertEqual(self.index()["releases"]["2.3.0"], before)
        (self.tmp / "product" / "maintainers.asc").unlink()
        self.assertEqual(self.index()["releases"]["2.3.0"], before)

    def release_2_3_1(self):
        """2.3.1, made and uploaded, not yet admitted."""
        (self.repo / "release" / "notes" / "2.3.1.md").write_text("2.3.1\n")
        self.git("add", "release")
        self.git("commit", "-q", "-m", "notes for 2.3.1")
        self.tag("2.3.1")
        self.build("2.3.1")
        self.stage("2.3.1").run()

    def test_a_key_file_that_cannot_verify_refuses_only_new_releases(self):
        self.admitted()
        self.release_2_3_1()
        (self.tmp / "product" / "maintainers.asc").unlink()
        with self.assertRaises(index_mod.IndexIncomplete) as caught:
            self.index()
        self.assertIn("2.3.0", caught.exception.index["releases"])
        self.assertFalse(caught.exception.passing)  # PARTIAL, not FAILED

    def test_notes_main_changed_keep_the_admitted_ones_when_they_fail(self):
        self.admitted()
        before = published(self.update)["releases"]["2.3.0"]
        (self.repo / "release" / "notes" / "2.3.0.md").write_text(
            "## 2.3.0\n\n- Everything, signed. Fixed.\n")
        self.git("commit", "-qam", "notes: typo")
        self.stage().publish_notes()
        # Signed by the maintainer, then the key file loses their key.
        (self.tmp / "product" / "maintainers.asc").write_text("")
        with self.assertRaises(index_mod.IndexIncomplete) as caught:
            self.index()
        self.assertEqual(caught.exception.index["releases"]["2.3.0"]
                         ["notes_sha256"], before["notes_sha256"])
        self.assertIn("listed with its admitted notes",
                      "\n".join(caught.exception.refused))

    def test_renewing_the_root_checks_nothing_again(self):
        self.admitted()
        record = (self.state / "admitted" / "2.3.0.json").read_text()
        root_dir = self.product.root_dir
        doc = json.loads((root_dir / "1.root.json").read_text())
        path = root_dir / "2.root.json"
        path.write_bytes(trust.dump(trust.renewed(doc, trust.ROOT_LIFETIME)))
        for name in ("root-a", "root-b"):
            minisig.sign(TRUST_KEYS / f"{name}.key", path,
                         layout.root_comment("gpgfrontend", 2),
                         root_dir / layout.root_sig_name(
                             2, minisig.key_id(minisig.public_key_of(
                                 TRUST_KEYS / f"{name}.pub"))))
        self.product = common.Product(
            self.tmp / "product" / "product.json",
            self.tmp / "product" / "policy.toml", root_dir=root_dir,
            index_keys_dir=self.product.index_keys_dir)
        self.assertEqual(self.product.trust[0]["version"], 2)
        # Nothing downloaded again: the cert's CA is not even given.
        self.cert.unlink()
        self.assertIn("2.3.0", self.index()["releases"])
        self.assertEqual((self.state / "admitted" / "2.3.0.json")
                         .read_text(), record)

    def test_an_unpinned_key_unlists_its_releases(self):
        self.admitted()
        other = self.new_key("Someone Else <else@example.org>")
        self.product = self.write_product(self.tmp / "p2", [other])
        self.assert_refused(self.index, "not signed with a pinned maintainer")

    def test_a_retired_key_counts_only_for_releases_admitted_before(self):
        self.admitted()
        # 2.3.1, signed by the same key, not admitted yet when it retires.
        self.release_2_3_1()
        other = self.new_key("Next Maintainer <next@example.org>")
        self.product = self.write_product(self.tmp / "p2", [other],
                                          retired=[self.fpr])
        with self.assertRaises(index_mod.IndexIncomplete) as caught:
            self.index()
        self.assertIn("2.3.0", caught.exception.index["releases"])
        self.assertNotIn("2.3.1", caught.exception.index["releases"])
        self.assertIn("not a pinned maintainer",
                      "\n".join(caught.exception.refused))

    def test_a_retired_key_keeps_its_releases(self):
        self.admitted()
        other = self.new_key("Next Maintainer <next@example.org>")
        self.product = self.write_product(self.tmp / "p2", [other],
                                          retired=[self.fpr])
        self.assertIn("2.3.0", self.index()["releases"])
        # Never to sign anything new, though: not another release, not new
        # notes for this one.
        self.assert_refused(lambda: self.stage("2.3.1", key=self.fpr),
                            "not a current pinned")
        (self.repo / "release" / "notes" / "2.3.0.md").write_text("New.\n")
        self.git("commit", "-qam", "notes: new")
        self.assert_refused(lambda: self.stage(key=self.fpr).publish_notes(),
                            "not a current pinned")

    def test_a_tag_moved_after_admission_is_caught(self):
        self.admitted()
        self.git("commit", "-q", "--allow-empty", "-m", "later")
        self.git("tag", "-f", "-s", "v2.3.0", "-m", "moved")
        self.assert_refused(self.index, r"v2\.3\.0 is [0-9a-f]+, not")

    def test_a_revoked_key_unlists_its_releases_once_the_root_says_so(self):
        self.admitted()
        cert = self.gnupg / "openpgp-revocs.d" / f"{self.fpr}.rev"
        revocation = self.tmp / "revoke.asc"
        revocation.write_text(cert.read_text().replace(":-----BEGIN",
                                                       "-----BEGIN"))
        self.gpg("--import", str(revocation))
        # The key file carries the revocation: alone, it unlists nothing.
        (self.tmp / "product" / "maintainers.asc").write_text(self.gpg(
            "--armor", "--export", self.fpr))
        self.assertIn("2.3.0", self.index()["releases"])
        # A new root (here: any change of its host policy) has everything
        # checked again, and gpg's REVKEYSIG is refused.
        self.product = self.write_product(
            self.tmp / "p2", [self.fpr],
            authenticode_subjects_retired=["/CN=An Old Certificate"])
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
    retag_later = StableTest.retag_later
    new_key = StableTest.new_key
    write_product = StableTest.write_product
    git = StableTest.git
    assert_refused = StableTest.assert_refused

    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp(prefix="gfr-import-"))
        self.gnupg = self.tmp / "gnupg"
        self.gnupg.mkdir(mode=0o700)
        self.fpr = self.new_key("Test Maintainer <maintainer@example.org>")
        keys = self.tmp / "keys"
        keys.mkdir()
        self.idx_key, self.idx_pub = keypair(keys, "index")
        self.product = self.write_product(self.tmp / "product", [self.fpr])
        self.builds = Store(str(self.tmp / "builds" / self.product.slug))
        self.update = Store(str(self.tmp / "update" / self.product.slug))
        self.state = self.tmp / "state"
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
            self.product, self.builds, self.update,
            index_pub=[self.idx_pub], index_key=self.idx_key,
            state_dir=self.state, force=True, git_dir=self.repo)

    def check(self):
        return verify_mod.verify(self.product, self.update, self.builds,
                                 deep=True)

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

    def test_an_imported_release_gets_its_commit_marker(self):
        self.publish_on_github("2.2.4", self.LEGS)
        _, d = self.run_import("2.2.4")
        self.assertIn("2.2.4", self.index()["releases"])
        self.assertTrue(self.builds.exists(f"commits/{d['commit']}"))
        self.check()

    def test_no_shape_of_an_imported_descriptor_is_a_traceback(self):
        self.publish_on_github("2.2.4", self.LEGS)
        _, d = self.run_import("2.2.4")
        path = f"releases/2.2.4/{common.DESCRIPTOR}"
        work = self.tmp / "admit"

        def admit(raw):
            shutil.rmtree(work, ignore_errors=True)
            work.mkdir()
            index_mod.admit_imported(
                self.product, self.builds, path, raw, git_dir=self.repo,
                state_dir=work / "state", tmp=work)
        assert_only_release_errors(self, admit, d, values=(None, 5, {}))

    def test_files_signed_before_their_tag_are_refused(self):
        self.publish_on_github("2.2.4", self.LEGS)
        self.run_import("2.2.4")
        self.index()
        # An older release's files and signatures, relabelled: they were
        # signed before this version's tag existed.
        self.retag_later("2.2.4", signed=False)
        self.assert_refused(self.index, "before its tag")

    def test_a_hotfix_signed_later_is_admitted_and_an_older_one_never_again(
            self):
        import time
        self.publish_on_github("2.2.4", self.LEGS)
        _, d = self.run_import("2.2.4")
        first = self.index()["releases"]["2.2.4"]
        a = next(x for x in d["artifacts"] if x["name"].endswith("15.dmg"))
        fixed = self.tmp / a["name"]
        fixed.write_bytes(b"hotfix\n" * 100)
        self.gpg("--faked-system-time", str(int(time.time()) + 3600),
                 "--local-user", f"{self.fpr}!", "--detach-sign",
                 "--output", f"{fixed}.sig", fixed)
        # A new file never replaces a published one: it gets its own path.
        hot = f"releases/2.2.4/hotfix-1/{a['name']}"
        self.builds.put_file(fixed, hot)
        self.builds.put_file(Path(f"{fixed}.sig"), hot + ".sig")
        hotfixed = dict(d, artifacts=[
            dict(x, sha256=common.sha256_file(fixed),
                 size=fixed.stat().st_size, path=hot, sig=hot + ".sig")
            if x is a else x for x in d["artifacts"]])
        path = self.tmp / "builds" / self.product.slug / "releases" / \
            "2.2.4" / common.DESCRIPTOR
        path.chmod(0o644)
        path.write_bytes(common.canonical_json(hotfixed))

        entry = self.index()["releases"]["2.2.4"]
        self.assertGreater(
            common.parse_iso(entry["signatures"][a["name"]]["created"]),
            common.parse_iso(first["signatures"][a["name"]]["created"]))
        history = (self.state / "admitted" / "2.2.4.history.jsonl") \
            .read_text().splitlines()
        self.assertEqual(len(history), 2)
        self.assertEqual(json.loads(history[-1])["signatures"][a["name"]]
                         ["sha256"], common.sha256_file(fixed))
        self.check()

        # The build it replaced, offered again: its signature is older than
        # the one admitted since, so it is a replay.
        path.write_bytes(common.canonical_json(d))
        self.assert_refused(self.index, "older than")

    def test_a_manifest_path_in_an_imported_descriptor_is_refused(self):
        """An imported release has no manifest of its own: one named in its
        descriptor is not what its files were checked against."""
        self.publish_on_github("2.2.4", self.LEGS)
        _, d = self.run_import("2.2.4")
        planted = "nightly/9.9.9-alpha.1/SHA256SUMS.txt"
        self.builds.put_bytes(b"%s  GpgFrontend-9.9.9-alpha.1-macos-15.dmg\n"
                              % (b"0" * 64), planted)
        root = self.tmp / "builds" / self.product.slug
        path = root / "releases" / "2.2.4" / common.DESCRIPTOR
        path.chmod(0o644)
        path.write_bytes(common.canonical_json(dict(d, sums=planted)))
        with self.assertRaises(index_mod.IndexIncomplete) as caught:
            self.index()
        self.assertNotIn("2.2.4", caught.exception.index["releases"])

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

