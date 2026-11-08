# Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""The signed update index: v1/index.json, the one thing clients trust.

Built from the release policy and the promoted descriptors (a stable or
imported release only once its maintainer signatures verified and the server
countersigned it, countersign.py), signed with the product's index key (trusted comment "product=<slug> serial=<n>"), and
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
import shutil
import sys
import tempfile
from pathlib import Path

import authenticode
import countersign
import imported
import minisig
import openpgp
from common import (BUNDLE, DESCRIPTOR, INDEX, SCHEMA, SIG, SUMS, Product,
                    ReleaseError, atomic_write, canonical_json, check_relpath,
                    iso, load_json, notes_at, parse_iso, parse_manifest,
                    parse_version, run, sha256_bytes, sha256_file,
                    tagged_commit, utcnow)
from store import Store

import release_candidate as rc  # noqa: E402  (scripts/, on the path via common)

LIFETIME = dt.timedelta(days=14)
RESIGN_BEFORE = dt.timedelta(days=7)
# How long before the bucket's lifecycle rule deletes a nightly the index
# stops naming it: a client must never be sent to a descriptor that is gone.
NIGHTLY_MARGIN = dt.timedelta(days=7)
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
            signing.get("fingerprint", "").upper() not in \
            product.trusted_maintainers:
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


class NotesNotUploaded(ReleaseError):
    """The notes, or their signature, are not in the store yet: nothing is
    wrong, a maintainer has not signed them yet. Unlike notes that are there
    and do not verify, which refuse the release."""


def _verified_notes(builds: Store, ring: openpgp.Keyring, notes_path: str,
                    digest: str, work: Path) -> None:
    """The notes at @p notes_path are exactly @p digest and carry a pinned
    maintainer's signature."""
    if not builds.exists(notes_path) or not builds.exists(notes_path + SIG):
        raise NotesNotUploaded(f"{notes_path}: not signed and uploaded")
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


RECORD_SCHEMA = 2


def _trust(product: Product, git_dir: Path, tag: str, kind: str) -> str:
    """A digest of everything an admission rested on besides the
    descriptor: the pinned keys (and the key file they come from), the
    Authenticode subjects, and the commit the tag names. When any of them
    changes, a release is checked again from nothing: a key taken out of the
    product unlists what only it signed, a moved tag is caught."""
    tagged = run(["git", "-C", git_dir, "rev-parse", "-q", "--verify",
                  f"refs/tags/{tag}^{{commit}}"], check=False).stdout.strip()
    facts = {"kind": kind, "maintainers": sorted(product.trusted_maintainers),
             "keyring": sha256_file(product.keyring_path)
             if Path(product.keyring_path).is_file() else "",
             "tag": tagged}
    if kind == "stable":
        facts["authenticode"] = sorted(product.authenticode_subjects)
    return sha256_bytes(canonical_json(facts))


def _read_record(path: Path) -> dict | None:
    """The admission record of a release, or None when there is none or it
    is from another schema (then the release is checked again, once)."""
    if not path.exists():
        return None
    record = load_json(path.read_text(), str(path))
    return record if record.get("schema") == RECORD_SCHEMA else None


