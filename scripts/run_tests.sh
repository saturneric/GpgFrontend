#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Run the GpgFrontend unit tests and stress tests.
#
# The tests are in two programs, and every GoogleTest phase runs both: the
# core's own tests in `gf_core_tests`, which starts the core on a plain
# QCoreApplication, and the tests that need the interface or the module SDK in
# the main binary, executed via `gpgfrontend -t`. A filter goes to both; a
# phase passes only if both do, and its log and XML report hold both. Three
# quirks this script works around:
#   1. The app's own CLI parser rejects unknown options, so GoogleTest flags
#      (filter, output, color, ...) must be passed through environment
#      variables (GTEST_FILTER, GTEST_OUTPUT, ...) instead of --gtest_* flags.
#   2. `gpgfrontend -t` exits 0 even when tests fail, so success/failure is
#      determined by parsing the GoogleTest summary, not the process exit code.
#   3. Parallel mode (-p) shards via GoogleTest's own GTEST_TOTAL_SHARDS /
#      GTEST_SHARD_INDEX -- no C++ change needed -- and gives each shard a
#      private HOME/XDG tree. The private HOME is mandatory, not hygiene: the
#      app mounts the profile before it ever sees -t (src/GpgFrontend.cpp), and
#      ProfileLoader takes an exclusive QLockFile on <profile>/profile.lock. A
#      second process would wait 5 s and then pop a modal force-unlock dialog
#      which, under QT_QPA_PLATFORM=offscreen, never returns.
#
# The pure-Rust engine logic is covered by `cargo test` in rust/ and is run as
# an additional "rust" phase (in the default and --unit-only modes), so a single
# invocation exercises both the C++/FFI suite and the Rust unit tests.
#
# Usage: scripts/run_tests.sh [options]
#   -b, --build            Build gpgfrontend and gf_core_tests before running
#       --no-build         Never build (fail if the binary is missing)
#       --unit-only        Run only unit tests (excludes *Stress* and the
#                          algorithm-generation coverage sweep)
#       --stress-only      Run only the *Stress* tests
#       --coverage-only    Run only the algorithm-generation coverage sweep
#                          (*GenerateAllDeclared*; sets GF_RUN_ALGO_COVERAGE=1)
#       --rust-only        Run only the Rust (cargo test) phase
#       --modules-only     Run only the modules' own gtest binaries
#       --require-modules  Treat missing module test binaries as a FAILURE
#                          rather than a skip. For CI: without it a tree
#                          configured without GPGFRONTEND_MODULES_BUILD_TESTS
#                          reports green having run nothing.
#       --rust-slow-only   Run only the ignored/slow Rust tests
#       --mobile-only      Run only the phone application's tests (an android
#                          flavor tree, e.g. --build-dir build-android)
#       --large-file-only  Run only the large-file tier: real SEIPD v1/v2 files
#                          over the former 1 GiB limit (multi-GB, GnuPG output
#                          over 4 GiB), each Rust test in its own process with a
#                          peak-memory assertion, plus the C++ *LargeFile* tests.
#                          Needs ~15 GiB free in GF_LARGE_FILE_DIR (default: the
#                          temp dir), gpg and sq; GF_LARGE_FILE_GIB sizes the
#                          multi-GB case (default 5).
#       --no-rust          Skip the Rust phase
#   -i, --stress-iter N    Iterations per stress test    (default: 1000)
#   -f, --filter PATTERN   Run a single GTEST_FILTER and nothing else
#   -l, --log-level LEVEL  App log level: debug|info|warn|error (default: warn)
#   -p, --parallel N       Shard the unit/custom phase across N processes
#                          (integer >= 1, or "auto" = clamp(nproc/2, 1, 6)).
#                          Default 1: one process, your real HOME -- identical
#                          to the behavior before this option existed.
#       --fresh-homes      Wipe the per-shard HOME dirs before running instead
#                          of reusing them
#       --shard-timeout S  Hard per-shard timeout, seconds  (default: 900)
#       --build-dir DIR    CMake build directory          (default: build,
#                          or build-asan with --asan/--asan-gui)
#   -j, --jobs N           Parallel build jobs            (default: nproc)
#       --asan             Run against a separate AddressSanitizer + UBSan
#                          build (see "Sanitizer mode" below)
#       --asan-gui [-- ARGS]
#                          Build the sanitizer tree if needed and launch the
#                          real GUI from it, with ARGS passed to the app
#       --reconfigure      With --asan/--asan-gui: wipe the sanitizer build
#                          directory and configure it from scratch
#   -h, --help             Show this help
#
# Phases (the "all" default runs rust + unit + stress + coverage, each in its
# own section so a slow or flaky phase is isolated; CI runs them as separate
# steps):
#   rust      pure-Rust unit tests (cargo test)
#   unit      fast C++/FFI tests   (excludes *Stress* and *GenerateAllDeclared*)
#   stress    *Stress* tests       (GF_STRESS_ITER iterations)
#   coverage  *GenerateAllDeclared* sweep (generates every declared key/subkey
#             algorithm per engine; slow, so gated on GF_RUN_ALGO_COVERAGE=1)
#   mobile    the phone application's tests, gf_mob_tests and its QML pages'
#             gf_mob_qml_tests (only in an android flavor tree, where the
#             unit and all modes include it; offscreen, in a fresh HOME)
#
# Examples:
#   scripts/run_tests.sh --build
#   scripts/run_tests.sh --stress-only --stress-iter 10000
#   scripts/run_tests.sh --coverage-only
#   scripts/run_tests.sh --filter '*GpgCoreEngineTest*'
#   scripts/run_tests.sh --rust-only
#   scripts/run_tests.sh --rust-slow-only
#   scripts/run_tests.sh --unit-only -p auto
#   scripts/run_tests.sh --asan                     # module + Lua tests + stress
#   scripts/run_tests.sh --asan -f '*RpgpCore*'     # focus rPGP FFI paths
#   scripts/run_tests.sh --asan-gui                 # reproduce a crash by hand
#
# Sanitizer mode (--asan): builds GpgFrontend in build-asan/ with
# -fsanitize=address,undefined injected directly (the in-tree
# GPGFRONTEND_ENABLE_ASAN option requires clang; this works with gcc) and
# leaves the normal build/ tree untouched. It composes with the modes above;
# with none given it runs the module test binaries, the Lua* tests and the
# *Stress* tests (default 2000 iterations). The Rust phases are skipped: cargo builds the
# crate without instrumentation. LeakSanitizer is off (detect_leaks=0),
# because the app keeps many intentional process-lifetime singletons; set
# ASAN_OPTIONS yourself to turn it on. A sanitizer report anywhere in a
# phase's log fails that phase.
#
# --asan-gui uses your REAL GpgFrontend config and keyring, so you can
# reproduce your own workflow. Reports are also written to
# <build-asan>/asan-gui-report.<pid>; the first "ERROR: AddressSanitizer"
# block with GpgFrontend frames (src/...) is the bug.
#
# Sharding notes: only the "unit" and custom (-f) phases are sharded; "stress"
# and "coverage" are each dominated by a single long test, so splitting them
# buys nothing. Per-shard logs are <build>/test-results/<phase>-shard<i>.log and
# the shard HOMEs live in <build>/test-homes/s<i>, keyed on the binary's mtime
# so a rebuilt binary always gets fresh profiles.

