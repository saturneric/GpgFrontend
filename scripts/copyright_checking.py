#!/usr/bin/env python3
# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
#
# This file is part of GpgFrontend.
#
# GpgFrontend is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# GpgFrontend is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GpgFrontend. If not, see <https://www.gnu.org/licenses/>.
#
# The initial version of the source code is inherited from
# the gpg4usb project, which is under GPL-3.0-or-later.
#
# All the source code of GpgFrontend was modified and released by
# Saturneric <eric@bktus.com> starting on May 12, 2021.
#
# SPDX-License-Identifier: GPL-3.0-or-later

"""Check, and optionally repair, the copyright headers of GpgFrontend sources.

A file that already carries a GpgFrontend header PASSES even when its year range
is older than the current target: the year records when that file was last
touched, so rewriting it in bulk would be a lie. Only files with no header at all
are reported as problems. Pass --update-year to normalise the years anyway.

Usage:
    copyright_checking.py                 # check only, never writes (CI default)
    copyright_checking.py --fix           # add headers to files that have none
    copyright_checking.py --fix --update-year
    copyright_checking.py src/core        # limit the scan to some paths
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from dataclasses import dataclass
from enum import Enum
from fnmatch import fnmatch
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent

# The year range stamped into headers this script writes itself.
DEFAULT_YEAR = "2021-2026"

# Where to look, relative to the repository root.
DEFAULT_ROOTS = ("src", "modules", "rust")

# Directory names that never hold our own sources, pruned wherever they appear.
EXCLUDED_DIR_NAMES = frozenset(
    {".git", ".idea", ".venv", ".vscode", "__pycache__", "node_modules", "target"}
)

# Repository-relative directory paths (glob syntax) that are vendored code.
EXCLUDED_DIR_PATHS = (
    "build",
    "build-*",
    "cmake-build-*",
    "third_party",
    "modules/src/m_email/vmime",
)

SOURCE_SUFFIXES = frozenset(
    {".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx", ".rs"}
)
SCRIPT_SUFFIXES = frozenset({".cmake"})
SCRIPT_FILENAMES = frozenset({"CMakeLists.txt"})

# How far into a file a copyright notice may start before we stop believing it.
NOTICE_SCAN_CHARS = 8192

_LICENSE_BODY = """\
This file is part of GpgFrontend.

GpgFrontend is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

GpgFrontend is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GpgFrontend. If not, see <https://www.gnu.org/licenses/>.

The initial version of the source code is inherited from
the gpg4usb project, which is under GPL-3.0-or-later.

All the source code of GpgFrontend was modified and released by
Saturneric <eric@bktus.com> starting on May 12, 2021.