def _admit_release(product: Product, builds: Store, path: str, raw: bytes,
                   d: dict, *, kind: str, git_dir: Path, state_dir: Path,
                   tmp: Path, check_release, notes_required: bool,
                   countersigner: "countersign.Countersigner | None"
                   ) -> tuple[dict, dict]:
    """What stable and imported admission share: the record, the notes.

    The release itself is checked (@p check_release) only when the record
    does not already vouch for this descriptor under the same trust inputs.
    The notes are main's (or the tag's): new ones are listed once their
    maintainer signature verifies; until they are uploaded the ones admitted
    before stay. Notes that are uploaded but do not verify refuse the
    release, as any bad signature does. A release without admitted notes is
    refused when @p notes_required.

    Only then is it countersigned (countersign.py), and the record says so
    last: a run stopped halfway countersigns again (what is there is kept),
    and never lists a release whose countersignatures are not all there.
    Without a @p countersigner (a dry run) nothing is signed or recorded."""
    version = d["version"]
    tag = f"v{version}"
    record_path = state_dir / "admitted" / f"{version}.json"
    record = _read_record(record_path)
    base = {"descriptor": path, "sha256": sha256_bytes(raw),
            "trust": _trust(product, git_dir, tag, kind)}
    if record is None or {k: record.get(k) for k in base} != base:
        check_release()
        record = None

    previous_notes = record.get("notes") if record else None
    notes = release_notes(git_dir, version)
    wanted = _notes_fields(version, notes) if notes is not None else None
    chosen = previous_notes
    if wanted is not None and wanted != previous_notes:
        try:
            with openpgp.Keyring(product.keyring_path,
                                 product.trusted_maintainers) as ring:
                _verified_notes(builds, ring, wanted["notes"],
                                wanted["notes_sha256"], tmp / version / "notes")
            chosen = wanted
        except NotesNotUploaded as e:
            if notes_required and previous_notes is None:
                raise ReleaseError(
                    f"{tag}: the notes main has for it are not signed and "
                    f"uploaded yet (gfrelease stable {version} --notes-only): "
                    f"{e}") from e
            print(f"{version}: listed "
                  f"{'with its admitted notes' if previous_notes else 'without notes'}"
                  f"; main's are not signed and uploaded yet (gfrelease "
                  f"stable {version} --notes-only)", file=sys.stderr)
    if notes_required and chosen is None:
        raise ReleaseError(f"{tag}: no release notes to publish with it")

    entry = {"descriptor": path, "sha256": base["sha256"]} | (chosen or {}) | \
        countersign.entry_fields(d, chosen)
    if countersigner is None:
        return d, entry

    done = (record or {}).get("countersigned") or {}
    work = tmp / version
    try:
        if done.get("key") != countersigner.key_id:
            countersigner.release(builds, d, work)
            done = {"key": countersigner.key_id, "notes": None}
        if chosen and done.get("notes") != chosen["notes_sha256"]:
            countersigner.notes(builds, version, chosen, work / "notes")
    finally:
        # A release's files are big; the first run admits every imported
        # one in turn.
        shutil.rmtree(work, ignore_errors=True)
    new_record = dict(base, schema=RECORD_SCHEMA, notes=chosen,
                      countersigned={"key": countersigner.key_id,
                                     "notes": chosen["notes_sha256"]
                                     if chosen else None})
    if new_record != record:
        atomic_write(record_path, json.dumps(new_record, sort_keys=True))
    return d, entry


def admit_stable(product: Product, builds: Store, path: str, raw: bytes, *,
                 git_dir: Path, state_dir: Path, cosign: str, tmp: Path,
                 authenticode_ca: str | None = None,
                 countersigner: "countersign.Countersigner | None" = None
                 ) -> tuple[dict, dict]:
    """A stable release is listed only with everything the offline stage
    promises (scripts/release/gfrelease/stable.py), checked again here: the
    maintainer's signed tag and manifest, what CI built, the Authenticode
    signatures, and signed notes.

    Returns the descriptor and its index entry. What was checked is recorded
    under the state directory (_admit_release): checking an installer means
    downloading it, so it is done again only when something it rested on
    changes.
    """
    d = load_json(raw, path)
    version = _check_release_envelope(product, d, path, "stable")
    _require(d, path, "commit", "sums", "sums_sig", "build", "artifacts")
    _require(d["build"], path + " build", "sums", "bundle")
    _check_contained(path, [d["sums"], d["sums_sig"], d["build"]["sums"],
                            d["build"]["bundle"]] +
                     [a[k] for a in d["artifacts"] for k in ("path", "sig")],
                     f"releases/{version}")
    return _admit_release(
        product, builds, path, raw, d, kind="stable", git_dir=git_dir,
        state_dir=state_dir, tmp=tmp, notes_required=True,
        countersigner=countersigner,
        check_release=lambda: _check_stable(
            product, builds, d, path, git_dir=git_dir, cosign=cosign,
            tmp=tmp, authenticode_ca=authenticode_ca))


