# Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""Authenticode for the stable Windows installers, by osslsigncode.

Signing happens on the maintainer's machine, with the certificate's key on a
hardware token (PKCS#11) or, for a rehearsal, in a file. Verifying checks the
signature and its chain, and that the signer is exactly the subject the
product pins (release/product.json, `signing.authenticode_subject`): a valid
signature by any other certificate is not ours. osslsigncode prints the
subject as "/CN=.../O=.../C=..." or, in other versions, as RFC 2253
"C=...,O=...,CN=..." (last RDN first); the pin may be written either way,
and the two are compared RDN by RDN, in certificate order.
"""

from __future__ import annotations

import re
from dataclasses import dataclass
from pathlib import Path

from common import ReleaseError, run

# The kinds the offline stage signs. A portable zip holds an unsigned exe for
# now; signing inside it would change the archive and is not done yet.
SIGNED_KINDS = ("msi", "msix")


@dataclass
class Signer:
    cert: str                    # certificate (chain) file, or PKCS#11 URI
    key: str                     # key file, or PKCS#11 URI
    pkcs11_module: str = ""      # the token's PKCS#11 module, if any
    timestamp_url: str = ""      # RFC 3161 TSA; the stage's one network call
    description: str = "GpgFrontend"

    def sign(self, src: Path, dst: Path) -> None:
        cmd = ["osslsigncode", "sign", "-h", "sha256", "-n", self.description]
        if self.pkcs11_module:
            cmd += ["-pkcs11module", self.pkcs11_module,
                    "-pkcs11cert", self.cert, "-key", self.key]
        else:
            cmd += ["-certs", self.cert, "-key", self.key]
        if self.timestamp_url:
            cmd += ["-ts", self.timestamp_url]
        if dst.exists():
            dst.unlink()
        run(cmd + ["-in", src, "-out", dst])


def _verified(file: Path, cafile: str | None) -> str:
    """osslsigncode's report on @p file, once it verified the signature."""
    cmd = ["osslsigncode", "verify", "-in", file]
    if cafile:
        cmd += ["-CAfile", cafile]
    out = run(cmd, check=False)
    text = out.stdout + out.stderr
    if out.returncode != 0 or "Signature verification: ok" not in text:
        raise ReleaseError(f"{file.name}: no valid Authenticode signature")
    return text


def _subject_in(text: str, file: Path) -> str:
    # The first subject after "Signer's certificate:" is the signer's; the
    # chain osslsigncode prints later repeats it among others.
    m = re.search(r"Signer's certificate:\s*\n(?:[^\n]*\n)*?\s*Subject:\s*([^\n]+)",
                  text)
    if not m:
        raise ReleaseError(f"{file.name}: the signer's subject is unreadable")
    return m.group(1).strip()


def _split_unescaped(s: str, sep: str) -> list[str]:
    return [p.replace("\\" + sep, sep)
            for p in re.split(r"(?<!\\)" + re.escape(sep), s)]


# A backslash escape in a subject value: osslsigncode's "\xC3", RFC 2253's
# "\C3" (both one byte of the UTF-8 encoding), or an escaped character.
_ESCAPE = re.compile(r"\\(?:x([0-9A-Fa-f]{2})|([0-9A-Fa-f]{2})|(.))", re.S)


def _unescape(value: str) -> str:
    """@p value with its escapes decoded, so "W\\xC3\\xBC", "W\\C3\\BC" and
    "Wü" are one value."""
    out = bytearray()
    pos = 0
    for m in _ESCAPE.finditer(value):
        out += value[pos:m.start()].encode("utf-8")
        hexpair = m.group(1) or m.group(2)
        out += bytes.fromhex(hexpair) if hexpair else m.group(3).encode("utf-8")
        pos = m.end()
    out += value[pos:].encode("utf-8")
    try:
        return out.decode("utf-8")
    except UnicodeDecodeError:
        raise ReleaseError(f"certificate subject value {value!r} is not UTF-8")


def _subject_rdns(subject: str) -> list[tuple[str, str]]:
    """@p subject as (attribute, value) pairs in certificate order, from
    either the "/CN=a/O=b" or the RFC 2253 "O=b,CN=a" form."""
    s = subject.strip()
    if s.startswith("/"):
        parts = _split_unescaped(s[1:], "/")
    else:
        parts = list(reversed(_split_unescaped(s, ",")))
    rdns = []
    for part in parts:
        attr, eq, value = part.strip().partition("=")
        if not eq or not attr.strip():
            raise ReleaseError(f"unreadable certificate subject {subject!r}")
        rdns.append((attr.strip().upper(), _unescape(value.strip())))
    return rdns


def verify(file: Path, subject: str | list[str],
           cafile: str | None = None, require_timestamp: bool = False) -> None:
    """@p file is validly signed by a certificate with @p subject, or one of
    them: the current certificate, and those it replaced, for what they
    signed before. With @p require_timestamp, the signature also carries an
    RFC 3161 time stamp: without one it stops verifying the day the
    certificate expires, and the release with it."""
    subjects = [subject] if isinstance(subject, str) else list(subject)
    subjects = [s for s in subjects if s]
    if not subjects:
        raise ReleaseError("no Authenticode subject is pinned")
    text = _verified(file, cafile)
    if require_timestamp and "Timestamp is not available" in text:
        raise ReleaseError(f"{file.name}: its Authenticode signature has no "
                           "time-stamp")
    got = _subject_in(text, file)
    if not any(_subject_rdns(got) == _subject_rdns(s) for s in subjects):
        pinned = subjects[0] if len(subjects) == 1 else subjects
        raise ReleaseError(f"{file.name}: signed by {got!r}, not {pinned!r}")
