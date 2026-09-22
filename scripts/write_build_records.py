#!/usr/bin/env python3
# Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""Write one build record per published artifact, from the artifacts themselves.

A record answers two questions, and they need different evidence:

  which source, build and module set produced this
      -> product.*, modules[]   (metadata, and module ENTRY identity)

  is this file I downloaded one of the files that build produced
      -> artifacts[]            (a digest of the file itself)

## Why this runs downstream, and what that deleted

Records used to be written by each build and signing job, which put release
attestation inside the build and package path. That forced a two-stage
unsigned/signed protocol, because a record written before signing described
bytes that signing then changed -- and it forced a cross-stage check that the
two digests differed, to catch a record describing the other stage's file.

Hashing the FINAL artifact cannot have that problem, so neither the stages nor
the check exist. There is one record per file that ships.

Nothing at runtime reads any of this. Module trust rests on the seed-signed
descriptors and the trust root compiled into the Host; provenance is a
separate, public, release-only layer and is not a trust input.

usage: write_build_records.py --artifacts DIR --build-info DIR --out DIR
"""

import argparse
import hashlib
import json
import sys
from pathlib import Path


def digest(path):
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def load_infos(info_root):
    """One build-info per leg that produced one, keyed by its directory name.

    The directory is named for the OS it was built on, which is as much as a
    downloaded artifact directory can be matched on.
    """
    infos = {}
    for path in sorted(info_root.rglob("build-info.json")):
        with path.open(encoding="utf-8") as f:
            infos[path.parent.name] = json.load(f)
    return infos


def info_for(leg_dir, infos):
    """The build info describing this leg.

    Linux and Windows ship build-info.json inside their own artifact, so it is
    simply there. macOS cannot: its artifact is produced by the signing job,
    which is forbidden to carry anything from the build side, so that leg
    uploads the file separately and it is matched by name.
    """
    local = leg_dir / "build-info.json"
    if local.is_file():
        with local.open(encoding="utf-8") as f:
            return json.load(f)

    best, best_len = None, -1
    for key, value in infos.items():
        tail = key[len("buildinfo-"):] if key.startswith("buildinfo-") else key
        if tail and tail in leg_dir.name and len(tail) > best_len:
            best, best_len = value, len(tail)
    return best


def main():
    parser = argparse.ArgumentParser(
        description="Write one build record per published artifact.")
    parser.add_argument("--artifacts", required=True, type=Path)
    parser.add_argument("--build-info", required=True, type=Path)
    parser.add_argument("--out", required=True, type=Path)
    args = parser.parse_args()

    args.out.mkdir(parents=True, exist_ok=True)
    infos = load_infos(args.build_info)

    written = 0
    for leg_dir in sorted(p for p in args.artifacts.iterdir() if p.is_dir()):
        files = sorted(
            p for p in leg_dir.iterdir()
            if p.is_file() and p.name.startswith("GpgFrontend-")
        )
        if not files:
            continue

        info = info_for(leg_dir, infos)
        if info is None:
            sys.exit(f"write_build_records: no build info for leg {leg_dir.name}")

        record = {
            "schema": 3,
            "leg": leg_dir.name,
            "product": {
                "version": info["project_version"],
                "build_id": info["build_id"],
                "os": info["host_os"],
            },
            "modules": info["modules"],
            "artifacts": [
                {"name": f.name, "sha256": digest(f), "size": f.stat().st_size}
                for f in files
            ],
        }

        # Deterministic: sorted keys, fixed separators, one trailing newline.
        # Two runs over the same bytes produce the same file.
        target = args.out / f"build-record-{leg_dir.name}.json"
        with target.open("w", encoding="utf-8") as f:
            json.dump(record, f, indent=2, sort_keys=True)
            f.write("\n")

        print(f"  {target.name}: {len(record['artifacts'])} artifact(s)")
        written += 1

    if written == 0:
        sys.exit("write_build_records: no artifacts to describe")
    print(f"{written} build record(s) written")


if __name__ == "__main__":
    main()
