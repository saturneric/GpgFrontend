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
import sys
import tempfile
from pathlib import Path

import authenticode
import imported
import minisig
import openpgp
from common import (BUNDLE, DESCRIPTOR, INDEX, SCHEMA, SIG, SUMS, Product,
                    ReleaseError, atomic_write, canonical_json, check_relpath,
                    iso, load_json, notes_at, parse_iso, parse_manifest,
                    parse_version, sha256_bytes, sha256_file, tagged_commit,
                    utcnow)
from store import Store

import release_candidate as rc  # noqa: E402  (scripts/, on the path via common)

LIFETIME = dt.timedelta(days=14)
RESIGN_BEFORE = dt.timedelta(days=7)
VOLATILE = ("serial", "generated_at", "expires")


def _check_contained(path: str, paths: list[str], prefix: str) -> None:
    """Every path a descriptor names stays inside its own release."""
    for p in paths:
        if not check_relpath(p).startswith(prefix + "/"):
            raise ReleaseError(f"{path}: {p} is outside its own release")


def _check_release_envelope(product: Product, d: dict, path: str,
                            kind: str) -> str:
    """What every final release descriptor (stable or imported) must say of
    itself: this product, this kind, its own path, a final version, its own
    tag, and a pinned maintainer's OpenPGP key. Returns the version."""
    version = d.get("version", "")
    if d.get("schema") != SCHEMA or d.get("product") != product.slug or \
            d.get("kind") != kind or \
            path != f"releases/{version}/{DESCRIPTOR}":
        raise ReleaseError(f"{path}: not a {product.slug} {kind} release")
    v = parse_version(version, path)
    if v.prerelease or v.build:
        raise ReleaseError(f"{path}: {version} is not a final release")
    if d.get("ref") != f"refs/tags/v{version}":
        raise ReleaseError(f"{path}: not the release tagged v{version}")
    signing = d.get("signing", {})
    if signing.get("scheme") != "openpgp" or \
            signing.get("fingerprint", "").upper() not in product.maintainers:
        raise ReleaseError(f"{path}: not signed with a pinned maintainer key")
    return version


def _require(d: dict, path: str, *fields: str) -> None:
    missing = [f for f in fields if f not in d]
    if missing:
        raise ReleaseError(f"{path}: lacks {', '.join(missing)}")
    for a in d.get("artifacts", []):
        if not isinstance(a, dict) or any(
                k not in a for k in ("name", "path", "sig", "sha256")):
            raise ReleaseError(f"{path}: an artifact entry is incomplete")


def _verified_notes(builds: Store, ring: openpgp.Keyring, notes_path: str,
                    digest: str, work: Path) -> None:
    """The notes at @p notes_path are exactly @p digest and carry a pinned
    maintainer's signature."""
    if not builds.exists(notes_path) or not builds.exists(notes_path + SIG):
        raise ReleaseError(f"{notes_path}: not signed and uploaded")
    work.mkdir(parents=True, exist_ok=True)
    notes, sig = work / "notes.md", work / "notes.md.sig"
    notes.write_bytes(builds.read(notes_path))
    sig.write_bytes(builds.read(notes_path + SIG))
    if sha256_bytes(notes.read_bytes()) != digest:
        raise ReleaseError(f"{notes_path}: not the reviewed notes")
    ring.verify(notes, sig)