def _check_stable(product: Product, builds: Store, d: dict, path: str, *,
                  git_dir: Path, cosign: str, tmp: Path,
                  authenticode_ca: str | None) -> None:
    version, tag = d["version"], f"v{d['version']}"
    work = tmp / version
    work.mkdir(parents=True, exist_ok=True)
    with openpgp.Keyring(product.keyring_path,
                         product.trusted_maintainers) as ring:
        # The tag: a maintainer's, and the commit that was built.
        ring.verify_tag(git_dir, tag)
        try:
            tagged_commit(git_dir, tag, expect=d["commit"])
        except ReleaseError as e:
            raise ReleaseError(f"{path}: {e}") from e

        # The final manifest, signed by a maintainer, and exactly the
        # descriptor's hashes.
        sums = builds.fetch_to(d["sums"], work / SUMS)
        ring.verify(sums, builds.fetch_to(d["sums_sig"], work / (SUMS + SIG)))
        if parse_manifest(sums.read_bytes(), d["sums"]) != \
                {a["name"]: a["sha256"] for a in d["artifacts"]}:
            raise ReleaseError(f"{path}: artifacts differ from the signed "
                               "manifest")

        # Every file's own signature, as published beside it: a release
        # with one missing or invalid is not listed. The downloads stay in
        # @p work for the countersignature.
        for a in d["artifacts"]:
            local = builds.fetch_to(a["path"], work / a["name"])
            if sha256_file(local) != a["sha256"]:
                raise ReleaseError(f"{a['name']}: not the published bytes")
            ring.verify(local, builds.fetch_to(a["sig"],
                                               work / (a["name"] + SIG)))

    # What CI built, as Sigstore says: every file's build hash.
    build = work / "build"
    build.mkdir(exist_ok=True)
    builds.fetch_to(d["build"]["sums"], build / SUMS)
    builds.fetch_to(d["build"]["bundle"], build / BUNDLE)
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
            # Downloaded and hashed above, with its signature.
            # The current certificate, or one it replaced: a release stays
            # admitted however often it is checked again.
            authenticode.verify(work / a["name"], product.authenticode_subjects,
                                authenticode_ca)
        else:
            raise ReleaseError(f"{a['name']}: unknown transform "
                               f"{a.get('transform')!r}")


