#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Measure gfpipe against key packages of growing size: how long its common
# operations take, and how much memory many copies of it hold at once. Run it
# before and after a change to gfpipe or the key package format and compare.
#
# The packages come from the ignored keypack test deferred_write_bench_packages
# (10, 1000 and 10000 keys by default) and are kept between runs; delete
# GF_BENCH_DIR to make new ones.
#
# Memory: N copies of `gfpipe -e -r <key>` are started on one FIFO, so each has
# opened the package and found its key, then waits for its input. The sums of
# Pss and Private_* over /proc/<pid>/smaps_rollup are what the N copies cost
# together; Pss counts shared pages (code, the page-cached package) once.
#
# Usage: scripts/bench_gfpipe.sh GFPIPE
#   GF_BENCH_DIR    where the packages go (default /tmp/gfpipe-bench)
#   GF_BENCH_KEYS   package sizes (default 10,1000,10000)
#   GF_BENCH_PROCS  copies started at once (default 1000)

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
GFPIPE="$(realpath "${1:?usage: $0 GFPIPE}")"
DIR="${GF_BENCH_DIR:-/tmp/gfpipe-bench}"
KEYS="${GF_BENCH_KEYS:-10,1000,10000}"
PROCS="${GF_BENCH_PROCS:-1000}"

for tool in hyperfine cargo; do
  command -v "$tool" >/dev/null || { echo "error: $tool not found" >&2; exit 1; }
done

IFS=, read -ra counts <<<"$KEYS"
missing=0
for n in "${counts[@]}"; do
  [[ -f "$DIR/$n/default.gfpack" ]] || missing=1
done
if (( missing )); then
  echo "== writing packages to $DIR"
  (cd "$REPO_ROOT/rust" && GF_BENCH_DIR="$DIR" GF_BENCH_KEYS="$KEYS" \
    cargo test --release --lib deferred_write_bench_packages -- --ignored --nocapture) |
    grep -E 'keys:' || true
fi

work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT
head -c 4096 /dev/urandom >"$work/data"

# Sum the smaps_rollup fields of every pid given, in KiB.
sum_rollup() {
  local field total=0 pid v
  for field in Rss Pss Private_Clean Private_Dirty; do
    total=0
    for pid in "$@"; do
      v=$(awk -v f="$field:" '$1 == f { print $2 }' "/proc/$pid/smaps_rollup" 2>/dev/null || echo 0)
      total=$((total + ${v:-0}))
    done
    printf '  %-14s %8d MiB total  %6d KiB each\n' "$field" $((total / 1024)) $((total / $#))
  done
}

for n in "${counts[@]}"; do
  pack="$DIR/$n/default.gfpack"
  last=$((n - 1))
  echo
  echo "== $n keys ($(stat -c %s "$pack") bytes)"
  run() { "$GFPIPE" --keypack "$pack" --batch --yes "$@"; }
  run -a -u user0@bench.test --detach-sign -o "$work/sig.asc" "$work/data"

  hyperfine -N --warmup 1 --min-runs 3 --style basic --export-markdown "$work/t.md" \
    -n verify "$GFPIPE --keypack $pack --verify $work/sig.asc $work/data" \
    -n "encrypt (e-mail)" "$GFPIPE --keypack $pack -e -r user$last@bench.test -o /dev/null $work/data" \
    -n "encrypt (user ID)" "$GFPIPE --keypack $pack -e -r '=Bench User $last <user$last@bench.test>' -o /dev/null $work/data" \
    -n "list keys" "$GFPIPE --keypack $pack -k" >/dev/null 2>&1
  sed -n '3,$p' "$work/t.md"

  fifo="$work/fifo"
  rm -f "$fifo"
  mkfifo "$fifo"
  exec 3<>"$fifo"
  pids=()
  for ((i = 0; i < PROCS; i++)); do
    "$GFPIPE" --keypack "$pack" -e -r "user$last@bench.test" -o /dev/null <"$fifo" 2>/dev/null &
    pids+=($!)
  done
  sleep 3
  echo "  $PROCS copies of gfpipe -e, waiting for input:"
  sum_rollup "${pids[@]}"
  exec 3>&-
  kill "${pids[@]}" 2>/dev/null || true
  wait 2>/dev/null || true
done
