# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""The update trust (scripts/release/gfrelease/trust.py): the root chain,
the index-keys list, and the commands that make and sign them.

Real minisign throughout, on the throwaway files of fixtures/trust/ and on
keys made fresh for each test."""

from __future__ import annotations

import json
import os
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

HERE = Path(__file__).resolve().parent
GFRELEASE = HERE.parent / "release" / "gfrelease"
sys.path.insert(0, str(GFRELEASE))

import common  # noqa: E402
import layout  # noqa: E402
import minisig  # noqa: E402
import trust  # noqa: E402

FIXTURE = HERE / "fixtures" / "trust"
SLUG = "gpgfrontend"


def fixture_key(name: str) -> tuple[Path, str, str]:
    """(secret key file, public key, key id) of a fixture key."""
    pub = minisig.public_key_of(FIXTURE / "keys" / f"{name}.pub")
    return FIXTURE / "keys" / f"{name}.key", pub, minisig.key_id(pub)


def new_key(directory: Path, name: str) -> tuple[Path, str, str]:
    secret, pub = directory / f"{name}.key", directory / f"{name}.pub"
    subprocess.run(["minisign", "-G", "-W", "-s", secret, "-p", pub],
                   check=True, capture_output=True)
    key = minisig.public_key_of(pub)
    return secret, key, minisig.key_id(key)


def cli(*args: str, check: bool = True) -> subprocess.CompletedProcess:
    out = subprocess.run([sys.executable, str(GFRELEASE), *args],
                         capture_output=True, text=True)
    if check and out.returncode != 0:
        raise AssertionError(out.stderr)
    return out


@unittest.skipUnless(shutil.which("minisign"), "needs minisign")
class TrustTest(unittest.TestCase):

    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp(prefix="gfr-trust-test-"))
        self.root = self.tmp / "root"
        self.keys = self.tmp / "index-keys"
        shutil.copytree(FIXTURE / "root", self.root)
        shutil.copytree(FIXTURE / "index-keys", self.keys)

    def tearDown(self):
        shutil.rmtree(self.tmp, ignore_errors=True)

    def walk(self, **kw):
        return trust.walk(trust.read_dir(self.root), SLUG, steps=None, **kw)

    def sig(self, n: int, name: str) -> Path:
        return self.root / layout.root_sig_name(n, fixture_key(name)[2])

    def write_root(self, n: int, doc: dict, signers) -> Path:
        path = self.root / layout.root_name(n)
        path.write_bytes(trust.dump(doc))
        for secret in signers:
            minisig.sign(secret, path, layout.root_comment(SLUG, n),
                         self.tmp / "sig")
            key_id = minisig.prehashed_key_id((self.tmp / "sig").read_bytes())
            (self.tmp / "sig").replace(self.root /
                                       layout.root_sig_name(n, key_id))
        return path

    # --- the root chain -------------------------------------------------

    def test_the_fixture_chain_walks_from_one_to_three(self):
        root, keys = trust.load_trust(self.root, self.keys, SLUG)
        self.assertEqual((root["version"], keys["version"]), (3, 2))

    def test_below_the_threshold_is_refused(self):
        # Root 2 keeps a, b and d's signatures; root 1 (a, b, c; 2 needed)
        # then has only b's.
        self.sig(2, "root-a").unlink()
        with self.assertRaisesRegex(common.ReleaseError,
                                    "1 of root 1's root keys"):
            self.walk()

    def test_a_root_signed_only_by_its_own_new_keys_is_refused(self):
        root1 = json.loads((self.root / "1.root.json").read_text())
        for path in self.root.glob("[23].*"):
            path.unlink()
        new = [new_key(self.tmp, n) for n in ("x", "y")]
        root2 = dict(root1, version=2, keys=dict(
            {k[2]: k[1] for k in new},
            **{i: root1["keys"][i]
               for i in root1["roles"]["index_list"]["keyids"]}))
        root2["roles"] = dict(root1["roles"], root={
            "keyids": [k[2] for k in new], "threshold": 2})
        self.write_root(2, root2, [k[0] for k in new])
        with self.assertRaisesRegex(common.ReleaseError,
                                    "0 of root 1's root keys"):
            self.walk()

    def test_a_skipped_version_is_never_taken(self):
        for path in self.root.glob("2.*"):
            path.unlink()
        self.assertEqual(self.walk()["version"], 1)
        root1 = self.walk()
        data3 = (self.root / "3.root.json").read_bytes()
        with self.assertRaisesRegex(common.ReleaseError, "version 3, not 2"):
            trust.accept_root(root1, data3, lambda _: None, SLUG, "3")

    def test_a_lower_version_is_never_taken(self):
        root3 = self.walk()
        data2 = (self.root / "2.root.json").read_bytes()
        read = trust.read_dir(self.root)
        with self.assertRaisesRegex(common.ReleaseError, "version 2, not 4"):
            trust.accept_root(root3, data2,
                              lambda i: read(layout.root_sig_name(2, i)),
                              SLUG, "2")

    def test_garbage_beside_enough_good_signatures_is_ignored(self):
        # c is a root key of root 1 that did not sign root 2.
        self.sig(2, "root-c").write_text("garbage\n")
        self.assertEqual(self.walk()["version"], 3)

    def test_a_signature_for_another_root_does_not_count(self):
        path = self.root / "2.root.json"
        minisig.sign(fixture_key("root-a")[0], path,
                     layout.root_comment(SLUG, 3), self.sig(2, "root-a"))
        with self.assertRaisesRegex(common.ReleaseError, "1 of root 1's"):
            self.walk()

    def test_a_legacy_signature_does_not_count(self):
        subprocess.run(["minisign", "-S", "-l", "-s",
                        fixture_key("root-a")[0], "-m",
                        self.root / "2.root.json", "-x",
                        self.sig(2, "root-a"), "-t",
                        layout.root_comment(SLUG, 2)],
                       check=True, capture_output=True, input=b"")
        with self.assertRaisesRegex(common.ReleaseError, "1 of root 1's"):
            self.walk()

    def test_the_walk_stops_after_its_steps(self):
        root1 = trust.walk(trust.read_dir(self.root), SLUG, steps=1)
        self.assertEqual(root1["version"], 1)
        root2 = trust.walk(trust.read_dir(self.root), SLUG, root1, steps=1)
        self.assertEqual(root2["version"], 2)
        self.assertEqual(trust.MAX_STEPS, 32)

    def test_the_first_root_must_be_the_anchor(self):
        anchor = common.sha256_file(self.root / "1.root.json")
        self.assertEqual(self.walk(anchor=anchor)["version"], 3)
        with self.assertRaisesRegex(common.ReleaseError, "not 0+"):
            self.walk(anchor="0" * 64)

    def test_the_index_list_key_is_never_a_root_key(self):
        doc = json.loads((self.root / "1.root.json").read_text())
        doc["roles"]["index_list"]["keyids"] = [
            doc["roles"]["root"]["keyids"][0]]
        with self.assertRaisesRegex(common.ReleaseError, "also a root key"):
            trust.load(json.dumps(doc).encode(), "1", trust.ROOT, SLUG)

    def test_a_key_listed_under_another_id_is_refused(self):
        doc = json.loads((self.root / "1.root.json").read_text())
        ids = list(doc["keys"])
        doc["keys"][ids[0]], doc["keys"][ids[1]] = \
            doc["keys"][ids[1]], doc["keys"][ids[0]]
        with self.assertRaisesRegex(common.ReleaseError, "not the key"):
            trust.load(json.dumps(doc).encode(), "1", trust.ROOT, SLUG)

    def test_only_the_newest_root_must_be_unexpired(self):
        doc = json.loads((self.root / "3.root.json").read_text())
        self.assertIsNone(trust.check_unexpired(doc, "root 3"))
        doc["expires"] = "2020-01-01T00:00:00Z"
        with self.assertRaisesRegex(common.ReleaseError, "expired"):
            trust.check_unexpired(doc, "root 3")

    # --- the index-keys list --------------------------------------------

    def accept_list(self, root, v, floor=0):
        name = layout.index_keys_name(v)
        read = trust.read_dir(self.keys)
        return trust.accept_index_keys(root, read(name),
                                       read(name + common.MINISIG), SLUG,
                                       name, floor)

    def test_a_list_signed_by_another_key_than_g_is_refused(self):
        root3 = self.walk()
        path = self.keys / "2.json"
        minisig.sign(fixture_key("root-a")[0], path,
                     layout.index_keys_comment(SLUG, 2))
        with self.assertRaisesRegex(common.ReleaseError,
                                    "not signed by root 3's"):
            self.accept_list(root3, 2)

    def test_a_new_g_does_not_take_the_old_ones_list(self):
        root3 = self.walk()
        self.assertEqual(self.accept_list(root3, 2)["version"], 2)
        # List 1 was signed by g1, which root 3 replaced with g2.
        with self.assertRaisesRegex(common.ReleaseError, "not signed by"):
            self.accept_list(root3, 1)

    def test_a_list_older_than_the_floor_is_refused(self):
        with self.assertRaisesRegex(common.ReleaseError, "older than 3"):
            self.accept_list(self.walk(), 2, floor=3)

    def test_an_expired_list_is_refused_by_whoever_checks_it(self):
        keys = self.accept_list(self.walk(), 2)
        trust.check_unexpired(keys, "index-keys 2")
        with self.assertRaisesRegex(common.ReleaseError, "expired"):
            trust.check_unexpired(keys, "index-keys 2",
                                  now=common.parse_iso(
                                      "2100-01-02T00:00:00Z"))

    # --- the commands ---------------------------------------------------

    def args(self):
        return ["--root-dir", str(self.root), "--index-keys-dir",
                str(self.keys)]

    def test_root_new_then_sign_then_verify(self):
        out = cli(*self.args(), "root", "new")
        self.assertIn("4.root.json: written", out.stdout)
        path = self.root / "4.root.json"
        new = json.loads(path.read_text())
        self.assertEqual(new["version"], 4)
        self.assertGreater(trust.days_left(new), 700)
        # Not yet signed: verify says so.
        self.assertNotEqual(cli(*self.args(), "root", "verify",
                                check=False).returncode, 0)
        for name in ("root-a", "root-d"):
            cli("root", "sign", str(path), "-s",
                str(fixture_key(name)[0]))
        out = cli(*self.args(), "root", "verify")
        self.assertIn("root 4: expires", out.stdout)
        self.assertIn("index-keys 2: expires", out.stdout)

    def test_verify_from_a_later_root(self):
        out = cli(*self.args(), "root", "verify", "--from", "2")
        self.assertIn("root 3", out.stdout)

    def test_renew_reads_g_once_from_a_process_substitution(self):
        """The real path: bash's <(...) -> gfrelease -> minisign."""
        g = fixture_key("g2")[0]
        cmd = " ".join([sys.executable, str(GFRELEASE), *self.args(),
                        "index-keys", "renew", "-s", f"<(cat {g})"])
        out = subprocess.run(["bash", "-c", cmd], capture_output=True,
                             text=True)
        self.assertEqual(out.returncode, 0, out.stderr)
        root, keys = trust.load_trust(self.root, self.keys, SLUG)
        self.assertEqual(keys["version"], 3)
        self.assertEqual(keys["keys"], json.loads(
            (self.keys / "2.json").read_text())["keys"])

    def test_rotate_makes_an_unattended_key_and_lists_it(self):
        secret = self.tmp / "out" / "index.key"
        secret.parent.mkdir()
        out = cli(*self.args(), "index-keys", "rotate", "--keep-old",
                  "--secret-out", str(secret), "-s",
                  str(fixture_key("g2")[0]))
        self.assertIn("without a password", out.stdout)
        _, keys = trust.load_trust(self.root, self.keys, SLUG)
        new = minisig.public_key_of(secret.with_name("index.key.pub"))
        self.assertEqual(set(keys["keys"]),
                         {minisig.key_id(new), fixture_key("index-2")[2]})
        # The new key signs unattended: no password asked.
        f = self.tmp / "f"
        f.write_text("x")
        minisig.verify([new], f, minisig.sign(secret, f, "c"))

    def test_a_new_g_takes_over_by_signing_the_next_list(self):
        # Root 3 replaced g1 with g2; the newest list is still g1's.
        for name in ("2.json", "2.json.minisig"):
            (self.keys / name).unlink()
        secret = self.tmp / "out" / "index.key"
        secret.parent.mkdir()
        cli(*self.args(), "index-keys", "rotate", "--secret-out", str(secret),
            "-s", str(fixture_key("g2")[0]))
        root, keys = trust.load_trust(self.root, self.keys, SLUG)
        self.assertEqual((root["version"], keys["version"]), (3, 2))
        # g1 cannot do the same: it is no root's G any more.
        out = cli(*self.args(), "index-keys", "renew", "-s",
                  str(fixture_key("g1")[0]), check=False)
        self.assertNotEqual(out.returncode, 0)

    def test_rotate_never_writes_a_secret_into_the_repository(self):
        out = cli(*self.args(), "index-keys", "rotate", "--secret-out",
                  str(common.ROOT / "index.key"), "-s",
                  str(fixture_key("g2")[0]), check=False)
        self.assertNotEqual(out.returncode, 0)
        self.assertIn("inside the repository", out.stderr)
        self.assertFalse((common.ROOT / "index.key").exists())
        self.assertEqual(trust.versions(self.keys), [1, 2])

    def test_the_ceremony_runs_dry(self):
        legacy = json.loads((common.ROOT / "release" /
                             "product.json").read_text())
        index = fixture_key("index-1")[1]
        legacy.update(repository="saturneric/GpgFrontend",
                      workflow=".github/workflows/build.yml",
                      signing={"index_keys": [index],
                               "maintainer_fingerprints": ["A" * 40],
                               "maintainer_fingerprints_retired": ["B" * 40],
                               "authenticode_subject": "CN=Now",
                               "authenticode_subjects_retired": ["CN=Then"]})
        path = self.tmp / "legacy.json"
        path.write_text(json.dumps(legacy))
        out = cli(*self.args(), "root", "init", "--dry-run",
                  "--legacy-product", str(path))
        where = Path(out.stdout.rsplit("everything is in ", 1)[1].strip())
        self.addCleanup(shutil.rmtree, where, ignore_errors=True)
        self.assertIn("['BBBB", out.stdout)  # retired ones, shown
        anchor = out.stdout.split("GFR_ROOT_SHA256=")[1].split()[0]
        root, keys = trust.load_trust(where / "root", where / "index-keys",
                                      SLUG, anchor)
        self.assertEqual(list(keys["keys"].values()), [index])
        self.assertEqual(root["host_policy"]["authenticode_subjects_retired"],
                         ["CN=Then"])
        self.assertEqual(root["roles"]["root"]["threshold"], 2)
        self.assertEqual(len(root["roles"]["root"]["keyids"]), 3)
        self.assertEqual(len(list((where / "root").glob("*.minisig"))), 2)
        self.assertGreater(trust.days_left(root), 700)
        # Nothing here: the trust dirs of this test are untouched.
        self.assertEqual(trust.versions(self.keys), [1, 2])

    def test_a_list_g_did_not_sign_is_never_left_behind(self):
        out = cli(*self.args(), "index-keys", "renew", "-s",
                  str(fixture_key("g1")[0]), check=False)
        self.assertNotEqual(out.returncode, 0)
        self.assertEqual(sorted(os.listdir(self.keys)),
                         ["1.json", "1.json.minisig", "2.json",
                          "2.json.minisig"])


