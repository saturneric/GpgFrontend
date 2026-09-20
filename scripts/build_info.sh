#!/usr/bin/env bash
# Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Read one value out of the build's own build-info.json.
#
# This exists so that no workflow step re-derives what the build system already
# decided. Before it, the module count was computed in eleven separate shells,
# the host tool directory read and suffixed by hand in eight, the module root
# hardcoded in seventeen, and the project version scraped out of CMakeLists.txt
# with a regular expression. Every one of those was a chance to be wrong on one
# platform only.
#
#   scripts/build_info.sh <build-dir> <key>
#
# Keys: project_version, build_id, host_os, host_tool_dir, module_packager,
#       module_namespace_root, module_count.
set -euo pipefail

BUILD_DIR="${1:-}"
KEY="${2:-}"
if [ -z "$BUILD_DIR" ] || [ -z "$KEY" ]; then
  echo "usage: $0 <build-dir> <key>" >&2
  exit 2
fi

INFO="$BUILD_DIR/artifacts/build-info.json"
if [ ! -f "$INFO" ]; then
  echo "build_info: $INFO does not exist; was this tree configured?" >&2
  exit 1
fi

python3 - "$INFO" "$KEY" <<'PY'
import json, sys
info, key = sys.argv[1], sys.argv[2]
with open(info, encoding="utf-8") as f:
    data = json.load(f)
if key not in data:
    sys.exit(f"build_info: {info} has no key {key!r}")
value = data[key]
print("\n".join(value) if isinstance(value, list) else value)
PY
