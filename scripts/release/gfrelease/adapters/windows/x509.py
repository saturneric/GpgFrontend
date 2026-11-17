# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""Certificate subjects: the pinned one as it is written ("/C=../CN=.." or
RFC 2253), as osslsigncode prints one, and as a certificate holds it (DER),
compared as (attribute, value) pairs. authenticode.py checks signers by
them, msix.py names a package's publisher by them."""

from __future__ import annotations

import base64
import re
from pathlib import Path

from ...errors import ReleaseError


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


def subject_rdns(subject: str) -> list[tuple[str, str]]:
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


# --- the certificate itself, where no tool prints its subject ---------------

# The attributes of a subject by the names osslsigncode (OpenSSL) prints
# them under, which is how a pin is written.
_ATTRIBUTES = {
    "2.5.4.3": "CN", "2.5.4.4": "SN", "2.5.4.5": "serialNumber",
    "2.5.4.6": "C", "2.5.4.7": "L", "2.5.4.8": "ST", "2.5.4.9": "street",
    "2.5.4.10": "O", "2.5.4.11": "OU", "2.5.4.12": "title",
    "2.5.4.15": "businessCategory", "2.5.4.17": "postalCode",
    "2.5.4.42": "GN", "2.5.4.97": "organizationIdentifier",
    "1.2.840.113549.1.9.1": "emailAddress",
    "0.9.2342.19200300.100.1.25": "DC",
    "1.3.6.1.4.1.311.60.2.1.1": "jurisdictionL",
    "1.3.6.1.4.1.311.60.2.1.2": "jurisdictionST",
    "1.3.6.1.4.1.311.60.2.1.3": "jurisdictionC",
}
# How each ASN.1 string type holds its text.
_STRINGS = {12: "utf-8", 19: "ascii", 22: "ascii", 18: "ascii",
            20: "latin-1", 30: "utf-16-be", 28: "utf-32-be"}


def _der(data: bytes, pos: int = 0) -> tuple[int, bytes, int]:
    """The DER element at @p pos: its tag, its contents and where the next
    one starts."""
    try:
        tag, length = data[pos], data[pos + 1]
        pos += 2
        if length & 0x80:
            n = length & 0x7F
            if not 1 <= n <= 4:
                raise ValueError
            length = int.from_bytes(data[pos:pos + n], "big")
            pos += n
        if pos + length > len(data):
            raise ValueError
    except (IndexError, ValueError):
        raise ReleaseError("a certificate that is not DER") from None
    return tag, data[pos:pos + length], pos + length


def _elements(data: bytes) -> list[tuple[int, bytes]]:
    out, pos = [], 0
    while pos < len(data):
        tag, body, pos = _der(data, pos)
        out.append((tag, body))
    return out


def _oid(body: bytes) -> str:
    arcs, value = [], 0
    for byte in body:
        value = (value << 7) | (byte & 0x7F)
        if not byte & 0x80:
            arcs.append(value)
            value = 0
    if not arcs:
        raise ReleaseError("a certificate that is not DER")
    first = min(arcs[0] // 40, 2)
    return ".".join(str(a) for a in [first, arcs[0] - 40 * first] + arcs[1:])


def certificate_subject(der: bytes) -> list[tuple[str, str]]:
    """The subject of the X.509 certificate @p der, as subject_rdns()
    gives one: (attribute, value) pairs in certificate order."""
    tbs = _elements(_der(_der(der)[1])[1])
    # version (when there), serial, signature, issuer, validity, subject
    fields = tbs[1:] if tbs and tbs[0][0] == 0xA0 else tbs
    if len(fields) < 5:
        raise ReleaseError("a certificate that is not DER")
    rdns = []
    for _, rdn in _elements(fields[4][1]):
        for _, pair in _elements(rdn):
            parts = _elements(pair)
            if len(parts) != 2:
                raise ReleaseError("a certificate that is not DER")
            (_, oid), (kind, value) = parts
            name = _oid(oid)
            try:
                text = value.decode(_STRINGS[kind])
            except (KeyError, UnicodeDecodeError):
                raise ReleaseError("a certificate subject of an unknown "
                                   "string type") from None
            rdns.append((_ATTRIBUTES.get(name, name).upper(), text))
    return rdns


def shown(rdns: list[tuple[str, str]]) -> str:
    return "".join(f"/{attr}={value.replace('/', chr(92) + '/')}"
                   for attr, value in rdns)


def certificates_in(file: str) -> list[bytes]:
    """The certificates of a PEM or DER file, each as DER."""
    try:
        data = Path(file).read_bytes()
    except OSError as e:
        raise ReleaseError(f"{file}: {e}") from e
    found = re.findall(rb"-----BEGIN CERTIFICATE-----(.+?)"
                       rb"-----END CERTIFICATE-----", data, re.S)
    try:
        return [base64.b64decode(b) for b in found] if found else [data]
    except ValueError as e:
        raise ReleaseError(f"{file}: not a certificate file") from e