set -uo pipefail

# --- defaults --------------------------------------------------------------
BUILD_DIR="${BUILD_DIR:-build}"
STRESS_ITER="${GF_STRESS_ITER:-1000}"
LOG_LEVEL="warn"
DO_BUILD="auto"   # auto | yes | no
MODE="all"        # all | unit | stress | coverage | custom | rust
RUN_RUST="auto"   # auto | no  (auto = include rust in all/unit modes)
REQUIRE_MODULES="no"  # yes = missing module test binaries are a failure
CUSTOM_FILTER=""
JOBS="$(nproc 2>/dev/null || echo 4)"
PARALLEL=1                                     # 1 = serial; N or "auto"
ASAN="no"         # no | tests | gui
RECONFIGURE="no"
BUILD_DIR_SET="no"
STRESS_ITER_SET="no"
declare -a APP_ARGS=()
FRESH_HOMES="no"
SHARD_TIMEOUT="${GF_SHARD_TIMEOUT:-900}"
SHUTDOWN_GRACE="${GF_SHUTDOWN_GRACE:-15}"   # seconds to wait after the summary

# GoogleTest filter for the algorithm-generation coverage sweep, shared between
# the unit phase (which excludes it) and the dedicated coverage phase.
COVERAGE_FILTER='*GenerateAllDeclared*'

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
    --coverage-only)  MODE="coverage" ;;
    --rust-only)      MODE="rust" ;;
    --modules-only)   MODE="modules" ;;
    --rust-slow-only) MODE="rust-slow" ;;
    --mobile-only)    MODE="mobile" ;;
    --large-file-only) MODE="large-file" ;;
    --no-rust)        RUN_RUST="no" ;;
    --require-modules) REQUIRE_MODULES="yes" ;;
    -i|--stress-iter) STRESS_ITER="${2:?missing value for $1}"; STRESS_ITER_SET="yes"; shift ;;
    -f|--filter)      MODE="custom"; CUSTOM_FILTER="${2:?missing value for $1}"; shift ;;
    -l|--log-level)   LOG_LEVEL="${2:?missing value for $1}"; shift ;;
    -p|--parallel)    PARALLEL="${2:?missing value for $1}"; shift ;;
    --fresh-homes)    FRESH_HOMES="yes" ;;
    --shard-timeout)  SHARD_TIMEOUT="${2:?missing value for $1}"; shift ;;
    --build-dir)      BUILD_DIR="${2:?missing value for $1}"; BUILD_DIR_SET="yes"; shift ;;
    --asan)           ASAN="tests" ;;
    --asan-gui)       ASAN="gui" ;;
    --reconfigure)    RECONFIGURE="yes" ;;
    --)               shift; APP_ARGS=("$@"); break ;;
    -j|--jobs)        JOBS="${2:?missing value for $1}"; shift ;;
    -h|--help)        usage; exit 0 ;;
    *) echo "error: unknown option '$1'" >&2; usage >&2; exit 2 ;;
  esac
  shift
done

# Every shard sets these explicitly; clear inherited values so a stray
# GTEST_TOTAL_SHARDS in the caller's environment can never silently shard -- or,
# worse, abort -- a run that was asked to be serial.
unset GTEST_TOTAL_SHARDS GTEST_SHARD_INDEX GTEST_SHARD_STATUS_FILE

# "auto" caps at half the cores rather than all of them. The measured curve
# flattens hard past 6 shards (N=6 -> 25.5 s, N=12 -> 22.2 s), and the headroom
# keeps the timing-sensitive tests honest: WaitFor() polls on a 20 ms loop and
# GFCoreTest.PassphraseServiceClosesAnUnansweredPrompt asserts a 6 s timeout.
resolve_parallel() {
  case "$PARALLEL" in
    auto)
      local n; n="$(nproc 2>/dev/null || echo 2)"
      PARALLEL=$(( n / 2 ))
      (( PARALLEL < 1 )) && PARALLEL=1
      (( PARALLEL > 6 )) && PARALLEL=6
      ;;
    ''|*[!0-9]*|0)
      echo "error: --parallel expects a positive integer or 'auto'" >&2
      exit 2 ;;
  esac

  # The cap used to exist because the suite lost races above half the cores:
  # GFCoreTest.PassphraseServiceClosesAnUnansweredPrompt failed 14 of 32 runs at
  # 8 concurrent processes, reporting kCancelled where it expects kFailed. That
  # was a real race in PassphraseService::DrivePrompt and has been fixed; 12
  # shards now run clean.
  #
  # The cap stays anyway, for a duller reason: every shard re-pays the fixture
  # setup -- a profile, a GnuPG context, an rPGP context, the key imports -- so
  # past six shards that fixed cost cancels out the smaller slice of tests.
  # Measured: 6 shards 16.4s, 8 shards 15s, 12 shards 13.5s. Ask for more if you
  # want; it is not faster, and it is no longer unsafe.
}
resolve_parallel

# --- locate repo and binary ------------------------------------------------
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$REPO_ROOT"

# The module test binaries are not named here: this script knows no module.
# Each test target registers itself with gf_register_module_test(), and the
# build writes what it registered to test-bin/module-tests.list; the modules
# phase runs exactly that list (see run_modules_phase).

