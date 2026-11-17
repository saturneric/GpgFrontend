# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""Release notes (release/notes/<version>.md), as other places show them.

The notes are written once, wrapped at about 78 columns, and published
exactly so: signed by the maintainer, pinned by the index. A GitHub release
body breaks the line at every newline, so for it the text of each paragraph
and list item is put back on one line (for_github). What is not running
text is kept as it is: headings, fenced code, tables, block quotes, HTML,
rules, and lines that end in a hard break.
"""

from __future__ import annotations

import re

_FENCE = re.compile(r"^ {0,3}(`{3,}|~{3,})")
_ITEM = re.compile(r"^(\s*)(?:[-*+]|[0-9]{1,9}[.)])(?:\s+|$)")
# Lines that are a block of their own, never part of running text.
_BLOCK = re.compile(r"^ {0,3}(?:#{1,6}(?:\s|$)|>|\||<|(?:[-*_]\s*){3,}$)")


def _heading_of(text: str, version: str) -> bool:
    """@p text is a heading naming @p version (`## 2.2.5 (2026-11-16)`)."""
    m = re.match(r"#{1,6}\s+v?(\S+)", text)
    return bool(m) and m.group(1) == version


def _hard_break(line: str) -> bool:
    return line.endswith("  ") or line.endswith("\\")


def unwrap(text: str) -> str:
    """@p text with every paragraph and list item on one line."""
    out: list[str] = []
    fence = None  # the fence a code block was opened with
    joinable = False  # the last line of out is running text
    for line in text.split("\n"):
        if fence is not None:
            out.append(line)
            m = _FENCE.match(line)
            if m and m.group(1)[0] == fence[0] and \
                    len(m.group(1)) >= len(fence) and \
                    not line.strip()[len(m.group(1)):].strip():
                fence = None
            continue
        m = _FENCE.match(line)
        if m:
            fence = m.group(1)
            out.append(line)
            joinable = False
            continue
        if not line.strip():
            out.append("")
            joinable = False
            continue
        if _BLOCK.match(line) or not _ITEM.match(line) and \
                line.startswith("    ") and not joinable:
            out.append(line if _hard_break(line) else line.rstrip())
            joinable = False
            continue
        if _ITEM.match(line) or not joinable:
            out.append(line if _hard_break(line) else line.rstrip())
            joinable = not _hard_break(line)
            continue
        # A continuation: one space between it and the line it continues.
        out[-1] = out[-1] + " " + line.strip()
        if _hard_break(line):
            out[-1] = out[-1] + ("  " if line.endswith("  ") else "")
        joinable = not _hard_break(line)
    return "\n".join(out)


def for_github(text: str, version: str) -> str:
    """@p text (release notes of @p version) as a GitHub release body: its
    lines ended in LF, the heading naming the version left out (the release
    is titled with it), and its running text unwrapped."""
    lines = text.replace("\r\n", "\n").replace("\r", "\n").split("\n")
    while lines and not lines[0].strip():
        lines.pop(0)
    if lines and _heading_of(lines[0], version):
        lines.pop(0)
        while lines and not lines[0].strip():
            lines.pop(0)
    return unwrap("\n".join(lines)).rstrip() + "\n"
