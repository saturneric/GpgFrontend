# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""Puts the release tooling (the gfrelease package, in scripts/release/) and
these tests' helpers on the path. Every test imports it first."""

import sys
from pathlib import Path

TESTS = Path(__file__).resolve().parent
ROOT = TESTS.parent.parent
GFRELEASE = ROOT / "scripts" / "release" / "gfrelease"

for _path in (TESTS, GFRELEASE.parent):
    if str(_path) not in sys.path:
        sys.path.insert(0, str(_path))
