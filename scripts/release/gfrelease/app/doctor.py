# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""gfrelease doctor: whether this machine can do its part, before it tries.

  maintainer  the tools of the offline stage, the stores it reads and
              writes, the signing key in GnuPG, the Authenticode token or
              files (on Windows: the certificate in the user's store), the
              checkout with its release tags and keyring
  ci          the tools stage needs, and the staging store
  host        the signing host's tools, every store, the index key and
              that the trusted index-keys list holds it
  (any)       the trust: root and index-keys list verify, and how long each
              has left

Each check prints ✓ or ✗ with what it found, and for a ✗ how to fix it.
Exit status 1 when any check failed.
"""

from __future__ import annotations

import base64
import json
import shutil
from pathlib import Path

from ..adapters import minisig
from ..adapters.git import release_tags
from ..adapters.process import WINDOWS, run
from ..adapters.windows import authenticode
from ..cli import ui
from ..errors import ReleaseError
from . import trust

ROLES = ("maintainer", "ci", "host")
# What each role runs: cmake derives a commit's version (GitVersion.cmake),
# gpgconf ends the throwaway GnuPG homes, osslsigncode and msitools sign and
# check what is inside a Windows package.
TOOLS = {
    "maintainer": ("git", "gpg", "gpgconf", "rclone", "cosign", "cmake",
                   "minisign", "osslsigncode", "msiinfo", "msibuild"),
    "ci": ("git", "rclone", "cosign"),
    "host": ("git", "gpg", "gpgconf", "rclone", "cosign", "cmake",
             "minisign", "osslsigncode", "msiinfo"),
}
# On Windows signtool signs, and Windows Installer itself opens an MSI.
ON_WINDOWS = {"osslsigncode": "signtool", "msiinfo": None, "msibuild": None}
# On Windows the maintainer's stage also packs the MSIX (msix.py), with
# the Windows SDK's makepri and makeappx; its tools are found where it
# installs them when they are not on PATH.
WINDOWS_ONLY = {"maintainer": ("makepri", "makeappx")}
SDK_TOOLS = ("signtool", "makepri", "makeappx")
STORES = {
    # All a maintainer's machine reads and writes: the host alone writes
    # the builds store.
    "maintainer": ("staging",),
    "ci": ("staging",),
    "host": ("staging", "builds", "update"),
}


class Doctor:
    def __init__(self, args, product, role: str):
        self.args, self.product, self.role = args, product, role
        self.checks: list[dict] = []

    def check(self, name: str, ok: bool, found: str, fix: str = "") -> None:
        self.checks.append({"check": name, "ok": ok, "found": found,
                            "fix": "" if ok else fix})

    def run(self) -> list[dict]:
        self.tools()
        self.stores()
        self.trust()
        if self.role == "maintainer":
            self.signing_key()
            self.authenticode()
        if self.role in ("maintainer", "host"):
            self.checkout()
        if self.role == "host":
            self.index_key()
        return self.checks

    def tools(self) -> None:
        tools = [ON_WINDOWS.get(n, n) if WINDOWS else n
                 for n in TOOLS[self.role]]
        if WINDOWS:
            tools += WINDOWS_ONLY.get(self.role, ())
        for name in filter(None, tools):
            command = {"rclone": self.args.rclone,
                       "cosign": self.args.cosign,
                       "signtool": getattr(self.args, "signtool", None),
                       }.get(name) or name
            path = shutil.which(command)
            if name in SDK_TOOLS:
                try:
                    path = authenticode.find_sdk_tool(
                        name, "" if command == name else command)
                except ReleaseError:
                    path = None
            fix = ui.install_hint(name)
            if command != name:
                fix = f"{fix}; or point --{name} at it (now {command})"
            self.check(f"tool {name}", path is not None,
                       path or "not found", fix)

    def stores(self) -> None:
        for name in STORES[self.role]:
            root = getattr(self.args, name, None)
            if not root:
                self.check(f"store {name}", False, "not set",
                           f"set --{name}, GFR_{name.upper()} or [stores] "
                           f"{name} in gfrelease.toml")
                continue
            try:
                out = run([self.args.rclone or "rclone", "lsf",
                           "--max-depth", "1", root], check=False,
                          timeout=60)
            except ReleaseError as e:
                self.check(f"store {name}", False, str(e),
                           ui.hint(str(e)) or "")
                continue
            if out.returncode in (0, 3):  # 3: there, and still empty
                self.check(f"store {name}", True, root)
            else:
                detail = out.stderr.strip().splitlines()
                self.check(f"store {name}", False,
                           f"{root}: {detail[-1] if detail else 'no answer'}",
                           ui.hint(out.stderr) or
                           "check the remote with `rclone lsd <remote>:`")

    def trust(self) -> None:
        try:
            root, keys = self.product.trust
        except ReleaseError as e:
            self.check("trust", False, str(e),
                       "the root and index-keys list must verify "
                       "(gfrelease trust root verify)")
            return
        for doc, what, soon, renew in (
                (root, f"root {root['version']}", trust.ROOT_WARN_DAYS,
                 "trust root new"),
                (keys, f"index-keys {keys['version']}",
                 trust.INDEX_KEYS_WARN_DAYS, "trust keys renew")):
            left = trust.days_left(doc)
            self.check(what, left >= soon,
                       f"expires {doc['expires']}, in {left:.0f} days",
                       f"renew it: gfrelease {renew}")

    def signing_key(self) -> None:
        key = (self.args.key or "").upper() or self.product.signing_key
        cmd = ["gpg", "--batch", "--list-secret-keys", "--with-colons", key]
        if self.args.gnupghome:
            cmd[1:1] = ["--homedir", self.args.gnupghome]
        try:
            out = run(cmd, check=False, timeout=60)
        except ReleaseError as e:
            self.check("signing key", False, str(e), ui.hint(str(e)) or "")
            return
        pinned = key in self.product.maintainers
        self.check("signing key", out.returncode == 0 and pinned,
                   key if out.returncode == 0 else f"{key}: no secret key",
                   "the key must be in GnuPG (--gnupghome) and a pinned "
                   "maintainer's primary fingerprint" if out.returncode == 0
                   else ui.hint("No secret key"))

    def authenticode(self) -> None:
        cert, key = self.args.authenticode_cert, self.args.authenticode_key
        thumbprint = getattr(self.args, "authenticode_thumbprint", None)
        if WINDOWS and (thumbprint or not (cert or key)):
            self.store_certificate(thumbprint)
            return
        if not cert and not key:
            self.check("authenticode", False, "not set",
                       "set authenticode_cert and authenticode_key (files "
                       "or pkcs11: URIs) in gfrelease.toml")
            return
        missing = [v for v in (cert, key)
                   if v and not v.startswith("pkcs11:") and
                   not Path(v).expanduser().is_file()]
        token = any(v and v.startswith("pkcs11:") for v in (cert, key))
        module = self.args.pkcs11_module
        if token and not (module and Path(module).is_file()):
            missing.append(f"pkcs11_module {module or '(not set)'}")
        self.check("authenticode", not missing,
                   ", ".join(missing) or "certificate and key set",
                   "point authenticode_cert/key at the files, or set "
                   "pkcs11_module for a token")

    def store_certificate(self, thumbprint: str | None) -> None:
        """The certificate signtool is to sign with: in the user's store,
        for code signing, in date, its key known to the store, and the one
        the trust root pins."""
        if not thumbprint:
            self.check("authenticode", False, "not set",
                       "set authenticode_thumbprint in gfrelease.toml: the "
                       "SHA-1 thumbprint of the certificate, as certmgr.msc "
                       "shows it under Personal")
            return
        try:
            thumbprint = authenticode.SigntoolSigner(thumbprint).thumbprint
            found = json.loads(authenticode.powershell(
                _STORE_CERTIFICATE, GFR_THUMBPRINT=thumbprint) or "null")
        except (ReleaseError, ValueError) as e:
            self.check("authenticode", False, str(e), "")
            return
        if not found:
            self.check("authenticode", False,
                       f"{thumbprint}: not in your certificate store",
                       "import the certificate (the card's software does, "
                       "or certutil -addstore -user My <file>), or correct "
                       "authenticode_thumbprint")
            return
        subject = authenticode.certificate_subject(
            base64.b64decode(found["certificate"]))
        wrong = []
        if not found["code_signing"]:
            wrong.append("not a code-signing certificate")
        if not found["has_key"]:
            wrong.append("the store knows no key for it")
        if found["days_left"] <= 0:
            wrong.append("expired")
        if subject != authenticode._subject_rdns(
                self.product.authenticode_subject or "/"):
            wrong.append("not the subject the trust root pins "
                         f"({self.product.authenticode_subject})")
        self.check("authenticode", not wrong,
                   f"{thumbprint}: " + (", ".join(wrong) or
                                        f"expires in {found['days_left']} "
                                        "days"),
                   "certutil -repairstore -user My <thumbprint> binds a "
                   "card's key to its certificate; a renewed certificate "
                   "has a new thumbprint")

    def checkout(self) -> None:
        git_dir = self.args.git_dir
        if not git_dir or not Path(git_dir).exists():
            self.check("checkout", False, git_dir or "not set",
                       "set --git-dir to a clone of the product")
            return
        try:
            tags = release_tags(Path(git_dir))
        except ReleaseError as e:
            self.check("checkout", False, str(e), "")
            return
        self.check("checkout", bool(tags),
                   f"{git_dir}: {len(tags)} release tags",
                   "fetch the tags: git fetch --tags")
        ring = self.product.keyring_path
        self.check("maintainers' keys", ring.is_file(), str(ring),
                   "release/maintainers.asc must be in the checkout")

    def index_key(self) -> None:
        path = self.args.index_key
        if not path or not Path(path).is_file():
            self.check("index key", False, path or "not set",
                       "set --index-key (GFR_INDEX_KEY) to the minisign "
                       "secret key")
            return
        try:
            pub = minisig.public_key(self.args.index_pub or "")
            if pub:
                self.product.check_host_keys(pub)
            self.check("index key", True,
                       f"{path}" + (", listed in the trusted index-keys list"
                                    if pub else ""))
        except ReleaseError as e:
            self.check("index key", False, str(e),
                       "list its public key (gfrelease trust keys rotate)")


# The certificate $env:GFR_THUMBPRINT names in the user's store, or nothing.
_STORE_CERTIFICATE = r"""
$c = Get-ChildItem -LiteralPath Cert:\CurrentUser\My |
    Where-Object { $_.Thumbprint -eq $env:GFR_THUMBPRINT } |
    Select-Object -First 1
if ($c) {
    ConvertTo-Json -Compress -InputObject ([pscustomobject]@{
        certificate  = [Convert]::ToBase64String($c.RawData)
        has_key      = [bool]$c.HasPrivateKey
        days_left    = [int][Math]::Floor(($c.NotAfter - (Get-Date)).TotalDays)
        code_signing = [bool]($c.EnhancedKeyUsageList |
            Where-Object { $_.ObjectId -eq '1.3.6.1.5.5.7.3.3' })
    })
}
"""


def render(checks: list[dict]) -> list[str]:
    lines = []
    for c in checks:
        lines.append(f"{'✓' if c['ok'] else '✗'} {c['check']}: {c['found']}")
        if c["fix"]:
            lines.append(f"    fix: {c['fix']}")
    return lines
