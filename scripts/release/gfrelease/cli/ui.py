# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""How gfrelease talks to whoever runs it.

stdout carries what a command did or found (the signing host's steps print
one JSON document there). stderr carries the problems, each starting
"error:" or "warning:", the first naming the first problem. -v prints each
tool gfrelease runs (rclone, git, gpg, cosign, minisign) on stderr as it
starts, prefixed "+".
"""

from __future__ import annotations

import logging
import os
import shlex
import sys

from .. import report


class _Shown(logging.Handler):
    """What the roles report (report.py): a warning on stderr, a line of
    progress on stdout."""

    def emit(self, record: logging.LogRecord) -> None:
        if record.levelno >= logging.WARNING:
            warn(record.getMessage())
        else:
            info(record.getMessage())


_SHOWN = _Shown()


def configure(*, verbose: bool = False) -> None:
    if _SHOWN not in report.log.handlers:
        report.log.addHandler(_SHOWN)
        report.log.setLevel(logging.INFO)
    if verbose:
        # Read by tool_started() and the store's own streams, and by every
        # gfrelease this one starts.
        os.environ["GFR_VERBOSE"] = "1"


def info(text: str = "") -> None:
    print(text)


def _lines(prefix: str, text: str) -> None:
    for i, line in enumerate(text.splitlines() or [""]):
        print((prefix if i == 0 else "  ") + line, file=sys.stderr)


def warn(text: str) -> None:
    _lines("warning: ", text)


def error(text: str) -> None:
    _lines("error: ", text)


def tool_started(cmd) -> None:
    if os.environ.get("GFR_VERBOSE"):
        print("+ " + shlex.join(str(c) for c in cmd), file=sys.stderr)
