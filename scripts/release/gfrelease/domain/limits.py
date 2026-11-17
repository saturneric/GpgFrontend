# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""How much of anything is read, and how far a clock may be off: limits the
update check (modules/src/m_ver_check) holds to as well."""

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
