# Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""The signed update index: v1/index.json, the one thing clients trust.

Built from the release policy and the descriptors in the builds store: main's
newest promoted build is the nightly channel, a release branch's is the beta
of its series, and a stable or imported release is listed only once its
maintainer signatures verified. Signed with the product's index key
(trusted comment "product=<slug> serial=<n>") and published to the update
store, then the mirrors. A run whose index would say nothing new, and is
not near expiry, publishes nothing; `resign` publishes regardless, to push
`expires` out.

The serial only grows: it is one more than both the host's own record and the
published index, so a client that has seen serial n never accepts an older
index again (its anti-rollback check), even after the host lost its state.
Of the published index only one that verifies with a pinned index key is
believed, for its serial as for anything else: the update store is a bucket,
and what sits in it is not the host's word until the signature says so.
"""

from __future__ import annotations

import datetime as dt
import json
import os
import shutil
import sys
import tempfile
from pathlib import Path

import authenticode
import descriptor
import minisig
import openpgp
import layout
from common import (BUNDLE, DESCRIPTOR, INDEX, MINISIG, SCHEMA, SIG, SUMS,
                    Problems, Product, ReleaseError, atomic_write,
                    candidate_error, is_passing, canonical_json,
                    check_relpath, git_version, iso, load_json, locked,
                    notes_at, parse_iso, parse_manifest, parse_version, refs,
                    resolve_commit, sha256_bytes, sha256_file, tag_time,
                    tagged_commit, utcnow)
from store import Store, StoreUnavailable

import policy as policy_mod  # noqa: E402  (scripts/release/, via common)
import release_candidate as rc  # noqa: E402  (scripts/, on the path via common)

LIFETIME = dt.timedelta(days=policy_mod.INDEX_LIFETIME_DAYS)
RESIGN_BEFORE = dt.timedelta(days=7)
# How long before the bucket's lifecycle rule deletes a nightly the index
# stops naming it: longer than an index lives, so a client never holds a
# valid index that sends it to a descriptor that is gone.
NIGHTLY_MARGIN = dt.timedelta(days=policy_mod.NIGHTLY_MARGIN_DAYS)
VOLATILE = ("serial", "generated_at", "expires")
# How many used serials a host that lost its state steps over before it
# concludes the history is not its own.
SERIAL_SKIPS = 64


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
            d.get("kind") != kind or not isinstance(version, str) or \
            path != layout.descriptor_of(layout.release_dir(version)):
        raise ReleaseError(f"{path}: not a {product.slug} {kind} release")
    if not parse_version(version, path).is_release:
        raise ReleaseError(f"{path}: {version} is not a final release")
    if d.get("ref") != refs.release_tag_ref(version):
        raise ReleaseError(f"{path}: not the release tagged v{version}")
    signing = d.get("signing", {})
    if signing.get("scheme") != layout.SCHEME_OPENPGP or \
            signing.get("fingerprint", "").upper() not in \
            product.trusted_maintainers:
        raise ReleaseError(f"{path}: not signed with a pinned maintainer key")
    return version


def _check_classified(product: Product, d: dict, path: str,
                      filesafe: str) -> None:
    """Every artifact is what the product's rules call its name: a client
    picks its download by these fields."""
    for a in d["artifacts"]:
        if product.classify(a["name"], filesafe) != \
                {k: a[k] for k in ("os", "arch", "kind", "flavor", "min_os")
                 if k in a}:
            raise ReleaseError(f"{path}: {a['name']} is not what the "
                               "artifact rules call it")


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
                  tmp: Path, *, git_dir: Path | None, cosign: str) -> dict:
    """A promoted build (main's, a nightly; a release branch's, a beta) is
    listed only if its descriptor is this product's, says what it is
    consistently, and what promotion checked holds again: the Sigstore
    bundle under build/ names this product's workflow at the descriptor's
    ref and commit, that manifest states exactly the descriptor's hashes,
    and git calls that commit this version. The builds store is a bucket:
    nothing in it is the host's word until it is checked."""
    d = load_json(raw, path)
    descriptor.check_shape(d, path)
    prefix = path.rsplit("/", 1)[0]
    if d.get("schema") != SCHEMA or d.get("product") != product.slug:
        raise ReleaseError(f"{path}: not a {product.slug} descriptor")
    ref = d.get("ref", "")
    if d.get("kind") != layout.KIND_NIGHTLY or not refs.publishes_builds(ref):
        raise ReleaseError(f"{path}: not a build of main or of a release "
                           "branch")
    descriptor.require(d, path, "version", "commit", "build", "artifacts",
                       "promoted_at")
    descriptor.require(d["build"], path + " build", "sums", "bundle")
    try:
        parse_iso(d["promoted_at"])
    except ReleaseError as e:
        raise ReleaseError(f"{path}: promoted_at: {e}") from e
    v = parse_version(d['version'], path)
    if v.is_final:
        raise ReleaseError(f"{path}: {d['version']} is a final version, which "
                           "a nightly never is")
    # A release branch builds its own series and no other: the beta of 2.2
    # is never a build that calls itself 2.3.
    series = refs.release_series(ref)
    if series and v.series != series:
        raise ReleaseError(f"{path}: {d['version']} is not a build of the "
                           f"{series} series its branch names")
    if prefix != layout.nightly_dir(d["version"]):
        raise ReleaseError(f"{path}: published under another version's name")
    _check_contained(path, [d["build"]["sums"], d["build"]["bundle"]] +
                     [a["path"] for a in d["artifacts"]], prefix)
    _check_classified(product, d, path, v.filesafe)
    if any(a.get("sha256") != a.get("build_sha256") or
           a.get("transform") != "none" for a in d["artifacts"]):
        raise ReleaseError(f"{path}: a nightly is published as built")

    build = tmp / v.filesafe
    build.mkdir(parents=True, exist_ok=True)
    builds.fetch_to(d["build"]["sums"], build / SUMS)
    builds.fetch_to(d["build"]["bundle"], build / BUNDLE)
    try:
        rc.cosign_verify(build, product.identity(ref), d["commit"], cosign)
    except (rc.CandidateError, rc.CandidateTimeout) as e:
        raise candidate_error(path, e) from e
    if parse_manifest((build / SUMS).read_bytes(), d["build"]["sums"]) != \
            descriptor.hashes(d):
        raise ReleaseError(f"{path}: artifacts differ from the signed manifest")
    if git_dir is None:
        raise ReleaseError("builds need --git-dir to be admitted")
    derived = git_version(git_dir, d["commit"], refs.branch_of(ref))
    if derived != d["version"]:
        raise ReleaseError(f"{path}: git says {d['commit'][:12]} on "
                           f"{refs.branch_of(ref)} is {derived}, not "
                           f"{d['version']}")
    return d


