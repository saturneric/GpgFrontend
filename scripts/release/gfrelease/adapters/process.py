# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""Running a tool: to its end, within a time limit, its failure a
ReleaseError and never a traceback."""

from __future__ import annotations

import os
import subprocess
from pathlib import Path
from typing import Callable

from ..errors import ReleaseError, ToolFailed, ToolTimeout

# The maintainer's stage also runs on Windows, where the Authenticode token's
# own software is; the signing host and CI are Linux.
WINDOWS = os.name == "nt"

# The longest any one tool (rclone, git, gpg, minisign, cosign, cmake) may
# run: a transfer that hangs ends as a failed step, and the next round tries
# again, rather than holding the host's lock for ever.
TOOL_TIMEOUT = 3600

# Told of every tool about to run (the command line's --verbose sets it).
on_start: Callable[[list], None] | None = None


def run(cmd, **kw) -> subprocess.CompletedProcess:
    """A tool, run to its end: a failure, a missing tool or one that does
    not finish (ToolTimeout) is a ReleaseError, never a traceback."""
    kw.setdefault("check", True)
    kw.setdefault("timeout", TOOL_TIMEOUT)
    kw.setdefault("capture_output", True)
    kw.setdefault("text", True)
    if kw["text"]:
        # What a tool prints can carry bytes from untrusted input (a user ID
        # in a forged signature, a file name): never a UnicodeDecodeError.
        kw.setdefault("encoding", "utf-8")
        kw.setdefault("errors", "replace")
    if on_start is not None:
        on_start(cmd)
    try:
        return subprocess.run([str(c) for c in cmd], **kw)
    except subprocess.CalledProcessError as e:
        # check=True: the caller needed the tool to work, not to judge.
        detail = (e.stderr or e.stdout or "").strip()
        raise ToolFailed(f"{Path(str(cmd[0])).name} failed: {detail}") from e
    except subprocess.TimeoutExpired as e:
        raise ToolTimeout(f"{Path(str(cmd[0])).name} did not finish within "
                          f"{kw['timeout']} s") from e
    except FileNotFoundError as e:
        raise ToolFailed(f"{cmd[0]} is not installed") from e
    except OSError as e:
        raise ToolFailed(f"{cmd[0]} cannot be run: {e}") from e


def run_with_secret(cmd: list, secret: bytes,
                    **kw) -> subprocess.CompletedProcess:
    """run() of @p cmd, where an item "{secret}" is the path of a pipe
    holding @p secret: never on a command line or on disk, and a key given
    as <(...) (read once) works. Not on Windows, which has no such path."""
    if WINDOWS:
        raise ReleaseError(f"{Path(str(cmd[0])).name} takes a secret through "
                           "a pipe Windows cannot name")
    r, w = os.pipe()
    try:
        os.write(w, secret)  # a key or a PIN: never fills the pipe
        os.close(w)
        w = -1
        return run([f"/dev/fd/{r}" if c == "{secret}" else c for c in cmd],
                   pass_fds=(r,), **kw)
    finally:
        os.close(r)
        if w >= 0:
            os.close(w)
