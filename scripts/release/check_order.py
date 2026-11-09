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

  vX.Y.Z (a release tag build): the tagged tree carries
               release/notes/X.Y.Z.md, the reviewed notes that are published
               with it (scripts/release/notes.sh drafts them).

Any other ref (a pull request, a base tag, any other tag) has nothing to
check.

  check_order.py [--ref REF] [--version VERSION] [--repo DIR]

REF is a branch name, or refs/tags/<tag> for a tag.

Without --version, asks cmake/GitVersion.cmake, as the build itself does.
"""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent.parent
sys.path.insert(0, str(HERE))

import gitversion  # noqa: E402
import policy as policy_mod  # noqa: E402
import refs  # noqa: E402
import semver  # noqa: E402


def git(repo: Path, *args: str) -> str:
    return subprocess.run(["git", *args], cwd=repo, check=True,
                          capture_output=True, text=True,
                          timeout=gitversion.TIMEOUT).stdout.strip()


def current_ref(repo: Path) -> str:
    """The branch being built, a tag as its full ref, or "" for anything
    else (a pull request's merge ref, a detached HEAD)."""
    ref = os.environ.get("GITHUB_REF", "")
    if refs.branch_of(ref):
        return refs.branch_of(ref)
    if refs.tag_of(ref):
        return ref
    if ref:
        return ""
    try:
        return git(repo, "symbolic-ref", "--short", "-q", "HEAD")
    except subprocess.CalledProcessError:
        return ""


def release_tags(repo: Path) -> list[semver.Version]:
    return [semver.parse(t) for t in git(repo, "tag", "--list", "v*").split()
            if refs.is_release_tag(t)]


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

    if ref == refs.MAIN_BRANCH:
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

    series = refs.branch_series(ref)
    if series:
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
    if not refs.is_release_tag(tag):
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
    if refs.tag_of(ref):
        problems = check_tag(refs.tag_of(ref), args.repo)
        for p in problems:
            print(f"::error::{p}", file=sys.stderr)
        return 1 if problems else 0
    # Only the branches whose builds are published have an order to keep
    # (release/2.2-fix is no release branch, and publishes nothing).
    if ref != refs.MAIN_BRANCH and not refs.branch_series(ref):
        print(f"check_order: nothing to check for ref {ref or '(none)'}")
        return 0

    try:
        version = args.version or gitversion.derive(args.repo, ref=ref)
        mainline = policy_mod.mainline(policy_mod.load_policy(args.policy))
        problems = check(version, ref, release_tags(args.repo), mainline)
    except subprocess.CalledProcessError as e:
        print(f"::error::check_order: {' '.join(map(str, e.cmd[:2]))} "
              f"failed: {(e.stderr or '').strip()}", file=sys.stderr)
        return 1
    except (subprocess.TimeoutExpired, OSError, ValueError) as e:
        print(f"::error::check_order: {e}", file=sys.stderr)
        return 1
    for p in problems:
        print(f"::error::{p}", file=sys.stderr)
    if not problems:
        print(f"check_order: {ref} builds {version}; order holds")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
