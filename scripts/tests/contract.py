#!/usr/bin/env python3
# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""The contract fixtures: what gfrelease publishes, as the update check
(modules/src/m_ver_check) reads it, written into
scripts/tests/fixtures/contract/ by the code that publishes it.

  <case>/update/v1/index.json(.minisig)   the signed index
  <case>/builds/...                         every descriptor and notes file
                                            the index names, where it names
                                            them
  limits.json                               domain/limits.py, as numbers

Two cases: `nightly`, a product with main's nightly and nothing released
yet, and `full`, every kind of entry an index has: releases (one with
notes, one withdrawn), an eol series with a successor, a stable series'
beta, a release candidate of a stable series not released yet, and the
nightly. The descriptors are domain/descriptor.py's, the entries and the
content domain/index_model.py's, the bytes formats.canonical_json's, and
the signature minisign's with the throwaway index key 2 of
fixtures/trust (listed in its index-keys 2, under root 3). Times are
fixed (NOW): signing is deterministic, so the same code writes the same
bytes.

test_contract.py holds the files to what this writes; the update check's
ContractTest reads them. A change to what is published shows as a change
to these files:

  python3 scripts/tests/contract.py     (writes them again)
"""

import datetime as dt
import sys
import tempfile
from pathlib import Path

import _paths
from gfrelease.adapters import minisig
from gfrelease.app.product import Product
from gfrelease.domain import descriptor, index_model, layout, limits
from gfrelease.domain.formats import canonical_json, iso
from gfrelease.paths import POLICY_FILE, PRODUCT_FILE

FIXTURE = _paths.TESTS / "fixtures" / "contract"
TRUST = _paths.TESTS / "fixtures" / "trust"
INDEX_KEY = TRUST / "keys" / "index-2.key"
ROOT_VERSION, INDEX_KEYS_VERSION = 3, 2
NOW = dt.datetime(2026, 11, 1, tzinfo=dt.timezone.utc)
SERIAL = 42
MAINTAINER = "A" * 40
IDENTITY = ("https://github.com/example/product/.github/workflows/"
            "build.yml@{ref}")
# What every build ships (product.json's artifact rules name each).
LEGS = ("linux-x86_64.AppImage", "linux-x86_64-portable.AppImage",
        "linux-aarch64.AppImage", "windows-x86_64.msi",
        "windows-x86_64-portable.zip", "macos-13-intel.dmg", "macos-14.dmg",
        "macos-15.dmg", "android-arm64-v8a.apk")
PLATFORMS = {"windows": "10.0.17763", "macos": "13.0",
             "linux_glibc": "2.35", "android_api": 29}


def product() -> Product:
    """The product's artifact rules and slug; trust from fixtures/trust."""
    return Product(PRODUCT_FILE, POLICY_FILE, root_dir=TRUST / "root",
                   index_keys_dir=TRUST / "index-keys")


def _hash(text: str) -> str:
    return index_model.sha256_bytes(text.encode())


def _artifacts(p: Product, directory: str, version: str, *,
               signed: bool) -> list[dict]:
    filesafe = version.replace("+", "-")
    out = []
    for leg in LEGS:
        name = f"{p.data['artifact_prefix']}{filesafe}-{leg}"
        digest = _hash(name)
        entry = p.classify(name, filesafe) | {
            "name": name, "path": f"{directory}/{name}",
            "size": len(name) * 1000, "sha256": digest,
            "build_sha256": digest, "transform": "none"}
        if signed:
            entry["sig"] = layout.sig_of(entry["path"])
        out.append(entry)
    return out


def nightly(p: Product, version: str, ref: str, commit: str) -> dict:
    """A promoted build's descriptor (main's, a release branch's, a release
    candidate tag's)."""
    directory = layout.nightly_dir(version)
    return descriptor.make(
        layout.KIND_NIGHTLY, p.slug, version, ref, commit,
        {"scheme": layout.SCHEME_SIGSTORE},
        _artifacts(p, directory, version, signed=False),
        built_at=iso(NOW - dt.timedelta(hours=2)),
        promoted_at=iso(NOW - dt.timedelta(hours=1)),
        build={"sums": layout.build_sums(directory),
               "bundle": layout.build_bundle(directory),
               "identity": IDENTITY.format(ref=ref)})


def stable(p: Product, version: str, commit: str) -> dict:
    """A stable release's descriptor, signed by the maintainer."""
    directory = layout.release_dir(version)
    ref = f"refs/tags/v{version}"
    return descriptor.make(
        layout.KIND_STABLE, p.slug, version, ref, commit,
        {"scheme": layout.SCHEME_OPENPGP, "fingerprint": MAINTAINER},
        _artifacts(p, directory, version, signed=True),
        built_at=iso(NOW - dt.timedelta(days=3)),
        released_at=iso(NOW - dt.timedelta(days=2)),
        build={"sums": layout.build_sums(directory),
               "bundle": layout.build_bundle(directory),
               "identity": IDENTITY.format(ref=ref)},
        sums=f"{directory}/{layout.SUMS}",
        sums_sig=layout.sig_of(f"{directory}/{layout.SUMS}"))


