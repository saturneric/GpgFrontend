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
import os
import re
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
                    tag_time, tagged_commit, utcnow)
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
RELEASE_BRANCH = re.compile(r"refs/heads/release/[0-9]+\.[0-9]+")


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


_STRINGS = ("product", "kind", "version", "ref", "commit", "sums", "sums_sig",
            "promoted_at", "built_at")
_ARTIFACT_STRINGS = ("name", "path", "sig", "sha256", "build_sha256",
                     "transform", "os", "arch", "kind", "flavor")
_COMMIT = re.compile(r"[0-9a-f]{40}")


def _str_map(value) -> bool:
    return isinstance(value, dict) and \
        all(isinstance(v, str) for v in value.values())


def _check_shape(d: dict, path: str) -> None:
    """Every field admission reads has the type it reads it as. A
    descriptor comes from an untrusted bucket: one of the wrong shape is
    refused like any other bad one, never a traceback that stops the run."""
    def wrong(what: str):
        raise ReleaseError(f"{path}: {what} has the wrong type")
    for k in _STRINGS:
        if k in d and not isinstance(d[k], str):
            wrong(k)
    if "commit" in d and not _COMMIT.fullmatch(d["commit"]):
        raise ReleaseError(f"{path}: not a commit: {d['commit']!r}")
    for k in ("signing", "build"):
        if k in d and not _str_map(d[k]):
            wrong(k)
    if "artifacts" not in d:
        return
    if not isinstance(d["artifacts"], list):
        wrong("artifacts")
    for a in d["artifacts"]:
        if not isinstance(a, dict):
            wrong("an artifact entry")
        for k in _ARTIFACT_STRINGS:
            if k in a and not isinstance(a[k], str):
                wrong(f"an artifact's {k}")
        if "min_os" in a and not _str_map(a["min_os"]):
            wrong("an artifact's min_os")
        if "size" in a and (not isinstance(a["size"], int) or
                            isinstance(a["size"], bool)):
            wrong("an artifact's size")


def _require(d: dict, path: str, *fields: str) -> None:
    missing = [f for f in fields if f not in d]
    if missing:
        raise ReleaseError(f"{path}: lacks {', '.join(missing)}")
    for a in d.get("artifacts", []):
        if not isinstance(a, dict) or any(
                k not in a for k in ("name", "path", "sig", "sha256")):
            raise ReleaseError(f"{path}: an artifact entry is incomplete")
    # Every check below compares {name: sha256}: a name listed twice would
    # collapse there, and its other entry (hash, path) reach the index
    # unchecked.
    for field in ("name", "path"):
        values = [a[field] for a in d.get("artifacts", [])]
        if len(values) != len(set(values)):
            raise ReleaseError(f"{path}: an artifact {field} is listed twice")


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
                  artifact_pub: str, tmp: Path) -> dict:
    """A nightly descriptor is listed only if it is this product's, says what
    it is consistently, and its manifest carries the artifact key's signature
    over exactly the hashes it states."""
    d = load_json(raw, path)
    _check_shape(d, path)
    prefix = path.rsplit("/", 1)[0]
    if d.get("schema") != SCHEMA or d.get("product") != product.slug:
        raise ReleaseError(f"{path}: not a {product.slug} descriptor")
    if d.get("kind") != "nightly" or not d.get("ref", "").startswith("refs/heads/"):
        raise ReleaseError(f"{path}: not a nightly")
    _require(d, path, "version", "sums", "sums_sig", "artifacts",
             "promoted_at")
    try:
        parse_iso(d["promoted_at"])
    except ReleaseError as e:
        raise ReleaseError(f"{path}: promoted_at: {e}") from e
    # Signed with the current artifact key, or one it replaced: a rotation
    # does not unlist the nightlies published before it.
    keys = {minisig.key_id(k): k for k in product.artifact_keys(artifact_pub)}
    signing = d.get("signing", {})
    key = keys.get(signing.get("key_id")) \
        if signing.get("scheme") == "minisign" and len(signing) == 2 else None
    if key is None:
        raise ReleaseError(f"{path}: not signed with the artifact key")
    v = parse_version(d['version'], path)
    if v.is_final:
        raise ReleaseError(f"{path}: {d['version']} is a final version, which "
                           "a nightly never is")
    filesafe = v.filesafe
    if prefix != f"nightly/{filesafe}":
        raise ReleaseError(f"{path}: published under another version's name")
    _check_contained(path, [d["sums"], d["sums_sig"]] +
                     [a[k] for a in d["artifacts"] for k in ("path", "sig")],
                     prefix)
    # The signature verify.py and every client check is the one beside the
    # manifest; the host checks that same object.
    if d["sums_sig"] != d["sums"] + ".minisig":
        raise ReleaseError(f"{path}: the manifest's signature is not beside "
                           "it")
    _check_classified(product, d, path, filesafe)

    sums, sig = tmp / SUMS, tmp / (SUMS + ".minisig")
    sums.write_bytes(builds.read(d["sums"]))
    sig.write_bytes(builds.read(d["sums_sig"]))
    comment = minisig.verify([key], sums, sig)
    # What the signature says it is: this product's manifest of this very
    # version, as promotion signs it. The same key countersigns releases'
    # manifests; one of those is never a nightly's.
    want = f"product={product.slug} file={SUMS} version={d['version']}"
    if comment != want:
        raise ReleaseError(f"{path}: the manifest's trusted comment is "
                           f"{comment!r}, not {want!r}")
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


