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
import json
import ssl
import tempfile
import urllib.error
import urllib.request
from pathlib import Path

import minisig
import openpgp
from common import (INDEX, SCHEMA, Product, ReleaseError, check_relpath,
                    parse_iso, sha256_bytes, sha256_file, utcnow)
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

    def read(self, path: str) -> bytes:
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
                    return r.read()
            except (urllib.error.URLError, OSError) as e:
                errors.append(f"{url}: {e}")
        raise ReleaseError("; ".join(errors))

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


def parse_sums(data: bytes) -> dict[str, str]:
    out = {}
    for line in data.decode("ascii").splitlines():
        digest, name = line.split("  ", 1)
        out[name] = digest
    return out


def verify(product: Product, update, builds, *, index_keys: list[str],
           artifact_key: str, deep: bool = False) -> dict:
    """Beta and nightly files are minisigned with the artifact key; stable
    ones carry a maintainer's OpenPGP signature, checked against the keys
    beside release/product.json (maintainers.asc) with the fingerprints it
    pins."""
    with tempfile.TemporaryDirectory(prefix="gfr-verify-") as tmp_name, \
            contextlib.ExitStack() as stack:
        tmp = Path(tmp_name)
        data, comment = _signed(update, INDEX, index_keys, tmp)
        index = json.loads(data)
        check_index(product, index, comment)
        report = {"product": product.slug, "serial": index["serial"],
                  "expires": index["expires"], "channels": index["channels"],
                  "checked": []}

        ring = None

        def keyring():
            nonlocal ring
            if ring is None:
                ring = stack.enter_context(openpgp.Keyring(
                    product.keyring_path, product.maintainers))
            return ring

        def fetch(rel: str, name: str) -> Path:
            local = tmp / name
            local.write_bytes(builds.read(check_relpath(rel)))
            return local

        entries = []
        if index.get("nightly"):
            entries.append(index["nightly"])
        entries += list(index.get("releases", {}).values())
        for entry in entries:
            raw = builds.read(check_relpath(entry["descriptor"]))
            if sha256_bytes(raw) != entry["sha256"]:
                raise ReleaseError(f"{entry['descriptor']}: sha256 is not the "
                                   "index's")
            descriptor = json.loads(raw)
            if descriptor.get("product") != product.slug:
                raise ReleaseError(f"{entry['descriptor']}: another product's")
            scheme = descriptor["signing"]["scheme"]
            if descriptor.get("kind") == "imported":
                # From before signed manifests: each file carries its own
                # maintainer signature, checked with --deep.
                if scheme != "openpgp":
                    raise ReleaseError(f"{entry['descriptor']}: an imported "
                                       "release is OpenPGP-signed")
                sums_data = "".join(
                    f"{a['sha256']}  {a['name']}\n"
                    for a in descriptor["artifacts"]).encode()
            elif scheme == "minisign":
                sums_data, _ = _signed(builds, check_relpath(descriptor["sums"]),
                                       [artifact_key], tmp)
            elif scheme == "openpgp":
                sums = fetch(descriptor["sums"], "SHA256SUMS.txt")
                keyring().verify(sums, fetch(descriptor["sums_sig"],
                                             "SHA256SUMS.txt.sig"))
                sums_data = sums.read_bytes()
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
            sums = parse_sums(sums_data)
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
                        minisig.verify([artifact_key], local, sig)
                    else:
                        keyring().verify(local, sig)
                    local.unlink()
            report["checked"].append({"version": descriptor["version"],
                                      "kind": descriptor["kind"],
                                      "artifacts": len(descriptor["artifacts"])})
        return report