def release_notes(git_dir: Path, version: str) -> bytes | None:
    """A release's notes as reviewed: main's copy (where a typo is fixed after
    the release) or, without one, the tagged tree's."""
    for ref in (refs.MAIN, refs.release_tag_ref(version)):
        notes = notes_at(git_dir, ref, version)
        if notes is not None:
            return notes
    return None


def _notes_fields(version: str, notes: bytes) -> dict:
    digest = sha256_bytes(notes)
    notes_path = layout.notes_path(version, digest)
    return {"notes": notes_path, "notes_sha256": digest,
            "notes_sig": notes_path + SIG}


RECORD_SCHEMA = 4


def _trust(product: Product, git_dir: Path, tag: str, kind: str) -> str:
    """A digest of everything an admission rested on besides the
    descriptor: the pinned keys (and the key file they come from), the
    Authenticode subjects, the commit the tag names and when the tag was
    made. When any of them changes, a release is checked again from
    nothing: a key taken out of the product unlists what only it signed, a
    moved or re-dated tag is caught."""
    tagged = resolve_commit(git_dir, refs.TAGS + tag) or ""
    try:
        tagged_at = tag_time(git_dir, tag) if tagged else None
    except ReleaseError:
        tagged_at = None
    facts = {"kind": kind, "maintainers": sorted(product.trusted_maintainers),
             "keyring": sha256_file(product.keyring_path)
             if Path(product.keyring_path).is_file() else "",
             "tag": tagged, "tag_time": tagged_at}
    if kind == layout.KIND_STABLE:
        facts["authenticode"] = sorted(product.authenticode_subjects)
    return sha256_bytes(canonical_json(facts))


def _read_record(path: Path) -> dict | None:
    """What was admitted of a release before, under whatever trust and
    schema; None when nothing was."""
    if not path.exists():
        return None
    return load_json(path.read_text(), str(path))


def _iso_of(created: int) -> str:
    return iso(dt.datetime.fromtimestamp(created, dt.timezone.utc))


def _check_signature_times(signatures: dict, stale: dict | None,
                           tag: str, tagged_at: int) -> None:
    """The maintainer signatures a release rests on: none older than its
    tag (an older release's files, relabelled, were signed before it), and
    none older than the one admitted before for that file (the build a
    hotfix replaced, offered again, is a replay). A newer one is a hotfix,
    and is admitted."""
    before = (stale or {}).get("signatures") or {}
    for name, sig in signatures.items():
        if sig["created"] < tagged_at:
            raise ReleaseError(
                f"{name}: signed at {_iso_of(sig['created'])}, before its tag "
                f"{tag} ({_iso_of(tagged_at)})")
        if name in before and sig["created"] < before[name]["created"]:
            raise ReleaseError(
                f"{name}: signed at {_iso_of(sig['created'])}, older than the "
                f"signature admitted before ({_iso_of(before[name]['created'])}"
                "): a replay is never admitted")


