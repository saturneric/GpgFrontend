#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# A BOUNDED run of the e-mail module's MIME fuzzer.
#
# The parser runs on wholly unauthenticated input -- a message has to be parsed
# before anything can say whether it is signed, let alone trustworthy -- so it
# has had a libFuzzer harness for some time. Nothing ever ran it: it needs
# clang and a sanitizer build, it is off by default, and it is deliberately not
# named *_test so the ordinary runner will not pick it up and hang forever on
# it. A harness nobody runs is not coverage.
#
# This is the compromise: a short, time-boxed run seeded from the committed
# corpus, cheap enough to sit in CI, which catches a parser that has started
# crashing or hanging without pretending to be a fuzzing campaign.
#
#   scripts/run_fuzz_smoke.sh [--seconds N] [--build-dir DIR]
#
# Requires clang. Exits 0 if the fuzzer ran clean, non-zero on a crash, a
# timeout inside the fuzzer, or a build failure.

set -uo pipefail

SECONDS_TO_RUN=60
BUILD_DIR="build-fuzz"
JOBS="$(nproc 2>/dev/null || echo 4)"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --seconds)   SECONDS_TO_RUN="${2:?missing value for $1}"; shift ;;
    --build-dir) BUILD_DIR="${2:?missing value for $1}"; shift ;;
    -j)          JOBS="${2:?missing value for $1}"; shift ;;
    -h|--help)   sed -n '3,21p' "$0"; exit 0 ;;
    *) echo "unknown option: $1" >&2; exit 2 ;;
  esac
  shift
done

cd "$(dirname "$0")/.." || exit 1

if ! command -v clang++ >/dev/null 2>&1; then
  echo "error: clang++ not found; the fuzzer harness requires clang" >&2
  exit 1
fi

CORPUS="modules/src/m_email/test/corpus/public"
if [[ ! -d "$CORPUS" ]]; then
  echo "error: seed corpus not found at $CORPUS" >&2
  exit 1
fi

if [[ ! -f "$BUILD_DIR/CMakeCache.txt" ]]; then
  echo "==> Configuring fuzzer build in $BUILD_DIR (clang + ASan/UBSan)"
  cmake -S . -B "$BUILD_DIR" -G Ninja \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_C_COMPILER=clang \
    -DCMAKE_CXX_COMPILER=clang++ \
    -DGPGFRONTEND_LINK_GPGME_INTO_CORE=ON \
    -DGPGFRONTEND_BUILD_MODULES=ON \
    -DGPGFRONTEND_MODULES_BUILD_TESTS=ON \
    -DGPGFRONTEND_MODULES_BUILD_FUZZERS=ON \
    -DCMAKE_C_FLAGS="-fsanitize=fuzzer-no-link,address,undefined -fno-omit-frame-pointer" \
    -DCMAKE_CXX_FLAGS="-fsanitize=fuzzer-no-link,address,undefined -fno-omit-frame-pointer" \
    || { echo "error: configure failed" >&2; exit 1; }
fi

echo "==> Building gf_mod_email_fuzz"
cmake --build "$BUILD_DIR" --target gf_mod_email_fuzz -j"$JOBS" \
  || { echo "error: fuzzer build failed" >&2; exit 1; }

FUZZER="$BUILD_DIR/artifacts/module-tests/gf_mod_email_fuzz"
if [[ ! -x "$FUZZER" ]]; then
  echo "error: fuzzer was not built at $FUZZER" >&2
  exit 1
fi

ARTIFACTS="$BUILD_DIR/fuzz-artifacts"
mkdir -p "$ARTIFACTS"

echo "==> Fuzzing for ${SECONDS_TO_RUN}s, seeded from $CORPUS"
# -max_total_time bounds the run; -max_len matches the harness's own cap;
# -rss_limit_mb catches a parser that starts allocating without bound, which a
# crash check alone would miss.
"$FUZZER" "$CORPUS" \
  -max_total_time="$SECONDS_TO_RUN" \
  -max_len=1048576 \
  -rss_limit_mb=2048 \
  -timeout=25 \
  -artifact_prefix="$ARTIFACTS/" \
  -print_final_stats=1
rc=$?

if [[ "$rc" -ne 0 ]]; then
  echo "error: fuzzer exited $rc; any reproducer is under $ARTIFACTS" >&2
  ls -la "$ARTIFACTS" 2>/dev/null
  exit "$rc"
fi

echo "==> Fuzz smoke passed"
