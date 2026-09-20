#!/bin/bash
# Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
#
# This file is part of GpgFrontend.
#
# GpgFrontend is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# GpgFrontend is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GpgFrontend. If not, see <https://www.gnu.org/licenses/>.
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Prove a macOS bundle's modules would actually load, and are the ones the
# descriptors name.
#
# Four checks, in the order their failures matter:
#
#   1. every dependency of every module native resolves INSIDE the bundle,
#      the way dyld resolves it;
#   2. the descriptors still verify against the natives, after the deployment
#      tools have finished rewriting load commands;
#   3. the deployment audit (§11.0) over the staging tree;
#   4. every ENTRY native still carries its __GPGFRONTEND binding section.
#
# Check 1 exists because a signed, notarized dmg shipped with three of its four
# modules unable to load: they referenced Qt frameworks the app does not link,
# so macdeployqt had never deployed them, and every other check passed. A
# bundle whose modules cannot load is a well-formed bundle.
#
# One script rather than a copy per workflow. The Developer ID and App Store
# legs assemble the same bundle from the same tree, and the last thing this
# pipeline needs is the same gate maintained twice -- the fault that prompted
# this extraction was one edit applied to two of the places that needed it.

set -euo pipefail

APP=""
NAMESPACE_ROOT=""
PACKAGER=""
EXPECTED=""

while [ $# -gt 0 ]; do
  case "$1" in
    --app)            APP="$2"; shift 2 ;;
    --namespace-root) NAMESPACE_ROOT="$2"; shift 2 ;;
    --packager)       PACKAGER="$2"; shift 2 ;;
    --expect-count)   EXPECTED="$2"; shift 2 ;;
    *) echo "unknown argument: $1" >&2; exit 2 ;;
  esac
done

if [ -z "$APP" ] || [ -z "$NAMESPACE_ROOT" ] || [ -z "$PACKAGER" ] \
   || [ -z "$EXPECTED" ]; then
  echo "usage: $0 --app APP --namespace-root DIR --packager PATH" >&2
  echo "          --expect-count N" >&2
  exit 2
fi

# A count this script cannot read is a broken invocation, not an expectation of
# zero. `test -eq` would accept an empty string as an error, but the message
# would be about integer syntax rather than about the caller.
case "$EXPECTED" in
  ''|*[!0-9]*)
    echo "--expect-count needs a non-negative integer, got \"$EXPECTED\"" >&2
    exit 2
    ;;
esac

test -d "$APP"
test -d "$NAMESPACE_ROOT"
test -x "$PACKAGER"

NATIVE_ROOT="$APP/Contents/Frameworks/GpgFrontendModules"
DESC_ROOT="$APP/Contents/Resources/modules"
FRAMEWORKS="$APP/Contents/Frameworks"

test -d "$NATIVE_ROOT"
test -d "$DESC_ROOT"

echo "--- module dependencies resolve in the bundle ---"
missing=0
checked=0
while IFS= read -r dylib; do
  here="$(dirname "$dylib")"

  # `otool -L` prints the path header, then the dylib's OWN LC_ID_DYLIB, then
  # its dependencies. Dropping just the header leaves the id looking like a
  # dependency -- and since a module's id is `@rpath/libgf_mod_x.dylib`, this
  # gate dutifully looked for it in Contents/Frameworks and reported every
  # module as broken. `otool -D` gives the id alone, so it is excluded by
  # value.
  own_id="$(otool -D "$dylib" 2>/dev/null | tail -n +2 \
              | sed 's/^[[:space:]]*//')"

  while IFS= read -r dep; do
    [ -n "$dep" ] || continue
    case "$dep" in
      /usr/lib/*|/System/*) continue ;;
      @loader_path/*) target="$here/${dep#@loader_path/}" ;;
      @executable_path/*)
        target="$APP/Contents/MacOS/${dep#@executable_path/}" ;;
      @rpath/*) target="$FRAMEWORKS/${dep#@rpath/}" ;;
      *)
        echo "  FAIL  $(basename "$dylib") loads \"$dep\", an absolute path off this machine" >&2
        missing=$((missing + 1))
        continue
        ;;
    esac

    if [ ! -e "$target" ]; then
      echo "  FAIL  $(basename "$dylib") needs \"$dep\"" >&2
      echo "        which is not at $(echo "$target" | sed "s#^$APP/##")" >&2
      missing=$((missing + 1))
    fi
    checked=$((checked + 1))
  done < <(otool -L "$dylib" | tail -n +2 \
             | sed 's/^[[:space:]]*//;s/ (compatibility.*//' \
             | grep -vxF "$own_id")
