# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""The trust ceremonies: the next root written and signed by the root
keys, the index-keys list renewed or a new index key rotated in, both by
G; and the signing host taking main's next roots and lists (trust update).
What each document says and when one is trusted is trust.py's; this writes
them.

A new root (renewing one, or changing the root keys, G or the host policy),
offline:

  1. gfrelease trust root new: release/root/<N+1>.root.json, the newest
     root one version up, expiring in two years; edit it by hand for
     anything else that changes.
  2. gfrelease trust root sign <file> -s <root key>: once per root key
     holder, as many of root N's keys as its threshold and as many of the
     new root's own; each run says who is still missing.
  3. gfrelease trust root verify, then commit release/root/: the signing
     host takes it on its next round.

The index-keys list (release/index-keys/<V>.json, signed by G): `trust keys
renew -s <G>` before it expires; `--rotate --secret-out <path>` lists a new
index key instead (with --keep-old beside the current ones). G's secret is
read once from the path given, so -s <(bw get notes ...) works."""

from __future__ import annotations

from pathlib import Path

from ..adapters import minisig
from ..adapters.files import atomic_write, locked
from ..adapters.git import tree_reader
from ..domain import layout, refs
from ..domain import policy as policy_mod
from ..domain.formats import load_json
from ..domain.layout import MINISIG
from ..errors import ReleaseError
from ..paths import ROOT
from . import trust
from .product import Product


def write_new(path: Path, data: bytes) -> None:
    """A trust file is written once: one there already is never replaced."""
    if path.exists():
        raise ReleaseError(f"{path} exists")
    atomic_write(path, data)


def new_root(product: Product) -> tuple[Path, dict, dict]:
    """The next root written beside the newest: that one, one version up,
    expiring in two years. Returns where, the new root and the one it
    follows."""
    root = trust.newest_root(product.root_dir, product.slug)
    nxt = trust.renewed(root, trust.ROOT_LIFETIME)
    path = product.root_dir / layout.root_name(nxt["version"])
    write_new(path, trust.dump(nxt))
    return path, nxt, root


def sign_root(file: Path, secret: str) -> str:
    """One root key's signature on @p file, beside it as
    <N>.root.json.<keyid>.minisig; returns the key id."""
    doc = load_json(file.read_bytes(), str(file))
    n, slug = doc.get("version"), doc.get("product")
    if not isinstance(n, int) or not isinstance(slug, str):
        raise ReleaseError(f"{file}: not a root")
    tmp = file.with_name(file.name + ".new" + MINISIG)
    try:
        minisig.sign(Path(secret), file, layout.root_comment(slug, n), tmp)
        key_id = minisig.prehashed_key_id(tmp.read_bytes())
        if key_id is None:
            raise ReleaseError("minisign made a legacy signature")
        write_new(file.with_name(layout.root_sig_name(n, key_id)),
                  tmp.read_bytes())
    finally:
        tmp.unlink(missing_ok=True)
    return key_id


def signatures(file: Path, slug: str) -> list[dict]:
    """Who signed the root @p file, against the thresholds of the root
    before it and its own: [{whose, threshold, signed, missing}]."""
    read = trust.read_dir(file.parent)
    data = file.read_bytes()
    doc = trust.load(data, file.name, trust.ROOT_KIND, slug)
    n = doc["version"]
    previous = trust.walk(read, slug, upto=n - 1, steps=None) if n > 1 \
        else None
    groups = []
    for whose, signer in ((f"root {n - 1}", previous), ("its own", doc)):
        if signer is None:
            continue
        role = signer["roles"]["root"]
        keys = {i: signer["keys"][i] for i in role["keyids"]}
        found = trust.signers(
            data, {i: read(layout.root_sig_name(n, i)) for i in keys}, keys,
            layout.root_comment(slug, n))
        groups.append({"whose": whose, "threshold": role["threshold"],
                       "signed": sorted(found),
                       "missing": sorted(set(keys) - found)})
    return groups


def outside_repository(path: str) -> Path:
    p = Path(path).resolve()
    if p == ROOT or ROOT in p.parents:
        raise ReleaseError(f"{p} is inside the repository")
    return p


def renew_index_keys(product: Product, secret: str, *,
                     secret_out: str | None = None,
                     keep_old: bool = False) -> dict:
    """The next index-keys list, written and signed with G (@p secret):
    the same keys, or with @p secret_out a new index key generated there
    (outside the repository) and listed, beside the current ones with
    @p keep_old. Returns where it went ("path"), when it expires, and the
    new key and its secret's file, if any."""
    slug, root_dir, keys_dir = product.slug, product.root_dir, \
        product.index_keys_dir
    root = trust.newest_root(root_dir, slug)
    trust.check_unexpired(root, f"root {root['version']}")
    found = trust.versions(keys_dir)
    if not found:
        raise ReleaseError(f"{keys_dir}: no index-keys list to follow")
    name = layout.index_keys_name(found[-1])
    read = trust.read_dir(keys_dir)
    # Expired or not: renewing one that lapsed is what this is for. Signed
    # by the trusted root's G, or after G was replaced by an earlier root's:
    # the new G takes over by signing the next one.
    for n in range(root["version"], 0, -1):
        signer = trust.walk(trust.read_dir(root_dir), slug, upto=n,
                            steps=None)
        try:
            current = trust.accept_index_keys(signer, read(name),
                                              read(name + MINISIG), slug,
                                              name)
            break
        except ReleaseError:
            if n == 1:
                raise
    nxt = trust.renewed(current, trust.INDEX_KEYS_LIFETIME)
    new_key, secret_file = None, None
    if secret_out is not None:
        secret_file = outside_repository(secret_out)
        new_key = minisig.generate(secret_file)
        nxt["keys"] = (dict(current["keys"]) if keep_old else {}) | \
            {minisig.key_id(new_key): new_key}
    path = keys_dir / layout.index_keys_name(nxt["version"])
    write_new(path, trust.dump(nxt))
    try:
        minisig.sign(Path(secret), path,
                     layout.index_keys_comment(slug, nxt["version"]))
        trust.accept_index_keys(root, path.read_bytes(),
                                path.with_name(path.name + MINISIG)
                                .read_bytes(), slug, path.name)
    except ReleaseError:
        path.unlink(missing_ok=True)
        path.with_name(path.name + MINISIG).unlink(missing_ok=True)
        raise
    return {"path": path, "expires": nxt["expires"], "new_key": new_key,
            "secret": secret_file}