def admit_nightly(product: Product, builds: Store, path: str, raw: bytes,
                  artifact_pub: str, tmp: Path) -> dict:
    """A nightly descriptor is listed only if it is this product's, says what
    it is consistently, and its manifest carries the artifact key's signature
    over exactly the hashes it states."""
    d = load_json(raw, path)
    prefix = path.rsplit("/", 1)[0]
    if d.get("schema") != SCHEMA or d.get("product") != product.slug:
        raise ReleaseError(f"{path}: not a {product.slug} descriptor")
    if d.get("kind") != "nightly" or not d.get("ref", "").startswith("refs/heads/"):
        raise ReleaseError(f"{path}: not a nightly")
    _require(d, path, "version", "sums", "sums_sig", "artifacts")
    # Signed with the current artifact key, or one it replaced: a rotation
    # does not unlist the nightlies published before it.
    keys = {minisig.key_id(k): k for k in product.artifact_keys(artifact_pub)}
    signing = d.get("signing", {})
    key = keys.get(signing.get("key_id")) \
        if signing.get("scheme") == "minisign" and len(signing) == 2 else None
    if key is None:
        raise ReleaseError(f"{path}: not signed with the artifact key")
    if prefix != f"nightly/{parse_version(d['version'], path).filesafe}":
        raise ReleaseError(f"{path}: published under another version's name")
    _check_contained(path, [d["sums"], d["sums_sig"]] +
                     [a[k] for a in d["artifacts"] for k in ("path", "sig")],
                     prefix)

    sums, sig = tmp / SUMS, tmp / (SUMS + ".minisig")
    sums.write_bytes(builds.read(d["sums"]))
    sig.write_bytes(builds.read(d["sums_sig"]))
    minisig.verify([key], sums, sig)
    listed = parse_manifest(sums.read_bytes(), d["sums"])
    if listed != {a["name"]: a["sha256"] for a in d["artifacts"]}:
        raise ReleaseError(f"{path}: artifacts differ from the signed manifest")
    if any(a.get("sha256") != a.get("build_sha256") or
           a.get("transform") != "none" for a in d["artifacts"]):
        raise ReleaseError(f"{path}: a nightly is published as built")
    return d


def release_notes(git_dir: Path, version: str) -> bytes | None:
    """A release's notes as reviewed: main's copy (where a typo is fixed after
    the release) or, without one, the tagged tree's."""
    for ref in ("refs/heads/main", f"refs/tags/v{version}"):
        notes = notes_at(git_dir, ref, version)
        if notes is not None:
            return notes
    return None


def _notes_fields(version: str, notes: bytes) -> dict:
    digest = sha256_bytes(notes)
    notes_path = f"notes/{version}/{digest}.md"
    return {"notes": notes_path, "notes_sha256": digest,
            "notes_sig": notes_path + SIG}