RECORD_SCHEMA = 3


def _trust(product: Product, git_dir: Path, tag: str, kind: str) -> str:
    """A digest of everything an admission rested on besides the
    descriptor: the pinned keys (and the key file they come from), the
    Authenticode subjects, the commit the tag names and when the tag was
    made. When any of them changes, a release is checked again from
    nothing: a key taken out of the product unlists what only it signed, a
    moved or re-dated tag is caught."""
    tagged = run(["git", "-C", git_dir, "rev-parse", "-q", "--verify",
                  f"refs/tags/{tag}^{{commit}}"], check=False).stdout.strip()
    try:
        tagged_at = tag_time(git_dir, tag)
    except ReleaseError:
        tagged_at = None
    facts = {"kind": kind, "maintainers": sorted(product.trusted_maintainers),
             "keyring": sha256_file(product.keyring_path)
             if Path(product.keyring_path).is_file() else "",
             "tag": tagged, "tag_time": tagged_at}
    if kind == "stable":
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
                   countersigner: "countersign.Countersigner | None"
                   ) -> tuple[dict, dict]:
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

    Only then is it countersigned (countersign.py), and the record says so
    last: a run stopped halfway countersigns again (what is there is kept),
    and never lists a release whose countersignatures are not all there.
    Every change of its signatures is appended to <version>.history.jsonl.
    Without a @p countersigner (a dry run) nothing is signed or recorded."""
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
        signatures = check_release()
        _check_signature_times(signatures, stale, tag, tagged_at)
    else:
        signatures = record["signatures"]

    previous_notes = record.get("notes") if record else None
    if record is None and stale is not None and stale.get("notes"):
        # Admitted under other trust: kept only once they verify again.
        try:
            with openpgp.Keyring(product.keyring_path,
                                 product.trusted_maintainers) as ring:
                _verified_notes(builds, ring, stale["notes"]["notes"],
                                stale["notes"]["notes_sha256"],
                                tmp / version / "stale-notes")
            previous_notes = stale["notes"]
        except ReleaseError:
            previous_notes = None
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
        countersign.entry_fields(d, chosen) | _signature_fields(signatures)
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
    new_record = dict(base, schema=RECORD_SCHEMA, kind=kind, notes=chosen,
                      signatures=signatures,
                      countersigned={"key": countersigner.key_id,
                                     "notes": chosen["notes_sha256"]
                                     if chosen else None})
    previous = (stale or {}).get("signatures")
    if signatures != previous:
        # Before the record: a crash in between repeats a line, never
        # loses one.
        _append_history(state_dir / "admitted" / f"{version}.history.jsonl",
                        {"at": iso(utcnow()), "descriptor": path,
                         "descriptor_sha256": base["sha256"],
                         "signatures": signatures, "previous": previous})
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
    _check_shape(d, path)
    version = _check_release_envelope(product, d, path, "stable")
    _require(d, path, "commit", "sums", "sums_sig", "build", "artifacts")
    _require(d["build"], path + " build", "sums", "bundle")
    _check_contained(path, [d["sums"], d["sums_sig"], d["build"]["sums"],
                            d["build"]["bundle"]] +
                     [a[k] for a in d["artifacts"] for k in ("path", "sig")],
                     f"releases/{version}")
    _check_classified(product, d, path, version)
    return _admit_release(
        product, builds, path, raw, d, kind="stable", git_dir=git_dir,
        state_dir=state_dir, tmp=tmp, notes_required=True,
        countersigner=countersigner,
        check_release=lambda: _check_stable(
            product, builds, d, path, git_dir=git_dir, cosign=cosign,
            tmp=tmp, authenticode_ca=authenticode_ca))


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
        signatures[SUMS] = _signed(ring.verify_signature(
            sums, builds.fetch_to(d["sums_sig"], work / (SUMS + SIG))),
            sha256_file(sums))
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
                                authenticode_ca,
                                require_timestamp=product
                                .authenticode_timestamp_required)
        else:
            raise ReleaseError(f"{a['name']}: unknown transform "
                               f"{a.get('transform')!r}")
    return signatures


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
    _check_shape(d, path)
    version = _check_release_envelope(product, d, path, imported.KIND)
    floor = parse_version(product.policy["signing"]["legacy_unsigned_before"])
    if parse_version(version) < floor:
        raise ReleaseError(f"{path}: {version} is older than {floor}")
    if not d.get("artifacts"):
        raise ReleaseError(f"{path}: lists no files")
    _require(d, path, "artifacts", "commit")
    # Its manifest is the one the server writes from the checked hashes
    # (countersign.manifest_path); one named here would be countersigned
    # wherever it pointed.
    if "sums" in d or "sums_sig" in d:
        raise ReleaseError(f"{path}: an imported release names no manifest")
    _check_contained(path, [a[k] for a in d["artifacts"]
                            for k in ("path", "sig")], f"releases/{version}")
    _check_classified(product, d, path, version)
    return _admit_release(
        product, builds, path, raw, d, kind=imported.KIND, git_dir=git_dir,
        state_dir=state_dir, tmp=tmp, notes_required=False,
        countersigner=countersigner,
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
    with openpgp.Keyring(product.keyring_path,
                         product.trusted_maintainers) as ring:
        for a in d["artifacts"]:
            local = builds.fetch_to(a["path"], work / a["name"])
            sig = builds.fetch_to(a["sig"], work / (a["name"] + SIG))
            if sha256_file(local) != a["sha256"]:
                raise ReleaseError(f"{a['name']}: not the descriptor's bytes")
            signatures[a["name"]] = _signed(ring.verify_signature(local, sig),
                                            a["sha256"])
    return signatures


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
          countersigner: "countersign.Countersigner | None" = None,
          previous: dict | None = None) -> dict:
    """The index's content, without serial and times. A release that fails
    admission is left out and appended to @p refused (when given; otherwise
    its error is raised). One that could not be checked because the store
    did not answer (StoreUnavailable) keeps its entry from @p previous, the
    published index, and is named in @p refused all the same: an outage
    never takes a release off the channel, and never passes unnoticed."""
    def refuse(path: str, error: ReleaseError) -> None:
        if refused is None:
            raise error
        refused.append(f"{path}: {error}")

    withdrawn = {parse_version(w["version"])
                 for w in product.policy.get("withdrawn", [])}
    nightly = kept_nightly = None
    betas: list[dict] = []
    releases: dict[str, dict] = {}
    retention = product.data.get("nightly_retention_days")
    cutoff = utcnow() - dt.timedelta(days=retention) + NIGHTLY_MARGIN \
        if retention else None
    with tempfile.TemporaryDirectory(prefix="gfr-index-") as tmp:
        for rel in builds.files("nightly", include=f"*/{DESCRIPTOR}"):
            path = f"nightly/{rel}"
            try:
                raw = builds.read(path)
                # Soon deleted by the bucket's lifecycle rule: never named,
                # and not even checked (its files may be going already).
                promoted = load_json(raw, path).get("promoted_at")
                if cutoff is not None and isinstance(promoted, str):
                    try:
                        if parse_iso(promoted) < cutoff:
                            continue
                    except ReleaseError:
                        pass  # admit_nightly refuses it
                d = admit_nightly(product, builds, path, raw, artifact_pub,
                                  Path(tmp))
            except StoreUnavailable as e:
                refuse(path, e)
                if (previous or {}).get("nightly") and \
                        previous["nightly"].get("descriptor") == path:
                    kept_nightly = previous["nightly"]
                if (previous or {}).get("beta") and \
                        previous["beta"].get("descriptor") == path:
                    betas.append(previous["beta"])
                continue
            except ReleaseError as e:
                refuse(path, e)
                continue
            if parse_version(d["version"]) in withdrawn:
                continue
            listed = {"version": d["version"], "descriptor": path,
                      "sha256": sha256_bytes(raw)}
            # main's builds are the nightly channel; a release/X.Y branch's
            # are the beta of that series (which one is chosen below, once
            # the releases are known).
            if RELEASE_BRANCH.fullmatch(d["ref"]):
                betas.append(listed)
                continue
            if d["ref"] != "refs/heads/main":
                continue
            if nightly is None or parse_version(d["version"]) > \
                    parse_version(nightly["version"]):
                nightly = listed
        if kept_nightly is not None and (nightly is None or parse_version(
                kept_nightly["version"]) > parse_version(nightly["version"])):
            nightly = kept_nightly

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
            except StoreUnavailable as e:
                refuse(path, e)
                version = rel.split("/", 1)[0]
                listed = (previous or {}).get("releases", {})
                if version in listed and \
                        listed[version].get("descriptor") == path:
                    releases[version] = listed[version]
                continue
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
    # shipped and before the policy moved on). The nightly and beta channels
    # are builds, never releases.
    for version in sorted(releases, key=parse_version):
        if parse_version(version) in withdrawn:
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

    # The beta channel: the newest release-branch build of a series users
    # are meant to be on, and newer than that series' own latest release.
    # Signed as a nightly is (minisign, the artifact key); a user opts in.
    def offered(b: dict) -> bool:
        v = parse_version(b["version"])
        entry = series.get(v.series)
        return entry is not None and \
            entry["status"] in ("stable", "mainline") and \
            (entry["latest"] is None or v > parse_version(entry["latest"]))
    candidates = [b for b in betas if offered(b)]
    beta = max(candidates, key=lambda b: parse_version(b["version"])) \
        if candidates else None

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
    listed = published(update)
    with tempfile.TemporaryDirectory(prefix="gfr-dry-") as tmp:
        state = Path(tmp) / "state"
        if Path(state_dir).is_dir():
            shutil.copytree(state_dir, state)
        # Admission runs as for real, countersigning included, against the
        # copy: what would clash is refused here too, nothing is written.
        content = build(product, builds, artifact_pub, git_dir=git_dir,
                        state_dir=state, cosign=cosign,
                        authenticode_ca=authenticode_ca, refused=refused,
                        previous=listed,
                        countersigner=countersign.Countersigner(
                            product, None, artifact_pub, dry=True))
    previous = listed or {}
    before = {k: v for k, v in previous.items() if k not in VOLATILE}
    return {
        "channels": content["channels"],
        "releases": sorted(content["releases"], key=parse_version),
        "nightly": (content["nightly"] or {}).get("version"),
        "beta": (content["beta"] or {}).get("version"),
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
            authenticode_ca: str | None = None,
            index_pub: list[str] | None = None) -> dict | None:
    """Builds, signs and publishes the index; None when it would say nothing
    new, is not near expiry and is whole where it is published. Raises
    IndexIncomplete, after publishing, when any release was refused or a
    mirror could not be written. @p index_pub, the pinned index keys, lets
    a published index whose signature does not verify be repaired."""
    refused: list[str] = []
    content = build(product, builds, artifact_pub, git_dir=git_dir,
                    state_dir=state_dir, cosign=cosign,
                    authenticode_ca=authenticode_ca, refused=refused,
                    countersigner=countersign.Countersigner(
                        product, artifact_key, artifact_pub)
                    if artifact_key else None,
                    previous=published(update))
    if artifact_key is None and content["releases"]:
        # Nothing is listed that the server has not countersigned.
        raise ReleaseError("stable releases need --artifact-key to be "
                           "countersigned and listed")
    index = _sign_and_publish(product, content, update, index_key=index_key,
                              state_dir=state_dir, force=force,
                              mirrors=mirrors, index_pub=index_pub,
                              problems=refused)
    if refused:
        raise IndexIncomplete(refused, index)
    return index


def _intact(update: Store, index_pub: list[str] | None) -> bool:
    """The published index is whole: its signature is there and, when the
    index keys are known, verifies. Two uploads make it, and a run that died
    between them left it broken until something new was said."""
    if not update.exists(INDEX + ".minisig"):
        return False
    if not index_pub:
        return True
    with tempfile.TemporaryDirectory(prefix="gfr-intact-") as tmp:
        local, sig = Path(tmp) / "index.json", Path(tmp) / "index.json.minisig"
        local.write_bytes(update.read(INDEX))
        sig.write_bytes(update.read(INDEX + ".minisig"))
        try:
            minisig.verify(index_pub, local, sig)
        except ReleaseError:
            return False
    return True


def _sync_mirrors(update: Store, mirrors: list[Store],
                  problems: list[str]) -> None:
    """Each mirror holds the update store's index, byte for byte."""
    if not mirrors:
        return
    data, sig = update.read(INDEX), update.read(INDEX + ".minisig")
    for mirror in mirrors:
        try:
            if mirror.exists(INDEX) and mirror.read(INDEX) == data and \
                    mirror.exists(INDEX + ".minisig") and \
                    mirror.read(INDEX + ".minisig") == sig:
                continue
            mirror.put_bytes(data, INDEX, immutable=False)
            mirror.put_bytes(sig, INDEX + ".minisig", immutable=False)
        except ReleaseError as e:
            problems.append(f"mirror {mirror}: {e}")


def _sign_and_publish(product: Product, content: dict, update: Store, *,
                      index_key: Path, state_dir: Path, force: bool,
                      mirrors: list[Store], index_pub: list[str] | None = None,
                      problems: list[str] | None = None) -> dict | None:
    problems = problems if problems is not None else []
    previous = published(update)
    now = utcnow()
    if previous is not None and not force:
        same = {k: v for k, v in previous.items() if k not in VOLATILE} == content
        if same and parse_iso(previous["expires"]) - now > RESIGN_BEFORE and \
                _intact(update, index_pub):
            _sync_mirrors(update, mirrors, problems)
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
            try:
                store.put_file(local, INDEX, immutable=False)
                store.put_file(sig, INDEX + ".minisig", immutable=False)
                # Every index ever published, kept: what was listed, and
                # when a release's signatures changed, can always be shown.
                store.put_file(local, history_path(serial))
                store.put_file(sig, history_path(serial) + ".minisig")
            except ReleaseError as e:
                if store is update:
                    raise
                problems.append(f"mirror {store}: {e}")
    return index


def history_path(serial: int) -> str:
    return f"v1/history/{serial}.json"
