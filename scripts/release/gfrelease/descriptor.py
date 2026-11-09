# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""The shape of a descriptor (release.json), checked before anything in it
is believed: by the host when it admits one (index.py), and by anyone who
checks a published index (verify.py)."""

from __future__ import annotations

from common import ReleaseError, refs

_STRINGS = ("product", "kind", "version", "ref", "commit", "sums", "sums_sig",
            "promoted_at", "built_at")
_ARTIFACT_STRINGS = ("name", "path", "sig", "sha256", "build_sha256",
                     "transform", "os", "arch", "kind", "flavor")


def _str_map(value) -> bool:
    return isinstance(value, dict) and \
        all(isinstance(v, str) for v in value.values())


def check_shape(d: dict, path: str) -> None:
    """Every field a reader reads has the type it reads it as. A
    descriptor comes from an untrusted bucket: one of the wrong shape is
    refused like any other bad one, never a traceback that stops the run."""
    def wrong(what: str):
        raise ReleaseError(f"{path}: {what} has the wrong type")
    for k in _STRINGS:
        if k in d and not isinstance(d[k], str):
            wrong(k)
    if "commit" in d and not refs.is_commit(d["commit"]):
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


def require(d: dict, path: str, *fields: str) -> None:
    """@p d has every one of @p fields, and each artifact entry is whole
    and listed once."""
    missing = [f for f in fields if f not in d]
    if missing:
        raise ReleaseError(f"{path}: lacks {', '.join(missing)}")
    for a in d.get("artifacts", []):
        if not isinstance(a, dict) or any(
                k not in a for k in ("name", "path", "sig", "sha256")):
            raise ReleaseError(f"{path}: an artifact entry is incomplete")
    # Every check of a descriptor compares {name: sha256}: a name listed twice would
    # collapse there, and its other entry (hash, path) reach the index
    # unchecked.
    for field in ("name", "path"):
        values = [a[field] for a in d.get("artifacts", [])]
        if len(values) != len(set(values)):
            raise ReleaseError(f"{path}: an artifact {field} is listed twice")


def names_build(entry) -> bool:
    """An index entry that names a build (the nightly, a beta): its version,
    its descriptor's path and that descriptor's hash."""
    return isinstance(entry, dict) and all(
        isinstance(entry.get(k), str) for k in ("version", "descriptor",
                                                "sha256"))
