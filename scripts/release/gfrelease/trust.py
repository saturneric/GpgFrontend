# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""Where the update's trust comes from: a root, and the index-keys list.

  release/root/<N>.root.json       the root: offline root keys and how many
                                   must sign the next root (roles.root),
                                   the key that signs the index-keys list
                                   (roles.index_list, "G"), and what the
                                   signing host admits (host_policy)
  release/root/<N>.root.json.<keyid>.minisig
                                   one root key's signature on it, trusted
                                   comment "product=<slug> root=<N>"
  release/index-keys/<V>.json      the keys that may sign v1/index.json
  release/index-keys/<V>.json.minisig
                                   G's signature, "product=<slug>
                                   index-keys=<V>"

Every signature is minisign, prehashed ("ED") only, and covers the file's
bytes as they are. A root N+1 is taken only when the root keys of N and of
N+1 itself each signed it, at least as often as each's threshold, and it is
exactly version N+1 (TUF's rule for root rotation). Roots are walked one
version at a time; only the newest one must be unexpired. An index-keys
list counts when G of the trusted root signed it; it is never older than
one taken before (the floor), and is unexpired.

Signature files that do not verify (another key, another comment, a legacy
signature, garbage) are not errors: they are not counted. Only the number of
distinct root keys that did sign matters.
"""

from __future__ import annotations

import datetime as dt
import itertools
import json
import tempfile
from pathlib import Path

import layout
import minisig
from common import (MINISIG, ReleaseError, atomic_write, iso, load_json,
                    parse_iso, policy_mod, sha256_bytes, utcnow)

SCHEMA = 1
ROOT, INDEX_KEYS = "root", "index-keys"
MAX_STEPS = 32  # roots taken from one source in one go
SKEW = dt.timedelta(days=1)  # a clock a little off is not an expiry
ROOT_LIFETIME = dt.timedelta(days=2 * 365)
INDEX_KEYS_LIFETIME = dt.timedelta(days=365)


def load(data: bytes, what: str, kind: str, slug: str) -> dict:
    """A root or an index-keys list, its shape checked; not its
    signatures."""
    doc = load_json(data, what)

    def bad(why: str):
        raise ReleaseError(f"{what}: {why}")
    if doc.get("schema") != SCHEMA or doc.get("type") != kind:
        bad(f"not a schema {SCHEMA} {kind}")
    if doc.get("product") != slug:
        bad(f"for {doc.get('product')!r}, not {slug!r}")
    version = doc.get("version")
    if not isinstance(version, int) or isinstance(version, bool) or \
            version < 1:
        bad("no version")
    parse_iso(doc.get("expires"))
    keys = doc.get("keys")
    if not isinstance(keys, dict) or not keys:
        bad("no keys")
    for key_id, key in keys.items():
        if not isinstance(key, str) or minisig.key_id(key) != key_id:
            bad(f"key {key_id} is not the key listed under it")
    if kind == ROOT:
        roles = doc.get("roles")
        if not isinstance(roles, dict):
            bad("no roles")
        for name in ("root", "index_list"):
            role = roles.get(name)
            ids = role.get("keyids") if isinstance(role, dict) else None
            if not isinstance(ids, list) or not ids or \
                    len(set(ids)) != len(ids) or \
                    any(not isinstance(i, str) or i not in keys for i in ids):
                bad(f"roles.{name} names keys it does not list")
            threshold = role.get("threshold")
            if not isinstance(threshold, int) or isinstance(threshold, bool) \
                    or not 1 <= threshold <= len(ids) or \
                    (name == "index_list" and threshold != 1):
                bad(f"roles.{name} has no usable threshold")
        if set(roles["root"]["keyids"]) & set(roles["index_list"]["keyids"]):
            bad("the index-keys list's key is also a root key")
        _check_host_policy(doc.get("host_policy"), what)
    return doc


def _check_host_policy(policy, what: str) -> None:
    def bad(why: str):
        raise ReleaseError(f"{what}: host_policy: {why}")
    if not isinstance(policy, dict):
        bad("missing")
    for field, pattern in (("repository", policy_mod.REPOSITORY),
                           ("workflow", policy_mod.WORKFLOW)):
        if not isinstance(policy.get(field), str) or \
                not pattern.fullmatch(policy[field]):
            bad(f"{field} is not a {field}")
    pgp = policy.get("stable_pgp")
    if not isinstance(pgp, dict):
        bad("no stable_pgp")
    for field in ("fingerprints", "fingerprints_retired"):
        fprs = pgp.get(field, [])
        if not isinstance(fprs, list) or any(
                not isinstance(f, str) or not policy_mod.FINGERPRINT
                .fullmatch(f) for f in fprs):
            bad(f"stable_pgp.{field} are not upper-case fingerprints")
    if not pgp.get("fingerprints"):
        bad("stable_pgp.fingerprints is empty")
    if not isinstance(policy.get("authenticode_subject", ""), str):
        bad("authenticode_subject is not a string")
    retired = policy.get("authenticode_subjects_retired", [])
    if not isinstance(retired, list) or \
            any(not isinstance(s, str) or not s for s in retired):
        bad("authenticode_subjects_retired is not a list of subjects")
    if policy.get("authenticode_timestamp", "required") not in \
            ("required", "optional"):
        bad("authenticode_timestamp is required or optional")


def signers(data: bytes, sigs: dict, keys: dict, comment: str) -> set:
    """The ids of @p keys (id -> public key) that signed @p data with
    exactly @p comment, by @p sigs (id -> signature bytes or None)."""
    found = set()
    with tempfile.TemporaryDirectory(prefix="gfr-trust-") as tmp:
        file = Path(tmp) / "data"
        file.write_bytes(data)
        for key_id, sig in sigs.items():
            if key_id not in keys or sig is None or \
                    minisig.prehashed_key_id(sig) != key_id:
                continue
            (Path(tmp) / "sig").write_bytes(sig)
            try:
                if minisig.verify([keys[key_id]], file,
                                  Path(tmp) / "sig") == comment:
                    found.add(key_id)
            except ReleaseError:
                continue
    return found


def accept_root(trusted: dict | None, data: bytes, read_sig, slug: str,
                what: str, first: int = 1) -> dict:
    """The root in @p data, when it may follow @p trusted (None: it is root
    @p first, trusted on its own signatures). @p read_sig(key_id) is that
    key's signature on it, or None."""
    root = load(data, what, ROOT, slug)
    want = trusted["version"] + 1 if trusted else first
    if root["version"] != want:
        raise ReleaseError(f"{what}: version {root['version']}, not {want}")
    comment = layout.root_comment(slug, want)
    for signer in [r for r in (trusted, root) if r is not None]:
        role = signer["roles"]["root"]
        keys = {i: signer["keys"][i] for i in role["keyids"]}
        found = signers(data, {i: read_sig(i) for i in keys}, keys, comment)
        if len(found) < role["threshold"]:
            whose = "its own" if signer is root else \
                f"root {signer['version']}'s"
            raise ReleaseError(f"{what}: signed by {len(found)} of "
                               f"{whose} root keys, {role['threshold']} "
                               "needed")
    return root


def walk(read, slug: str, trusted: dict | None = None, *,
         anchor: str | None = None, upto: int | None = None,
         steps: int | None = MAX_STEPS, keep=None,
         first: int = 1) -> dict | None:
    """The roots after @p trusted (from @p first when None), each read with
    @p read(name) -> bytes or None, one version at a time until one is
    missing, @p upto is reached, or @p steps were taken. Root 1's sha256
    must be @p anchor when given. Each accepted root is handed to
    @p keep(n, data, sigs) before the next is read. Returns the newest
    trusted root (None when there is none at all); one that does not
    follow raises."""
    for _ in range(steps) if steps else itertools.count():
        n = trusted["version"] + 1 if trusted else first
        if upto is not None and n > upto:
            break
        name = layout.root_name(n)
        data = read(name)
        if data is None:
            break
        if n == 1 and anchor and sha256_bytes(data) != anchor:
            raise ReleaseError(f"{name}: its sha256 is not {anchor}, the "
                               "root this host was set up to trust")
        sigs = {}

        def read_sig(key_id):
            if key_id not in sigs:
                sigs[key_id] = read(layout.root_sig_name(n, key_id))
            return sigs[key_id]
        trusted = accept_root(trusted, data, read_sig, slug, name, first)
        if keep:
            keep(n, data, {k: v for k, v in sigs.items() if v is not None})
    return trusted


def accept_index_keys(root: dict, data: bytes, sig: bytes | None,
                      slug: str, what: str, floor: int = 0) -> dict:
    """The index-keys list in @p data, when the trusted @p root's G signed
    it and it is not older than @p floor."""
    keys = load(data, what, INDEX_KEYS, slug)
    if keys["version"] < floor:
        raise ReleaseError(f"{what}: version {keys['version']} is older "
                           f"than {floor}, taken before")
    g = {i: root["keys"][i] for i in root["roles"]["index_list"]["keyids"]}
    comment = layout.index_keys_comment(slug, keys["version"])
    if not signers(data, {i: sig for i in g}, g, comment):
        raise ReleaseError(f"{what}: not signed by root "
                           f"{root['version']}'s index_list key")
    return keys


def days_left(doc: dict, now: dt.datetime | None = None) -> float:
    return (parse_iso(doc["expires"]) - (now or utcnow())) / \
        dt.timedelta(days=1)


def check_unexpired(doc: dict, what: str,
                    now: dt.datetime | None = None) -> None:
    if parse_iso(doc["expires"]) + SKEW < (now or utcnow()):
        raise ReleaseError(f"{what} expired at {doc['expires']}")


def read_dir(directory: Path):
    """read() for walk(): the files of @p directory."""
    def read(name: str) -> bytes | None:
        path = Path(directory) / name
        return path.read_bytes() if path.is_file() else None
    return read


def versions(directory: Path) -> list[int]:
    """The index-keys versions in @p directory, oldest first."""
    return sorted(int(p.name[:-len(".json")])
                  for p in Path(directory).glob("*.json")
                  if p.name[:-len(".json")].isdigit())


def load_trust(root_dir: Path, index_keys_dir: Path, slug: str,
               anchor: str | None = None,
               first: int = 1) -> tuple[dict, dict]:
    """The newest root of @p root_dir's chain, walked from @p first, and
    the newest index-keys list of @p index_keys_dir; both must hold. Their
    expiry is the caller's to check (check_unexpired)."""
    root = walk(read_dir(root_dir), slug, anchor=anchor, steps=None,
                first=first)
    if root is None:
        raise ReleaseError(f"{root_dir}: no {layout.root_name(first)}")
    found = versions(index_keys_dir)
    if not found:
        raise ReleaseError(f"{index_keys_dir}: no index-keys list")
    name = layout.index_keys_name(found[-1])
    read = read_dir(index_keys_dir)
    keys = accept_index_keys(root, read(name), read(name + MINISIG), slug,
                             name)
    return root, keys


def renewed(doc: dict, lifetime: dt.timedelta,
            now: dt.datetime | None = None) -> dict:
    """@p doc as its next version: the same, one higher, expiring
    @p lifetime from now."""
    nxt = json.loads(json.dumps(doc))
    nxt["version"] = doc["version"] + 1
    nxt["expires"] = iso((now or utcnow()) + lifetime)
    return nxt


def dump(doc: dict) -> bytes:
    """A trust file as it is committed, and edited by hand."""
    return (json.dumps(doc, indent=2, ensure_ascii=False) + "\n").encode()


ROOT_WARN_DAYS = 60
INDEX_KEYS_WARN_DAYS = 30


def update(read_root, read_keys, keys_versions: list[int], root_dir: Path,
           keys_dir: Path, slug: str, anchor: str | None
           ) -> tuple[dict, dict, list[str]]:
    """The signing host's trust, brought forward from main and kept in
    @p root_dir and @p keys_dir: roots read with @p read_root(name), the
    index-keys lists @p keys_versions with @p read_keys(name). Root 1 is
    taken only as @p anchor. Returns the trusted root and list, and what
    should be looked at (a next root or list that does not hold, an
    expiry near): none of it stops signing. An expired root or list
    does (ReleaseError)."""
    problems = []
    root_dir.mkdir(parents=True, exist_ok=True)
    keys_dir.mkdir(parents=True, exist_ok=True)

    def keep(n: int, data: bytes, sigs: dict) -> None:
        for key_id, sig in sigs.items():
            atomic_write(root_dir / layout.root_sig_name(n, key_id), sig)
        atomic_write(root_dir / layout.root_name(n), data)

    if not (root_dir / layout.root_name(1)).is_file():
        if not anchor:
            raise ReleaseError("no root trusted yet, and no --root-sha256 "
                               "to take the first one as")
        if walk(read_root, slug, anchor=anchor, steps=1, keep=keep) is None:
            raise ReleaseError(f"main has no {layout.root_name(1)}")
    trusted = walk(read_dir(root_dir), slug, anchor=anchor, steps=None)
    try:
        root = walk(read_root, slug, trusted, keep=keep)
    except ReleaseError as e:
        # Never a reason to stop: the roots taken so far are kept.
        root = walk(read_dir(root_dir), slug, anchor=anchor, steps=None)
        problems.append(f"{e}; root {root['version']} stays trusted")

    floor_path = keys_dir / "floor.json"
    g = root["roles"]["index_list"]["keyids"]
    floor = load_json(floor_path.read_bytes(), str(floor_path)) \
        if floor_path.is_file() else {"index_list": g, "version": 0}
    if floor.get("index_list") != g:
        # Another G: what the old one signed counts no more, and its
        # versions say nothing about the new one's.
        for old in versions(keys_dir):
            for name in (layout.index_keys_name(old),
                         layout.index_keys_name(old) + MINISIG):
                (keys_dir / name).unlink(missing_ok=True)
        floor = {"index_list": g, "version": 0}
    keys = None
    for v in sorted(set(keys_versions) | set(versions(keys_dir)),
                    reverse=True):
        name = layout.index_keys_name(v)
        here = (keys_dir / name).is_file()
        read = read_dir(keys_dir) if here else read_keys
        try:
            keys = accept_index_keys(root, read(name), read(name + MINISIG),
                                     slug, name, floor["version"])
        except ReleaseError as e:
            problems.append(str(e))
            continue
        if not here:
            atomic_write(keys_dir / (name + MINISIG), read(name + MINISIG))
            atomic_write(keys_dir / name, read(name))
        break
    if keys is None:
        raise ReleaseError(f"no index-keys list that root {root['version']}'s"
                           " index_list key signed")
    atomic_write(floor_path, json.dumps(
        {"index_list": g, "version": keys["version"]}) + "\n")

    for doc, what, warn in ((root, f"root {root['version']}",
                             ROOT_WARN_DAYS),
                            (keys, f"index-keys {keys['version']}",
                             INDEX_KEYS_WARN_DAYS)):
        check_unexpired(doc, what)
        if days_left(doc) < warn:
            problems.append(f"{what} expires at {doc['expires']}, in "
                            f"{days_left(doc):.0f} days: sign the next one")
    return root, keys, problems
