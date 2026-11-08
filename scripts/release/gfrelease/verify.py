# Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""Check a published index the way a client does, and further.

  index.json  minisigned by a pinned index key; the trusted comment names
              this product and the serial; schema known; not expired; its
              channels name only what it lists.
  descriptor  its sha256 is the one the index states.
  manifest    SHA256SUMS.txt minisigned by the artifact key (a nightly's by
              promotion, a stable or imported release's by the server's
              countersignature), and every artifact's sha256 in the
              descriptor is the one it lists.
  notes       their sha256 is the index's, and the countersignature holds.
  --deep      every artifact downloaded, hashed, and its .minisig checked;
              for a stable or imported release also the maintainer's OpenPGP
              .sig on it, on the manifest and on the notes.

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

import countersign
import imported
import minisig
import openpgp
from common import (INDEX, SCHEMA, SUMS, Product, ReleaseError, check_relpath,
                    load_json, parse_iso, parse_manifest, parse_version,
                    sha256_bytes, sha256_file, utcnow)
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
        # No channel offers what the index itself says is withdrawn.
        withdrawn = {parse_version(w.get("version"), "withdrawn")
                     for w in index.get("withdrawn", []) if isinstance(w, dict)}
        for channel, version in channels.items():
            if version is not None and \
                    parse_version(version, f"channels.{channel}") in withdrawn:
                raise ReleaseError(f"channels.{channel} offers {version}, "
                                   "which is withdrawn")

        # A nightly is main's build: the descriptor's ref says so.
        entries = [(None, nightly, "refs/heads/main")] if nightly else []
        entries += [(v, e, None) for v, e in releases.items()]
        for listed_as, entry, ref in entries:
            raw = builds.read(check_relpath(entry["descriptor"]))
            if sha256_bytes(raw) != entry["sha256"]:
                raise ReleaseError(f"{entry['descriptor']}: sha256 is not the "
                                   "index's")
            descriptor = load_json(raw, entry["descriptor"])
            if descriptor.get("product") != product.slug:
                raise ReleaseError(f"{entry['descriptor']}: another product's")
            want = listed_as or entry.get("version")
            if ref is not None and not str(descriptor.get("ref", "")) \
                    .startswith(ref):
                raise ReleaseError(f"{entry['descriptor']}: built from "
                                   f"{descriptor.get('ref')}, listed as a "
                                   f"build of {ref}")
            if descriptor.get("version") != want:
                raise ReleaseError(f"{entry['descriptor']}: is "
                                   f"{descriptor.get('version')}, listed as "
                                   f"{want}")
            scheme = descriptor.get("signing", {}).get("scheme")
            kind = descriptor.get("kind")
            if kind == "nightly":
                if scheme != "minisign":
                    raise ReleaseError(f"{entry['descriptor']}: a nightly is "
                                       "minisigned")
                sums_data, _ = _signed(builds, check_relpath(descriptor["sums"]),
                                       artifact_keys, tmp)
                sums = parse_manifest(sums_data, descriptor["sums"])
            elif kind in ("stable", imported.KIND):
                if scheme != "openpgp":
                    raise ReleaseError(f"{entry['descriptor']}: a {kind} "
                                       "release is signed by a maintainer")
                # What the application relies on: the release server's
                # countersignature, made after it checked the maintainer's.
                manifest = countersign.manifest_path(descriptor)
                if entry.get("sums_minisig") != manifest + countersign.MINISIG:
                    raise ReleaseError(f"{entry['descriptor']}: listed without "
                                       "the server's countersignature")
                sums_data, _ = _signed(builds, manifest, artifact_keys, tmp)
                sums = parse_manifest(sums_data, manifest)
                # When each file's maintainer signature was made, as the
                # server admitted it: one per file, nothing else.
                signatures = entry.get("signatures")
                if not isinstance(signatures, dict) or set(signatures) != \
                        {a["name"] for a in descriptor["artifacts"]}:
                    raise ReleaseError(f"{entry['descriptor']}: listed without "
                                       "a signature time for every file")
                for name, sig in signatures.items():
                    if not isinstance(sig, dict) or \
                            not isinstance(sig.get("signer"), str):
                        raise ReleaseError(f"{name}: no signer recorded")
                    parse_iso(sig.get("created"))
                # The maintainer's own signature on the manifest: what a
                # person checks by hand. An imported release has none.
                if deep and descriptor.get("sums_sig"):
                    local = fetch(manifest, SUMS)
                    keyring().verify(local, fetch(descriptor["sums_sig"],
                                                  SUMS + ".sig"))
            else:
                raise ReleaseError(f"{entry['descriptor']}: unknown kind "
                                   f"{kind!r}")
            # A release's notes: the index pins their hash; the server's
            # countersignature is checked always, the maintainer's with --deep.
            if "notes" in entry:
                notes = fetch(entry["notes"], "notes.md")
                if sha256_file(notes) != entry["notes_sha256"]:
                    raise ReleaseError(f"{entry['notes']}: not the index's "
                                       "notes")
                if entry.get("notes_minisig") != \
                        entry["notes"] + countersign.MINISIG:
                    raise ReleaseError(f"{entry['notes']}: listed without "
                                       "the server's countersignature")
                minisig.verify(artifact_keys, notes,
                               fetch(entry["notes_minisig"],
                                     "notes.md.minisig"))
                if deep:
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
                    if kind == "nightly":
                        minisig.verify(artifact_keys, local,
                                       fetch(artifact["sig"],
                                             artifact["name"] + ".minisig"))
                    else:
                        minisig.verify(artifact_keys, local,
                                       fetch(artifact["path"] +
                                             countersign.MINISIG,
                                             artifact["name"] + ".minisig"))
                        signed = keyring().verify_signature(
                            local, fetch(artifact["sig"],
                                         artifact["name"] + ".sig"))
                        recorded = entry["signatures"][artifact["name"]]
                        if signed["primary"] != recorded["signer"] or \
                                signed["created"] != int(parse_iso(
                                    recorded["created"]).timestamp()):
                            raise ReleaseError(
                                f"{artifact['name']}: its signature is not the "
                                "one the index records")
                    local.unlink()
            report["checked"].append({"version": descriptor["version"],
                                      "kind": descriptor["kind"],
                                      "artifacts": len(descriptor["artifacts"])})
        return report
