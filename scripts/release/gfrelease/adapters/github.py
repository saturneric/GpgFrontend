# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""A product's repository on GitHub, over its REST API.

Read without a token (an import reads published releases), and written
with one (the sync of stable releases: GitHub Actions' GITHUB_TOKEN, with
contents: write). The token is sent to the API and the upload host only,
never to a URL an answer names elsewhere.
"""

from __future__ import annotations

import http.client
import json
import shutil
import urllib.error
import urllib.parse
import urllib.request
from pathlib import Path

from ..domain.formats import load_json
from ..domain.limits import SMALL_OBJECT_LIMIT
from ..errors import ReleaseError, ToolFailed

API = "https://api.github.com"
UPLOADS = "https://uploads.github.com"
USER_AGENT = "gfrelease/1"
JSON = "application/vnd.github+json"
# The most of an API answer read: a page of releases with their notes.
ANSWER_LIMIT = 32 << 20
PAGES = 50  # of 100 releases each


class _Redirect(urllib.request.HTTPRedirectHandler):
    """Follows a redirect, without the token when it leads to another
    host (a download, or a renamed repository's new place)."""

    def redirect_request(self, req, fp, code, msg, headers, newurl):
        new = super().redirect_request(req, fp, code, msg, headers, newurl)
        if new is not None and urllib.parse.urlsplit(newurl).netloc != \
                urllib.parse.urlsplit(req.full_url).netloc:
            new.remove_header("Authorization")
        return new


_OPENER = urllib.request.build_opener(_Redirect())


class GitHubUnavailable(ToolFailed):
    """GitHub could not be asked, or failed to answer (a network failure,
    a 5xx, a rate limit): the next run may well get through."""


class GitHub:
    """Repository @p repository ("owner/name"), written with @p token."""

    def __init__(self, repository: str, token: str | None = None, *,
                 api: str = API, uploads: str = UPLOADS):
        self.repository = repository
        self.token = token
        self.api = api.rstrip("/")
        self.uploads = uploads.rstrip("/")

    def _open(self, url: str, accept: str, method: str = "GET",
              data=None, headers: dict | None = None):
        """An answer from @p url; the token goes along only to GitHub's own
        hosts (this one's API and upload bases)."""
        h = {"User-Agent": USER_AGENT, "Accept": accept,
             "X-GitHub-Api-Version": "2022-11-28"} | (headers or {})
        if self.token and (url.startswith(self.api + "/") or
                           url.startswith(self.uploads + "/")):
            h["Authorization"] = f"Bearer {self.token}"
        req = urllib.request.Request(url, data=data, headers=h,
                                     method=method)
        return _OPENER.open(req, timeout=300)

    def _call(self, method: str, path: str, body: dict | None = None, *,
              missing_ok: bool = False):
        """The API's JSON answer to @p method @p path (under the repository
        ), None for 204, or, with @p missing_ok,
        for 404."""
        url = f"{self.api}/repos/{self.repository}{path}"
        data = json.dumps(body).encode() if body is not None else None
        headers = {"Content-Type": "application/json"} if data else None
        return self._answer(url, method, data, headers, missing_ok)

    def _answer(self, url: str, method: str, data, headers,
                missing_ok: bool):
        what = f"GitHub: {method} {url}"
        try:
            with self._open(url, JSON, method, data, headers) as r:
                raw = r.read(ANSWER_LIMIT + 1)
                status = r.status
        except urllib.error.HTTPError as e:
            if e.code == 404 and missing_ok:
                return None
            detail = e.read(4096).decode("utf-8", "replace").strip()
            # A rate limit or GitHub's own failure says nothing of the
            # request: worth trying again.
            kind = GitHubUnavailable if e.code >= 500 or e.code == 429 or (
                e.code == 403 and "rate limit" in detail.lower()) \
                else ReleaseError
            raise kind(f"{what}: {e.code} {e.reason}: {detail[:500]}") from e
        except (OSError, http.client.HTTPException) as e:
            raise GitHubUnavailable(f"{what}: {e}") from e
        if len(raw) > ANSWER_LIMIT:
            raise ReleaseError(f"{what}: an answer larger than "
                               f"{ANSWER_LIMIT} bytes")
        if status == 204 or not raw:
            return None
        try:
            return json.loads(raw)
        except (ValueError, UnicodeDecodeError, RecursionError) as e:
            raise ReleaseError(f"{what}: not JSON ({e})") from e

    def release(self, tag: str) -> dict:
        """The published release of @p tag, read without a token."""
        quoted = urllib.parse.quote(tag, safe="")
        url = f"{self.api}/repos/{self.repository}/releases/tags/{quoted}"
        try:
            with self._open(url, JSON) as r:
                data = r.read(SMALL_OBJECT_LIMIT + 1)
        except (OSError, http.client.HTTPException) as e:
            raise ReleaseError(f"GitHub release {tag}: {e}") from e
        release = load_json(data, f"GitHub release {tag}")
        _check_assets(release, f"GitHub release {tag}")
        return release

    def find(self, tag: str) -> dict | None:
        """The release of @p tag, a draft included (only a token that
        writes the repository sees drafts), or None. A published one is
        taken over a draft of the same tag."""
        found = []
        for page in range(1, PAGES + 1):
            batch = self._call("GET", f"/releases?per_page=100&page={page}")
            if not isinstance(batch, list):
                raise ReleaseError("GitHub: the releases are not a list")
            for release in batch:
                if isinstance(release, dict) and \
                        release.get("tag_name") == tag:
                    _check_assets(release, f"GitHub release {tag}")
                    found.append(release)
            if len(batch) < 100:
                break
        found.sort(key=lambda r: bool(r.get("draft")))
        return found[0] if found else None

    def latest_tag(self) -> str | None:
        """The tag of the release GitHub calls the latest."""
        latest = self._call("GET", "/releases/latest", missing_ok=True)
        return latest.get("tag_name") if isinstance(latest, dict) else None

    def tag_commit(self, tag: str) -> str | None:
        """The commit tag @p tag points at (through an annotated tag), or
        None when there is no such tag."""
        quoted = urllib.parse.quote(tag, safe="")
        ref = self._call("GET", f"/git/ref/tags/{quoted}", missing_ok=True)
        if ref is None:
            return None
        target = ref.get("object", {}) if isinstance(ref, dict) else {}
        for _ in range(4):  # a tag of a tag, at most a few deep
            if target.get("type") != "tag":
                break
            tag_object = self._call("GET", f"/git/tags/{target.get('sha')}")
            target = tag_object.get("object", {}) \
                if isinstance(tag_object, dict) else {}
        if target.get("type") != "commit" or \
                not isinstance(target.get("sha"), str):
            raise ReleaseError(f"GitHub: tag {tag} does not point at a "
                               "commit")
        return target["sha"]

    def create(self, tag: str, name: str, body: str) -> dict:
        """A draft release of the existing tag @p tag."""
        release = self._call("POST", "/releases", {
            "tag_name": tag, "name": name, "body": body, "draft": True})
        _check_assets(release, f"GitHub release {tag}")
        return release

    def update(self, release: dict, **fields) -> dict:
        updated = self._call("PATCH", f"/releases/{int(release['id'])}",
                             fields)
        _check_assets(updated, f"GitHub release {release.get('tag_name')}")
        return updated

    def upload(self, release: dict, name: str, path: Path) -> dict:
        """@p path, streamed, as file @p name of @p release."""
        base = str(release.get("upload_url", "")).split("{", 1)[0]
        if not base.startswith(self.uploads + "/"):
            raise ReleaseError(f"GitHub: an upload URL elsewhere: {base!r}")
        url = f"{base}?name={urllib.parse.quote(name, safe='')}"
        size = path.stat().st_size
        with open(path, "rb") as f:
            asset = self._answer(url, "POST", f, {
                "Content-Type": "application/octet-stream",
                "Content-Length": str(size)}, False)
        if not isinstance(asset, dict) or asset.get("name") != name or \
                asset.get("size") != size:
            raise ReleaseError(f"GitHub: {name} was not taken as sent")
        return asset

    def delete_asset(self, asset: dict) -> None:
        self._call("DELETE", f"/releases/assets/{int(asset['id'])}")

    def download(self, url: str, dst: Path) -> None:
        if not url.startswith("https://"):
            raise ReleaseError(f"{dst.name}: not an https download: {url!r}")
        part = dst.with_name(dst.name + ".part")
        try:
            with self._open(url, "application/octet-stream") as r, \
                    open(part, "wb") as f:
                shutil.copyfileobj(r, f, 1 << 20)
        except (OSError, http.client.HTTPException) as e:
            raise ReleaseError(f"{dst.name}: {e}") from e
        part.replace(dst)

    def page(self, tag: str) -> str:
        return f"https://github.com/{self.repository}/releases/tag/{tag}"


def _check_assets(release, what: str) -> None:
    """Everything read of @p release has the type it is read as."""
    if not isinstance(release, dict):
        raise ReleaseError(f"{what}: not a release")
    assets = release.get("assets", [])
    if not isinstance(assets, list) or not all(
            isinstance(a, dict) and isinstance(a.get("name"), str) and
            isinstance(a.get("browser_download_url"), str) and
            isinstance(a.get("size"), int) for a in assets):
        raise ReleaseError(f"{what}: not a list of assets")