def _signature_fields(signatures: dict) -> dict:
    """The index entry's record of when each file's maintainer signature
    was made, and by whom (the manifest's is not a file a client gets)."""
    return {"signatures": {
        name: {"signer": sig["signer"], "created": _iso_of(sig["created"])}
        for name, sig in sorted(signatures.items()) if name != SUMS}}


def _append_history(path: Path, line: dict) -> None:
    """One line per change of what a release rests on; never rewritten."""
    path.parent.mkdir(parents=True, exist_ok=True)
    with open(path, "a", encoding="utf-8") as f:
        f.write(json.dumps(line, sort_keys=True) + "\n")
        f.flush()
        os.fsync(f.fileno())


def _admit_release(product: Product, builds: Store, path: str, raw: bytes,
                   d: dict, *, kind: str, git_dir: Path, state_dir: Path,
                   tmp: Path, check_release, notes_required: bool,
                   dry: bool) -> tuple[dict, dict]:
    """What stable and imported admission share: the record, the signature
    times, the notes.

    The release itself is checked (@p check_release, which returns every
    maintainer signature it verified) only when the record does not already
    vouch for this descriptor under the same trust inputs. A release keeps
    the kind it was first admitted as. The notes are main's (or the tag's):
    new ones are listed once their maintainer signature verifies; until they
    are uploaded the ones admitted before stay (after a trust change, once
    they verify again). Notes that are uploaded but do not verify refuse the
    release, as any bad signature does. A release without admitted notes is
    refused when @p notes_required.

    Then its commit marker is written (the application asks it whether a
    build is official), and the record last. Every change of its signatures
    is appended to <version>.history.jsonl. A @p dry run writes nothing to
    the store, and records into a copy of the state directory."""
    version = d["version"]
    tag = f"v{version}"
    record_path = state_dir / "admitted" / f"{version}.json"
    stale = _read_record(record_path)
    if stale is not None and stale.get("kind", kind) != kind:
        raise ReleaseError(f"{path}: {version} was admitted as "
                           f"{stale['kind']}, and a release never changes "
                           "kind")
    base = {"descriptor": path, "sha256": sha256_bytes(raw),
            "trust": _trust(product, git_dir, tag, kind)}
    record = stale if stale is not None and \
        stale.get("schema") == RECORD_SCHEMA and \
        {k: stale.get(k) for k in base} == base else None
    if record is None:
        tagged_at = tag_time(git_dir, tag)
        try:
            signatures = check_release()
        finally:
            # A release's files are big; the first run admits every
            # imported one in turn.
            shutil.rmtree(tmp / version, ignore_errors=True)
        _check_signature_times(signatures, stale, tag, tagged_at)
    else:
        signatures = record["signatures"]

    previous_notes = record.get("notes") if record else None
    if record is None and stale is not None and stale.get("notes"):
        # Admitted under other trust: kept only once they verify again.
        try:
            with openpgp.Keyring.of(product) as ring:
                _verified_notes(builds, ring, stale["notes"]["notes"],
                                stale["notes"]["notes_sha256"],
                                tmp / version / "stale-notes")
            previous_notes = stale["notes"]
        except StoreUnavailable:
            # Not an answer about the notes: the release keeps its entry
            # for this round (build), and is checked again on the next.
            raise
        except ReleaseError:
            previous_notes = None
    notes = release_notes(git_dir, version)
    wanted = _notes_fields(version, notes) if notes is not None else None
    chosen = previous_notes
    if wanted is not None and wanted != previous_notes:
        try:
            with openpgp.Keyring.of(product) as ring:
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

    entry = {"descriptor": path, "sha256": base["sha256"]} | \
        (chosen or {}) | _signature_fields(signatures)
    new_record = dict(base, schema=RECORD_SCHEMA, kind=kind, notes=chosen,
                      signatures=signatures)
    if dry or new_record == record:
        return d, entry
    marker = layout.commit_marker(d["commit"])
    if not builds.exists(marker):
        builds.put_bytes(b"", marker)
    previous = (stale or {}).get("signatures")
    if signatures != previous:
        # Before the record: a crash in between repeats a line, never
        # loses one.
        _append_history(state_dir / "admitted" / f"{version}.history.jsonl",
                        {"at": iso(utcnow()), "descriptor": path,
                         "descriptor_sha256": base["sha256"],
                         "signatures": signatures, "previous": previous})
    atomic_write(record_path, json.dumps(new_record, sort_keys=True))
    return d, entry