# --- sanitizer build (--asan / --asan-gui) ---------------------------------
if [[ "$ASAN" != "no" ]]; then
  [[ "$BUILD_DIR_SET" == "yes" ]] || BUILD_DIR="build-asan"
  [[ "$STRESS_ITER_SET" == "yes" ]] || STRESS_ITER="${GF_STRESS_ITER:-2000}"
  RUN_RUST="no"
  [[ "$MODE" == "all" ]] && MODE="asan"

  SAN_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=undefined -g"
  if [[ "$RECONFIGURE" == "yes" ]]; then
    case "$BUILD_DIR" in
      ""|"/"|".") echo "error: refusing to wipe '$BUILD_DIR'" >&2; exit 2 ;;
    esac
    echo "==> Wiping $BUILD_DIR"
    rm -rf -- "$BUILD_DIR"
  fi
  if [[ ! -f "$BUILD_DIR/CMakeCache.txt" ]]; then
    echo "==> Configuring sanitizer build in $BUILD_DIR (ASan/UBSan)"
    cmake -S . -B "$BUILD_DIR" -G Ninja \
      -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      -DGPGFRONTEND_LINK_GPGME_INTO_CORE=On \
      -DGPGFRONTEND_BUILD_MODULES=ON \
      -DGPGFRONTEND_BUILD_TESTS=ON \
      -DGPGFRONTEND_MODULES_BUILD_TESTS=ON \
      -DCMAKE_C_FLAGS="$SAN_FLAGS" \
      -DCMAKE_CXX_FLAGS="$SAN_FLAGS" \
      -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined" \
      -DCMAKE_SHARED_LINKER_FLAGS="-fsanitize=address,undefined" \
      -DCMAKE_MODULE_LINKER_FLAGS="-fsanitize=address,undefined" \
      || { echo "error: configure failed" >&2; exit 1; }
  fi

  # A sanitized tree is rebuilt unless --no-build says otherwise: it is not the
  # tree you iterate in, so it is easy to leave stale.
  if [[ "$DO_BUILD" != "no" ]]; then
    targets=(gpgfrontend gf_core_tests)
    [[ "$ASAN" == "tests" ]] && targets+=(gf_module_tests)
    echo "==> Building ${targets[*]} ($BUILD_DIR, -j$JOBS); a full sanitized rebuild is slow"
    cmake --build "$BUILD_DIR" --target "${targets[@]}" -j"$JOBS" \
      || { echo "error: build failed" >&2; exit 1; }
    DO_BUILD="no"
  fi

  if [[ "$ASAN" == "gui" ]]; then
    GUI_BIN="$BUILD_DIR/artifacts/gpgfrontend"
    [[ -x "$GUI_BIN" ]] || { echo "error: not built: $GUI_BIN" >&2; exit 1; }
    REPORT_BASE="$REPO_ROOT/$BUILD_DIR/asan-gui-report"
    # detect_leaks=0         the app keeps intentional process-lifetime singletons
    # halt_on_error=1        stop at the first real error so its stack is clear
    # abort_on_error=1       SIGABRT after printing (a core, if ulimit allows)
    # malloc_context_size    deeper allocation stacks, to find where a bad
    #                        buffer was allocated
    # detect_odr_violation=0 no false ODR reports across the shared libraries
    # log_path               also write each report to a file, one per pid
    export ASAN_OPTIONS="detect_leaks=0:halt_on_error=1:abort_on_error=1:print_stacktrace=1:malloc_context_size=30:detect_odr_violation=0:log_path=${REPORT_BASE}"
    export UBSAN_OPTIONS="print_stacktrace=1:halt_on_error=1:log_path=${REPORT_BASE}"
    echo "==> Launching the instrumented GUI: $GUI_BIN"
    echo "    ASan reports -> ${REPORT_BASE}.<pid>"
    exec "$GUI_BIN" ${APP_ARGS[@]+"${APP_ARGS[@]}"}
  fi

  # Offscreen, because the XCB platform plugin has a harmless third-party
  # over-read in libxkbcommon-x11 at QApplication startup that ASan aborts on
  # before any test runs.
  export QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-offscreen}"
  # halt_on_error=1: stop at the first corruption with a full report.
  # detect_leaks=0: ignore intentional singletons.
  export ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0:halt_on_error=1:abort_on_error=1:strict_string_checks=1:detect_stack_use_after_return=1:print_stats=0}"
  export UBSAN_OPTIONS="${UBSAN_OPTIONS:-print_stacktrace=1:halt_on_error=1}"
  REQUIRE_MODULES="yes"
fi

BIN="$BUILD_DIR/artifacts/gpgfrontend"
CORE_BIN="$BUILD_DIR/artifacts/gf_core_tests"
RUST_DIR="$REPO_ROOT/rust"

# The programs that hold the tests. A tree that builds the core alone (the
# android flavor) has no gpgfrontend, so its tests are gf_core_tests' only.
TEST_PROGRAMS=(core app)
BUILD_TARGETS=(gpgfrontend gf_core_tests)
# The phone application's own tests are built there, and only there.
MOBILE="no"
MOB_BIN="$BUILD_DIR/artifacts/gf_mob_tests"
MOB_QML_BIN="$BUILD_DIR/artifacts/gf_mob_qml_tests"
if grep -qE '^GPGFRONTEND_BUILD_FLAVOR:STRING=android$' \
     "$BUILD_DIR/CMakeCache.txt" 2>/dev/null; then
  TEST_PROGRAMS=(core)
  BUILD_TARGETS=(gf_core_tests)
  BIN="$CORE_BIN"
  # A cross build for a device has no tests to run here.
  if ! grep -qE '^ANDROID_ABI:' "$BUILD_DIR/CMakeCache.txt" 2>/dev/null; then
    MOBILE="yes"
    BUILD_TARGETS+=(gf_mob_tests gf_mob_qml_tests)
  fi
fi
if [[ "$MODE" == "mobile" && "$MOBILE" != "yes" ]]; then
  echo "error: $BUILD_DIR builds no phone application tests" >&2
  echo "       use an android flavor tree: --build-dir build-android" >&2
  exit 2
fi

# The Rust phases are driven by cargo and need neither the C++ binary nor a
# CMake build, so skip all gpgfrontend build/lookup work in the rust-only modes.
if [[ "$MODE" != "rust" && "$MODE" != "rust-slow" ]]; then
  if [[ "$DO_BUILD" == "yes" ||
        ( "$DO_BUILD" == "auto" && ( ! -x "$BIN" || ! -x "$CORE_BIN" ||
          ( "$MOBILE" == "yes" && ( ! -x "$MOB_BIN" || ! -x "$MOB_QML_BIN" ) ) ) ) ]]; then
    echo "==> Building ${BUILD_TARGETS[*]} ($BUILD_DIR, -j$JOBS)"
    cmake --build "$BUILD_DIR" --target "${BUILD_TARGETS[@]}" -j"$JOBS" || {
      echo "error: build failed" >&2; exit 1; }
  fi

  required_bins=("$BIN" "$CORE_BIN")
  [[ "$MOBILE" == "yes" ]] && required_bins+=("$MOB_BIN" "$MOB_QML_BIN")
  for bin in "${required_bins[@]}"; do
    if [[ ! -x "$bin" ]]; then
      echo "error: test binary not found: $bin" >&2
      echo "       run with --build, or build the 'gpgfrontend' and" \
           "'gf_core_tests' targets first." >&2
      exit 1
    fi
  done

  # The unit tests are opt-in at configure time. Without them the binary has
  # no -t mode, and every phase below would fail as an unknown option.
  if [[ "$MODE" != "modules" ]] &&
     ! grep -qiE '^GPGFRONTEND_BUILD_TESTS:BOOL=(ON|TRUE|YES|Y|1)$' "$BUILD_DIR/CMakeCache.txt" 2>/dev/null; then
    echo "error: $BUILD_DIR was configured without the unit tests" >&2
    echo "       reconfigure with: cmake -B $BUILD_DIR -DGPGFRONTEND_BUILD_TESTS=ON" >&2
    exit 1
  fi
fi

RESULTS_DIR="$BUILD_DIR/test-results"
mkdir -p "$RESULTS_DIR"

if [[ -t 1 ]]; then export GTEST_COLOR="yes"; else export GTEST_COLOR="no"; fi

# With GTEST_COLOR=yes GoogleTest emits its escape sequence *before* the
# bracket, so the teed log holds "\033[0;32m[  PASSED  ]" and an anchored
# '^\[  PASSED  \]' never matches -- a fully green interactive run used to
# report "no GoogleTest summary found" and RESULT: FAIL. Tolerate the prefix
# instead of giving up the colour. Shards write to files with GTEST_COLOR=no,
# so this only matters on the serial interactive path.
GT_ANSI='(\x1b\[[0-9;]*m)?'

# A sanitizer report is a failure even when every assertion passed.
sanitizer_reported() {
  [[ "$ASAN" != "no" ]] &&
    grep -qE 'ERROR: AddressSanitizer|runtime error:|SUMMARY: (Address|Undefined)Sanitizer' "$1"
}

# The program a GoogleTest phase is running in: select_test_binary core|app.
TEST_CMD=()
select_test_binary() {
  case "$1" in
    core) TEST_CMD=("$CORE_BIN") ;;
    app) TEST_CMD=("$BIN" -t) ;;
  esac
}

