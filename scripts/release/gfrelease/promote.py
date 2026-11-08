# Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""The signing host's promotion of nightlies: staging -> final location.

For each branch, only its newest staged build is promoted (a burst of pushes
to main costs one promotion; older candidates expire in staging). Before
anything is signed:

  1. its Sigstore bundle must name this product's build workflow at exactly
     the ref and commit staged.json claims (scripts/release_candidate.py);
  2. every file must match the signed SHA256SUMS.txt, and nothing else may be
     present;
  3. the version must be what git says that commit is (cmake/GitVersion.cmake
     against the host's own mirror), not merely what the build claimed.

Then every file and the manifest are minisigned with the product's artifact
key, everything is copied to nightly/<version>/ (immutable: a rerun skips
what is there, a different object is an error), the descriptor is written
last, and the commit marker after it. Release tags are never promoted here:
a stable release passes through the offline signing stage.
"""

from __future__ import annotations

import shutil
import sys
import tempfile
from pathlib import Path

import minisig
from common import (BUNDLE, DESCRIPTOR, SCHEMA, STAGED, SUMS, Product,
                    ReleaseError, canonical_json, git_version, iso,
                    parse_version, run, sha256_file, utcnow)
from stage import read_staged
from store import Store

import index as index_mod

import release_candidate as rc  # noqa: E402  (scripts/, on the path via common)


class PromoteIncomplete(ReleaseError):
    """Some candidates could not be promoted; the others were. Each failure
    is named, and the run still fails."""

    def __init__(self, failed: list[str], done: list[dict]):
        super().__init__("not promoted:\n  " + "\n  ".join(failed))
        self.failed = failed
        self.done = done


def candidates(staging: Store, failed: list[str] | None = None
               ) -> list[tuple[str, dict]]:
    """(prefix, staged.json) of every complete nightly candidate. One whose
    staged.json cannot be read is appended to @p failed (or raised), and
    blocks no other."""
    out = []
    for path in staging.files("branches", include=f"**/{STAGED}"):
        prefix = f"branches/{path.rsplit('/', 1)[0]}"
        try:
            out.append((prefix,
                        read_staged(staging.read(f"{prefix}/{STAGED}"))))
        except ReleaseError as e:
            if failed is None:
                raise
            failed.append(f"{prefix}: {e}")
    return out


def newest_per_ref(found: list[tuple[str, dict]]) -> list[tuple[str, dict]]:
    best: dict[str, tuple[str, dict]] = {}
    for prefix, staged in found:
        current = best.get(staged["ref"])
        if current is None or parse_version(staged["version"]) > \
                parse_version(current[1]["version"]):
            best[staged["ref"]] = (prefix, staged)
    return [best[ref] for ref in sorted(best)]


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
    return f"nightly/{parse_version(staged['version']).filesafe}"


def _finish_marker(product: Product, builds: Store, dest: str, staged: dict,
                   artifact_pub: str) -> None:
    """Promoted already. A run that stopped between the descriptor and the
    commit marker left only the marker to write: for the commit the
    published descriptor names, once it is admitted, never for what a
    staged.json claims."""
    marker = f"commits/{staged['commit']}"
    if builds.exists(marker):
        return
    path = f"{dest}/{DESCRIPTOR}"
    with tempfile.TemporaryDirectory(prefix="gfr-marker-") as tmp:
        d = index_mod.admit_nightly(product, builds, path, builds.read(path),
                                    artifact_pub, Path(tmp))
    if d["commit"] != staged["commit"] or d["ref"] != staged["ref"]:
        raise ReleaseError(f"{dest} is {d['commit']} on {d['ref']}, not "
                           "what this candidate claims")
    builds.put_bytes(b"", marker)


def promote_one(product: Product, staging: Store, builds: Store,
                prefix: str, staged: dict, *, git_dir: Path,
                artifact_key: Path, artifact_pub: str, cosign: str) -> dict:
    ref, commit = staged["ref"], staged["commit"]
    if staged["product"] != product.slug:
        raise ReleaseError(f"{prefix} is staged for {staged['product']}")
    if not ref.startswith("refs/heads/"):
        raise ReleaseError(f"{prefix}: {ref} is not a nightly ref")
    branch = ref[len("refs/heads/"):]
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
        (work / STAGED).unlink()

        identity = product.identity(ref)
        try:
            verified = rc.verify(work, identity, commit, cosign)
        except rc.CandidateError as e:
            raise ReleaseError(f"{prefix}: {e}") from e
        files = [p.name for p in verified if p.name not in (SUMS, BUNDLE)]
        if sorted(files) != sorted(staged["files"]):
            raise ReleaseError(f"{prefix}: staged.json lists other files "
                               "than the signed manifest")

        derived = git_version(git_dir, commit, branch)
        if derived != staged["version"]:
            raise ReleaseError(f"{prefix}: git says {commit[:12]} on {branch} "
                               f"is {derived}, the build claims "
                               f"{staged['version']}")

        artifacts = []
        for name in sorted(files):
            path = work / name
            digest = sha256_file(path)
            sig = minisig.sign(artifact_key, path,
                               f"product={product.slug} file={name} "
                               f"version={derived}")
            minisig.verify([artifact_pub], path, sig)
            entry = product.classify(name, filesafe)
            entry.update({
                "name": name, "size": path.stat().st_size, "sha256": digest,
                "build_sha256": digest, "transform": "none",
                "path": f"{dest}/{name}", "sig": f"{dest}/{sig.name}",
            })
            artifacts.append(entry)
        sums_sig = minisig.sign(artifact_key, work / SUMS,
                                f"product={product.slug} file={SUMS} "
                                f"version={derived}")

        build = Path(tmp) / "build"
        build.mkdir()
        shutil.copy2(work / SUMS, build / SUMS)
        shutil.copy2(work / BUNDLE, build / BUNDLE)

        descriptor = {
            "schema": SCHEMA, "product": product.slug, "kind": "nightly",
            "version": derived, "series": parse_version(derived).series,
            "ref": ref, "commit": commit, "built_at": staged["built_at"],
            "promoted_at": iso(utcnow()),
            "signing": {"scheme": "minisign",
                        "key_id": minisig.key_id(artifact_pub)},
            "build": {"sums": f"{dest}/build/{SUMS}",
                      "bundle": f"{dest}/build/{BUNDLE}",
                      "identity": identity},
            "sums": f"{dest}/{SUMS}", "sums_sig": f"{dest}/{sums_sig.name}",
            "artifacts": artifacts,
        }

        for entry in artifacts:
            builds.put_file(work / entry["name"], entry["path"])
            builds.put_file(work / Path(entry["sig"]).name, entry["sig"])
        builds.put_file(work / SUMS, descriptor["sums"])
        builds.put_file(sums_sig, descriptor["sums_sig"])
        builds.put_file(build / SUMS, descriptor["build"]["sums"])
        builds.put_file(build / BUNDLE, descriptor["build"]["bundle"])
        # Last: a descriptor only ever names objects that are already there.
        builds.put_bytes(canonical_json(descriptor), f"{dest}/{DESCRIPTOR}")
        builds.put_bytes(b"", f"commits/{commit}")
    return descriptor


def promote(product: Product, staging: Store, builds: Store, *,
            git_dir: Path, artifact_key: Path, artifact_pub: str,
            cosign: str = "cosign", fetch: bool = False) -> list[dict]:
    if fetch:
        run(["git", "-C", git_dir, "fetch", "--quiet", "--prune", "--tags",
             "origin"])
    done: list[dict] = []
    failed: list[str] = []
    for tried in by_ref_newest_first(candidates(staging, failed)):
        for prefix, staged in tried:
            # The build of a commit a release tag names (main or release/X.Y
            # pushed at vX.Y.Z): the stable release, which only the offline
            # stage publishes. Not a nightly, and not a failure either.
            if parse_version(staged["version"]).is_final:
                print(f"{prefix}: {staged['version']} is a final version, "
                      "published as the stable release, never as a nightly",
                      file=sys.stderr)
                continue
            try:
                dest = final_prefix(staged)
                if builds.exists(f"{dest}/{DESCRIPTOR}"):
                    _finish_marker(product, builds, dest, staged,
                                   artifact_pub)
                    break
                done.append(promote_one(
                    product, staging, builds, prefix, staged,
                    git_dir=git_dir, artifact_key=artifact_key,
                    artifact_pub=artifact_pub, cosign=cosign))
                break
            except ReleaseError as e:
                # The next older candidate of this ref gets its turn.
                failed.append(f"{prefix}: {e}")
    if failed:
        raise PromoteIncomplete(failed, done)
    return done
