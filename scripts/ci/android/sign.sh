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
# Sign the APK and the AAB of one ABI with the release key, and check that
# one key signed both.
#
#   sign.sh <arch> <unsigned dir>
#
# The APK takes apksigner (Android 11+ wants its v2+ schemes), the AAB
# jarsigner (jarsign.sh). Every APK is signed by the same key, so a newer
# build installs over an older one on a phone. The names come from the
# arch and $ARTIFACT_VERSION, never from the artifact:
#
#   $RUNNER_TEMP/upload-artifact/GpgFrontend-<version>-android-<arch>.apk
#       the release download
#   $RUNNER_TEMP/upload-bundle/GpgFrontend-<version>-android-<arch>.aab
#       the bundle, which goes up apart and never reaches a release
#
# Inputs (environment): ANDROID_KEYSTORE, ANDROID_KEYSTORE_PASSWORD,
# ANDROID_KEY_ALIAS, ANDROID_KEY_PASSWORD, ARTIFACT_VERSION, ANDROID_HOME,
# RUNNER_TEMP, JAVA_HOME_17_X64.

set -euo pipefail

ARCH="${1:?usage: $0 <arch> <unsigned dir>}"
unsigned="${2:?}"
: "${ANDROID_KEYSTORE:?}" "${ANDROID_KEY_ALIAS:?}" "${ARTIFACT_VERSION:?}" \
  "${ANDROID_HOME:?}" "${RUNNER_TEMP:?}"
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

export JAVA_HOME="$JAVA_HOME_17_X64" PATH="$JAVA_HOME_17_X64/bin:$PATH"
bt="$ANDROID_HOME/build-tools/36.0.0"
ks="$(mktemp "$RUNNER_TEMP/release.XXXXXX")"
trap 'rm -f "$ks"' EXIT
printf '%s' "$ANDROID_KEYSTORE" | base64 -d > "$ks"

out="$RUNNER_TEMP/upload-artifact"
mkdir -p "$out"
apk="$out/GpgFrontend-${ARTIFACT_VERSION}-android-${ARCH}.apk"
"$bt/apksigner" sign --ks "$ks" --ks-key-alias "$ANDROID_KEY_ALIAS" \
  --ks-pass env:ANDROID_KEYSTORE_PASSWORD \
  --key-pass env:ANDROID_KEY_PASSWORD \
  --out "$apk" "$unsigned/unsigned.apk"
rm -f "$apk.idsig"
"$bt/apksigner" verify --verbose --print-certs "$apk" |
  grep -E "^Verified using|^Signer #1 certificate (DN|SHA-256)"

aab="$RUNNER_TEMP/upload-bundle/GpgFrontend-${ARTIFACT_VERSION}-android-${ARCH}.aab"
"$here/jarsign.sh" ANDROID "$unsigned/unsigned.aab" "$aab"

# Both packages by the one key, compared in apksigner's notation.
apk_cert="$("$bt/apksigner" verify --print-certs "$apk" |
  sed -n 's/^Signer #1 certificate SHA-256 digest: //p')"
aab_cert="$(keytool -printcert -jarfile "$aab" |
  sed -n 's/^[[:space:]]*SHA256: //p' | tr -d ':' | tr 'A-F' 'a-f')"
echo "AAB signer certificate SHA-256: $aab_cert"
if [ -z "$apk_cert" ] || [ "$apk_cert" != "$aab_cert" ]; then
  echo "::error::the APK and the AAB were not signed by the same key" >&2
  exit 1
fi
