#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Audit the native dependency closure of every module namespace in a tree.
#
# This is a DEPLOYMENT-CORRECTNESS check, not descriptor authentication, and
# the difference matters enough to state at the top of the file. A .gfmodule
# binds exactly one entry native and deliberately does not enumerate or
# authenticate the rest of the closure -- copying a dependency graph into JSON
# would create a second truth that can drift while looking authoritative. What
# covers those dependencies at runtime is the platform's own mechanism: Apple
# code signing plus Library Validation on macOS, and nothing at all on Linux
# and Windows.
#
# So this runs at build time, on the build machine, and proves nothing about
# the bytes on a user's disk. What it does catch is the class of failure that
# actually happens: a module that links against a build-tree absolute path, a
# RUNPATH that cannot reach Qt from where the module will finally live, or a
# private helper that was renamed in one place and not the other.
#
# Usage: scripts/audit_module_natives.sh --namespace-root DIR [options]
#     --namespace-root DIR   the tree holding <key>/native/ directories
#     --packager PATH        gf_module_packager, asked which native is each
#                            namespace's entry. Without it every native is
#                            held to the entry's rules, which is stricter than
#                            the format requires of a private helper.
#     --qt-relative PATH     a path, relative to a module's native directory,
#                            that its RUNPATH must be able to reach (Linux)
#     --expect-count N       fail unless exactly N namespaces were audited
#     --warnings-are-errors  treat orphan helpers as failures too
#
# An orphan private helper -- a library in native/ that nothing in the
# namespace imports -- is a WARNING by default. Making it fatal would invent a
# requirement the format does not have; reporting it catches the stale file a
# rename leaves behind.

set -u -o pipefail

NAMESPACE_ROOT=""
QT_RELATIVE=""
EXPECT_COUNT=-1
STRICT=0
PACKAGER=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --namespace-root) NAMESPACE_ROOT="${2:-}"; shift 2 ;;
    --qt-relative) QT_RELATIVE="${2:-}"; shift 2 ;;
    --expect-count) EXPECT_COUNT="${2:--1}"; shift 2 ;;
    --packager) PACKAGER="${2:-}"; shift 2 ;;
    --warnings-are-errors) STRICT=1; shift ;;
    -h|--help) sed -n '3,35p' "$0"; exit 0 ;;
    *) echo "audit_module_natives: unknown argument: $1" >&2; exit 2 ;;
  esac
done

if [[ -z "$NAMESPACE_ROOT" ]]; then
  echo "audit_module_natives: --namespace-root is required" >&2
  exit 2
fi
if [[ ! -d "$NAMESPACE_ROOT" ]]; then
  echo "audit_module_natives: $NAMESPACE_ROOT: this directory does not exist" >&2
  exit 1
fi

# Which file is a namespace's entry, as newline-separated "<key>/<basename>"
# lines. Taken from the signed descriptor via the packager rather than from a
# filename pattern: the pattern is exactly what a rename breaks, and a helper
# misread as an entry is held to rules the format never placed on it.
#
# A plain string rather than an associative array because macOS ships bash
# 3.2, which has no `declare -A`. There are four modules; a grep over four
# lines is not the cost worth a second dialect of this script.
IS_ENTRY=""
ENTRIES_KNOWN=0

if [ -n "$PACKAGER" ]; then
  if [ ! -x "$PACKAGER" ]; then
    echo "audit_module_natives: $PACKAGER: not an executable" >&2
    exit 2
  fi

  # Captured rather than piped, and its stderr deliberately NOT discarded.
  # When this fails it is because the descriptors do not match the natives,
  # and `verify-module-set` says exactly which module and why -- a message
  # worth ten of anything repeated here. Throwing it away once turned "this
  # native was rewritten after its descriptor was signed" into "reported no
  # entries", which sent the reader to the wrong file entirely.
  PACKAGER_OUT="$("$PACKAGER" verify-module-set \
    --namespace-root "$NAMESPACE_ROOT" --print-entries 2>&1)"
  PACKAGER_RC=$?

  if [ "$PACKAGER_RC" -ne 0 ]; then
    echo "audit_module_natives: the module set does not verify, so there is" >&2
    echo "  no authoritative answer to which native is which module's entry." >&2
    echo "  If this ran before the descriptors were finalized, that is the" >&2
    echo "  bug: deployment rewrites the natives, so every descriptor is" >&2
    echo "  stale until the seal and finalize steps have run." >&2
    echo "--- $PACKAGER verify-module-set ---" >&2
    echo "$PACKAGER_OUT" >&2
    exit 1
  fi

  IS_ENTRY="$(printf '%s\n' "$PACKAGER_OUT" | awk '$1 == "entry" {
    n = split($3, parts, "/")
    print $2 "/" parts[n]
  }')"
  [ -n "$IS_ENTRY" ] && ENTRIES_KNOWN=1

  if [ "$ENTRIES_KNOWN" -eq 0 ]; then
    echo "audit_module_natives: $PACKAGER verified the set but named no" >&2
    echo "  entries, which means this tree holds no modules at all." >&2
    exit 1
  fi