# --- phase runner ----------------------------------------------------------
# run_phase <name> <gtest-filter> <stress-iterations> [algo-coverage]
# The optional 4th argument is the value for GF_RUN_ALGO_COVERAGE: pass "1" to
# enable the (slow, opt-in) algorithm-generation sweep, or leave it empty so
# those tests skip themselves. It is always exported (empty by default) so a
# value inherited from the caller's environment never leaks into a phase that
# should not run the sweep.
run_phase() {
  local name="$1" filter="$2" iter="$3" algo_coverage="${4:-}"
  local log="$RESULTS_DIR/${name}.log"

  echo
  echo "============================================================"
  echo "  Phase: ${name}"
  echo "  Filter: ${filter}"
  echo "  Log level: ${LOG_LEVEL}"
  echo "  GF_STRESS_ITER: ${iter}"
  echo "  GF_RUN_ALGO_COVERAGE: ${algo_coverage:-<unset>}"
  echo "============================================================"

  GTEST_FILTER="$filter" \
  GTEST_OUTPUT="xml:${RESULTS_DIR}/${name}.xml" \
  GF_STRESS_ITER="$iter" \
  GF_RUN_ALGO_COVERAGE="$algo_coverage" \
    "${TEST_CMD[@]}" -l "$LOG_LEVEL" 2>&1 | tee "$log"

  # Exit code is unreliable; decide from the GoogleTest summary.
  if sanitizer_reported "$log"; then
    echo "error: sanitizer reported an error in phase '${name}'" >&2
    return 1
  fi
  if grep -qE "^${GT_ANSI}\[  FAILED  \]|[0-9]+ FAILED TEST" "$log"; then
    return 1
  fi
  if ! grep -qE "^${GT_ANSI}\[  PASSED  \]" "$log"; then
    echo "warning: no GoogleTest summary found for phase '${name}'" >&2
    return 1
  fi
  return 0
}

# --- sharded phase runner --------------------------------------------------
# Sharding is GoogleTest's own: GTEST_TOTAL_SHARDS/GTEST_SHARD_INDEX assign test
# case i to shard i % N, preserving relative registration order within a shard.
# That ordering guarantee is load-bearing -- GpgCoreTestSubkeygen adds subkeys to
# a fixture key that GpgCoreTestBasicOpera asserts on, and only registration
# order keeps them apart -- so never combine this with --gtest_shuffle.
SHARD_HOME_ROOT="$BUILD_DIR/test-homes"

shard_home() { printf '%s/s%s' "$SHARD_HOME_ROOT" "$1"; }

# Each shard gets a private TMPDIR so the throwaway GNUPGHOMEs the fixtures
# create land somewhere nameable and get wiped, instead of accumulating in /tmp
# (which is how ~300 of them and 1.9 GB got there). It deliberately does NOT
# live under $BUILD_DIR: gpg-agent addresses its sockets through a symlink
# root + "/gfp-" + 8 hex, budgeted against
# 108 - len("/S.gpg-agent.browser") - 1 = 87 bytes (GnuPGSocketBudget::Bytes()),
# and the GnuPGHome* tests build a QTemporaryDir *inside* QDir::tempPath() and
# then a link inside that. A repo-relative path like
# <repo>/build/test-homes/s0/tmp is already 55 bytes, so those tests would blow
# the budget and fail. Keep it short and near the root instead.
shard_tmpdir() { printf '/tmp/gf-test-%s/s%s' "$(id -u)" "$1"; }

# The tests need room for tempPath + "/GpgFrontend-XXXXXX" + a subdir + the
# 13-byte link inside 87 bytes. 40 leaves comfortable margin; anything longer
# and we leave TMPDIR alone and accept the usual /tmp litter rather than break
# the socket-budget tests.
shard_tmpdir_ok() { (( ${#1} <= 40 )); }

# Shard HOMEs are reused across runs so the app-key and profile bootstrap is paid
# once, but they are keyed on the binary's mtime: a stale profile from an older
# binary can hit a schema migration, and every migration prompt goes through
# GuiProfileLoaderDelegate -> modal QMessageBox -> offscreen hang.
prepare_shard_home() {
  local i="$1" home stamp want
  home="$(shard_home "$i")"
  case "$home" in
    "$SHARD_HOME_ROOT"/s[0-9]*) ;;
    *) echo "error: refusing to prepare '$home'" >&2; return 1 ;;
  esac
  stamp="$home/.gf-stamp"
  want="$(stat -c %Y "$BIN" "$CORE_BIN" 2>/dev/null | tr '\n' ' ')"
  if [[ "$FRESH_HOMES" == "yes" || "$(cat "$stamp" 2>/dev/null)" != "$want" ]]; then
    rm -rf -- "$home"
  fi
  mkdir -p "$home"/{.local/share,.local/state,.config,.cache} || return 1
  printf '%s\n' "$want" > "$stamp"

  # The temp dir is always fresh: it is pure scratch, and leaving one run's
  # GNUPGHOMEs behind is exactly what this is meant to stop.
  local tmp; tmp="$(shard_tmpdir "$i")"
  case "$tmp" in
    /tmp/gf-test-[0-9]*/s[0-9]*) rm -rf -- "$tmp"; mkdir -p "$tmp" || return 1 ;;
    *) echo "error: refusing to prepare '$tmp'" >&2; return 1 ;;
  esac
}

# A wedged shard must never outlive the script. Killing the bookkeeping subshell
# is not enough -- it would orphan the `timeout` and the gpgfrontend under it --
# so each shard records the pid of its `timeout`, which forwards the signal on.
SHARD_PIDS_ALL=()
SHARD_PIDFILES=()
kill_shards() {
  local pid f
  for f in ${SHARD_PIDFILES[@]+"${SHARD_PIDFILES[@]}"}; do
    pid="$(cat "$f" 2>/dev/null)"
    [[ "$pid" =~ ^[0-9]+$ ]] && kill -TERM "$pid" 2>/dev/null
  done
  for pid in ${SHARD_PIDS_ALL[@]+"${SHARD_PIDS_ALL[@]}"}; do
    [[ -n "$pid" ]] && kill -TERM "$pid" 2>/dev/null
  done
}
trap 'kill_shards; exit 130' INT
trap 'kill_shards; exit 143' TERM

