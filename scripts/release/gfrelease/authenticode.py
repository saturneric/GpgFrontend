# Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""Authenticode for the stable Windows packages, by osslsigncode.

Signing happens on the maintainer's machine, with the certificate's key on a
hardware token (PKCS#11) or, for a rehearsal, in a file. Verifying checks the
signature and its chain, and that the signer is exactly the subject the
trust root pins (host_policy.authenticode_subject, trust.py): a valid
signature by any other certificate is not ours. osslsigncode prints the
subject as "/CN=.../O=.../C=..." or, in other versions, as RFC 2253
"C=...,O=...,CN=..." (last RDN first); the pin may be written either way,
and the two are compared RDN by RDN, in certificate order.

A Windows package is signed inside and out (plan()):

  msi    every program inside (the payload in its cabinet, msi.py), then
         the package itself
  zip    every program inside (a portable build: there is no outside)
  msix   the package itself

"Every program inside" is product.json's `authenticode` rule: the files
with one of its `sign_inside` extensions, but none under a folder named in
`leave_unsigned` (GnuPG's own binaries, shipped as upstream built them).
Whatever else a package holds stays byte for byte as CI built it, which the
stage checks before anything leaves the machine (unchanged_apart_from()),
and the signing host checks every signature again (verify_package()).
"""

from __future__ import annotations

import os
import re
import tempfile
import time
import zipfile
from dataclasses import dataclass, field
from typing import Callable
from pathlib import Path, PurePosixPath

import msi
from common import ReleaseError, ToolFailed, run

# Signing goes through the time-stamping services in turn: one being
# slow or rate-limited costs a retry, not the release. Only that is
# retried: a wrong PIN tried again could lock the token.
ATTEMPTS = 6
RETRY_DELAY = 10
_TSA_FAILURE = re.compile(r"time-?stamp|curl|http", re.I)
_LOGIN_FAILURE = re.compile(r"\bPIN\b|login|passphrase|password|bad decrypt",
                            re.I)


@dataclass(frozen=True)
class Rules:
    """Which files inside a package are programs to sign."""
    sign_inside: tuple[str, ...] = (".exe", ".dll")
    leave_unsigned: tuple[str, ...] = ()

    @classmethod
    def of(cls, product) -> "Rules":
        rule = product.data.get("authenticode", {})
        return cls(tuple(e.lower() for e in rule.get("sign_inside",
                                                     cls.sign_inside)),
                   tuple(d.lower() for d in rule.get("leave_unsigned", ())))

    def wants(self, path: str) -> bool:
        p = PurePosixPath(path)
        return p.suffix.lower() in self.sign_inside and \
            not any(part.lower() in self.leave_unsigned
                    for part in p.parent.parts)


def plan(info: dict) -> tuple[bool, bool]:
    """(signed inside, signed outside) for a file classified as @p info."""
    kind = info["kind"]
    if kind == "msi":
        return True, True
    if kind == "msix":
        return False, True
    if kind == "zip" and info.get("os") == "windows":
        return True, False
    return False, False


@dataclass
class Signer:
    cert: str                    # certificate (chain) file, or PKCS#11 URI
    key: str                     # key file, or PKCS#11 URI
    pkcs11_module: str = ""      # the token's PKCS#11 module, if any
    # RFC 3161 TSAs, tried in turn: the stage's one network call. A string
    # may list several, separated by commas.
    timestamp_url: str | list[str] = ""
    description: str = "GpgFrontend"
    # The token's PIN (or the key file's passphrase), asked once: handed to
    # each osslsigncode through a pipe, never on a command line or disk.
    pin: str | None = field(default=None, repr=False)
    # Asks for the PIN on the first signature (a dry run, or a stage past
    # this step, never asks); an empty answer leaves it to the token.
    ask_pin: Callable[[], str] | None = field(default=None, repr=False)

    @property
    def timestamps(self) -> list[str]:
        urls = self.timestamp_url
        if isinstance(urls, str):
            urls = urls.split(",")
        return [u.strip() for u in urls if u.strip()]

    def _once(self, src: Path, dst: Path, ts: str | None) -> None:
        cmd = ["osslsigncode", "sign", "-h", "sha256", "-n", self.description]
        if self.pkcs11_module:
            cmd += ["-pkcs11module", self.pkcs11_module,
                    "-pkcs11cert", self.cert, "-key", self.key]
        else:
            cmd += ["-certs", self.cert, "-key", self.key]
        if ts:
            cmd += ["-ts", ts]
        if dst.exists():
            dst.unlink()
        if self.ask_pin is not None:
            ask, self.ask_pin = self.ask_pin, None
            self.pin = ask() or None
        if self.pin is None:
            run(cmd + ["-in", src, "-out", dst])
            return
        r, w = os.pipe()
        try:
            os.write(w, self.pin.encode() + b"\n")
            os.close(w)
            w = -1
            run(cmd + ["-readpass", f"/dev/fd/{r}", "-in", src, "-out", dst],
                pass_fds=(r,))
        finally:
            os.close(r)
            if w >= 0:
                os.close(w)

    def sign(self, src: Path, dst: Path) -> None:
        urls = self.timestamps
        if not urls:
            self._once(src, dst, None)
            return
        for attempt in range(ATTEMPTS):
            try:
                self._once(src, dst, urls[attempt % len(urls)])
                return
            except ToolFailed as e:
                if attempt == ATTEMPTS - 1 or \
                        not _TSA_FAILURE.search(str(e)) or \
                        _LOGIN_FAILURE.search(str(e)):
                    raise
                time.sleep(RETRY_DELAY)

    def sign_bytes(self, data: bytes, name: str) -> bytes:
        """@p data, a program called @p name, signed."""
        with tempfile.TemporaryDirectory(prefix="gfr-sign-") as tmp:
            src = Path(tmp) / f"in{PurePosixPath(name).suffix}"
            dst = Path(tmp) / f"out{PurePosixPath(name).suffix}"
            src.write_bytes(data)
            self.sign(src, dst)
            return dst.read_bytes()


def contents(file: Path, info: dict) -> dict[str, bytes]:
    """What a package signed inside holds, by path ("/"-separated)."""
    if info["kind"] == "msi":
        return msi.Payload.read(file).files()
    try:
        with zipfile.ZipFile(file) as z:
            return {i.filename: z.read(i) for i in z.infolist()
                    if not i.is_dir()}
    except (zipfile.BadZipFile, OSError) as e:
        raise ReleaseError(f"{file.name}: {e}") from e


def _rezip(src: Path, dst: Path, new: dict[str, bytes]) -> None:
    """@p src written again to @p dst, the entries in @p new replaced; the
    order, names, times, permissions and compression stay."""
    with zipfile.ZipFile(src) as zin, zipfile.ZipFile(dst, "w") as zout:
        zout.comment = zin.comment
        for item in zin.infolist():
            data = new.get(item.filename, None) if not item.is_dir() else None
            if data is None:
                data = zin.read(item)
            info = zipfile.ZipInfo(item.filename, item.date_time)
            info.compress_type = item.compress_type
            info.external_attr = item.external_attr
            info.create_system = item.create_system
            info.comment = item.comment
            zout.writestr(info, data)


def sign_package(src: Path, dst: Path, info: dict, signer: Signer,
                 rules: Rules, progress=None) -> list[str]:
    """@p src signed as plan() says, written to @p dst; returns the paths
    signed inside. @p progress(done, total, path) follows the programs."""
    inside, outside = plan(info)
    if not inside and not outside:
        raise ReleaseError(f"{src.name}: a {info['kind']} is not signed")
    signed: dict[str, bytes] = {}
    if inside:
        files = contents(src, info)
        targets = sorted(p for p in files if rules.wants(p))
        for n, path in enumerate(targets, 1):
            signed[path] = signer.sign_bytes(files[path], path)
            if progress:
                progress(n, len(targets), path)
    if dst.exists():
        dst.unlink()
    if info["kind"] == "zip":
        _rezip(src, dst, signed)
        return sorted(signed)
    body = src
    if signed:
        body = dst.with_name(dst.name + ".payload")
        msi.Payload.read(src).replace(signed, body)
    if outside:
        signer.sign(body, dst)
    if body != src:
        body.unlink()
    return sorted(signed)


def unchanged_apart_from(build: Path, final: Path, info: dict,
                         signed: list[str]) -> None:
    """@p final holds exactly the files @p build does, and only those in
    @p signed differ."""
    if not plan(info)[0]:
        return
    before, after = contents(build, info), contents(final, info)
    if set(before) != set(after):
        raise ReleaseError(f"{final.name}: holds other files than CI built")
    changed = sorted(p for p in before if before[p] != after[p])
    unexpected = set(changed) - set(signed)
    if unexpected:
        raise ReleaseError(f"{final.name}: {sorted(unexpected)} changed "
                           "without being signed")


def verify_package(file: Path, info: dict, subject: str | list[str],
                   cafile: str | None, rules: Rules,
                   require_timestamp: bool = False) -> int:
    """@p file is signed as plan() says: the package by @p subject, and every
    program inside the rules name; returns how many were checked inside."""
    inside, outside = plan(info)
    if outside:
        verify(file, subject, cafile, require_timestamp)
    if not inside:
        return 0
    programs = {p: d for p, d in contents(file, info).items()
                if rules.wants(p)}
    with tempfile.TemporaryDirectory(prefix="gfr-verify-") as tmp:
        for path, data in sorted(programs.items()):
            local = Path(tmp) / f"program{PurePosixPath(path).suffix}"
            local.write_bytes(data)
            try:
                verify(local, subject, cafile, require_timestamp)
            except ReleaseError as e:
                raise ReleaseError(f"{file.name}: {path}: "
                                   f"{str(e).split(': ', 1)[-1]}") from e
    return len(programs)


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