@unittest.skipUnless(shutil.which("minisign") and shutil.which("git"),
                     "needs minisign and git")
class TrustUpdateTest(unittest.TestCase):
    """The signing host's `gfrelease trust update`: main's trust files,
    from a mirror, into its state."""

    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp(prefix="gfr-trust-update-"))
        self.repo = self.tmp / "repo"
        self.state = self.tmp / "state"
        (self.repo / "release").mkdir(parents=True)
        self.git("init", "-q", "-b", "main")
        self.anchor = common.sha256_file(FIXTURE / "root" / "1.root.json")

    def tearDown(self):
        shutil.rmtree(self.tmp, ignore_errors=True)

    def git(self, *args):
        subprocess.run(["git", "-c", "user.name=t", "-c", "user.email=t@t",
                        *args], cwd=self.repo, check=True,
                       capture_output=True)

    def main_has(self, roots=(1, 2, 3), lists=(1, 2), extra=None):
        """main's release/root/ and release/index-keys/: these of the
        fixture's, and @p extra {path: bytes}."""
        for d in ("root", "index-keys"):
            shutil.rmtree(self.repo / "release" / d, ignore_errors=True)
            (self.repo / "release" / d).mkdir()
        for f in (FIXTURE / "root").iterdir():
            if int(f.name.split(".")[0]) in roots:
                shutil.copy(f, self.repo / "release" / "root")
        for f in (FIXTURE / "index-keys").iterdir():
            if int(f.name.split(".")[0]) in lists:
                shutil.copy(f, self.repo / "release" / "index-keys")
        for path, data in (extra or {}).items():
            (self.repo / "release" / path).write_bytes(data)
        self.git("add", "-A", "release")
        self.git("commit", "-q", "--allow-empty", "-m", "trust")

    def update(self, anchor=None, now=None):
        env = dict(os.environ, GFR_NOW=now) if now else None
        return subprocess.run(
            [sys.executable, str(GFRELEASE),
             "--root-dir", str(self.state / "root"),
             "--index-keys-dir", str(self.state / "index-keys"),
             "--root-sha256", anchor or self.anchor,
             "trust", "update", "--git-dir", str(self.repo),
             "--state-dir", str(self.state)],
            capture_output=True, text=True, env=env)

    def trusted(self):
        return trust.load_trust(self.state / "root",
                                self.state / "index-keys", SLUG,
                                self.anchor)

    def test_the_first_start_takes_the_anchor_and_walks_on(self):
        self.main_has()
        out = self.update()
        self.assertEqual(out.returncode, 0, out.stderr)
        self.assertIn("root 3:", out.stdout)
        root, keys = self.trusted()
        self.assertEqual((root["version"], keys["version"]), (3, 2))

    def test_another_first_root_than_the_anchor_is_refused(self):
        self.main_has()
        out = self.update(anchor="0" * 64)
        self.assertEqual(out.returncode, 1)
        self.assertIn("is not " + "0" * 64, out.stderr)
        self.assertFalse((self.state / "root" / "1.root.json").exists())

    def test_an_invalid_next_root_on_main_keeps_the_current_one(self):
        self.main_has(roots=(1, 2), lists=(1,))
        self.assertEqual(self.update().returncode, 0)
        unsigned = (FIXTURE / "root" / "3.root.json").read_bytes()
        self.main_has(roots=(1, 2), lists=(1,),
                      extra={"root/3.root.json": unsigned})
        out = self.update()
        self.assertEqual(out.returncode, 3, out.stderr)
        self.assertIn("root 2 stays trusted", out.stderr)
        self.assertEqual(self.trusted()[0]["version"], 2)

    def test_an_invalid_next_list_keeps_the_current_one(self):
        self.main_has()
        self.assertEqual(self.update().returncode, 0)
        forged = json.loads((FIXTURE / "index-keys" / "2.json").read_text())
        forged["version"] = 3
        self.main_has(extra={"index-keys/3.json": trust.dump(forged)})
        out = self.update()
        self.assertEqual(out.returncode, 3, out.stderr)
        self.assertIn("3.json: not signed by root 3's", out.stderr)
        self.assertEqual(self.trusted()[1]["version"], 2)

    def test_a_new_g_discards_the_list_and_its_floor(self):
        # Root 2 still names g1: list 1 is taken.
        self.main_has(roots=(1, 2), lists=(1,))
        self.assertEqual(self.update().returncode, 0)
        self.assertEqual(self.trusted()[1]["version"], 1)
        # Root 3 names g2, and main has only g1's list: nothing to sign
        # with any more.
        self.main_has(roots=(1, 2, 3), lists=(1,))
        out = self.update()
        self.assertEqual(out.returncode, 1)
        self.assertIn("no index-keys list that root 3's", out.stderr)
        self.assertEqual(trust.versions(self.state / "index-keys"), [])
        # g2's list 2.
        self.main_has()
        self.assertEqual(self.update().returncode, 0)
        self.assertEqual(self.trusted()[1]["version"], 2)

    def test_a_list_rolled_back_on_main_is_not_taken(self):
        self.main_has()
        self.assertEqual(self.update().returncode, 0)
        self.main_has(lists=())
        self.assertEqual(self.update().returncode, 0)
        self.assertEqual(self.trusted()[1]["version"], 2)
        floor = json.loads((self.state / "index-keys" /
                            "floor.json").read_text())
        self.assertEqual(floor["version"], 2)

    def test_an_expiry_near_is_reported_and_one_past_stops(self):
        self.main_has()
        out = self.update(now="2099-12-10T00:00:00Z")
        self.assertEqual(out.returncode, 3, out.stderr)
        self.assertIn("index-keys 2 expires at", out.stderr)
        self.assertIn("root 3 expires at", out.stderr)
        out = self.update(now="2100-01-02T00:00:00Z")
        self.assertEqual(out.returncode, 1)
        self.assertIn("expired", out.stderr)


if __name__ == "__main__":
    unittest.main()