def admit_stable(product: Product, builds: Store, path: str, raw: bytes, *,
                 git_dir: Path, state_dir: Path, cosign: str, tmp: Path,
                 authenticode_ca: str | None = None) -> tuple[dict, dict]:
    """A stable release is listed only with everything the offline stage
    promises (scripts/release/gfrelease/stable.py), checked again here.

    Returns the descriptor and its index entry. What was checked is recorded
    under the state directory, keyed by the descriptor's and the notes'
    hashes: a release is checked again only when either changes, not on every
    round (checking an installer means downloading it).
    """
    d = load_json(raw, path)
    version = _check_release_envelope(product, d, path, "stable")
    _require(d, path, "commit", "sums", "sums_sig", "build", "artifacts")
    _require(d["build"], path + " build", "sums", "bundle")
    tag = f"v{version}"
    prefix = f"releases/{version}"
    _check_contained(path, [d["sums"], d["sums_sig"], d["build"]["sums"],
                            d["build"]["bundle"]] +
                     [a[k] for a in d["artifacts"] for k in ("path", "sig")],
                     prefix)

    notes = release_notes(git_dir, version)
    if notes is None:
        raise ReleaseError(f"{tag}: no release notes to publish with it")
    entry = {"descriptor": path, "sha256": sha256_bytes(raw)} | \
        _notes_fields(version, notes)

    record = state_dir / "admitted" / f"{version}.json"
    previous = load_json(record.read_text(), str(record)) \
        if record.exists() else None
    if previous == entry:
        return d, entry
    if previous is not None and \
            {k: previous.get(k) for k in ("descriptor", "sha256")} == \
            {k: entry[k] for k in ("descriptor", "sha256")}:
        # The release itself was admitted; only main's notes moved on. Until
        # the new ones are signed and uploaded, the admitted ones stay listed
        # rather than the whole release (or the index) going away.
        try:
            with openpgp.Keyring(product.keyring_path,
                                 product.maintainers) as ring:
                _verified_notes(builds, ring, entry["notes"],
                                entry["notes_sha256"], tmp / version / "notes")
        except ReleaseError as e:
            print(f"{tag}: listed with its admitted notes; main's newer ones "
                  f"are not signed and uploaded yet (gfrelease stable "
                  f"{version} --notes-only): {e}", file=sys.stderr)
            return d, previous
        atomic_write(record, json.dumps(entry, sort_keys=True))
        return d, entry

    work = tmp / version
    work.mkdir(parents=True, exist_ok=True)
    with openpgp.Keyring(product.keyring_path, product.maintainers) as ring:
        # The tag: a maintainer's, and the commit that was built.
        ring.verify_tag(git_dir, tag)
        try:
            tagged_commit(git_dir, tag, expect=d["commit"])
        except ReleaseError as e:
            raise ReleaseError(f"{path}: {e}") from e

        # The final manifest, signed by a maintainer, and exactly the
        # descriptor's hashes.
        for name, rel in ((SUMS, d["sums"]), (SUMS + SIG, d["sums_sig"])):
            (work / name).write_bytes(builds.read(rel))
        ring.verify(work / SUMS, work / (SUMS + SIG))
        if parse_manifest((work / SUMS).read_bytes(), d["sums"]) != \
                {a["name"]: a["sha256"] for a in d["artifacts"]}:
            raise ReleaseError(f"{path}: artifacts differ from the signed "
                               "manifest")

        # The notes, signed by a maintainer.
        try:
            _verified_notes(builds, ring, entry["notes"],
                            entry["notes_sha256"], work / "notes")
        except ReleaseError as e:
            raise ReleaseError(
                f"{tag}: the notes main has for it are not signed and uploaded "
                f"yet (gfrelease stable {version} --notes-only): {e}") from e

    # What CI built, as Sigstore says: every file's build hash.
    build = work / "build"
    build.mkdir(exist_ok=True)
    (build / SUMS).write_bytes(builds.read(d["build"]["sums"]))
    (build / BUNDLE).write_bytes(builds.read(d["build"]["bundle"]))
    try:
        rc.cosign_verify(build, product.identity(d["ref"]), d["commit"], cosign)
        built = rc.parse_manifest((build / SUMS).read_bytes())
    except rc.CandidateError as e:
        raise ReleaseError(f"{path}: {e}") from e
    for a in d["artifacts"]:
        if built.get(a["name"]) != a.get("build_sha256"):
            raise ReleaseError(f"{a['name']}: not what CI built")
        if a.get("transform") == "none":
            if a["sha256"] != a["build_sha256"]:
                raise ReleaseError(f"{a['name']}: changed without a transform")
        elif a.get("transform") == "authenticode":
            local = builds.fetch_to(a["path"], work / a["name"])
            if sha256_file(local) != a["sha256"]:
                raise ReleaseError(f"{a['name']}: not the published bytes")
            # The current certificate, or one it replaced: a release stays
            # admitted however often it is checked again.
            authenticode.verify(local, product.authenticode_subjects,
                                authenticode_ca)
            local.unlink()
        else:
            raise ReleaseError(f"{a['name']}: unknown transform "
                               f"{a.get('transform')!r}")

    atomic_write(record, json.dumps(entry, sort_keys=True))
    return d, entry


def admit_imported(product: Product, builds: Store, path: str, raw: bytes, *,
                   git_dir: Path, state_dir: Path,
                   tmp: Path) -> tuple[dict, dict]:
    """A release from before this pipeline (gfrelease import): listed when
    every file verifies against its own maintainer signature, made when it
    was released, and its tag is the commit git calls that version.

    There is no signed manifest to check instead, so every file is
    downloaded and checked, once: the result is recorded under the state
    directory by the descriptor's hash, like a stable release's.
    """
    d = load_json(raw, path)
    version = _check_release_envelope(product, d, path, imported.KIND)
    floor = parse_version(product.policy["signing"]["legacy_unsigned_before"])
    if parse_version(version) < floor:
        raise ReleaseError(f"{path}: {version} is older than {floor}")
    if not d.get("artifacts"):
        raise ReleaseError(f"{path}: lists no files")
    _require(d, path, "artifacts")
    _check_contained(path, [a[k] for a in d["artifacts"]
                            for k in ("path", "sig")], f"releases/{version}")
    for a in d["artifacts"]:
        if product.classify(a["name"], version) != \
                {k: a[k] for k in ("os", "arch", "kind", "flavor", "min_os")
                 if k in a}:
            raise ReleaseError(f"{a['name']}: not what the artifact rules "
                               "call it")

    entry = {"descriptor": path, "sha256": sha256_bytes(raw)}
    record = state_dir / "admitted" / f"{version}.json"
    if not (record.exists() and
            load_json(record.read_text(), str(record)) == entry):
        _check_imported_files(product, builds, d, path, git_dir=git_dir,
                              tmp=tmp)
        atomic_write(record, json.dumps(entry, sort_keys=True))
    return d, entry | _imported_notes(product, builds, version,
                                      git_dir=git_dir, state_dir=state_dir,
                                      tmp=tmp)


