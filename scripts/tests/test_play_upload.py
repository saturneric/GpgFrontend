#!/usr/bin/env python3
# Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""The Google Play upload, offline: the service-account assertion, the edit's
request sequence against a fake API, and the workflow job that runs it.

Run: python3 -m unittest discover -s scripts/tests
"""

import base64
import contextlib
import hashlib
import io
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

SCRIPTS = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(SCRIPTS / "android"))

import play_upload as pu  # noqa: E402

WORKFLOW = SCRIPTS.parent / ".github" / "workflows" / "build.yml"
PACKAGE = "com.bktus.gpgfrontend"
SESSION = pu.UPLOAD_API + "/" + PACKAGE + "/edits/e1/bundles?upload_id=u1"


def unb64url(text):
    return base64.urlsafe_b64decode(text + "=" * (-len(text) % 4))


def creds(**extra):
    value = {"type": "service_account",
             "client_email": "ci@example.iam.gserviceaccount.com",
             "private_key": "PEM", "private_key_id": "kid1"}
    value.update(extra)
    return value


class FakePlay:
    """The Play API as far as one edit goes, recording every request."""

    def __init__(self, fail=None, sha256=None):
        self.calls = []
        self.fail = fail
        self.sha256 = sha256

    def __call__(self, method, url, headers, body):
        self.calls.append((method, url, dict(headers), body))
        if self.fail and self.fail in url and method != "DELETE":
            return pu.Response(400, {}, b'{"error": "nope"}')
        if method == "POST" and url.endswith("/edits"):
            return pu.Response(200, {}, b'{"id": "e1"}')
        if url.endswith("/bundles?uploadType=resumable"):
            return pu.Response(200, {"Location": SESSION}, b"")
        if url == SESSION:
            digest = self.sha256 or hashlib.sha256(body).hexdigest()
            return pu.Response(200, {}, json.dumps(
                {"versionCode": 2531012, "sha256": digest}).encode())
        return pu.Response(200, {}, b"{}")

    def methods(self):
        return [(m, u.split("/edits")[-1]) for m, u, _, _ in self.calls]


class AssertionTest(unittest.TestCase):

    def test_b64url_has_no_padding(self):
        self.assertEqual(pu.b64url(b"\xff\xfe"), "__4")

    def test_claims_and_header(self):
        signed = []

        def sign(pem, data):
            signed.append((pem, data))
            return b"sig"

        jwt = pu.make_assertion(creds(token_uri="https://evil.example/t"),
                                1000, sign)
        header, claims, sig = jwt.split(".")
        self.assertEqual(json.loads(unb64url(header)),
                         {"alg": "RS256", "typ": "JWT", "kid": "kid1"})
        self.assertEqual(json.loads(unb64url(claims)), {
            "iss": "ci@example.iam.gserviceaccount.com",
            "scope": pu.SCOPE, "aud": pu.TOKEN_URI,
            "iat": 1000, "exp": 4600})
        self.assertEqual(unb64url(sig), b"sig")
        self.assertEqual(signed, [("PEM", f"{header}.{claims}".encode())])

    def test_credentials_are_checked(self):
        for text in ("", "[]", json.dumps(creds(type="user")),
                     json.dumps(creds(private_key=""))):
            with self.subTest(text=text):
                with self.assertRaises(pu.PlayError):
                    pu.load_credentials(text)
        self.assertEqual(pu.load_credentials(json.dumps(creds()))["private_key"],
                         "PEM")

    def test_token_exchange(self):
        seen = []

        def transport(method, url, headers, body):
            seen.append((method, url, body))
            return pu.Response(200, {}, b'{"access_token": "tok"}')

        self.assertEqual(pu.fetch_token("a.b.c", transport), "tok")
        self.assertEqual(seen, [(
            "POST", pu.TOKEN_URI,
            b"grant_type=urn%3Aietf%3Aparams%3Aoauth%3Agrant-type%3Ajwt-bearer"
            b"&assertion=a.b.c")])

    @unittest.skipUnless(shutil.which("openssl"), "openssl is not installed")
    def test_openssl_signature_verifies(self):
        with tempfile.TemporaryDirectory() as tmp:
            key, pub = Path(tmp) / "k.pem", Path(tmp) / "p.pem"
            sig, data = Path(tmp) / "sig", Path(tmp) / "data"
            subprocess.run(["openssl", "genpkey", "-algorithm", "RSA",
                            "-pkeyopt", "rsa_keygen_bits:2048",
                            "-out", str(key)], check=True, capture_output=True)
            subprocess.run(["openssl", "pkey", "-in", str(key), "-pubout",
                            "-out", str(pub)], check=True, capture_output=True)
            data.write_bytes(b"header.claims")
            sig.write_bytes(pu.openssl_sign(key.read_text(), b"header.claims"))
            done = subprocess.run(
                ["openssl", "dgst", "-sha256", "-verify", str(pub),
                 "-signature", str(sig), str(data)], capture_output=True)
            self.assertEqual(done.returncode, 0, done.stderr)
            self.assertEqual(sorted(p.name for p in Path(tmp).iterdir()),
                             ["data", "k.pem", "p.pem", "sig"])

    def test_bad_key_fails_cleanly(self):
        if not shutil.which("openssl"):
            self.skipTest("openssl is not installed")
        with self.assertRaises(pu.PlayError):
            pu.openssl_sign("not a key", b"x")


class PublishTest(unittest.TestCase):

    def setUp(self):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        self.bundle = Path(tmp.name) / "app.aab"
        self.bundle.write_bytes(b"PK bundle bytes")

    def publish(self, play):
        return pu.publish(PACKAGE, self.bundle, "internal", "completed",
                          "nightly-abc1234", "tok", play)

    def test_one_edit_in_order(self):
        play = FakePlay()
        self.assertEqual(self.publish(play), 2531012)
        self.assertEqual(play.methods(), [
            ("POST", ""),
            ("POST", "/e1/bundles?uploadType=resumable"),
            ("PUT", "/e1/bundles?upload_id=u1"),
            ("PUT", "/e1/tracks/internal"),
            ("POST", "/e1:commit"),
        ])
        track = json.loads(play.calls[3][3])
        self.assertEqual(track, {"track": "internal", "releases": [{
            "versionCodes": ["2531012"], "status": "completed",
            "name": "nightly-abc1234"}]})
        start = play.calls[1][2]
        self.assertEqual(start["X-Upload-Content-Length"],
                         str(len(b"PK bundle bytes")))

    def test_token_only_goes_to_the_api(self):
        play = FakePlay()
        self.publish(play)
        for method, url, headers, body in play.calls:
            with self.subTest(method=method, url=url):
                if url == SESSION:
                    self.assertNotIn("Authorization", headers)
                    self.assertEqual(body, b"PK bundle bytes")
                else:
                    self.assertEqual(headers["Authorization"], "Bearer tok")
                    self.assertTrue(url.startswith(pu.API) or
                                    url.startswith(pu.UPLOAD_API))

    def test_every_post_has_a_body(self):
        play = FakePlay()
        self.publish(play)
        for method, url, _, body in play.calls:
            if method == "POST":
                with self.subTest(url=url):
                    self.assertIsNotNone(body)

    def test_failure_deletes_the_edit(self):
        play = FakePlay(fail="/tracks/")
        with self.assertRaisesRegex(pu.PlayError, r"HTTP 400.*nope"):
            self.publish(play)
        self.assertEqual(play.methods()[-1], ("DELETE", "/e1"))
        self.assertNotIn(("POST", "/e1:commit"), play.methods())

    def test_other_bytes_on_play_are_refused(self):
        play = FakePlay(sha256="00" * 32)
        with self.assertRaisesRegex(pu.PlayError, "other bytes"):
            self.publish(play)
        self.assertEqual(play.methods()[-1], ("DELETE", "/e1"))

    def test_no_edit_nothing_to_delete(self):
        play = FakePlay(fail="/edits")
        with self.assertRaises(pu.PlayError):
            self.publish(play)
        self.assertEqual(play.methods(), [("POST", "")])

    @unittest.skipUnless(shutil.which("openssl"), "openssl is not installed")
    def test_dry_run_sends_nothing(self):
        key = subprocess.run(
            ["openssl", "genpkey", "-algorithm", "RSA",
             "-pkeyopt", "rsa_keygen_bits:2048"],
            check=True, capture_output=True, text=True).stdout
        env = {pu.CREDENTIALS_ENV: json.dumps(creds(private_key=key))}
        out = io.StringIO()
        with mock.patch.dict(os.environ, env), \
                mock.patch.object(pu, "http",
                                  side_effect=AssertionError("network")), \
                contextlib.redirect_stdout(out):
            rc = pu.main(["--package", PACKAGE, "--bundle", str(self.bundle),
                          "--dry-run"])
        self.assertEqual(rc, 0)
        self.assertIn(":commit", out.getvalue())


# ---------------------------------------------------------------- workflow


def jobs(text):
    body = text[text.index("\njobs:\n"):]
    parts = re.split(r"^  ([A-Za-z0-9_-]+):\s*$", body, flags=re.M)
    return dict(zip(parts[1::2], parts[2::2]))


class WorkflowTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        job = jobs(WORKFLOW.read_text(encoding="utf-8"))["play-upload"]
        cls.job = re.sub(r"^\s*#.*\n", "", job, flags=re.M)

    def condition(self):
        match = re.search(r"(?m)^    if: >-\n((?:      .*\n)+)", self.job)
        return " ".join(match.group(1).split())

    def test_right_after_signing_without_the_approval_gate(self):
        self.assertRegex(self.job, r"(?m)^    needs: \[sign-android\]$")

    def test_pushes_to_main_and_version_tags_only(self):
        self.assertEqual(self.condition(),
                         "github.event_name == 'push' && "
                         "(github.ref == 'refs/heads/main' || "
                         "startsWith(github.ref, 'refs/tags/v'))")

    def test_every_ref_goes_to_the_testers_only(self):
        self.assertIn("--track internal", self.job)
        self.assertNotIn("production", self.job)
        self.assertIn('name="$GITHUB_REF_NAME"', self.job)

    def test_secrets_in_their_own_environment(self):
        self.assertRegex(self.job, r"(?m)^    environment: google-play$")
        self.assertNotIn("secrets.ANDROID_", self.job)

    def test_least_privilege(self):
        perms = re.search(r"(?m)^    permissions:\n((?:      .*\n)+)", self.job)
        self.assertEqual(perms.group(1).split(), ["contents:", "read"])

    def test_uploads_the_arm64_bundle_signed_with_the_upload_key(self):
        self.assertIn("name: unsigned-android-arm64", self.job)
        self.assertNotIn("pattern:", self.job)
        self.assertIn("jarsigner -keystore", self.job)
        self.assertIn("scripts/android/play_upload.py", self.job)


if __name__ == "__main__":
    unittest.main()
