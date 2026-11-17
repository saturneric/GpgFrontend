# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""Where a setting comes from, the first that has one:

  1. the command line (--builds r2:...)
  2. the environment, GFR_<NAME> (GFR_BUILDS=r2:...): how CI and the
     signing host's container set them
  3. the config file: ./gfrelease.toml, else
     $XDG_CONFIG_HOME/gfrelease/config.toml, as `name = "value"` lines
     (a [table] around them is allowed and means nothing)
  4. a default: this checkout for git_dir, the cache and state directories
     for work and state_dir
"""

from __future__ import annotations

import os
import tomllib
from pathlib import Path

from ..errors import ReleaseError
from ..paths import ROOT

# Every setting, each also an option (--name) and a variable (GFR_NAME).
KNOWN = (
    # The product and its trust.
    "product", "policy", "root_dir", "index_keys_dir", "root_sha256",
    # The stores and tools.
    "staging", "builds", "update", "git_dir", "rclone", "cosign", "cafile",
    # The maintainer's signing.
    "work", "key", "gnupghome", "authenticode_thumbprint", "signtool",
    "timestamp_url", "authenticode_cert", "authenticode_key",
    "authenticode_ca",
    # The signing host's.
    "state_dir", "index_key", "index_pub",
)

# The settings that name a file or directory: "~" is expanded in them.
PATHS = {"product", "policy", "root_dir", "index_keys_dir", "git_dir",
         "cafile", "work", "gnupghome", "signtool", "authenticode_cert",
         "authenticode_key", "authenticode_ca", "state_dir", "index_key"}


def _xdg(name: str, fallback: str) -> Path:
    value = os.environ.get(name)
    return Path(value) if value else Path.home() / fallback


def config_path() -> Path | None:
    for path in (Path.cwd() / "gfrelease.toml",
                 _xdg("XDG_CONFIG_HOME", ".config") / "gfrelease" /
                 "config.toml"):
        if path.is_file():
            return path
    return None


def load_config(path: Path | None) -> dict[str, str]:
    if path is None:
        return {}
    try:
        data = tomllib.loads(path.read_text())
    except (OSError, tomllib.TOMLDecodeError, UnicodeError) as e:
        raise ReleaseError(f"{path}: {e}") from e
    items = []
    for name, value in data.items():
        items += value.items() if isinstance(value, dict) else [(name, value)]
    found = {}
    for name, value in items:
        if name not in KNOWN:
            raise ReleaseError(f"{path}: {name} is not a setting")
        if not isinstance(value, (str, int, float)) or isinstance(value, bool):
            raise ReleaseError(f"{path}: {name} must be a string")
        found[name] = str(value)
    return found


def defaults() -> dict[str, str]:
    found = {
        "work": str(_xdg("XDG_CACHE_HOME", ".cache") / "gfrelease" / "stable"),
        "state_dir": str(_xdg("XDG_STATE_HOME", ".local/state") /
                         "gfrelease"),
    }
    if (ROOT / ".git").exists():
        found["git_dir"] = str(ROOT)
    return found


def fill(args) -> None:
    """Every setting @p args (argparse) has and left unset, from the
    environment, the config file or the defaults."""
    config = load_config(config_path())
    fallback = defaults()
    for name in KNOWN:
        if not hasattr(args, name) or getattr(args, name) is not None:
            continue
        value = os.environ.get(f"GFR_{name.upper()}") or config.get(name)
        if value and name in PATHS:
            value = str(Path(value).expanduser())
        setattr(args, name, value or fallback.get(name))
