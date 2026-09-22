#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Prove that a built module reaches the host ONLY through GFHostApi.
#
# A module is handed a capability table at activation, minted from its signed
# manifest, and everything it may do it does through that table. A direct link
# edge to the host defeats the whole arrangement: a module that calls
# GFGpgDecrypt by name never passes the gate that would have refused it.
#
# The build already forbids this -- gf_add_module() links gf_module_runtime
# and nothing else, refuses a host library anywhere in the link closure, and
# passes -Wl,--no-undefined on Linux. This script is the check on the ARTEFACT
# rather than on the intent, which is worth having separately because a link
# line is a claim and a binary is evidence: a dlopen, a hand-written build, or
# a linker that quietly tolerated something all show up here and nowhere else.
#
# Three rules, per native:
#
#   1. No undefined dynamic symbol named GF*. Every such symbol would have to
#      be resolved by the host at load time, which is the old bypass exactly.
#   2. The only defined GF* export is GFModuleGetApi, the one symbol the
#      bootstrap requires. Anything else is host API leaking back out.
#   3. No host implementation library in the needed list: libgf_core,
#      libgf_ui, libgf_sdk.
#
# Usage: scripts/check_module_boundary.sh [options] [FILE...]
#     --namespace-root DIR   scan DIR/<key>/native/* instead of naming files
#                            (default: build/artifacts/modules)
#     --expect-count N       fail unless exactly N natives were checked
#     --quiet                only report failures
#
# Exits non-zero on the first rule broken, naming the file and the symbols.
#
# Tool-guarded like scripts/audit_module_natives.sh: where the platform's
# inspector is missing the check SKIPS with a message rather than passing
# silently, because "we could not look" and "we looked and it was fine" are
# different answers and only one of them is a pass.

set -euo pipefail

NAMESPACE_ROOT="build/artifacts/modules"
EXPECT_COUNT=""
QUIET=0
FILES=()

while [[ $# -gt 0 ]]; do
  case "$1" in
    --namespace-root) NAMESPACE_ROOT="$2"; shift 2 ;;
    --expect-count)   EXPECT_COUNT="$2";   shift 2 ;;
    --quiet)          QUIET=1;             shift ;;
    -h|--help)        sed -n '3,40p' "$0"; exit 0 ;;
    -*) echo "check_module_boundary: unknown option $1" >&2; exit 2 ;;
    *)  FILES+=("$1"); shift ;;
  esac
done

have() { command -v "$1" >/dev/null 2>&1; }
say()  { [[ "$QUIET" -eq 1 ]] || echo "$@"; }

# The one symbol a module is required to export, and the only one it may.
readonly ALLOWED_EXPORT="GFModuleGetApi"
readonly FORBIDDEN_LIBS="libgf_core|libgf_ui|libgf_sdk"

failures=0
checked=0

