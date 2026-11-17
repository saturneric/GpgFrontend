# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""cosign: whether a candidate's manifest carries the Sigstore signature
of the product's build workflow, at one commit. What it decides is a
CandidateError (or CandidateTimeout: nothing decided)."""

from __future__ import annotations

import subprocess

from ..domain.manifest import (BUNDLE, MANIFEST, CandidateError,
                               CandidateTimeout)

ISSUER = "https://token.actions.githubusercontent.com"
# cosign checks a bundle against a transparency log it may have to reach.
COSIGN_TIMEOUT = 600


def verify_blob(directory, identity, sha, cosign):
    """The Sigstore identity: this workflow, this commit, GitHub's issuer."""
    if not identity or not sha:
        raise CandidateError("an exact identity and workflow sha are required")
    for name in (MANIFEST, BUNDLE):
        path = directory / name
        if path.is_symlink() or not path.is_file():
            raise CandidateError(f"missing {name}")

    cmd = [
        cosign, "verify-blob",
        "--bundle", str(directory / BUNDLE),
        "--certificate-identity", identity,
        "--certificate-oidc-issuer", ISSUER,
        "--certificate-github-workflow-sha", sha,
        str(directory / MANIFEST),
    ]
    try:
        # Captured: its "Verified OK" says nothing the exit status does
        # not, and whoever reads this tool's stderr takes it for a
        # problem. What it says when it refuses goes into the error.
        out = subprocess.run(cmd, timeout=COSIGN_TIMEOUT,
                             capture_output=True, encoding="utf-8",
                             errors="replace")
    except FileNotFoundError:
        raise CandidateError(f"{cosign} is not installed")
    except subprocess.TimeoutExpired:
        raise CandidateTimeout(f"{cosign} did not finish within "
                               f"{COSIGN_TIMEOUT} s")
    if out.returncode != 0:
        said = (out.stderr or out.stdout or "").strip().splitlines()
        raise CandidateError(f"{BUNDLE} does not verify {MANIFEST} "
                             f"as {identity} at {sha}"
                             + (f" (cosign: {said[-1][:300]})" if said else ""))
