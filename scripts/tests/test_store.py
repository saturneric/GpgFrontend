#!/usr/bin/env python3
# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""Store over local directories, through the real rclone and through
trustutil.local_rclone, which the other tests use in its place: whatever
one answers, the other must too.

Run: python3 -m unittest discover -s scripts/tests
"""

import shutil
import tempfile
import unittest
from pathlib import Path


import _paths  # noqa: E402,F401
from trustutil import local_rclone, tools  # noqa: E402

from gfrelease import common  # noqa: E402
from gfrelease.adapters.store import Store  # noqa: E402

TREE = {
    "top.txt": b"top\n",
    "a/release.json": b"{}\n",
    "a/b/staged.json": b"b\n",
    "a/c/x/staged.json": b"x\n",
    "a/c/x/other.json": b"o\n",
    "big.bin": b"z" * (common.SMALL_OBJECT_LIMIT + 1),
}


def outcome(call):
    """What @p call gave: its value, or the class of what it raised."""
    try:
        return call()
    except Exception as e:  # noqa: BLE001 -- compared, not handled
        return type(e).__name__


@unittest.skipUnless(tools("rclone"), "needs rclone")
class StoreContractTest(unittest.TestCase):

    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp(prefix="gfr-store-"))
        self.addCleanup(shutil.rmtree, self.tmp, True)

    def store(self, name: str) -> Store:
        root = self.tmp / name
        for rel, data in TREE.items():
            (root / rel).parent.mkdir(parents=True, exist_ok=True)
            (root / rel).write_bytes(data)
        (root / "empty").mkdir()
        return Store(str(root))

    def run_all(self, name: str) -> dict:
        """Every Store call the tooling makes, over a store of its own."""
        s, out = self.store(name), {}
        local = self.tmp / f"{name}-local"
        local.mkdir()
        for prefix, include in (("", None), ("a", None), ("missing", None),
                                ("empty", None), ("top.txt", None),
                                ("a", "*/staged.json"),
                                ("a", "**/staged.json"),
                                ("a", "*/*/staged.json"),
                                ("", "*/x/*.json")):
            out[f"files {prefix} {include}"] = outcome(
                lambda: s.files(prefix, include))
        for path in ("top.txt", "a", "a/c", "empty", "missing",
                     "a/missing.json", "missing/x.json"):
            out[f"exists {path}"] = outcome(lambda: s.exists(path))
        for path in ("top.txt", "big.bin", "missing", "a/missing.json"):
            out[f"read {path}"] = outcome(lambda: s.read(path))
            out[f"sha256 {path}"] = outcome(lambda: s.sha256(path))
            out[f"fetch_to {path}"] = outcome(lambda: s.fetch_to(
                path, local / "fetched" / path.replace("/", "_")).name)
        for prefix in ("a", "missing"):
            dest = local / f"dl-{prefix}"
            out[f"download {prefix}"] = outcome(
                lambda: (s.download(prefix, dest),
                         sorted(p.relative_to(dest).as_posix()
                                for p in dest.rglob("*") if p.is_file()))[1])
        source = local / "upload.txt"
        source.write_bytes(b"new\n")
        out["put new"] = outcome(lambda: s.put_file(source, "up/new.txt"))
        out["put same"] = outcome(lambda: s.put_file(source, "up/new.txt"))
        out["put other"] = outcome(lambda: s.put_file(source, "top.txt"))
        out["put mutable"] = outcome(
            lambda: s.put_file(source, "top.txt", immutable=False))
        out["put bytes"] = outcome(lambda: s.put_bytes(b"b", "up/b.txt"))
        out["after"] = outcome(lambda: (s.files(), s.read("top.txt"),
                                        s.read("up/b.txt")))
        return out

    def test_local_rclone_answers_as_rclone_does(self):
        real = self.run_all("real")
        patch = local_rclone()
        patch.start()
        self.addCleanup(patch.stop)
        local = self.run_all("local")
        for key in real:
            with self.subTest(key):
                self.assertEqual(local[key], real[key])


if __name__ == "__main__":
    unittest.main()