def _check_imported_files(product: Product, builds: Store, d: dict, path: str,
                          *, git_dir: Path, tmp: Path) -> None:
    version, tag = d["version"], f"v{d['version']}"
    try:
        tagged_commit(git_dir, tag, expect=d.get("commit"), version=version)
    except ReleaseError as e:
        raise ReleaseError(f"{path}: {e}") from e

    work = tmp / version
    work.mkdir(parents=True, exist_ok=True)
    with openpgp.Keyring(product.keyring_path, product.maintainers) as ring:
        for a in d["artifacts"]:
            local = builds.fetch_to(a["path"], work / a["name"])
            sig = builds.fetch_to(a["sig"], work / (a["name"] + SIG))
            if sha256_file(local) != a["sha256"]:
                raise ReleaseError(f"{a['name']}: not the descriptor's bytes")
            ring.verify(local, sig)
            local.unlink()
            sig.unlink()


def _imported_notes(product: Product, builds: Store, version: str, *,
                    git_dir: Path, state_dir: Path, tmp: Path) -> dict:
    """The index fields for an imported release's notes: main's
    release/notes/<version>.md, once a maintainer has signed and uploaded it
    (gfrelease stable <version> --notes-only). Until then the release is
    listed without notes, which clients show as a link, and the host says so.
    Checked again only when main's notes change."""
    notes = release_notes(git_dir, version)
    if notes is None:
        return {}
    fields = _notes_fields(version, notes)
    record = state_dir / "admitted" / f"{version}.notes"
    if record.exists() and record.read_text() == fields["notes_sha256"]:
        return fields
    if not builds.exists(fields["notes"]) or \
            not builds.exists(fields["notes_sig"]):
        print(f"{version}: listed without notes; main's are not signed and "
              f"uploaded yet (gfrelease stable {version} --notes-only)",
              file=sys.stderr)
        return {}
    with openpgp.Keyring(product.keyring_path, product.maintainers) as ring:
        _verified_notes(builds, ring, fields["notes"], fields["notes_sha256"],
                        tmp / f"{version}-notes")
    atomic_write(record, fields["notes_sha256"])
    return fields


class IndexIncomplete(ReleaseError):
    """Some releases were refused. The index of everything else was still
    built (and published, unless nothing changed): one bad release must not
    take the nightly and every other release off the channel with it. The run
    fails all the same, naming each refusal, so it is never silent."""

    def __init__(self, refused: list[str], index: dict | None):
        super().__init__("refused, and left out of the index:\n  " +
                         "\n  ".join(refused))
        self.refused = refused
        self.index = index


