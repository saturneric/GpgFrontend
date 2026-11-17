# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""How gfrelease talks to whoever runs it.

stdout carries what a command did or found: readable text by default,
one JSON document with --json. stderr carries the problems, each line
starting "error:" or "warning:", the first line naming the first problem
(what the signing host's container reads), and, where the tool knows how
to fix one, a "hint:" line after it. A command that leaves something to do
ends with one "next:" line saying what.

-v prints each tool gfrelease runs (rclone, git, gpg, cosign, minisign) on
stderr as it starts, prefixed "+"; -q keeps only the errors.
"""

from __future__ import annotations

import json
import logging
import os
import sys

from .. import report
from ..app.hints import hint, install_hint  # noqa: F401  (ui.install_hint)

JSON = False
QUIET = False


class _Shown(logging.Handler):
    """What the roles report (report.py), shown as this module shows
    everything: a warning on stderr, a line of progress on stdout."""

    def emit(self, record: logging.LogRecord) -> None:
        if record.levelno >= logging.WARNING:
            warn(record.getMessage())
        else:
            info(record.getMessage())


_SHOWN = _Shown()


def configure(*, json_out: bool = False, quiet: bool = False,
              verbose: bool = False) -> None:
    global JSON, QUIET
    JSON, QUIET = json_out, quiet
    if _SHOWN not in report.log.handlers:
        report.log.addHandler(_SHOWN)
        report.log.setLevel(logging.INFO)
    if verbose:
        # Read by tool_started() (every tool adapters/process.py runs)
        # and the store's own streams, and by every gfrelease this one
        # starts.
        os.environ["GFR_VERBOSE"] = "1"


def info(text: str = "") -> None:
    """A line of what was done or found; nothing with --json or -q."""
    if not JSON and not QUIET:
        print(text)


def next_step(text: str) -> None:
    info(f"next: {text}")


def warn(text: str) -> None:
    if not QUIET:
        for i, line in enumerate(text.splitlines() or [""]):
            print(("warning: " if i == 0 else "  ") + line, file=sys.stderr)


def error(text: str) -> None:
    for i, line in enumerate(text.splitlines() or [""]):
        print(("error: " if i == 0 else "  ") + line, file=sys.stderr)
    found = hint(text)
    if found:
        print(f"hint: {found}", file=sys.stderr)


def emit(document) -> None:
    """The command's result, with --json."""
    if JSON:
        print(json.dumps(document, indent=2, sort_keys=True))


def tool_started(cmd) -> None:
    if os.environ.get("GFR_VERBOSE"):
        import shlex
        print("+ " + shlex.join(str(c) for c in cmd), file=sys.stderr)
