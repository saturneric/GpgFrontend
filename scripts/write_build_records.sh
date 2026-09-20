#!/usr/bin/env bash
# Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Write one build record per published artifact, from the artifacts themselves.
#
# A record answers two questions, and they need different evidence:
#
#   which source, build and module set produced this
#       -> product.*, modules[]   (metadata, and module ENTRY identity)
#
#   is this file I downloaded one of the bytes that build produced
#       -> artifacts[]            (a digest of the file itself)
#
# ## Why this runs downstream, and what that deleted
#
# Records used to be written by each build and signing job, which put release
# attestation inside the build and package path. That forced a two-stage
# unsigned/signed protocol, because a record written before signing described
# bytes that signing then changed -- and it forced a cross-stage check that the
# two digests differed, to catch a record describing the other stage's file.
#
# Hashing the FINAL artifact cannot have that problem, so neither the stages
# nor the check exist. There is one record per file that ships.
#
# Nothing at runtime reads any of this. Module trust rests on the seed-signed
# descriptors and the trust root compiled into the Host; provenance is a
# separate, public, release-only layer and is not a trust input.
set -euo pipefail

ARTIFACTS=""
BUILD_INFO=""
OUT=""

while [ $# -gt 0 ]; do
  case "$1" in
    --artifacts)  ARTIFACTS="$2"; shift 2 ;;
    --build-info) BUILD_INFO="$2"; shift 2 ;;
    --out)        OUT="$2"; shift 2 ;;
    *) echo "write_build_records: unknown argument: $1" >&2; exit 2 ;;
  esac
done

if [ -z "$ARTIFACTS" ] || [ -z "$BUILD_INFO" ] || [ -z "$OUT" ]; then
  echo "usage: $0 --artifacts DIR --build-info DIR --out DIR" >&2
  exit 2
fi

mkdir -p "$OUT"

python3 - "$ARTIFACTS" "$BUILD_INFO" "$OUT" <<'PY'
import hashlib, json, os, sys
from pathlib import Path

artifacts_root, info_root, out_root = (Path(p) for p in sys.argv[1:4])

# One build-info per leg that produced one. Keyed by the OS it was built on,
# which is as much as a downloaded artifact directory can be matched on.
infos = {}
for path in sorted(info_root.rglob("build-info.json")):
    with path.open(encoding="utf-8") as f:
        infos[path.parent.name] = json.load(f)

def digest(path):
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()

def info_for(leg_dir):
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

written = 0
for leg_dir in sorted(p for p in artifacts_root.iterdir() if p.is_dir()):
    files = sorted(
        p for p in leg_dir.iterdir()
        if p.is_file() and p.name.startswith("GpgFrontend-")
    )
    if not files:
        continue

    info = info_for(leg_dir)
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

    # Deterministic: sorted keys, fixed separators, one trailing newline. Two
    # runs over the same bytes produce the same file.
    target = out_root / f"build-record-{leg_dir.name}.json"
    with target.open("w", encoding="utf-8") as f:
        json.dump(record, f, indent=2, sort_keys=True)
        f.write("\n")

    print(f"  {target.name}: {len(record['artifacts'])} artifact(s)")
    written += 1

if written == 0:
    sys.exit("write_build_records: no artifacts to describe")
print(f"{written} build record(s) written")
PY
