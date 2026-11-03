#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Check that an Android APK runs on devices with 16 KB memory pages.
#
# Two things have to hold, and they fail separately:
#
#  * every shared object in the APK -- ours, Qt's, anything bundled -- has all
#    of its LOAD segments aligned to at least 16 KB (readelf -l, the Align
#    column). One 4 KB-aligned library is enough for the app not to start on a
#    16 KB device;
#  * the APK stores uncompressed libraries at 16 KB boundaries (zipalign -P 16),
#    so they can be mapped straight out of it.
#
# Usage: scripts/android/check-elf-alignment.sh <app.apk>
#
# READELF and ZIPALIGN override the tools; by default readelf from PATH and the
# newest zipalign in $ANDROID_SDK_ROOT/build-tools.

set -euo pipefail

apk="${1:?usage: $0 <app.apk>}"
[[ -f "$apk" ]] || { echo "error: no such file: $apk" >&2; exit 2; }

readelf_bin="${READELF:-readelf}"
zipalign_bin="${ZIPALIGN:-}"
if [[ -z "$zipalign_bin" ]]; then
  sdk="${ANDROID_SDK_ROOT:-${ANDROID_HOME:-}}"
  zipalign_bin="$(find "$sdk"/build-tools -name zipalign -type f 2>/dev/null |
    sort -V | tail -n 1 || true)"
fi
[[ -x "$zipalign_bin" ]] || { echo "error: zipalign not found; set ZIPALIGN" >&2; exit 2; }

work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT
unzip -q -o "$apk" 'lib/*' -d "$work"

failed=0
count=0
while IFS= read -r -d '' so; do
  count=$((count + 1))
  name="${so#"$work"/}"
  while read -r align; do
    if (( align < 0x4000 )); then
      printf 'FAIL %s: LOAD segment aligned to %#x\n' "$name" "$align"
      failed=1
      break
    fi
  done < <("$readelf_bin" -lW "$so" | awk '$1 == "LOAD" { print $NF }')
done < <(find "$work/lib" -name '*.so' -print0)

if (( count == 0 )); then
  echo "error: the APK contains no shared objects" >&2
  exit 2
fi
(( failed )) || echo "ELF: all ${count} shared objects are 16 KB aligned"

if "$zipalign_bin" -c -P 16 -v 4 "$apk" > "$work/zipalign.log" 2>&1; then
  echo "APK: zip alignment verified for 16 KB pages"
else
  grep -v ' (OK' "$work/zipalign.log" | head -n 20
  echo "FAIL APK: not zip-aligned for 16 KB pages"
  failed=1
fi

exit "$failed"
