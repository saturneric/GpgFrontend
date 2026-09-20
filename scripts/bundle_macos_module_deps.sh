#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Bring each module's private native dependencies into its own namespace, and
# point the module at them with @loader_path.
#
# ## What this is for
#
# A module native links whatever it needs -- m_email pulls OpenSSL in through
# vmime's TLS backend -- and Homebrew builds its dylibs with an ABSOLUTE
# install name: `/opt/homebrew/opt/openssl@3/lib/libssl.3.dylib`. That path is
# correct on the build machine and wrong on a user's: Homebrew may not be
# installed at all, and on Intel Macs it lives under /usr/local instead. A
# module shipped that way fails to load, with a dyld error naming a path the
# user has never heard of.
#
# Linux has no equivalent problem because linuxdeployqt already does this, and
# Windows because windeployqt does. macOS has `macdeployqt`, but it processes
# the .app bundle and the module natives are not in it -- so nothing was doing
# this for them, and nothing noticed until the dependency audit was pointed at
# a macOS tree.
#
# ## Why @loader_path and this directory
#
# This is exactly what the per-module `native/` directory is for. A module owns
# its namespace; private dependencies live beside the entry and the platform
# loader resolves them from there. That is the design's own answer to private
# dependencies (they are deployment artifacts, not descriptor contents), and it
# means two modules cannot collide on a dependency's file name.
#
# The descriptor is untouched and does not need regenerating: macOS binds its
# entry by `apple-binding-id`, an identifier embedded in a Mach-O section at
# link time, not by a digest of the file. `install_name_tool` rewrites load
# commands and `codesign` rewrites __LINKEDIT; neither touches section
# contents. This step could not be run in this position on a platform that
# bound file bytes.
#
# Usage: scripts/bundle_macos_module_deps.sh --namespace-root DIR
#
# NOTE: macOS only. It needs otool, install_name_tool and codesign.

set -u -o pipefail

NAMESPACE_ROOT=""

while [ $# -gt 0 ]; do
  case "$1" in
    --namespace-root) NAMESPACE_ROOT="${2:-}"; shift 2 ;;
    -h|--help) sed -n '3,40p' "$0"; exit 0 ;;
    *) echo "bundle_macos_module_deps: unknown argument: $1" >&2; exit 2 ;;
  esac
done

[ -n "$NAMESPACE_ROOT" ] || { echo "bundle_macos_module_deps: --namespace-root is required" >&2; exit 2; }
[ -d "$NAMESPACE_ROOT" ] || { echo "bundle_macos_module_deps: $NAMESPACE_ROOT: no such directory" >&2; exit 1; }

for tool in otool install_name_tool codesign; do
  command -v "$tool" >/dev/null || {
    echo "bundle_macos_module_deps: $tool is required (this is macOS only)" >&2
    exit 2
  }
done

# A dependency that is already relocatable, or is guaranteed present on every
# Mac, is left exactly as it is. Copying a system library into an application
# is how you ship a stale libSystem and get refused by Gatekeeper.
is_system_or_relocatable() {
  case "$1" in
    @loader_path/*|@rpath/*|@executable_path/*) return 0 ;;
    /usr/lib/*|/System/*) return 0 ;;
    *) return 1 ;;
  esac
}

# Load commands only, and never the file's own id.
#
# `otool -L` prints THREE kinds of line: the path header first, then the
# dylib's own LC_ID_DYLIB, then the actual dependencies. `tail -n +2` drops
# only the header and leaves the id looking exactly like a dependency, so a
# dylib appears to depend on itself. That was true here and did no harm only
# because a module's id is `@rpath/...`, which the filter above happens to
# skip -- and it did real harm the moment the same shape was used somewhere
# that resolves @rpath.
#
# `otool -D` prints the id and nothing else, so it can be excluded by value.
dependencies_of() {
  local id
  id="$(otool -D "$1" 2>/dev/null | tail -n +2 | sed 's/^[[:space:]]*//')"
  otool -L "$1" | tail -n +2 | sed 's/^[[:space:]]*//;s/ (compatibility.*//' \
    | { if [ -n "$id" ]; then grep -vxF "$id"; else cat; fi; }
}

# install_name_tool invalidates whatever signature a Mach-O had, and on Apple
# Silicon an invalidly signed dylib does not load AT ALL -- it is not a warning
# there, it is a hard refusal. Homebrew ships ad-hoc signed dylibs, and the
# linker ad-hoc signs its own output, so every file touched here needs one
# again. sign-macos replaces these properly later; this only has to keep the
# file loadable in between.
resign_adhoc() {
  codesign --remove-signature "$1" 2>/dev/null || true
  codesign --force --sign - "$1" || {
    echo "  FAIL  $1 could not be ad-hoc signed" >&2
    return 1
  }
}

FAILURES=0
fail() { echo "  FAIL  $*" >&2; FAILURES=$((FAILURES + 1)); }

echo "bundling module dependencies under $NAMESPACE_ROOT"

for ns_dir in "$NAMESPACE_ROOT"/*/; do
  [ -d "$ns_dir" ] || continue
  native_dir="${ns_dir}native"
  [ -d "$native_dir" ] || continue

  ns="$(basename "$ns_dir")"
  copied_any=0

  # To a fixpoint: a bundled dependency has dependencies of its own. libssl
  # pulls libcrypto, and stopping after one pass would leave the second
  # pointing back into the Cellar.
  pass=0
  while [ "$pass" -lt 16 ]; do
    pass=$((pass + 1))
    changed=0

    for macho in "$native_dir"/*.dylib "$native_dir"/*.so; do
      [ -f "$macho" ] || continue
      file -b "$macho" | grep -q 'Mach-O' || continue

      while IFS= read -r dep; do
        [ -n "$dep" ] || continue
        is_system_or_relocatable "$dep" && continue

        base="$(basename "$dep")"
        local_copy="$native_dir/$base"

        if [ ! -f "$local_copy" ]; then
          if [ ! -f "$dep" ]; then
            fail "$ns/$(basename "$macho") needs \"$dep\", which is not on this machine"
            continue
          fi
          cp "$dep" "$local_copy" || { fail "$dep could not be copied into $ns"; continue; }
          # Homebrew's cellar is read-only; the copy inherits that.
          chmod u+w "$local_copy"
          install_name_tool -id "@loader_path/$base" "$local_copy" \
            || { fail "$base: its id could not be rewritten"; continue; }
          resign_adhoc "$local_copy" || continue
          echo "  bundled $ns/$base"
          copied_any=1
          changed=1
        fi

        install_name_tool -change "$dep" "@loader_path/$base" "$macho" \
          || { fail "$(basename "$macho"): \"$dep\" could not be rewritten"; continue; }
        resign_adhoc "$macho" || continue
        changed=1
      done < <(dependencies_of "$macho")
    done

    [ "$changed" -eq 0 ] && break
  done

  if [ "$pass" -ge 16 ]; then
    fail "$ns: its dependency closure did not settle in 16 passes"
  fi
  [ "$copied_any" -eq 0 ] && echo "  ok      $ns needs nothing bundled"
done

if [ "$FAILURES" -gt 0 ]; then
  echo "bundle_macos_module_deps: this tree is not deployable" >&2
  exit 1
fi

echo "done; the dependency audit is what proves it worked"
