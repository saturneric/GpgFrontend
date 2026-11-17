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

import _paths  # noqa: E402,F401
from gfrelease.domain import layout  # noqa: E402
from gfrelease.adapters import minisig  # noqa: E402
from gfrelease.app import trust  # noqa: E402


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
    if os.name == "nt":
        # Git for Windows brings a gpg of its own and would sign with that:
        # the tests' keys are in the home of the one on PATH.
        env.update({"GIT_CONFIG_COUNT": "1",
                    "GIT_CONFIG_KEY_0": "gpg.program",
                    "GIT_CONFIG_VALUE_0": shutil.which("gpg") or "gpg"})
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


# --- the same tests on Windows ----------------------------------------------

WINDOWS = os.name == "nt"

if WINDOWS:
    # gfrelease refuses to sign with minisign on Windows, where it could
    # only hand the key over as a file. The keys of these tests are files
    # made for them, so here they are signed with that way.
    def _sign_with_a_key_file(secret_key, file, trusted_comment, sig=None):
        sig = sig or file.with_name(file.name + minisig.MINISIG)
        minisig.run(["minisign", "-S", "-s", secret_key, "-m", file,
                     "-x", sig, "-t", trusted_comment])
        return sig

    minisig.sign = _sign_with_a_key_file


def stand_in(directory: Path, name: str, source: str) -> Path:
    """A command made for a test: the Python program @p source, as a file
    the release tooling can run by its path alone. That is a script with
    its interpreter line, and on Windows a .cmd beside it."""
    directory = Path(directory)
    if not WINDOWS:
        script = directory / name
        script.write_text(f"#!{sys.executable}\n{source}")
        script.chmod(0o755)
        return script
    (directory / f"{name}.py").write_text(source, "utf-8")
    script = directory / f"{name}.cmd"
    script.write_text(f'@"{sys.executable}" "%~dp0{name}.py" %*\r\n')
    return script


# --- rclone over local directories, in process -----------------------------
#
# Each rclone run costs some 45 ms of startup, and a chain test makes a
# hundred of them: nine tenths of the suite's time. For a store that is a
# local directory, local_rclone() answers in process as rclone would.
# StoreContractTest holds it to the real rclone, case by case, and
# GF_REAL_RCLONE=1 runs every test through the real one. A store with a
# stand-in rclone (an outage) and the gfrelease command line still run it.

REAL_RCLONE = os.environ.get("GF_REAL_RCLONE") == "1"


def _rclone_glob(pattern: str):
    """rclone's filter glob: * within a name, ** across names; one not
    starting with / matches the end of a path."""
    import re
    out, i = [], 0
    while i < len(pattern):
        if pattern.startswith("**", i):
            out.append(".*")
            i += 2
        elif pattern[i] == "*":
            out.append("[^/]*")
            i += 1
        elif pattern[i] == "?":
            out.append("[^/]")
            i += 1
        else:
            out.append(re.escape(pattern[i]))
            i += 1
    body = "".join(out)
    if pattern.startswith("/"):
        return re.compile(body[1:] if body.startswith("/") else body + "$")
    return re.compile(f"(?:^|.*/){body}$")


def _local_files(path: Path) -> list[str]:
    return sorted(p.relative_to(path).as_posix() for p in path.rglob("*")
                  if p.is_file())


def _copy_tree(src: Path, dst: Path) -> None:
    for rel in _local_files(src):
        (dst / rel).parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(src / rel, dst / rel)


