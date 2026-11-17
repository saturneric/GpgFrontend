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

This is the release tooling's reference client. What it checks of an index
is what the update check (modules/src/m_ver_check) checks, and the two are
held to the same files: scripts/tests/fixtures/contract, read by both
test_contract.py and the update check's ContractTest.
"""

from __future__ import annotations

import contextlib
import http.client
import shutil
import ssl
import tempfile
import urllib.error
import urllib.request
from pathlib import Path

from ..adapters import minisig, openpgp
from ..adapters.clock import utcnow
from ..adapters.files import sha256_file
from ..adapters.store import NotFound
from ..domain import descriptor, index_model, layout, refs
from ..domain import policy as policy_mod
from ..domain.formats import (SCHEMA, check_relpath, load_json, parse_iso,
                              parse_version, sha256_bytes)
from ..domain.layout import INDEX, MINISIG, SIG, SUMS
from ..domain.limits import EXPIRY_SKEW, MAX_INDEX_LIFETIME, SMALL_OBJECT_LIMIT
from ..domain.manifest import parse_manifest
from ..errors import ReleaseError
from . import trust
from .product import Product


class HttpSource:
    """Product-relative paths over HTTPS bases, first that answers wins."""

    def __init__(self, bases: list[str], slug: str, cafile: str | None = None):
        for base in bases:
            if not policy_mod.is_https_base(base):
                raise ReleaseError(f"{base}: a base is https:// and ends in /")
        self.bases = bases
        self.slug = slug
        self.context = ssl.create_default_context(cafile=cafile)

    def _each(self, path: str, consume):
        """@p consume of the first base's answer; NotFound when every base
        answered that there is no such object."""
        errors, missing = [], 0
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
                missing += getattr(e, "code", None) == 404
        if missing == len(self.bases):
            raise NotFound("; ".join(errors))
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
                shutil.copyfileobj(r, f, 1 << 20)
            return Path(local)
        return self._each(path, copy)

    def __str__(self) -> str:
        return ", ".join(f"{b}{self.slug}/" for b in self.bases)


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
    for name in ("nightly", "beta", "prerelease"):
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
        for name in ("beta", "rc"):
            if entry.get(name) is not None and \
                    not descriptor.names_build(entry[name]):
                raise ReleaseError(f"series {key}'s {name} does not name a "
                                   "build")
        if not isinstance(entry.get("prerelease", False), bool):
            raise ReleaseError(f"the index's series {key}'s prerelease has "
                               "the wrong type")
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
    nightly = index.get("nightly") or {}
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

    def candidate(b: dict, key: str) -> tuple[str, dict]:
        """A release candidate is X.Y.0-rc.N of a series users are meant
        to move to and that has no release yet (index_model.choose_rcs): built
        from its own tag."""
        v = parse_version(b["version"], f"series {key}")
        entry = series[key]
        if not v.is_release_candidate or \
                entry.get("status") not in policy_mod.CURRENT_STATUSES or \
                entry.get("latest") is not None or v in withdrawn:
            raise ReleaseError(f"series {key} offers {b['version']}, which "
                               "is not a release candidate it may offer")
        return refs.release_tag_ref(b["version"]), b

    def newest(name: str, found: list[tuple[str, dict]]) -> None:
        """An index that still says the beta or the prerelease for all
        series (before they were dropped): the listed one is the newest of
        the series', and is listed when any is."""
        if name not in channels and index.get(name) is None:
            return
        listed = index.get(name) or {}
        if channels.get(name) != listed.get("version"):
            raise ReleaseError(f"channels.{name} {channels.get(name)} is not "
                               f"the listed {name}")
        best = max((b for _, b in found),
                   key=lambda b: parse_version(b["version"]), default=None)
        if (listed or None) != best:
            raise ReleaseError(f"the listed {name} is not the newest of the "
                               f"series' ({name})")

    betas, rcs = [], []
    for key, entry in series.items():
        for name, out in (("beta", betas), ("rc", rcs)):
            b = entry.get(name)
            if b is None:
                continue
            if parse_version(b["version"], f"series {key}").series != key:
                raise ReleaseError(f"series {key} offers {b['version']} as "
                                   f"its {name}")
            out.append(offered(b, f"series {key}") if name == "beta"
                       else candidate(b, key))
        if bool(entry.get("rc")) != entry.get("prerelease", False):
            raise ReleaseError(f"series {key} is a prerelease exactly when "
                               "it offers a release candidate")
    newest("beta", betas)
    newest("prerelease", rcs)
    return betas + rcs


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


