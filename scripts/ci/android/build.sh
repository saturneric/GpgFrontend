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
# Build the Android flavour into build/: the core, the APK and the AAB,
# both unsigned (build/artifacts/GpgFrontend-<abi>-unsigned.{apk,aab}).
#
# Inputs (environment):
#   QT_ARCH                 the Qt kit, android_arm64_v8a or android_x86_64
#   GPGFRONTEND_BUILD_MODE  nightly or release (the application id follows)
#   GF_ANDROID_QT           the Qt version, installed under $RUNNER_TEMP/Qt
#   GF_ANDROID_NDK          the NDK version, under $ANDROID_HOME/ndk
#   ANDROID_HOME, JAVA_HOME_17_X64, RUNNER_TEMP  as GitHub's runner has them

set -euo pipefail

: "${QT_ARCH:?}" "${GPGFRONTEND_BUILD_MODE:?}" "${GF_ANDROID_QT:?}" \
  "${GF_ANDROID_NDK:?}" "${ANDROID_HOME:?}" "${RUNNER_TEMP:?}"

export JAVA_HOME="$JAVA_HOME_17_X64" PATH="$JAVA_HOME_17_X64/bin:$PATH"
ndk="$ANDROID_HOME/ndk/$GF_ANDROID_NDK"
qt="$RUNNER_TEMP/Qt/$GF_ANDROID_QT"
"$qt/$QT_ARCH/bin/qt-cmake" -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DGPGFRONTEND_BUILD_FLAVOR=android \
  -DGPGFRONTEND_BUILD_MODE="$GPGFRONTEND_BUILD_MODE" \
  -DANDROID_SDK_ROOT="$ANDROID_HOME" -DANDROID_NDK_ROOT="$ndk" \
  -DQT_HOST_PATH="$qt/gcc_64"
cmake --build build --parallel
cmake --build build --target apk
cmake --build build --target aab