def update_from_git(product: Product, git_dir: Path,
                    state_dir: Path) -> tuple[dict, dict, list[str]]:
    """The signing host's trust brought forward from main's release/root/
    and release/index-keys/ in its mirror @p git_dir, under its lock
    (trust.update)."""
    with locked(state_dir):
        read_root, _ = tree_reader(git_dir, refs.MAIN, "release/root")
        read_keys, names = tree_reader(git_dir, refs.MAIN,
                                       "release/index-keys")
        return trust.update(
            read_root, read_keys,
            policy_mod.numbered(names, layout.INDEX_KEYS_FILE),
            product.root_dir, product.index_keys_dir, product.slug,
            product.root_sha256)


def summary(root: dict, keys: dict) -> dict:
    """The root and the index-keys list: version, expiry, days left."""
    return {name: {"version": doc["version"], "expires": doc["expires"],
                   "days_left": round(trust.days_left(doc), 1)}
            for name, doc in (("root", root), ("index_keys", keys))}


def renewal_due(root: dict, keys: dict) -> str | None:
    """"root" or "index_keys" when that one should be renewed soon (the
    root first), else None."""
    if trust.days_left(root) < trust.ROOT_WARN_DAYS:
        return "root"
    if trust.days_left(keys) < trust.INDEX_KEYS_WARN_DAYS:
        return "index_keys"
    return None
