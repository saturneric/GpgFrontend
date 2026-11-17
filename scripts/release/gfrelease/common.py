# Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""What every gfrelease role shared, now each in a module of its own:
handed on from there until the last of its users imports it there."""

# flake8: noqa: F401
from .adapters import git as gitversion
from .adapters.clock import utcnow
from .adapters.files import atomic_write, locked, sha256_file
from .adapters.git import (check_git_version, git_version, is_ancestor,
                           notes_at, release_tags, resolve_commit, tag_time,
                           tagged_commit)
from .adapters.process import TOOL_TIMEOUT, WINDOWS, run, run_with_secret
from .app import candidate as rc
from .app.candidate import (candidate_error, check_candidate,
                            verify_candidate)
from .app.product import Product
from .domain import policy as policy_mod
from .domain import refs, semver
from .domain.formats import (SCHEMA, canonical_json, check_relpath, iso,
                             load_json, parse_iso, parse_version,
                             sha256_bytes)
from .domain.layout import (BUNDLE, DESCRIPTOR, INDEX, MINISIG, SIG, STAGED,
                            SUMS)
from .domain.limits import EXPIRY_SKEW, NOTES_LIMIT, SMALL_OBJECT_LIMIT
from .domain.manifest import parse_manifest, render_manifest
from .errors import (Problems, ReleaseError, ToolFailed, ToolTimeout,
                     is_passing)
from .paths import RELEASE_DIR, ROOT
