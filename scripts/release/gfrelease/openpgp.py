# Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""OpenPGP for stable releases, by gpg itself.

Signing happens on the maintainer's machine with their own key (a signing
subkey is fine): detached, binary `.sig` files, the form every release has
carried. Verifying happens anywhere, against nothing but the maintainers'
public keys: a throwaway keyring is made from release/maintainers.asc for
each check, and a signature counts only when gpg calls it valid (VALIDSIG)
and the key it chains to has a primary fingerprint the product pins. Trust
settings, the caller's own keyring and an agent play no part.
"""

from __future__ import annotations

import os
import tempfile
from pathlib import Path

from common import ReleaseError, run


def sign(file: Path, fingerprint: str, gnupghome: str | None = None) -> Path:
    """`file.sig` beside @p file, made with the key @p fingerprint names."""
    sig = file.with_name(file.name + ".sig")
    cmd = ["gpg", "--batch", "--yes", "--detach-sign",
           "--local-user", f"{fingerprint}!", "--output", sig, file]
    if gnupghome:
        cmd[1:1] = ["--homedir", gnupghome]
    run(cmd)
    return sig


def _signatures(status: str) -> list[dict]:
    """Every signature a gpg status stream reports, one dict each: how gpg
    judged it (GOODSIG, EXPKEYSIG, REVKEYSIG, ...) and, when it is
    cryptographically valid, its VALIDSIG facts."""
    sigs: list[dict] = []
    for line in status.splitlines():
        parts = line.split()
        if len(parts) < 2 or parts[0] != "[GNUPG:]":
            continue
        word = parts[1]
        if word == "NEWSIG":
            sigs.append({})
            continue
        if not sigs:  # a stream without NEWSIG (older gpg): one signature
            sigs.append({})
        if word in ("GOODSIG", "EXPSIG", "EXPKEYSIG", "REVKEYSIG", "BADSIG",
                    "ERRSIG"):
            sigs[-1]["judgement"] = word
        elif word == "VALIDSIG" and len(parts) >= 5:
            # VALIDSIG <fpr> <date> <ts> <expire> <ver> <rsv> <algo> <hash>
            #          <class> [<primary-fpr>]
            sigs[-1]["signer"] = parts[2].upper()
            sigs[-1]["created"] = int(parts[4]) if parts[4].isdigit() else 0
            sigs[-1]["primary"] = (parts[11] if len(parts) > 11
                                   else parts[2]).upper()
    return [s for s in sigs if s]


class Keyring:
    """The maintainers' public keys, imported fresh into a throwaway home."""

    def __init__(self, keys: Path, pinned: list[str]):
        if not Path(keys).is_file():
            raise ReleaseError(f"{keys}: no maintainer keys to verify against")
        if not pinned:
            raise ReleaseError("no maintainer fingerprint is pinned")
        self.keys = Path(keys)
        self.pinned = {f.upper() for f in pinned}

    def __enter__(self) -> "Keyring":
        self._tmp = tempfile.TemporaryDirectory(prefix="gfr-gpg-")
        self.home = self._tmp.name
        os.chmod(self.home, 0o700)
        try:
            run(["gpg", "--homedir", self.home, "--batch", "--quiet",
                 "--import", self.keys])
        except BaseException:
            # __exit__ never runs for a failed __enter__.
            self.__exit__(None, None, None)
            raise
        return self

    def __exit__(self, *exc) -> None:
        run(["gpgconf", "--homedir", self.home, "--kill", "all"], check=False)
        self._tmp.cleanup()

    def _key_state(self, fingerprint: str) -> tuple[int | None, bool]:
        """When the (sub)key @p fingerprint expires (None for never), and
        whether it or its primary is revoked, as gpg has them. gpg reports
        a key that is both expired and revoked as EXPKEYSIG: the revocation
        is read here."""
        out = run(["gpg", "--homedir", self.home, "--batch", "--with-colons",
                   "--list-keys", fingerprint], check=False)
        expiry, validity, primary_validity = None, "", ""
        for line in out.stdout.splitlines():
            fields = line.split(":")
            if fields[0] in ("pub", "sub"):
                expiry, validity = fields[6], fields[1]
                if fields[0] == "pub":
                    primary_validity = fields[1]
            elif fields[0] == "fpr" and len(fields) > 9 and \
                    fields[9].upper() == fingerprint:
                return (int(expiry) if expiry and expiry.isdigit() else None,
                        "r" in (validity, primary_validity))
        return None, False

    def _accept(self, status: str, what: str) -> dict:
        """The pinned primary fingerprint of a signature gpg found valid, made
        by a key that was good when it signed:

          GOODSIG    accepted;
          EXPKEYSIG  accepted only when the signature was made before the key
                     expired (it signed in time; it has expired since);
          REVKEYSIG  refused: a revoked key may be a compromised one, and
                     its signatures prove nothing any more;
          EXPSIG     refused: the signature itself has expired.

        Every signature in the file is looked at, not just the first."""
        reasons = []
        for sig in _signatures(status):
            primary = sig.get("primary")
            if primary is None:
                continue
            if primary not in self.pinned:
                reasons.append(f"signed by {primary}, which is not a pinned "
                               "maintainer key")
                continue
            judgement = sig.get("judgement")
            if judgement == "GOODSIG":
                return sig
            if judgement == "EXPKEYSIG":
                expiry, revoked = self._key_state(sig["signer"])
                if revoked:
                    reasons.append(f"signed by {primary}, whose key is "
                                   "revoked")
                    continue
                if expiry is not None and sig["created"] < expiry:
                    return sig
                reasons.append(f"signed by {primary} after its key expired")
            elif judgement == "REVKEYSIG":
                reasons.append(f"signed by {primary}, whose key is revoked")
            else:
                reasons.append(f"signed by {primary}, but gpg says "
                               f"{judgement or 'nothing'}")
        if not reasons:
            raise ReleaseError(f"{what}: no valid signature by a maintainer key")
        raise ReleaseError(f"{what}: {'; '.join(reasons)}")

    def verify(self, data: Path, asc: Path) -> str:
        """The pinned primary fingerprint that signed @p data, or an error."""
        return self.verify_signature(data, asc)["primary"]

    def verify_signature(self, data: Path, asc: Path) -> dict:
        """The accepted signature on @p data: its pinned primary
        fingerprint ("primary") and when it was made ("created", Unix
        seconds), or an error."""
        out = run(["gpg", "--homedir", self.home, "--batch", "--status-fd", "1",
                   "--verify", asc, data], check=False)
        if out.returncode != 0:
            raise ReleaseError(f"{Path(data).name}: the signature does not verify")
        sig = self._accept(out.stdout, Path(data).name)
        return {"primary": sig["primary"], "created": sig["created"]}

    def verify_tag(self, git_dir: Path, tag: str) -> str:
        """A signed annotated tag, by a pinned maintainer key."""
        # gpg itself, and nothing the user's or the system's git config says
        # (a minimum trust level, another program): the pinning below
        # decides.
        env = dict(os.environ, GNUPGHOME=self.home,
                   GIT_CONFIG_GLOBAL=os.devnull, GIT_CONFIG_NOSYSTEM="1")
        out = run(["git", "-c", "gpg.program=gpg",
                   "-c", "gpg.minTrustLevel=undefined", "-C", git_dir,
                   "verify-tag", "--raw", tag], env=env, check=False)
        # git also refuses what gpg judged acceptable here (a tag a key made
        # before it expired) or not at all: the status lines decide, by the
        # same rules as for a file.
        try:
            return self._accept(out.stderr, f"tag {tag}")["primary"]
        except ReleaseError as e:
            raise ReleaseError(f"tag {tag} is not signed by a maintainer "
                               f"key ({e})") from e
