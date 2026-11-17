# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""What fixes a problem: a tool to install, a setting to make. The command
line adds one to an error it prints (cli/ui.py), doctor to a check that
failed."""

from __future__ import annotations

import os
import re
from pathlib import Path

from ..paths import RELEASE_DIR


def _packages() -> dict[str, str]:
    """scripts/release/tools.txt: command -> Debian/Ubuntu package."""
    found = {}
    try:
        text = (RELEASE_DIR / "tools.txt").read_text("utf-8")
    except OSError:
        return found
    for line in text.splitlines():
        parts = line.split()
        if len(parts) == 2 and not line.startswith("#"):
            found[parts[0]] = parts[1]
    return found


# The same tools on Windows, where the maintainer's stage also runs: what
# winget calls them.
WINGET = {"git": "Git.Git", "gpg": "GnuPG.Gpg4win", "gpgconf": "GnuPG.Gpg4win",
          "rclone": "Rclone.Rclone", "cosign": "Sigstore.Cosign",
          "cmake": "Kitware.CMake", "minisign": "jedisct1.minisign"}


def install_hint(command: str) -> str:
    name = Path(command).name
    if os.name == "nt":
        if name.lower().endswith(".exe"):
            name = name[:-4]
        if name == "signtool":
            return ("install the Windows SDK's signing tools, or point "
                    "--signtool at signtool.exe")
        if name in ("makeappx", "makepri"):
            return (f"install the Windows SDK, which brings {name}.exe, or "
                    "put it on PATH")
        if name == "cosign":
            # winget installs it under the name of its release file.
            return ("install cosign: winget install Sigstore.Cosign, and "
                    "set cosign = \"cosign-windows-amd64\" in gfrelease.toml")
        package = WINGET.get(name)
        return f"install it: winget install {package}" if package else \
            f"install {name}, or point gfrelease at it"
    if name == "cosign":
        return ("install cosign: https://docs.sigstore.dev/cosign/"
                "system_config/installation/")
    package = _packages().get(name)
    return f"install it: apt install {package}" if package else \
        f"install {name}, or point gfrelease at it"


HINTS: list[tuple[re.Pattern, object]] = [
    (re.compile(r"(\S+) is not installed"),
     lambda m: install_hint(m.group(1))),
    (re.compile(r"didn't find section in config file|"
                r"not found in config file|NOTICE: Config file .* not found",
                re.I),
     "the rclone remote is not configured: `rclone listremotes` shows "
     "what is (RCLONE_CONFIG names the file)"),
    (re.compile(r"another run holds (\S+)"),
     lambda m: "another gfrelease command of this host is running; wait "
               "for it" + ("" if os.name == "nt" else
                           f" (`fuser {m.group(1)}` shows which)")),
    (re.compile(r"No secret key|secret key not available|"
                r"is not a current pinned maintainer", re.I),
     "check `gpg --list-secret-keys` (and --gnupghome): the key must be a "
     "pinned maintainer's primary fingerprint"),
    (re.compile(r"^missing --", re.M),
     "set them once in gfrelease.toml (`gfrelease config init`), or as "
     "GFR_* variables"),
    (re.compile(r"did not finish within|did not answer|reading .*: |"
                r"listing .*: |writing .*: "),
     "a store or tool did not answer: run it again; `gfrelease doctor` "
     "checks the tools and stores"),
]


def hint(text: str) -> str | None:
    for pattern, answer in HINTS:
        m = pattern.search(text)
        if m:
            return answer(m) if callable(answer) else answer
    return None
