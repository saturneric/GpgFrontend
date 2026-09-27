#!/usr/bin/env python3
# Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""The release-candidate trust chain, end to end and offline.

  Sigstore identity -> signed SHA256SUMS -> exact payload bytes
                    -> exact published file set

Sigstore itself is replaced by a fake cosign that binds a bundle to the
manifest's digest and to the identity, issuer and workflow sha it was signed
under, and refuses verification unless all four match exactly. That is the
property the real one provides, and what this code relies on.

Run: python3 -m unittest discover -s scripts/tests
"""

import contextlib
import io
import json
import os
import random
import re
import shutil
import stat
import sys
import tempfile
import textwrap
import unittest
from pathlib import Path

SCRIPTS = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(SCRIPTS))

import release_candidate as rc  # noqa: E402

WORKFLOW = SCRIPTS.parent / ".github" / "workflows" / "build.yml"

IDENTITY = ("https://github.com/saturneric/GpgFrontend/"
            ".github/workflows/build.yml@refs/heads/main")
SHA = "0123456789abcdef0123456789abcdef01234567"

FAKE_COSIGN = textwrap.dedent("""\
    #!{python}
    import hashlib, json, sys

    def opt(name):
        return args[args.index(name) + 1] if name in args else None

    cmd, args = sys.argv[1], sys.argv[2:]
    if any("regexp" in a for a in args):
        sys.exit("fake cosign: a pattern is not an identity")
    blob = args[-1]
    with open(blob, "rb") as f:
        blob_sha = hashlib.sha256(f.read()).hexdigest()

    if cmd == "sign-blob":
        claims = {{"identity": opt("--identity"), "issuer": opt("--issuer"),
                  "sha": opt("--sha"), "blob": blob_sha}}
        with open(opt("--bundle"), "w") as f:
            json.dump(claims, f)
        sys.exit(0)

    if cmd == "verify-blob":
        with open(opt("--bundle")) as f:
            claims = json.load(f)
        wanted = {{"identity": opt("--certificate-identity"),
                  "issuer": opt("--certificate-oidc-issuer"),
                  "sha": opt("--certificate-github-workflow-sha"),
                  "blob": blob_sha}}
        sys.exit(0 if claims == wanted else "fake cosign: verification failed")

    sys.exit("fake cosign: unknown command " + cmd)
    """)


def build_info(os_name="Linux", modules=("m.a", "m.b")):
    return {
        "project_version": "2.2.0",
        "build_id": "abc1234",
        "host_os": os_name,
        "modules": list(modules),
        "module_count": len(modules),
    }


# Two legs, three deliverables: the smallest shape with every relation in it.
DEFAULT_LEGS = {
    "ubuntu-24.04-installed": {"GpgFrontend-x86_64.AppImage": b"appimage\n"},
    "windows-2022-portable": {
        "GpgFrontend-x86_64-portable.zip": b"zip\x00bytes",
        "GpgFrontend-x86_64.msi": b"msi\xff",
    },
}


class Tree:
    """A temporary workspace: artifact downloads, a candidate, a cosign."""

    def __init__(self, root):
        self.root = root
        self.artifacts = root / "in"
        self.candidate = root / "candidate"
        self.artifacts.mkdir()
        self.cosign = root / "cosign"
        self.cosign.write_text(FAKE_COSIGN.format(python=sys.executable))
        self.cosign.chmod(self.cosign.stat().st_mode | stat.S_IEXEC)

    def add_leg(self, leg, files, info=None):
        d = self.artifacts / f"gpgfrontend-{leg}"
        d.mkdir()
        for name, data in files.items():
            (d / name).write_bytes(data)
        if info is not False:
            b = self.artifacts / f"buildinfo-{leg}"
            b.mkdir()
            (b / "build-info.json").write_text(json.dumps(info or build_info()))

    def add_legs(self, legs=DEFAULT_LEGS, order=None):
        for leg in order or legs:
            self.add_leg(leg, legs[leg])

    def stage(self):
        with contextlib.redirect_stdout(io.StringIO()):
            rc.stage(self.artifacts, self.candidate)

    def sign(self, identity=IDENTITY, issuer=rc.ISSUER, sha=SHA):
        import subprocess
        subprocess.run(
            [str(self.cosign), "sign-blob",
             "--bundle", str(self.candidate / rc.BUNDLE),
             "--identity", identity, "--issuer", issuer, "--sha", sha,
             str(self.candidate / rc.MANIFEST)],
            check=True)

    def verify(self, identity=IDENTITY, sha=SHA):
        return rc.verify(self.candidate, identity, sha, str(self.cosign))

    def signed_candidate(self, **legs):
        self.add_legs(**legs)
        self.stage()
        self.sign()


class TreeCase(unittest.TestCase):

    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()
        self.tree = Tree(Path(self._tmp.name))

    def tearDown(self):
        self._tmp.cleanup()

    def assertRejected(self, fn, pattern):
        with self.assertRaisesRegex(rc.CandidateError, pattern):
            fn()


# ------------------------------------------------------------------- chain


class ChainTest(TreeCase):

    def test_whole_chain_publishes_exactly_n_plus_three(self):
        self.tree.signed_candidate()
        published = [p.name for p in self.tree.verify()]

        deliverables = sorted(n for f in DEFAULT_LEGS.values() for n in f)
        self.assertEqual(
            published,
            sorted(deliverables + [rc.RECORDS]) + [rc.MANIFEST, rc.BUNDLE])
        self.assertEqual(len(published), len(deliverables) + 3)

    def test_manifest_is_sha256sum_compatible(self):
        self.tree.signed_candidate()
        lines = (self.tree.candidate / rc.MANIFEST).read_text().splitlines()
        for line in lines:
            sha, name = line.split("  ")
            self.assertEqual(rc.digest(self.tree.candidate / name), sha)
        self.assertEqual([l.split("  ")[1] for l in lines],
                         sorted(l.split("  ")[1] for l in lines))

    def test_github_output_lists_verified_files(self):
        self.tree.signed_candidate()
        out = self.tree.root / "gh-output"
        with contextlib.redirect_stdout(io.StringIO()):
            code = rc.main(["verify", "--dir", str(self.tree.candidate),
                            "--identity", IDENTITY, "--sha", SHA,
                            "--cosign", str(self.tree.cosign),
                            "--github-output", str(out)])
        self.assertEqual(code, 0)
        text = out.read_text()
        match = re.fullmatch(r"files<<(\S+)\n(.*)\n\1\n", text, re.S)
        self.assertIsNotNone(match, text)
        listed = [Path(p) for p in match.group(2).split("\n")]
        self.assertTrue(all(p.is_absolute() for p in listed))
        self.assertEqual({p.name for p in listed},
                         {p.name for p in self.tree.candidate.iterdir()})

    def test_failure_writes_no_output(self):
        self.tree.signed_candidate()
        (self.tree.candidate / "stray").write_bytes(b"x")
        out = self.tree.root / "gh-output"
        with contextlib.redirect_stderr(io.StringIO()):
            code = rc.main(["verify", "--dir", str(self.tree.candidate),
                            "--identity", IDENTITY, "--sha", SHA,
                            "--cosign", str(self.tree.cosign),
                            "--github-output", str(out)])
        self.assertEqual(code, 1)
        self.assertFalse(out.exists())


# ---------------------------------------------------------------- identity


class IdentityTest(TreeCase):

    def test_other_workflow_identity_rejected(self):
        self.tree.add_legs()
        self.tree.stage()
        self.tree.sign(identity=IDENTITY.replace("build.yml", "evil.yml"))
        self.assertRejected(self.tree.verify, "does not verify")

    def test_other_issuer_rejected(self):
        self.tree.add_legs()
        self.tree.stage()
        self.tree.sign(issuer="https://issuer.example")
        self.assertRejected(self.tree.verify, "does not verify")

    def test_other_commit_rejected(self):
        self.tree.add_legs()
        self.tree.stage()
        self.tree.sign(sha="f" * 40)
        self.assertRejected(self.tree.verify, "does not verify")

    def test_verifier_expects_other_identity(self):
        self.tree.signed_candidate()
        self.assertRejected(
            lambda: self.tree.verify(identity=IDENTITY + "x"), "does not verify")

    def test_empty_identity_or_sha_refused(self):
        self.tree.signed_candidate()
        self.assertRejected(lambda: self.tree.verify(identity=""), "required")
        self.assertRejected(lambda: self.tree.verify(sha=""), "required")

    def test_identity_is_exact_never_a_pattern(self):
        seen = []
        real_run = rc.subprocess.run

        def spy(cmd, *a, **kw):
            seen.append(cmd)
            return real_run(cmd, *a, **kw)

        self.tree.signed_candidate()
        rc.subprocess.run = spy
        try:
            self.tree.verify()
        finally:
            rc.subprocess.run = real_run

        (cmd,) = seen
        self.assertIn("--certificate-identity", cmd)
        self.assertEqual(cmd[cmd.index("--certificate-identity") + 1], IDENTITY)
        self.assertEqual(cmd[cmd.index("--certificate-oidc-issuer") + 1],
                         "https://token.actions.githubusercontent.com")
        self.assertEqual(
            cmd[cmd.index("--certificate-github-workflow-sha") + 1], SHA)
        self.assertFalse(any("regexp" in part for part in cmd))


# ------------------------------------------------------------------- bytes


class BytesTest(TreeCase):

    def setUp(self):
        super().setUp()
        self.tree.signed_candidate()
        self.c = self.tree.candidate

    def test_modified_deliverable(self):
        path = self.c / "GpgFrontend-x86_64.msi"
        data = bytearray(path.read_bytes())
        data[0] ^= 1
        path.write_bytes(bytes(data))
        self.assertRejected(self.tree.verify, "sha256 does not match")

    def test_modified_records(self):
        path = self.c / rc.RECORDS
        path.write_text(path.read_text().replace("2.2.0", "9.9.9"))
        self.assertRejected(self.tree.verify, "sha256 does not match")

    def _edit_manifest(self, edit):
        path = self.c / rc.MANIFEST
        path.write_bytes(edit(path.read_bytes()))
        self.assertRejected(self.tree.verify, "does not verify")

    def test_manifest_hash_edited(self):
        self._edit_manifest(lambda b: ("0" * 64).encode() + b[64:])

    def test_manifest_line_added(self):
        self._edit_manifest(lambda b: b + b"0" * 64 + b"  GpgFrontend-extra\n")

    def test_manifest_line_removed(self):
        self._edit_manifest(lambda b: b"".join(b.splitlines(True)[1:]))

    def test_manifest_reordered(self):
        self._edit_manifest(lambda b: b"".join(reversed(b.splitlines(True))))

    def test_manifest_crlf(self):
        self._edit_manifest(lambda b: b.replace(b"\n", b"\r\n"))

    def test_bundle_from_other_manifest(self):
        (self.tree.root / "other").mkdir()
        other = Tree(self.tree.root / "other")
        other.add_legs({"leg": {"GpgFrontend-other": b"other"}})
        other.stage()
        other.sign()
        shutil.copy(other.candidate / rc.BUNDLE, self.c / rc.BUNDLE)
        self.assertRejected(self.tree.verify, "does not verify")


# --------------------------------------------------------------------- set


class SetTest(TreeCase):

    def setUp(self):
        super().setUp()
        self.tree.signed_candidate()
        self.c = self.tree.candidate

    def test_missing_deliverable(self):
        (self.c / "GpgFrontend-x86_64.AppImage").unlink()
        self.assertRejected(self.tree.verify,
                            r"missing \['GpgFrontend-x86_64.AppImage'\]")

    def test_extra_file(self):
        (self.c / "GpgFrontend-unsigned.dmg").write_bytes(b"x")
        self.assertRejected(self.tree.verify,
                            r"unlisted \['GpgFrontend-unsigned.dmg'\]")

    def test_extra_build_info_never_published(self):
        (self.c / "build-info.json").write_text("{}")
        self.assertRejected(self.tree.verify, r"unlisted \['build-info.json'\]")

    def test_missing_records(self):
        (self.c / rc.RECORDS).unlink()
        self.assertRejected(self.tree.verify, r"missing \['build-records.json'\]")

    def test_missing_bundle(self):
        (self.c / rc.BUNDLE).unlink()
        self.assertRejected(self.tree.verify, "missing SHA256SUMS.txt.sigstore")

    def test_missing_manifest(self):
        (self.c / rc.MANIFEST).unlink()
        self.assertRejected(self.tree.verify, "missing SHA256SUMS.txt$")

    def test_subdirectory(self):
        (self.c / "nested").mkdir()
        self.assertRejected(self.tree.verify, "not a regular file")

    def test_symlink_to_listed_content(self):
        target = self.c / "GpgFrontend-x86_64.AppImage"
        moved = self.tree.root / "elsewhere"
        target.rename(moved)
        target.symlink_to(moved)
        self.assertRejected(self.tree.verify, "not a regular file")


# ------------------------------------------------------------------- names


class NamesTest(TreeCase):

    def test_same_name_in_two_legs(self):
        self.tree.add_leg("a", {"GpgFrontend-same": b"1"})
        self.tree.add_leg("b", {"GpgFrontend-same": b"2"})
        self.assertRejected(self.tree.stage, "produced by both a and b")

    def test_reserved_names(self):
        for name in sorted(rc.RESERVED):
            with self.subTest(name=name):
                shutil.rmtree(self.tree.artifacts)
                self.tree.artifacts.mkdir()
                self.tree.add_leg("a", {name: b"x"})
                self.assertRejected(self.tree.stage, "reserved provenance name")

    def test_unsafe_names(self):
        for name in (".hidden", "-flag", "has space", "tab\there", "ümlaut"):
            with self.subTest(name=name):
                shutil.rmtree(self.tree.artifacts)
                self.tree.artifacts.mkdir()
                self.tree.add_leg("a", {name: b"x"})
                self.assertRejected(self.tree.stage, "unsafe file name")

    def test_subdirectory_in_leg(self):
        self.tree.add_leg("a", {"GpgFrontend-a": b"x"})
        (self.tree.artifacts / "gpgfrontend-a" / "nested").mkdir()
        self.assertRejected(self.tree.stage, "not a regular file")

    def test_symlink_in_leg(self):
        self.tree.add_leg("a", {"GpgFrontend-a": b"x"})
        (self.tree.artifacts / "gpgfrontend-a" / "GpgFrontend-link").symlink_to(
            self.tree.artifacts / "gpgfrontend-a" / "GpgFrontend-a")
        self.assertRejected(self.tree.stage, "not a regular file")

    def test_unknown_artifact_directory(self):
        self.tree.add_leg("a", {"GpgFrontend-a": b"x"})
        (self.tree.artifacts / "unsigned-macos-app-macos-15").mkdir()
        self.assertRejected(self.tree.stage, "not a release artifact")

    def test_existing_output_refused(self):
        self.tree.add_leg("a", {"GpgFrontend-a": b"x"})
        self.tree.candidate.mkdir()
        self.assertRejected(self.tree.stage, "already exists")

    def test_refused_input_writes_nothing(self):
        self.tree.add_leg("a", {"GpgFrontend-a": b"x"})
        self.tree.add_leg("b", {"build-records.json": b"x"})
        self.assertRejected(self.tree.stage, "reserved")
        self.assertFalse(self.tree.candidate.exists())

    def test_manifest_parser(self):
        good = "a" * 64
        cases = {
            "duplicate": f"{good}  {rc.RECORDS}\n{good}  {rc.RECORDS}\n",
            "malformed line": f"{good} {rc.RECORDS}\n",
            "carriage return": f"{good}  {rc.RECORDS}\r\n",
            "trailing newline": f"{good}  {rc.RECORDS}",
            "unsafe name": f"{good}  {rc.RECORDS}\n{good}  ../x\n",
            "lists 'SHA256SUMS.txt'": f"{good}  {rc.RECORDS}\n{good}  SHA256SUMS.txt\n",
            "does not cover": f"{good}  GpgFrontend-a\n",
            "malformed line ": f"{good.upper()}  {rc.RECORDS}\n",
        }
        for why, text in cases.items():
            with self.subTest(why=why):
                self.assertRejected(lambda: rc.parse_manifest(text.encode()),
                                    re.escape(why.strip()))


# ----------------------------------------------------------- legs, records


class RecordsTest(TreeCase):

    def test_deliverable_without_build_info(self):
        self.tree.add_leg("a", {"GpgFrontend-a": b"x"}, info=False)
        self.assertRejected(self.tree.stage, r"gpgfrontend-a \(no build info\)")

    def test_build_info_without_deliverable(self):
        self.tree.add_leg("a", {"GpgFrontend-a": b"x"})
        (self.tree.artifacts / "buildinfo-b").mkdir()
        self.assertRejected(self.tree.stage, r"buildinfo-b \(no deliverables\)")

    def test_pairing_is_exact_not_substring(self):
        # The old rule matched `macos-15` inside `macos-15-intel-installed`.
        self.tree.add_leg("macos-15-intel-installed", {"GpgFrontend-a": b"x"},
                          info=False)
        b = self.tree.artifacts / "buildinfo-macos-15"
        b.mkdir()
        (b / "build-info.json").write_text(json.dumps(build_info()))
        self.assertRejected(self.tree.stage, "unpaired")

    def test_no_deliverables_at_all(self):
        self.assertRejected(self.tree.stage, "no deliverable artifacts")

    def test_empty_leg(self):
        self.tree.add_leg("a", {})
        self.assertRejected(self.tree.stage, "no deliverables")

    def test_build_info_dir_shapes(self):
        for files in ({}, {"build-info.json": "{}", "other.json": "{}"},
                      {"other.json": "{}"}):
            with self.subTest(files=sorted(files)):
                shutil.rmtree(self.tree.artifacts)
                self.tree.artifacts.mkdir()
                self.tree.add_leg("a", {"GpgFrontend-a": b"x"}, info=False)
                b = self.tree.artifacts / "buildinfo-a"
                b.mkdir()
                for name, text in files.items():
                    (b / name).write_text(text)
                self.assertRejected(self.tree.stage, "expected exactly")

    def test_incomplete_build_info(self):
        info = build_info()
        del info["build_id"]
        self.tree.add_leg("a", {"GpgFrontend-a": b"x"}, info=info)
        self.assertRejected(self.tree.stage, r"missing \['build_id'\]")

    def test_duplicate_legs_in_aggregate(self):
        r = rc.make_record("a", build_info(), ["GpgFrontend-a"])
        self.assertRejected(lambda: rc.build_aggregate([r, dict(r)]),
                            "duplicate build records")

    def test_versions_are_pinned(self):
        # Changing what a record means is a schema bump, and changing the
        # container is a format bump. Neither happens by accident.
        self.tree.signed_candidate()
        doc = json.loads((self.tree.candidate / rc.RECORDS).read_text())
        self.assertEqual(doc["format"], "gpgfrontend-build-records")
        self.assertEqual(doc["format_version"], 1)
        self.assertEqual({r["schema"] for r in doc["records"]}, {4})
        for record in doc["records"]:
            self.assertEqual(
                set(record), {"schema", "leg", "product", "modules", "artifacts"})
            self.assertTrue(all(isinstance(n, str) for n in record["artifacts"]))

    def test_records_describe_each_leg(self):
        self.tree.signed_candidate()
        doc = json.loads((self.tree.candidate / rc.RECORDS).read_text())
        self.assertEqual(
            {r["leg"]: r["artifacts"] for r in doc["records"]},
            {leg: sorted(files) for leg, files in DEFAULT_LEGS.items()})

    def _resign_records(self, edit):
        """Tamper with the records AND re-sign, so only check_records stands."""
        self.tree.signed_candidate()
        c = self.tree.candidate
        doc = json.loads((c / rc.RECORDS).read_text())
        edit(doc)
        (c / rc.RECORDS).write_text(rc.dump_json(doc))
        hashes = rc.parse_manifest((c / rc.MANIFEST).read_bytes())
        hashes[rc.RECORDS] = rc.digest(c / rc.RECORDS)
        (c / rc.MANIFEST).write_text(rc.render_manifest(hashes))
        self.tree.sign()

    def test_signed_records_missing_a_name(self):
        self._resign_records(lambda d: d["records"][0]["artifacts"].pop())
        self.assertRejected(self.tree.verify, "unrecorded")

    def test_signed_records_name_claimed_twice(self):
        def edit(d):
            d["records"][1]["artifacts"].append(d["records"][0]["artifacts"][0])
        self._resign_records(edit)
        self.assertRejected(self.tree.verify, "claimed by")

    def test_signed_records_duplicate_leg(self):
        def edit(d):
            d["records"].append(dict(d["records"][0], artifacts=[]))
        self._resign_records(edit)
        self.assertRejected(self.tree.verify, "duplicate leg")

    def test_signed_records_wrong_schema(self):
        self._resign_records(lambda d: d["records"][0].update(schema=3))
        self.assertRejected(self.tree.verify, "record schema")

    def test_signed_records_wrong_format(self):
        self._resign_records(lambda d: d.update(format_version=2))
        self.assertRejected(self.tree.verify, "unexpected format")

    def test_signed_records_not_json(self):
        self.tree.signed_candidate()
        c = self.tree.candidate
        (c / rc.RECORDS).write_text("{")
        hashes = rc.parse_manifest((c / rc.MANIFEST).read_bytes())
        hashes[rc.RECORDS] = rc.digest(c / rc.RECORDS)
        (c / rc.MANIFEST).write_text(rc.render_manifest(hashes))
        self.tree.sign()
        self.assertRejected(self.tree.verify, "malformed")


# ------------------------------------------------------------- determinism


class DeterminismTest(unittest.TestCase):

    LEGS = {f"leg-{i}": {f"GpgFrontend-{i}-{j}": bytes([i, j]) * 7
                         for j in range(3)} for i in range(6)}

    def _stage(self, order):
        with tempfile.TemporaryDirectory() as tmp:
            tree = Tree(Path(tmp))
            tree.add_legs(self.LEGS, order=order)
            tree.stage()
            return {p.name: p.read_bytes() for p in tree.candidate.iterdir()}

    def test_input_order_does_not_matter(self):
        legs = list(self.LEGS)
        baseline = self._stage(legs)
        rng = random.Random(1234)
        for _ in range(5):
            rng.shuffle(legs)
            self.assertEqual(self._stage(legs), baseline)

    def test_aggregate_order_does_not_matter(self):
        records = [rc.make_record(leg, build_info(), list(reversed(sorted(f))))
                   for leg, f in self.LEGS.items()]
        baseline = rc.dump_json(rc.build_aggregate(records))
        rng = random.Random(99)
        for _ in range(5):
            rng.shuffle(records)
            self.assertEqual(rc.dump_json(rc.build_aggregate(records)), baseline)

    def test_output_is_lf_only(self):
        out = self._stage(list(self.LEGS))
        for name in (rc.MANIFEST, rc.RECORDS):
            self.assertNotIn(b"\r", out[name])
            self.assertTrue(out[name].endswith(b"\n"))


# ---------------------------------------------------------------- workflow


def jobs(text):
    """{job id: job text} for a workflow's top-level `jobs:` map."""
    body = text[text.index("\njobs:\n"):]
    parts = re.split(r"^  ([A-Za-z0-9_-]+):\s*$", body, flags=re.M)
    return dict(zip(parts[1::2], parts[2::2]))


