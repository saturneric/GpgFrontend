#!/usr/bin/env python3
# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""The git refs the release model knows, spelled once.

Three kinds of ref publish builds, and each feeds one channel:

  refs/heads/main          the nightly channel
  refs/heads/release/X.Y   the beta channel of series X.Y
  refs/tags/vX.Y.Z         a stable release (semver.RELEASE_TAG)

Nothing else is ever staged, promoted, signed or listed. CI decides the same
question in bash (scripts/ci/build_mode.sh, before any Python runs) and
cmake/GitVersion.cmake derives versions from the same branch shape;
scripts/tests/test_build_mode.py holds the spellings together.
"""

from __future__ import annotations

import re

from semver import RELEASE_TAG

HEADS = "refs/heads/"
TAGS = "refs/tags/"
MAIN_BRANCH = "main"
MAIN = HEADS + MAIN_BRANCH

_RELEASE_BRANCH = re.compile(r"release/([0-9]+\.[0-9]+)")
_COMMIT = re.compile(r"[0-9a-f]{40}")


def is_commit(text) -> bool:
    """A full, lower-case commit id."""
    return isinstance(text, str) and _COMMIT.fullmatch(text) is not None


def branch_of(ref: str) -> str:
    """The branch a full ref names, or "" when it is not a branch."""
    return ref[len(HEADS):] if ref.startswith(HEADS) else ""


def tag_of(ref: str) -> str:
    """The tag a full ref names, or "" when it is not a tag."""
    return ref[len(TAGS):] if ref.startswith(TAGS) else ""


def branch_series(branch: str) -> str:
    """"X.Y" for the branch release/X.Y, "" for any other."""
    m = _RELEASE_BRANCH.fullmatch(branch)
    return m.group(1) if m else ""


def release_series(ref: str) -> str:
    """"X.Y" for refs/heads/release/X.Y, "" for any other ref."""
    return branch_series(branch_of(ref))


def release_branch(series: str) -> str:
    """The branch a released series is maintained on."""
    return f"release/{series}"


def publishes_builds(ref: str) -> bool:
    """A branch whose builds are published: main, or a release branch."""
    return ref == MAIN or bool(release_series(ref))


def is_release_tag(tag: str) -> bool:
    return RELEASE_TAG.fullmatch(tag) is not None


def release_tag(version: str) -> str:
    """The tag a final version is released as."""
    return f"v{version}"


def release_tag_ref(version: str) -> str:
    """The ref of that tag."""
    return TAGS + release_tag(version)
