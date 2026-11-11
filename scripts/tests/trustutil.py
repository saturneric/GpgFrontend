# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""What the release tooling's tests share: the tools they need, a git that
reads no one's configuration, and minisign keys and roots made on the
spot."""

from __future__ import annotations

import os
import shutil
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent.parent
GFRELEASE = ROOT / "scripts" / "release" / "gfrelease"
sys.path.insert(0, str(GFRELEASE))

import layout  # noqa: E402
import minisig  # noqa: E402
import trust  # noqa: E402


def tools(*names: str) -> bool:
    """Whether every command is installed. With GF_REQUIRE_RELEASE_TOOLS=1
    (CI's setup job) a missing one fails the run instead of skipping the
    tests that need it: a chain test skipped in CI is a chain untested."""
    listed = {line.split()[0] for line in
              (ROOT / "scripts" / "release" / "tools.txt").read_text()
              .splitlines() if line.strip() and not line.startswith("#")}
    unlisted = set(names) - listed - {"gpgconf"}
    if unlisted:
        raise AssertionError(f"not in scripts/release/tools.txt: {unlisted}")
    missing = [n for n in names if not shutil.which(n)]
    if missing and os.environ.get("GF_REQUIRE_RELEASE_TOOLS") == "1":
        raise AssertionError(f"GF_REQUIRE_RELEASE_TOOLS=1, but {missing} "
                             "are not installed")
    return not missing


def git_env() -> dict:
    """The environment for git in a test: no CI variables, no global or
    system configuration (commit signing, hooks), a fixed identity."""
    env = {k: v for k, v in os.environ.items()
           if not k.startswith("GITHUB_") and k != "CI"}
    env.update({"GIT_AUTHOR_NAME": "t", "GIT_AUTHOR_EMAIL": "t@example.org",
                "GIT_COMMITTER_NAME": "t",
                "GIT_COMMITTER_EMAIL": "t@example.org",
                "GIT_CONFIG_GLOBAL": os.devnull, "GIT_CONFIG_NOSYSTEM": "1"})
    return env


def gfrelease_env(**extra: str) -> dict:
    """The environment for the gfrelease command line in a test: none of
    the GFR_* settings of whoever runs it (a signing host's anchor or
    product) leak in."""
    env = {k: v for k, v in os.environ.items() if not k.startswith("GFR_")}
    env.update(extra)
    return env


def keypair(directory: Path, name: str) -> tuple[Path, str]:
    """A minisign key pair made without a password: (secret, public key)."""
    secret = Path(directory) / f"{name}.key"
    return secret, minisig.generate(secret)


def write_root(root_dir: Path, doc: dict, secrets, slug: str) -> Path:
    """@p doc written as its <N>.root.json in @p root_dir and signed with
    each of @p secrets, each signature named after its key."""
    n = doc["version"]
    path = Path(root_dir) / layout.root_name(n)
    path.write_bytes(trust.dump(doc))
    tmp = path.with_name(path.name + ".new" + minisig.MINISIG)
    for secret in secrets:
        minisig.sign(secret, path, layout.root_comment(slug, n), tmp)
        key_id = minisig.prehashed_key_id(tmp.read_bytes())
        tmp.replace(path.with_name(layout.root_sig_name(n, key_id)))
    return path