def admit_imported(product: Product, builds: Store, path: str, raw: bytes, *,
                   git_dir: Path, state_dir: Path, tmp: Path,
                   countersigner: "countersign.Countersigner | None" = None
                   ) -> tuple[dict, dict]:
    """A release from before this pipeline (gfrelease import): listed when
    every file verifies against its own maintainer signature, made when it
    was released, and its tag is the commit git calls that version.

    There is no signed manifest to check instead, so every file is
    downloaded and checked, once (_admit_release records it). Its notes are
    optional: those releases predate reviewed notes, and are listed without
    until a maintainer signs main's (gfrelease stable <v> --notes-only).
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
    return _admit_release(
        product, builds, path, raw, d, kind=imported.KIND, git_dir=git_dir,
        state_dir=state_dir, tmp=tmp, notes_required=False,
        countersigner=countersigner,
        check_release=lambda: _check_imported_files(
            product, builds, d, path, git_dir=git_dir, tmp=tmp))


def _check_imported_files(product: Product, builds: Store, d: dict, path: str,
                          *, git_dir: Path, tmp: Path) -> None:
    version, tag = d["version"], f"v{d['version']}"
    try:
        tagged_commit(git_dir, tag, expect=d.get("commit"), version=version)
    except ReleaseError as e:
        raise ReleaseError(f"{path}: {e}") from e

    work = tmp / version
    work.mkdir(parents=True, exist_ok=True)
    with openpgp.Keyring(product.keyring_path,
                         product.trusted_maintainers) as ring:
        for a in d["artifacts"]:
            local = builds.fetch_to(a["path"], work / a["name"])
            sig = builds.fetch_to(a["sig"], work / (a["name"] + SIG))
            if sha256_file(local) != a["sha256"]:
                raise ReleaseError(f"{a['name']}: not the descriptor's bytes")
            ring.verify(local, sig)


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
          refused: list[str] | None = None,
          countersigner: "countersign.Countersigner | None" = None) -> dict:
    """The index's content, without serial and times. A release that fails
    admission is left out and appended to @p refused (when given; otherwise
    its error is raised)."""
    def refuse(path: str, error: ReleaseError) -> None:
        if refused is None:
            raise error
        refused.append(f"{path}: {error}")

    nightly = None
    releases: dict[str, dict] = {}
    retention = product.data.get("nightly_retention_days")
    cutoff = utcnow() - dt.timedelta(days=retention) + NIGHTLY_MARGIN \
        if retention else None
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
            # Soon deleted by the bucket's lifecycle rule: never named.
            if cutoff is not None and \
                    parse_iso(d.get("promoted_at", "")) < cutoff:
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
                                              tmp=Path(tmp),
                                              countersigner=countersigner)
                else:
                    d, entry = admit_stable(product, builds, path, raw,
                                            git_dir=git_dir,
                                            state_dir=state_dir,
                                            cosign=cosign, tmp=Path(tmp),
                                            authenticode_ca=authenticode_ca,
                                            countersigner=countersigner)
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
    if cutoff is not None and nightly is None and \
            builds.files("nightly", include=f"*/{DESCRIPTOR}"):
        print(f"no main nightly promoted since {iso(cutoff)}: the nightly "
              "channel names none until main is built again", file=sys.stderr)
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


def dry_run(product: Product, builds: Store, update: Store, *,
            artifact_pub: str, state_dir: Path, git_dir: Path | None = None,
            cosign: str = "cosign", authenticode_ca: str | None = None
            ) -> dict:
    """Everything an index run decides, and nothing it writes: admission
    runs against a copy of the state directory, nothing is signed, uploaded
    or countersigned. Returns what would be listed, what was refused, and
    how it differs from the published index."""
    refused: list[str] = []
    with tempfile.TemporaryDirectory(prefix="gfr-dry-") as tmp:
        state = Path(tmp) / "state"
        if Path(state_dir).is_dir():
            shutil.copytree(state_dir, state)
        content = build(product, builds, artifact_pub, git_dir=git_dir,
                        state_dir=state, cosign=cosign,
                        authenticode_ca=authenticode_ca, refused=refused)
    previous = published(update) or {}
    before = {k: v for k, v in previous.items() if k not in VOLATILE}
    return {
        "channels": content["channels"],
        "releases": sorted(content["releases"], key=parse_version),
        "nightly": (content["nightly"] or {}).get("version"),
        "refused": refused,
        "changes": sorted(k for k in set(before) | set(content)
                          if before.get(k) != content.get(k)),
        "added": sorted(set(content["releases"]) -
                        set(before.get("releases", {}))),
        "removed": sorted(set(before.get("releases", {})) -
                          set(content["releases"])),
    }


def publish(product: Product, builds: Store, update: Store, *,
            index_key: Path, artifact_pub: str, state_dir: Path,
            artifact_key: Path | None = None,
            force: bool = False, mirrors: list[Store] = (),
            git_dir: Path | None = None, cosign: str = "cosign",
            authenticode_ca: str | None = None) -> dict | None:
    """Builds, signs and publishes the index; None when it would say nothing
    new and is not near expiry. Raises IndexIncomplete, after publishing,
    when any release was refused."""
    refused: list[str] = []
    content = build(product, builds, artifact_pub, git_dir=git_dir,
                    state_dir=state_dir, cosign=cosign,
                    authenticode_ca=authenticode_ca, refused=refused,
                    countersigner=countersign.Countersigner(
                        product, artifact_key, artifact_pub)
                    if artifact_key else None)
    if artifact_key is None and content["releases"]:
        # Nothing is listed that the server has not countersigned.
        raise ReleaseError("stable releases need --artifact-key to be "
                           "countersigned and listed")
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
