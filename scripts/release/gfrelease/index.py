# Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""The signed update index: v1/index.json, the one thing clients trust.

Built from the release policy and the promoted descriptors, signed with the
product's index key (trusted comment "product=<slug> serial=<n>"), and
published to the update store, then the mirrors. A run whose index would say
nothing new, and is not near expiry, publishes nothing; `resign` publishes
regardless, to push `expires` out.

The serial only grows: it is one more than both the host's own record and the
published index, so a client that has seen serial n never accepts an older
index again (its anti-rollback check), even after the host lost its state.
"""

from __future__ import annotations

import datetime as dt
import json
import tempfile
from pathlib import Path

import minisig
from common import (DESCRIPTOR, INDEX, SCHEMA, Product, ReleaseError,
                    canonical_json, check_relpath, iso, parse_iso, semver,
                    sha256_bytes, utcnow)
from store import Store
from verify import parse_sums

LIFETIME = dt.timedelta(days=14)
RESIGN_BEFORE = dt.timedelta(days=7)
VOLATILE = ("serial", "generated_at", "expires")


def admit_nightly(product: Product, builds: Store, path: str, raw: bytes,
                  artifact_pub: str, tmp: Path) -> dict:
    """A nightly descriptor is listed only if it is this product's, says what
    it is consistently, and its manifest carries the artifact key's signature
    over exactly the hashes it states."""
    d = json.loads(raw)
    prefix = path.rsplit("/", 1)[0]
    if d.get("schema") != SCHEMA or d.get("product") != product.slug:
        raise ReleaseError(f"{path}: not a {product.slug} descriptor")
    if d.get("kind") != "nightly" or not d.get("ref", "").startswith("refs/heads/"):
        raise ReleaseError(f"{path}: not a nightly")
    if d["signing"] != {"scheme": "minisign",
                        "key_id": minisig.key_id(artifact_pub)}:
        raise ReleaseError(f"{path}: not signed with the artifact key")
    if prefix != f"nightly/{str(semver.parse(d['version'])).replace('+', '-')}":
        raise ReleaseError(f"{path}: published under another version's name")
    for p in [d["sums"], d["sums_sig"]] + [a[k] for a in d["artifacts"]
                                           for k in ("path", "sig")]:
        if not check_relpath(p).startswith(prefix + "/"):
            raise ReleaseError(f"{path}: {p} is outside its own release")

    sums, sig = tmp / "SHA256SUMS.txt", tmp / "SHA256SUMS.txt.minisig"
    sums.write_bytes(builds.read(d["sums"]))
    sig.write_bytes(builds.read(d["sums_sig"]))
    minisig.verify([artifact_pub], sums, sig)
    listed = parse_sums(sums.read_bytes())
    if listed != {a["name"]: a["sha256"] for a in d["artifacts"]}:
        raise ReleaseError(f"{path}: artifacts differ from the signed manifest")
    if any(a["sha256"] != a["build_sha256"] or a["transform"] != "none"
           for a in d["artifacts"]):
        raise ReleaseError(f"{path}: a nightly is published as built")
    return d


def build(product: Product, builds: Store, artifact_pub: str) -> dict:
    """The index's content, without serial and times."""
    nightly = None
    with tempfile.TemporaryDirectory(prefix="gfr-index-") as tmp:
        for rel in builds.files("nightly", include=f"*/{DESCRIPTOR}"):
            path = f"nightly/{rel}"
            raw = builds.read(path)
            d = admit_nightly(product, builds, path, raw, artifact_pub, Path(tmp))
            # Only main's nightlies make the channel; release/X.Y builds are
            # published for testers who fetch them by hand.
            if d["ref"] != "refs/heads/main":
                continue
            if nightly is None or semver.parse(d["version"]) > \
                    semver.parse(nightly["version"]):
                nightly = {"version": d["version"], "descriptor": path,
                           "sha256": sha256_bytes(raw)}

    series = {}
    for key, entry in product.policy["series"].items():
        series[key] = {k: entry.get(k) for k in ("status", "eol", "successor")}
        series[key]["platforms"] = entry.get("platforms", {})
        series[key]["latest"] = None
        if series[key]["eol"] is not None:
            series[key]["eol"] = str(series[key]["eol"])

    return {
        "schema": SCHEMA, "product": product.slug,
        "series": series,
        "channels": {"stable": None, "beta": None,
                     "nightly": nightly["version"] if nightly else None},
        "withdrawn": [{"version": w["version"], "reason": w["reason"]}
                      for w in product.policy.get("withdrawn", [])],
        "releases": {},
        "nightly": nightly,
    }


def published(update: Store) -> dict | None:
    if not update.exists(INDEX):
        return None
    return json.loads(update.read(INDEX))


def read_serial(state_dir: Path) -> int:
    path = state_dir / "serial"
    return int(path.read_text()) if path.exists() else 0


def publish(product: Product, builds: Store, update: Store, *,
            index_key: Path, artifact_pub: str, state_dir: Path,
            force: bool = False, mirrors: list[Store] = ()) -> dict | None:
    content = build(product, builds, artifact_pub)
    previous = published(update)
    now = utcnow()
    if previous is not None and not force:
        same = {k: v for k, v in previous.items() if k not in VOLATILE} == content
        if same and parse_iso(previous["expires"]) - now > RESIGN_BEFORE:
            return None

    state_dir.mkdir(parents=True, exist_ok=True)
    serial = max(read_serial(state_dir),
                 previous["serial"] if previous else 0) + 1
    index = dict(content, serial=serial, generated_at=iso(now),
                 expires=iso(now + LIFETIME))

    with tempfile.TemporaryDirectory(prefix="gfr-sign-") as tmp:
        local = Path(tmp) / "index.json"
        local.write_bytes(canonical_json(index))
        sig = minisig.sign(index_key, local,
                           f"product={product.slug} serial={serial}")
        # Recorded before the upload: a crash after it can only skip a
        # serial, never reuse one.
        (state_dir / "serial").write_text(f"{serial}\n")
        for store in [update, *mirrors]:
            store.put_file(local, INDEX, immutable=False)
            store.put_file(sig, INDEX + ".minisig", immutable=False)
    return index
