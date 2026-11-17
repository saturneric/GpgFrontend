# Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""The signing host's promotion of branch builds: staging -> final location.

main's builds are the nightly channel, a release/X.Y branch's the beta of
its series, and a release candidate's tag (vX.Y.0-rc.N, staged beside that
branch's builds) the prerelease of a series not released yet; no other ref
is promoted. For each branch, only its newest
staged build is (a burst of pushes costs one promotion; the signing host
deletes older candidates). Before anything is signed:

  1. its Sigstore bundle must name this product's build workflow at exactly
     the ref and commit staged.json claims (app/candidate.py);
  2. every file must match the signed SHA256SUMS.txt, and nothing else may be
     present;
  3. the version must be what git says that commit is (cmake/GitVersion.cmake
     against the host's own mirror), not merely what the build claimed.

Then everything is copied to nightly/<version>/ (immutable: a rerun skips
what is there, a different object is an error), CI's manifest and Sigstore
bundle under build/, the descriptor last, and the commit marker after it.
Nothing is signed here: the index names the descriptor by its hash, and
index.admit_nightly checks what promotion checked again before it does.
Release tags are never promoted here: a stable release passes through the
offline signing stage.

What that stage, and `import`, hand over lies in the staging store too,
under signed/, laid out as it is to be in the builds store (receive()). The
staging store is anyone's to write who has a token for it, CI among them:
so a release is received only when the index would admit it as it lies
there, every signature checked (index.admit_stable, admit_imported), and
exactly the bytes that were checked are copied. Notes handed over for a
release already there are received when a pinned maintainer signed them.

A release already there is received again when what is handed over for it
passes the same admission: a file added (an MSIX made from the MSI
afterwards), or one signed anew. Only what changed need be handed over,
with the manifest and the descriptor; the rest is taken from the builds
store (_complete). Admission is what keeps an older state of the release
from coming back that way: no maintainer signature may be older than the
one admitted before for its file (index._check_signature_times).
"""

from __future__ import annotations

import shutil
import tempfile
from pathlib import Path

from ..domain import layout
from ..adapters import openpgp
from ..cli import ui
from ..common import (BUNDLE, DESCRIPTOR, SCHEMA, SIG, STAGED, SUMS, Problems,
                    Product, ReleaseError, ToolFailed, canonical_json,
                    check_git_version, check_candidate, check_relpath, iso,
                    load_json,
                    parse_version, refs, sha256_file, utcnow,
                    verify_candidate)
from .stage import read_staged
from ..adapters.store import NotFound, Store

from . import index as index_mod



class PromoteIncomplete(ReleaseError):
    """Some candidates could not be promoted; the others were. Each failure
    is named, and the run still fails."""

    what = "promoted"

    def __init__(self, failed: Problems, done: list[dict]):
        super().__init__(f"not {self.what}:\n  " + "\n  ".join(failed))
        self.failed = failed
        self.passing = failed.passing  # common.Problems
        self.done = done


def candidates(staging: Store, failed: Problems | None = None
               ) -> list[tuple[str, dict]]:
    """(prefix, staged.json) of every complete branch candidate. One whose
    staged.json cannot be read is appended to @p failed (or raised), and
    blocks no other."""
    out = []
    for path in staging.files(layout.BRANCHES, include=f"**/{STAGED}"):
        prefix = f"{layout.BRANCHES}/{path.rsplit('/', 1)[0]}"
        try:
            out.append((prefix,
                        read_staged(staging.read(f"{prefix}/{STAGED}"))))
        except ReleaseError as e:
            if failed is None:
                raise
            failed.note(prefix, e)
    return out


def by_ref_newest_first(found: list[tuple[str, dict]]
                        ) -> list[list[tuple[str, dict]]]:
    """The candidates of each ref, newest first. staged.json is anyone's to
    write: the newest one that is genuine is promoted, and a forged one
    claiming a higher version only costs a failed attempt."""
    refs: dict[str, list[tuple[str, dict]]] = {}
    for prefix, staged in found:
        refs.setdefault(staged["ref"], []).append((prefix, staged))
    return [sorted(refs[ref], key=lambda c: parse_version(c[1]["version"]),
                   reverse=True) for ref in sorted(refs)]


def final_prefix(staged: dict) -> str:
    """Where a candidate is published: named after its version, never after
    the file-safe spelling CI claims for it."""
    return layout.nightly_dir(staged["version"])


def _finish_marker(product: Product, builds: Store, dest: str, staged: dict,
                   *, git_dir: Path, cosign: str) -> None:
    """Promoted already. A run that stopped between the descriptor and the
    commit marker left only the marker to write: for the commit the
    published descriptor names, once it is admitted, never for what a
    staged.json claims."""
    marker = layout.commit_marker(staged["commit"])
    if builds.exists(marker):
        return
    path = layout.descriptor_of(dest)
    with tempfile.TemporaryDirectory(prefix="gfr-marker-") as tmp:
        d = index_mod.admit_nightly(product, builds, path, builds.read(path),
                                    Path(tmp), git_dir=git_dir, cosign=cosign)
    if d["commit"] != staged["commit"] or d["ref"] != staged["ref"]:
        raise ReleaseError(f"{dest} is {d['commit']} on {d['ref']}, not "
                           "what this candidate claims")
    builds.put_bytes(b"", marker)


def promote_one(product: Product, staging: Store, builds: Store,
                prefix: str, staged: dict, *, git_dir: Path,
                cosign: str) -> dict:
    ref, commit = staged["ref"], staged["commit"]
    if staged["product"] != product.slug:
        raise ReleaseError(f"{prefix} is staged for {staged['product']}")
    # staged.json is anyone's to write: only builds of main, of release
    # branches and of release candidate tags are published, whatever else is
    # staged.
    if not refs.promotes(ref):
        raise ReleaseError(f"{prefix}: {ref} is neither main, a release "
                           "branch nor a release candidate's tag")
    rc_series = refs.rc_series(ref)
    # A tag's version is what the tag says, wherever it is: GitVersion reads
    # it from the tag, and the branch it would otherwise use is its series'.
    branch = refs.branch_of(ref) or refs.release_branch(rc_series)
    # A final version is the stable release's, published only through the
    # offline stage: a branch build of the tagged commit is never one.
    if parse_version(staged["version"]).is_final:
        raise ReleaseError(f"{prefix}: {staged['version']} is a final "
                           "version, which a nightly never is")
    dest = final_prefix(staged)
    filesafe = dest.rsplit("/", 1)[1]
    if staged["filesafe"] != filesafe:
        raise ReleaseError(f"{prefix}: staged as {staged['filesafe']}, which "
                           f"is not the file-safe {staged['version']}")

    with tempfile.TemporaryDirectory(prefix="gfr-promote-") as tmp:
        work = Path(tmp) / "candidate"
        staging.download(prefix, work)
        # Gone already when the candidate expired in between: the manifest
        # check below refuses what is left of it.
        (work / STAGED).unlink(missing_ok=True)

        identity = product.identity(ref)
        files = verify_candidate(prefix, work, staged, identity, cosign)

        derived = check_git_version(git_dir, commit, ref, branch,
                                    staged["version"], prefix)
        series = refs.branch_series(branch)
        if series and parse_version(derived).series != series:
            # The branch reached a later series' base tag: not its beta.
            raise ReleaseError(f"{prefix}: {branch} builds {derived}, "
                               f"outside its series {series}")
        if rc_series:
            check_candidate(git_dir, ref, commit, derived)

        artifacts = []
        for name in sorted(files):
            path = work / name
            digest = sha256_file(path)
            entry = product.classify(name, filesafe)
            entry.update({
                "name": name, "size": path.stat().st_size, "sha256": digest,
                "build_sha256": digest, "transform": "none",
                "path": f"{dest}/{name}",
            })
            artifacts.append(entry)

        build = Path(tmp) / "build"
        build.mkdir()
        shutil.copy2(work / SUMS, build / SUMS)
        shutil.copy2(work / BUNDLE, build / BUNDLE)

        descriptor = {
            "schema": SCHEMA, "product": product.slug,
            "kind": layout.KIND_NIGHTLY,
            "version": derived, "series": parse_version(derived).series,
            "ref": ref, "commit": commit, "built_at": staged["built_at"],
            "promoted_at": iso(utcnow()),
            "signing": {"scheme": layout.SCHEME_SIGSTORE},
            "build": {"sums": layout.build_sums(dest),
                      "bundle": layout.build_bundle(dest),
                      "identity": identity},
            "artifacts": artifacts,
        }

        for entry in artifacts:
            builds.put_file(work / entry["name"], entry["path"])
        builds.put_file(build / SUMS, descriptor["build"]["sums"])
        builds.put_file(build / BUNDLE, descriptor["build"]["bundle"])
        # Last: a descriptor only ever names objects that are already there.
        builds.put_bytes(canonical_json(descriptor),
                         layout.descriptor_of(dest))
        builds.put_bytes(b"", layout.commit_marker(commit))
    return descriptor


def promote(product: Product, staging: Store, builds: Store, *,
            git_dir: Path, cosign: str = "cosign") -> list[dict]:
    done: list[dict] = []
    failed = Problems()
    for tried in by_ref_newest_first(candidates(staging, failed)):
        for prefix, staged in tried:
            # The build of a commit a release tag names (main or release/X.Y
            # pushed at vX.Y.Z): the stable release, which only the offline
            # stage publishes. Not a nightly, and not a failure either.
            if parse_version(staged["version"]).is_final:
                ui.warn(f"{prefix}: {staged['version']} is a final "
                        "version, published as the stable release, never as "
                        "a nightly")
                continue
            try:
                dest = final_prefix(staged)
                if builds.exists(layout.descriptor_of(dest)):
                    _finish_marker(product, builds, dest, staged,
                                   git_dir=git_dir, cosign=cosign)
                    break
                done.append(promote_one(
                    product, staging, builds, prefix, staged,
                    git_dir=git_dir, cosign=cosign))
                break
            except ReleaseError as e:
                # The next older candidate of this ref gets its turn.
                failed.note(prefix, e)
            except OSError as e:
                # A local file that could not be read or written: no verdict
                # on the candidate, and no reason to stop the others.
                failed.note(prefix, ToolFailed(f"local file: {e}"))
    if failed:
        raise PromoteIncomplete(failed, done)
    return done


# --- what a maintainer hands over ------------------------------------------

def _fetch_staged(staging: Store, version: str, local: Path) -> None:
    """Every file the stage left out of what it handed over in @p local
    (its .sig is there, the file is not: signing left it as CI built it),
    from the tag's build still staged. Taken as any handed-over file: what
    admission then checks is what lies on this disk."""
    for sig in sorted(local.glob(f"*{SIG}")):
        name = sig.name[:-len(SIG)]
        if name == SUMS or (local / name).exists():
            continue
        try:
            staging.fetch_to(f"{layout.staged_tag(version)}/{name}",
                             local / name)
        except NotFound as e:
            raise ReleaseError(f"{name} was left to be taken from the "
                               "tag's staged build, which no longer holds "
                               "it") from e


def _receive_release(product: Product, staging: Store, builds: Store,
                     path: str, *, git_dir: Path, state_dir: Path,
                     cosign: str, authenticode_ca: str | None) -> dict | None:
    """The release whose descriptor is handed over at @p path (under
    signed/ of @p staging), copied to the builds store once it is checked
    (of one published already, what was handed over of it); None when it
    is there already."""
    outbox = staging.sub(layout.SIGNED)
    version = path.split("/")[1]
    if path != layout.descriptor_of(layout.release_dir(version)) or \
            not parse_version(version, path).is_release:
        raise ReleaseError("not where a release is handed over")
    published = None
    if builds.exists(path):
        published = builds.read(path)
        if published == outbox.read(path):
            return None
    with tempfile.TemporaryDirectory(prefix="gfr-receive-") as tmp:
        root = Path(tmp) / "handed"
        outbox.download(layout.release_dir(version),
                        root / layout.RELEASES / version)
        raw = (root / path).read_bytes()
        kept: set[str] = set()
        if published is not None:
            # Before anything is looked for in the tag's staged build,
            # which need not be there any more: what is published is.
            kept = _complete(builds, root, version, load_json(raw, path))
        _fetch_staged(staging, version, root / layout.RELEASES / version)
        try:
            outbox.download(f"{layout.NOTES}/{version}",
                            root / layout.NOTES / version)
        except NotFound:
            pass  # none handed over: admission says whether it needs any
        # Checked as the index will check it, where it lies now: on this
        # disk, so that what is copied is what was checked.
        local = Store(str(root), builds.rclone)
        kind = load_json(raw, path).get("kind")
        try:
            if kind == layout.KIND_STABLE:
                d, entry = index_mod.admit_stable(
                    product, local, path, raw, git_dir=git_dir,
                    state_dir=state_dir, cosign=cosign,
                    tmp=Path(tmp) / "check",
                    authenticode_ca=authenticode_ca, dry=True)
            elif kind == layout.KIND_IMPORTED:
                d, entry = index_mod.admit_imported(
                    product, local, path, raw, git_dir=git_dir,
                    state_dir=state_dir, tmp=Path(tmp) / "check", dry=True)
            else:
                raise ReleaseError("neither a stable nor an imported "
                                   "release")
        except index_mod.KeptListed as e:
            d, entry = e.d, e.entry
        names = [a[k] for a in d["artifacts"] for k in ("path", "sig")]
        if kind == layout.KIND_STABLE:
            names += [d["sums"], d["sums_sig"], d["build"]["sums"],
                      d["build"]["bundle"]]
        for name in names:
            # Of a release published before, what was taken from the
            # builds store is there, and what was handed over takes the
            # place of what it had.
            if name not in kept:
                builds.put_file(root / name, name,
                                immutable=published is None)
        # Content-addressed, so never in the place of others.
        for name in [entry[k] for k in ("notes", "notes_sig") if k in entry]:
            builds.put_file(root / name, name)
        # Last: a descriptor only ever names objects that are already
        # there, and until it is, a release published before is the one it
        # was.
        builds.put_file(root / path, path, immutable=published is None)
    return d


def _complete(builds: Store, root: Path, version: str,
              handed: dict) -> set[str]:
    """For a release that is published and handed over again: whatever the
    descriptor handed over names and did not come with it, fetched into
    @p root from the builds store, so that only what changed need be
    handed over and the whole is still checked as the index will check
    it. Returns the names fetched. The descriptor is not believed yet: a
    name it gives is only ever looked for."""
    listed = handed.get("artifacts")
    names = [a.get(k) for a in (listed if isinstance(listed, list) else [])
             if isinstance(a, dict) for k in ("path", "sig")]
    build = handed.get("build")
    if isinstance(build, dict):
        names += [build.get(k) for k in ("sums", "bundle")]
    fetched = set()
    for name in names:
        if not isinstance(name, str) or (root / check_relpath(name)).exists():
            continue
        try:
            builds.fetch_to(name, root / name)
            fetched.add(name)
        except NotFound:
            pass  # admission refuses a release with a file missing
    try:
        # Content-addressed: beside whatever notes were handed over.
        builds.download(f"{layout.NOTES}/{version}",
                        root / layout.NOTES / version)
    except NotFound:
        pass  # admission says whether it needs any
    return fetched


def _receive_notes(product: Product, outbox: Store, builds: Store,
                   path: str) -> bool:
    """Notes handed over at @p path for a release the builds store has,
    copied there once a pinned maintainer's signature on them verifies;
    False when there was nothing to do."""
    parts = path.split("/")
    if len(parts) != 3 or path != layout.notes_path(parts[1], parts[2][:-3]):
        raise ReleaseError("not where notes are handed over")
    version, digest = parts[1], parts[2][:-3]
    if builds.exists(path) or not builds.exists(
            layout.descriptor_of(layout.release_dir(version))):
        # Published already, or of a release still to be received (or
        # refused): its notes come with it.
        return False
    with tempfile.TemporaryDirectory(prefix="gfr-receive-") as tmp:
        notes = outbox.fetch_to(path, Path(tmp) / "notes.md")
        sig = outbox.fetch_to(path + SIG, Path(tmp) / ("notes.md" + SIG))
        if sha256_file(notes) != digest:
            raise ReleaseError("not the notes its name says")
        with openpgp.Keyring.of(product) as ring:
            ring.verify(notes, sig)
        # The signature first: notes that are there count as received.
        builds.put_file(sig, path + SIG)
        builds.put_file(notes, path)
    return True


class ReceiveIncomplete(PromoteIncomplete):
    """Something handed over was not received; the rest was."""

    what = "received"


def receive(product: Product, staging: Store, builds: Store, *,
            git_dir: Path, state_dir: Path, cosign: str = "cosign",
            authenticode_ca: str | None = None) -> list[dict]:
    """Everything a maintainer handed over (signed/ in the staging store)
    that is not in the builds store yet, checked and copied there; returns
    the descriptors of the releases received."""
    outbox = staging.sub(layout.SIGNED)
    done: list[dict] = []
    failed = Problems()

    def each(prefix: str, name: str, one) -> None:
        for listed in outbox.files(prefix, include=f"*/{name}"):
            path = f"{prefix}/{listed}"
            try:
                one(path)
            except ReleaseError as e:
                failed.note(f"{layout.SIGNED}/{path}", e)
            except OSError as e:
                failed.note(f"{layout.SIGNED}/{path}",
                            ToolFailed(f"local file: {e}"))

    def release(path: str) -> None:
        d = _receive_release(product, staging, builds, path, git_dir=git_dir,
                             state_dir=state_dir, cosign=cosign,
                             authenticode_ca=authenticode_ca)
        if d is not None:
            done.append(d)

    each(layout.RELEASES, DESCRIPTOR, release)
    each(layout.NOTES, "*.md",
         lambda path: _receive_notes(product, outbox, builds, path))
    if failed:
        raise ReceiveIncomplete(failed, done)
    return done
