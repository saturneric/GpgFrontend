# Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""Build and verify the nightly release candidate.

The release is one invariant:

  final verified artifacts -> signed canonical manifest
                           -> immutable release candidate
                           -> verify-only publication

`stage` runs in the provenance job. It takes the final artifacts of every leg,
checks each against the build info that describes it, and writes a candidate
directory:

  <deliverables>        every file of every `<slug>-<leg>` artifact
  SHA256SUMS.txt        the canonical release set: every file above, hashed

The provenance job then signs SHA256SUMS.txt, once, and uploads the directory
as the `release-candidate` artifact. Nothing writes to it after that.

verify(), called by gfrelease on the signing host (promote) and in the
maintainer's stable stage, over the staged candidate, is the whole
publication gate: the Sigstore
identity, then the manifest, then exact set equality between the manifest
and the directory, then every hash. It returns the verified file list, so
what is published is what was verified and not whatever a glob finds.

## One authority per fact

  the bytes          SHA256SUMS.txt, and only there
  the pairing        the artifact name: `<slug>-<leg>` and
                     `buildinfo-<leg>` share <leg> exactly

Which leg made which file is never published: the release carries the
downloads, their manifest and its signature, and nothing about the builds.

Nothing here knows a platform, an architecture or a package format. A new
build leg only has to upload those two artifacts.

`build-info.json` is an internal CI interface. It is read here and never
published; the name is reserved so it cannot be, even by accident.

Stdlib only: the provenance and release runners install nothing for this.

