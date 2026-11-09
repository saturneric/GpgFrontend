# Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""Check a published index the way a client does, and further.

  index.json  minisigned by a pinned index key; the trusted comment names
              this product and the serial; schema known; not expired; its
              channels name only what it lists, a beta only a build of its
              own series' release branch. Over HTTPS, every update base
              serves it, not just the first that answers.
  descriptor  its sha256 is the one the index states.
  manifest    SHA256SUMS.txt minisigned by the artifact key (a nightly's or
              a beta's by promotion, a stable or imported release's by the
              server's countersignature), and every artifact's sha256 in
              the descriptor is the one it lists.
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
import descriptor
import layout
import minisig
import openpgp
from common import (INDEX, MINISIG, SCHEMA, SIG, SMALL_OBJECT_LIMIT, SUMS,
                    Product, ReleaseError, check_relpath, load_json,
                    parse_iso, parse_manifest, parse_version, policy_mod,
                    refs, sha256_bytes, sha256_file, utcnow)

# How far behind the first base a mirror may be while the host is writing
# it: the index goes to the update store first, then to each mirror.
MIRROR_LAG = 2


class HttpSource:
    """Product-relative paths over HTTPS bases, first that answers wins."""

    def __init__(self, bases: list[str], slug: str, cafile: str | None = None):
        for base in bases:
            if not base.startswith("https://") or not base.endswith("/"):
                raise ReleaseError(f"{base}: a mirror base is https:// and ends in /")
        self.bases = bases
        self.slug = slug
        self.cafile = cafile
        self.context = ssl.create_default_context(cafile=cafile)

    def each(self) -> list["HttpSource"]:
        """One source per base: what a client that reached only that mirror
        would read."""
        return [HttpSource([base], self.slug, self.cafile)
                for base in self.bases]

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
        """A small object, whole; anything larger is refused (Store.read)."""
        def small(r):
            data = r.read(SMALL_OBJECT_LIMIT + 1)
            if len(data) > SMALL_OBJECT_LIMIT:
                raise ReleaseError(f"{path}: larger than "
                                   f"{SMALL_OBJECT_LIMIT} bytes")
            return data
        return self._each(path, small)

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
    data, sig = source.read(path), source.read(path + MINISIG)
    local = tmp / Path(path).name
    local.write_bytes(data)
    (tmp / (local.name + MINISIG)).write_bytes(sig)
    comment = minisig.verify(keys, local, tmp / (local.name + MINISIG))
    return data, comment


def _typed(value, kind, what: str):
    if not isinstance(value, kind) or isinstance(value, bool):
        raise ReleaseError(f"the index's {what} has the wrong type")
    return value


def check_index(product: Product, index: dict, comment: str) -> None:
    """The index says of itself what its signature says of it, is not
    expired, and every part read below has the type it is read as: whoever
    holds the index key decides what it lists, never how this fails."""
    if index.get("schema") != SCHEMA:
        raise ReleaseError(f"index schema {index.get('schema')} is unknown")
    if index.get("product") != product.slug:
        raise ReleaseError(f"the index is for {index.get('product')!r}, "
                           f"not {product.slug!r}")
    want = layout.index_comment(product.slug,
                                _typed(index.get("serial"), int, "serial"))
    if comment != want:
        raise ReleaseError(f"trusted comment {comment!r} is not {want!r}")
    if parse_iso(index.get("expires")) <= utcnow():
        raise ReleaseError(f"the index expired at {index['expires']}")

    for version in _typed(index.get("channels"), dict, "channels").values():
        if version is not None:
            _typed(version, str, "channels")
    for name in ("nightly", "beta"):
        if index.get(name) is not None and \
                not descriptor.names_build(index[name]):
            raise ReleaseError(f"the index's {name} does not name a build")
    for version, entry in _typed(index.get("releases"), dict,
                                 "releases").items():
        _typed(entry, dict, f"release {version}")
        for field in ("descriptor", "sha256"):
            _typed(entry.get(field), str, f"release {version}'s {field}")
    for key, entry in _typed(index.get("series"), dict, "series").items():
        _typed(entry, dict, f"series {key}")
        if entry.get("beta") is not None and \
                not descriptor.names_build(entry["beta"]):
            raise ReleaseError(f"series {key}'s beta does not name a build")
        if entry.get("latest") is not None:
            _typed(entry["latest"], str, f"series {key}'s latest")
    for w in _typed(index.get("withdrawn", []), list, "withdrawn"):
        _typed(w, dict, "withdrawn entry")


def _check_channels(index: dict) -> list[tuple[str, dict]]:
    """The channels name only what the index lists, nothing withdrawn is
    offered, and every beta is one its series may offer. Returns the betas
    to check, each with the release branch it must have been built from."""
    releases, channels = index["releases"], index["channels"]
    series = index["series"]
    nightly, beta = index.get("nightly") or {}, index.get("beta") or {}
    if channels.get("beta") != beta.get("version"):
        raise ReleaseError(f"channels.beta {channels.get('beta')} is not "
                           "the listed beta")
    if channels.get("stable") is not None and \
            channels["stable"] not in releases:
        raise ReleaseError(f"channels.stable {channels['stable']} is not "
                           "a listed release")
    if channels.get("nightly") != nightly.get("version"):
        raise ReleaseError(f"channels.nightly {channels.get('nightly')} is "
                           "not the listed nightly")
    withdrawn = {parse_version(w.get("version"), "withdrawn")
                 for w in index.get("withdrawn", [])}
    for channel, version in channels.items():
        if version is not None and \
                parse_version(version, f"channels.{channel}") in withdrawn:
            raise ReleaseError(f"channels.{channel} offers {version}, "
                               "which is withdrawn")
    for key, entry in series.items():
        latest = entry.get("latest")
        if latest is not None and (
                latest not in releases or
                parse_version(latest, f"series {key}") in withdrawn or
                parse_version(latest).series != key):
            raise ReleaseError(f"series {key} offers {latest}, which is not "
                               "a listed release of it")

    def offered(b: dict, where: str) -> tuple[str, dict]:
        """A beta is a build of a series users are meant to be on, newer
        than that series' latest release (index.build)."""
        v = parse_version(b["version"], where)
        entry = series.get(v.series)
        if entry is None or \
                entry.get("status") not in policy_mod.CURRENT_STATUSES:
            raise ReleaseError(f"{where} offers {b['version']}, of a series "
                               "that offers no beta")
        if v in withdrawn or (entry.get("latest") is not None and
                              v <= parse_version(entry["latest"])):
            raise ReleaseError(f"{where} offers {b['version']}, which is "
                               "withdrawn or not newer than the series' "
                               "latest release")
        return refs.HEADS + refs.release_branch(v.series), b

    betas = []
    for key, entry in series.items():
        if entry.get("beta") is None:
            continue
        if parse_version(entry["beta"]["version"],
                         f"series {key}").series != key:
            raise ReleaseError(f"series {key} offers "
                               f"{entry['beta']['version']} as its beta")
        betas.append(offered(entry["beta"], f"series {key}"))
    if beta:
        # The newest of the series' betas, for clients that predate them.
        # (An index from before the per-series entry names only this one.)
        if any("beta" in entry for entry in series.values()):
            newest = max((b for _, b in betas),
                         key=lambda b: parse_version(b["version"]),
                         default=None)
            if beta != newest:
                raise ReleaseError(f"the listed beta {beta['version']} is not "
                                   "the newest of the series' betas")
        else:
            betas.append(offered(beta, "the listed beta"))
    elif betas:
        raise ReleaseError("a series offers a beta the index does not list")
    return betas


def check_mirrors(product: Product, update, index_keys: list[str],
                  serial: int) -> list[dict]:
    """Every update base on its own: a client that reaches only the second
    mirror is served a signed, unexpired index of this product too. One a
    serial or two behind is being synced; one further back, or broken, is
    an error. Returns each base's serial."""
    seen, problems = [], []
    for source in update.each():
        try:
            with tempfile.TemporaryDirectory(prefix="gfr-mirror-") as tmp:
                data, comment = _signed(source, INDEX, index_keys, Path(tmp))
            index = load_json(data, INDEX)
            check_index(product, index, comment)
            if index["serial"] < serial - MIRROR_LAG:
                raise ReleaseError(f"serves serial {index['serial']}, the "
                                   f"first base {serial}")
            seen.append({"base": str(source), "serial": index["serial"]})
        except ReleaseError as e:
            problems.append(f"{source}: {e}")
    if problems:
        raise ReleaseError("an update mirror does not serve the index:\n  "
                           + "\n  ".join(problems))
    return seen


def verify(product: Product, update, builds, *, index_keys: list[str],
           artifact_key: str, deep: bool = False) -> dict:
    """A nightly's or beta's files are minisigned with the artifact key by
    promotion; a stable or imported release's by the server's
    countersignature, made once the maintainer's OpenPGP signatures
    verified. Those are checked as well with @p deep, against the keys
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
        if hasattr(update, "each"):
            report["bases"] = check_mirrors(product, update, index_keys,
                                            index["serial"])

        ring = None

        def keyring():
            nonlocal ring
            if ring is None:
                ring = stack.enter_context(openpgp.Keyring.of(product))
            return ring

        def fetch(rel: str, name: str) -> Path:
            return builds.fetch_to(check_relpath(rel), tmp / name)

        # A nightly is main's build, a beta its series' release branch's:
        # the same chain of trust, the descriptor's ref says which it is.
        nightly = index.get("nightly")
        entries = [(None, nightly, refs.MAIN)] if nightly else []
        entries += [(None, b, ref) for ref, b in _check_channels(index)]
        entries += [(v, e, None) for v, e in index["releases"].items()]
        for listed_as, entry, ref in entries:
            where = entry["descriptor"]
            raw = builds.read(check_relpath(where))
            if sha256_bytes(raw) != entry["sha256"]:
                raise ReleaseError(f"{where}: sha256 is not the index's")
            d = load_json(raw, where)
            descriptor.check_shape(d, where)
            descriptor.require(d, where, "version", "kind", "artifacts")
            if d.get("product") != product.slug:
                raise ReleaseError(f"{where}: another product's")
            want = listed_as or entry["version"]
            if ref is not None and d.get("ref") != ref:
                raise ReleaseError(f"{where}: built from {d.get('ref')}, "
                                   f"listed as a build of {ref}")
            if d["version"] != want:
                raise ReleaseError(f"{where}: is {d['version']}, listed as "
                                   f"{want}")
            scheme = d.get("signing", {}).get("scheme")
            kind = d["kind"]
            if kind == layout.KIND_NIGHTLY:
                if ref is None:
                    raise ReleaseError(f"{where}: a build, listed as a "
                                       "release")
                if scheme != layout.SCHEME_MINISIGN:
                    raise ReleaseError(f"{where}: a nightly is minisigned")
                descriptor.require(d, where, "sums")
                sums_data, _ = _signed(builds, check_relpath(d["sums"]),
                                       artifact_keys, tmp)
                sums = parse_manifest(sums_data, d["sums"])
            elif kind in (layout.KIND_STABLE, layout.KIND_IMPORTED):
                if ref is not None:
                    raise ReleaseError(f"{where}: a release, listed as a "
                                       "build")
                if scheme != layout.SCHEME_OPENPGP:
                    raise ReleaseError(f"{where}: a {kind} release is signed "
                                       "by a maintainer")
                # What the application relies on: the release server's
                # countersignature, made after it checked the maintainer's.
                manifest = countersign.manifest_path(d)
                if entry.get("sums_minisig") != manifest + MINISIG:
                    raise ReleaseError(f"{where}: listed without the server's "
                                       "countersignature")
                sums_data, _ = _signed(builds, check_relpath(manifest),
                                       artifact_keys, tmp)
                sums = parse_manifest(sums_data, manifest)
                # When each file's maintainer signature was made, as the
                # server admitted it: one per file, nothing else.
                signatures = entry.get("signatures")
                if not isinstance(signatures, dict) or set(signatures) != \
                        {a["name"] for a in d["artifacts"]}:
                    raise ReleaseError(f"{where}: listed without a signature "
                                       "time for every file")
                for name, sig in signatures.items():
                    if not isinstance(sig, dict) or \
                            not isinstance(sig.get("signer"), str):
                        raise ReleaseError(f"{name}: no signer recorded")
                    parse_iso(sig.get("created"))
                # The maintainer's own signature on the manifest: what a
                # person checks by hand. An imported release has none.
                if deep and d.get("sums_sig"):
                    local = fetch(manifest, SUMS)
                    keyring().verify(local, fetch(d["sums_sig"], SUMS + SIG))
            else:
                raise ReleaseError(f"{where}: unknown kind {kind!r}")
            # A release's notes: the index pins their hash; the server's
            # countersignature is checked always, the maintainer's with --deep.
            if "notes" in entry:
                for field in ("notes", "notes_sha256", "notes_sig"):
                    _typed(entry.get(field), str, f"{want}'s {field}")
                notes = fetch(entry["notes"], "notes.md")
                if sha256_file(notes) != entry["notes_sha256"]:
                    raise ReleaseError(f"{entry['notes']}: not the index's "
                                       "notes")
                if entry.get("notes_minisig") != entry["notes"] + MINISIG:
                    raise ReleaseError(f"{entry['notes']}: listed without "
                                       "the server's countersignature")
                minisig.verify(artifact_keys, notes,
                               fetch(entry["notes_minisig"],
                                     "notes.md" + MINISIG))
                if deep:
                    keyring().verify(notes, fetch(entry["notes_sig"],
                                                  "notes.md" + SIG))
            for artifact in d["artifacts"]:
                name = artifact["name"]
                if sums.get(name) != artifact["sha256"]:
                    raise ReleaseError(f"{name}: the descriptor's sha256 is "
                                       "not the signed manifest's")
                if deep:
                    local = fetch(artifact["path"], name)
                    if sha256_file(local) != artifact["sha256"]:
                        raise ReleaseError(f"{name}: the download does not "
                                           "match its sha256")
                    if kind == layout.KIND_NIGHTLY:
                        minisig.verify(artifact_keys, local,
                                       fetch(artifact["sig"], name + MINISIG))
                    else:
                        minisig.verify(artifact_keys, local,
                                       fetch(artifact["path"] + MINISIG,
                                             name + MINISIG))
                        signed = keyring().verify_signature(
                            local, fetch(artifact["sig"], name + SIG))
                        recorded = entry["signatures"][name]
                        if signed["primary"] != recorded["signer"] or \
                                signed["created"] != int(parse_iso(
                                    recorded["created"]).timestamp()):
                            raise ReleaseError(
                                f"{name}: its signature is not the one the "
                                "index records")
                    local.unlink()
            report["checked"].append({"version": d["version"], "kind": kind,
                                      "artifacts": len(d["artifacts"])})
        return report
