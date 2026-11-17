# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""Local files: hashing one, replacing one whole, and the host's lock."""

from __future__ import annotations

import contextlib
import hashlib
import os
from pathlib import Path

from ..domain import layout
from ..errors import ReleaseError
from .process import WINDOWS

if WINDOWS:
    import msvcrt
else:
    import fcntl


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with Path(path).open("rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def atomic_write(path: Path, data: bytes | str) -> None:
    """Replaces @p path whole or not at all: a crash leaves the old file or
    the new one, never a truncated one."""
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    tmp = path.with_name(path.name + ".new")
    with open(tmp, "wb") as f:
        f.write(data.encode("utf-8") if isinstance(data, str) else data)
        f.flush()
        os.fsync(f.fileno())
    tmp.replace(path)


@contextlib.contextmanager
def locked(state_dir: Path):
    """One run at a time per host: promote and index both write the
    same state directory, and the stores through it. A second run is
    refused at once.

    The lock is this host's only. The maintainer's `stable`
    and CI's `stage` write the stores from other machines, each under paths
    of its own (releases/<version>/, branches/, tags/)."""
    state_dir = Path(state_dir)
    state_dir.mkdir(parents=True, exist_ok=True)
    lock = state_dir / layout.STATE_LOCK
    with open(lock, "a") as f:
        try:
            if WINDOWS:
                # A range is locked from the file's position: always byte 0,
                # which need not exist. Released when the file closes.
                f.seek(0)
                msvcrt.locking(f.fileno(), msvcrt.LK_NBLCK, 1)
            else:
                fcntl.flock(f, fcntl.LOCK_EX | fcntl.LOCK_NB)
        except (BlockingIOError, PermissionError):
            raise ReleaseError(f"another run holds {lock}")
        yield