usage: gfrelease candidate stage --artifacts DIR --out DIR
"""

import argparse
import json
import shutil
import subprocess
import sys
from pathlib import Path

from ..adapters import git
from ..adapters.files import sha256_file as digest
from ..domain import policy, refs
from ..domain.formats import parse_version
from ..domain.manifest import (BUILD_INFO, BUNDLE, MANIFEST, RESERVED,
                               SAFE_NAME, CandidateError)
from ..domain.manifest import parse as parse_manifest
from ..domain.manifest import render as render_manifest
from ..errors import ReleaseError, ToolTimeout

BUILD_INFO_PREFIX = "buildinfo-"


def deliverable_prefix() -> str:
    """`<slug>-`: what a leg's deliverables artifact is called before the
    leg (release/product.json's slug)."""
    return policy.load_product()["slug"] + "-"


ISSUER = "https://token.actions.githubusercontent.com"
# cosign checks a bundle against a transparency log it may have to reach.
COSIGN_TIMEOUT = 600


class CandidateTimeout(Exception):
    """cosign did not finish: nothing was decided about the candidate."""


def regular_files(directory):
    """Every entry of `directory`, which must all be plain regular files."""
    files = {}
    for entry in directory.iterdir():
        if entry.is_symlink() or not entry.is_file():
            raise CandidateError(f"{entry}: not a regular file")
        files[entry.name] = entry
    return files


def check_name(name, where):
    if not SAFE_NAME.match(name):
        raise CandidateError(f"{where}: unsafe file name {name!r}")
    if name in RESERVED:
        raise CandidateError(f"{where}: {name!r} is a reserved provenance name")


# --------------------------------------------------------------------- stage


def collect_legs(artifacts_root):
    """{leg: (deliverable dir, build info dir)}, paired by exact name."""
    deliverables, infos = {}, {}
    prefix = deliverable_prefix()
    for entry in artifacts_root.iterdir():
        if entry.is_symlink() or not entry.is_dir():
            raise CandidateError(f"{entry}: expected one directory per artifact")
        if entry.name.startswith(prefix):
            deliverables[entry.name[len(prefix):]] = entry
        elif entry.name.startswith(BUILD_INFO_PREFIX):
            infos[entry.name[len(BUILD_INFO_PREFIX):]] = entry
        else:
            raise CandidateError(f"{entry.name}: not a release artifact")

    if not deliverables:
        raise CandidateError("no deliverable artifacts")

    unpaired = sorted(
        [f"{prefix}{leg} (no build info)"
         for leg in deliverables.keys() - infos.keys()] +
        [f"{BUILD_INFO_PREFIX}{leg} (no deliverables)"
         for leg in infos.keys() - deliverables.keys()])
    if unpaired:
        raise CandidateError("unpaired artifacts: " + ", ".join(unpaired))

    return {leg: (deliverables[leg], infos[leg]) for leg in deliverables}


def load_build_info(info_dir):
    files = regular_files(info_dir)
    if set(files) != {BUILD_INFO}:
        raise CandidateError(
            f"{info_dir.name}: expected exactly {BUILD_INFO}, got {sorted(files)}")
    try:
        with files[BUILD_INFO].open(encoding="utf-8") as f:
            info = json.load(f)
    except ValueError as e:
        raise CandidateError(f"{info_dir.name}: malformed {BUILD_INFO} ({e})")
    if not isinstance(info, dict):
        raise CandidateError(f"{info_dir.name}: malformed {BUILD_INFO}")
    return info


def check_build_info(leg, info):
    """A leg's build info names the build, so the leg really finished."""
    missing = sorted({"project_version", "build_id", "host_os", "modules"}
                     - set(info))
    if missing:
        raise CandidateError(f"{BUILD_INFO_PREFIX}{leg}: missing {missing}")


def stage(artifacts_root, out):
    # Everything is checked before anything is written: a refused input
    # leaves no half-built candidate behind.
    legs = collect_legs(artifacts_root)

    sources, owner = {}, {}
    for leg in sorted(legs):
        deliverable_dir, info_dir = legs[leg]
        check_build_info(leg, load_build_info(info_dir))

        files = regular_files(deliverable_dir)
        if not files:
            raise CandidateError(f"{deliverable_dir.name}: no deliverables")

        for name in sorted(files):
            check_name(name, deliverable_dir.name)
            if name in owner:
                raise CandidateError(
                    f"{name!r} is produced by both {owner[name]} and {leg}")
            owner[name] = leg
            sources[name] = files[name]

    try:
        out.mkdir(parents=True)
    except FileExistsError:
        raise CandidateError(f"{out}: already exists")

    # Hashed after the copy, so the manifest describes the bytes that ship.
    hashes = {}
    for name in sorted(sources):
        shutil.copyfile(sources[name], out / name)
        hashes[name] = digest(out / name)

    (out / MANIFEST).write_text(render_manifest(hashes),
                                encoding="utf-8", newline="\n")

    print(f"staged {len(sources)} deliverable(s) from {len(legs)} leg(s)")
    for name in sorted(hashes):
        print(f"  {hashes[name]}  {name}")


# -------------------------------------------------------------------- verify


def cosign_verify(directory, identity, sha, cosign):
    """The Sigstore identity: this workflow, this commit, GitHub's issuer."""
    if not identity or not sha:
        raise CandidateError("an exact identity and workflow sha are required")
    for name in (MANIFEST, BUNDLE):
        path = directory / name
        if path.is_symlink() or not path.is_file():
            raise CandidateError(f"missing {name}")

    cmd = [
        cosign, "verify-blob",
        "--bundle", str(directory / BUNDLE),
        "--certificate-identity", identity,
        "--certificate-oidc-issuer", ISSUER,
        "--certificate-github-workflow-sha", sha,
        str(directory / MANIFEST),
    ]
    try:
        # Captured: its "Verified OK" says nothing the exit status does
        # not, and whoever reads this tool's stderr takes it for a
        # problem. What it says when it refuses goes into the error.
        out = subprocess.run(cmd, timeout=COSIGN_TIMEOUT,
                             capture_output=True, encoding="utf-8",
                             errors="replace")
    except FileNotFoundError:
        raise CandidateError(f"{cosign} is not installed")
    except subprocess.TimeoutExpired:
        raise CandidateTimeout(f"{cosign} did not finish within "
                               f"{COSIGN_TIMEOUT} s")
    if out.returncode != 0:
        said = (out.stderr or out.stdout or "").strip().splitlines()
        raise CandidateError(f"{BUNDLE} does not verify {MANIFEST} "
                             f"as {identity} at {sha}"
                             + (f" (cosign: {said[-1][:300]})" if said else ""))


def verify(directory, identity, sha, cosign="cosign"):
    """Every check publication depends on. Returns the files to publish."""
    # 1. The signature, before a single byte of the manifest is trusted.
    cosign_verify(directory, identity, sha, cosign)

    # 2. The manifest, strictly: anything sha256sum -c would read loosely is
    #    refused rather than interpreted.
    entries = parse_manifest((directory / MANIFEST).read_bytes())

    # 3. Exact set equality. Nothing unlisted is published, nothing listed is
    #    missing.
    present = set(regular_files(directory)) - {MANIFEST, BUNDLE}
    if present != set(entries):
        missing = sorted(set(entries) - present)
        extra = sorted(present - set(entries))
        raise CandidateError(
            f"candidate does not match {MANIFEST}: "
            f"missing {missing}, unlisted {extra}")

    # 4. Every byte.
    for name in sorted(entries):
        if digest(directory / name) != entries[name]:
            raise CandidateError(f"{name}: sha256 does not match {MANIFEST}")

    return [directory / name for name in sorted(entries)] + \
        [directory / MANIFEST, directory / BUNDLE]


# ---------------------------------------------------------------------- main


def main(argv=None):
    parser = argparse.ArgumentParser(prog="gfrelease candidate",
                                     description=__doc__.splitlines()[0])
    sub = parser.add_subparsers(dest="command", required=True)

    p = sub.add_parser("stage", help="write the release candidate")
    p.add_argument("--artifacts", required=True, type=Path)
    p.add_argument("--out", required=True, type=Path)

    args = parser.parse_args(argv)
    try:
        stage(args.artifacts, args.out)
    except CandidateError as e:
        print(f"release_candidate: {e}", file=sys.stderr)
        return 1
    return 0


# ------------------------------------------------- the candidate, as admitted


def candidate_error(where: str, error: Exception) -> ReleaseError:
    """This module's verdict on a candidate, as a ReleaseError; a cosign
    that did not finish is no verdict (ToolTimeout)."""
    if isinstance(error, CandidateTimeout):
        return ToolTimeout(f"{where}: {error}")
    return ReleaseError(f"{where}: {error}")


def verify_candidate(where: str, work: Path, staged: dict, identity: str,
                     cosign: str) -> list[str]:
    """The staged candidate in @p work (staged.json taken out), as CI
    signed it for @p identity and the commit @p staged names; returns its
    files, which must be the ones @p staged lists."""
    try:
        verified = verify(work, identity, staged["commit"], cosign)
    except (CandidateError, CandidateTimeout) as e:
        raise candidate_error(where, e) from e
    files = sorted(p.name for p in verified
                   if p.name not in (MANIFEST, BUNDLE))
    if files != sorted(staged["files"]):
        raise ReleaseError(f"{where}: staged.json lists other files than the "
                           "signed manifest")
    return files


def check_candidate(git_dir: Path, ref: str, commit: str, version: str, *,
                    unreleased: bool = True) -> None:
    """A release candidate is its tag's build, of a commit on its series'
    release branch, and comes before that series' first release: never
    after it, never of a patch (refs.RC_TAG holds the shape). Without
    @p unreleased, a series released since is no error: the index then
    simply no longer lists the candidate (index_model.choose_rcs)."""
    tag = refs.tag_of(ref)
    if version != tag[1:]:
        raise ReleaseError(f"{ref}: git says {commit[:12]} is {version}, "
                           f"not {tag[1:]}")
    if git.resolve_commit(git_dir, ref) != commit:
        raise ReleaseError(f"{ref} does not point to {commit[:12]}")
    series = refs.rc_series(ref)
    branch = refs.HEADS + refs.release_branch(series)
    if git.resolve_commit(git_dir, branch) is None or not git.is_ancestor(
            git_dir, commit, branch):
        raise ReleaseError(f"{ref}: {commit[:12]} is not on "
                           f"{refs.release_branch(series)}")
    if not unreleased:
        return
    released = [t for t in git.release_tags(git_dir)
                if parse_version(t).series == series]
    if released:
        raise ReleaseError(f"{ref}: {series} has been released already "
                           f"({max(released, key=parse_version)}); a "
                           "candidate only comes before its first release")
