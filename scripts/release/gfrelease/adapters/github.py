# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""A product's repository on GitHub, over its REST API."""

from __future__ import annotations

import http.client
import shutil
import urllib.request
from pathlib import Path

from ..domain.formats import load_json
from ..domain.limits import SMALL_OBJECT_LIMIT
from ..errors import ReleaseError

API = "https://api.github.com"
USER_AGENT = "gfrelease-import/1"


class GitHub:
    """A product's releases on GitHub, read without a token."""

    def __init__(self, repository: str):
        self.repository = repository

    def _open(self, url: str, accept: str):
        req = urllib.request.Request(url, headers={"User-Agent": USER_AGENT,
                                                   "Accept": accept})
        return urllib.request.urlopen(req, timeout=60)

    def release(self, tag: str) -> dict:
        url = f"{API}/repos/{self.repository}/releases/tags/{tag}"
        try:
            with self._open(url, "application/vnd.github+json") as r:
                data = r.read(SMALL_OBJECT_LIMIT + 1)
        except (OSError, http.client.HTTPException) as e:
            raise ReleaseError(f"GitHub release {tag}: {e}") from e
        release = load_json(data, f"GitHub release {tag}")
        # Everything read of it below has the type it is read as.
        assets = release.get("assets", [])
        if not isinstance(assets, list) or not all(
                isinstance(a, dict) and isinstance(a.get("name"), str) and
                isinstance(a.get("browser_download_url"), str) and
                isinstance(a.get("size"), int) for a in assets):
            raise ReleaseError(f"GitHub release {tag}: not a list of assets")
        return release

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