def admit_stable(product: Product, builds: Store, path: str, raw: bytes, *,
                 git_dir: Path, state_dir: Path, cosign: str, tmp: Path,
                 authenticode_ca: str | None = None, dry: bool = False
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
    descriptor.check_shape(d, path)
    version = _check_release_envelope(product, d, path, layout.KIND_STABLE)
    descriptor.require(d, path, "commit", "sums", "sums_sig", "build", "artifacts")
    descriptor.require(d["build"], path + " build", "sums", "bundle")
    _require_sigs(d, path)
    _check_contained(path, [d["sums"], d["sums_sig"], d["build"]["sums"],
                            d["build"]["bundle"]] +
                     [a[k] for a in d["artifacts"] for k in ("path", "sig")],
                     layout.release_dir(version))
    _check_classified(product, d, path, version)
    return _admit_release(
        product, builds, path, raw, d, kind=layout.KIND_STABLE,
        git_dir=git_dir,
        state_dir=state_dir, tmp=tmp, notes_required=True, dry=dry,
        check_release=lambda: _check_stable(
            product, builds, d, path, git_dir=git_dir, cosign=cosign,
            tmp=tmp, authenticode_ca=authenticode_ca))


def _require_sigs(d: dict, path: str) -> None:
    """A release's every file carries its maintainer's OpenPGP signature."""
    if any(not isinstance(a.get("sig"), str) for a in d["artifacts"]):
        raise ReleaseError(f"{path}: a file without its signature")


def _signed(sig: dict, sha256: str) -> dict:
    return {"sha256": sha256, "signer": sig["primary"],
            "created": sig["created"]}


def _check_stable(product: Product, builds: Store, d: dict, path: str, *,
                  git_dir: Path, cosign: str, tmp: Path,
                  authenticode_ca: str | None) -> dict:
    """Every check of a stable release; returns the maintainer signatures
    it rests on, by file name (and the manifest's)."""
    version, tag = d["version"], f"v{d['version']}"
    signatures = {}
    work = tmp / version
    work.mkdir(parents=True, exist_ok=True)
    with openpgp.Keyring.of(product) as ring:
        # The tag: a maintainer's, and the commit that was built.
        ring.verify_tag(git_dir, tag)
        try:
            tagged_commit(git_dir, tag, expect=d["commit"])
        except ReleaseError as e:
            raise ReleaseError(f"{path}: {e}") from e

        # The final manifest, signed by a maintainer, and exactly the
        # descriptor's hashes.
        sums = builds.fetch_to(d["sums"], work / SUMS)
        signatures[SUMS] = _signed(ring.verify_signature(
            sums, builds.fetch_to(d["sums_sig"], work / (SUMS + SIG))),
            sha256_file(sums))
        if parse_manifest(sums.read_bytes(), d["sums"]) != \
                descriptor.hashes(d):
            raise ReleaseError(f"{path}: artifacts differ from the signed "
                               "manifest")

        # Every file's own signature, as published beside it: a release
        # with one missing or invalid is not listed.
        for a in d["artifacts"]:
            local = builds.fetch_to(a["path"], work / a["name"])
            if sha256_file(local) != a["sha256"]:
                raise ReleaseError(f"{a['name']}: not the published bytes")
            signatures[a["name"]] = _signed(ring.verify_signature(
                local, builds.fetch_to(a["sig"], work / (a["name"] + SIG))),
                a["sha256"])

    # What CI built, as Sigstore says: every file's build hash.
    build = work / "build"
    build.mkdir(exist_ok=True)
    builds.fetch_to(d["build"]["sums"], build / SUMS)
    builds.fetch_to(d["build"]["bundle"], build / BUNDLE)
    try:
        rc.cosign_verify(build, product.identity(d["ref"]), d["commit"], cosign)
    except (rc.CandidateError, rc.CandidateTimeout) as e:
        raise candidate_error(path, e) from e
    built = parse_manifest((build / SUMS).read_bytes(), d["build"]["sums"])
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
                                authenticode_ca,
                                require_timestamp=product
                                .authenticode_timestamp_required)
        else:
            raise ReleaseError(f"{a['name']}: unknown transform "
                               f"{a.get('transform')!r}")
    return signatures


