# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""A stable release as GitHub shows it, mirrored from the signed index.

The index is what users get; a GitHub release is a copy of it for people
who look there: the files, each with its maintainer signature, and the
signed notes as the body. The manifest (SHA256SUMS.txt) stays in the builds store: a
person checks a file by its own signature. Only what the pipeline released
(a descriptor of kind stable) is mirrored; an imported release came from
GitHub and stays as it is there.

desired() is the release the index says there should be; plan() the steps
from what GitHub has to that. A release is made as a draft, filled, and
published last, so an interrupted run leaves only a draft. Nothing is ever
deleted from a published release or replaced in it: a file there with the
same name and other bytes is a conflict, for a person to look at. Files
GitHub has that the index does not name are left alone.
"""

from __future__ import annotations

from dataclasses import dataclass

from ..errors import ReleaseError
from . import layout, refs
from .layout import SIG
from .release_notes import for_github

# The most a release body may hold, in characters (GitHub's limit).
BODY_LIMIT = 125_000


@dataclass(frozen=True)
class Asset:
    name: str    # its name on GitHub
    path: str    # where it lies in the builds store
    sha256: str
    size: int


@dataclass(frozen=True)
class Release:
    version: str
    tag: str
    name: str
    commit: str  # the tag must point at it
    body: str
    latest: bool  # GitHub's "Latest release" badge
    assets: tuple[Asset, ...]


@dataclass(frozen=True)
class Step:
    """One step of a plan. kind: create (a draft), upload, replace (a
    file of a draft that has other bytes, or never finished uploading),
    edit (name and body), publish, latest (mark it the latest), conflict
    (refused: a published file with other bytes)."""
    kind: str
    asset: Asset | None = None
    detail: str = ""


def banner(reason: str) -> str:
    return (f"> **Withdrawn:** {reason.rstrip('.')}. Do not install this "
            "release; the update check offers a newer one.\n")


def mirrored(descriptor: dict) -> bool:
    """A release the pipeline made, which GitHub mirrors."""
    return descriptor.get("kind") == layout.KIND_STABLE


def desired(index: dict, descriptor: dict, entry: dict, notes: str,
            small: dict[str, tuple[str, int]]) -> Release:
    """The GitHub release of the release @p descriptor, listed in @p index
    as @p entry, with @p notes (their text, checked against the index).
    @p small: the sha256 and size of each file's signature by its path in
    the builds store."""
    version = descriptor["version"]
    assets = []
    for a in sorted(descriptor["artifacts"], key=lambda a: a["name"]):
        assets.append(Asset(a["name"], a["path"], a["sha256"], a["size"]))
        assets.append(Asset(a["name"] + SIG, a["sig"], *small[a["sig"]]))

    body = ""
    for w in index.get("withdrawn", []):
        if w.get("version") == version:
            body += banner(w.get("reason") or "withdrawn") + "\n"
    body += for_github(notes, version)
    if len(body) > BODY_LIMIT:
        raise ReleaseError(f"{version}: a release body of {len(body)} "
                           f"characters, more than GitHub's {BODY_LIMIT}")
    return Release(version=version, tag=refs.release_tag(version),
                   name=refs.release_tag(version),
                   commit=descriptor["commit"], body=body,
                   latest=index["channels"].get("stable") == version,
                   assets=tuple(assets))


def _same_text(a: str | None, b: str) -> bool:
    return (a or "").replace("\r\n", "\n").strip() == b.strip()


def plan(want: Release, have: dict | None, latest_tag: str | None) -> list:
    """The steps from @p have (GitHub's release of the tag, or None) to
    @p want; @p latest_tag is the tag of GitHub's latest release. An empty
    plan: nothing to do."""
    steps: list[Step] = []
    draft = have is None or have.get("draft", False)
    if have is None:
        steps.append(Step("create"))
    elif have.get("name") != want.name or \
            not _same_text(have.get("body"), want.body):
        steps.append(Step("edit"))
    present = {a["name"]: a for a in (have or {}).get("assets", [])}
    for asset in want.assets:
        there = present.get(asset.name)
        if there is None:
            steps.append(Step("upload", asset))
            continue
        why = _differs(asset, there)
        if why is None:
            continue
        if draft or there.get("state") != "uploaded":
            steps.append(Step("replace", asset, why))
        else:
            steps.append(Step("conflict", asset, why))
    if any(s.kind == "conflict" for s in steps):
        return steps
    if draft:
        steps.append(Step("publish"))
    elif want.latest and latest_tag != want.tag:
        steps.append(Step("latest"))
    return steps


def _differs(asset: Asset, there: dict) -> str | None:
    """Why GitHub's file @p there is not @p asset, or None when it is."""
    if there.get("state") != "uploaded":
        return "its upload never finished"
    # GitHub states the sha256 of every file uploaded since 2025; of an
    # older one only its size is known.
    if there.get("size") != asset.size:
        return f"{there.get('size')} bytes, not {asset.size}"
    digest = there.get("digest")
    if digest is not None and digest != f"sha256:{asset.sha256}":
        return f"{digest}, not sha256:{asset.sha256}"
    return None
