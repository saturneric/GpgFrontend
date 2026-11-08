# Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""Check a published index the way a client does, and further.

  index.json  minisigned by a pinned index key; the trusted comment names
              this product and the serial; schema known; not expired.
  descriptor  its sha256 is the one the index states.
  manifest    SHA256SUMS.txt minisigned by the artifact key, and every
              artifact's sha256 in the descriptor is the one it lists.
  --deep      every artifact downloaded, hashed, and its own .minisig checked.

Reads either a Store (the signing host's read-back) or the public HTTPS bases
from release/product.json, tried in order, as a client would.
"""

from __future__ import annotations

import contextlib
import ssl
import tempfile
import urllib.error
import urllib.request
from pathlib import Path

import imported
import minisig
import openpgp
from common import (INDEX, SCHEMA, SUMS, Product, ReleaseError, check_relpath,
                    load_json, parse_iso, parse_manifest, sha256_bytes,
                    sha256_file, utcnow)
from store import Store


class HttpSource:
    """Product-relative paths over HTTPS bases, first that answers wins."""

    def __init__(self, bases: list[str], slug: str, cafile: str | None = None):
        for base in bases:
            if not base.startswith("https://") or not base.endswith("/"):
                raise ReleaseError(f"{base}: a mirror base is https:// and ends in /")
        self.bases = bases
        self.slug = slug
        self.context = ssl.create_default_context(cafile=cafile)

    def _each(self, path: str, consume):
        errors = []
        for base in self.bases:
            url = f"{base}{self.slug}/{check_relpath(path)}"
            # Its own name: Cloudflare's bot filter answers urllib's default
            # User-Agent with 403, which would read as a broken mirror.
            request = urllib.request.Request(
                url, headers={"User-Agent": "gfrelease-verify/1"})
            try:
                with urllib.request.urlopen(request, timeout=60,
                                            context=self.context) as r:
                    return consume(r)
            except (urllib.error.URLError, OSError) as e:
                errors.append(f"{url}: {e}")
        raise ReleaseError("; ".join(errors))

    def read(self, path: str) -> bytes:
        return self._each(path, lambda r: r.read())

    def fetch_to(self, path: str, local: Path) -> Path:
        """Streamed to disk, as Store.fetch_to: an artifact is never held in
        memory whole."""
        def copy(r):
            with open(local, "wb") as f:
                for chunk in iter(lambda: r.read(1 << 20), b""):
                    f.write(chunk)
            return Path(local)
        return self._each(path, copy)

    def __str__(self) -> str:
        return ", ".join(f"{b}{self.slug}/" for b in self.bases)


def _signed(source, path: str, keys: list[str], tmp: Path) -> tuple[bytes, str]:
    data, sig = source.read(path), source.read(path + ".minisig")
    local = tmp / Path(path).name
    local.write_bytes(data)
    (tmp / (local.name + ".minisig")).write_bytes(sig)
    comment = minisig.verify(keys, local, tmp / (local.name + ".minisig"))
    return data, comment


def check_index(product: Product, index: dict, comment: str) -> None:
    if index.get("schema") != SCHEMA:
        raise ReleaseError(f"index schema {index.get('schema')} is unknown")
    if index.get("product") != product.slug:
        raise ReleaseError(f"the index is for {index.get('product')!r}, "
                           f"not {product.slug!r}")
    want = f"product={product.slug} serial={index.get('serial')}"
    if comment != want:
        raise ReleaseError(f"trusted comment {comment!r} is not {want!r}")
    if parse_iso(index["expires"]) <= utcnow():
        raise ReleaseError(f"the index expired at {index['expires']}")


def verify(product: Product, update, builds, *, index_keys: list[str],
           artifact_key: str, deep: bool = False) -> dict:
    """Nightly files are minisigned with the artifact key; stable ones carry
    a maintainer's OpenPGP signature, checked against the keys
    beside release/product.json (maintainers.asc) with the fingerprints it
    pins."""
    with tempfile.TemporaryDirectory(prefix="gfr-verify-") as tmp_name, \
            contextlib.ExitStack() as stack:
        tmp = Path(tmp_name)
        data, comment = _signed(update, INDEX, index_keys, tmp)
        # The current artifact key, or one it replaced.
        artifact_keys = product.artifact_keys(artifact_key)
        index = load_json(data, INDEX)
        check_index(product, index, comment)
        report = {"product": product.slug, "serial": index["serial"],
                  "expires": index["expires"], "channels": index["channels"],
                  "checked": []}

        ring = None

        def keyring():
            nonlocal ring
            if ring is None:
                ring = stack.enter_context(openpgp.Keyring(
                    product.keyring_path, product.trusted_maintainers))
            return ring

        def fetch(rel: str, name: str) -> Path:
            return builds.fetch_to(check_relpath(rel), tmp / name)

        # The channels name only what the index lists.
        releases = index.get("releases", {})
        channels = index.get("channels", {})
        nightly = index.get("nightly") or {}
        if channels.get("stable") is not None and \
                channels["stable"] not in releases:
            raise ReleaseError(f"channels.stable {channels['stable']} is not "
                               "a listed release")
        if channels.get("nightly") != nightly.get("version"):
            raise ReleaseError(f"channels.nightly {channels.get('nightly')} is "
                               "not the listed nightly")

        entries = [(None, nightly)] if nightly else []
        entries += list(releases.items())
        for listed_as, entry in entries:
            raw = builds.read(check_relpath(entry["descriptor"]))
            if sha256_bytes(raw) != entry["sha256"]:
                raise ReleaseError(f"{entry['descriptor']}: sha256 is not the "
                                   "index's")
            descriptor = load_json(raw, entry["descriptor"])
            if descriptor.get("product") != product.slug:
                raise ReleaseError(f"{entry['descriptor']}: another product's")
            want = listed_as or nightly.get("version")
            if descriptor.get("version") != want:
                raise ReleaseError(f"{entry['descriptor']}: is "
                                   f"{descriptor.get('version')}, listed as "
                                   f"{want}")
            scheme = descriptor["signing"]["scheme"]
            if descriptor.get("kind") == imported.KIND:
                # From before signed manifests: each file carries its own
                # maintainer signature, checked with --deep.
                if scheme != "openpgp":
                    raise ReleaseError(f"{entry['descriptor']}: an imported "
                                       "release is OpenPGP-signed")
                sums = {a["name"]: a["sha256"] for a in descriptor["artifacts"]}
            elif scheme == "minisign":
                sums_data, _ = _signed(builds, check_relpath(descriptor["sums"]),
                                       artifact_keys, tmp)
                sums = parse_manifest(sums_data, descriptor["sums"])
            elif scheme == "openpgp":
                local = fetch(descriptor["sums"], SUMS)
                keyring().verify(local, fetch(descriptor["sums_sig"],
                                              SUMS + ".sig"))
                sums = parse_manifest(local.read_bytes(), descriptor["sums"])
            else:
                raise ReleaseError(f"{entry['descriptor']}: unknown signing "
                                   f"scheme {scheme!r}")
            # A stable or imported release's notes: a maintainer's signature.
            if scheme == "openpgp" and "notes" in entry:
                notes = fetch(entry["notes"], "notes.md")
                if sha256_file(notes) != entry["notes_sha256"]:
                    raise ReleaseError(f"{entry['notes']}: not the index's "
                                       "notes")
                keyring().verify(notes, fetch(entry["notes_sig"],
                                              "notes.md.sig"))
            for artifact in descriptor["artifacts"]:
                if sums.get(artifact["name"]) != artifact["sha256"]:
                    raise ReleaseError(f"{artifact['name']}: the descriptor's "
                                       "sha256 is not the signed manifest's")
                if deep:
                    local = fetch(artifact["path"], artifact["name"])
                    if sha256_file(local) != artifact["sha256"]:
                        raise ReleaseError(f"{artifact['name']}: the download "
                                           "does not match its sha256")
                    sig = fetch(artifact["sig"], artifact["name"] + ".sig")
                    if scheme == "minisign":
                        minisig.verify(artifact_keys, local, sig)
                    else:
                        keyring().verify(local, sig)
                    local.unlink()
            report["checked"].append({"version": descriptor["version"],
                                      "kind": descriptor["kind"],
                                      "artifacts": len(descriptor["artifacts"])})
        return report
