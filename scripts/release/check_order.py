#!/usr/bin/env python3
# Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""Fail a build whose version would sort wrongly against the releases.

Run by CI on every push to main and release/X.Y. A nightly must be newer than
everything already released on its line, or clients comparing versions would
offer an older build as an update, or never offer a newer one:

  main         newer than every release tag in the repository, and in the
               series release/policy.toml calls mainline. Fails when the base
               tag scripts/release/cut.sh sets was not pushed, or when the
               policy was not moved on with it.
  release/X.Y  in series X.Y, and newer than every X.Y tag.

  vX.Y.Z[-beta.N|-rc.N] (a release tag build): the tagged tree carries
               release/notes/X.Y.Z[-...].md, the reviewed notes that are
               published with it (scripts/release/notes.sh drafts them).

Any other ref (a pull request, a base tag) has nothing to check.

  check_order.py [--ref REF] [--version VERSION] [--repo DIR]

REF is a branch name, or refs/tags/<tag> for a tag.

Without --version, asks cmake/GitVersion.cmake, as the build itself does.
"""

from __future__ import annotations

import argparse
import os
import re
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent.parent
sys.path.insert(0, str(HERE))

import policy as policy_mod  # noqa: E402
import semver  # noqa: E402


def git(repo: Path, *args: str) -> str:
    return subprocess.run(["git", *args], cwd=repo, check=True,
                          capture_output=True, text=True).stdout.strip()


def derived_version(repo: Path, ref: str) -> str:
    out = subprocess.run(
        ["cmake", f"-DGF_SOURCE_DIR={repo}", f"-DGPGFRONTEND_VERSION_REF={ref}",
         "-DGF_FIELD=version", "-P", str(ROOT / "cmake" / "GitVersion.cmake")],
        check=True, capture_output=True, text=True)
    return out.stdout.strip()


def current_ref(repo: Path) -> str:
    ref = os.environ.get("GITHUB_REF", "")
    if ref.startswith("refs/heads/"):
        return ref[len("refs/heads/"):]
    if ref.startswith("refs/tags/"):
        return ref
    if ref:
        return ""
    try:
        return git(repo, "symbolic-ref", "--short", "-q", "HEAD")
    except subprocess.CalledProcessError:
        return ""


def release_tags(repo: Path) -> list[semver.Version]:
    return [semver.parse(t) for t in git(repo, "tag", "--list", "v*").split()
            if semver.RELEASE_TAG.match(t)]


def check(version: str, ref: str, tags: list[semver.Version],
          mainline: str) -> list[str]:
    v = semver.parse(version)
    problems = []

    def behind(newest) -> bool:
        # A build exactly on a tag (no commits since, so no build metadata)
        # is that release, not a nightly behind it.
        if newest is None:
            return False
        return not (v > newest or (v == newest and not v.build))

    if ref == "main":
        newest = max(tags, default=None)
        if behind(newest):
            problems.append(
                f"main builds {v}, which is not newer than the release "
                f"{newest}. Was the base tag of the last cut pushed? "
                "(scripts/release/cut.sh)")
        if mainline and v.series != mainline:
            problems.append(
                f"main builds the {v.series} series, but release/policy.toml "
                f"calls {mainline} mainline. Cut and policy move together.")
        return problems

    m = re.match(r"^release/(\d+\.\d+)$", ref)
    if m:
        series = m.group(1)
        if v.series != series:
            problems.append(
                f"{ref} builds {v}, outside its series {series}. The branch "
                "must not reach a base tag of a later series.")
        mine = [t for t in tags if t.series == series]
        newest = max(mine, default=None)
        if behind(newest):
            problems.append(f"{ref} builds {v}, not newer than {newest}")
    return problems


def check_tag(tag: str, repo: Path) -> list[str]:
    if not semver.RELEASE_TAG.match(tag):
        return []
    notes = repo / "release" / "notes" / f"{tag[1:]}.md"
    if not notes.is_file() or not notes.read_text(encoding="utf-8").strip():
        return [f"{tag} has no release notes: add release/notes/{notes.name} "
                "(scripts/release/notes.sh drafts it) before tagging"]
    return []


def main(argv: list[str]) -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--repo", type=Path, default=ROOT)
    ap.add_argument("--ref")
    ap.add_argument("--version")
    ap.add_argument("--policy", type=Path, default=policy_mod.POLICY)
    args = ap.parse_args(argv)

    ref = args.ref if args.ref is not None else current_ref(args.repo)
    if ref.startswith("refs/tags/"):
        problems = check_tag(ref[len("refs/tags/"):], args.repo)
        for p in problems:
            print(f"::error::{p}", file=sys.stderr)
        return 1 if problems else 0
    if ref != "main" and not ref.startswith("release/"):
        print(f"check_order: nothing to check for ref {ref or '(none)'}")
        return 0

    version = args.version or derived_version(args.repo, ref)
    mainline = policy_mod.mainline(policy_mod.load_policy(args.policy))
    problems = check(version, ref, release_tags(args.repo), mainline)
    for p in problems:
        print(f"::error::{p}", file=sys.stderr)
    if not problems:
        print(f"check_order: {ref} builds {version}; order holds")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
