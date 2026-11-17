# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""How the documents gfrelease writes and reads are spelled: JSON, times,
versions, paths. Nothing here reads a file or runs a tool."""

from __future__ import annotations

import datetime as dt
import hashlib
import json
import re

from ..errors import ReleaseError
from . import semver
from .limits import SMALL_OBJECT_LIMIT

SCHEMA = 1

# What a path inside a descriptor or the index may look like: relative to the
# product root, no scheme, no "..", no empty or dot segments, nothing a URL
# join or a filesystem could read as escaping it.
_SEGMENT = re.compile(r"[A-Za-z0-9][A-Za-z0-9._+-]*")


def check_relpath(path: str) -> str:
    # fullmatch: "$" would also match before a trailing newline, and let
    # "a/b\n/c" through.
    if not isinstance(path, str) or \
            any(not _SEGMENT.fullmatch(p) for p in path.split("/")):
        raise ReleaseError(f"not a product-relative path: {path!r}")
    return path


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def unique_keys(pairs: list[tuple]) -> dict:
    """json's object_pairs_hook refusing a key given twice: it reads
    differently in different parsers, so the checks here would pass on one
    value and a client act on the other."""
    keys = [k for k, _ in pairs]
    if len(keys) != len(set(keys)):
        raise ValueError("a key is given twice")
    return dict(pairs)


def load_json(data: bytes | str, what: str) -> dict:
    """A JSON object read from a store or the state directory; anything else
    is a ReleaseError naming @p what, never a traceback."""
    if not isinstance(data, (bytes, str)):
        raise ReleaseError(f"{what}: missing")
    if len(data) > SMALL_OBJECT_LIMIT:
        raise ReleaseError(f"{what}: larger than {SMALL_OBJECT_LIMIT} bytes")
    try:
        value = json.loads(data, object_pairs_hook=unique_keys)
    except (ValueError, UnicodeDecodeError, RecursionError) as e:
        raise ReleaseError(f"{what}: not JSON ({e})") from e
    if not isinstance(value, dict):
        raise ReleaseError(f"{what}: not a JSON object")
    return value


def canonical_json(value) -> bytes:
    """One byte form per value: what is hashed and signed is reproducible."""
    return (json.dumps(value, indent=1, sort_keys=True, ensure_ascii=False)
            + "\n").encode("utf-8")


def parse_version(text, what: str = "") -> semver.Version:
    """semver.parse, failing as a ReleaseError."""
    try:
        return semver.parse(text if isinstance(text, str) else "")
    except ValueError as e:
        raise ReleaseError(f"{what + ': ' if what else ''}{e}") from e


def iso(t: dt.datetime) -> str:
    return t.strftime("%Y-%m-%dT%H:%M:%SZ")


_UTC_TIME = re.compile(r"[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:"
                       r"[0-9]{2}Z", re.ASCII)


def parse_iso(text: str) -> dt.datetime:
    """A time as iso() writes it, every field zero-padded: strptime alone
    would take 2028-1-5T1:2:3Z too, which the update check refuses."""
    try:
        if not _UTC_TIME.fullmatch(text):
            raise ValueError("not YYYY-MM-DDTHH:MM:SSZ")
        return dt.datetime.strptime(text, "%Y-%m-%dT%H:%M:%SZ").replace(
            tzinfo=dt.timezone.utc)
    except (TypeError, ValueError) as e:
        raise ReleaseError(f"not a UTC time: {text!r}") from e