# run_shard <phase> <index> <total> <filter> <iter> <algo-coverage>
# Backgrounds one shard. Nothing is streamed to the terminal -- N interleaved
# GoogleTest streams are noise -- so each shard owns its own log. The shard
# stamps its exit status into <log>.rc so completion can be reported in finish
# order without depending on `wait -n -p` (bash >= 5.1).
run_shard() {
  local name="$1" i="$2" total="$3" filter="$4" iter="$5" algo="${6:-}"
  local home log rc pidfile
  home="$(shard_home "$i")"
  log="$RESULTS_DIR/${name}-shard${i}.log"
  rc="${log%.log}.rc"
  pidfile="${log%.log}.pid"
  rm -f "$rc" "$pidfile" "${log%.log}.xml"
  SHARD_START[$i]="$SECONDS"
  SHARD_SETTLE[$i]=""

  (
    export HOME="$home"
    export XDG_DATA_HOME="$home/.local/share"   # profile root lives here:
    export XDG_CONFIG_HOME="$home/.config"      #   $XDG_DATA_HOME/BKTUS/GpgFrontend
    export XDG_STATE_HOME="$home/.local/state"
    export XDG_CACHE_HOME="$home/.cache"
    local tmp; tmp="$(shard_tmpdir "$i")"
    if shard_tmpdir_ok "$tmp"; then export TMPDIR="$tmp"; fi
    export QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-offscreen}"
    export GTEST_COLOR="no"
    export GTEST_TOTAL_SHARDS="$total" GTEST_SHARD_INDEX="$i"
    export GTEST_FILTER="$filter"
    export GTEST_OUTPUT="xml:${RESULTS_DIR}/${name}-shard${i}.xml"
    export GF_STRESS_ITER="$iter" GF_RUN_ALGO_COVERAGE="$algo"
    timeout --kill-after=30 "$SHARD_TIMEOUT" "${TEST_CMD[@]}" -l "$LOG_LEVEL" &
    inner=$!
    printf '%s\n' "$inner" > "$pidfile"
    wait "$inner"
    echo $? > "$rc"
  ) >"$log" 2>&1 &

  SHARD_PIDS[$i]=$!
  SHARD_PIDS_ALL+=("$!")
  SHARD_PIDFILES+=("$pidfile")
}

# Has this shard's log reached the final GoogleTest summary yet?
# The "[0-9]+ test" tail is what separates the summary lines
# ("[  FAILED  ] 3 tests, listed below:") from the per-test ones
# ("[  FAILED  ] Suite.Test (0 ms)") that are printed mid-run -- without it, a
# shard that failed an early test would look settled and get terminated while
# it was still running.
shard_has_summary() {
  grep -qE "^${GT_ANSI}\[  (PASSED|FAILED)  \] [0-9]+ test" "$1"
}

# shard_verdict <log> -> PASS | HUNG | FAIL | TIMEOUT | NOSUMMARY
# (returns 0 only for PASS and HUNG)
#
# The GoogleTest summary is the source of truth, exactly as in run_phase -- the
# binary's exit code is unreliable. The process status is consulted only to
# classify the *absence* of a summary, and to label the case where the tests all
# passed but the process then wedged on the way out. That last one is a known
# shutdown race (the DataObjectOperator GC thread vs SingletonStorageCollection
# teardown) that concurrency makes far more likely: ~1 in 12 shards on a short
# run, versus never observed serially. It is not a test failure and must not be
# reported as one, but it is worth saying out loud every time it happens.
shard_verdict() {
  local log="$1" rc
  rc="$(cat "${log%.log}.rc" 2>/dev/null)"
  [[ "$rc" =~ ^[0-9]+$ ]] || rc=1
  if sanitizer_reported "$log" ||
     grep -qE "^${GT_ANSI}\[  FAILED  \]|[0-9]+ FAILED TEST" "$log"; then
    echo FAIL; return 1
  fi
  if grep -qE "^${GT_ANSI}\[  PASSED  \]" "$log"; then
    # 143/124/137: we or `timeout` had to end it after the tests were done.
    if (( rc == 143 || rc == 124 || rc == 137 )); then echo HUNG; return 0; fi
    echo PASS; return 0
  fi
  if (( rc == 124 || rc == 137 )); then echo TIMEOUT; return 1; fi
  echo NOSUMMARY; return 1
}

# gtest_count <log> <bracket-label>  e.g. gtest_count unit.log '  PASSED  '
gtest_count() {
  grep -oE "\[$2\] [0-9]+ test" "$1" 2>/dev/null \
    | grep -oE '[0-9]+' | awk '{s+=$1} END{print s+0}'
}

# Only the failing shards are replayed, one at a time, so a failure reads like a
# serial one instead of N interleaved ones.
dump_shard_failure() {
  local name="$1" i="$2"
  local log="$RESULTS_DIR/${name}-shard${i}.log"
  echo
  echo "------- shard ${i} (${SHARD_STATUS[$i]}) : ${log} -------"
  case "${SHARD_STATUS[$i]}" in
    TIMEOUT|NOSUMMARY) tail -n 80 "$log" ;;
    *)
      # Replay each [ RUN ] .. [  FAILED  ] block; drop the passing ones.
      awk '/\[ RUN      \]/ { buf = $0 "\n"; in_run = 1; next }
           in_run          { buf = buf $0 "\n" }
           /\[       OK \]/ { in_run = 0; next }
           /\[  FAILED  \]/ { if (in_run) printf "%s", buf; in_run = 0 }' "$log" \
        | head -n 400
      sed -n '/\[==========\].*ran\./,$p' "$log" | head -n 60
      ;;
  esac
}

# The gfp-XXXXXXXX socket symlinks are removed on a clean shutdown and leaked on
# a crash or a timeout. Only *dangling* ones are swept, so a link still held by a
# running instance is never touched.
sweep_gnupg_links() {
  local root="${XDG_RUNTIME_DIR:-${TMPDIR:-/tmp}}"
  [[ -d "$root" ]] || return 0
  find "$root" -maxdepth 1 -name 'gfp-????????' -type l ! -xtype d -delete \
    2>/dev/null || true
}

