#!/usr/bin/env bash
# Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Draft the release notes for a version, from the Conventional Commits since
# the release before it:
#
#   scripts/release/notes.sh 2.3.0 [--force]
#
# Writes release/notes/<version>.md. Edit that file and land it in a PR
# before tagging: the tagged tree must carry it (scripts/release/check_order.py
# fails a tag build without it), and it is what gets published and signed.
# Needs git-cliff (https://git-cliff.org); configured by cliff.toml.
set -euo pipefail

version="${1:-}"
force="${2:-}"
[[ "$version" =~ ^[0-9]+\.[0-9]+\.[0-9]+(-(beta|rc)\.[0-9]+)?$ ]] || {
  echo "usage: $0 X.Y.Z[-beta.N|-rc.N] [--force]" >&2
  exit 2
}
command -v git-cliff >/dev/null || {
  echo "notes: git-cliff is not installed (cargo install git-cliff)" >&2
  exit 1
}

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
top="$(git rev-parse --show-toplevel)"
out="${top}/release/notes/${version}.md"
if [ -e "$out" ] && [ "$force" != "--force" ]; then
  echo "notes: ${out} exists; it may hold edits. Pass --force to redraft." >&2
  exit 1
fi

# The newest release reachable from HEAD that is older than this one: on a
# release branch that is the previous fix, on main the previous release main
# saw (a final, or the last beta of this very version).
previous="$(python3 - "$top" "$version" <<PY
import subprocess, sys
sys.path.insert(0, "${here}")
import semver
top, version = sys.argv[1], semver.parse(sys.argv[2])
tags = subprocess.run(["git", "tag", "--merged", "HEAD", "--list", "v*"],
                      cwd=top, check=True, capture_output=True,
                      text=True).stdout.split()
older = [semver.parse(t) for t in tags
         if semver.RELEASE_TAG.match(t) and semver.parse(t) < version]
print(f"v{max(older)}" if older else "")
PY
)"

range="HEAD"
[ -n "$previous" ] && range="${previous}..HEAD"
mkdir -p "$(dirname "$out")"
(cd "$top" && git-cliff --config cliff.toml --tag "v${version}" \
  --output "$out" "$range")
echo "Drafted ${out} from ${range}. Edit it, then land it before tagging."