SPDX-License-Identifier: GPL-3.0-or-later
"""


def _notice_lines(year: str) -> list[str]:
    header = f"Copyright (C) {year} Saturneric <eric@bktus.com>"
    return [header, ""] + _LICENSE_BODY.splitlines()


def source_header(year: str) -> str:
    """The C/C++/Rust block-comment header."""
    lines = [f" * {line}".rstrip() for line in _notice_lines(year)]
    return "\n".join(["/**", *lines, " *", " */"])


def script_header(year: str) -> str:
    """The CMake/shell hash-comment header."""
    return "\n".join(f"# {line}".rstrip() for line in _notice_lines(year))


class Status(Enum):
    """What the header of a single file looks like."""

    CURRENT = "up to date"
    DATED = "older year"
    VARIANT = "hand-edited header"
    FOREIGN = "third-party notice"
    MISSING = "no header"
    EMPTY = "empty file"
    IGNORED = "ignored by git"
    ERROR = "unreadable"


@dataclass(frozen=True)
class Style:
    """A header flavour plus the pattern that recognises it in a file."""

    name: str
    header: str
    pattern: re.Pattern[str]


# "2026", "2021-2026" and "2021 - 2026" are all acceptable year spellings.
_YEAR_PATTERN = r"\d{4}(?:\s*-\s*\d{4})?"


def build_style(name: str, header: str, year: str) -> Style:
    """Compile a tolerant matcher for `header`: any year, any line ending."""
    parts = []
    for line in header.split("\n"):
        # Trailing blanks drift in over time; accept them on every line.
        escaped = re.escape(line).replace(re.escape(year), _YEAR_PATTERN)
        parts.append(escaped + r"[ \t]*")
    body = r"\r?\n".join(parts)
    # Rust sources open the block with "/*" rather than "/**".
    body = body.replace(re.escape("/**"), r"/\*\*?", 1)
    return Style(name=name, header=header, pattern=re.compile(body))


def make_styles(year: str) -> tuple[Style, Style]:
    return (
        build_style("source", source_header(year), year),
        build_style("script", script_header(year), year),
    )


def style_for(path: Path, source: Style, script: Style) -> Style | None:
    if path.name in SCRIPT_FILENAMES or path.suffix in SCRIPT_SUFFIXES:
        return script
    if path.suffix in SOURCE_SUFFIXES:
        return source
    return None


def is_excluded_dir(path: Path) -> bool:
    if path.name in EXCLUDED_DIR_NAMES:
        return True
    try:
        rel = path.relative_to(REPO_ROOT).as_posix()
    except ValueError:
        return False  # outside the repository: only the name rules apply
    return any(fnmatch(rel, pattern) for pattern in EXCLUDED_DIR_PATHS)


def iter_files(root: Path, source: Style, script: Style):
    """Yield every candidate file under `root`, pruning vendored directories."""
    if root.is_file():
        if style_for(root, source, script) is not None:
            yield root
        return

    stack = [root]
    while stack:
        current = stack.pop()
        try:
            entries = sorted(current.iterdir())
        except OSError as exc:
            print(f"[error]   {display(current)}: {exc}")
            continue
        for entry in entries:
            if entry.is_symlink():
                continue
            if entry.is_dir():
                if not is_excluded_dir(entry):
                    stack.append(entry)
            elif entry.is_file() and style_for(entry, source, script) is not None:
                yield entry


def git_ignored(root: Path, files: list[Path]) -> set[Path]:
    """Return the subset of `files` that git ignores (build and generated output).

    `root` may live in a submodule, so ask that checkout rather than the outer
    one. A missing git, or a directory outside any checkout, means "nothing is
    ignored" and every file gets checked.
    """
    if not files:
        return set()
    try:
        top = subprocess.run(
            ["git", "-C", str(root), "rev-parse", "--show-toplevel"],
            capture_output=True,
            text=True,
            check=True,
        ).stdout.strip()
        payload = "".join(f"{path}\0" for path in files)
        proc = subprocess.run(
            ["git", "-C", top, "check-ignore", "-z", "--stdin"],
            input=payload,
            capture_output=True,
            text=True,
        )
    except (OSError, subprocess.SubprocessError):
        return set()
    if proc.returncode not in (0, 1):  # 1 simply means "none of them"
        return set()
    # check-ignore echoes each ignored path exactly as it was fed in.
    by_text = {str(path): path for path in files}
    return {by_text[line] for line in proc.stdout.split("\0") if line in by_text}


def display(path: Path) -> str:
    try:
        return path.relative_to(REPO_ROOT).as_posix()
    except ValueError:
        return str(path)


def detect_newline(content: str) -> str:
    return "\r\n" if "\r\n" in content else "\n"


def classify(content: str, style: Style) -> tuple[Status, re.Match[str] | None]:
    """Decide what `content` already carries, without modifying anything."""
    if not content.strip():
        return Status.EMPTY, None

    match = style.pattern.search(content)
    if match is not None:
        found = match.group(0)
        exact = found in (style.header, style.header.replace("\n", "\r\n"))
        return (Status.CURRENT if exact else Status.DATED), match

    head = content[:NOTICE_SCAN_CHARS]
    if "GpgFrontend" in head and "SPDX-License-Identifier: GPL-3.0-or-later" in head:
        # Our licence, but the block has been reflowed or trimmed by hand.
        return Status.VARIANT, None
    if "Copyright" in head or "SPDX-License-Identifier" in head:
        return Status.FOREIGN, None
    return Status.MISSING, None


def insert_header(content: str, header: str) -> str:
    """Prepend `header`, keeping any BOM, shebang and the file's line endings."""
    newline = detect_newline(content)

    bom = ""
    if content.startswith("\ufeff"):  # keep a byte-order mark first
        bom, content = content[0], content[1:]

    preamble: list[str] = []
    if content.startswith("#!"):
        line, _, content = content.partition("\n")
        preamble.append(line.rstrip("\r"))
        # Keep a Python encoding cookie next to its shebang.
        nxt = content.partition("\n")[0]
        if re.match(r"^#.*coding[:=]", nxt):
            line, _, content = content.partition("\n")
            preamble.append(line.rstrip("\r"))
        preamble.append("")

    body = content.lstrip("\r\n")
    lines = [*preamble, *header.split("\n")]
    if body:
        lines.append("")
    return bom + newline.join(lines) + newline + body


def replace_header(content: str, match: re.Match[str], header: str) -> str:
    rendered = header.replace("\n", detect_newline(content))
    return content[: match.start()] + rendered + content[match.end() :]


def read_text(path: Path) -> str:
    # newline="" keeps CRLF intact so we can write it back unchanged.
    with open(path, "r", encoding="utf-8", newline="") as handle:
        return handle.read()


def write_text(path: Path, content: str) -> None:
    with open(path, "w", encoding="utf-8", newline="") as handle:
        handle.write(content)