merge_shard_xml() {
  local name="$1" total="$2" merger="$REPO_ROOT/scripts/merge_gtest_xml.py"
  local -a parts=()
  local i
  for ((i = 0; i < total; i++)); do
    [[ -f "$RESULTS_DIR/${name}-shard${i}.xml" ]] && \
      parts+=("$RESULTS_DIR/${name}-shard${i}.xml")
  done
  # Best effort: the per-shard XMLs are the real artifact, the merge only keeps
  # <phase>.xml from going stale. Never let it decide the phase result.
  if (( ${#parts[@]} == 0 )) || ! command -v python3 >/dev/null 2>&1 \
     || [[ ! -f "$merger" ]]; then
    rm -f "$RESULTS_DIR/${name}.xml"
    return 0
  fi
  python3 "$merger" -o "$RESULTS_DIR/${name}.xml" "${parts[@]}" \
    || { echo "warning: could not merge shard XML for '${name}'" >&2
         rm -f "$RESULTS_DIR/${name}.xml"; }
  return 0
}

# run_phase_sharded <name> <gtest-filter> <stress-iterations> [algo-coverage]
# Same contract as run_phase: returns 0/1 and leaves the phase's combined output
# at $RESULTS_DIR/<name>.log, so the summary block needs no special case.
run_phase_sharded() {
  local name="$1" filter="$2" iter="$3" algo_coverage="${4:-}"
  local total="$PARALLEL" i left rc=0

  SHARD_PIDS=(); SHARD_START=(); SHARD_STATUS=(); SHARD_DONE=(); SHARD_SETTLE=()

  # Drop every shard artifact from a previous run first. A run at a lower -p
  # than the last one would otherwise leave higher-numbered logs behind, and
  # both the concatenation and the XML merge would count them again.
  rm -f "$RESULTS_DIR/${name}-shard"*.log "$RESULTS_DIR/${name}-shard"*.xml \
        "$RESULTS_DIR/${name}-shard"*.rc "$RESULTS_DIR/${name}-shard"*.pid

  echo
  echo "============================================================"
  echo "  Phase: ${name}"
  echo "  Filter: ${filter}"
  echo "  Shards: ${total}  (HOMEs: ${SHARD_HOME_ROOT}/s0..s$((total - 1)))"
  echo "  Shard TMPDIR: $(shard_tmpdir 'N')"
  echo "  Log level: ${LOG_LEVEL}"
  echo "  GF_STRESS_ITER: ${iter}"
  echo "  GF_RUN_ALGO_COVERAGE: ${algo_coverage:-<unset>}"
  echo "============================================================"

  for ((i = 0; i < total; i++)); do
    prepare_shard_home "$i" || return 1
    run_shard "$name" "$i" "$total" "$filter" "$iter" "$algo_coverage"
    printf '  [shard %d/%d] started   pid=%-7s %s-shard%d.log\n' \
      "$((i + 1))" "$total" "${SHARD_PIDS[$i]}" "$name" "$i"
  done

  # Poll rather than block on `wait -n`: a shard that has printed its summary
  # but not exited is in the shutdown race described above, and holding the whole
  # phase hostage to --shard-timeout for it would defeat the point of -p. Give it
  # SHUTDOWN_GRACE seconds to leave on its own, then end it and take the summary.
  left="$total"
  while (( left > 0 )); do
    local progressed=""
    for ((i = 0; i < total; i++)); do
      [[ -n "${SHARD_DONE[$i]:-}" ]] && continue
      local log="$RESULTS_DIR/${name}-shard${i}.log"

      if [[ -f "$RESULTS_DIR/${name}-shard${i}.rc" ]]; then
        SHARD_DONE[$i]=1; (( left-- )); progressed=1
        SHARD_STATUS[$i]="$(shard_verdict "$log")" || rc=1
        printf '  [shard %d/%d] %-9s %4ds  passed=%-5s failed=%-4s skipped=%-4s\n' \
          "$((i + 1))" "$total" "${SHARD_STATUS[$i]}" \
          "$(( SECONDS - SHARD_START[i] ))" \
          "$(gtest_count "$log" '  PASSED  ')" \
          "$(gtest_count "$log" '  FAILED  ')" \
          "$(gtest_count "$log" '  SKIPPED ')"
        continue
      fi

      [[ -f "$log" ]] && shard_has_summary "$log" || continue
      if [[ -z "${SHARD_SETTLE[$i]:-}" ]]; then
        SHARD_SETTLE[$i]="$SECONDS"
      elif (( SECONDS - SHARD_SETTLE[i] >= SHUTDOWN_GRACE )); then
        local pid; pid="$(cat "$RESULTS_DIR/${name}-shard${i}.pid" 2>/dev/null)"
        if [[ "$pid" =~ ^[0-9]+$ ]]; then
          kill -TERM "$pid" 2>/dev/null
          SHARD_SETTLE[$i]="$SECONDS"   # re-arm; the reap picks it up next tick
        fi
      fi
    done
    (( left > 0 )) && [[ -z "$progressed" ]] && sleep 1
  done

  local hung=0
  for ((i = 0; i < total; i++)); do
    [[ "${SHARD_STATUS[$i]}" == "HUNG" ]] && (( hung++ ))
  done
  if (( hung > 0 )); then
    echo "  note: ${hung} shard(s) passed their tests but wedged on shutdown and" >&2
    echo "        had to be terminated (known GC-vs-teardown race, not a test failure)" >&2
  fi

  for ((i = 0; i < total; i++)); do
    case "${SHARD_STATUS[$i]}" in
      PASS|HUNG) ;;
      *) dump_shard_failure "$name" "$i" ;;
    esac
  done

  # Enumerate rather than glob: a glob would also pick up higher-numbered logs
  # if some earlier run used a larger -p.
  local -a shard_logs=()
  for ((i = 0; i < total; i++)); do
    shard_logs+=("$RESULTS_DIR/${name}-shard${i}.log")
  done
  cat "${shard_logs[@]}" > "$RESULTS_DIR/${name}.log"
  sweep_gnupg_links
  merge_shard_xml "$name" "$total"
  return "$rc"
}

# Sharding is offered only where it pays. "unit" is 1328 tests over ~109 s and
# splits ~4.3x at N=6; "stress" (~57 s, one 26 s test) and "coverage" (~1467 s,
# one 1051 s test) are each dominated by a single long test, so splitting them
# buys nothing and multiplies concurrent keygen load. To shard a stress subset
# deliberately, use custom mode: -f '*Stress*' -p N -i 50.
declare -A PHASE_SHARDS=()

phase_shards() {
  case "${1%-core}" in
    unit|custom|unit-app|custom-app) echo "$PARALLEL" ;;
    *)
      if (( PARALLEL > 1 )); then
        echo "note: --parallel is ignored for the '${1%-*}' phase (a single long test dominates it)" >&2
      fi
      echo 1 ;;
  esac
}

run_program_phase() {
  local n; n="$(phase_shards "$1")"
  PHASE_SHARDS["$1"]="$n"
  if (( n > 1 )); then
    run_phase_sharded "$@"
  else
    # Clear any shard artifacts a previous parallel run left, so the results
    # directory describes this run and nothing else.
    rm -f "$RESULTS_DIR/${1}-shard"*.log "$RESULTS_DIR/${1}-shard"*.xml \
          "$RESULTS_DIR/${1}-shard"*.rc "$RESULTS_DIR/${1}-shard"*.pid
    run_phase "$@"
  fi
}