def admit_imported(product: Product, builds: Store, path: str, raw: bytes, *,
                   git_dir: Path, state_dir: Path, tmp: Path,
                   dry: bool = False) -> tuple[dict, dict]:
    """A release from before this pipeline (gfrelease import): listed when
    every file verifies against its own maintainer signature, made when it
    was released, and its tag is the commit git calls that version.

    There is no signed manifest to check instead, so every file is
    downloaded and checked, once (_admit_release records it). Its notes are
    optional: those releases predate reviewed notes, and are listed without
    until a maintainer signs main's (gfrelease stable <v> --notes-only).
    """
    d = load_json(raw, path)
    descriptor.check_shape(d, path)
    version = _check_release_envelope(product, d, path, layout.KIND_IMPORTED)
    floor = product.legacy_floor
    if parse_version(version) < floor:
        raise ReleaseError(f"{path}: {version} is older than {floor}")
    if not d.get("artifacts"):
        raise ReleaseError(f"{path}: lists no files")
    descriptor.require(d, path, "artifacts", "commit")
    _require_sigs(d, path)
    # Each file is checked on its own signature; there is no manifest.
    if "sums" in d or "sums_sig" in d:
        raise ReleaseError(f"{path}: an imported release names no manifest")
    _check_contained(path, [a[k] for a in d["artifacts"]
                            for k in ("path", "sig")],
                     layout.release_dir(version))
    _check_classified(product, d, path, version)
    return _admit_release(
        product, builds, path, raw, d, kind=layout.KIND_IMPORTED,
        git_dir=git_dir,
        state_dir=state_dir, tmp=tmp, notes_required=False, dry=dry,
        check_release=lambda: _check_imported_files(
            product, builds, d, path, git_dir=git_dir, tmp=tmp))


def _check_imported_files(product: Product, builds: Store, d: dict, path: str,
                          *, git_dir: Path, tmp: Path) -> dict:
    version, tag = d["version"], f"v{d['version']}"
    try:
        tagged_commit(git_dir, tag, expect=d.get("commit"), version=version)
    except ReleaseError as e:
        raise ReleaseError(f"{path}: {e}") from e

    work = tmp / version
    work.mkdir(parents=True, exist_ok=True)
    signatures = {}
    with openpgp.Keyring.of(product) as ring:
        for a in d["artifacts"]:
            local = builds.fetch_to(a["path"], work / a["name"])
            sig = builds.fetch_to(a["sig"], work / (a["name"] + SIG))
            if sha256_file(local) != a["sha256"]:
                raise ReleaseError(f"{a['name']}: not the descriptor's bytes")
            signatures[a["name"]] = _signed(ring.verify_signature(local, sig),
                                            a["sha256"])
    return signatures


class IndexIncomplete(ReleaseError):
    """Some releases were refused, or could not be checked, or a mirror
    could not be written. The index of everything else was still built (and
    published, unless nothing changed): one bad release must not take the
    nightly and every other release off the channel with it. The run fails
    all the same, naming each one, so it is never silent. @p refused says
    whether running again may help (common.Problems)."""

    def __init__(self, refused: Problems, index: dict | None):
        super().__init__("refused or not checked (the index lists "
                         "everything else):\n  " + "\n  ".join(refused))
        self.refused = refused
        self.passing = refused.passing
        self.index = index


def _kept_builds(previous: dict | None, path: str) -> tuple[dict | None,
                                                            list[dict]]:
    """What the published index lists for the descriptor at @p path, which
    could not be read this round: (as the nightly, as betas)."""
    previous = previous or {}
    nightly = previous.get("nightly")
    nightly = nightly if descriptor.names_build(nightly) and \
        nightly["descriptor"] == path else None
    series = previous.get("series")
    betas = [previous.get("beta")] + [
        s.get("beta") for s in (series.values() if isinstance(series, dict)
                                else []) if isinstance(s, dict)]
    return nightly, [b for b in betas
                     if descriptor.names_build(b) and b["descriptor"] == path][:1]


