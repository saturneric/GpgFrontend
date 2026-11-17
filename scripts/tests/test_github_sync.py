#!/usr/bin/env python3
# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""A stable release admitted by the host (test_gfrelease's StableTest),
mirrored to a GitHub that answers as GitHub's REST API does
(github_sync): made as a draft, filled with checked files, published; left
alone once it is so; and refused, with nothing published, when a file does
not check or GitHub has something else.

Run: python3 -m unittest discover -s scripts/tests
"""

import hashlib
import http.server
import itertools
import json
import shutil
import threading
import unittest
import urllib.parse

import _paths  # noqa: F401
from gfrelease.adapters.github import GitHub
from gfrelease.app import github_sync
from gfrelease.domain import github_release as gh
from test_gfrelease import StableTest

REPO = "example/product"


class FakeGitHub:
    """The parts of GitHub's REST API the sync uses, in memory: releases
    with their assets, the latest release, and tags. Every write is
    recorded in .writes."""

    def __init__(self):
        self.releases: list[dict] = []
        self.tags: dict[str, str] = {}  # tag -> commit
        self.latest = None  # a release's id
        self.writes: list[str] = []
        self.make_latest: list = []  # what each update asked
        self.fail_upload_after = None  # uploads taken before one fails
        self.ids = itertools.count(1)
        fake = self

        class Handler(http.server.BaseHTTPRequestHandler):
            def log_message(self, *args):
                pass

            def answer(self, status, body=None):
                data = b"" if body is None else json.dumps(body).encode()
                self.send_response(status)
                self.send_header("Content-Type", "application/json")
                self.send_header("Content-Length", str(len(data)))
                self.end_headers()
                self.wfile.write(data)

            def body(self):
                n = int(self.headers.get("Content-Length") or 0)
                return self.rfile.read(n)

            def do_GET(self):
                self.answer(*fake.get(self.path))

            def do_POST(self):
                self.answer(*fake.post(self.path, self.body(),
                                       self.headers.get("Authorization")))

            def do_PATCH(self):
                self.answer(*fake.patch(self.path, json.loads(self.body())))

            def do_DELETE(self):
                self.answer(*fake.delete(self.path))

        self.server = http.server.ThreadingHTTPServer(("127.0.0.1", 0),
                                                      Handler)
        threading.Thread(target=self.server.serve_forever,
                         daemon=True).start()
        self.base = f"http://127.0.0.1:{self.server.server_address[1]}"

    def close(self):
        self.server.shutdown()
        self.server.server_close()

    def client(self) -> GitHub:
        return GitHub(REPO, "token", api=self.base,
                      uploads=f"{self.base}/uploads")

    def _release(self, rid: int) -> dict | None:
        return next((r for r in self.releases if r["id"] == rid), None)

    def by_tag(self, tag: str) -> dict | None:
        return next((r for r in self.releases if r["tag_name"] == tag), None)

    def get(self, path):
        url = urllib.parse.urlsplit(path)
        p = url.path.removeprefix(f"/repos/{REPO}")
        if p == "/releases":
            page = int(urllib.parse.parse_qs(url.query)["page"][0])
            return 200, self.releases[(page - 1) * 100:page * 100]
        if p == "/releases/latest":
            latest = self._release(self.latest)
            return (200, latest) if latest else (404, {"message": "no"})
        if p.startswith("/git/ref/tags/"):
            tag = p.removeprefix("/git/ref/tags/")
            if tag not in self.tags:
                return 404, {"message": "Not Found"}
            # An annotated tag, as a signed tag is.
            return 200, {"object": {"type": "tag", "sha": "t" + tag}}
        if p.startswith("/git/tags/t"):
            return 200, {"object": {"type": "commit",
                                    "sha": self.tags[p[len("/git/tags/t"):]]}}
        return 404, {"message": "Not Found"}

    def post(self, path, data, authorization):
        self.writes.append(f"POST {path}")
        if authorization != "Bearer token":
            return 401, {"message": "Bad credentials"}
        url = urllib.parse.urlsplit(path)
        if url.path == f"/repos/{REPO}/releases":
            body = json.loads(data)
            if body["tag_name"] not in self.tags:
                return 422, {"message": "tag_name is not a tag"}
            rid = next(self.ids)
            release = {"id": rid, "tag_name": body["tag_name"],
                       "name": body["name"], "body": body["body"],
                       "draft": body.get("draft", False), "assets": [],
                       "upload_url": f"{self.base}/uploads/repos/{REPO}/"
                                     f"releases/{rid}/assets{{?name,label}}"}
            self.releases.insert(0, release)
            return 201, release
        rid = int(url.path.split("/")[-2])
        name = urllib.parse.parse_qs(url.query)["name"][0]
        if self.fail_upload_after is not None:
            if self.fail_upload_after == 0:
                return 502, {"message": "Bad Gateway"}
            self.fail_upload_after -= 1
        asset = {"id": next(self.ids), "name": name, "size": len(data),
                 "state": "uploaded",
                 "digest": "sha256:" + hashlib.sha256(data).hexdigest(),
                 "browser_download_url": f"https://example.org/{name}",
                 "data": data.decode("latin-1")}
        self._release(rid)["assets"].append(asset)
        return 201, asset

    def patch(self, path, body):
        self.writes.append(f"PATCH {path} {sorted(body)}")
        self.make_latest.append(body.get("make_latest"))
        release = self._release(int(path.split("/")[-1]))
        for key in ("name", "body", "draft"):
            if key in body:
                release[key] = body[key]
        if not release["draft"] and body.get("make_latest") == "true":
            self.latest = release["id"]
        return 200, release

    def delete(self, path):
        self.writes.append(f"DELETE {path}")
        aid = int(path.split("/")[-1])
        for release in self.releases:
            release["assets"] = [a for a in release["assets"]
                                 if a["id"] != aid]
        return 204, None


class GitHubSyncTest(StableTest):

    def setUp(self):
        super().setUp()
        self.github = FakeGitHub()
        self.addCleanup(self.github.close)

    def sync(self, **kw):
        return github_sync.sync(self.product, self.update, self.builds,
                                self.github.client(), **kw)

    def tagged(self, version="2.3.0"):
        self.github.tags[f"v{version}"] = self.git(
            "rev-parse", f"v{version}^{{commit}}")

    def stored(self, path: str) -> bytes:
        return (self.tmp / "builds" / self.product.slug / path).read_bytes()

    def test_a_stable_release_is_published_with_every_checked_file(self):
        self.admitted()
        self.tagged()
        d = json.loads(self.stored("releases/2.3.0/release.json"))
        report = self.sync()
        self.assertEqual(report["problems"], [])
        [one] = report["releases"]
        self.assertEqual(one["state"], "synced", one)
        release = self.github.by_tag("v2.3.0")
        self.assertFalse(release["draft"])
        self.assertEqual(self.github.latest, release["id"])
        self.assertEqual(release["name"], "v2.3.0")
        self.assertEqual(release["body"], "- Everything, signed.\n")
        want = {}
        for a in d["artifacts"]:
            want[a["name"]] = a["path"]
            want[a["name"] + ".sig"] = a["sig"]
        got = {a["name"]: a["data"].encode("latin-1")
               for a in release["assets"]}
        self.assertEqual(sorted(got), sorted(want))
        for name, path in want.items():
            self.assertEqual(got[name], self.stored(path), name)

        # Again: nothing to do, and nothing written.
        writes = len(self.github.writes)
        report = self.sync()
        self.assertEqual([r["state"] for r in report["releases"]],
                         ["unchanged"])
        self.assertEqual(len(self.github.writes), writes)

    def test_a_dry_run_only_plans(self):
        self.admitted()
        self.tagged()
        [one] = self.sync(dry_run=True)["releases"]
        self.assertEqual(one["state"], "planned")
        self.assertEqual(one["steps"][0], "create")
        self.assertEqual(one["steps"][-1], "publish")
        self.assertEqual(self.github.writes, [])

    def test_a_file_that_does_not_check_is_never_published(self):
        self.admitted()
        self.tagged()
        d = json.loads(self.stored("releases/2.3.0/release.json"))
        msi = next(a for a in d["artifacts"] if a["kind"] == "msi")
        root = self.tmp / "builds" / self.product.slug
        pristine = self.tmp / "pristine"
        shutil.copytree(root, pristine)
        for what, damage in {
                "other bytes": lambda: (root / msi["path"]).write_bytes(
                    b"not the release"),
                "another file's signature": lambda: (root / msi["sig"])
                .write_bytes((root / d["sums_sig"]).read_bytes())}.items():
            with self.subTest(what):
                shutil.rmtree(root)
                shutil.copytree(pristine, root)
                damage()
                self.github.releases.clear()
                report = self.sync()
                self.assertEqual(report["releases"][0]["state"], "refused")
                self.assertIn(msi["name"], report["problems"][0])
                release = self.github.by_tag("v2.3.0")
                # A draft at most, which only the repository's writers see,
                # and without the file.
                self.assertTrue(release is None or release["draft"])
                self.assertNotIn(msi["name"], [
                    a["name"] for a in (release or {}).get("assets", [])])

    def test_without_its_tag_on_github_nothing_is_made(self):
        self.admitted()
        report = self.sync()
        self.assertIn("GitHub has no tag v2.3.0", report["problems"][0])
        self.github.tags["v2.3.0"] = "0" * 40
        report = self.sync()
        self.assertIn("points at " + "0" * 40, report["problems"][0])
        self.assertEqual(self.github.writes, [])

    def test_an_interrupted_upload_is_finished_by_the_next_run(self):
        self.admitted()
        self.tagged()
        self.github.fail_upload_after = 3
        report = self.sync()
        self.assertEqual(report["releases"][0]["state"], "refused")
        self.assertTrue(report["problems"].passing)
        release = self.github.by_tag("v2.3.0")
        self.assertTrue(release["draft"])
        self.assertEqual(len(release["assets"]), 3)
        self.github.fail_upload_after = None
        self.assertEqual(self.sync()["releases"][0]["state"], "synced")
        release = self.github.by_tag("v2.3.0")
        self.assertFalse(release["draft"])
        self.assertEqual(len(self.github.releases), 1)

    def test_a_published_file_with_other_bytes_is_refused(self):
        self.admitted()
        self.tagged()
        self.sync()
        release = self.github.by_tag("v2.3.0")
        release["assets"][0]["digest"] = "sha256:" + "f" * 64
        writes = len(self.github.writes)
        report = self.sync()
        self.assertEqual(report["releases"][0]["state"], "refused")
        self.assertIn("is not the published file", report["problems"][0])
        self.assertFalse(report["problems"].passing)
        self.assertEqual(len(self.github.writes), writes)

    def test_changed_notes_rewrite_the_body_and_nothing_else(self):
        self.admitted()
        self.tagged()
        self.sync()
        release = self.github.by_tag("v2.3.0")
        release["body"] = "edited by hand"
        writes = len(self.github.writes)
        [one] = self.sync()["releases"]
        self.assertEqual(one["steps"], ["edit"])
        self.assertEqual(self.github.writes[writes:],
                         [f"PATCH /repos/{REPO}/releases/{release['id']} "
                          "['body', 'make_latest', 'name']"])
        self.assertTrue(release["body"].startswith("- Everything"))

    def test_a_version_the_index_does_not_list_is_a_problem(self):
        self.admitted()
        report = self.sync(versions=["9.9.9"])
        self.assertEqual(report["releases"], [])
        self.assertIn("9.9.9: not a release the index lists",
                      report["problems"])


class LatestTest(unittest.TestCase):
    """What the sync asks of GitHub for a release that is not the stable
    channel's (2.1.14 published after 2.2.5): never to make it the
    latest."""

    def setUp(self):
        self.github = FakeGitHub()
        self.addCleanup(self.github.close)
        self.github.tags = {"v2.2.5": "a" * 40, "v2.1.14": "b" * 40}
        client = self.github.client()
        latest = client.create("v2.2.5", "v2.2.5", "notes")
        client.update(latest, draft=False, make_latest="true")
        self.latest = latest["id"]

    def release(self, latest: bool) -> gh.Release:
        return gh.Release(version="2.1.14", tag="v2.1.14", name="v2.1.14",
                          commit="b" * 40, body="notes", latest=latest,
                          assets=())

    def carry_out(self, want):
        client = self.github.client()
        have = client.find(want.tag)
        steps = gh.plan(want, have, client.latest_tag())
        github_sync._carry_out(want, have, steps, client, {"artifacts": []},
                               {}, None, None)
        return steps

    def test_an_older_series_release_is_published_not_as_the_latest(self):
        steps = self.carry_out(self.release(False))
        self.assertEqual([s.kind for s in steps], ["create", "publish"])
        self.assertFalse(self.github.by_tag("v2.1.14")["draft"])
        self.assertEqual(self.github.latest, self.latest)
        # Said, not left out: GitHub makes a release it publishes the
        # latest unless told otherwise.
        self.assertEqual(self.github.make_latest[-1], "false")
        # Its notes changed later: still not the latest.
        self.github.by_tag("v2.1.14")["body"] = "old"
        self.assertEqual([s.kind for s in self.carry_out(
            self.release(False))], ["edit"])
        self.assertEqual(self.github.make_latest[-1], "false")
        self.assertEqual(self.github.latest, self.latest)

    def test_the_stable_channels_release_is(self):
        self.carry_out(self.release(True))
        self.assertEqual(self.github.latest,
                         self.github.by_tag("v2.1.14")["id"])


def load_tests(loader, tests, pattern):
    """Only this file's tests: StableTest's own run in test_gfrelease."""
    suite = unittest.TestSuite(
        GitHubSyncTest(name) for name in sorted(vars(GitHubSyncTest))
        if name.startswith("test_"))
    suite.addTests(loader.loadTestsFromTestCase(LatestTest))
    return suite


if __name__ == "__main__":
    unittest.main()
