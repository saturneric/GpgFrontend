#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Build GpgFrontend in a SEPARATE directory instrumented with AddressSanitizer
# + UndefinedBehaviorSanitizer, then run the stress tests under it to hunt for
# memory corruption (heap-buffer-overflow, use-after-free, double-free) and
# undefined behaviour — especially across the Rust/rPGP FFI boundary.
#
# Why a separate script + build dir:
#   * The in-tree GPGFRONTEND_ENABLE_ASAN option hard-requires clang. This box
#     only has gcc, so we inject -fsanitize flags directly instead, which gcc
#     supports. The normal build/ tree is left completely untouched.
#   * A sanitizer build is ABI-incompatible and ~slow; keeping it in
#     build-asan/ means you never relink your fast iteration build.
#
# Notes:
#   * The Rust crate (gf_rust) is compiled by cargo WITHOUT instrumentation, so
#     ASan won't see overflows internal to Rust. It DOES see the C++/FFI side:
#     buffers handed across the boundary, key-model strings, alloc/free pairing.
#     That is exactly the hot path the *Stress tests exercise.
#   * LeakSanitizer is disabled by default (detect_leaks=0): the app keeps many
#     intentional process-lifetime singletons that would otherwise drown the
#     real corruption signal. Override via ASAN_OPTIONS if you want leak checks.
#
# Usage: scripts/run_tests_asan.sh [options]
#   -i, --stress-iter N    Iterations per stress test     (default: 2000)
#   -f, --filter PATTERN   GTEST_FILTER to run            (default: *Stress*)
#   -l, --log-level LEVEL  App log level: debug|info|warn|error (default: warn)
#       --reconfigure      Wipe build-asan/ and reconfigure from scratch
#       --build-dir DIR    Sanitizer build directory      (default: build-asan)
#   -j, --jobs N           Parallel build jobs            (default: nproc)
#   -h, --help             Show this help
#
# Examples:
#   scripts/run_tests_asan.sh                       # all stress tests, 2000x
#   scripts/run_tests_asan.sh -i 5000               # deeper fuzzing run
#   scripts/run_tests_asan.sh -f '*RpgpCore*'       # focus rPGP FFI paths

set -uo pipefail

# --- defaults --------------------------------------------------------------
BUILD_DIR="${BUILD_DIR:-build-asan}"
STRESS_ITER="${GF_STRESS_ITER:-2000}"
FILTER='*Stress*'
LOG_LEVEL="warn"
RECONFIGURE="no"
JOBS="$(nproc 2>/dev/null || echo 4)"

usage() {
  awk 'NR>1 && /^#/ {sub(/^# ?/, ""); print; next} NR>1 {exit}' "${BASH_SOURCE[0]}"
}

# --- parse arguments -------------------------------------------------------
while [[ $# -gt 0 ]]; do
  case "$1" in
    -i|--stress-iter) STRESS_ITER="${2:?missing value for $1}"; shift ;;
    -f|--filter)      FILTER="${2:?missing value for $1}"; shift ;;
    -l|--log-level)   LOG_LEVEL="${2:?missing value for $1}"; shift ;;
    --reconfigure)    RECONFIGURE="yes" ;;
    --build-dir)      BUILD_DIR="${2:?missing value for $1}"; shift ;;
    -j|--jobs)        JOBS="${2:?missing value for $1}"; shift ;;
    -h|--help)        usage; exit 0 ;;
    *) echo "error: unknown option '$1'" >&2; usage >&2; exit 2 ;;
  esac
  shift
done

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$REPO_ROOT"

SAN_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=undefined -g"

# --- configure (only when needed) ------------------------------------------
if [[ "$RECONFIGURE" == "yes" ]]; then
  echo "==> Wiping $BUILD_DIR"
  rm -rf "$BUILD_DIR"
fi

