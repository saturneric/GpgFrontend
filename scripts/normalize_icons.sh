#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Normalize toolbar icons in place so they read as the same size.
#
# Every icon in resource/lfs/icons is a transparent 200x200 canvas, but the
# glyphs inside used to range from ~160 px to full bleed, so a lock looked a
# size larger than a pen. Scaling by the longest side alone is not enough: it
# leaves tall glyphs (locks) looking small next to square ones (key pairs). The
# rule here is optical, equal-area sizing:
#
#   1. trim the glyph to its alpha bounds (w x h);
#   2. scale by s = min(AREA / sqrt(w*h), CAP / max(w, h)), so every glyph's
#      bounding box covers AREA^2 px and no side exceeds CAP px;
#   3. center it on a transparent SIZE x SIZE canvas.
#
# Re-running on an already normalized icon is a no-op up to a pixel of
# rounding.
#
# Usage: scripts/normalize_icons.sh resource/lfs/icons/lock.png [...]
# Requires ImageMagick (convert).

set -euo pipefail

SIZE=200
AREA=160
CAP=180

if [[ $# -eq 0 ]]; then
  echo "usage: $0 <icon.png>..." >&2
  exit 2
fi

for icon in "$@"; do
  read -r w h < <(convert "$icon" -trim -format '%w %h\n' info:)
  geometry=$(awk -v w="$w" -v h="$h" -v a="$AREA" -v c="$CAP" 'BEGIN {
    s = a / sqrt(w * h)
    m = (w > h ? w : h)
    if (c / m < s) s = c / m
    printf "%dx%d", int(w * s + 0.5), int(h * s + 0.5)
  }')
  convert "$icon" -trim +repage -resize "${geometry}!" -background none \
    -gravity center -extent "${SIZE}x${SIZE}" "$icon"
  echo "$icon: ${w}x${h} -> ${geometry}"
done