fi

# Whether a native is its namespace's entry. Without a packager nothing here
# knows, so everything is treated as one -- stricter, never laxer.
is_entry() {
  [ "$ENTRIES_KNOWN" -eq 0 ] && return 0
  printf '%s\n' "$IS_ENTRY" | grep -qxF "$1/$(basename "$2")"
}

FAILURES=0
WARNINGS=0
AUDITED=0

fail() { echo "  FAIL  $*" >&2; FAILURES=$((FAILURES + 1)); }
warn() { echo "  warn  $*"; WARNINGS=$((WARNINGS + 1)); }

have() { command -v "$1" >/dev/null 2>&1; }

# A path is "from the build tree" if it is absolute and not one of the system
# locations a deployed binary may legitimately name. Checked by shape rather
# than against a list of known build directories, because the interesting case
# is always the build directory nobody thought to list.
is_build_tree_path() {
  local p="$1"
  [[ "$p" != /* ]] && return 1
  case "$p" in
    /usr/lib|/usr/lib/*|/usr/lib64|/usr/lib64/*) return 1 ;;
    /lib|/lib/*|/lib64|/lib64/*) return 1 ;;
    /usr/local/lib|/usr/local/lib/*|/usr/local/lib64|/usr/local/lib64/*)
      return 1 ;;
    /System/*|/Library/Frameworks/*) return 1 ;;
    *) return 0 ;;
  esac
}

audit_elf() {
  local file="$1" ns="$2" native_dir="$3"
  local dyn runpath entry=0
  is_entry "$ns" "$file" && entry=1

  dyn="$(readelf -d "$file" 2>/dev/null)" || {
    fail "$ns/$(basename "$file"): readelf could not read it"
    return
  }

  runpath="$(printf '%s\n' "$dyn" |
    sed -n 's/.*(RUNPATH).*\[\(.*\)\]/\1/p;s/.*(RPATH).*\[\(.*\)\]/\1/p' |
    head -1)"

  if [[ -z "$runpath" ]]; then
    # Only the entry must carry one. A private helper that needs nothing but
    # system libraries legitimately has no RUNPATH, and failing it would be
    # inventing a requirement -- the same mistake the orphan rule avoids.
    if [[ $entry -eq 1 ]]; then
      fail "$ns/$(basename "$file"): it has no RUNPATH, so it can only resolve
        its dependencies from whatever the process already loaded"
    fi
    return
  fi

  local origin_seen=0
  local IFS=':'
  # shellcheck disable=SC2206
  local entries=($runpath)
  unset IFS
  local rpath_entry
  for rpath_entry in "${entries[@]}"; do
    if is_build_tree_path "$rpath_entry"; then
      fail "$ns/$(basename "$file"): its RUNPATH names \"$rpath_entry\", which
        is a build-tree path and will not exist on a user's machine"
    fi
    [[ "$rpath_entry" == \$ORIGIN* || "$rpath_entry" == '${ORIGIN}'* ]] \
      && origin_seen=1
  done

  if [[ $origin_seen -eq 0 && $entry -eq 1 ]]; then
    fail "$ns/$(basename "$file"): its RUNPATH does not mention \$ORIGIN, so it
        cannot find the private helpers beside it"
  fi

  # Reachability is checked against the real directory rather than reasoned
  # about, because the number of levels between a module's native directory
  # and Qt differs per layout and getting it wrong is a startup failure rather
  # than a build failure.
  # Deliberately NOT named `entry`: that is the is-this-the-entry flag above,
  # and shadowing it here made every reachability comparison meaningless while
  # still reporting success.
  if [[ -n "$QT_RELATIVE" && $entry -eq 1 ]]; then
    local reachable=0
    for rpath_entry in "${entries[@]}"; do
      [[ "$rpath_entry" == \$ORIGIN* || "$rpath_entry" == '${ORIGIN}'* ]] \
        || continue
      local resolved="${rpath_entry/\$\{ORIGIN\}/$native_dir}"
      resolved="${resolved/\$ORIGIN/$native_dir}"
      if [[ -d "$resolved" ]] &&
         [[ "$(cd "$resolved" 2>/dev/null && pwd -P)" == \
            "$(cd "$native_dir/$QT_RELATIVE" 2>/dev/null && pwd -P)" ]]; then
        reachable=1
      fi
    done
    if [[ $reachable -eq 0 ]]; then
      fail "$ns/$(basename "$file"): no \$ORIGIN entry in its RUNPATH reaches
        \"$QT_RELATIVE\", where Qt lives in this layout"
    fi
  fi

  # A DT_NEEDED naming a module-owned library that is not in native/ is a
  # broken deployment: the loader will look for it by name and find whatever a
  # system path happens to hold, or nothing.
  while read -r needed; do
    [[ -z "$needed" ]] && continue
    case "$needed" in
      libgf_mod_*|libgf_mail_*)
        if [[ ! -e "$native_dir/$needed" ]]; then
          fail "$ns/$(basename "$file"): it needs \"$needed\", which is
        module-owned and is not in this namespace"
        fi
        ;;
    esac
  done < <(printf '%s\n' "$dyn" |
           sed -n 's/.*(NEEDED).*\[\(.*\)\]/\1/p')
}

audit_macho() {
  local file="$1" ns="$2"
  local install_name

  install_name="$(otool -D "$file" 2>/dev/null | sed -n '2p')"
  if [[ -n "$install_name" ]] && is_build_tree_path "$install_name"; then
    fail "$ns/$(basename "$file"): its install name is \"$install_name\", a
        build-tree path"
  fi

  while read -r dep; do
    [[ -z "$dep" ]] && continue
    case "$dep" in
      @loader_path/*|@rpath/*|@executable_path/*) ;;
      /usr/lib/*|/System/*) ;;
      *)
        fail "$ns/$(basename "$file"): it loads \"$dep\", which is neither
        relocatable nor a system library"
        ;;
    esac
  done < <(otool -L "$file" 2>/dev/null | tail -n +2 |
           sed 's/^[[:space:]]*//;s/ (compatibility.*//')
}

audit_pe() {
  local file="$1" ns="$2" native_dir="$3"

  while read -r imported; do
    [[ -z "$imported" ]] && continue
    case "$imported" in
      gf_mod_*|libgf_mod_*)
        if [[ ! -e "$native_dir/$imported" ]]; then
          fail "$ns/$(basename "$file"): it imports \"$imported\", which is
        module-owned and is not in this namespace"
        fi
        ;;
    esac
  done < <(objdump -p "$file" 2>/dev/null |
           sed -n 's/^\tDLL Name: //p')
}

echo "auditing $NAMESPACE_ROOT"

for ns_dir in "$NAMESPACE_ROOT"/*/; do
  [[ -d "$ns_dir" ]] || continue
  ns="$(basename "$ns_dir")"
  native_dir="${ns_dir}native"
  [[ -d "$native_dir" ]] || continue

  shopt -s nullglob
  natives=("$native_dir"/*.so "$native_dir"/*.so.* "$native_dir"/*.dylib \
           "$native_dir"/*.dll)
  shopt -u nullglob
  [[ ${#natives[@]} -gt 0 ]] || continue

  AUDITED=$((AUDITED + 1))

  # What every native in this namespace names, so an orphan can be spotted
  # afterwards rather than guessed at.
  referenced=""

  for file in "${natives[@]}"; do
    case "$file" in
      *.dylib)
        if have otool; then
          audit_macho "$file" "$ns"
          referenced+=$'\n'"$(otool -L "$file" 2>/dev/null | tail -n +2 |
            sed 's/^[[:space:]]*//;s/ (compatibility.*//' |
            sed 's|.*/||')"
        else
          warn "$ns: otool is not available, so Mach-O dependencies were not
        audited"
        fi
        ;;
      *.dll)
        if have objdump; then
          audit_pe "$file" "$ns" "$native_dir"
          referenced+=$'\n'"$(objdump -p "$file" 2>/dev/null |
            sed -n 's/^\tDLL Name: //p')"
        else
          warn "$ns: objdump is not available, so PE imports were not audited"
        fi
        ;;
      *)
        if have readelf; then
          audit_elf "$file" "$ns" "$native_dir"
          referenced+=$'\n'"$(readelf -d "$file" 2>/dev/null |
            sed -n 's/.*(NEEDED).*\[\(.*\)\]/\1/p')"
        else
          warn "$ns: readelf is not available, so ELF dependencies were not
        audited"
        fi
        ;;
    esac
  done

  # The entry itself is never an orphan; anything else nothing names is a file
  # a rename probably left behind.
  for file in "${natives[@]}"; do
    base="$(basename "$file")"
    is_entry "$ns" "$file" && continue
    if ! printf '%s\n' "$referenced" | grep -qxF "$base"; then
      warn "$ns/native/$base: nothing in this namespace names it; it may be
        left over from a rename"
    fi
  done
done

if [[ $EXPECT_COUNT -ge 0 && $AUDITED -ne $EXPECT_COUNT ]]; then
  fail "$NAMESPACE_ROOT: $AUDITED namespace(s) audited, and $EXPECT_COUNT were
        expected"
fi

if [[ $FAILURES -gt 0 ]] || { [[ $STRICT -eq 1 ]] && [[ $WARNINGS -gt 0 ]]; }; then
  echo "audit_module_natives: this tree is not deployable" >&2
  exit 1
fi

echo "  $AUDITED namespace(s) audited, $WARNINGS warning(s)"
