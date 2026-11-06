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
# Does the thing we ship actually work: mount the signed, notarized dmg, assess
# it as macOS would, launch it and count the modules.
#
#   smoke.sh <signed-artifact-dir> <buildinfo-artifact-dir>
#
# Every other macOS check asks whether the bundle is AUTHENTIC. None of them
# asks whether it RUNS. That gap shipped a notarized dmg in which three of four
# modules could not load, caught only because a person opened it and read the
# log.

set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
: "${RUNNER_TEMP:?}"
SIGNED="${1:?signed artifact directory}"
BUILDINFO="${2:?build info directory}"

# ---------------------------------------------------------------------------
echo "::group::Mount the disk image"
DMG="$(find "$SIGNED" -name '*.dmg' | head -1)"
test -n "$DMG" || { echo "no dmg in the artifact" >&2; ls -R "$SIGNED" >&2; exit 1; }

MNT="$RUNNER_TEMP/dmg"
hdiutil attach "$DMG" -nobrowse -noautoopen -readonly -mountpoint "$MNT"
# Copied OUT of the read-only image: the app writes nothing, but a mounted dmg
# is a strange place to launch from and `ditto` keeps the signature intact
# where `cp -r` does not.
APP="$RUNNER_TEMP/GpgFrontend.app"
ditto "$MNT/GpgFrontend.app" "$APP"
hdiutil detach "$MNT"
echo "::endgroup::"

# ---------------------------------------------------------------------------
echo "::group::Assess the signature as macOS would"
# --deep because the modules are nested code: a shallow verify would pass on a
# bundle whose module dylibs are unsigned or mis-signed, which is the failure
# Library Validation exists to catch.
codesign --verify --deep --strict --verbose=2 "$APP"

# What Gatekeeper decides when the app is launched.
spctl --assess --type execute --verbose=4 "$APP"

# The app and every module must share a Team ID. This is the whole basis on
# which Library Validation permits loading them, so it is asserted rather than
# assumed.
APP_TEAM="$(codesign -dv --verbose=4 "$APP" 2>&1 | sed -n 's/^TeamIdentifier=//p')"
test -n "$APP_TEAM" && test "$APP_TEAM" != "not set"
echo "app team identifier: $APP_TEAM"

while IFS= read -r dylib; do
  team="$(codesign -dv --verbose=4 "$dylib" 2>&1 | sed -n 's/^TeamIdentifier=//p')"
  if [ "$team" != "$APP_TEAM" ]; then
    echo "  FAIL  $(basename "$dylib") is team \"$team\", app is \"$APP_TEAM\"" >&2
    exit 1
  fi
  echo "  ok    $(basename "$dylib")"
done < <(find "$APP/Contents/Frameworks/GpgFrontendModules" -type f -name '*.dylib' | sort)

# The exception must not have come back.
if codesign -d --entitlements - --xml "$APP" 2>/dev/null \
     | grep -q 'disable-library-validation'; then
  echo "the shipped app carries disable-library-validation" >&2
  exit 1
fi
echo "library validation is ON"
echo "::endgroup::"

# ---------------------------------------------------------------------------
echo "::group::Launch it and count the modules"
# How many modules this build produced. This job has no build tree, so the
# figure is downloaded from build-info.json rather than derived. Counting the
# descriptors inside the bundle instead would be circular: a module that never
# made it into the bundle would lower both the expectation and the result.
INFO="$(find "$BUILDINFO" -name 'build-info.json' | head -1)"
test -n "$INFO" || { echo "no build-info.json for this leg" >&2; exit 1; }
EXPECTED="$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["module_count"])' "$INFO")"
echo "this build produced $EXPECTED module(s)"

# An isolated HOME, so this touches no runner state and starts from a profile
# that has never existed.
export HOME="$RUNNER_TEMP/smoke-home"
mkdir -p "$HOME"

# Offscreen only if it is actually in the bundle. macdeployqt deploys the
# platform plugins the app uses, which on macOS is `cocoa` alone -- asking
# unconditionally for `offscreen` aborts before main(). And it cannot simply
# be supplied: library validation is ON, so a plugin signed by anyone but us
# would be refused, and copying one INTO the bundle breaks the very signature
# verified above. `cocoa` is the fallback and works: a GitHub macOS runner has
# a window session, and the status option exits as soon as it has written the
# report.
if [ -f "$APP/Contents/PlugIns/platforms/libqoffscreen.dylib" ]; then
  export QT_QPA_PLATFORM=offscreen
else
  echo "no offscreen plugin in the bundle; using the default platform"
fi

STATUS="$RUNNER_TEMP/module-status.json"
rc=0
"$APP/Contents/MacOS/GpgFrontend" --module-status "$STATUS" \
  >"$RUNNER_TEMP/smoke-stdout.log" 2>&1 || rc=$?

if [ ! -f "$STATUS" ]; then
  echo "the signed app produced no status report (exit $rc)" >&2
  echo "--- stdout ---" >&2
  cat "$RUNNER_TEMP/smoke-stdout.log" >&2 || true
  exit 1
fi

cat "$STATUS"
"$ROOT/scripts/check_module_status.py" "$STATUS" "$EXPECTED"

test "$rc" -eq 0 || { echo "the app exited $rc" >&2; exit 1; }
echo "::endgroup::"
