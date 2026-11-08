#!/usr/bin/env python3
# Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""The one way the release tooling asks which version a commit is:
cmake/GitVersion.cmake, the derivation the build itself uses.

  gitversion.py [--repo DIR] [--rev COMMIT] [--ref REF] [--field FIELD]
"""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent.parent
SCRIPT = ROOT / "cmake" / "GitVersion.cmake"


def derive(repo: Path, ref: str = "", rev: str = "",
           field: str = "version") -> str:
    """@p field of the version git gives @p rev (the work tree's HEAD when
    empty) on @p ref. Raises subprocess.CalledProcessError, or
    FileNotFoundError without cmake."""
    cmd = ["cmake", f"-DGF_SOURCE_DIR={repo}", f"-DGF_FIELD={field}"]
    if rev:
        cmd.append(f"-DGPGFRONTEND_VERSION_REV={rev}")
    if ref:
        cmd.append(f"-DGPGFRONTEND_VERSION_REF={ref}")
    cmd += ["-P", str(SCRIPT)]
    return subprocess.run(cmd, check=True, capture_output=True,
                          text=True).stdout.strip()


def main(argv: list[str]) -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--repo", type=Path, default=ROOT)
    ap.add_argument("--rev", default="")
    ap.add_argument("--ref", default="")
    ap.add_argument("--field", default="version")
    args = ap.parse_args(argv)
    try:
        print(derive(args.repo, args.ref, args.rev, args.field))
    except subprocess.CalledProcessError as e:
        print(f"gitversion: {(e.stderr or '').strip()}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
