# Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""What every gfrelease role shares: errors, hashing, configuration, the lock.

Being taken apart: what has moved is handed on from where it is now."""

from __future__ import annotations

import re
from pathlib import Path

from .adapters import git as gitversion
from .app import candidate as rc
from .cli import ui
from .domain import policy as policy_mod
from .domain import refs, semver
from .paths import RELEASE_DIR, ROOT  # noqa: F401  (others take them here)
from .domain.formats import (SCHEMA, canonical_json, check_relpath, iso,
                              load_json, parse_iso, parse_version,
                              sha256_bytes)
from .domain.limits import EXPIRY_SKEW, NOTES_LIMIT, SMALL_OBJECT_LIMIT
from .errors import (Problems, ReleaseError, ToolFailed, ToolTimeout,
                     is_passing)
from .adapters import process
from .adapters.clock import utcnow
from .adapters.files import atomic_write, locked, sha256_file
from .adapters.git import (check_git_version, git_version, is_ancestor,
                            notes_at, release_tags, resolve_commit, tag_time,
                            tagged_commit)
from .adapters.process import TOOL_TIMEOUT, WINDOWS, run, run_with_secret
from .app.candidate import (candidate_error, check_candidate,
                            verify_candidate)
from .domain.layout import (BUNDLE, DESCRIPTOR, INDEX, MINISIG, SIG, STAGED,
                             SUMS)
from .domain.manifest import parse_manifest, render_manifest

# The command line's --verbose: every tool run is told of (until the command
# line sets this itself).
process.on_start = ui.tool_started


class Product:
    """release/product.json, the policy of the tree it sits in, and the
    trust it is published under: the root and the index-keys list
    (trust.py), from @p root_dir and @p index_keys_dir (by default this
    tree's release/root/ and release/index-keys/), root 1 being
    @p root_sha256 when given."""

    def __init__(self, product_path: Path, policy_path: Path,
                 root_dir: Path | None = None,
                 index_keys_dir: Path | None = None,
                 root_sha256: str | None = None):
        try:
            self.data = policy_mod.load_product(product_path)
            policy_mod.check_product(self.data)
            self.policy = policy_mod.load_policy(policy_path)
            policy_mod.check_policy(self.policy)
        except policy_mod.PolicyError as e:
            raise ReleaseError(f"{product_path}, {policy_path}:\n{e}") from e
        except (OSError, ValueError) as e:
            # Unreadable, or not JSON or TOML at all.
            raise ReleaseError(f"{product_path}, {policy_path}: {e}") from e
        self.slug = self.data["slug"]
        # The maintainers' public keys sit beside the product file; only
        # those whose fingerprint the root pins are ever used.
        self.keyring_path = Path(product_path).with_name("maintainers.asc")
        self.root_dir = Path(root_dir or ROOT / "release" / "root")
        self.index_keys_dir = Path(index_keys_dir or
                                   ROOT / "release" / "index-keys")
        self.root_sha256 = root_sha256
        self._trust = None

    @property
    def trust(self) -> tuple[dict, dict]:
        """The root and the index-keys list, checked and unexpired; read
        when first asked for (staging needs neither)."""
        if self._trust is None:
            from .app import trust  # trust.py builds on this module
            root, keys = trust.load_trust(self.root_dir, self.index_keys_dir,
                                          self.slug, self.root_sha256)
            trust.check_unexpired(root, f"root {root['version']}")
            trust.check_unexpired(keys, f"index-keys {keys['version']}")
            self._trust = root, keys
        return self._trust

    @property
    def host_policy(self) -> dict:
        """What the root says the signing host admits."""
        return self.trust[0]["host_policy"]

    @property
    def repository(self) -> str:
        return self.host_policy["repository"]

    @property
    def maintainers(self) -> list[str]:
        """Who may sign a stable release: their primary fingerprints."""
        return [f.upper() for f in
                self.host_policy["stable_pgp"]["fingerprints"]]

    @property
    def retired_maintainers(self) -> list[str]:
        """A maintainer key replaced: what it signed is still accepted when
        a release admitted before is checked again, never for a new one."""
        return [f.upper() for f in self.host_policy["stable_pgp"]
                .get("fingerprints_retired", []) if f not in self.maintainers]

    @property
    def trusted_maintainers(self) -> list[str]:
        return self.maintainers + self.retired_maintainers

    @property
    def authenticode_subject(self) -> str:
        return self.host_policy.get("authenticode_subject", "")

    @property
    def authenticode_subjects(self) -> list[str]:
        """The current certificate's subject and the replaced ones (accepted
        as the retired maintainers are)."""
        return [s for s in [self.authenticode_subject] +
                self.host_policy.get("authenticode_subjects_retired", [])
                if s]

    @property
    def authenticode_timestamp_required(self) -> bool:
        # A signature without an RFC 3161 time stamp stops verifying when
        # the certificate expires; required unless the root says otherwise.
        return self.host_policy.get("authenticode_timestamp",
                                    "required") == "required"

    @property
    def index_keys(self) -> list[str]:
        """The keys a client accepts on the index: the index-keys list's."""
        return list(self.trust[1]["keys"].values())

    @property
    def signing_key(self) -> str:
        """The key that signs anything new by default: the first pinned
        maintainer's, or "" when none is pinned."""
        return self.maintainers[0] if self.maintainers else ""

    @property
    def legacy_floor(self) -> semver.Version:
        """The first release signed with a pinned key: nothing older is
        imported or listed."""
        return parse_version(self.policy["signing"]["legacy_unsigned_before"])

    def check_host_keys(self, index_pub: str) -> None:
        """What this host signs with must be what clients accept, or
        everything it publishes is refused by every client while its own
        read-back, done with the same wrong key, passes."""
        if index_pub not in self.index_keys:
            raise ReleaseError(f"--index-pub is not in index-keys "
                               f"{self.trust[1]['version']}: clients would "
                               "refuse what this host signs")

    def identity(self, ref: str) -> str:
        """The Sigstore identity of this product's build workflow at a ref."""
        return (f"https://github.com/{self.repository}/"
                f"{self.host_policy['workflow']}@{ref}")

    def classify(self, name: str, filesafe: str) -> dict:
        """os, arch, kind, flavor (and min_os) of a published file, from the
        product's own table; a file it does not know is refused, so nothing
        reaches a descriptor undescribed."""
        prefix = f"{self.data['artifact_prefix']}{filesafe}-"
        if not name.startswith(prefix):
            raise ReleaseError(f"{name}: not named {prefix}...")
        rest = name[len(prefix):]
        for entry in self.data["artifacts"]:
            m = re.fullmatch(entry["match"], rest, re.ASCII)
            if not m:
                continue
            groups = m.groupdict()
            out = {k: entry[k].format(**groups)
                   for k in ("os", "arch", "kind", "flavor")}
            if "min_os" in entry:
                out["min_os"] = {k: v.format(**groups)
                                 for k, v in entry["min_os"].items()}
            return out
        raise ReleaseError(f"{name}: no artifact rule in release/product.json")


