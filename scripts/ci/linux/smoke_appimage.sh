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
# Launch the built AppImage against a fresh profile and require every module
# the build produced to load.

set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
: "${RUNNER_TEMP:?}"

EXPECTED="$("$ROOT/scripts/build_info.sh" "$ROOT/build" module_count)"

cd "$RUNNER_TEMP"
IMAGE="$(find "$ROOT/build/upload-artifact" -name '*.AppImage' | head -1)"
test -n "$IMAGE" || { echo "no AppImage was built" >&2; exit 1; }

rm -rf squashfs-root
"$IMAGE" --appimage-extract >/dev/null

# A profile that has never existed, and nothing of the runner's touched. XDG_*
# as well as HOME: the defaults derive from HOME, but only if nothing else is
# already exported.
export HOME="$RUNNER_TEMP/smoke-home"
export XDG_DATA_HOME="$HOME/.local/share"
export XDG_CONFIG_HOME="$HOME/.config"
export XDG_CACHE_HOME="$HOME/.cache"
mkdir -p "$XDG_DATA_HOME" "$XDG_CONFIG_HOME" "$XDG_CACHE_HOME"

# Offscreen only if it is actually deployed. linuxdeployqt is asked for
# `-extra-plugins=platforms`, which brings all of them including this one --
# but asking unconditionally for a plugin that is not there aborts before
# main() with "no Qt platform plugin could be initialized", which reads like a
# broken build and is not one. That is exactly how the macOS leg failed.
if find ./squashfs-root -name 'libqoffscreen.so' | grep -q .; then
  export QT_QPA_PLATFORM=offscreen
else
  echo "no offscreen plugin deployed; using the default platform"
fi

STATUS="$RUNNER_TEMP/module-status.json"
rc=0
./squashfs-root/AppRun --module-status "$STATUS" \
  >"$RUNNER_TEMP/smoke-stdout.log" 2>&1 || rc=$?

if [ ! -f "$STATUS" ]; then
  echo "the AppImage produced no status report (exit $rc)" >&2
  echo "--- stdout (a missing shared library appears here) ---" >&2
  cat "$RUNNER_TEMP/smoke-stdout.log" >&2 || true
  exit 1
fi

cat "$STATUS"
"$ROOT/scripts/check_module_status.py" "$STATUS" "$EXPECTED"

test "$rc" -eq 0 || { echo "the app exited $rc" >&2; exit 1; }