if [[ ! -f "$BUILD_DIR/CMakeCache.txt" ]]; then
  echo "==> Configuring sanitizer build in $BUILD_DIR (gcc + ASan/UBSan)"
  cmake -S . -B "$BUILD_DIR" -G Ninja \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DGPGFRONTEND_LINK_GPGME_INTO_CORE=On \
    -DGPGFRONTEND_BUILD_MODULES=ON \
    -DCMAKE_C_FLAGS="$SAN_FLAGS" \
    -DCMAKE_CXX_FLAGS="$SAN_FLAGS" \
    -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined" \
    -DCMAKE_SHARED_LINKER_FLAGS="-fsanitize=address,undefined" \
    -DCMAKE_MODULE_LINKER_FLAGS="-fsanitize=address,undefined" \
    || { echo "error: configure failed" >&2; exit 1; }
fi

# --- build -----------------------------------------------------------------
echo "==> Building gpgfrontend ($BUILD_DIR, -j$JOBS) — this is a full sanitized rebuild"
cmake --build "$BUILD_DIR" --target gpgfrontend -j"$JOBS" \
  || { echo "error: build failed" >&2; exit 1; }

BIN="$BUILD_DIR/artifacts/gpgfrontend"
if [[ ! -x "$BIN" ]]; then
  echo "error: test binary not found: $BIN" >&2
  exit 1
fi

RESULTS_DIR="$BUILD_DIR/test-results"
mkdir -p "$RESULTS_DIR"
LOG="$RESULTS_DIR/asan-stress.log"

if [[ -t 1 ]]; then export GTEST_COLOR="yes"; else export GTEST_COLOR="no"; fi

# Force Qt's offscreen platform. The XCB platform plugin triggers a benign
# over-read inside libxkbcommon-x11/libxcb (xkb_x11_keymap_new_from_device) at
# QApplication startup that ASan flags as a heap-buffer-overflow and aborts on,
# long before any test runs. It is third-party, harmless, and unrelated to the
# code under test; offscreen avoids the X11 connection entirely (also faster).
export QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-offscreen}"

# halt_on_error=1: stop at the first corruption with a full report (clean signal
# under a tight stress loop). detect_leaks=0: ignore intentional singletons.
export ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0:halt_on_error=1:abort_on_error=1:strict_string_checks=1:detect_stack_use_after_return=1:print_stats=0}"
export UBSAN_OPTIONS="${UBSAN_OPTIONS:-print_stacktrace=1:halt_on_error=1}"

echo
echo "============================================================"
echo "  Sanitizer stress run"
echo "  Filter:         $FILTER"
echo "  Log level:      $LOG_LEVEL"
echo "  GF_STRESS_ITER: $STRESS_ITER"
echo "  ASAN_OPTIONS:   $ASAN_OPTIONS"
echo "  UBSAN_OPTIONS:  $UBSAN_OPTIONS"
echo "============================================================"

GTEST_FILTER="$FILTER" \
GTEST_OUTPUT="xml:${RESULTS_DIR}/asan-stress.xml" \
GF_STRESS_ITER="$STRESS_ITER" \
  "$BIN" -t -l "$LOG_LEVEL" 2>&1 | tee "$LOG"

# --- verdict ---------------------------------------------------------------
# Three independent failure signals: a sanitizer report, a GoogleTest failure,
# or a missing GoogleTest summary (binary crashed before printing it).
rc=0
if grep -qE 'ERROR: AddressSanitizer|runtime error:|SUMMARY: (Address|Undefined)Sanitizer' "$LOG"; then
  echo "RESULT: FAIL (sanitizer detected an error)"
  rc=1
elif grep -qE '^\[  FAILED  \]|[0-9]+ FAILED TEST' "$LOG"; then
  echo "RESULT: FAIL (test assertion failed)"
  rc=1
elif ! grep -qE '^\[  PASSED  \]' "$LOG"; then
  echo "RESULT: FAIL (no GoogleTest summary — possible crash)"
  rc=1
else
  passed="$(grep -oE '\[  PASSED  \] [0-9]+ test' "$LOG" | grep -oE '[0-9]+' | head -1)"
  echo "RESULT: PASS (${passed:-0} tests, clean under ASan/UBSan)"
fi

echo "  Log: $LOG"
exit "$rc"