def build(product: Product, builds: Store, *,
          git_dir: Path | None = None, state_dir: Path | None = None,
          cosign: str = "cosign", authenticode_ca: str | None = None,
          refused: Problems | None = None, dry: bool = False,
          previous: dict | None = None) -> dict:
    """The index's content, without serial and times. A release that fails
    admission is left out and appended to @p refused (when given; otherwise
    its error is raised). One that could not be checked (a store that did
    not answer, a tool that did not run: common.is_passing) keeps its entry
    from @p previous, the published index (one whose signature verified,
    trusted_published), and is named in @p refused all the same: an outage
    never takes a release off the channel, and never passes unnoticed."""
    def refuse(path: str, error: ReleaseError) -> None:
        if refused is None:
            raise error
        refused.note(path, error)

    withdrawn = {parse_version(w["version"])
                 for w in product.policy.get("withdrawn", [])}
    nightly = None
    betas: list[dict] = []
    releases: dict[str, dict] = {}
    retention = product.data.get("nightly_retention_days")
    cutoff = utcnow() - dt.timedelta(days=retention) + NIGHTLY_MARGIN \
        if retention else None

    def newer(a: dict, b: dict | None) -> bool:
        return b is None or \
            parse_version(a["version"]) > parse_version(b["version"])

    with tempfile.TemporaryDirectory(prefix="gfr-index-") as tmp:
        promoted = builds.files(layout.NIGHTLY, include=f"*/{DESCRIPTOR}")
        for rel in promoted:
            path = f"{layout.NIGHTLY}/{rel}"
            try:
                raw = builds.read(path)
                # Soon deleted by the bucket's lifecycle rule: never named,
                # and not even checked (its files may be going already).
                promoted_at = load_json(raw, path).get("promoted_at")
                if cutoff is not None and isinstance(promoted_at, str):
                    try:
                        if parse_iso(promoted_at) < cutoff:
                            continue
                    except ReleaseError:
                        pass  # admit_nightly refuses it
                d = admit_nightly(product, builds, path, raw, Path(tmp),
                                  git_dir=git_dir, cosign=cosign)
            except ReleaseError as e:
                refuse(path, e)
                if not is_passing(e):
                    continue
                kept, kept_betas = _kept_builds(previous, path)
                # Kept as listed, but never past what the policy says now.
                if kept is not None and \
                        parse_version(kept["version"]) not in withdrawn and \
                        newer(kept, nightly):
                    nightly = kept
                betas += [b for b in kept_betas
                          if parse_version(b["version"]) not in withdrawn]
                continue
            if parse_version(d["version"]) in withdrawn:
                continue
            listed = {"version": d["version"], "descriptor": path,
                      "sha256": sha256_bytes(raw)}
            # main's builds are the nightly channel; a release/X.Y branch's
            # are the beta of that series (which one is chosen below, once
            # the releases are known). admit_nightly admits no other ref.
            if d["ref"] != refs.MAIN:
                betas.append(listed)
            elif newer(listed, nightly):
                nightly = listed

        for rel in builds.files(layout.RELEASES, include=f"*/{DESCRIPTOR}"):
            path = f"{layout.RELEASES}/{rel}"
            if git_dir is None or state_dir is None:
                raise ReleaseError("stable releases need --git-dir and "
                                   "--state-dir to be admitted")
            try:
                raw = builds.read(path)
                if load_json(raw, path).get("kind") == layout.KIND_IMPORTED:
                    d, entry = admit_imported(product, builds, path, raw,
                                              git_dir=git_dir,
                                              state_dir=state_dir,
                                              tmp=Path(tmp), dry=dry)
                else:
                    d, entry = admit_stable(product, builds, path, raw,
                                            git_dir=git_dir,
                                            state_dir=state_dir,
                                            cosign=cosign, tmp=Path(tmp),
                                            authenticode_ca=authenticode_ca,
                                            dry=dry)
            except ReleaseError as e:
                refuse(path, e)
                if not is_passing(e):
                    continue
                version = rel.split("/", 1)[0]
                listed = (previous or {}).get("releases")
                listed = listed if isinstance(listed, dict) else {}
                if isinstance(listed.get(version), dict) and \
                        listed[version].get("descriptor") == path:
                    releases[version] = listed[version]
                continue
            releases[d["version"]] = entry

    series = {}
    for key, entry in product.policy["series"].items():
        series[key] = {k: entry.get(k) for k in ("status", "eol", "successor")}
        series[key]["platforms"] = entry.get("platforms", {})
        series[key]["latest"] = None
        series[key]["beta"] = None
        if series[key]["eol"] is not None:
            series[key]["eol"] = str(series[key]["eol"])

    # Each series' newest release; the stable channel is the newest release
    # of the series users are meant to be on (stable, or mainline once its .0
    # shipped and before the policy moved on). The nightly and beta channels
    # are builds, never releases.
    for version in sorted(releases, key=parse_version):
        if parse_version(version) in withdrawn:
            continue
        key = parse_version(version).series
        if key in series:
            series[key]["latest"] = version
    if cutoff is not None and nightly is None and promoted:
        print(f"no main nightly promoted since {iso(cutoff)}: the nightly "
              "channel names none until main is built again", file=sys.stderr)
    current = [s["latest"] for s in series.values()
               if s["status"] in policy_mod.CURRENT_STATUSES and s["latest"]]
    stable = max(current, key=parse_version) if current else None

    # The beta of a series: its release branch's newest build, when users
    # are meant to be on that series and the build is newer than its latest
    # release. Admitted as a nightly is; a user opts in, and is only ever
    # offered the beta of the series they run.
    for b in betas:
        v = parse_version(b["version"])
        entry = series.get(v.series)
        if entry is None or \
                entry["status"] not in policy_mod.CURRENT_STATUSES or \
                (entry["latest"] is not None and
                 v <= parse_version(entry["latest"])):
            continue
        if newer(b, entry["beta"]):
            entry["beta"] = b
    # The newest of them all, where clients older than the per-series entry
    # look for it.
    beta = None
    for entry in series.values():
        if entry["beta"] is not None and newer(entry["beta"], beta):
            beta = entry["beta"]

    return {
        "schema": SCHEMA, "product": product.slug,
        "series": series,
        "channels": {"stable": stable,
                     "beta": beta["version"] if beta else None,
                     "nightly": nightly["version"] if nightly else None},
        "withdrawn": [{"version": w["version"], "reason": w["reason"]}
                      for w in product.policy.get("withdrawn", [])],
        "releases": releases,
        "nightly": nightly,
        "beta": beta,
    }


