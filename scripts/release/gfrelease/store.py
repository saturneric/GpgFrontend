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
from pathlib import Path

from common import ReleaseError, check_relpath, run, sha256_file


# rclone's exit status for "directory not found" / "file not found": the only
# answers that mean an object is absent. Anything else (a 403, a network
# failure) is an error, and must never pass for "not there": the immutability
# check below would then upload over whatever it could not see.
_NOT_FOUND = (3, 4)


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

    def files(self, prefix: str = "", include: str | None = None) -> list[str]:
        """Every file under prefix, recursively, relative to it."""
        filters = ["--include", include] if include else []
        out = run([self.rclone, "lsf", *filters, "--recursive", "--files-only",
                   self._at(prefix)], check=False)
        if out.returncode in _NOT_FOUND:
            return []
        if out.returncode != 0:
            raise ReleaseError(f"listing {self._at(prefix)}: {out.stderr.strip()}")
        return sorted(line for line in out.stdout.splitlines() if line)

    def exists(self, path: str) -> bool:
        out = run([self.rclone, "lsjson", "--files-only", self._at(path)],
                  check=False)
        if out.returncode in _NOT_FOUND:
            return False
        if out.returncode != 0:
            raise ReleaseError(f"looking up {self._at(path)}: {out.stderr.strip()}")
        return json.loads(out.stdout or "[]") != []

    def read(self, path: str) -> bytes:
        out = run([self.rclone, "cat", self._at(path)], text=False)
        return out.stdout

    def download(self, prefix: str, local: Path) -> None:
        run([self.rclone, "copy", self._at(prefix), str(local)])

    def fetch_to(self, path: str, local: Path) -> Path:
        """One object, streamed to @p local: an installer is never held in
        memory whole."""
        run([self.rclone, "copyto", self._at(path), str(local)])
        if not Path(local).is_file():
            raise ReleaseError(f"{self._at(path)}: nothing was read")
        return Path(local)

    def sha256(self, path: str) -> str:
        """Of the stored bytes themselves, streamed: no backend's own hash
        (an S3 ETag is not one for a multipart upload) is trusted for it."""
        h = hashlib.sha256()
        with tempfile.TemporaryFile() as err:
            with subprocess.Popen([self.rclone, "cat", self._at(path)],
                                  stdout=subprocess.PIPE,
                                  stderr=err) as proc:
                for chunk in iter(lambda: proc.stdout.read(1 << 20), b""):
                    h.update(chunk)
            if proc.returncode != 0:
                err.seek(0)
                detail = err.read().decode("utf-8", "replace").strip()
                raise ReleaseError(f"reading {self._at(path)} failed: {detail}")
        return h.hexdigest()

    def put_file(self, local: Path, path: str, immutable: bool = True) -> None:
        """Immutable by default: an existing object with other bytes is an
        error, the same bytes are skipped. What makes a rerun safe and a
        replaced artifact impossible.

        Checked here rather than left to `rclone --immutable`, which
        `copyto` ignores (it overwrote a different file in testing). Runs on
        one store are serialized (the host's lock; staging paths are per
        version), so the check and the upload cannot interleave with
        another writer of the same path."""
        if immutable and self.exists(path):
            if self.sha256(path) != sha256_file(local):
                raise ReleaseError(f"{self._at(path)} exists with other bytes; "
                                   "a published object is never replaced")
            return
        run([self.rclone, "copyto", str(local), self._at(path)])

    def put_bytes(self, data: bytes, path: str, immutable: bool = True) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            local = Path(tmp) / "object"
            local.write_bytes(data)
            self.put_file(local, path, immutable)
