# Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""Releases made before this pipeline, brought over from GitHub Releases.

They were built and uploaded by hand: no Sigstore provenance, no signed
manifest, a lightweight tag. What they do have is a maintainer's detached
OpenPGP signature on every file, made when they were released. That is what
an imported release rests on, and nothing is signed again:

  1. every file of GitHub release v<version> that the product's artifact
     rules know, with its `.sig` (files no rule knows are left out, and said);
  2. each signature checked against the pinned maintainer keys, and kept
     exactly as it is;
  3. the tag resolved in a clone, and git asked which version it is;
  4. everything uploaded to releases/<version>/, the descriptor last.

The signing host admits an imported release by checking every file against
its `.sig` itself (index.admit_imported), once. Nothing here needs a secret
key: it runs wherever the token for the artifact store is. Releases before
policy `signing.legacy_unsigned_before` were signed with keys no longer
pinned and are not imported.
"""

from __future__ import annotations

import json
import urllib.request
from pathlib import Path

import openpgp
from common import (DESCRIPTOR, SCHEMA, Product, ReleaseError, canonical_json,
                    iso, load_json, parse_iso, parse_version, sha256_file,
                    tagged_commit, utcnow)
from store import Store

KIND = "imported"
USER_AGENT = "gfrelease-import/1"


class GitHubSource:
    """A product's releases on GitHub, read without a token."""

    def __init__(self, repository: str, api: str = "https://api.github.com"):
        self.repository = repository
        self.api = api

    def _open(self, url: str, accept: str):
        req = urllib.request.Request(url, headers={"User-Agent": USER_AGENT,
                                                   "Accept": accept})
        return urllib.request.urlopen(req, timeout=60)

    def release(self, tag: str) -> dict:
        url = f"{self.api}/repos/{self.repository}/releases/tags/{tag}"
        try:
            with self._open(url, "application/vnd.github+json") as r:
                return json.load(r)
        except OSError as e:
            raise ReleaseError(f"GitHub release {tag}: {e}") from e

    def download(self, url: str, dst: Path) -> None:
        part = dst.with_name(dst.name + ".part")
        try:
            with self._open(url, "application/octet-stream") as r, \
                    open(part, "wb") as f:
                for chunk in iter(lambda: r.read(1 << 20), b""):
                    f.write(chunk)
        except OSError as e:
            raise ReleaseError(f"{dst.name}: {e}") from e
        part.replace(dst)

    def page(self, tag: str) -> str:
        return f"https://github.com/{self.repository}/releases/tag/{tag}"


def _floor(product: Product):
    return parse_version(product.policy["signing"]["legacy_unsigned_before"])


class Import:
    def __init__(self, product: Product, version: str, work: Path, *,
                 builds: Store | None, git_dir: Path,
                 source: GitHubSource | None = None):
        v = parse_version(version)
        if v.prerelease or v.build:
            raise ReleaseError(f"{version}: only final releases are imported")
        if v < _floor(product):
            raise ReleaseError(f"{version} is older than "
                               f"{_floor(product)}, the first release signed "
                               "with a pinned key")
        self.product = product
        self.version = str(v)
        self.tag = f"v{self.version}"
        self.dir = Path(work) / self.version
        self.dir.mkdir(parents=True, exist_ok=True)
        self.builds, self.git_dir = builds, git_dir
        self.source = source or GitHubSource(product.repository)
        self.skipped: list[str] = []

    @property
    def prefix(self) -> str:
        return f"releases/{self.version}"

    def _select(self, release: dict) -> dict[str, dict]:
        """The files to bring over: name -> classification plus the GitHub
        asset and its signature's asset."""
        assets = {a["name"]: a for a in release.get("assets", [])}
        chosen = {}
        for name, asset in sorted(assets.items()):
            if name.endswith(".sig") or name.endswith(".asc"):
                continue
            try:
                info = self.product.classify(name, self.version)
            except ReleaseError:
                self.skipped.append(f"{name} (no artifact rule)")
                continue
            sig = assets.get(name + ".sig")
            if sig is None:
                self.skipped.append(f"{name} (no signature)")
                continue
            chosen[name] = {**info, "asset": asset, "sig_asset": sig}
        if not chosen:
            raise ReleaseError(f"{self.tag}: no signed file the product knows")
        return chosen

    def _fetch(self, name: str, asset: dict) -> Path:
        local = self.dir / name
        if not (local.is_file() and local.stat().st_size == asset["size"]):
            self.source.download(asset["browser_download_url"], local)
        if local.stat().st_size != asset["size"]:
            raise ReleaseError(f"{name}: {local.stat().st_size} bytes, GitHub "
                               f"says {asset['size']}")
        return local

    def run(self) -> dict:
        release = self.source.release(self.tag)
        if release.get("tag_name") != self.tag or release.get("draft"):
            raise ReleaseError(f"GitHub has no published release {self.tag}")
        files = self._select(release)

        # The tag, and that git calls its commit this version.
        commit = tagged_commit(self.git_dir, self.tag, version=self.version)

        # Every file against its original signature, before anything leaves.
        signers = set()
        with openpgp.Keyring(self.product.keyring_path,
                             self.product.trusted_maintainers) as ring:
            for name, info in files.items():
                local = self._fetch(name, info["asset"])
                sig = self._fetch(name + ".sig", info["sig_asset"])
                signers.add(ring.verify(local, sig))
                info["sha256"] = sha256_file(local)
                info["size"] = local.stat().st_size
        if len(signers) != 1:
            raise ReleaseError(f"{self.tag}: files signed by several keys "
                               f"{sorted(signers)}")

        artifacts = []
        p = self.prefix
        for name in sorted(files):
            info = files[name]
            if self.builds is not None:
                self.builds.put_file(self.dir / name, f"{p}/{name}")
                self.builds.put_file(self.dir / (name + ".sig"),
                                     f"{p}/{name}.sig")
            artifacts.append({
                k: info[k] for k in ("os", "arch", "kind", "flavor", "min_os",
                                     "sha256", "size") if k in info} | {
                "name": name, "path": f"{p}/{name}", "sig": f"{p}/{name}.sig"})

        released = release.get("published_at") or ""
        descriptor = {
            "schema": SCHEMA, "product": self.product.slug, "kind": KIND,
            "version": self.version,
            "series": parse_version(self.version).series,
            "ref": f"refs/tags/{self.tag}", "commit": commit,
            "released_at": iso(parse_iso(released)) if released else None,
            "imported_at": iso(utcnow()),
            "source": self.source.page(self.tag),
            "signing": {"scheme": "openpgp", "fingerprint": signers.pop()},
            "artifacts": artifacts,
        }
        # A rerun keeps the first import's time, so the descriptor's bytes
        # (immutable once uploaded) come out the same.
        local = self.dir / DESCRIPTOR
        previous = None
        if self.builds is not None and self.builds.exists(f"{p}/{DESCRIPTOR}"):
            previous = load_json(self.builds.read(f"{p}/{DESCRIPTOR}"),
                                 f"{p}/{DESCRIPTOR}")
        elif local.is_file():
            previous = load_json(local.read_bytes(), str(local))
        if previous and previous.get("imported_at"):
            descriptor["imported_at"] = previous["imported_at"]
        local.write_bytes(canonical_json(descriptor))
        if self.builds is not None:
            # Last: a descriptor only ever names objects already there.
            self.builds.put_bytes(canonical_json(descriptor),
                                  f"{p}/{DESCRIPTOR}")
        return descriptor
