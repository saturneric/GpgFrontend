#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Build the m_notes editor page (modules/src/m_notes/web/dist/) from its
# sources and the pinned package-lock.json. dist/ is committed: building the
# application never needs Node. This is how dist/ is (re)made, and --check is
# how CI proves the committed dist/ is exactly what the sources build to.
#
#   scripts/build_note_editor.sh            rebuild dist/ and run the tests
#   scripts/build_note_editor.sh --check    rebuild elsewhere, compare, change nothing
set -euo pipefail
WEB="$(cd "$(dirname "$0")/.." && pwd)/modules/src/m_notes/web"
cd "$WEB"

if [[ "${1:-}" == "--check" ]]; then
  work=$(mktemp -d)
  trap 'rm -rf "$work"' EXIT
  cp -r package.json package-lock.json build.mjs src test "$work/"
  (cd "$work" && npm ci --no-audit --no-fund --silent && node build.mjs && npm test --silent)
  for f in editor.js editor.css index.html; do
    if ! cmp -s "$work/dist/$f" "dist/$f"; then
      echo "dist/$f differs from what the sources build to; run $0" >&2
      exit 1
    fi
  done
  echo "dist/ matches its sources"
  exit 0
fi

npm ci --no-audit --no-fund --silent
node build.mjs
npm test --silent
echo "built $(wc -c < dist/editor.js) bytes of editor.js"
