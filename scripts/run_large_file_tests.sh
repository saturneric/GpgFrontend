#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Run the Rust large-file tier: the `#[ignore]`d `large_file_*` tests, which
# build real multi-GB SEIPD v1/v2 files at run time and check byte-for-byte
# recovery, integrity enforcement, transactional output and bounded peak
# memory. Shared by `scripts/run_tests.sh --large-file-only` and CI.
#
# Each test runs alone, in its own process, so its peak-RSS assertion measures
# that test and nothing else. Release mode, since debug cryptography would take
# hours over this much data.
#
# Nothing here may pass vacuously: a missing cargo or an empty test list is a
# failure, and the interop tests fail (not skip) without gpg/sq unless
# GF_ALLOW_MISSING_INTEROP_TOOLS=1 is set explicitly.
#
# Usage: scripts/run_large_file_tests.sh [LOG]
#   GF_LARGE_FILE_DIR  where the files go (default: the temp dir; ~15 GiB free)
#   GF_LARGE_FILE_GIB  size of the multi-gigabyte case (default 5)

set -uo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
RUST_DIR="$REPO_ROOT/rust"
LOG="${1:-/dev/null}"

if ! command -v cargo >/dev/null 2>&1; then
  echo "error: cargo not found; the large-file tier cannot run" | tee "$LOG" >&2
  exit 1
fi

: > "$LOG"
mapfile -t tests < <(
  cd "$RUST_DIR" && cargo test --release --lib -- --ignored --list 2>/dev/null |
    sed -n 's/: test$//p' | grep 'large_file_'
)
if (( ${#tests[@]} == 0 )); then
  echo "error: no large_file_ tests found" | tee -a "$LOG" >&2
  exit 1
fi

rc=0
for t in "${tests[@]}"; do
  echo "--- ${t}" | tee -a "$LOG"
  ( cd "$RUST_DIR" && cargo test --release --lib -- --ignored --exact "$t" --nocapture --test-threads=1 ) 2>&1 | tee -a "$LOG"
  (( PIPESTATUS[0] == 0 )) || rc=1
done
exit "$rc"
