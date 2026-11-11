# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""Where everything lives in the stores, and what each signature says it
signs. Spelled once: the roles that write a path (stage, promote, stable,
import, index) and the ones that read it (index, verify) can
only agree.

The staging store (CI writes, the host and the maintainer read):

  branches/<branch>/<filesafe>/   a candidate built from main or release/X.Y
  tags/<filesafe>/                a candidate built from a release tag

The builds store:

  nightly/<filesafe>/             a promoted build of main (the nightly
                                  channel) or of release/X.Y (the beta)
  releases/<version>/             a stable or imported release
  notes/<version>/<sha256>.md     release notes, content-addressed
  commits/<sha>                   a marker: this commit's build is published

The update store:

  v1/index.json                   the signed index (common.INDEX)
  v1/history/<serial>.json        every index ever published

The signing host's container (the Release-Server repository) lists
branches/**/staged.json, releases/**, notes/** and nightly/*/release.json to
tell whether anything changed; scripts/tests/test_gfrelease.py holds these
names to what it lists.
"""

from __future__ import annotations

from common import DESCRIPTOR, parse_version

BRANCHES = "branches"
TAGS = "tags"
NIGHTLY = "nightly"
RELEASES = "releases"
NOTES = "notes"

# The kinds a descriptor may say it is, and how each kind is signed.
KIND_NIGHTLY = "nightly"    # a build of main or of release/X.Y
KIND_STABLE = "stable"      # a release through the offline stage
KIND_IMPORTED = "imported"  # a release from before this pipeline
SCHEME_SIGSTORE = "sigstore"  # a build: CI's manifest, Sigstore-signed
SCHEME_OPENPGP = "openpgp"    # a release: the maintainer's signatures


def staged_branch(branch: str, filesafe: str) -> str:
    return f"{BRANCHES}/{branch.replace('/', '-')}/{filesafe}"


def staged_tag(filesafe: str) -> str:
    return f"{TAGS}/{filesafe}"


def nightly_dir(version: str) -> str:
    """Named after the version itself, never after a file-safe spelling
    something else claims for it."""
    return f"{NIGHTLY}/{parse_version(version).filesafe}"


def release_dir(version: str) -> str:
    return f"{RELEASES}/{version}"


def descriptor_of(directory: str) -> str:
    return f"{directory}/{DESCRIPTOR}"


def notes_path(version: str, digest: str) -> str:
    return f"{NOTES}/{version}/{digest}.md"


def commit_marker(commit: str) -> str:
    return f"commits/{commit}"


def history_path(serial: int) -> str:
    return f"v1/history/{serial}.json"


def index_comment(slug: str, serial) -> str:
    """The trusted comment of the index key's signature on an index."""
    return f"product={slug} serial={serial}"
