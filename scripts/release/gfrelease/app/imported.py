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
  4. everything handed to the signing host: uploaded to the staging
     store, under signed/releases/<version>/, the descriptor last.

The signing host receives an imported release (promote.py) and admits it by
checking every file against its `.sig` itself (index.admit_imported).
Nothing here needs a secret key: it runs wherever a token that writes the
staging store is. Releases before
policy `signing.legacy_unsigned_before` were signed with keys no longer
pinned and are not imported.
"""

from __future__ import annotations

from pathlib import Path

from ..adapters import openpgp
from ..adapters.clock import utcnow
from ..adapters.files import atomic_write, sha256_file
from ..adapters.git import tag_time, tagged_commit
from ..adapters.github import GitHub
from ..adapters.store import Store
from ..domain import descriptor as descriptors
from ..domain import layout, refs
from ..domain.formats import (canonical_json, iso, load_json, parse_iso,
                              parse_version)
from ..domain.layout import DESCRIPTOR, SIG
from ..errors import ReleaseError
from .product import Product

class Import:
    def __init__(self, product: Product, version: str, work: Path, *,
                 staging: Store | None, git_dir: Path,
                 source: GitHub | None = None):
        v = parse_version(version)
        if not v.is_release:
            raise ReleaseError(f"{version}: only final releases are imported")
        if v < product.legacy_floor:
            raise ReleaseError(f"{version} is older than "
                               f"{product.legacy_floor}, the first release signed "
                               "with a pinned key")
        self.product = product
        self.version = str(v)
        self.tag = refs.release_tag(self.version)
        self.dir = Path(work) / self.version
        self.dir.mkdir(parents=True, exist_ok=True)
        # Where it is handed to the host; None only downloads and checks.
        self.outbox = staging.sub(layout.SIGNED) if staging else None
        self.git_dir = git_dir
        self.source = source or GitHub(product.repository)
        self.skipped: list[str] = []

    @property
    def prefix(self) -> str:
        return layout.release_dir(self.version)

    def _select(self, release: dict) -> dict[str, dict]:
        """The files to bring over: name -> classification plus the GitHub
        asset and its signature's asset."""
        assets = {a["name"]: a for a in release.get("assets", [])}
        chosen = {}
        for name, asset in sorted(assets.items()):
            if name.endswith(SIG) or name.endswith(".asc"):
                continue
            try:
                info = self.product.classify(name, self.version)
            except ReleaseError:
                self.skipped.append(f"{name} (no artifact rule)")
                continue
            sig = assets.get(name + SIG)
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
        tagged_at = tag_time(self.git_dir, self.tag)

        # Every file against its original signature, before anything leaves.
        signers = set()
        with openpgp.Keyring.of(self.product) as ring:
            for name, info in files.items():
                local = self._fetch(name, info["asset"])
                sig = self._fetch(name + SIG, info["sig_asset"])
                signed = ring.verify_signature(local, sig)
                # The host refuses a file signed before its tag (an older
                # release's, relabelled); so does the import, before any
                # upload.
                if signed["created"] < tagged_at:
                    raise ReleaseError(
                        f"{name}: signed before {self.tag} was made")
                signers.add(signed["primary"])
                info["sha256"] = sha256_file(local)
                info["size"] = local.stat().st_size
        if len(signers) != 1:
            raise ReleaseError(f"{self.tag}: files signed by several keys "
                               f"{sorted(signers)}")

        artifacts = []
        p = self.prefix
        for name in sorted(files):
            info = files[name]
            if self.outbox is not None:
                self.outbox.put_file(self.dir / name, f"{p}/{name}")
                self.outbox.put_file(self.dir / (name + SIG),
                                     layout.sig_of(f"{p}/{name}"))
            artifacts.append(descriptors.classified(info) | {
                k: info[k] for k in ("sha256", "size") if k in info} | {
                "name": name, "path": f"{p}/{name}",
                "sig": layout.sig_of(f"{p}/{name}")})

        released = release.get("published_at") or ""
        descriptor = descriptors.make(
            layout.KIND_IMPORTED, self.product.slug, self.version,
            refs.release_tag_ref(self.version), commit,
            {"scheme": layout.SCHEME_OPENPGP, "fingerprint": signers.pop()},
            artifacts,
            released_at=iso(parse_iso(released)) if released else None,
            imported_at=iso(utcnow()), source=self.source.page(self.tag))
        # A rerun keeps the first import's time, so the descriptor's bytes
        # (immutable once uploaded) come out the same.
        local = self.dir / DESCRIPTOR
        previous = None
        remote = layout.descriptor_of(p)
        if self.outbox is not None and self.outbox.exists(remote):
            previous = load_json(self.outbox.read(remote), remote)
        elif local.is_file():
            previous = load_json(local.read_bytes(), str(local))
        if previous and previous.get("imported_at"):
            descriptor["imported_at"] = previous["imported_at"]
        atomic_write(local, canonical_json(descriptor))
        if self.outbox is not None:
            # Last: a descriptor only ever names objects already there.
            self.outbox.put_bytes(canonical_json(descriptor), remote)
        return descriptor