def published(update: Store) -> dict | None:
    """The index in the update store as it stands, unverified: for showing,
    never for deciding (trusted_published)."""
    if not update.exists(INDEX):
        return None
    return load_json(update.read(INDEX), INDEX)


def _published_pair(product: Product, update: Store, keys: list[str]
                    ) -> tuple[dict, bytes, bytes] | None:
    """The published index, its bytes and its signature's, when a pinned
    index key signed exactly them for this product and the serial it
    states; None when there is none, or it is not one to believe (said on
    stderr). A store that does not answer is StoreUnavailable, as ever:
    that is not knowing, which is not the same."""
    if not update.exists(INDEX):
        return None
    try:
        data, sig = update.read(INDEX), update.read(INDEX + MINISIG)
        with tempfile.TemporaryDirectory(prefix="gfr-published-") as tmp:
            local = Path(tmp) / "index.json"
            local.write_bytes(data)
            (Path(tmp) / "index.json.minisig").write_bytes(sig)
            comment = minisig.verify(keys, local,
                                     Path(tmp) / "index.json.minisig")
        index = load_json(data, INDEX)
        serial = index.get("serial")
        if not isinstance(serial, int) or isinstance(serial, bool) or \
                serial < 1:
            raise ReleaseError(f"{INDEX}: no serial")
        if comment != layout.index_comment(product.slug, serial):
            raise ReleaseError(f"{INDEX}: signed as {comment!r}")
        parse_iso(index.get("expires"))
    except StoreUnavailable:
        raise
    except ReleaseError as e:
        print(f"the published index is not one this host signed, and is "
              f"replaced: {e}", file=sys.stderr)
        return None
    return index, data, sig


def trusted_published(product: Product, update: Store,
                      index_pub: list[str]) -> dict | None:
    """The published index when it verifies with one of @p index_pub, the
    pinned index keys; None otherwise."""
    pair = _published_pair(product, update, index_pub)
    return pair[0] if pair else None


def read_serial(state_dir: Path) -> int:
    path = state_dir / "serial"
    if not path.exists():
        return 0
    try:
        return int(path.read_text())
    except ValueError as e:
        raise ReleaseError(f"{path}: not a serial") from e


def dry_run(product: Product, builds: Store, update: Store, *,
            state_dir: Path, git_dir: Path | None = None,
            index_pub: list[str], cosign: str = "cosign",
            authenticode_ca: str | None = None) -> dict:
    """Everything an index run decides, and nothing it writes: admission
    runs against a copy of the state directory, nothing is signed or
    uploaded. Returns what would be listed, what was refused, and
    how it differs from the published index."""
    refused = Problems()
    listed = trusted_published(product, update, index_pub)
    with tempfile.TemporaryDirectory(prefix="gfr-dry-") as tmp:
        state = Path(tmp) / "state"
        if Path(state_dir).is_dir():
            # Copied while no run is writing it: a record half replaced
            # would read as a release never admitted.
            with locked(state_dir, shared=True):
                shutil.copytree(state_dir, state)
        # Admission runs as for real, against the copy; nothing is written.
        content = build(product, builds, git_dir=git_dir,
                        state_dir=state, cosign=cosign,
                        authenticode_ca=authenticode_ca, refused=refused,
                        previous=listed, dry=True)
    previous = listed or {}
    before = {k: v for k, v in previous.items() if k not in VOLATILE}
    return {
        "channels": content["channels"],
        "releases": sorted(content["releases"], key=parse_version),
        "nightly": (content["nightly"] or {}).get("version"),
        "beta": (content["beta"] or {}).get("version"),
        "betas": {key: entry["beta"]["version"]
                  for key, entry in content["series"].items()
                  if entry["beta"]},
        "refused": list(refused),
        "changes": sorted(k for k in set(before) | set(content)
                          if before.get(k) != content.get(k)),
        "added": sorted(set(content["releases"]) -
                        set(before.get("releases", {}))),
        "removed": sorted(set(before.get("releases", {})) -
                          set(content["releases"])),
    }


