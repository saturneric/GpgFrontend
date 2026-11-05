#!/usr/bin/env python3
# Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""Upload a signed app bundle to a Google Play track.

One edit of the Google Play Developer API (v3), in order:

  insert edit -> upload bundle -> set the track's release -> commit

On any failure the edit is deleted, so a half-made edit never lingers in
Play Console. The service account authenticates with a JWT it signs itself
(RFC 7523); OpenSSL makes the RS256 signature, so this needs nothing but the
standard library and the `openssl` command. No third-party package or action
ever sees the service-account key.

The key comes from the environment variable PLAY_SERVICE_ACCOUNT_JSON: the
JSON key file Google Cloud gives for the service account. The token endpoint
is fixed here, never read from that file.

usage: play_upload.py --package NAME --bundle FILE [--track internal]
                      [--status completed] [--release-name NAME] [--dry-run]

--dry-run signs the assertion (so a broken key fails here too) and prints
the requests it would make, sending none.
"""

import argparse
import base64
import hashlib
import json
import os
import subprocess
import sys
import tempfile
import time
import urllib.error
import urllib.parse
import urllib.request
from pathlib import Path

CREDENTIALS_ENV = "PLAY_SERVICE_ACCOUNT_JSON"
TOKEN_URI = "https://oauth2.googleapis.com/token"
SCOPE = "https://www.googleapis.com/auth/androidpublisher"
API = "https://androidpublisher.googleapis.com/androidpublisher/v3/applications"
UPLOAD_API = ("https://androidpublisher.googleapis.com/upload/"
              "androidpublisher/v3/applications")
JWT_BEARER = "urn:ietf:params:oauth:grant-type:jwt-bearer"
BUNDLE_TYPE = "application/octet-stream"
# A bundle of some 50 MB over a runner's link, with room to spare.
TIMEOUT = 900


class PlayError(Exception):
    pass


# ----------------------------------------------------------------- transport


class Response:
    def __init__(self, status, headers, body):
        self.status = status
        self.headers = {k.lower(): v for k, v in headers.items()}
        self.body = body

    def json(self):
        return json.loads(self.body.decode("utf-8")) if self.body else {}


def http(method, url, headers=None, body=None):
    """The one place that touches the network; tests replace it."""
    request = urllib.request.Request(url, data=body, method=method,
                                     headers=headers or {})
    try:
        with urllib.request.urlopen(request, timeout=TIMEOUT) as r:
            return Response(r.status, dict(r.headers), r.read())
    except urllib.error.HTTPError as e:
        return Response(e.code, dict(e.headers or {}), e.read())


# ---------------------------------------------------------------------- auth


def b64url(data):
    return base64.urlsafe_b64encode(data).rstrip(b"=").decode("ascii")


def openssl_sign(pem, data):
    """RS256: an RSASSA-PKCS1-v1_5 SHA-256 signature of `data` by `pem`."""
    with tempfile.TemporaryDirectory() as tmp:
        key = Path(tmp) / "key.pem"
        fd = os.open(key, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
        with os.fdopen(fd, "w", encoding="ascii") as f:
            f.write(pem)
        try:
            done = subprocess.run(
                ["openssl", "dgst", "-sha256", "-sign", str(key)],
                input=data, capture_output=True, check=False)
        finally:
            key.unlink()
    if done.returncode != 0:
        raise PlayError("openssl could not sign with the service-account key")
    return done.stdout


def load_credentials(text):
    try:
        creds = json.loads(text)
    except ValueError:
        raise PlayError(f"{CREDENTIALS_ENV} is not JSON")
    if not isinstance(creds, dict) or creds.get("type") != "service_account":
        raise PlayError(f"{CREDENTIALS_ENV} is not a service-account key")
    for field in ("client_email", "private_key"):
        if not isinstance(creds.get(field), str) or not creds[field]:
            raise PlayError(f"{CREDENTIALS_ENV} has no {field}")
    return creds


def make_assertion(creds, now, sign=openssl_sign):
    header = {"alg": "RS256", "typ": "JWT"}
    if creds.get("private_key_id"):
        header["kid"] = creds["private_key_id"]
    claims = {
        "iss": creds["client_email"],
        "scope": SCOPE,
        "aud": TOKEN_URI,
        "iat": now,
        "exp": now + 3600,
    }
    signing_input = (b64url(json.dumps(header, separators=(",", ":")).encode())
                     + "." +
                     b64url(json.dumps(claims, separators=(",", ":")).encode()))
    signature = sign(creds["private_key"], signing_input.encode("ascii"))
    return signing_input + "." + b64url(signature)


def check(response, what):
    if not 200 <= response.status < 300:
        detail = response.body.decode("utf-8", "replace").strip()
        raise PlayError(f"{what} failed (HTTP {response.status}): {detail}")
    return response


def fetch_token(assertion, transport=http):
    body = urllib.parse.urlencode(
        {"grant_type": JWT_BEARER, "assertion": assertion}).encode("ascii")
    r = check(transport("POST", TOKEN_URI,
                        {"Content-Type": "application/x-www-form-urlencoded"},
                        body), "token exchange")
    token = r.json().get("access_token")
    if not token:
        raise PlayError("token exchange returned no access token")
    return token


# --------------------------------------------------------------------- edit


def sha256(path):
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


class Edit:
    def __init__(self, package, token, transport=http):
        self.package = urllib.parse.quote(package, safe="")
        self.auth = {"Authorization": f"Bearer {token}"}
        self.transport = transport
        self.id = None

    def url(self, suffix="", upload=False):
        base = UPLOAD_API if upload else API
        url = f"{base}/{self.package}/edits"
        if self.id is not None:
            url += "/" + urllib.parse.quote(self.id, safe="")
        return url + suffix

    def call(self, method, url, what, payload=None, headers=None):
        h = dict(self.auth)
        # Google refuses a POST without a length (HTTP 411); b"" sends one.
        body = b"" if method == "POST" else None
        if payload is not None:
            h["Content-Type"] = "application/json"
            body = json.dumps(payload).encode("utf-8")
        h.update(headers or {})
        return check(self.transport(method, url, h, body), what)

    def insert(self):
        self.id = self.call("POST", self.url(), "creating the edit",
                            payload={}).json().get("id")
        if not self.id:
            raise PlayError("creating the edit returned no edit id")

    def upload_bundle(self, path):
        """Resumable upload in one piece; returns the bundle's versionCode."""
        data = path.read_bytes()
        start = self.call(
            "POST", self.url("/bundles?uploadType=resumable", upload=True),
            "starting the bundle upload", payload={},
            headers={"X-Upload-Content-Type": BUNDLE_TYPE,
                     "X-Upload-Content-Length": str(len(data))})
        # The session URL authorizes the upload by itself: the token is not
        # sent with the bytes.
        session = start.headers.get("location", "")
        if not session.startswith("https://"):
            raise PlayError("the bundle upload returned no upload session")
        bundle = check(self.transport(
            "PUT", session, {"Content-Type": BUNDLE_TYPE,
                             "Content-Length": str(len(data))}, data),
            "uploading the bundle").json()
        expected = hashlib.sha256(data).hexdigest()
        if bundle.get("sha256", "").lower() != expected:
            raise PlayError("Play received other bytes than the bundle: "
                            f"sha256 {bundle.get('sha256')} != {expected}")
        code = bundle.get("versionCode")
        if not isinstance(code, int):
            raise PlayError("the bundle upload returned no versionCode")
        return code

    def set_track(self, track, code, status, name):
        release = {"versionCodes": [str(code)], "status": status}
        if name:
            release["name"] = name
        self.call("PUT", self.url("/tracks/" + urllib.parse.quote(track,
                                                                  safe="")),
                  f"setting the {track} track",
                  payload={"track": track, "releases": [release]})

    def commit(self):
        self.call("POST", self.url(":commit"), "committing the edit")

    def delete(self):
        """Best effort: the error that led here is the one worth reporting."""
        if self.id is None:
            return
        try:
            self.transport("DELETE", self.url(), dict(self.auth), None)
        except OSError:
            pass


