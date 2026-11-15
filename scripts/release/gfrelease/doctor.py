# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""gfrelease doctor: whether this machine can do its part, before it tries.

  maintainer  the tools of the offline stage, the stores it reads and
              writes, the signing key in GnuPG, the Authenticode token or
              files, the checkout with its release tags and keyring
  ci          the tools stage needs, and the staging store
  host        the signing host's tools, every store, the index key and
              that the trusted index-keys list holds it
  (any)       the trust: root and index-keys list verify, and how long each
              has left

Each check prints ✓ or ✗ with what it found, and for a ✗ how to fix it.
Exit status 1 when any check failed.
"""

from __future__ import annotations

import shutil
from pathlib import Path

import minisig
import trust
import ui
from common import ReleaseError, run

ROLES = ("maintainer", "ci", "host")
TOOLS = {
    "maintainer": ("git", "gpg", "rclone", "cosign", "minisign",
                   "osslsigncode"),
    "ci": ("git", "rclone", "cosign"),
    "host": ("git", "gpg", "rclone", "cosign", "minisign", "osslsigncode"),
}
STORES = {
    "maintainer": ("staging", "builds"),
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
        for name in TOOLS[self.role]:
            command = {"rclone": self.args.rclone,
                       "cosign": self.args.cosign}.get(name) or name
            path = shutil.which(command)
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
                (root, f"root {root['version']}", 60, "trust root new"),
                (keys, f"index-keys {keys['version']}", 30,
                 "trust keys renew")):
            left = trust.days_left(doc)
            self.check(what, left > soon,
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

    def checkout(self) -> None:
        git_dir = self.args.git_dir
        if not git_dir or not Path(git_dir).exists():
            self.check("checkout", False, git_dir or "not set",
                       "set --git-dir to a clone of the product")
            return
        try:
            tags = run(["git", "-C", git_dir, "tag", "-l", "v*"],
                       timeout=60).stdout.split()
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
            pub = minisig.public_key_of(Path(self.args.index_pub)) \
                if self.args.index_pub and Path(self.args.index_pub).is_file()\
                else self.args.index_pub
            if pub:
                self.product.check_host_keys(pub)
            self.check("index key", True,
                       f"{path}" + (", listed in the trusted index-keys list"
                                    if pub else ""))
        except ReleaseError as e:
            self.check("index key", False, str(e),
                       "list its public key (gfrelease trust keys rotate)")


def render(checks: list[dict]) -> list[str]:
    lines = []
    for c in checks:
        lines.append(f"{'✓' if c['ok'] else '✗'} {c['check']}: {c['found']}")
        if c["fix"]:
            lines.append(f"    fix: {c['fix']}")
    return lines
