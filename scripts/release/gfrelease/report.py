# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""What a role says as it goes, besides what it returns: a warning (what a
run went on without, or replaced) or a line of progress. The command line
shows them (cli/ui.py); a caller that installs nothing hears nothing."""

from __future__ import annotations

import logging

log = logging.getLogger("gfrelease")
log.addHandler(logging.NullHandler())
log.propagate = False


def warn(text: str) -> None:
    log.warning(text)


def info(text: str) -> None:
    log.info(text)
