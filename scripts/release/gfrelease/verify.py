# Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""Check a published index the way a client does, and further.

  index.json  minisigned by a key of the index-keys list (trust.py); the
              trusted comment names this product and the serial; schema
              known; generated, and not expired, as a client checks it; its
              channels name only what it lists, a beta only a build of its
              own series' release branch.
  descriptor  its sha256 is the one the index states.
  manifest    every artifact's sha256 in the descriptor is the one its
              manifest lists (a nightly's or a beta's is CI's, under build/;
              a stable release's the maintainer's).
  notes       their sha256 is the index's.
  --deep      every artifact downloaded and hashed; for a stable or imported
              release also the maintainer's OpenPGP .sig on it, on the
              manifest and on the notes.

Reads either a Store (the signing host's read-back) or the public HTTPS bases
from release/product.json, as a client would.
"""

from __future__ import annotations

import contextlib
import http.client
import ssl
import datetime as dt
import tempfile
import urllib.error
import urllib.request
from pathlib import Path

import descriptor
import layout
import minisig
import openpgp
import trust
from common import (EXPIRY_SKEW, INDEX, MINISIG, SCHEMA, SIG,
                    SMALL_OBJECT_LIMIT, SUMS, Product, ReleaseError,
                    check_relpath, load_json, parse_iso, parse_manifest,
                    parse_version, policy_mod, refs, sha256_bytes,
                    sha256_file, utcnow)

# The longest an index may be valid for, from when it was made: what the
# update check allows (kMaxIndexLifetimeSeconds).
MAX_INDEX_LIFETIME = dt.timedelta(days=60)


class HttpSource:
    """Product-relative paths over HTTPS bases, first that answers wins."""

    def __init__(self, bases: list[str], slug: str, cafile: str | None = None):
        for base in bases:
            if not policy_mod.is_https_base(base):
                raise ReleaseError(f"{base}: a base is https:// and ends in /")
        self.bases = bases
        self.slug = slug
        self.cafile = cafile
        self.context = ssl.create_default_context(cafile=cafile)

    def _each(self, path: str, consume):
        errors = []
        for base in self.bases:
            url = f"{base}{self.slug}/{check_relpath(path)}"
            # Its own name: Cloudflare's bot filter answers urllib's default
            # User-Agent with 403, which would read as a broken store.
            request = urllib.request.Request(
                url, headers={"User-Agent": "gfrelease-verify/1"})
            try:
                with urllib.request.urlopen(request, timeout=60,
                                            context=self.context) as r:
                    return consume(r)
            except (urllib.error.URLError, http.client.HTTPException,
                    OSError) as e:
                # HTTPException: an answer cut off (IncompleteRead) or not
                # HTTP at all, which is neither of the others.
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


def _signed(source, path: str, keys: list[str]) -> tuple[bytes, str]:
    data, sig = source.read(path), source.read(path + MINISIG)
    return data, minisig.verify_bytes(keys, data, sig, path)


def _file_name(name) -> str:
    """An artifact's name, used as a local file's: one plain segment."""
    if not isinstance(name, str) or "/" in name:
        raise ReleaseError(f"{name!r}: not a file name")
    return check_relpath(name)


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
    generated = parse_iso(index.get("generated_at"))
    expires = parse_iso(index.get("expires"))
    if expires - generated > MAX_INDEX_LIFETIME:
        raise ReleaseError(f"the index is valid from {index['generated_at']} "
                           f"to {index['expires']}, longer than any client "
                           "takes")
    # As a client sees it: a clock a little off is not an expiry.
    if expires + EXPIRY_SKEW < utcnow():
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
        newest = max((b for _, b in betas),
                     key=lambda b: parse_version(b["version"]), default=None)
        if beta != newest:
            raise ReleaseError(f"the listed beta {beta['version']} is not "
                               "the newest of the series' betas")
    elif betas:
        raise ReleaseError("a series offers a beta the index does not list")
    return betas


def _index_keys(product: Product, source, index: dict) -> list[str]:
    """The keys a client would take @p index on: this tree's trust,
    walked forward to the root and index-keys list the index names, read
    from @p source as a client reads them. Both must be published there.
    An index naming an older root or list than this tree's is refused,
    though a client that has not seen the newer ones takes it: the host
    has not taken main's trust yet (gfrelease trust update), which this
    check is there to say."""
    root, keys = product.trust

    def read(path: str) -> bytes | None:
        try:
            return source.read(path)
        except ReleaseError:
            return None  # not there, as far as a client can tell
    want = _typed(index.get("root_version"), int, "root_version")
    if want > root["version"]:
        root = trust.walk(lambda name: read(f"{layout.ROOT_DIR}/{name}"),
                          product.slug, root, upto=want, steps=None)
    published = read(f"{layout.ROOT_DIR}/{layout.root_name(want)}")
    if root["version"] != want or published is None or \
            trust.load(published, "root", trust.ROOT_KIND, product.slug) != root:
        raise ReleaseError(f"the index names root {want}, which {source} "
                           "does not lead to")
    want = _typed(index.get("index_keys_version"), int, "index_keys_version")
    path = f"{layout.INDEX_KEYS_DIR}/{layout.index_keys_name(want)}"
    same_g = root["roles"]["index_list"] == \
        product.trust[0]["roles"]["index_list"]
    keys = trust.accept_index_keys(root, read(path) or b"",
                                   read(path + MINISIG), product.slug, path,
                                   keys["version"] if same_g else 0)
    trust.check_unexpired(root, f"root {root['version']}")
    trust.check_unexpired(keys, f"index-keys {keys['version']}")
    return list(keys["keys"].values())


def _check_entry(product: Product, builds, entry: dict,
                 listed_as: str | None, ref: str | None, fetch, keyring,
                 deep: bool) -> dict:
    """One entry of the index: a build (@p ref, the branch it must have
    been built from) or a release (@p listed_as, its version). @p fetch
    and @p keyring are verify()'s. Returns what was checked of it."""
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
        descriptor.require(d, where, "build")
        manifest = d["build"].get("sums", "")
        sums = parse_manifest(builds.read(check_relpath(manifest)),
                              manifest)
    elif kind in (layout.KIND_STABLE, layout.KIND_IMPORTED):
        if ref is not None:
            raise ReleaseError(f"{where}: a release, listed as a "
                               "build")
        if scheme != layout.SCHEME_OPENPGP:
            raise ReleaseError(f"{where}: a {kind} release is signed "
                               "by a maintainer")
        # A stable release's manifest is the maintainer's; an
        # imported one has none, and each file is checked on its own
        # signature (with --deep).
        if d.get("sums"):
            sums = parse_manifest(
                builds.read(check_relpath(d["sums"])), d["sums"])
        else:
            sums = descriptor.hashes(d)
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
            local = fetch(d["sums"], SUMS)
            keyring().verify(local, fetch(d["sums_sig"], SUMS + SIG))
    else:
        raise ReleaseError(f"{where}: unknown kind {kind!r}")
    # A release's notes: the index pins their hash; the maintainer's
    # signature is checked with --deep.
    if "notes" in entry:
        for field in ("notes", "notes_sha256", "notes_sig"):
            _typed(entry.get(field), str, f"{want}'s {field}")
        notes = fetch(entry["notes"], "notes.md")
        if sha256_file(notes) != entry["notes_sha256"]:
            raise ReleaseError(f"{entry['notes']}: not the index's "
                               "notes")
        if deep:
            keyring().verify(notes, fetch(entry["notes_sig"],
                                          "notes.md" + SIG))
    for artifact in d["artifacts"]:
        name = artifact["name"]
        if sums.get(name) != artifact["sha256"]:
            raise ReleaseError(f"{name}: the descriptor's sha256 is "
                               "not the manifest's")
        if deep:
            local = fetch(artifact["path"], name)
            if sha256_file(local) != artifact["sha256"]:
                raise ReleaseError(f"{name}: the download does not "
                                   "match its sha256")
            if kind != layout.KIND_NIGHTLY:
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
    return {"version": d["version"], "kind": kind,
            "artifacts": len(d["artifacts"])}


def verify(product: Product, update, builds, *, deep: bool = False) -> dict:
    """Everything hangs off the signed index: the descriptors by their
    hashes, the files by the descriptors'. A stable or imported release's
    maintainer signatures are checked as well with @p deep, against the
    keys beside release/product.json (maintainers.asc) whose fingerprints
    the trust root pins, retired ones included."""
    with tempfile.TemporaryDirectory(prefix="gfr-verify-") as tmp_name, \
            contextlib.ExitStack() as stack:
        tmp = Path(tmp_name)
        # What the index names to be trusted with, before it is: the
        # signature then decides.
        index_keys = _index_keys(product, update,
                                 load_json(update.read(INDEX), INDEX))
        data, comment = _signed(update, INDEX, index_keys)
        index = load_json(data, INDEX)
        check_index(product, index, comment)
        report = {"product": product.slug, "serial": index["serial"],
                  "expires": index["expires"], "channels": index["channels"],
                  "checked": []}

        ring = None

        def keyring():
            nonlocal ring
            if ring is None:
                ring = stack.enter_context(openpgp.Keyring.of(product,
                                                              retired=True))
            return ring

        def fetch(rel: str, name: str) -> Path:
            return builds.fetch_to(check_relpath(rel), tmp / _file_name(name))

        # A nightly is main's build, a beta its series' release branch's:
        # the same chain of trust, the descriptor's ref says which it is.
        nightly = index.get("nightly")
        entries = [(None, nightly, refs.MAIN)] if nightly else []
        entries += [(None, b, ref) for ref, b in _check_channels(index)]
        entries += [(v, e, None) for v, e in index["releases"].items()]
        for listed_as, entry, ref in entries:
            report["checked"].append(_check_entry(
                product, builds, entry, listed_as, ref, fetch, keyring,
                deep))
        return report