def _local_rclone_run(args: tuple, text: bool):
    """rclone @p args as Store runs it, over local paths: lsf, lsjson,
    cat, copy, copyto and hashsum, with rclone's exit status 3 for a path
    that is not there."""
    import hashlib
    import json
    import subprocess

    verb, rest = args[0], list(args[1:])
    opts, paths = {}, []
    while rest:
        a = rest.pop(0)
        if a in ("--include", "--count"):
            opts[a] = rest.pop(0)
        elif a.startswith("--"):
            opts[a] = True
        else:
            paths.append(Path(a))

    def done(code=0, out=b"", err=""):
        if text:
            out = out.decode("utf-8", "replace")
        else:
            err = err.encode()
        return subprocess.CompletedProcess(["rclone", *args], code, out, err)

    if verb == "hashsum":
        paths = paths[1:]  # the algorithm, sha256 here
    src = paths[0]
    if not src.exists():
        return done(3, err=f"error: directory not found: {src}")
    if verb == "lsf":
        if src.is_file():
            return done(out=f"{src.name}\n".encode())
        names = _local_files(src)
        if "--include" in opts:
            match = _rclone_glob(opts["--include"])
            names = [n for n in names if match.match(n)]
        return done(out="".join(f"{n}\n" for n in names).encode())
    if verb == "lsjson":
        found = [src] if src.is_file() else sorted(
            p for p in src.iterdir() if p.is_file())
        return done(out=json.dumps(
            [{"Path": p.name, "Name": p.name, "Size": p.stat().st_size,
              "IsDir": False} for p in found]).encode())
    if verb == "cat":
        files = [src] if src.is_file() else [src / n for n in _local_files(src)]
        data = b"".join(f.read_bytes() for f in files)
        if "--count" in opts:
            data = data[:int(opts["--count"])]
        return done(out=data)
    if verb == "hashsum":
        files = ([(src.name, src)] if src.is_file() else
                 [(n, src / n) for n in _local_files(src)])
        return done(out="".join(
            f"{hashlib.sha256(f.read_bytes()).hexdigest()}  {n}\n"
            for n, f in files).encode())
    if verb in ("copy", "copyto"):
        dst = paths[1]
        if src.is_dir():
            _copy_tree(src, dst)
        else:
            target = dst / src.name if verb == "copy" else dst
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(src, target)
        return done()
    raise AssertionError(f"local_rclone does not know `rclone {verb}`")


def local_rclone():
    """A patch (start() it) that answers Store's rclone in process for a
    store over local directories with the plain rclone; anything else, and
    everything with GF_REAL_RCLONE=1, runs rclone itself."""
    import re
    from unittest import mock
    from gfrelease.adapters import store

    real = store.Store._rclone

    def _rclone(self, *args, **kw):
        remote = re.compile(r"^[A-Za-z0-9_.-]{2,}:")
        if REAL_RCLONE or self.rclone != "rclone" or \
                any(remote.match(str(a)) for a in args):
            return real(self, *args, **kw)
        return _local_rclone_run(args, kw.get("text", True))

    return mock.patch.object(store.Store, "_rclone", _rclone)


def symlink(test, link: Path, target: Path) -> None:
    """@p link made a symbolic link to @p target, or the test skipped where
    this user may not make one (Windows, without developer mode)."""
    try:
        link.symlink_to(target)
    except OSError as e:
        test.skipTest(f"no symbolic links here: {e}")


def signing_tools() -> bool:
    """Whether Authenticode can be made and checked here: osslsigncode and
    msitools, or on Windows signtool (the system brings the rest)."""
    if not WINDOWS:
        return tools("osslsigncode", "openssl", "msiinfo", "msibuild")
    from gfrelease.adapters.windows import authenticode
    try:
        authenticode.find_signtool()
        return True
    except authenticode.ReleaseError:
        if os.environ.get("GF_REQUIRE_RELEASE_TOOLS") == "1":
            raise AssertionError("GF_REQUIRE_RELEASE_TOOLS=1, but signtool "
                                 "is not installed")
        return False


def msix_resources(test, directory: Path, program: str) -> Path:
    """resource/msix/ as @p test needs it, and used for as long as it
    runs: the manifest starting @p program (a path inside the test's MSI)
    instead of GpgFrontend's own."""
    from gfrelease.adapters.windows import msix
    from unittest import mock
    out = Path(directory) / "msix-resources"
    if not out.exists():  # there already in a copy of a test's directory
        shutil.copytree(msix.RESOURCES, out)
        manifest = out / msix.MANIFEST
        text = manifest.read_text("utf-8")
        ours = 'Executable="VFS\\AppData\\GpgFrontend\\bin\\GpgFrontend.exe"'
        assert ours in text
        manifest.write_text(text.replace(
            ours, 'Executable="VFS\\%s"' % program.replace("/", "\\")),
            "utf-8")
    patch = mock.patch.object(msix, "RESOURCES", out)
    patch.start()
    test.addCleanup(patch.stop)
    return out