@dataclass
class Report:
    counts: dict[Status, int]
    fixed: int = 0
    failures: int = 0

    @classmethod
    def empty(cls) -> "Report":
        return cls(counts={status: 0 for status in Status})


def process(path: Path, style: Style, args: argparse.Namespace, report: Report) -> None:
    try:
        content = read_text(path)
    except UnicodeDecodeError:
        report.counts[Status.ERROR] += 1
        report.failures += 1
        print(f"[error]   {display(path)}: not valid UTF-8")
        return
    except OSError as exc:
        report.counts[Status.ERROR] += 1
        report.failures += 1
        print(f"[error]   {display(path)}: {exc}")
        return

    status, match = classify(content, style)
    report.counts[status] += 1

    if status is Status.CURRENT:
        return

    if status is Status.DATED:
        # An older year is fine: it says when the file was last changed.
        if args.update_year and match is not None:
            if args.fix:
                write_text(path, replace_header(content, match, style.header))
                report.fixed += 1
                print(f"[updated] {display(path)}: year set to {args.year}")
            else:
                report.failures += 1
                print(f"[dated]   {display(path)}: year is not {args.year}")
        elif args.verbose:
            print(f"[ok]      {display(path)}: header present, older year")
        return

    if status is Status.MISSING:
        if args.fix:
            write_text(path, insert_header(content, style.header))
            report.fixed += 1
            print(f"[added]   {display(path)}: inserted {style.name} header")
        else:
            report.failures += 1
            print(f"[missing] {display(path)}: no copyright header")
        return

    # Never rewrite a header we did not author, and never stack a second one on
    # top of a hand-edited block. Report and let a human decide.
    label = "variant" if status is Status.VARIANT else status.name.lower()
    if status is Status.EMPTY:
        if args.verbose:
            print(f"[skip]    {display(path)}: empty file")
        return
    if args.strict:
        report.failures += 1
        print(f"[{label:7}] {display(path)}: {status.value}, left untouched")
    elif args.verbose:
        print(f"[skip]    {display(path)}: {status.value}")


def parse_args(argv: list[str] | None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        "paths",
        nargs="*",
        help=f"files or directories to scan (default: {', '.join(DEFAULT_ROOTS)})",
    )
    parser.add_argument(
        "--fix",
        action="store_true",
        help="write the missing headers instead of only reporting them",
    )
    parser.add_argument(
        "--update-year",
        action="store_true",
        help="also normalise headers whose year range is not the target one",
    )
    parser.add_argument(
        "--year",
        default=DEFAULT_YEAR,
        help=f"year range to write into new headers (default: {DEFAULT_YEAR})",
    )
    parser.add_argument(
        "--strict",
        action="store_true",
        help="also fail on hand-edited and third-party headers",
    )
    parser.add_argument(
        "--no-git",
        action="store_true",
        help="do not ask git which files are ignored; check every file found",
    )
    parser.add_argument(
        "-v", "--verbose", action="store_true", help="list skipped files too"
    )
    return parser.parse_args(argv)


def resolve_roots(paths: list[str]) -> list[Path]:
    roots = []
    for raw in paths or list(DEFAULT_ROOTS):
        path = Path(raw)
        resolved = (path if path.is_absolute() else REPO_ROOT / path).resolve()
        if not resolved.exists():
            print(f"[warn]    {display(resolved)}: does not exist, skipped")
            continue
        roots.append(resolved)
    return roots


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    source, script = make_styles(args.year)
    report = Report.empty()

    mode = "fixing" if args.fix else "checking"
    print(f"Copyright {mode} (target year {args.year})")

    seen: set[Path] = set()
    for root in resolve_roots(args.paths):
        candidates = [p for p in iter_files(root, source, script) if p not in seen]
        ignored = set() if args.no_git else git_ignored(root, candidates)
        for path in candidates:
            seen.add(path)
            if path in ignored:
                # Generated artifacts such as the cbindgen header: writing into
                # them would be undone by the next build.
                report.counts[Status.IGNORED] += 1
                if args.verbose:
                    print(f"[skip]    {display(path)}: ignored by git")
                continue
            style = style_for(path, source, script)
            assert style is not None
            process(path, style, args, report)

    counts = report.counts
    print(
        f"\nScanned {len(seen)} files: "
        f"{counts[Status.CURRENT]} current, "
        f"{counts[Status.DATED]} with an older year, "
        f"{counts[Status.VARIANT]} hand-edited, "
        f"{counts[Status.FOREIGN]} third-party, "
        f"{counts[Status.MISSING]} without a header, "
        f"{counts[Status.IGNORED]} ignored by git"
    )
    if report.fixed:
        print(f"Wrote {report.fixed} files.")

    if report.failures:
        print(f"FAILED: {report.failures} files need attention.")
        return 1
    print("OK: every file carries a copyright header.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
