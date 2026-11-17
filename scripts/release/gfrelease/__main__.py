# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""Run as scripts/gfrelease, python3 scripts/release/gfrelease, or
python3 -m gfrelease with scripts/release/ on the path."""

import sys
from pathlib import Path

if __package__ in (None, ""):
    # Run as a directory: the package is imported from beside it.
    sys.path[0] = str(Path(__file__).resolve().parent.parent)

from gfrelease.cli.main import main  # noqa: E402

sys.exit(main())
