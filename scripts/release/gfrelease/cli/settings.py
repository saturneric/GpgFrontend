# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""Where gfrelease's settings come from, first that has one wins:

  1. the option on the command line (--builds r2:...)
  2. the environment, as GFR_<OPTION> (GFR_BUILDS=r2:...): how the signing
     host's container and CI configure it
  3. a config file, gfrelease.toml: $GFR_CONFIG, else ./gfrelease.toml,
     else ~/.config/gfrelease/config.toml (XDG_CONFIG_HOME)
  4. a default: the checkout this tool is in for --git-dir, the cache and
     state directories for --work and --state-dir, rclone and cosign from
     PATH

The config file groups the settings by who needs them; a key in any group
works, a key gfrelease does not know is an error. `gfrelease config init`
writes one to fill in, `gfrelease config show` prints every setting, its
value and where it came from.

  [stores]      staging, builds, update (rclone paths, the product root
                included)
  [maintainer]  key, gnupghome, work, authenticode_cert, authenticode_key,
                pkcs11_module, authenticode_thumbprint, signtool,
                timestamp_url, authenticode_ca
  [host]        state_dir, index_key, index_pub
  [trust]       root_dir, index_keys_dir, root_sha256
  [tools]       git_dir, rclone, cosign, product, policy, cafile
"""

from __future__ import annotations

import difflib
import os
import tomllib
from pathlib import Path

from ..errors import ReleaseError
from ..paths import ROOT

GROUPS = {
    "stores": ("staging", "builds", "update"),
    "maintainer": ("key", "gnupghome", "work", "authenticode_cert",
                   "authenticode_key", "pkcs11_module",
                   "authenticode_thumbprint", "signtool", "timestamp_url",
                   "authenticode_ca"),
    "host": ("state_dir", "index_key", "index_pub"),
    "trust": ("root_dir", "index_keys_dir", "root_sha256"),
    "tools": ("git_dir", "rclone", "cosign", "product", "policy", "cafile"),
}
KNOWN = {name for names in GROUPS.values() for name in names}

TEMPLATE = """\
# gfrelease's settings (scripts/release/gfrelease/settings.py). An option on
# the command line, or GFR_<NAME> in the environment, wins over these.
# `gfrelease config show` prints what is in effect, and from where.

[stores]
# rclone paths, the product root included.
# staging = "r2:<staging bucket>"
# builds = "r2:<builds bucket>/<product>"
# update = "r2:<update bucket>/<product>"

[maintainer]
# Your signing key's fingerprint (default: the first pinned maintainer).
# key = "..."
# gnupghome = "~/.gnupg"
# Where `gfrelease stable` keeps its files and progress.
# work = "~/.cache/gfrelease/stable"
# The Authenticode certificate and key: files, or PKCS#11 URIs.
# authenticode_cert = "pkcs11:..."
# authenticode_key = "pkcs11:..."
# pkcs11_module = "/usr/lib/x86_64-linux-gnu/opensc-pkcs11.so"
# On Windows instead: the certificate in your store, by its SHA-1
# thumbprint, signed with by signtool (its key where the store says).
# authenticode_thumbprint = "..."
# signtool = "C:/Program Files (x86)/Windows Kits/10/bin/<version>/x64/signtool.exe"
# timestamp_url = "http://time.certum.pl/,http://timestamp.digicert.com"

[host]
# state_dir = "~/.local/state/gfrelease"
# index_key = "/run/secrets/index.key"

[tools]
# git_dir = "~/src/<product>"
# rclone = "rclone"
# cosign = "cosign"
"""


def _xdg(name: str, fallback: str) -> Path:
    value = os.environ.get(name)
    return Path(value) if value else Path.home() / fallback


def config_path() -> Path | None:
    """The config file in effect, if any. One named by GFR_CONFIG must
    exist."""
    named = os.environ.get("GFR_CONFIG")
    if named:
        path = Path(named).expanduser()
        if not path.is_file():
            raise ReleaseError(f"GFR_CONFIG names {path}, which is not a file")
        return path
    for path in (Path.cwd() / "gfrelease.toml",
                 _xdg("XDG_CONFIG_HOME", ".config") / "gfrelease" /
                 "config.toml"):
        if path.is_file():
            return path
    return None


def default_config_path() -> Path:
    return _xdg("XDG_CONFIG_HOME", ".config") / "gfrelease" / "config.toml"


def load_config(path: Path | None) -> dict[str, str]:
    """The settings of @p path, flat: name -> value."""
    if path is None:
        return {}
    try:
        data = tomllib.loads(path.read_text())
    except (OSError, tomllib.TOMLDecodeError, UnicodeError) as e:
        raise ReleaseError(f"{path}: {e}") from e
    found: dict[str, str] = {}
    problems = []
    for group, values in data.items():
        if not isinstance(values, dict):
            problems.append(f"{group} is not a [group]")
            continue
        if group not in GROUPS:
            problems.append(f"[{group}] is not a group: "
                            f"{', '.join(sorted(GROUPS))}{_like(group, GROUPS)}")
            continue
        for name, value in values.items():
            if name not in KNOWN:
                problems.append(f"[{group}] {name} is not a setting"
                                f"{_like(name, KNOWN)}")
            elif not isinstance(value, (str, int, float)) or \
                    isinstance(value, bool):
                problems.append(f"[{group}] {name} must be a string")
            else:
                found[name] = str(value)
    if problems:
        raise ReleaseError(f"{path}:\n" + "\n".join(problems))
    return found


def _like(word: str, choices) -> str:
    close = difflib.get_close_matches(word, list(choices), n=1)
    return f" (did you mean {close[0]}?)" if close else ""


def defaults() -> dict[str, str]:
    found = {
        "work": str(_xdg("XDG_CACHE_HOME", ".cache") / "gfrelease" / "stable"),
        "state_dir": str(_xdg("XDG_STATE_HOME", ".local/state") /
                         "gfrelease"),
    }
    if (ROOT / ".git").exists():
        found["git_dir"] = str(ROOT)
    return found


# Settings that name a file or directory: "~" is expanded in them.
PATHS = {"gnupghome", "work", "authenticode_ca", "state_dir", "index_key",
         "root_dir", "index_keys_dir", "git_dir", "product", "policy",
         "cafile", "pkcs11_module", "signtool"}


class Settings:
    """Every setting's value and where it came from, for one command."""

    def __init__(self):
        self.environ = os.environ
        self.path = config_path()
        self.config = load_config(self.path)
        self.defaults = defaults()

    def lookup(self, name: str) -> tuple[str | None, str | None]:
        """(value, source) from the environment, the config file or the
        defaults; (None, None) when none has it."""
        env = self.environ.get(f"GFR_{name.upper()}")
        if env:
            return env, f"GFR_{name.upper()}"
        if name in self.config:
            value = self.config[name]
            if name in PATHS:
                value = str(Path(value).expanduser())
            return value, str(self.path)
        if name in self.defaults:
            return self.defaults[name], "default"
        return None, None

    def fill(self, args, names) -> dict[str, str]:
        """Every one of @p names that @p args (argparse) left unset, from
        lookup(); returns name -> source for each value args now holds."""
        sources = {}
        for name in names:
            if getattr(args, name, None) is not None:
                sources[name] = "command line"
                continue
            value, source = self.lookup(name)
            if value is not None:
                setattr(args, name, value)
                sources[name] = source
        return sources
