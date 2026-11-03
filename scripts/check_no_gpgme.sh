#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Prove that a build without the GnuPG engine (GPGFRONTEND_BUILD_FLAVOR=android)
# carries none of GpgME, libgpg-error or libassuan.
#
# The build leaves them out on purpose: the GnuPG backend is not compiled and
# nothing links them. This checks the result rather than the intent, because a
# development machine usually has their headers in /usr/include, so a stray
# #include compiles anyway and is only caught where they are absent, which is
# the cross build.
#
# Three rules:
#
#   1. No compiled object included gpgme.h, gpg-error.h or assuan.h.
#   2. No binary needs libgpgme, libgpg-error or libassuan.
#   3. No binary defines or references a gpgme_, gpg_err_, gpgrt_ or assuan_
#      symbol.
#
# Usage: scripts/check_no_gpgme.sh <build-dir>
# NM and READELF may name the tools to use, for a cross build.

set -euo pipefail

build_dir=${1:?usage: $0 <build-dir>}
nm_tool=${NM:-nm}
readelf_tool=${READELF:-readelf}
failed=0

fail() {
  echo "FAIL: $*" >&2
  failed=1
}

indent() {
  while IFS= read -r line; do echo "  $line"; done
}

header_pattern='/(gpgme|gpg-error|assuan)\.h$'
symbol_pattern='^_?(gpgme_|gpg_err_|gpgrt_|assuan_)'
library_pattern='lib(gpgme|gpg-error|assuan)\.'

# 1. Headers, from the dependencies the compiler recorded.
if [[ -f "$build_dir/build.ninja" ]]; then
  deps=$(cd "$build_dir" && ninja -t deps)
else
  deps=$(find "$build_dir" \( -name '*.d' -o -name 'compiler_depend.make' \) \
    -exec cat {} +)
fi
if [[ -z "$deps" ]]; then
  fail "no recorded header dependencies in $build_dir; build it first"
fi
included=$(printf '%s\n' "$deps" | sed -E 's/[[:space:]]+/\n/g' |
  grep -E "$header_pattern" | sort -u || true)
if [[ -n "$included" ]]; then
  fail "GnuPG library headers were included:"
  indent <<<"$included" >&2
fi

# 2. and 3. The binaries: every program, library and module the build put in
# artifacts/, which in the android flavor is the core, the startup library and
# the core tests.
mapfile -t binaries < <(find "$build_dir/artifacts" -type f \
  \( -name '*.so' -o -name '*.so.*' -o -name '*.a' -o -perm -u+x \) |
  while IFS= read -r file; do
    # ELF files and archives only: scripts and data are not linked.
    case "$(head -c 4 "$file" | od -An -c | tr -d ' ')" in
      177ELF | '!<ar') echo "$file" ;;
    esac
  done | sort)
if [[ ${#binaries[@]} -eq 0 ]]; then
  fail "no binaries in $build_dir/artifacts"
fi

for binary in "${binaries[@]}"; do
  if [[ "$binary" != *.a ]]; then
    needed=$("$readelf_tool" -d "$binary" | grep NEEDED |
      grep -E "$library_pattern" || true)
    [[ -n "$needed" ]] && fail "$binary needs: $needed"
  fi

  # The symbol table, and the dynamic one for a binary that was stripped.
  symbols=$({ "$nm_tool" "$binary" 2>/dev/null
    "$nm_tool" -D "$binary" 2>/dev/null; } | awk '{print $NF}' |
    grep -E "$symbol_pattern" | sort -u || true)
  if [[ -n "$symbols" ]]; then
    fail "$binary has GnuPG library symbols:"
    indent <<<"$symbols" >&2
  fi
done

if [[ $failed -ne 0 ]]; then
  exit 1
fi
echo "OK: no GpgME, libgpg-error or libassuan in ${#binaries[@]} binaries"
