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
  vX.Y.0-rc.N (a release candidate's tag): series X.Y has no release yet.
               A candidate only ever comes before its series' first release.

Any other ref (a pull request, a base tag, any other tag) has nothing to
check.

  gfrelease ci check-order [--ref REF] [--version VERSION] [--repo DIR]

REF is a branch name, or refs/tags/<tag> for a tag.

Without --version, asks cmake/GitVersion.cmake, as the build itself does.
"""

from __future__ import annotations

from pathlib import Path

from ..adapters import git
from ..domain import layout, refs, semver


def release_tags(repo: Path) -> list[semver.Version]:
    """Every release (vX.Y.Z) tagged in @p repo."""
    return [semver.parse(t) for t in git.release_tags(repo)]


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


def check_rc_tag(tag: str, tags: list[semver.Version]) -> list[str]:
    series = refs.rc_series(refs.TAGS + tag)
    released = sorted(t for t in tags if t.series == series)
    if released:
        return [f"{tag} is a release candidate of {series}, which already has "
                f"the release {released[-1]}: a candidate only comes before "
                f"{series}.0. Fixes go out as betas and fix releases."]
    return []


def check_tag(tag: str, repo: Path) -> list[str]:
    if refs.is_rc_tag(tag):
        return check_rc_tag(tag, release_tags(repo))
    if not refs.is_release_tag(tag):
        return []
    notes = repo / layout.notes_source(tag[1:])
    if not notes.is_file() or not notes.read_text(encoding="utf-8").strip():
        return [f"{tag} has no release notes: add release/notes/{notes.name} "
                "(scripts/release/notes.sh drafts it) before tagging"]
    return []