class Case:
    """One published product: the files of its builds store, and what its
    index lists."""

    def __init__(self, p: Product):
        self.p = p
        self.builds: dict[str, bytes] = {}

    def add_build(self, d: dict) -> dict:
        path = layout.descriptor_of(layout.nightly_dir(d["version"]))
        raw = canonical_json(d)
        self.builds[path] = raw
        return index_model.build_entry(d["version"], path, raw)

    def add_release(self, d: dict, notes: bytes | None) -> dict:
        path = layout.descriptor_of(layout.release_dir(d["version"]))
        raw = canonical_json(d)
        self.builds[path] = raw
        fields = None
        if notes is not None:
            fields = index_model.notes_fields(d["version"], notes)
            self.builds[fields["notes"]] = notes
        created = int((NOW - dt.timedelta(days=2)).timestamp())
        signatures = {a["name"]: {"signer": MAINTAINER, "created": created}
                      for a in d["artifacts"]}
        signatures[layout.SUMS] = {"signer": MAINTAINER, "created": created}
        return index_model.release_entry(path, index_model.sha256_bytes(raw),
                                         fields, signatures)

    def write(self, out: Path, content: dict) -> None:
        index = index_model.dated(content, SERIAL, NOW)
        update = out / "update"
        local = update / layout.INDEX
        local.parent.mkdir(parents=True, exist_ok=True)
        local.write_bytes(canonical_json(index))
        minisig.sign(INDEX_KEY, local,
                     layout.index_comment(self.p.slug, SERIAL))
        for path, data in self.builds.items():
            target = out / "builds" / path
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(data)


def nightly_case(p: Product) -> tuple[Case, dict]:
    case = Case(p)
    listed = case.add_build(nightly(p, "2.4.0-alpha.12+g0123456789ab",
                                    "refs/heads/main", "1" * 40))
    policy = {"series": {"2.4": {"status": "mainline",
                                 "platforms": PLATFORMS}}}
    return case, index_model.content(
        p.slug, policy, ROOT_VERSION, INDEX_KEYS_VERSION, releases={},
        nightly=listed, betas=[], rcs=[])


def full_case(p: Product) -> tuple[Case, dict]:
    case = Case(p)
    policy = {
        "series": {
            "2.1": {"status": "eol", "eol": dt.date(2026, 6, 30),
                    "successor": "2.2", "platforms": PLATFORMS},
            "2.2": {"status": "stable", "platforms": PLATFORMS},
            "2.3": {"status": "stable", "platforms": PLATFORMS},
            "2.4": {"status": "mainline", "platforms": PLATFORMS},
        },
        "withdrawn": [{"version": "2.2.4",
                       "reason": "its installer did not start"}],
    }
    releases = {
        "2.1.9": case.add_release(stable(p, "2.1.9", "2" * 40), None),
        "2.2.5": case.add_release(stable(p, "2.2.5", "3" * 40),
                                  b"## 2.2.5\n\n- Everything, fixed.\n"),
    }
    nightly_entry = case.add_build(nightly(
        p, "2.4.0-alpha.12+g0123456789ab", "refs/heads/main", "1" * 40))
    beta = case.add_build(nightly(p, "2.2.6-alpha.3+gabcdef012345",
                                  "refs/heads/release/2.2", "4" * 40))
    rc = case.add_build(nightly(p, "2.3.0-rc.1", "refs/tags/v2.3.0-rc.1",
                                "5" * 40))
    return case, index_model.content(
        p.slug, policy, ROOT_VERSION, INDEX_KEYS_VERSION, releases=releases,
        nightly=nightly_entry, betas=[beta], rcs=[rc])


CASES = {"nightly": nightly_case, "full": full_case}


def write(out: Path) -> None:
    """Every fixture, into @p out."""
    p = product()
    for name, make in CASES.items():
        case, content = make(p)
        case.write(out / name, content)
    (out / "limits.json").write_bytes(canonical_json(limits.as_json()))


def files(directory: Path) -> dict[str, bytes]:
    return {p.relative_to(directory).as_posix(): p.read_bytes()
            for p in sorted(directory.rglob("*")) if p.is_file()}


if __name__ == "__main__":
    with tempfile.TemporaryDirectory(prefix="gfr-contract-") as tmp:
        write(Path(tmp))
        new = files(Path(tmp))
    if FIXTURE.exists():
        for old in FIXTURE.rglob("*"):
            if old.is_file() and old.relative_to(FIXTURE).as_posix() \
                    not in new:
                old.unlink()
    for rel, data in new.items():
        (FIXTURE / rel).parent.mkdir(parents=True, exist_ok=True)
        (FIXTURE / rel).write_bytes(data)
    print(f"{FIXTURE}: {len(new)} files", file=sys.stderr)
