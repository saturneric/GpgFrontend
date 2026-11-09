#!/usr/bin/env python3
# Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""SemVer 2.0.0 parsing and precedence, for the release tooling.

The one comparator every script here uses, so the order a build is given by
cmake/GitVersion.cmake, the order check in CI and the channels the generator
computes all agree. Build metadata is kept but never decides precedence.
"""

from __future__ import annotations

import functools
import re
from dataclasses import dataclass

# ASCII digits only, and \Z rather than $: "$" also matches before a trailing
# newline, and "\d" also matches digits of other scripts.
_NUM = r"(?:0|[1-9][0-9]*)"
_ID = rf"(?:{_NUM}|[0-9]*[A-Za-z-][0-9A-Za-z-]*)"
_RE = re.compile(
    rf"v?({_NUM})\.({_NUM})\.({_NUM})"
    rf"(?:-({_ID}(?:\.{_ID})*))?"
    r"(?:\+([0-9A-Za-z-]+(?:\.[0-9A-Za-z-]+)*))?\Z")

# What a release tag may look like: a final vX.Y.Z, nothing else. Releases
# are tagged; nightlies (main) and betas (release/X.Y) never are. A
# prerelease tag (the historical v2.2.0-beta.1, a vX.Y.0-alpha.0 base tag,
# the moving `nightly`) is not a release, and nothing publishes one.
# scripts/ci/build_mode.sh spells the same shape in bash, and
# scripts/tests/test_build_mode.py holds the two together.
RELEASE_TAG = re.compile(rf"v({_NUM})\.({_NUM})\.({_NUM})\Z")


@functools.total_ordering
@dataclass(frozen=True)
class Version:
    major: int
    minor: int
    patch: int
    prerelease: tuple[str, ...] = ()
    build: str = ""

    @classmethod
    def parse(cls, text: str) -> "Version":
        m = _RE.match(text.strip(" \t\r\n"))
        if not m:
            raise ValueError(f"not a SemVer version: {text!r}")
        pre = tuple(m.group(4).split(".")) if m.group(4) else ()
        return cls(int(m.group(1)), int(m.group(2)), int(m.group(3)), pre,
                   m.group(5) or "")

    @property
    def core(self) -> tuple[int, int, int]:
        return (self.major, self.minor, self.patch)

    @property
    def series(self) -> str:
        return f"{self.major}.{self.minor}"

    @property
    def is_final(self) -> bool:
        return not self.prerelease

    @property
    def is_release(self) -> bool:
        """What a release is tagged and published as: final, and exactly a
        tagged commit (no build metadata)."""
        return not self.prerelease and not self.build

    @property
    def filesafe(self) -> str:
        """The spelling that names files and storage prefixes: build
        metadata's "+" folded to "-", as cmake/GitVersion.cmake does."""
        return str(self).replace("+", "-")

    def __str__(self) -> str:
        text = f"{self.major}.{self.minor}.{self.patch}"
        if self.prerelease:
            text += "-" + ".".join(self.prerelease)
        if self.build:
            text += "+" + self.build
        return text

    def _key(self):
        # A final release sorts above all its prereleases; identifiers compare
        # numerically when both are numbers, a number below any word, words in
        # ASCII order, and a shorter list below a longer one it prefixes.
        if not self.prerelease:
            return (self.core, 1, ())
        ids = tuple((0, int(p), "") if p.isdecimal() else (1, 0, p)
                    for p in self.prerelease)
        return (self.core, 0, ids)

    def __eq__(self, other):
        if not isinstance(other, Version):
            return NotImplemented
        return self._key() == other._key()

    def __lt__(self, other):
        if not isinstance(other, Version):
            return NotImplemented
        return self._key() < other._key()

    def __hash__(self):
        return hash(self._key())


def parse(text: str) -> Version:
    return Version.parse(text)
