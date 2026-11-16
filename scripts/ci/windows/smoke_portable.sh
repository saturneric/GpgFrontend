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
# Extract the portable archive, launch it with only Windows' own directories on
# PATH, and require every module the build produced to load.

set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
: "${RUNNER_TEMP:?}"
cd "$ROOT"

ZIP="$(find build/upload-artifact -name '*-portable.zip' | head -1)"
test -n "$ZIP" || { echo "no portable archive was built" >&2; exit 1; }

SMOKE="$(cygpath -u "$RUNNER_TEMP")/smoke"
rm -rf "$SMOKE"
mkdir -p "$SMOKE"
unzip -q "$ZIP" -d "$SMOKE"
test -f "$SMOKE/bin/GpgFrontend.exe"

# A CI runner has no desktop to draw on, and windeployqt deploys only the
# `windows` platform plugin. The offscreen one is copied into the EXTRACTED
# tree, never into the shipped payload: it is needed to run the test and would
# be dead weight in a user's download. It cannot affect module loading either
# way.
OFFSCREEN=/mingw64/share/qt6/plugins/platforms/qoffscreen.dll
if [ -f "$OFFSCREEN" ]; then
  cp "$OFFSCREEN" "$SMOKE/bin/platforms/"
  export QT_QPA_PLATFORM=offscreen
else
  echo "::warning::no offscreen plugin; trying the default platform"
fi

# A profile that has never existed. Portable builds keep the profile beside the
# executable, so most of this lands under $SMOKE anyway -- the rest is set so
# that nothing reaches the runner's own profile.
USERPROFILE="$(cygpath -w "$SMOKE/home")"
LOCALAPPDATA="$(cygpath -w "$SMOKE/home/AppData/Local")"
APPDATA="$(cygpath -w "$SMOKE/home/AppData/Roaming")"
export USERPROFILE LOCALAPPDATA APPDATA
mkdir -p "$SMOKE/home/AppData/Local" "$SMOKE/home/AppData/Roaming"

# How many modules this build produced, from the list CMake writes at configure
# time. Never a literal: a hardcoded count means adding a module breaks every
# gate at once, and the obvious repair is to bump the number in each, which is
# how a gate stops checking.
EXPECTED="$(./scripts/build_info.sh build module_count)"
STATUS="$SMOKE/module-status.json"

# Only Windows' own directories on PATH for the launch. With the msys2 shell's
# PATH, a DLL the payload forgot is quietly found in /mingw64/bin and the
# launch proves nothing about the archive.
#
# What this launch covers: startup (the executable and the gf_* libraries load,
# so their whole import closure is in the archive) and module loading (every
# module DLL is mapped, so its import closure is too). What it does not:
# anything loaded later by name, and no TLS connection is made.
WINDOWS_PATH="$(cygpath -S):$(cygpath -W)"
cd "$SMOKE/bin"
rc=0
PATH="$WINDOWS_PATH" ./GpgFrontend.exe --module-status "$(cygpath -w "$STATUS")" \
  >"$SMOKE/stdout.log" 2>&1 || rc=$?

if [ ! -f "$STATUS" ]; then
  echo "the portable build produced no status report (exit $rc)" >&2
  echo "--- stdout (a missing DLL announces itself here) ---" >&2
  cat "$SMOKE/stdout.log" >&2 || true
  exit 1
fi

cat "$STATUS"
python "$ROOT/scripts/check_module_status.py" "$STATUS" "$EXPECTED"

test "$rc" -eq 0 || { echo "the app exited $rc" >&2; exit 1; }

# Which TLS backend Qt gets, checked by file, not by a handshake. Schannel is
# the system's own and needs nothing from the archive. Qt's OpenSSL backend
# would load libssl and libcrypto by name, which no launch above can see, and
# the build deliberately does not deploy them for it. So it must not arrive
# here unnoticed, e.g. if windeployqt stops skipping it.
test -f tls/qschannelbackend.dll \
  || { echo "the Schannel TLS backend is missing" >&2; exit 1; }
if [ -e tls/qopensslbackend.dll ]; then
  echo "tls/qopensslbackend.dll shipped, but nothing deploys the" \
       "libssl/libcrypto it loads by name" >&2
  exit 1
fi