# run_gtest_phase <name> <gtest-filter> <stress-iterations> [algo-coverage]
# Runs the phase in gf_core_tests and then in `gpgfrontend -t`, as the
# sub-phases <name>-core and <name>-app, and leaves <name>.log and <name>.xml
# holding both. A filter may well match tests in only one of them; one that
# matches none in either is a mistake, not a pass.
run_gtest_phase() {
  local name="$1"; shift
  local rc=0 kind ran=0
  local -a logs=() xmls=()

  for kind in "${TEST_PROGRAMS[@]}"; do
    select_test_binary "$kind"
    run_program_phase "${name}-${kind}" "$@" || rc=1
    logs+=("$RESULTS_DIR/${name}-${kind}.log")
    [[ -f "$RESULTS_DIR/${name}-${kind}.xml" ]] &&
      xmls+=("$RESULTS_DIR/${name}-${kind}.xml")
  done
  PHASE_SHARDS["$name"]="${PHASE_SHARDS[${name}-core]}"

  cat "${logs[@]}" > "$RESULTS_DIR/${name}.log"
  rm -f "$RESULTS_DIR/${name}.xml"
  if (( ${#xmls[@]} > 0 )) && command -v python3 >/dev/null 2>&1; then
    python3 "$REPO_ROOT/scripts/merge_gtest_xml.py" \
      -o "$RESULTS_DIR/${name}.xml" "${xmls[@]}" ||
      echo "warning: could not merge the XML reports for '${name}'" >&2
  fi

  # "[==========] N tests from M test suites ran." once per program and shard;
  # colour codes stripped first, as GoogleTest puts one after the bracket.
  ran="$(sed 's/\x1b\[[0-9;]*m//g' "$RESULTS_DIR/${name}.log" |
         grep -oE '^\[==========\] [0-9]+ tests? from' | grep -oE '[0-9]+' |
         awk '{s += $1} END {print s + 0}')"
  if (( ran == 0 )); then
    echo "error: the filter of phase '${name}' matched no test in either" \
         "program" >&2
    rc=1
  fi
  return "$rc"
}

# --- rust phase runner -----------------------------------------------------
# Runs the pure-Rust unit tests via `cargo test`. Unlike `gpgfrontend -t`,
# cargo's exit code is authoritative, so it (not log parsing) decides the result.
run_rust_phase() {
  local log="$RESULTS_DIR/rust.log"

  echo
  echo "============================================================"
  echo "  Phase: rust"
  echo "  Runner: cargo test (${RUST_DIR})"
  echo "============================================================"

  if ! command -v cargo >/dev/null 2>&1; then
    echo "warning: cargo not found; skipping rust phase" | tee "$log" >&2
    return 0
  fi

  ( cd "$RUST_DIR" && cargo test ) 2>&1 | tee "$log"
  return "${PIPESTATUS[0]}"
}

# --- rust-slow phase runner ------------------------------------------------
# Runs the slow `#[ignore]`d Rust tests: RSA and post-quantum key generation and
# very large streams. They are excluded from the default `cargo test` because a
# handful of them dominate the suite's wall clock. A low thread count keeps
# several multi-second keygens from contending for the same cores.
#
# `deferred_*` tests are skipped here on purpose: those are also `#[ignore]`d,
# but they encode behavior the engine does not implement yet (see the ignore
# reason on each), so they are expected to fail and must not gate CI. Run them
# deliberately with `cd rust && cargo test -- --ignored deferred_`.
run_rust_slow_phase() {
  local log="$RESULTS_DIR/rust-slow.log"

  echo
  echo "============================================================"
  echo "  Phase: rust-slow"
  echo "  Runner: cargo test -- --ignored (${RUST_DIR})"
  echo "============================================================"

  if ! command -v cargo >/dev/null 2>&1; then
    echo "warning: cargo not found; skipping rust-slow phase" | tee "$log" >&2
    return 0
  fi

  ( cd "$RUST_DIR" && cargo test -- --ignored --skip deferred_ --skip large_file_ --test-threads=2 ) 2>&1 | tee "$log"
  return "${PIPESTATUS[0]}"
}

# --- rust-large phase runner -----------------------------------------------
# The large-file tier (see scripts/run_large_file_tests.sh):
# real multi-GB SEIPD v1/v2 files, each test in its own process with a
# peak-memory assertion. A missing cargo or an empty test list fails.
run_rust_large_phase() {
  local log="$RESULTS_DIR/rust-large.log"

  echo
  echo "============================================================"
  echo "  Phase: rust-large"
  echo "  Runner: scripts/run_large_file_tests.sh"
  echo "  GF_LARGE_FILE_DIR: ${GF_LARGE_FILE_DIR:-<temp dir>}"
  echo "============================================================"

  "$REPO_ROOT/scripts/run_large_file_tests.sh" "$log"
}

# --- modules phase runner --------------------------------------------------
# Runs the modules' own gtest binaries. These are separate executables rather
# than part of `gpgfrontend -t`: the module code lives in the `modules`
# submodule and is deliberately built without the GF SDK behind it, so it
# cannot be linked into the main test library. Like cargo, a plain gtest binary
# returns an authoritative exit code, so it (not log parsing) decides the
# result.
#
# The binaries only exist when the tree was configured with
# -DGPGFRONTEND_MODULES_BUILD_TESTS=ON; without that this phase is a no-op.
run_modules_phase() {
  local log="$RESULTS_DIR/modules.log"
  local dir="$BUILD_DIR/test-bin"

  echo
  echo "============================================================"
  echo "  Phase: modules"
  echo "  Runner: module gtest binaries (${dir})"
  echo "============================================================"

  : > "$log"

  # The list the build wrote, not a glob: a glob that finds *something*
  # cannot tell a full run from one where a registered target silently
  # stopped being built. Which binaries belong on it is the module tree's own
  # statement, made where each test target is defined.
  local list="${dir}/module-tests.list"
  local -a binaries=()
  if [[ -f "$list" ]]; then
    while IFS= read -r b; do
      [[ -n "$b" ]] && binaries+=("$b")
    done < "$list"
  fi

  if [[ ${#binaries[@]} -eq 0 ]]; then
    if [[ "$REQUIRE_MODULES" == "yes" ]]; then
      echo "error: no module tests registered (${list} missing or empty;" \
        "configure with -DGPGFRONTEND_MODULES_BUILD_TESTS=ON)" \
        | tee -a "$log" >&2
      return 1
    fi
    echo "warning: no module tests registered; skipping modules phase" \
      "(configure with -DGPGFRONTEND_MODULES_BUILD_TESTS=ON)" | tee -a "$log" >&2
    return 0
  fi

  # A registered binary that is missing was never built: a failure when a
  # complete run is required, otherwise reported and skipped.
  local -a present=()
  local missing=0
  for b in "${binaries[@]}"; do
    if [[ -x "$b" ]]; then
      present+=("$b")
    else
      echo "$([[ "$REQUIRE_MODULES" == "yes" ]] && echo error || echo warning):" \
        "registered module test binary not built: $(basename "$b")" \
        | tee -a "$log" >&2
      missing=1
    fi
  done
  [[ "$missing" -eq 0 || "$REQUIRE_MODULES" != "yes" ]] || return 1
  binaries=("${present[@]}")

  local rc=0
  for b in "${binaries[@]}"; do
    echo "--- $(basename "$b") ---" | tee -a "$log"
    if ! "$b" 2>&1 | tee -a "$log"; then rc=1; fi
    # tee swallows the binary's status, so take it from PIPESTATUS.
    [[ "${PIPESTATUS[0]}" -eq 0 ]] || rc=1
  done

  return "$rc"
}

# --- mobile phase runner ---------------------------------------------------
# The phone application's tests: its controllers and models on the core
# (gf_mob_tests, GoogleTest) and its QML pages (gf_mob_qml_tests, Qt Quick
# Test). Both return an authoritative exit code. They run offscreen in a HOME
# made afresh for each run, so they never see the real profile, and a run
# that ran nothing is a failure, not a pass.
run_mobile_phase() {
  local log="$RESULTS_DIR/mobile.log"
  local home="$BUILD_DIR/test-homes/mobile"

  echo
  echo "============================================================"
  echo "  Phase: mobile"
  echo "  Runner: $(basename "$MOB_BIN"), $(basename "$MOB_QML_BIN")"
  echo "============================================================"

  : > "$log"
  rm -rf -- "$home"
  mkdir -p "$home"
  home="$(cd "$home" && pwd)"

  local rc=0 b
  for b in "$MOB_BIN" "$MOB_QML_BIN"; do
    echo "--- $(basename "$b") ---" | tee -a "$log"
    if ! env HOME="$home" XDG_CONFIG_HOME="$home/.config" \
           XDG_DATA_HOME="$home/.local/share" XDG_CACHE_HOME="$home/.cache" \
           XDG_STATE_HOME="$home/.local/state" \
           QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-offscreen}" \
           "$b" 2>&1 | tee -a "$log"; then
      rc=1
    fi
    # tee swallows the binary's status, so take it from PIPESTATUS.
    [[ "${PIPESTATUS[0]}" -eq 0 ]] || rc=1
  done

  local qml_ran
  qml_ran="$(grep -oE '^Totals: [0-9]+ passed, [0-9]+ failed' "$log" |
             grep -oE '[0-9]+' | awk '{s += $1} END {print s + 0}')"
  if (( $(gtest_count "$log" '==========') == 0 || qml_ran == 0 )); then
    echo "error: a mobile test program ran no test" | tee -a "$log" >&2
    rc=1
  fi
  return "$rc"
}

# --- run requested phases --------------------------------------------------
overall_rc=0
declare -a phases=()

case "$MODE" in
  custom)
    # Under --asan the module tests run too: they parse untrusted input, which
    # is where a sanitizer is most useful, and a filter is about the main suite.
    if [[ "$ASAN" != "no" ]]; then
      run_modules_phase || overall_rc=1
      phases+=("modules")
    fi
    # Enable the sweep so a custom -f filter can target the coverage tests; it
    # is harmless for any other filter (non-sweep tests ignore the variable).
    run_gtest_phase "custom" "$CUSTOM_FILTER" "$STRESS_ITER" "1" || overall_rc=1
    phases+=("custom")
    ;;
  unit)
    if [[ "$RUN_RUST" != "no" ]]; then
      run_rust_phase || overall_rc=1
      phases+=("rust")
    fi
    run_modules_phase || overall_rc=1
    phases+=("modules")
    run_gtest_phase "unit" "*-*Stress*:${COVERAGE_FILTER}" "$STRESS_ITER" || overall_rc=1
    phases+=("unit")
    if [[ "$MOBILE" == "yes" ]]; then
      run_mobile_phase || overall_rc=1
      phases+=("mobile")
    fi
    ;;
  modules)
    run_modules_phase || overall_rc=1
    phases+=("modules")
    ;;
  asan)
    run_modules_phase || overall_rc=1
    phases+=("modules")
    # The Lua error model rests on no longjmp crossing a live C++ object; a
    # skipped destructor or a leaked allocation shows up here and nowhere else.
    run_gtest_phase "lua" 'Lua*' "$STRESS_ITER" || overall_rc=1
    phases+=("lua")
    run_gtest_phase "stress" '*Stress*' "$STRESS_ITER" || overall_rc=1
    phases+=("stress")
    ;;
  stress)
    run_gtest_phase "stress" '*Stress*' "$STRESS_ITER" || overall_rc=1
    phases+=("stress")
    ;;
  coverage)
    run_gtest_phase "coverage" "$COVERAGE_FILTER" "$STRESS_ITER" "1" || overall_rc=1
    phases+=("coverage")
    ;;
  rust)
    run_rust_phase || overall_rc=1
    phases+=("rust")
    ;;
  mobile)
    run_mobile_phase || overall_rc=1
    phases+=("mobile")
    ;;
  rust-slow)
    run_rust_slow_phase || overall_rc=1
    phases+=("rust-slow")
    ;;
  large-file)
    run_rust_large_phase || overall_rc=1
    phases+=("rust-large")
    export GF_RUN_LARGE_FILE_TESTS=1
    run_gtest_phase "large" '*LargeFile*' "$STRESS_ITER" || overall_rc=1
    phases+=("large")
    # A skipped or filtered-out test must not read as a pass.
    if ! grep -qE '\[ +OK +\] [A-Za-z]+\.LargeFile' "$RESULTS_DIR/large.log"; then
      echo "error: no C++ LargeFile test ran to completion" >&2
      overall_rc=1
    fi
    ;;
  all)
    if [[ "$RUN_RUST" != "no" ]]; then
      run_rust_phase || overall_rc=1
      phases+=("rust")
    fi
    run_modules_phase || overall_rc=1
    phases+=("modules")
    run_gtest_phase "unit" "*-*Stress*:${COVERAGE_FILTER}" "$STRESS_ITER" || overall_rc=1
    phases+=("unit")
    if [[ "$MOBILE" == "yes" ]]; then
      run_mobile_phase || overall_rc=1
      phases+=("mobile")
    fi
    run_gtest_phase "stress" '*Stress*' "$STRESS_ITER" || overall_rc=1
    phases+=("stress")
    run_gtest_phase "coverage" "$COVERAGE_FILTER" "$STRESS_ITER" "1" || overall_rc=1
    phases+=("coverage")
    if [[ "$RUN_RUST" != "no" ]]; then
      run_rust_slow_phase || overall_rc=1
      phases+=("rust-slow")
    fi
    ;;
