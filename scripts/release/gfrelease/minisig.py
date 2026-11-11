# Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""minisign, by its own command: signing with a key file, verifying against
a public key given as its base64 line.

Keys used unattended are made without a password (`minisign -G -W`) and
protected as files: Docker secrets readable by the signing host's user only.
minisign cannot take a password from anywhere but a terminal.

A secret key is read exactly once, here, and handed to minisign through a
pipe: a key given as `<(bw get notes ...)` is a /dev/fd/N path that can be
read only once and that a subprocess would not inherit.
"""

from __future__ import annotations

import base64
import os
import tempfile
from pathlib import Path

from common import MINISIG, ReleaseError, run


def _id(raw: bytes) -> str:
    """The key id in a decoded key or signature (bytes 2-10, stored little
    endian), as minisign prints it."""
    return raw[2:10][::-1].hex().upper()


def key_id(public_key: str) -> str:
    """The 8-byte key id, as minisign prints it."""
    try:
        raw = base64.b64decode(public_key, validate=True)
    except (TypeError, ValueError) as e:
        raise ReleaseError("not a minisign Ed25519 public key") from e
    if len(raw) != 42 or raw[:2] != b"Ed":
        raise ReleaseError("not a minisign Ed25519 public key")
    return _id(raw)


def public_key_of(path: Path) -> str:
    """The base64 line of a .pub file."""
    lines = [l for l in Path(path).read_text().splitlines()
             if l and not l.startswith("untrusted comment:")]
    if len(lines) != 1:
        raise ReleaseError(f"{path}: not a minisign public key file")
    key_id(lines[0])
    return lines[0]


def sign(secret_key: Path, file: Path, trusted_comment: str,
         sig: Path | None = None) -> Path:
    """@p file signed into @p sig (default: beside it). A key with a
    password makes minisign ask for it on the terminal."""
    if "\n" in trusted_comment:
        raise ReleaseError("a trusted comment is one line")
    sig = sig or file.with_name(file.name + MINISIG)
    secret = Path(secret_key).read_bytes()
    r, w = os.pipe()
    try:
        os.write(w, secret)  # a key is a few hundred bytes: never blocks
        os.close(w)
        w = None
        run(["minisign", "-S", "-s", f"/dev/fd/{r}", "-m", file, "-x", sig,
             "-t", trusted_comment], pass_fds=(r,), capture_output=False)
    finally:
        os.close(r)
        if w is not None:
            os.close(w)
    return sig


def prehashed_key_id(sig: bytes) -> str | None:
    """The key id of a prehashed ("ED") signature, as key_id() spells it;
    None for a legacy ("Ed") one or anything that is not a signature."""
    lines = sig.split(b"\n")
    try:
        raw = base64.b64decode(lines[1].strip(), validate=True)
    except (IndexError, ValueError):
        return None
    if len(raw) != 74 or raw[:2] != b"ED":
        return None
    return _id(raw)


def trusted_comment(sig: bytes) -> str:
    """The trusted comment (line 3). The untrusted one (line 1) is anyone's
    to write and is never decoded."""
    lines = sig.split(b"\n")
    prefix = b"trusted comment: "
    if len(lines) < 4 or not lines[2].startswith(prefix):
        raise ReleaseError("not a minisign signature")
    try:
        return lines[2][len(prefix):].rstrip(b"\r").decode("utf-8")
    except UnicodeDecodeError as e:
        raise ReleaseError("a minisign trusted comment that is not UTF-8") \
            from e


def verify(public_keys: list[str], file: Path, sig: Path) -> str:
    """Verifies file against any of the keys; returns the trusted comment,
    whose signature minisign has then checked as well. Only a prehashed
    ("ED") signature verifies (-H): every one this tooling makes is."""
    if not public_keys:
        raise ReleaseError("no public key to verify against")
    errors = []
    for key in public_keys:
        out = run(["minisign", "-V", "-H", "-q", "-P", key, "-m", file,
                   "-x", sig], check=False)
        if out.returncode == 0:
            return trusted_comment(Path(sig).read_bytes())
        errors.append((out.stderr or out.stdout).strip())
    raise ReleaseError(f"{Path(file).name}: no pinned key verifies it "
                       f"({'; '.join(errors)})")


def verify_bytes(public_keys: list[str], data: bytes, sig: bytes,
                 name: str = "data") -> str:
    """verify() for what is held in memory: @p data and @p sig are written
    to a private directory first. @p name is what an error calls them."""
    with tempfile.TemporaryDirectory(prefix="gfr-minisig-") as tmp:
        file = Path(tmp) / Path(name).name
        file.write_bytes(data)
        sig_file = file.with_name(file.name + MINISIG)
        sig_file.write_bytes(sig)
        return verify(public_keys, file, sig_file)


def generate(secret: Path, password: bool = False) -> str:
    """A new key pair, @p secret and @p secret.pub beside it; returns the
    public key. Without @p password it is made with -W, for a key used
    unattended; with one, minisign asks for it on the terminal."""
    secret = Path(secret)
    pub = secret.with_name(secret.name + ".pub")
    if secret.exists() or pub.exists():
        raise ReleaseError(f"{secret} exists")
    run(["minisign", "-G", *(() if password else ("-W",)), "-s", secret,
         "-p", pub], capture_output=not password)
    return public_key_of(pub)
