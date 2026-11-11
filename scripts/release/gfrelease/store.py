# Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""A place files live: an R2 bucket prefix in production, a directory in a
rehearsal. Both through rclone, so a rehearsal runs exactly the production
code path.

A root is any rclone path: "r2:bktus-builds/<slug>", "/srv/rehearsal/builds".
For R2 the remote comes from rclone's environment configuration
(RCLONE_CONFIG_<NAME>_TYPE=s3 and so on), never from a file in the repo.
"""

from __future__ import annotations

import hashlib
import json
import subprocess
import tempfile
import threading
from pathlib import Path

from common import (SMALL_OBJECT_LIMIT, TOOL_TIMEOUT, ReleaseError,
                    ToolTimeout, check_relpath, run, sha256_file)


# rclone's exit status for "directory not found" / "file not found": the only
# answers that mean an object is absent. Anything else (a 403, a network
# failure) is an error, and must never pass for "not there": the immutability
# check below would then upload over whatever it could not see.
_NOT_FOUND = (3, 4)


class StoreUnavailable(ReleaseError):
    """The store could not be asked (a network failure, a 403, an outage):
    nothing about the object is known. Unlike an object that is not there,
    which is an answer."""

    passing = True  # common.is_passing: the next run may well be answered


class Store:
    def __init__(self, root: str, rclone: str = "rclone"):
        if not root:
            raise ReleaseError("a store root is required")
        self.root = root.rstrip("/")
        self.rclone = rclone

    def _at(self, path: str = "") -> str:
        return f"{self.root}/{check_relpath(path)}" if path else self.root

    def __str__(self) -> str:
        return self.root

    def _rclone(self, *args, **kw):
        """rclone, its exit status left to the caller. One that does not
        finish says nothing about the object: StoreUnavailable."""
        try:
            return run([self.rclone, *args], check=False, **kw)
        except ToolTimeout as e:
            raise StoreUnavailable(f"{self.root}: {e}") from e

    def files(self, prefix: str = "", include: str | None = None) -> list[str]:
        """Every file under prefix, recursively, relative to it."""
        filters = ["--include", include] if include else []
        out = self._rclone("lsf", *filters, "--recursive", "--files-only",
                           self._at(prefix))
        if out.returncode in _NOT_FOUND:
            return []
        if out.returncode != 0:
            raise StoreUnavailable(f"listing {self._at(prefix)}: "
                                   f"{out.stderr.strip()}")
        return sorted(line for line in out.stdout.splitlines() if line)

    def exists(self, path: str) -> bool:
        out = self._rclone("lsjson", "--files-only", self._at(path))
        if out.returncode in _NOT_FOUND:
            return False
        if out.returncode != 0:
            raise StoreUnavailable(f"looking up {self._at(path)}: "
                                   f"{out.stderr.strip()}")
        try:
            return json.loads(out.stdout or "[]") != []
        except ValueError as e:
            raise StoreUnavailable(f"looking up {self._at(path)}: rclone "
                                   f"answered something else ({e})") from e

    def _get(self, args: list, what: str):
        """An rclone read: not found is an answer (a ReleaseError), any
        other failure is StoreUnavailable."""
        out = self._rclone(*args, text=False)
        if out.returncode == 0:
            return out
        detail = out.stderr.decode("utf-8", "replace").strip()
        if out.returncode in _NOT_FOUND:
            raise ReleaseError(f"{what}: not found ({detail})")
        raise StoreUnavailable(f"reading {what}: {detail}")

    def read(self, path: str) -> bytes:
        """A small object (a descriptor, the index, a manifest, a signature,
        notes), whole. Anything larger is refused unread: these come from
        buckets others write to, and are held in memory."""
        data = self._get(["cat", "--count", str(SMALL_OBJECT_LIMIT + 1),
                          self._at(path)], self._at(path)).stdout
        if len(data) > SMALL_OBJECT_LIMIT:
            raise ReleaseError(f"{self._at(path)}: larger than "
                               f"{SMALL_OBJECT_LIMIT} bytes")
        return data

    def download(self, prefix: str, local: Path) -> None:
        """Everything under @p prefix, into @p local."""
        out = self._rclone("copy", self._at(prefix), str(local))
        if out.returncode in _NOT_FOUND:
            raise ReleaseError(f"{self._at(prefix)}: not found "
                               f"({out.stderr.strip()})")
        if out.returncode != 0:
            raise StoreUnavailable(f"reading {self._at(prefix)}: "
                                   f"{out.stderr.strip()}")

    def fetch_to(self, path: str, local: Path) -> Path:
        """One object, streamed to @p local: an installer is never held in
        memory whole."""
        self._get(["copyto", self._at(path), str(local)], self._at(path))
        if not Path(local).is_file():
            raise ReleaseError(f"{self._at(path)}: nothing was read")
        return Path(local)

    def sha256(self, path: str) -> str:
        """Of the stored bytes themselves, streamed: no backend's own hash
        (an S3 ETag is not one for a multipart upload) is trusted for it."""
        h = hashlib.sha256()
        with tempfile.TemporaryFile() as err:
            try:
                proc = subprocess.Popen([self.rclone, "cat", self._at(path)],
                                        stdout=subprocess.PIPE, stderr=err)
            except OSError as e:
                raise ReleaseError(f"{self.rclone} cannot be run: {e}") from e
            # A read that stalls is cut off like any other tool (rclone has
            # no deadline of its own for a whole transfer).
            stalled = threading.Timer(TOOL_TIMEOUT, proc.kill)
            stalled.start()
            try:
                with proc:
                    for chunk in iter(lambda: proc.stdout.read(1 << 20), b""):
                        h.update(chunk)
            finally:
                stalled.cancel()
            if proc.returncode != 0:
                err.seek(0)
                detail = err.read().decode("utf-8", "replace").strip()
                raise StoreUnavailable(f"reading {self._at(path)} failed: "
                                       f"{detail}")
        return h.hexdigest()

    def put_file(self, local: Path, path: str, immutable: bool = True) -> None:
        """Immutable by default: an existing object with other bytes is an
        error, the same bytes are skipped. What makes a rerun safe and a
        replaced artifact impossible.

        Checked here rather than left to `rclone --immutable`, which
        `copyto` ignores (it overwrote a different file in testing). The
        check and the upload are two steps: what keeps another writer from
        coming between them is that every path has one writer. The host's
        runs are serialized by its lock; the maintainer's stage and CI write
        other paths (releases/<version>/, which the host only reads,
        the staging store per version)."""
        if immutable and self.exists(path):
            if self.sha256(path) != sha256_file(local):
                raise ReleaseError(f"{self._at(path)} exists with other bytes; "
                                   "a published object is never replaced")
            return
        out = self._rclone("copyto", str(local), self._at(path))
        if out.returncode != 0:
            raise StoreUnavailable(f"writing {self._at(path)}: "
                                   f"{out.stderr.strip()}")

    def put_bytes(self, data: bytes, path: str, immutable: bool = True) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            local = Path(tmp) / "object"
            local.write_bytes(data)
            self.put_file(local, path, immutable)