esac

# --- summary ---------------------------------------------------------------
echo
echo "============================================================"
echo "  Summary"
echo "============================================================"
for p in "${phases[@]}"; do
  log="$RESULTS_DIR/${p}.log"
  if [[ "$p" == "rust" || "$p" == "rust-slow" || "$p" == "rust-large" ]]; then
    # cargo emits one "test result: ok. N passed; M failed; K ignored; ..."
    # line per test binary (lib, integration, doc); sum across all of them.
    passed="$(grep -oE '[0-9]+ passed' "$log" | grep -oE '[0-9]+' | awk '{s+=$1} END{print s+0}')"
    failed="$(grep -oE '[0-9]+ failed' "$log" | grep -oE '[0-9]+' | awk '{s+=$1} END{print s+0}')"
    skipped="$(grep -oE '[0-9]+ ignored' "$log" | grep -oE '[0-9]+' | awk '{s+=$1} END{print s+0}')"
  elif [[ "$p" == "mobile" ]]; then
    # GoogleTest's summary, plus Qt Quick Test's "Totals: P passed, F failed,
    # S skipped" line.
    qml="$(grep -oE '^Totals: [0-9]+ passed, [0-9]+ failed, [0-9]+ skipped' "$log" | head -n1)"
    read -r qp qf qs <<<"$(grep -oE '[0-9]+' <<<"${qml:-0 0 0}" | xargs)"
    passed=$(( $(gtest_count "$log" '  PASSED  ') + ${qp:-0} ))
    failed=$(( $(gtest_count "$log" '  FAILED  ') + ${qf:-0} ))
    skipped=$(( $(gtest_count "$log" '  SKIPPED ') + ${qs:-0} ))
  else
    # Sum rather than take the first match: a sharded phase's log is the
    # concatenation of its shard logs, so it carries one summary per shard.
    # The '[0-9]+ test' tail keeps this from counting the per-test
    # '[  FAILED  ] Suite.Test' lines. Correct for one shard too.
    passed="$(gtest_count "$log" '  PASSED  ')"
    failed="$(gtest_count "$log" '  FAILED  ')"
    skipped="$(gtest_count "$log" '  SKIPPED ')"
  fi
  n="${PHASE_SHARDS[$p]:-1}"
  if (( n > 1 )); then label="${log} (${n} shards)"; else label="$log"; fi
  printf '  %-8s passed=%-4s failed=%-4s skipped=%-4s  (%s)\n' \
    "$p" "${passed:-0}" "${failed:-0}" "${skipped:-0}" "$label"
done
echo "  XML reports: ${RESULTS_DIR}/*.xml"
echo

if [[ "$overall_rc" -eq 0 ]]; then
  echo "RESULT: PASS"
else
  echo "RESULT: FAIL"
fi
exit "$overall_rc"
