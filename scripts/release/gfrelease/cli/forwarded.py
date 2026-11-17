# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""The commands with a command line of their own: policy, candidate,
ci check-order and ci version, handed everything after their name."""

from __future__ import annotations

import sys

from ..adapters import git as gitversion_mod
from ..app import candidate as candidate_mod
from ..app import check_order as check_order_mod
from ..domain import policy as policy_mod

# Commands with a command line of their own, given everything after their
# name: they need no product, settings or store.
FORWARDED = {
    "policy": lambda argv: policy_mod.main(argv),
    "candidate": lambda argv: candidate_mod.main(argv),
    "ci check-order": lambda argv: check_order_mod.main(argv),
    "ci version": lambda argv: gitversion_mod.main(argv),
}


def forwarded(argv: list[str]):
    """The forwarded command @p argv starts with, and the rest of it."""
    for words in (2, 1):
        name = " ".join(argv[:words])
        if len(argv) >= words and name in FORWARDED:
            return FORWARDED[name], argv[words:]
    if argv[:1] == ["ci"]:
        print("usage: gfrelease ci {check-order,version} ...",
              file=sys.stderr)
        return None, None
    return None, argv