done < <(find "$NATIVE_ROOT" -type f -name '*.dylib' | sort)

echo "resolved $checked dependency reference(s)"
if [ "$missing" -gt 0 ]; then
  echo "$missing module dependency reference(s) do not resolve in the bundle;" >&2
  echo "these modules would be refused at load on a user's machine" >&2
  exit 1
fi

# The proof the descriptors still describe what is there, now that the
# deployment tools have finished rewriting load commands. They can, because
# macOS binds `apple-binding-id` -- a section codesign and install_name_tool
# both leave alone -- and not a digest of the file.
echo "--- descriptors still verify against the bundled natives ---"
"$PACKAGER" verify-module-set \
  --namespace-root "$DESC_ROOT" \
  --expect-count "$EXPECTED"

echo "--- deployment audit ---"
"$(dirname "$0")/audit_module_natives.sh" \
  --namespace-root "$NAMESPACE_ROOT" \
  --packager "$PACKAGER" \
  --expect-count "$EXPECTED"

echo "--- GpgFrontend binding sections, in the bundle ---"
# ENTRY natives only, taken from what each signed descriptor binds, and checked
# on the copy that SHIPS rather than the staging copy it came from.
#
# Not every dylib under here: since the dependency bundling step a namespace
# also holds private dependencies -- libssl, libcrypto -- and those are
# ordinary third-party libraries that have never carried a GpgFrontend section
# and never should. Requiring one of them would fail the build for a file doing
# exactly what it is supposed to.
#
# Reading the staging tree instead looks equivalent and is not. A bundle
# assembled with NO modules at all passes every check above it -- the
# dependency walk finds nothing to complain about, and the sections are all
# present in the tree the modules were copied FROM. That is how this leg came
# to build a sandboxed app containing none of them.
found=0
while read -r _ key dylib; do
  [ -n "$dylib" ] || continue

  shipped="$NATIVE_ROOT/$key/$(basename "$dylib")"
  if [ ! -f "$shipped" ]; then
    echo "FAIL the descriptor for $key binds $(basename "$dylib")," >&2
    echo "     which is not in the bundle at Contents/Frameworks/GpgFrontendModules/$key/" >&2
    exit 1
  fi

  # otool prints the section contents as bytes; what matters here is that the
  # section exists at all, which is what -sectcreate was asked for and what the
  # descriptor's binding depends on.
  if ! otool -s __GPGFRONTEND __gf_binding "$shipped" \
       | grep -q 'Contents of (__GPGFRONTEND,__gf_binding) section'; then
    echo "FAIL $shipped carries no __GPGFRONTEND,__gf_binding section" >&2
    exit 1
  fi
  printf 'ok   %s/%s\n' "$key" "$(basename "$shipped")"
  found=$((found + 1))
done < <("$PACKAGER" verify-module-set \
           --namespace-root "$NAMESPACE_ROOT" --print-entries \
         | grep '^entry ')

test "$found" -eq "$EXPECTED" || {
  echo "expected $EXPECTED module entry natives, found $found" >&2
  exit 1
}

echo "the bundle's $EXPECTED module(s) verify, resolve and are bound"
