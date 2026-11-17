# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""How much of anything is read, how long anything is valid, and how far a
clock may be off: the limits the update check (modules/src/m_ver_check)
holds to as well. scripts/tests/fixtures/contract/limits.json says them
for the update check's tests, which compare them with its own."""

from __future__ import annotations

import datetime as dt

# The most a descriptor, an index, a manifest, a signature or release notes
# may weigh. They are read into memory whole, and come from buckets and
# mirrors others can write to.
SMALL_OBJECT_LIMIT = 8 << 20
# Release notes: what the update check reads of them at most
# (UpdateChecker::kMaxNotesBytes); longer ones it would drop.
NOTES_LIMIT = 256 << 10
# How far a clock may be off before something has expired: the update
# check allows as much (kExpirySkewSeconds).
EXPIRY_SKEW = dt.timedelta(days=1)
# How long the signing host signs an index for.
INDEX_LIFETIME = dt.timedelta(days=14)
# The longest an index may be valid for, from when it was made: what the
# update check allows (kMaxIndexLifetimeSeconds).
MAX_INDEX_LIFETIME = dt.timedelta(days=60)
# The most keys a role may name: a client fetches a signature for each root
# key before it checks anything (RootTrust.h's kMaxRootKeys).
MAX_ROLE_KEYS = 16
# Roots taken from one source in one go (RootTrust.h's kMaxRootSteps).
MAX_ROOT_STEPS = 32


def as_json() -> dict:
    """The limits, in the units the update check spells them in."""
    return {"max_fetch_bytes": SMALL_OBJECT_LIMIT,
            "max_notes_bytes": NOTES_LIMIT,
            "expiry_skew_seconds": int(EXPIRY_SKEW.total_seconds()),
            "index_lifetime_seconds": int(INDEX_LIFETIME.total_seconds()),
            "max_index_lifetime_seconds":
                int(MAX_INDEX_LIFETIME.total_seconds()),
            "max_root_keys": MAX_ROLE_KEYS,
            "max_root_steps": MAX_ROOT_STEPS}