def _descriptor(product: Product, builds, entry: dict, want: str,
                ref: str | None = None) -> dict:
    """The descriptor @p entry of the index names: the bytes it hashes,
    of this product, of version @p want (and built from @p ref)."""
    where = entry["descriptor"]
    raw = builds.read(check_relpath(where))
    if sha256_bytes(raw) != entry["sha256"]:
        raise ReleaseError(f"{where}: sha256 is not the index's")
    d = load_json(raw, where)
    descriptor.check_shape(d, where)
    descriptor.require(d, where, "version", "kind", "artifacts")
    if d.get("product") != product.slug:
        raise ReleaseError(f"{where}: another product's")
    if ref is not None and d.get("ref") != ref:
        raise ReleaseError(f"{where}: built from {d.get('ref')}, "
                           f"listed as a build of {ref}")
    if d["version"] != want:
        raise ReleaseError(f"{where}: is {d['version']}, listed as "
                           f"{want}")
    return d


def _matches(d: dict, sums: dict) -> None:
    for artifact in d["artifacts"]:
        if sums.get(artifact["name"]) != artifact["sha256"]:
            raise ReleaseError(f"{artifact['name']}: the descriptor's sha256 "
                               "is not the manifest's")


def check_release(product: Product, builds, version: str,
                  entry: dict) -> dict:
    """Release @p version as the index lists it (@p entry): its descriptor,
    once it is the one the index names, a stable or imported release
    signed by a maintainer, with a signature time recorded for every file
    and every file's sha256 its manifest's. A stable release's manifest is
    the maintainer's; an imported one has none, and each file rests on its
    own signature (checked_artifact)."""
    d = _descriptor(product, builds, entry, version)
    where, kind = entry["descriptor"], d["kind"]
    if kind == layout.KIND_NIGHTLY:
        raise ReleaseError(f"{where}: a build, listed as a release")
    if kind not in (layout.KIND_STABLE, layout.KIND_IMPORTED):
        raise ReleaseError(f"{where}: unknown kind {kind!r}")
    if d.get("signing", {}).get("scheme") != layout.SCHEME_OPENPGP:
        raise ReleaseError(f"{where}: a {kind} release is signed "
                           "by a maintainer")
    if d.get("sums"):
        sums = parse_manifest(builds.read(check_relpath(d["sums"])),
                              d["sums"])
    else:
        sums = descriptor.hashes(d)
    # When each file's maintainer signature was made, as the server
    # admitted it: one per file, nothing else.
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
    _matches(d, sums)
    return d


def checked_notes(entry: dict, fetch, keyring=None) -> Path | None:
    """A release's notes, fetched (@p fetch, as verify()'s): their hash is
    the one the index pins, and with @p keyring (verify()'s) their
    maintainer signature verifies. None when the release has none."""
    if "notes" not in entry:
        return None
    version = entry.get("version", "a release")
    for field in ("notes", "notes_sha256", "notes_sig"):
        _typed(entry.get(field), str, f"{version}'s {field}")
    notes = fetch(entry["notes"], "notes.md")
    if sha256_file(notes) != entry["notes_sha256"]:
        raise ReleaseError(f"{entry['notes']}: not the index's notes")
    if keyring is not None:
        keyring().verify(notes, fetch(entry["notes_sig"], "notes.md" + SIG))
    return notes


