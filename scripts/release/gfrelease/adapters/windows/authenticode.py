# Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""Authenticode for the stable Windows packages.

Signing happens on the maintainer's Windows machine:

  SigntoolSigner  Windows' signtool, with the certificate in the user's
                  store named by its SHA-1 thumbprint, and its key wherever
                  the store says (a smart card the vendor's software drives)
  Signer          osslsigncode with a certificate and key file: rehearsals
                  and tests, on any system

Verifying checks the signature and its chain, and that the signer is exactly
the subject the trust root pins (host_policy.authenticode_subject, trust.py):
a valid signature by any other certificate is not ours. It is osslsigncode
again, and on Windows the system's own check (Get-AuthenticodeSignature).
osslsigncode prints the subject as "/CN=.../O=.../C=..." or, in other
versions, as RFC 2253 "C=...,O=...,CN=..." (last RDN first); the pin may be
written either way, and the two are compared RDN by RDN, in certificate
order. On Windows the subject is read from the certificate itself, so how
Windows would print it never matters.

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

A signer signs files where they are, as many as it is given at once
(sign_files()): a token that asks for its PIN per signtool run asks once
for every program of a release, and once more for the packages. So a
package is opened (open_package(): its programs laid out as files), the
programs are signed, and it is closed again around them (close_package());
the stable stage does that for every package at once (stable.py).
"""

from __future__ import annotations

import base64
import json
import os
import re
import shutil
import tempfile
import threading
import time
import zipfile
from dataclasses import dataclass
from pathlib import Path, PurePosixPath

from ...errors import ReleaseError, ToolFailed
from ..process import WINDOWS, run
from . import msi, x509

# Signing goes through the time-stamping services in turn: one being
# slow or rate-limited costs a retry, not the release. Only that is
# retried: a wrong PIN tried again could lock the token.
ATTEMPTS = 6
RETRY_DELAY = 10
_TSA_FAILURE = re.compile(r"time-?stamp|curl|http", re.I)

# signtool says a time-stamping service failed in words, or only as the
# network error it met (0x80072EE7 and its neighbours, WinINet's). What the
# card, its PIN or the key store answer comes as codes too, and those are
# never tried again: 0x8010.... (the smart card service: a wrong or blocked
# PIN, no card), 0x8009.... (the key store), 0x800704C7 (cancelled) and
# 0xC0000225 (another program holds the card).
_SIGNTOOL_TSA = re.compile(r"timestamp server|0x80072[0-9a-f]{3}", re.I)
_SIGNTOOL_CARD = re.compile(r"0x8010[0-9a-f]{4}|0x8009[0-9a-f]{4}|"
                            r"0x800704c7|0xc0000225|\bPIN\b|smart ?card",
                            re.I)
# What it says of a package whose manifest names another publisher than the
# certificate's subject (ERROR_BAD_FORMAT, and no word of why).
_SIGNTOOL_PUBLISHER = re.compile(r"0x8007000b", re.I)
# One signtool run asks for the PIN and waits for it: long enough to find
# the card, not for ever behind a dialog nobody sees.
SIGNTOOL_TIMEOUT = 1800
# The longest command line Windows starts (32,767 characters), with room.
_COMMAND_LINE = 30000


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


def _timestamps(urls: str | list[str]) -> list[str]:
    if isinstance(urls, str):
        urls = urls.split(",")
    return [u.strip() for u in urls if u.strip()]


@dataclass
class Signer:
    """osslsigncode with a certificate and key file: a rehearsal's or a
    test's signer (the maintainer signs with signtool, SigntoolSigner)."""
    cert: str                    # certificate (chain) file
    key: str                     # key file, without a passphrase
    # RFC 3161 TSAs, tried in turn: the stage's one network call. A string
    # may list several, separated by commas.
    timestamp_url: str | list[str] = ""
    description: str = "GpgFrontend"

    @property
    def timestamps(self) -> list[str]:
        return _timestamps(self.timestamp_url)

    def _once(self, src: Path, dst: Path, ts: str | None) -> None:
        cmd = ["osslsigncode", "sign", "-h", "sha256", "-n", self.description,
               "-certs", self.cert, "-key", self.key]
        if ts:
            cmd += ["-ts", ts]
        if dst.exists():
            dst.unlink()
        run(cmd + ["-in", src, "-out", dst])

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
                        not _TSA_FAILURE.search(str(e)):
                    raise
                time.sleep(RETRY_DELAY)

    def sign_files(self, files: list[Path], on_done=None) -> None:
        """Every one of @p files signed where it is; @p on_done(file) after
        each."""
        for file in files:
            signed = file.with_name(file.name + ".signed")
            self.sign(file, signed)
            signed.replace(file)
            if on_done:
                on_done(file)


def find_signtool(explicit: str = "") -> str:
    """signtool.exe: the one named, else the one on PATH, else the newest
    Windows SDK's, its x64 build before its x86 one."""
    return find_sdk_tool("signtool", explicit)


def find_sdk_tool(name: str, explicit: str = "") -> str:
    """A tool of the Windows SDK (signtool, makeappx, makepri), found as
    find_signtool() says."""
    if explicit:
        found = shutil.which(explicit)
        if found is None:
            raise ReleaseError(f"{name} {explicit} is not there")
        return found
    found = shutil.which(name)
    if found:
        return found
    roots = [Path(base) / "Windows Kits" / "10" / "bin"
             for base in dict.fromkeys(
                 os.environ.get(name, "") for name in
                 ("ProgramFiles(x86)", "ProgramFiles")) if base]

    def rank(tool: Path) -> tuple:
        try:
            version = tuple(int(n) for n in tool.parent.parent.name.split("."))
        except ValueError:
            version = ()
        arch = tool.parent.name.lower()
        return version, {"x64": 2, "x86": 1}.get(arch, 0)

    tools = [t for root in roots for t in root.glob(f"*/*/{name}.exe")]
    if not tools:
        raise ReleaseError(
            f"{name} is not installed: it was looked for on PATH and under "
            f"{', '.join(str(r) for r in roots) or 'no Windows SDK folder'}")
    return str(max(tools, key=rank))


@dataclass
class SigntoolSigner:
    """signtool with a certificate of the user's store, which finds the key
    for it: on a smart card, the card's software asks for the PIN, once for
    each run of signtool. So everything given to sign_files() is signed by
    one run."""
    thumbprint: str              # the certificate's SHA-1, as certmgr shows
    signtool: str = ""           # default: find_signtool()
    timestamp_url: str | list[str] = ""
    description: str = "GpgFrontend"

    def __post_init__(self):
        self.thumbprint = re.sub(r"[\s:]", "", self.thumbprint).upper()
        if not re.fullmatch(r"[0-9A-F]{40}", self.thumbprint):
            raise ReleaseError("an Authenticode thumbprint is a "
                               "certificate's SHA-1: 40 hex digits")

    @property
    def timestamps(self) -> list[str]:
        return _timestamps(self.timestamp_url)

    def _once(self, files: list[Path], ts: str | None,
              on_done=None) -> tuple[list[Path], str]:
        """One run of signtool over @p files; returns those it did not
        sign, and what it said about them."""
        cmd = [find_signtool(self.signtool), "sign", "/v",
               "/sha1", self.thumbprint, "/fd", "sha256",
               "/d", self.description]
        if ts:
            cmd += ["/tr", ts, "/td", "sha256"]
        before = {f: _stamp(f) for f in files}
        left = list(files)
        stop = threading.Event()

        def watch() -> None:
            # signtool rewrites each file as it signs it, in the order
            # given: that is all the progress it shows before it ends.
            while left:
                ended = stop.is_set()
                if _stamp(left[0]) != before[left[0]]:
                    file = left.pop(0)
                    if on_done:
                        on_done(file)
                elif ended:
                    return
                else:
                    stop.wait(0.3)

        watcher = threading.Thread(target=watch, daemon=True)
        watcher.start()
        try:
            out = run(cmd + files, check=False, timeout=SIGNTOOL_TIMEOUT)
        finally:
            stop.set()
            watcher.join()
        said = (out.stdout + out.stderr).strip()
        if out.returncode == 0:
            for file in left:
                if on_done:
                    on_done(file)
            return [], said
        # A file it changed and did not finish may be half signed (a
        # signature without its time stamp): only what it names as signed
        # counts.
        named = {line.split(":", 1)[1].strip().lower()
                 for line in said.splitlines()
                 if line.startswith("Successfully signed:")}
        return [f for f in files if str(f).lower() not in named], said

    def _batch(self, files: list[Path], on_done=None) -> None:
        urls = self.timestamps
        pristine = {f: f.read_bytes() for f in files} if urls else {}
        left = list(files)
        reported = set()

        def once(file: Path) -> None:
            # A file signed again after a failed run is one file still.
            if on_done and file not in reported:
                reported.add(file)
                on_done(file)

        for attempt in range(ATTEMPTS if urls else 1):
            ts = urls[attempt % len(urls)] if urls else None
            for file in left:
                if attempt and file.read_bytes() != pristine[file]:
                    file.write_bytes(pristine[file])
            left, said = self._once(left, ts, once)
            if not left:
                return
            if attempt == ATTEMPTS - 1 or not urls or \
                    not _SIGNTOOL_TSA.search(said) or \
                    _SIGNTOOL_CARD.search(said):
                detail = "\n".join(said.splitlines()[-8:])
                if _SIGNTOOL_PUBLISHER.search(said) and any(
                        f.suffix.lower() == ".msix" for f in left):
                    detail += ("\nan MSIX is signed only by the certificate "
                               "its manifest names as publisher: the subject "
                               "the trust root pins")
                raise ToolFailed(f"signtool failed: {detail}")
            time.sleep(RETRY_DELAY)

    def sign_files(self, files: list[Path], on_done=None) -> None:
        """Every one of @p files signed where it is, by one run of signtool
        (more only when their names outgrow a command line); @p on_done(file)
        as each is signed."""
        files = [Path(f) for f in files]
        batch: list[Path] = []
        for file in files:
            if batch and sum(len(str(f)) + 3 for f in batch + [file]) > \
                    _COMMAND_LINE:
                self._batch(batch, on_done)
                batch = []
            batch.append(file)
        if batch:
            self._batch(batch, on_done)

    def sign(self, src: Path, dst: Path) -> None:
        """@p src signed into @p dst, which keeps src's extension: signtool
        tells what a file is by it."""
        shutil.copyfile(src, dst)
        self.sign_files([dst])


def _stamp(file: Path) -> tuple[int, int]:
    s = file.stat()
    return s.st_mtime_ns, s.st_size


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


@dataclass
class Opened:
    """A package with the programs to sign inside it laid out as files."""
    src: Path
    info: dict
    outside: bool
    programs: dict[str, Path]    # path inside the package -> the file


def open_package(src: Path, info: dict, rules: Rules,
                 workdir: Path) -> Opened:
    """@p src with every program plan() and @p rules name written into
    @p workdir, each under its own extension (a signer tells what a file is
    by it), in the order of their paths."""
    inside, outside = plan(info)
    if not inside and not outside:
        raise ReleaseError(f"{src.name}: a {info['kind']} is not signed")
    programs = {}
    if inside:
        files = contents(src, info)
        workdir.mkdir(parents=True, exist_ok=True)
        for n, path in enumerate(sorted(p for p in files if rules.wants(p))):
            programs[path] = workdir / f"{n:04d}{PurePosixPath(path).suffix}"
            programs[path].write_bytes(files[path])
    return Opened(src, info, outside, programs)


def close_package(opened: Opened, dst: Path) -> list[str]:
    """The package written to @p dst around its programs as they are now
    (signed); returns their paths. Its own signature, when it takes one, is
    still to be made, on @p dst."""
    if dst.exists():
        dst.unlink()
    signed = {path: file.read_bytes()
              for path, file in opened.programs.items()}
    if opened.info["kind"] == "zip":
        _rezip(opened.src, dst, signed)
    elif signed:
        msi.Payload.read(opened.src).replace(signed, dst)
    else:
        shutil.copyfile(opened.src, dst)
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
    with tempfile.TemporaryDirectory(prefix="gfr-verify-") as tmp:
        # Laid out as the stage signed them.
        programs = open_package(file, info, rules, Path(tmp)).programs
        _verify([(local, f"{file.name}: {path}")
                 for path, local in programs.items()],
                subject, cafile, require_timestamp)
    return len(programs)


def _verified(file: Path, cafile: str | None, name: str) -> str:
    """osslsigncode's report on @p file, once it verified the signature."""
    cmd = ["osslsigncode", "verify", "-in", file]
    if cafile:
        cmd += ["-CAfile", cafile]
    out = run(cmd, check=False)
    text = out.stdout + out.stderr
    if out.returncode != 0 or "Signature verification: ok" not in text:
        raise ReleaseError(f"{name}: no valid Authenticode signature")
    return text


def _subject_in(text: str, name: str) -> str:
    # The first subject after "Signer's certificate:" is the signer's; the
    # chain osslsigncode prints later repeats it among others.
    m = re.search(r"Signer's certificate:\s*\n(?:[^\n]*\n)*?\s*Subject:\s*([^\n]+)",
                  text)
    if not m:
        raise ReleaseError(f"{name}: the signer's subject is unreadable")
    return m.group(1).strip()


def powershell(script: str, **env: str) -> str:
    """What @p script prints, run by Windows PowerShell with @p env added to
    the environment (how it is handed anything). The script goes encoded on
    the command line: no file a policy could refuse to run, no quoting."""
    prelude = ("$ErrorActionPreference = 'Stop'\n"
               "$ProgressPreference = 'SilentlyContinue'\n"
               "[Console]::OutputEncoding = [Text.Encoding]::UTF8\n")
    encoded = base64.b64encode((prelude + script).encode("utf-16-le"))
    return run(["powershell", "-NoProfile", "-NonInteractive",
                "-EncodedCommand", encoded.decode("ascii")],
               env=dict(powershell_environ(), **env), timeout=600).stdout


def powershell_environ() -> dict[str, str]:
    """This environment, as Windows PowerShell is started with it. Started
    from PowerShell 7, the inherited PSModulePath points it at 7's modules,
    and it loses its own (the Cert: drive among them)."""
    return {k: v for k, v in os.environ.items()
            if k.upper() != "PSMODULEPATH"}


# What Windows makes of each file listed in $env:GFR_FILES: its verdict, the
# signer's certificate, whether the signature is time-stamped, and what is
# wrong with the signer's chain (for a rehearsal's certificate, which no
# root of the machine vouches for).
_INSPECT = r"""
$out = foreach ($file in [IO.File]::ReadAllLines($env:GFR_FILES)) {
    $s = Get-AuthenticodeSignature -LiteralPath $file
    $c = $s.SignerCertificate
    $faults = @()
    if ($c) {
        $chain = New-Object Security.Cryptography.X509Certificates.X509Chain
        $chain.ChainPolicy.RevocationMode = 'NoCheck'
        [void]$chain.Build($c)
        $faults = @($chain.ChainStatus | ForEach-Object { $_.Status.ToString() })
    }
    [pscustomobject]@{
        status      = $s.Status.ToString()
        certificate = if ($c) { [Convert]::ToBase64String($c.RawData) } else { '' }
        timestamped = [bool]$s.TimeStamperCertificate
        faults      = $faults
    }
}
ConvertTo-Json -InputObject @($out) -Compress
"""


def _inspected(files: list[Path]) -> list[dict]:
    with tempfile.TemporaryDirectory(prefix="gfr-verify-") as tmp:
        listing = Path(tmp) / "files.txt"
        listing.write_text("".join(f"{f}\n" for f in files), "utf-8")
        text = powershell(_INSPECT, GFR_FILES=str(listing))
    try:
        found = json.loads(text)
        if len(found) != len(files):
            raise ValueError
    except ValueError:
        raise ReleaseError("Windows did not answer for every file whose "
                           "signature it was asked about") from None
    return found


def _verify(files: list[tuple[Path, str]], subject: str | list[str],
            cafile: str | None, require_timestamp: bool) -> None:
    """verify() for each (file, the name an error calls it)."""
    subjects = [subject] if isinstance(subject, str) else list(subject)
    subjects = [s for s in subjects if s]
    if not subjects:
        raise ReleaseError("no Authenticode subject is pinned")
    pinned = subjects[0] if len(subjects) == 1 else subjects

    if not WINDOWS:
        for file, name in files:
            text = _verified(file, cafile, name)
            if require_timestamp and "Timestamp is not available" in text:
                raise ReleaseError(f"{name}: its Authenticode signature has "
                                   "no time-stamp")
            got = _subject_in(text, name)
            if not any(x509.subject_rdns(got) == x509.subject_rdns(s)
                       for s in subjects):
                raise ReleaseError(f"{name}: signed by {got!r}, not "
                                   f"{pinned!r}")
        return

    rehearsal = x509.certificates_in(cafile) if cafile else []
    for (file, name), found in zip(files, _inspected([f for f, _ in files])):
        try:
            signer = base64.b64decode(found["certificate"])
        except ValueError:
            signer = b""
        # A rehearsal's certificate signs itself: Windows finds the
        # signature whole and only the root unknown, and then it must be
        # that very certificate.
        valid = found["status"] == "Valid" or (
            found["status"] == "UnknownError" and
            found["faults"] == ["UntrustedRoot"] and signer in rehearsal)
        if not valid or not signer:
            raise ReleaseError(f"{name}: no valid Authenticode signature")
        if require_timestamp and not found["timestamped"]:
            raise ReleaseError(f"{name}: its Authenticode signature has no "
                               "time-stamp")
        got = x509.certificate_subject(signer)
        if not any(got == x509.subject_rdns(s) for s in subjects):
            raise ReleaseError(f"{name}: signed by {x509.shown(got)!r}, not "
                               f"{pinned!r}")


def verify(file: Path, subject: str | list[str],
           cafile: str | None = None, require_timestamp: bool = False) -> None:
    """@p file is validly signed by a certificate with @p subject, or one of
    them: the current certificate, and those it replaced, for what they
    signed before. With @p require_timestamp, the signature also carries an
    RFC 3161 time stamp: without one it stops verifying the day the
    certificate expires, and the release with it.

    @p cafile is what a rehearsal's signatures chain to: a CA bundle for
    osslsigncode; on Windows, where the system's roots decide, the one
    self-signed certificate that may sign in their place."""
    _verify([(file, file.name)], subject, cafile, require_timestamp)
