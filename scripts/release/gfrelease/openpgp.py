# Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""OpenPGP for stable releases, by gpg itself.

Signing happens on the maintainer's machine with their own key (a signing
subkey is fine): detached, binary `.sig` files, the form every release has
carried. Verifying happens anywhere, against nothing but the maintainers'
public keys: a throwaway keyring is made from release/maintainers.asc for
each check, and a signature counts only when gpg calls it valid (VALIDSIG)
and the key it chains to has a primary fingerprint the product pins. Trust
settings, the caller's own keyring and an agent play no part.
"""

from __future__ import annotations

import os
import tempfile
from pathlib import Path

from common import ReleaseError, run


def sign(file: Path, fingerprint: str, gnupghome: str | None = None) -> Path:
    """`file.sig` beside @p file, made with the key @p fingerprint names."""
    sig = file.with_name(file.name + ".sig")
    cmd = ["gpg", "--batch", "--yes", "--detach-sign",
           "--local-user", f"{fingerprint}!", "--output", sig, file]
    if gnupghome:
        cmd[1:1] = ["--homedir", gnupghome]
    run(cmd)
    return sig


def _validsig_primary(status: str) -> str | None:
    """The primary key fingerprint of a VALIDSIG status line, if any."""
    for line in status.splitlines():
        parts = line.split()
        if len(parts) >= 3 and parts[0] == "[GNUPG:]" and parts[1] == "VALIDSIG":
            # VALIDSIG <fpr> <date> <ts> <expire> <ver> <rsv> <algo> <hash>
            #          <class> [<primary-fpr>]
            return parts[11] if len(parts) > 11 else parts[2]
    return None


class Keyring:
    """The maintainers' public keys, imported fresh into a throwaway home."""

    def __init__(self, keys: Path, pinned: list[str]):
        if not Path(keys).is_file():
            raise ReleaseError(f"{keys}: no maintainer keys to verify against")
        if not pinned:
            raise ReleaseError("no maintainer fingerprint is pinned")
        self.keys = Path(keys)
        self.pinned = {f.upper() for f in pinned}

    def __enter__(self) -> "Keyring":
        self._tmp = tempfile.TemporaryDirectory(prefix="gfr-gpg-")
        self.home = self._tmp.name
        os.chmod(self.home, 0o700)
        run(["gpg", "--homedir", self.home, "--batch", "--quiet",
             "--import", self.keys])
        return self

    def __exit__(self, *exc) -> None:
        run(["gpgconf", "--homedir", self.home, "--kill", "all"], check=False)
        self._tmp.cleanup()

    def _accept(self, status: str, what: str) -> str:
        primary = _validsig_primary(status)
        if primary is None:
            raise ReleaseError(f"{what}: no valid signature by a maintainer key")
        if primary.upper() not in self.pinned:
            raise ReleaseError(f"{what}: signed by {primary}, which is not a "
                               "pinned maintainer key")
        return primary.upper()

    def verify(self, data: Path, asc: Path) -> str:
        """The pinned primary fingerprint that signed @p data, or an error."""
        out = run(["gpg", "--homedir", self.home, "--batch", "--status-fd", "1",
                   "--verify", asc, data], check=False)
        if out.returncode != 0:
            raise ReleaseError(f"{Path(data).name}: the signature does not verify")
        return self._accept(out.stdout, Path(data).name)

    def verify_tag(self, git_dir: Path, tag: str) -> str:
        """A signed annotated tag, by a pinned maintainer key."""
        env = dict(os.environ, GNUPGHOME=self.home)
        out = run(["git", "-C", git_dir, "verify-tag", "--raw", tag], env=env,
                  check=False)
        if out.returncode != 0:
            raise ReleaseError(f"tag {tag} is not signed by a maintainer key")
        return self._accept(out.stderr, f"tag {tag}")
