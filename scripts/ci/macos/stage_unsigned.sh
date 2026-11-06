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
# Pack the unsigned bundle into build/unsigned/ for the signing job.
#
#   stage_unsigned.sh [--dmg-dressing]
#
# The signing job is forbidden to take configuration, environment or signing
# policy from this artifact, so none is sent: no build-info file, no
# entitlements, no profile. --dmg-dressing adds the two files that dress the
# Developer ID disk image window, which are inert: .DS_Store holds window
# bounds and icon positions, the icns is the mounted volume's icon. Neither can
# reach the signature or the entitlements.

set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"

DRESSING=0
while [ $# -gt 0 ]; do
  case "$1" in
    --dmg-dressing) DRESSING=1; shift ;;
    *) echo "unknown argument: $1" >&2; exit 2 ;;
  esac
done

OUT="$ROOT/build/unsigned"
rm -rf "$OUT"
mkdir -p "$OUT"
# ditto, not zip: it is the only archiver that round-trips a bundle's symlinks
# and permission bits through actions/upload-artifact intact.
ditto -c -k --keepParent "$ROOT/build/package/GpgFrontend.app" \
  "$OUT/GpgFrontend.app.zip"

if [ "$DRESSING" -eq 1 ]; then
  cp "$ROOT/resource/lfs/dmg/DS_Store" "$OUT/"
  # The volume icon is the app icon. Renamed on the way out rather than
  # committed a third time: the repository already carries this exact file
  # twice.
  cp "$ROOT/resource/lfs/icns/GpgFrontend.icns" "$OUT/VolumeIcon.icns"
fi
ls -l "$OUT"
