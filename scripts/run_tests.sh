#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Run the GpgFrontend unit tests and stress tests.
#
# The test suite is linked into the main binary and executed via
# `gpgfrontend -t`. Two quirks this script works around:
#   1. The app's own CLI parser rejects unknown options, so GoogleTest flags
#      (filter, output, color, ...) must be passed through environment
#      variables (GTEST_FILTER, GTEST_OUTPUT, ...) instead of --gtest_* flags.
#   2. `gpgfrontend -t` exits 0 even when tests fail, so success/failure is
#      determined by parsing the GoogleTest summary, not the process exit code.
#
# Usage: scripts/run_tests.sh [options]
#   -b, --build            Build the gpgfrontend target before running
#       --no-build         Never build (fail if the binary is missing)
#       --unit-only        Run only unit tests (excludes *Stress* tests)
#       --stress-only      Run only the *Stress* tests
#   -i, --stress-iter N    Iterations per stress test    (default: 1000)
#   -f, --filter PATTERN   Run a single GTEST_FILTER and nothing else
#       --build-dir DIR    CMake build directory          (default: build)
#   -j, --jobs N           Parallel build jobs            (default: nproc)
#   -h, --help             Show this help
#
# Examples:
#   scripts/run_tests.sh --build
#   scripts/run_tests.sh --stress-only --stress-iter 10000
#   scripts/run_tests.sh --filter '*GpgCoreEngineTest*'

set -uo pipefail

# --- defaults --------------------------------------------------------------
BUILD_DIR="${BUILD_DIR:-build}"
STRESS_ITER="${GF_STRESS_ITER:-1000}"
DO_BUILD="auto"   # auto | yes | no
MODE="all"        # all | unit | stress | custom
CUSTOM_FILTER=""
JOBS="$(nproc 2>/dev/null || echo 4)"

usage() {
  awk 'NR>1 && /^#/ {sub(/^# ?/, ""); print; next} NR>1 {exit}' "${BASH_SOURCE[0]}"
}

# --- parse arguments -------------------------------------------------------
while [[ $# -gt 0 ]]; do
  case "$1" in
    -b|--build)       DO_BUILD="yes" ;;
    --no-build)       DO_BUILD="no" ;;
    --unit-only)      MODE="unit" ;;
    --stress-only)    MODE="stress" ;;
    -i|--stress-iter) STRESS_ITER="${2:?missing value for $1}"; shift ;;
    -f|--filter)      MODE="custom"; CUSTOM_FILTER="${2:?missing value for $1}"; shift ;;
    --build-dir)      BUILD_DIR="${2:?missing value for $1}"; shift ;;
    -j|--jobs)        JOBS="${2:?missing value for $1}"; shift ;;
    -h|--help)        usage; exit 0 ;;
    *) echo "error: unknown option '$1'" >&2; usage >&2; exit 2 ;;
  esac
  shift
done

# --- locate repo and binary ------------------------------------------------
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$REPO_ROOT"

BIN="$BUILD_DIR/artifacts/gpgfrontend"

if [[ "$DO_BUILD" == "yes" || ( "$DO_BUILD" == "auto" && ! -x "$BIN" ) ]]; then
  echo "==> Building gpgfrontend ($BUILD_DIR, -j$JOBS)"
  cmake --build "$BUILD_DIR" --target gpgfrontend -j"$JOBS" || {
    echo "error: build failed" >&2; exit 1; }
fi

if [[ ! -x "$BIN" ]]; then
  echo "error: test binary not found: $BIN" >&2
  echo "       run with --build, or build the 'gpgfrontend' target first." >&2
  exit 1
fi

RESULTS_DIR="$BUILD_DIR/test-results"
mkdir -p "$RESULTS_DIR"

if [[ -t 1 ]]; then export GTEST_COLOR="yes"; else export GTEST_COLOR="no"; fi

# --- phase runner ----------------------------------------------------------
# run_phase <name> <gtest-filter> <stress-iterations>
run_phase() {
  local name="$1" filter="$2" iter="$3"
  local log="$RESULTS_DIR/${name}.log"

  echo
  echo "============================================================"
  echo "  Phase: ${name}"
  echo "  Filter: ${filter}"
  echo "  GF_STRESS_ITER: ${iter}"
  echo "============================================================"

  GTEST_FILTER="$filter" \
  GTEST_OUTPUT="xml:${RESULTS_DIR}/${name}.xml" \
  GF_STRESS_ITER="$iter" \
    "$BIN" -t 2>&1 | tee "$log"

  # Exit code is unreliable; decide from the GoogleTest summary.
  if grep -qE '^\[  FAILED  \]|[0-9]+ FAILED TEST' "$log"; then
    return 1
  fi
  if ! grep -qE '^\[  PASSED  \]' "$log"; then
    echo "warning: no GoogleTest summary found for phase '${name}'" >&2
    return 1
  fi
  return 0
}

# --- run requested phases --------------------------------------------------
overall_rc=0
declare -a phases=()

case "$MODE" in
  custom)
    run_phase "custom" "$CUSTOM_FILTER" "$STRESS_ITER" || overall_rc=1
    phases+=("custom")
    ;;
  unit)
    run_phase "unit" '*-*Stress*' "$STRESS_ITER" || overall_rc=1
    phases+=("unit")
    ;;
  stress)
    run_phase "stress" '*Stress*' "$STRESS_ITER" || overall_rc=1
    phases+=("stress")
    ;;
  all)
    run_phase "unit" '*-*Stress*' "$STRESS_ITER" || overall_rc=1
    phases+=("unit")
    run_phase "stress" '*Stress*' "$STRESS_ITER" || overall_rc=1
    phases+=("stress")
    ;;
esac

# --- summary ---------------------------------------------------------------
echo
echo "============================================================"
echo "  Summary"
echo "============================================================"
for p in "${phases[@]}"; do
  log="$RESULTS_DIR/${p}.log"
  passed="$(grep -oE '\[  PASSED  \] [0-9]+ test' "$log" | grep -oE '[0-9]+' | head -1)"
  failed="$(grep -oE '\[  FAILED  \] [0-9]+ test' "$log" | grep -oE '[0-9]+' | head -1)"
  skipped="$(grep -oE '\[  SKIPPED \] [0-9]+ test' "$log" | grep -oE '[0-9]+' | head -1)"
  printf '  %-8s passed=%-4s failed=%-4s skipped=%-4s  (%s)\n' \
    "$p" "${passed:-0}" "${failed:-0}" "${skipped:-0}" "$log"
done
echo "  XML reports: ${RESULTS_DIR}/*.xml"
echo

if [[ "$overall_rc" -eq 0 ]]; then
  echo "RESULT: PASS"
else
  echo "RESULT: FAIL"
fi
exit "$overall_rc"
