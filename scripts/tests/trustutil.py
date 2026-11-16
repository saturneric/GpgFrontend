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
    import authenticode
    try:
        authenticode.find_signtool()
        return True
    except authenticode.ReleaseError:
        if os.environ.get("GF_REQUIRE_RELEASE_TOOLS") == "1":
            raise AssertionError("GF_REQUIRE_RELEASE_TOOLS=1, but signtool "
                                 "is not installed")
        return False


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
        import authenticode
        directory = Path(directory)
        if WINDOWS:
            self.ca = directory / f"{name}.cer"
            self.thumbprint = authenticode.powershell(
                _NEW_CERTIFICATE, GFR_SUBJECT=f"CN={name}, O=Rehearsal, C=DE",
                GFR_OUT=str(self.ca)).strip()
            # As the certificate has it, whichever way Windows ordered it.
            self.subject = authenticode._shown(
                authenticode.certificate_subject(self.ca.read_bytes()))
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
        import authenticode
        if WINDOWS:
            return authenticode.SigntoolSigner(self.thumbprint, **kw)
        return authenticode.Signer(str(self.ca), str(self.key), **kw)

    def close(self) -> None:
        if WINDOWS:
            import authenticode
            authenticode.powershell(_REMOVE_CERTIFICATE,
                                    GFR_THUMBPRINT=self.thumbprint)
