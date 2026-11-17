# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""What goes wrong, and whether trying again may help: the one module every
layer imports."""

from __future__ import annotations


class ReleaseError(Exception):
    pass


class ToolFailed(ReleaseError):
    """A tool could not do its job (git, cmake, rclone, gpg importing keys,
    minisign signing): nothing was decided about what it worked on, and the
    next run may well succeed. Unlike a verification that ran and said no,
    which is a ReleaseError."""

    passing = True  # is_passing


class ToolTimeout(ToolFailed):
    """A tool did not finish within its time limit."""


class Problems(list):
    """What a run could not do, one line each, and whether trying again may
    help: a problem is passing when it came from a store that did not
    answer or a tool that did not finish, lasting when the thing itself was
    refused (a forged candidate, a bad signature), which the next run would
    only refuse again."""

    passing = False

    def note(self, where: str, error: Exception) -> None:
        text = str(error)
        # Most errors name their place already: never twice.
        head, sep, rest = text.partition(": ")
        if sep and (head == where or where.endswith("/" + head)):
            text = rest
        self.append(f"{where}: {text}")
        if is_passing(error):
            self.passing = True


def is_passing(error: Exception) -> bool:
    """Not a judgement of what was asked about, only a failure to ask:
    worth trying again as it is (ToolFailed, store.StoreUnavailable)."""
    return getattr(error, "passing", False)
