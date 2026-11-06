#!/bin/bash
# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
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
# Sign an app bundle explicitly, inside out, with the hardened runtime.
#
#   IDENTITY=<codesign identity> sign_bundle.sh <app> <entitlements.plist>
#
# Only the app itself gets entitlements; nested code never does. Never
# `codesign --deep` to sign: it signs what it finds the way it guesses.

set -euo pipefail
APP="${1:?app bundle}"
ENTITLEMENTS="${2:?entitlements plist}"
: "${IDENTITY:?IDENTITY must be set}"

# `--timestamp` is a network round trip to timestamp.apple.com for every
# single object, and that service throttles and occasionally just fails:
#
#   libgf_sdk.2.2.2.dylib: A timestamp was expected but was not found
#
# A bundle this size signs around a hundred objects. Signing is idempotent
# under --force, so a retry is safe. Only timestamp-shaped failures are
# retried: a wrong identity, a locked keychain or a malformed object must
# still fail on the first attempt and say so -- retrying those would turn a
# clear error into a slow one.
codesign_retrying() {
  local attempt=1
  local output
  while [ "$attempt" -le 5 ]; do
    if output="$(codesign "$@" 2>&1)"; then
      [ -n "$output" ] && printf '%s\n' "$output"
      return 0
    fi
    case "$output" in
      *timestamp*|*Timestamp*|*"network connection"*|*"service is not available"*|*"Connection refused"*)
        echo "  timestamp service failed (attempt $attempt/5), retrying:" >&2
        printf '    %s\n' "$output" >&2
        sleep $((attempt * 5))
        attempt=$((attempt + 1))
        ;;
      *)
        printf '%s\n' "$output" >&2
        return 1
        ;;
    esac
  done
  echo "codesign could not obtain a timestamp after 5 attempts:" >&2
  printf '%s\n' "$output" >&2
  return 1
}

sign_inner() {   # nested code: hardened runtime, never entitlements
  codesign_retrying --force --timestamp --options=runtime --sign "$IDENTITY" "$1"
}

# Gate: fail closed on any executable bundle type this script does not handle
# deliberately. Two nets -- known bundle suffixes, and the structural giveaway
# of a Contents/MacOS directory anywhere but the app root. This is what stops
# the loose Mach-O pass from signing the executable inside an unknown bundle
# while leaving that bundle unsigned. If GpgFrontend ever gains an XPC
# service, an appex, a helper app or a plugin bundle, its signing and
# entitlements get added here on purpose.
UNEXPECTED="$(
  find "$APP/Contents" -type d \
    \( -name '*.app' -o -name '*.appex' -o -name '*.xpc' \
       -o -name '*.bundle' -o -name '*.systemextension' \
       -o -name '*.pluginkit' -o -name '*.qlgenerator' \)
  find "$APP/Contents" -type d -name 'MacOS' ! -path "$APP/Contents/MacOS"
)"
if [ -n "$UNEXPECTED" ]; then
  echo "unexpected nested code bundle(s); extend sign_bundle.sh deliberately:" >&2
  printf '%s\n' "$UNEXPECTED" >&2
  exit 1
fi

# Pass 1: loose Mach-O files outside any framework -- the gf_* dylibs Xcode
# embeds, the module natives, and every Qt plugin and third-party dylib
# macdeployqt copied in.
#
# NB: `[ x ] && continue` would abort the loop under `set -e` whenever the
# test is false, so the skips are written as full if-blocks.
find "$APP/Contents" -type f ! -path '*.framework/*' | while IFS= read -r f; do
  if [ "$f" = "$APP/Contents/MacOS/GpgFrontend" ]; then
    continue
  fi
  if file -b "$f" | grep -q 'Mach-O'; then
    sign_inner "$f"
  fi
done

# Pass 2: framework bundles, signed at the .framework path. The Versions/A
# plus symlink layout is the normal shape of a macOS framework, not a reason
# to sign a subdirectory. Nested code a framework carries of its own is signed
# first; its own principal executable is not signed separately, because the
# bundle signature is what covers it.
sign_framework() {
  local fw="$1"
  local name
  name="$(basename "$fw" .framework)"
  find "$fw" -type f | while IFS= read -r f; do
    case "$f" in
      "$fw"/*.framework/*) continue ;;       # deeper framework, done
      "$fw"/Versions/*/"$name") continue ;;  # versioned framework binary
      "$fw"/"$name") continue ;;             # flat framework binary
    esac
    if file -b "$f" | grep -q 'Mach-O'; then
      sign_inner "$f"
    fi
  done
  sign_inner "$fw"
}

# -d walks depth first, so an inner framework is fully signed before the one
# that contains it.
find -d "$APP/Contents" -type d -name '*.framework' | while IFS= read -r fw; do
  sign_framework "$fw"
done

# Pass 3: the application itself, last, and the only thing that gets
# entitlements. Through the same retry: it is the single call whose loss
# wastes the whole job.
codesign_retrying --force --timestamp --options=runtime \
  --entitlements "$ENTITLEMENTS" --sign "$IDENTITY" "$APP"

# --deep survives here for verification only, where recursing over everything
# is exactly what is wanted. It never signs.
codesign --verify --deep --strict --verbose=4 "$APP"
