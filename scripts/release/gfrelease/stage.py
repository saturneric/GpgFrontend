# Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""CI's only write: a verified release candidate, into the staging store.

The candidate is what the provenance job made and signed
(scripts/release_candidate.py): the deliverables, SHA256SUMS.txt and its
Sigstore bundle. staged.json says which build it is and is uploaded last, so
a candidate without one is incomplete and never promoted. Nothing in staging
is ever served: a signer promotes it, or it expires.
"""

from __future__ import annotations

import re
from pathlib import Path

from common import (BUNDLE, SCHEMA, STAGED, SUMS, Product, ReleaseError,
                    canonical_json, load_json, parse_manifest, parse_version,
                    semver)
from store import Store


def staging_prefix(ref: str, filesafe: str) -> str:
    if ref.startswith("refs/heads/"):
        branch = ref[len("refs/heads/"):]
        if not re.match(r"^(main|release/\d+\.\d+)$", branch):
            raise ReleaseError(f"{ref} does not publish builds")
        return f"branches/{branch.replace('/', '-')}/{filesafe}"
    if ref.startswith("refs/tags/") and semver.RELEASE_TAG.match(ref[10:]):
        return f"tags/{filesafe}"
    raise ReleaseError(f"{ref} does not publish builds")


def stage(product: Product, candidate: Path, staging: Store, *, ref: str,
          commit: str, version: str, filesafe: str, built_at: str) -> str:
    if not re.fullmatch(r"[0-9a-f]{40}", commit):
        raise ReleaseError(f"not a commit: {commit}")
    if parse_version(version).filesafe != filesafe:
        raise ReleaseError(f"{filesafe} is not the file-safe {version}")

    # The candidate as the provenance job left it: its own manifest, every
    # file listed, nothing else.
    manifest = parse_manifest((candidate / SUMS).read_bytes(), str(candidate))
    present = {p.name for p in candidate.iterdir() if p.is_file()}
    if present != set(manifest) | {SUMS, BUNDLE}:
        raise ReleaseError(f"{candidate} is not a release candidate: "
                           f"{sorted(present ^ (set(manifest) | {SUMS, BUNDLE}))}")
    for name in manifest:
        product.classify(name, filesafe)  # fail here, not on the signing host

    prefix = staging_prefix(ref, filesafe)
    # A re-run of the whole workflow signs the manifest again, and the new
    # Sigstore bundle never has the old one's bytes. The build staged there
    # already is the same commit's: the candidate is complete, done.
    if staging.exists(f"{prefix}/{STAGED}"):
        existing = read_staged(staging.read(f"{prefix}/{STAGED}"))
        if existing["commit"] != commit:
            raise ReleaseError(f"{prefix} already holds {existing['commit']}, "
                               f"not {commit}")
        return prefix
    # No staged.json yet: whatever is under the prefix is an earlier attempt
    # that never completed (nothing reads a candidate without staged.json),
    # and is replaced; staged.json, written last, is what makes it one.
    for name in sorted(manifest) + [SUMS, BUNDLE]:
        staging.put_file(candidate / name, f"{prefix}/{name}",
                         immutable=False)
    staged = {"schema": SCHEMA, "product": product.slug, "ref": ref,
              "commit": commit, "version": version, "filesafe": filesafe,
              "built_at": built_at, "files": sorted(manifest)}
    staging.put_bytes(canonical_json(staged), f"{prefix}/{STAGED}")
    return prefix


def read_staged(data: bytes) -> dict:
    staged = load_json(data, STAGED)
    for key in ("schema", "product", "ref", "commit", "version", "filesafe",
                "built_at", "files"):
        if key not in staged:
            raise ReleaseError(f"staged.json lacks {key}")
    if staged["schema"] != SCHEMA:
        raise ReleaseError(f"staged.json schema {staged['schema']} is unknown")
    # It comes from the untrusted staging bucket: every field promote reads
    # has the type it reads it as.
    for key in ("product", "ref", "version", "filesafe", "built_at"):
        if not isinstance(staged[key], str):
            raise ReleaseError(f"staged.json: {key} is not a string")
    if not staged["ref"].startswith("refs/"):
        raise ReleaseError(f"staged.json: not a ref: {staged['ref']!r}")
    parse_version(staged["version"], STAGED)
    if not isinstance(staged["commit"], str) or \
            not re.fullmatch(r"[0-9a-f]{40}", staged["commit"]):
        raise ReleaseError(f"staged.json: not a commit: {staged['commit']!r}")
    if not isinstance(staged["files"], list) or \
            not all(isinstance(f, str) for f in staged["files"]):
        raise ReleaseError("staged.json: files is not a list of names")
    return staged
