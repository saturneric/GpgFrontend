#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Launch the FULL GpgFrontend GUI built with AddressSanitizer + UBSan so that an
# intermittent heap corruption (e.g. the user-reported `realloc(): invalid next
# size` during encrypt+sign / decrypt+verify) is caught at the moment of the bad
# memory access, with a precise symbolized stack — instead of a vague glibc abort
# that happens long after the corruption.
#
# This reuses the same build-asan/ tree that scripts/run_tests_asan.sh produces:
# the `gpgfrontend` binary runs the unit tests with `-t` and the real GUI without
# it. So the instrumented GUI is already built; this script just runs it with the
# right sanitizer options and saves the report.
#
# IMPORTANT: this uses your REAL GpgFrontend config and keyring (same as the
# normal app), so you can reproduce your actual workflow. It is ~2-3x slower and
# uses more memory than a normal build.
#
# Usage:
#   scripts/run_gui_asan.sh            # build (if needed) + launch the GUI
#   scripts/run_gui_asan.sh --build    # force-rebuild the ASan GUI first
#
# When it crashes, the sanitizer report is printed to the terminal AND written to
#   <build-asan>/asan-gui-report.<pid>
# Look for the first "ERROR: AddressSanitizer" block that contains GpgFrontend
# frames (src/...): that stack is the actual bug.

set -uo pipefail

BUILD_DIR="${BUILD_DIR:-build-asan}"
JOBS="$(nproc 2>/dev/null || echo 4)"
BIN="$BUILD_DIR/artifacts/gpgfrontend"
REPORT_BASE="$(cd "$(dirname "$0")/.." && pwd)/$BUILD_DIR/asan-gui-report"

if [[ "${1:-}" == "--build" || ! -x "$BIN" ]]; then
  if [[ ! -d "$BUILD_DIR" ]]; then
    echo "error: $BUILD_DIR/ does not exist. Run scripts/run_tests_asan.sh once" >&2
    echo "       first to create the sanitizer build tree." >&2
    exit 1
  fi
  echo "==> Building instrumented GUI ($BUILD_DIR, -j$JOBS)"
  cmake --build "$BUILD_DIR" --target gpgfrontend -j"$JOBS" || exit 1
fi

# detect_leaks=0      : the app keeps intentional process-lifetime singletons.
# halt_on_error=1     : stop at the first real error so its stack is unambiguous.
# abort_on_error=1    : SIGABRT after printing (leaves a core if ulimit allows).
# malloc_context_size : deeper allocation backtraces to find where a bad buffer
#                       was allocated.
# detect_odr_violation=0 : avoid false ODR positives across the shared libraries.
# log_path            : also write the report to a file (one per pid).
export ASAN_OPTIONS="detect_leaks=0:halt_on_error=1:abort_on_error=1:print_stacktrace=1:malloc_context_size=30:detect_odr_violation=0:log_path=${REPORT_BASE}"
export UBSAN_OPTIONS="print_stacktrace=1:halt_on_error=1:log_path=${REPORT_BASE}"

echo "==> Launching instrumented GUI: $BIN"
echo "    ASan reports -> ${REPORT_BASE}.<pid>"
echo "    Reproduce the crash (encrypt+sign / decrypt+verify). On a memory error"
echo "    the app will abort and the report will name the faulting line."
echo

exec "$BIN" "$@"