def build(product: Product, builds: Store, artifact_pub: str, *,
          git_dir: Path | None = None, state_dir: Path | None = None,
          cosign: str = "cosign", authenticode_ca: str | None = None,
          refused: list[str] | None = None) -> dict:
    """The index's content, without serial and times. A release that fails
    admission is left out and appended to @p refused (when given; otherwise
    its error is raised)."""
    def refuse(path: str, error: ReleaseError) -> None:
        if refused is None:
            raise error
        refused.append(f"{path}: {error}")

    nightly = None
    releases: dict[str, dict] = {}
    with tempfile.TemporaryDirectory(prefix="gfr-index-") as tmp:
        for rel in builds.files("nightly", include=f"*/{DESCRIPTOR}"):
            path = f"nightly/{rel}"
            try:
                raw = builds.read(path)
                d = admit_nightly(product, builds, path, raw, artifact_pub,
                                  Path(tmp))
            except ReleaseError as e:
                refuse(path, e)
                continue
            # Only main's nightlies make the channel; release/X.Y builds are
            # published for testers who fetch them by hand.
            if d["ref"] != "refs/heads/main":
                continue
            if nightly is None or parse_version(d["version"]) > \
                    parse_version(nightly["version"]):
                nightly = {"version": d["version"], "descriptor": path,
                           "sha256": sha256_bytes(raw)}

        for rel in builds.files("releases", include=f"*/{DESCRIPTOR}"):
            path = f"releases/{rel}"
            if git_dir is None or state_dir is None:
                raise ReleaseError("stable releases need --git-dir and "
                                   "--state-dir to be admitted")
            try:
                raw = builds.read(path)
                if load_json(raw, path).get("kind") == imported.KIND:
                    d, entry = admit_imported(product, builds, path, raw,
                                              git_dir=git_dir,
                                              state_dir=state_dir,
                                              tmp=Path(tmp))
                else:
                    d, entry = admit_stable(product, builds, path, raw,
                                            git_dir=git_dir,
                                            state_dir=state_dir,
                                            cosign=cosign, tmp=Path(tmp),
                                            authenticode_ca=authenticode_ca)
            except ReleaseError as e:
                refuse(path, e)
                continue
            releases[d["version"]] = entry

    series = {}
    for key, entry in product.policy["series"].items():
        series[key] = {k: entry.get(k) for k in ("status", "eol", "successor")}
        series[key]["platforms"] = entry.get("platforms", {})
        series[key]["latest"] = None
        if series[key]["eol"] is not None:
            series[key]["eol"] = str(series[key]["eol"])

    # Each series' newest release; the stable channel is the newest release
    # of the series users are meant to be on (stable, or mainline once its .0
    # shipped and before the policy moved on). There is no other channel.
    withdrawn = {w["version"] for w in product.policy.get("withdrawn", [])}
    for version in sorted(releases, key=parse_version):
        if version in withdrawn:
            continue
        key = parse_version(version).series
        if key in series:
            series[key]["latest"] = version
    current = [s["latest"] for s in series.values()
               if s["status"] in ("stable", "mainline") and s["latest"]]
    stable = max(current, key=parse_version) if current else None

    return {
        "schema": SCHEMA, "product": product.slug,
        "series": series,
        "channels": {"stable": stable,
                     "nightly": nightly["version"] if nightly else None},
        "withdrawn": [{"version": w["version"], "reason": w["reason"]}
                      for w in product.policy.get("withdrawn", [])],
        "releases": releases,
        "nightly": nightly,
    }


def published(update: Store) -> dict | None:
    if not update.exists(INDEX):
        return None
    return load_json(update.read(INDEX), INDEX)


def read_serial(state_dir: Path) -> int:
    path = state_dir / "serial"
    if not path.exists():
        return 0
    try:
        return int(path.read_text())
    except ValueError as e:
        raise ReleaseError(f"{path}: not a serial") from e


def publish(product: Product, builds: Store, update: Store, *,
            index_key: Path, artifact_pub: str, state_dir: Path,
            force: bool = False, mirrors: list[Store] = (),
            git_dir: Path | None = None, cosign: str = "cosign",
            authenticode_ca: str | None = None) -> dict | None:
    """Builds, signs and publishes the index; None when it would say nothing
    new and is not near expiry. Raises IndexIncomplete, after publishing,
    when any release was refused."""
    refused: list[str] = []
    content = build(product, builds, artifact_pub, git_dir=git_dir,
                    state_dir=state_dir, cosign=cosign,
                    authenticode_ca=authenticode_ca, refused=refused)
    index = _sign_and_publish(product, content, update, index_key=index_key,
                              state_dir=state_dir, force=force,
                              mirrors=mirrors)
    if refused:
        raise IndexIncomplete(refused, index)
    return index


def _sign_and_publish(product: Product, content: dict, update: Store, *,
                      index_key: Path, state_dir: Path, force: bool,
                      mirrors: list[Store]) -> dict | None:
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
        atomic_write(state_dir / "serial", f"{serial}\n")
        for store in [update, *mirrors]:
            store.put_file(local, INDEX, immutable=False)
            store.put_file(sig, INDEX + ".minisig", immutable=False)
    return index
