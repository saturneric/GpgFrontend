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
# Build and sign the Developer ID disk image.
#
#   IDENTITY=<codesign identity> make_dmg.sh <app> <dressing-dir> <out.dmg>
#
# <dressing-dir> holds DS_Store and VolumeIcon.icns from the build job. They
# are inert Finder data -- window bounds, icon positions, the volume icon --
# and cannot reach the signature or the entitlements.
#
# hdiutil rather than create-dmg: the signing job runs Apple tooling only.
# create-dmg drove Finder over AppleScript to place the icons, which is exactly
# the part that flakes on a headless runner. The committed .DS_Store carries
# the same layout without Finder ever being involved.

set -euo pipefail
APP="${1:?app bundle}"
DRESSING="${2:?dressing directory}"
DMG="${3:?output dmg}"
: "${IDENTITY:?IDENTITY must be set}"
: "${RUNNER_TEMP:?}"

STAGE="$RUNNER_TEMP/dmg-root"
RW="$RUNNER_TEMP/rw.dmg"
MNT="$RUNNER_TEMP/dmg-mnt"
rm -rf "$STAGE"
mkdir -p "$STAGE" "$(dirname "$DMG")"
ditto "$APP" "$STAGE/$(basename "$APP")"
ln -s /Applications "$STAGE/Applications"
cp "$DRESSING/DS_Store" "$STAGE/.DS_Store"
cp "$DRESSING/VolumeIcon.icns" "$STAGE/.VolumeIcon.icns"

# A read/write image first: the volume's custom-icon bit can only be set on a
# mounted volume, and hdiutil cannot set it from -srcfolder.
#
# -fs HFS+ is deliberate. Left to itself hdiutil now builds an APFS image,
# which attaches as a nested container -- detaching that by mount point
# unmounts the volume but fails to eject the image, leaving the convert step
# reading a still-attached file. HFS+ is also the traditional read-only
# app-image format, mountable everywhere.
#
# -srcfolder makes hdiutil attach the new image internally and detach it once
# the copy is done; on hosted runners XProtect or Spotlight scanning the fresh
# volume can hold it at that detach ("create failed - Resource busy"), so
# retry from a clean file each time.
for attempt in 1 2 3 4 5; do
  rm -f "$RW"
  if hdiutil create -format UDRW -fs HFS+ -volname GpgFrontend \
    -srcfolder "$STAGE" -ov "$RW"; then
    break
  fi
  if [ "$attempt" -eq 5 ]; then
    echo "could not create the disk image after 5 attempts" >&2
    exit 1
  fi
  echo "hdiutil create failed (attempt $attempt/5), retrying" >&2
  # A failed create can leave its internal attachment behind; drop it so the
  # next attempt does not trip over the same busy device.
  hdiutil info \
    | awk -v img="$RW" 'index($0, img) {f=1} f && /^\/dev\/disk/ {print $1; exit}' \
    | while read -r dev; do
        hdiutil detach "$dev" -force > /dev/null 2>&1 || true
      done || true
  sleep $((attempt * 5))
done

# Detach by device node, not by mount point: the first line of `hdiutil
# attach` output is the image's top-level device, and that is the only handle
# that reliably ejects the whole image.
ATTACH_OUT="$(hdiutil attach "$RW" -nobrowse -noautoopen -readwrite -noverify -mountpoint "$MNT")"
printf '%s\n' "$ATTACH_OUT"
DEV="$(printf '%s\n' "$ATTACH_OUT" | awk 'NR==1 {print $1}')"

# Without the custom-icon attribute Finder ignores .VolumeIcon.icns. SetFile
# ships with the Xcode command line tools and is deprecated, so warn rather
# than fail if a future runner image drops it.
if command -v SetFile > /dev/null 2>&1; then
  SetFile -a C "$MNT"
else
  echo "::warning::SetFile not found; dmg volume icon not applied"
fi

# Spotlight or a volume scan can hold the image briefly after SetFile, so
# retry before resorting to -force.
for attempt in 1 2 3 4 5; do
  if hdiutil detach "$DEV" > /dev/null 2>&1; then
    DEV=""
    break
  fi
  echo "detach attempt ${attempt} failed; retrying"
  sleep 3
done
if [ -n "$DEV" ]; then
  hdiutil detach "$DEV" -force
fi

hdiutil convert "$RW" -format UDZO -o "$DMG" -ov
rm -f "$RW"

# Same timestamp exposure as every other signature, and by this point the app
# is built, signed, notarized and stapled -- so losing the job to a throttled
# timestamp service here is the most expensive place for it to happen.
for attempt in 1 2 3 4 5; do
  if codesign --force --timestamp --sign "$IDENTITY" "$DMG"; then
    break
  fi
  if [ "$attempt" -eq 5 ]; then
    echo "could not sign the disk image after 5 attempts" >&2
    exit 1
  fi
  echo "dmg signing failed (attempt $attempt/5), retrying" >&2
  sleep $((attempt * 5))
done
