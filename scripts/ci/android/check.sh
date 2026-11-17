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
# What signing cannot change, checked where the build tree is, and the
# packages laid out for signing in build/unsigned/{unsigned.apk,unsigned.aab}.
#
#   check.sh <abi>
#
# The libraries: no GnuPG engine in the Android flavour. The APK: its
# application id is $ANDROID_PACKAGE (the one CI uploads to Play under), and
# every native library is 16 KB aligned (apksigner signs without moving
# entries). The AAB: a bundle is not installed as it is (Play or bundletool
# cuts APKs from it, and aligns them then); it must hold its manifest and
# this ABI's libraries, and no signature yet.
#
# Inputs (environment): ANDROID_PACKAGE, GF_ANDROID_NDK, ANDROID_HOME.

set -euo pipefail

ABI="${1:?usage: $0 <abi>}"
: "${ANDROID_PACKAGE:?}" "${GF_ANDROID_NDK:?}" "${ANDROID_HOME:?}"
llvm="$ANDROID_HOME/ndk/$GF_ANDROID_NDK/toolchains/llvm/prebuilt/linux-x86_64/bin"
bt="$ANDROID_HOME/build-tools/36.0.0"

NM="$llvm/llvm-nm" READELF="$llvm/llvm-readelf" scripts/check_no_gpgme.sh build

apk="build/artifacts/GpgFrontend-${ABI}-unsigned.apk"
"$bt/aapt2" dump badging "$apk" | grep -E "^package|targetSdkVersion"
# The id CI uploads to Play under is the one the build gave it.
"$bt/aapt2" dump packagename "$apk" | grep -qx "$ANDROID_PACKAGE" || {
  echo "::error::the APK is not $ANDROID_PACKAGE" >&2; exit 1; }
READELF="$llvm/llvm-readelf" ZIPALIGN="$bt/zipalign" \
  scripts/android/check-elf-alignment.sh "$apk"

aab="build/artifacts/GpgFrontend-${ABI}-unsigned.aab"
entries="$(unzip -Z1 "$aab")"
grep -qx "base/manifest/AndroidManifest.xml" <<< "$entries"
grep -q "^base/lib/${ABI}/.*\.so$" <<< "$entries"
if grep -q "^META-INF/" <<< "$entries"; then
  echo "::error::$aab is already signed" >&2
  exit 1
fi

mkdir -p build/unsigned
cp "$apk" build/unsigned/unsigned.apk
cp "$aab" build/unsigned/unsigned.aab
