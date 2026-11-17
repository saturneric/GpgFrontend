# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""What time it is: now, or GFR_NOW for tests and rehearsals."""

from __future__ import annotations

import datetime as dt
import os


def utcnow() -> dt.datetime:
    override = os.environ.get("GFR_NOW")  # tests and rehearsals only
    if override:
        t = dt.datetime.fromisoformat(override)
        # Without a zone it is UTC, never this machine's local time.
        if t.tzinfo is None:
            t = t.replace(tzinfo=dt.timezone.utc)
        return t.astimezone(dt.timezone.utc)
    return dt.datetime.now(dt.timezone.utc).replace(microsecond=0)