def permissions(job_text):
    match = re.search(r"^    permissions:\n((?:      .*\n|\s*#.*\n)+)",
                      job_text, re.M)
    if not match:
        return {}
    return dict(re.findall(r"^      ([a-z-]+):\s*(\S+)", match.group(1), re.M))


def upload_names(text):
    return re.findall(
        r"uses: actions/upload-artifact@\S+.*\n\s+with:\n(?:\s+#.*\n)*"
        r"\s+name: (\S+)", text)


def normalize(expr):
    return re.sub(r"\$\{\{\s*(.*?)\s*\}\}", r"${{\1}}", expr)


class WorkflowTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.text = WORKFLOW.read_text(encoding="utf-8")
        cls.jobs = jobs(cls.text)

    def code(self, job):
        """A job's text without comments, so prose cannot satisfy a check."""
        return re.sub(r"^\s*#.*\n", "", self.jobs[job], flags=re.M)

    def test_oidc_and_write_never_combined(self):
        for name, job in self.jobs.items():
            perms = permissions(job)
            with self.subTest(job=name):
                self.assertFalse(perms.get("id-token") == "write" and
                                 perms.get("contents") == "write")
        self.assertEqual(
            [n for n, j in self.jobs.items()
             if permissions(j).get("id-token") == "write"], ["provenance"])
        self.assertEqual(
            [n for n, j in self.jobs.items()
             if permissions(j).get("contents") == "write"], ["release"])

    def test_provenance_signs_after_every_platform_check(self):
        needs = re.search(r"^    needs: \[(.*)\]", self.jobs["provenance"], re.M)
        self.assertEqual({n.strip() for n in needs.group(1).split(",")},
                         {"build", "sign-macos", "macos-smoke"})

    def test_release_depends_only_on_the_candidate(self):
        self.assertRegex(self.jobs["release"], r"(?m)^    needs: \[provenance\]$")

    def test_single_signature(self):
        self.assertEqual(self.text.count("cosign sign-blob"), 1)
        self.assertIn("cosign sign-blob", self.code("provenance"))
        self.assertIn("--bundle candidate/SHA256SUMS.txt.sigstore.json",
                      self.code("provenance"))

    def test_candidate_uploaded_once_never_overwritten(self):
        self.assertEqual(upload_names(self.text).count("release-candidate"), 1)
        self.assertNotRegex(self.code("provenance"), r"overwrite:\s*true")
        self.assertNotIn("upload-artifact", self.code("release"))

    def test_release_is_verify_only(self):
        release = self.code("release")
        for forbidden in ("sha256sum", "sign-blob", "merge-multiple",
                          "pattern:", "regexp", "artifacts/*", "mv ", "cp "):
            with self.subTest(forbidden=forbidden):
                self.assertNotIn(forbidden, release)
        self.assertEqual(release.count("download-artifact"), 1)
        self.assertIn("name: release-candidate", release)
        self.assertIn("release_candidate.py verify", release)
        self.assertIn('--identity "https://github.com/${{ github.workflow_ref }}"',
                      release)
        self.assertIn('--sha "${{ github.sha }}"', release)
        self.assertRegex(release,
                         r"(?m)^\s+files: \$\{\{ steps\.verify\.outputs\.files \}\}$")

    def test_verify_runs_before_the_tag_moves(self):
        release = self.code("release")
        self.assertLess(release.index("release_candidate.py verify"),
                        release.index("gh release delete"))

    def test_every_deliverable_has_build_info_with_the_same_key(self):
        names = [normalize(n) for n in upload_names(self.text)]
        deliverables = {n[len("gpgfrontend-"):] for n in names
                        if n.startswith("gpgfrontend-")}
        infos = {n[len("buildinfo-"):] for n in names
                 if n.startswith("buildinfo-")}
        self.assertTrue(deliverables)
        self.assertEqual(deliverables, infos)

    def test_no_upload_collides_with_release_patterns(self):
        # provenance downloads gpgfrontend-* and buildinfo-*. Any other upload
        # in those namespaces would be pulled into the release.
        for name in upload_names(self.text):
            if name.startswith(("gpgfrontend-", "buildinfo-")):
                self.assertIn("${{", name)


if __name__ == "__main__":
    unittest.main()