def publish(package, bundle, track, status, name, token, transport=http):
    edit = Edit(package, token, transport)
    try:
        edit.insert()
        code = edit.upload_bundle(bundle)
        edit.set_track(track, code, status, name)
        edit.commit()
    except BaseException:
        edit.delete()
        raise
    return code


# ---------------------------------------------------------------------- main


def dry_run_transport(method, url, headers, body):
    shown = len(body) if body is not None else 0
    print(f"would {method} {url} ({shown} bytes)")
    if url == TOKEN_URI:
        return Response(200, {}, b'{"access_token": "dry-run"}')
    if url.endswith("/bundles?uploadType=resumable"):
        return Response(200, {"Location": UPLOAD_API + "/dry-run"}, b"")
    if method == "PUT" and url == UPLOAD_API + "/dry-run":
        return Response(200, {}, json.dumps(
            {"versionCode": 0,
             "sha256": hashlib.sha256(body).hexdigest()}).encode())
    if method == "POST" and url.endswith("/edits"):
        return Response(200, {}, b'{"id": "dry-run"}')
    return Response(200, {}, b"{}")


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--package", required=True)
    parser.add_argument("--bundle", required=True, type=Path)
    parser.add_argument("--track", default="internal")
    # A draft is what Play accepts for an app that was never published.
    parser.add_argument("--status", default="completed",
                        choices=["completed", "draft"])
    parser.add_argument("--release-name", default="")
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args(argv)

    transport = dry_run_transport if args.dry_run else http
    try:
        if not args.bundle.is_file():
            raise PlayError(f"{args.bundle}: no such bundle")
        creds = load_credentials(os.environ.get(CREDENTIALS_ENV, ""))
        assertion = make_assertion(creds, int(time.time()))
        token = fetch_token(assertion, transport)
        code = publish(args.package, args.bundle, args.track, args.status,
                       args.release_name, token, transport)
    except PlayError as e:
        print(f"error: {e}", file=sys.stderr)
        return 1
    if args.dry_run:
        print("dry run: nothing was sent")
    else:
        print(f"{args.package}: versionCode {code} ({sha256(args.bundle)}) "
              f"is on the {args.track} track as {args.status}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
