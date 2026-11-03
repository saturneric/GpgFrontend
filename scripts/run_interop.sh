#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Check that the phone application and the desktop can use one profile.
#
# One program at a time on a throwaway profile, as the profile lock requires.
# First a profile the phone makes:
#
#   1. the android-flavor tree's gf_mob_tests starts like gf_mob, makes a key
#      and writes a message encrypted to it and signed by it;
#   2. the desktop tree's gf_core_tests starts as the application
#      (GF_TEST_AS_APPLICATION=1, so key databases come from the profile's
#      settings), finds that key in the phone's keyring -- opened as an rPGP
#      database even where the desktop's DEFAULT one is GnuPG's -- decrypts
#      and verifies the message, and answers it;
#   3. gf_mob_tests decrypts and verifies the answer.
#
# Then a profile the desktop makes: the desktop creates it, the phone opens
# it, and the desktop opens it again; none of them records an engine.
#
# The stages are DISABLED_ tests (ProfileInteropTest.*), so no ordinary run
# sees them. Both trees must be configured with GPGFRONTEND_BUILD_TESTS=ON
# and built; nothing is built here.
#
# Usage: scripts/run_interop.sh [--mobile-build-dir DIR] [--build-dir DIR]
#                               [--keep]
#   --mobile-build-dir DIR  android-flavor host tree (default: build-android)
#   --build-dir DIR         desktop tree (default: build)
#   --keep                  keep the profiles and the exchanged files

set -uo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MOBILE_DIR="${REPO_ROOT}/build-android"
DESKTOP_DIR="${REPO_ROOT}/build"
KEEP="no"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --mobile-build-dir) MOBILE_DIR="$(realpath -m "${2:?$1 needs a directory}")"; shift 2 ;;
    --build-dir) DESKTOP_DIR="$(realpath -m "${2:?$1 needs a directory}")"; shift 2 ;;
    --keep) KEEP="yes"; shift ;;
    -h|--help) sed -n '2,/^set -uo/p' "${BASH_SOURCE[0]}" | sed '$d;s/^# \{0,1\}//'; exit 0 ;;
    *) echo "error: unknown option $1" >&2; exit 2 ;;
  esac
done

MOBILE_BIN="${MOBILE_DIR}/artifacts/gf_mob_tests"
DESKTOP_BIN="${DESKTOP_DIR}/artifacts/gf_core_tests"
for b in "$MOBILE_BIN" "$DESKTOP_BIN"; do
  [[ -x "$b" ]] || { echo "error: $b not built" >&2; exit 2; }
done

WORK="$(mktemp -d "${TMPDIR:-/tmp}/gf-interop.XXXXXX")"
if [[ "$KEEP" == "yes" ]]; then
  echo "keeping ${WORK}"
else
  trap 'rm -rf "$WORK"' EXIT
fi
EXCHANGE="${WORK}/exchange"
mkdir -p "${WORK}/phone-first" "${WORK}/desktop-first" "$EXCHANGE"

# in_home DIR [VAR=VALUE...] CMD...: CMD with HOME and every XDG directory
# under DIR, so nothing reaches a real profile.
in_home() {
  local home="$1"
  shift
  env HOME="$home" XDG_DATA_HOME="$home/.local/share" \
    XDG_CONFIG_HOME="$home/.config" XDG_CACHE_HOME="$home/.cache" \
    XDG_STATE_HOME="$home/.local/state" "$@"
}

# stage HOME NAME BIN FILTER [ENV=VALUE...]: one stage in the profile under
# HOME; it passes only when exactly that one test ran and passed.
stage() {
  local home="$1" name="$2" bin="$3" filter="$4"
  shift 4
  local log="${WORK}/${name}.log"
  echo "== ${name}"
  in_home "$home" QT_QPA_PLATFORM=offscreen \
    GF_INTEROP_DIR="$EXCHANGE" GTEST_FILTER="$filter" \
    GTEST_ALSO_RUN_DISABLED_TESTS=1 "$@" "$bin" > "$log" 2>&1
  local rc=$?
  grep -E '^interop:|^\[  (PASSED|FAILED) +\]|Failure|Expected|Which is|Value of' \
    "$log"
  if [[ $rc -ne 0 ]] || ! grep -q '^\[  PASSED  \] 1 test' "$log"; then
    echo "FAIL ${name} (log: ${log})"
    KEEP="yes"
    trap - EXIT
    return 1
  fi
  echo "PASS ${name}"
}

PHONE="${WORK}/phone-first"
DESK="${WORK}/desktop-first"
stage "$PHONE" mobile-writes "$MOBILE_BIN" 'ProfileInteropTest.DISABLED_MobileWritesAMessage' &&
  stage "$PHONE" desktop-replies "$DESKTOP_BIN" 'ProfileInteropTest.DISABLED_DesktopReadsAndReplies' \
    GF_TEST_AS_APPLICATION=1 &&
  stage "$PHONE" mobile-reads "$MOBILE_BIN" 'ProfileInteropTest.DISABLED_MobileReadsTheReply' &&
  stage "$DESK" desktop-creates "$DESKTOP_BIN" 'ProfileInteropTest.DISABLED_DesktopKeepsItsEngine' \
    GF_TEST_AS_APPLICATION=1 &&
  stage "$DESK" mobile-opens "$MOBILE_BIN" 'ProfileInteropTest.DISABLED_MobileOpensADesktopProfile' &&
  stage "$DESK" desktop-reopens "$DESKTOP_BIN" 'ProfileInteropTest.DISABLED_DesktopKeepsItsEngine' \
    GF_TEST_AS_APPLICATION=1
rc=$?

if [[ $rc -eq 0 ]]; then
  echo "RESULT: PASS"
else
  echo "RESULT: FAIL (kept ${WORK})"
fi
exit $rc