def publish(product: Product, builds: Store, update: Store, *,
            index_key: Path, index_pub: list[str],
            state_dir: Path, force: bool = False, mirrors: list[Store] = (),
            git_dir: Path | None = None, cosign: str = "cosign",
            authenticode_ca: str | None = None) -> dict | None:
    """Builds, signs and publishes the index; None when it would say nothing
    new, is not near expiry and is whole where it is published. Raises
    IndexIncomplete, after publishing, when any release was refused or a
    mirror could not be written. @p index_pub: the index keys clients pin.
    Only a published index one of them signed is believed, and nothing is
    published that none of them verifies."""
    keys = list(index_pub)
    refused = Problems()
    before = _published_pair(product, update, keys)
    content = build(product, builds, git_dir=git_dir,
                    state_dir=state_dir, cosign=cosign,
                    authenticode_ca=authenticode_ca, refused=refused,
                    previous=before[0] if before else None)
    index = _sign_and_publish(product, content, update, index_key=index_key,
                              state_dir=state_dir, force=force,
                              mirrors=list(mirrors), keys=keys,
                              problems=refused)
    if refused:
        raise IndexIncomplete(refused, index)
    return index


def _sync_mirrors(data: bytes, sig: bytes, mirrors: list[Store],
                  problems: Problems) -> None:
    """Each mirror holds the update store's index, byte for byte."""
    for mirror in mirrors:
        try:
            if mirror.exists(INDEX) and mirror.read(INDEX) == data and \
                    mirror.exists(INDEX + MINISIG) and \
                    mirror.read(INDEX + MINISIG) == sig:
                continue
            # As _sign_and_publish: the signature first.
            mirror.put_bytes(sig, INDEX + MINISIG, immutable=False)
            mirror.put_bytes(data, INDEX, immutable=False)
        except ReleaseError as e:
            problems.note(f"mirror {mirror}", e)


def _sign_and_publish(product: Product, content: dict, update: Store, *,
                      index_key: Path, state_dir: Path, force: bool,
                      mirrors: list[Store], keys: list[str],
                      problems: Problems) -> dict | None:
    # Read again, after admission (which can take long): what is there now.
    # Whole and verified, or it does not count: an index whose signature is
    # missing (a run that died between the two uploads) or does not verify
    # is replaced, and nothing it says is believed.
    before = _published_pair(product, update, keys)
    previous = before[0] if before else None
    now = utcnow()
    if previous is not None and not force:
        same = {k: v for k, v in previous.items() if k not in VOLATILE} == content
        if same and parse_iso(previous["expires"]) - now > RESIGN_BEFORE:
            _sync_mirrors(before[1], before[2], mirrors, problems)
            return None

    state_dir.mkdir(parents=True, exist_ok=True)
    recorded = read_serial(state_dir)
    if previous is None and recorded == 0 and update.exists(INDEX):
        # Neither the host's record nor a published index it can believe:
        # any serial it picked could be lower than one clients have seen,
        # and every one of them would refuse the new index as a replay.
        raise ReleaseError(
            f"{update}/{INDEX} does not verify and {state_dir / 'serial'} is "
            "gone: the next serial is not known. Write the last published "
            "serial into that file (v1/history/ holds every one) and run "
            "again")
    serial = max(recorded, previous["serial"] if previous else 0) + 1
    # A serial whose history is already there was used by a run this host no
    # longer remembers (its state was lost after a publish that never went
    # live): never signed twice, so the next free one it is.
    for _ in range(SERIAL_SKIPS):
        if not update.exists(layout.history_path(serial)):
            break
        serial += 1
    else:
        raise ReleaseError(f"{update}: v1/history/ holds every serial up to "
                           f"{serial - 1}, far beyond the published one; "
                           "something else writes there")
    index = dict(content, serial=serial, generated_at=iso(now),
                 expires=iso(now + LIFETIME))

    with tempfile.TemporaryDirectory(prefix="gfr-sign-") as tmp:
        local = Path(tmp) / "index.json"
        local.write_bytes(canonical_json(index))
        sig = minisig.sign(index_key, local,
                           layout.index_comment(product.slug, serial))
        # What clients will check, checked before it leaves: an index key
        # that is not one of the pinned ones publishes nothing.
        minisig.verify(keys, local, sig)
        # Recorded before the upload: a crash after it can only skip a
        # serial, never reuse one.
        atomic_write(state_dir / "serial", f"{serial}\n")
        history = layout.history_path(serial)

        def keep(store: Store) -> None:
            # Every index ever published, kept: what was listed, and when a
            # release's signatures changed, can always be shown.
            store.put_file(local, history)
            store.put_file(sig, history + MINISIG)

        def go_live(store: Store) -> None:
            store.put_file(sig, INDEX + MINISIG, immutable=False)
            store.put_file(local, INDEX, immutable=False)

        # History first: no index is ever live without it, and one that
        # then fails to go live is only a serial skipped.
        keep(update)
        go_live(update)
        for mirror in mirrors:
            # A mirror serves clients: the index goes live there whatever
            # becomes of its copy of the history.
            for step in (go_live, keep):
                try:
                    step(mirror)
                except ReleaseError as e:
                    problems.note(f"mirror {mirror}", e)
    return index