fail() {
  echo "check_module_boundary: FAIL $1" >&2
  shift
  while [[ $# -gt 0 ]]; do echo "    $1" >&2; shift; done
  failures=$((failures + 1))
}

check_elf() {
  local f="$1"

  if ! have nm || ! have readelf; then
    say "  SKIP $f (nm/readelf not available)"
    return 0
  fi

  local undef exported needed
  undef=$(nm -D --undefined-only "$f" 2>/dev/null \
            | grep -oE '\bGF[A-Z][A-Za-z0-9_]*' | sort -u || true)
  exported=$(nm -D --defined-only "$f" 2>/dev/null \
            | grep -oE '\bGF[A-Z][A-Za-z0-9_]*' | sort -u || true)
  needed=$(readelf -d "$f" 2>/dev/null \
            | grep -E 'NEEDED' | grep -oE "(${FORBIDDEN_LIBS})[^]]*" || true)

  if [[ -n "$undef" ]]; then
    fail "$f imports host symbols; it must reach the host only through the \
GFHostApi table it is given at activation" $undef
  fi

  local extra
  extra=$(echo "$exported" | grep -v "^${ALLOWED_EXPORT}$" || true)
  if [[ -n "$extra" ]]; then
    fail "$f exports GF* symbols other than ${ALLOWED_EXPORT}" $extra
  fi

  if [[ -n "$needed" ]]; then
    fail "$f links a host implementation library" $needed
  fi
}

check_macho() {
  local f="$1"

  if ! have nm || ! have otool; then
    say "  SKIP $f (nm/otool not available)"
    return 0
  fi

  local undef exported needed
  undef=$(nm -gU -u "$f" 2>/dev/null \
            | grep -oE '\b_?GF[A-Z][A-Za-z0-9_]*' | sed 's/^_//' | sort -u \
            || true)
  exported=$(nm -gU --defined-only "$f" 2>/dev/null \
            | grep -oE '\b_?GF[A-Z][A-Za-z0-9_]*' | sed 's/^_//' | sort -u \
            || true)
  needed=$(otool -L "$f" 2>/dev/null \
            | grep -oE "(${FORBIDDEN_LIBS})[^ ]*" || true)

  if [[ -n "$undef" ]]; then
    fail "$f imports host symbols" $undef
  fi
  local extra
  extra=$(echo "$exported" | grep -v "^${ALLOWED_EXPORT}$" || true)
  if [[ -n "$extra" ]]; then
    fail "$f exports GF* symbols other than ${ALLOWED_EXPORT}" $extra
  fi
  if [[ -n "$needed" ]]; then
    fail "$f links a host implementation library" $needed
  fi
}

check_pe() {
  local f="$1"

  if ! have objdump; then
    say "  SKIP $f (objdump not available)"
    return 0
  fi

  # A PE import table names the DLL and the symbol together, which is more
  # than ELF gives us -- so the forbidden-library rule and the forbidden-symbol
  # rule are one read.
  local imports exported
  imports=$(objdump -p "$f" 2>/dev/null \
            | grep -iE "DLL Name: (gf_core|gf_ui|gf_sdk)" || true)
  exported=$(objdump -p "$f" 2>/dev/null \
            | grep -oE '\bGF[A-Z][A-Za-z0-9_]*' | sort -u || true)

  if [[ -n "$imports" ]]; then
    fail "$f imports from a host implementation dll" $imports
  fi
  local extra
  extra=$(echo "$exported" | grep -v "^${ALLOWED_EXPORT}$" || true)
  if [[ -n "$extra" ]]; then
    fail "$f exports GF* symbols other than ${ALLOWED_EXPORT}" $extra
  fi
}

check_one() {
  local f="$1"
  [[ -f "$f" ]] || { fail "$f does not exist"; return; }
  checked=$((checked + 1))
  say "  $f"

  case "$f" in
    *.dll) check_pe "$f" ;;
    *.dylib) check_macho "$f" ;;
    *) check_elf "$f" ;;
  esac
}

if [[ ${#FILES[@]} -eq 0 ]]; then
  if [[ ! -d "$NAMESPACE_ROOT" ]]; then
    echo "check_module_boundary: no such namespace root: $NAMESPACE_ROOT" >&2
    exit 2
  fi
  while IFS= read -r -d '' f; do
    FILES+=("$f")
  done < <(find "$NAMESPACE_ROOT" -mindepth 3 -maxdepth 3 -type f \
             \( -name '*.so' -o -name '*.dylib' -o -name '*.dll' \) -print0 \
           | sort -z)
fi

say "check_module_boundary: ${#FILES[@]} native(s)"
for f in "${FILES[@]}"; do check_one "$f"; done

if [[ -n "$EXPECT_COUNT" && "$checked" -ne "$EXPECT_COUNT" ]]; then
  echo "check_module_boundary: expected $EXPECT_COUNT native(s), checked \
$checked" >&2
  exit 1
fi

if [[ "$failures" -ne 0 ]]; then
  echo "check_module_boundary: $failures native(s) broke the boundary" >&2
  exit 1
fi

say "check_module_boundary: ok"
