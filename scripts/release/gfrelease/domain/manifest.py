# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""A release candidate's manifest (SHA256SUMS.txt) and the provenance names
beside it: how CI writes it (app/candidate.py) and how everyone who checks
a candidate reads it, strictly."""

from __future__ import annotations

import re

from ..errors import ReleaseError

MANIFEST = "SHA256SUMS.txt"
BUNDLE = "SHA256SUMS.txt.sigstore.json"
BUILD_INFO = "build-info.json"

# Provenance names. A deliverable may never take one: it would shadow, or be
# shadowed by, the file that describes it.
RESERVED = frozenset({MANIFEST, BUNDLE, BUILD_INFO})

# \Z, not $: "$" also matches before a trailing newline, which a file name
# may end in.
SAFE_NAME = re.compile(r"^[A-Za-z0-9][A-Za-z0-9._+-]*\Z")
MANIFEST_LINE = re.compile(r"^([0-9a-f]{64})  (\S+)\Z")


class CandidateError(Exception):
    pass


class CandidateTimeout(Exception):
    """cosign did not finish: nothing was decided about the candidate."""


def render(hashes):
    """SHA256SUMS.txt text for {name: sha256}, in `sha256sum -c` format."""
    return "".join(f"{hashes[name]}  {name}\n" for name in sorted(hashes))


def parse(data):
    """{name: sha256} from the signed bytes, strictly."""
    try:
        text = data.decode("ascii")
    except UnicodeDecodeError:
        raise CandidateError(f"{MANIFEST}: not ASCII")
    if "\r" in text:
        raise CandidateError(f"{MANIFEST}: carriage return")
    if not text.endswith("\n"):
        raise CandidateError(f"{MANIFEST}: no trailing newline")

    entries = {}
    for number, line in enumerate(text[:-1].split("\n"), 1):
        match = MANIFEST_LINE.match(line)
        if not match:
            raise CandidateError(f"{MANIFEST}:{number}: malformed line")
        sha, name = match.groups()
        if not SAFE_NAME.match(name):
            raise CandidateError(f"{MANIFEST}:{number}: unsafe name {name!r}")
        if name in RESERVED:
            raise CandidateError(f"{MANIFEST}:{number}: lists {name!r}")
        if name in entries:
            raise CandidateError(f"{MANIFEST}:{number}: duplicate {name!r}")
        entries[name] = sha
    return entries


def parse_manifest(data: bytes, what: str) -> dict[str, str]:
    """{name: sha256} of a SHA256SUMS.txt, read as strictly as CI's own
    (parse()), failing as a ReleaseError naming @p what."""
    try:
        return parse(data)
    except CandidateError as e:
        raise ReleaseError(f"{what}: {e}") from e


def render_manifest(hashes: dict[str, str]) -> bytes:
    """The SHA256SUMS.txt bytes for {name: sha256}."""
    return render(hashes).encode("ascii")