def pack_as_a_zip(tree: Path, files: dict, out: Path,
                  resources: Path | None = None) -> None:
    """What msix._pack makes, as far as anything but Windows reads it: the
    tree and the files in a zip, with the two parts a signer looks for."""
    import zipfile
    with zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED) as z:
        for file in sorted(p for p in tree.rglob("*") if p.is_file()):
            z.write(file, file.relative_to(tree).as_posix())
        for path, data in files.items():
            z.writestr(f"VFS/{path}", data)
        z.writestr("AppxBlockMap.xml", '<?xml version="1.0" encoding="UTF-8" '
                   'standalone="no"?><BlockMap xmlns="http://schemas.'
                   'microsoft.com/appx/2010/blockmap" HashMethod="http://'
                   'www.w3.org/2001/04/xmlenc#sha256"></BlockMap>')
        z.writestr("[Content_Types].xml", '<?xml version="1.0" encoding='
                   '"UTF-8" standalone="yes"?><Types xmlns="http://schemas.'
                   'openxmlformats.org/package/2006/content-types"><Default '
                   'Extension="xml" ContentType="application/vnd.ms-appx.'
                   'manifest+xml"/><Override PartName="/AppxBlockMap.xml" '
                   'ContentType="application/vnd.ms-appx.blockmap+xml"/>'
                   '</Types>')


def packs_msix(test) -> None:
    """An MSIX is packed by the Windows SDK. Elsewhere @p test gets one
    that is a package to everything it is checked with there (osslsigncode
    signs and verifies it), so the same release is made on every platform."""
    if WINDOWS:
        return
    from gfrelease.adapters.windows import msix
    from unittest import mock
    for name, value in (("available", lambda: True),
                        ("_pack", pack_as_a_zip)):
        patch = mock.patch.object(msix, name, value)
        patch.start()
        test.addCleanup(patch.stop)


# A code-signing certificate that signs itself, made in the user's store
# (where signtool finds its key) and gone again after the test. One left
# behind by a run that was killed expires in two days, and is removed here
# once it has.
_NEW_CERTIFICATE = r"""
Get-ChildItem Cert:\CurrentUser\My |
    Where-Object { $_.Subject -like '*O=Rehearsal*' -and
                   $_.NotAfter -lt (Get-Date) } |
    ForEach-Object { Remove-Item -LiteralPath $_.PSPath -DeleteKey }
$c = New-SelfSignedCertificate -Type CodeSigningCert `
    -Subject $env:GFR_SUBJECT -CertStoreLocation Cert:\CurrentUser\My `
    -NotAfter (Get-Date).AddDays(2) -KeyAlgorithm RSA -KeyLength 2048 `
    -HashAlgorithm SHA256
[IO.File]::WriteAllBytes($env:GFR_OUT, $c.RawData)
$c.Thumbprint
"""
_REMOVE_CERTIFICATE = r"""
Remove-Item -LiteralPath ('Cert:\CurrentUser\My\' + $env:GFR_THUMBPRINT) `
    -DeleteKey -ErrorAction SilentlyContinue
"""


class SigningCertificate:
    """A throwaway Authenticode certificate, and how this platform signs
    with it and checks what it signed: `subject` to pin, `ca` to pass as
    the rehearsal's authenticode_ca, signer() to sign with."""

    def __init__(self, directory: Path, name: str = "GpgFrontend Test Signing"):
        from gfrelease.adapters.windows import authenticode, x509
        directory = Path(directory)
        if WINDOWS:
            self.ca = directory / f"{name}.cer"
            self.thumbprint = authenticode.powershell(
                _NEW_CERTIFICATE, GFR_SUBJECT=f"CN={name}, O=Rehearsal, C=DE",
                GFR_OUT=str(self.ca)).strip()
            # As the certificate has it, whichever way Windows ordered it.
            self.subject = x509.shown(
                x509.certificate_subject(self.ca.read_bytes()))
        else:
            import subprocess
            self.ca = directory / f"{name}.pem"
            self.key = directory / f"{name}.key"
            self.subject = f"/CN={name}/O=Rehearsal/C=DE"
            subprocess.run(["openssl", "req", "-x509", "-newkey", "rsa:2048",
                            "-nodes", "-days", "2", "-subj", self.subject,
                            "-keyout", self.key, "-out", self.ca],
                           check=True, capture_output=True)

    def options(self) -> list[str]:
        """What names this certificate to `gfrelease stable`."""
        if WINDOWS:
            return ["--authenticode-thumbprint", self.thumbprint]
        return ["--authenticode-cert", str(self.ca),
                "--authenticode-key", str(self.key)]

    def signer(self, **kw):
        from gfrelease.adapters.windows import authenticode
        if WINDOWS:
            return authenticode.SigntoolSigner(self.thumbprint, **kw)
        return authenticode.Signer(str(self.ca), str(self.key), **kw)

    def close(self) -> None:
        if WINDOWS:
            from gfrelease.adapters.windows import authenticode
            authenticode.powershell(_REMOVE_CERTIFICATE,
                                    GFR_THUMBPRINT=self.thumbprint)
