#!/usr/bin/env bash
# Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Cut release/X.Y and move main on to X.(Y+1).
#
#   scripts/release/cut.sh X.Y [--at <ref>] [--no-sign]
#
# Run on main, after tagging vX.Y.0 there, with release/policy.toml already
# edited for the new state: X.Y stable, the old stable legacy (with its eol
# date), X.(Y+1) mainline. It then
#
#   1. creates release/X.Y at vX.Y.0 (or at --at <ref>),
#   2. commits the policy change and the re-rendered SECURITY.md on main, and
#   3. tags that commit vX.(Y+1).0-alpha.0, signed unless --no-sign.
#
# The tag is the base main's versions count from (cmake/GitVersion.cmake), so
# main stays newer than everything on release/X.Y however its fixes are
# tagged. It sits on a commit only main has, so the branch never reaches it.
# Nothing is pushed: the commands to do so are printed at the end.
set -euo pipefail

usage() {
  echo "usage: $0 X.Y [--at <ref>] [--no-sign]" >&2
  exit 2
}

[ $# -ge 1 ] || usage
series="$1"
shift
at=""
sign="-s"
while [ $# -gt 0 ]; do
  case "$1" in
    --at) at="${2:?--at needs a ref}"; shift 2 ;;
    --no-sign) sign="-a"; shift ;;
    *) usage ;;
  esac
done

[[ "$series" =~ ^([0-9]+)\.([0-9]+)$ ]] || usage
major="${BASH_REMATCH[1]}"
minor="${BASH_REMATCH[2]}"
next="${major}.$((minor + 1))"
branch="release/${series}"
base_tag="v${next}.0-alpha.0"
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
top="$(git rev-parse --show-toplevel)"
cd "$top"

die() { echo "cut: $*" >&2; exit 1; }

[ "$(git symbolic-ref --short -q HEAD)" = "main" ] || die "run this on main"
[ -z "${at}" ] && at="v${series}.0"
git rev-parse -q --verify "${at}^{commit}" >/dev/null ||
  die "${at} does not exist; tag v${series}.0 first, or pass --at"
git rev-parse -q --verify "refs/heads/${branch}" >/dev/null &&
  die "${branch} already exists"
git rev-parse -q --verify "refs/tags/${base_tag}" >/dev/null &&
  die "${base_tag} already exists"
git merge-base --is-ancestor "${at}" HEAD ||
  die "${at} is not on main"

# Only the policy may be changed, and it must be.
dirty="$(git status --porcelain --untracked-files=no)"
others="$(printf '%s\n' "$dirty" | grep -v -E '^.. (release/policy\.toml|SECURITY\.md)$' || true)"
[ -z "$others" ] || die "commit or stash everything but release/policy.toml first:
$others"
printf '%s\n' "$dirty" | grep -q -E '^.. release/policy\.toml$' ||
  die "edit release/policy.toml first: ${series} stable, ${next} mainline"

policy=(python3 "${here}/policy.py" --policy "${top}/release/policy.toml"
  --product "${top}/release/product.json" --security "${top}/SECURITY.md")
"${policy[@]}" check
[ "$("${policy[@]}" mainline)" = "${next}" ] ||
  die "release/policy.toml must name ${next} mainline"
python3 - "${top}/release/policy.toml" "${series}" <<'PY' ||
import sys, tomllib
with open(sys.argv[1], "rb") as f:
    entry = tomllib.load(f)["series"].get(sys.argv[2], {})
sys.exit(0 if entry.get("status") == "stable" else 1)
PY
  die "release/policy.toml must name ${series} stable"
"${policy[@]}" render-security

git branch "${branch}" "${at}"
git add release/policy.toml SECURITY.md
git commit -q -m "chore(release): open ${next} development" \
  -m "release/${series} is cut at ${at}; main now builds ${next}.0-alpha.N."
git tag "${sign}" "${base_tag}" -m "Base of ${next} development"

echo "Cut ${branch} at ${at}; main is based on ${base_tag}."
echo "Review, then push:"
echo "  git push origin ${branch} main ${base_tag}"
