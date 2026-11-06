# Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""minisign, by its own command: signing with a key file, verifying against
a public key given as its base64 line.

Keys used unattended are made without a password (`minisign -G -W`) and
protected as files: Docker secrets readable by the signing host's user only.
minisign cannot take a password from anywhere but a terminal.
"""

from __future__ import annotations

import base64
from pathlib import Path

from common import ReleaseError, run


def key_id(public_key: str) -> str:
    """The 8-byte key id, as minisign prints it."""
    raw = base64.b64decode(public_key)
    if len(raw) != 42 or raw[:2] != b"Ed":
        raise ReleaseError("not a minisign Ed25519 public key")
    return raw[2:10][::-1].hex().upper()


def public_key_of(path: Path) -> str:
    """The base64 line of a .pub file."""
    lines = [l for l in Path(path).read_text().splitlines()
             if l and not l.startswith("untrusted comment:")]
    if len(lines) != 1:
        raise ReleaseError(f"{path}: not a minisign public key file")
    key_id(lines[0])
    return lines[0]


def sign(secret_key: Path, file: Path, trusted_comment: str) -> Path:
    if "\n" in trusted_comment:
        raise ReleaseError("a trusted comment is one line")
    sig = file.with_name(file.name + ".minisig")
    run(["minisign", "-S", "-s", secret_key, "-m", file, "-x", sig,
         "-t", trusted_comment], input="")
    return sig


def trusted_comment(sig: bytes) -> str:
    lines = sig.decode("utf-8").splitlines()
    if len(lines) < 4 or not lines[2].startswith("trusted comment: "):
        raise ReleaseError("not a minisign signature")
    return lines[2][len("trusted comment: "):]


def verify(public_keys: list[str], file: Path, sig: Path) -> str:
    """Verifies file against any of the keys (current and next); returns the
    trusted comment, whose signature minisign has then checked as well."""
    if not public_keys:
        raise ReleaseError("no public key to verify against")
    errors = []
    for key in public_keys:
        out = run(["minisign", "-V", "-q", "-P", key, "-m", file, "-x", sig],
                  check=False)
        if out.returncode == 0:
            return trusted_comment(Path(sig).read_bytes())
        errors.append((out.stderr or out.stdout).strip())
    raise ReleaseError(f"{Path(file).name}: no pinned key verifies it "
                       f"({'; '.join(errors)})")