def checked_artifact(d: dict, artifact: dict, entry: dict, fetch,
                     keyring) -> Path:
    """One of @p d's files, fetched: its sha256 is the descriptor's and,
    for a release, its maintainer signature is the one the index records
    (@p entry). The caller removes it."""
    name = artifact["name"]
    local = fetch(artifact["path"], name)
    if sha256_file(local) != artifact["sha256"]:
        raise ReleaseError(f"{name}: the download does not match its sha256")
    if d["kind"] != layout.KIND_NIGHTLY:
        signed = keyring().verify_signature(local,
                                            fetch(artifact["sig"], name + SIG))
        recorded = entry["signatures"][name]
        if signed["primary"] != recorded["signer"] or \
                signed["created"] != int(parse_iso(
                    recorded["created"]).timestamp()):
            raise ReleaseError(f"{name}: its signature is not the one the "
                               "index records")
    return local


def _check_entry(product: Product, builds, entry: dict,
                 listed_as: str | None, ref: str | None, fetch, keyring,
                 deep: bool) -> dict:
    """One entry of the index: a build (@p ref, the branch it must have
    been built from) or a release (@p listed_as, its version). @p fetch
    and @p keyring are verify()'s. Returns what was checked of it."""
    if ref is None:
        d = check_release(product, builds, listed_as, entry)
        # The maintainer's own signature on the manifest: what a person
        # checks by hand. An imported release has none.
        if deep and d.get("sums_sig"):
            local = fetch(d["sums"], SUMS)
            keyring().verify(local, fetch(d["sums_sig"], SUMS + SIG))
    else:
        d = _descriptor(product, builds, entry, entry["version"], ref)
        where, kind = entry["descriptor"], d["kind"]
        if kind in (layout.KIND_STABLE, layout.KIND_IMPORTED):
            raise ReleaseError(f"{where}: a release, listed as a build")
        if kind != layout.KIND_NIGHTLY:
            raise ReleaseError(f"{where}: unknown kind {kind!r}")
        descriptor.require(d, where, "build")
        manifest = d["build"].get("sums", "")
        _matches(d, parse_manifest(builds.read(check_relpath(manifest)),
                                   manifest))
    # A release's notes: the index pins their hash; the maintainer's
    # signature is checked with --deep.
    checked_notes(entry | {"version": d["version"]}, fetch,
                  keyring if deep else None)
    if deep:
        for artifact in d["artifacts"]:
            checked_artifact(d, artifact, entry, fetch, keyring).unlink()
    return {"version": d["version"], "kind": d["kind"],
            "artifacts": len(d["artifacts"])}


def read_index(product: Product, update) -> dict:
    """The published index, once its signature verifies with a key of the
    index-keys list it names (trusted through the root) and it checks out
    as a client checks it (check_index)."""
    # What the index names to be trusted with, before it is: the
    # signature then decides.
    index_keys = _index_keys(product, update,
                             load_json(update.read(INDEX), INDEX))
    data, comment = minisig.read_signed(update, INDEX, index_keys)
    index = load_json(data, INDEX)
    check_index(product, index, comment)
    return index


def verify(product: Product, update, builds, *, deep: bool = False) -> dict:
    """Everything hangs off the signed index: the descriptors by their
    hashes, the files by the descriptors'. A stable or imported release's
    maintainer signatures are checked as well with @p deep, against the
    keys beside release/product.json (maintainers.asc) whose fingerprints
    the trust root pins, retired ones included."""
    with tempfile.TemporaryDirectory(prefix="gfr-verify-") as tmp_name, \
            contextlib.ExitStack() as stack:
        tmp = Path(tmp_name)
        index = read_index(product, update)
        report = {"product": product.slug, "serial": index["serial"],
                  "expires": index["expires"],
                  "channels": index_model.summary(index),
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
