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
# Sign the arm64-v8a bundle with Play's upload key and send it to the
# internal track of $ANDROID_PACKAGE's app.
#
#   play.sh <unsigned dir>
#
# Play App Signing re-signs what users get, so the bundle is signed with
# the upload key (jarsign.sh PLAY_UPLOAD), not the release key. The release
# is named by what the run publishes: a release or a release candidate
# after its tag, a beta or a nightly after its version. The versionCode is
# the build number cmake/GitVersion.cmake derived; a build Play already
# holds one as great for (a fix or beta of an older series, or a rerun) is
# skipped (play_upload.py --only-if-newer).
#
# Inputs (environment): PLAY_UPLOAD_KEYSTORE, PLAY_UPLOAD_KEYSTORE_PASSWORD,
# PLAY_UPLOAD_KEY_ALIAS, PLAY_UPLOAD_KEY_PASSWORD, PLAY_SERVICE_ACCOUNT_JSON,
# ARTIFACT_VERSION, BUILD_NUMBER, ANDROID_PACKAGE, PUBLISH_KIND,
# GITHUB_REF_NAME, RUNNER_TEMP.

set -euo pipefail

unsigned="${1:?usage: $0 <unsigned dir>}"
: "${ARTIFACT_VERSION:?}" "${BUILD_NUMBER:?}" "${ANDROID_PACKAGE:?}" \
  "${PUBLISH_KIND:?}" "${RUNNER_TEMP:?}"
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

case "$PUBLISH_KIND" in
  final | rc) name="$GITHUB_REF_NAME" ;;
  beta) name="beta-${ARTIFACT_VERSION}" ;;
  nightly-main) name="nightly-${ARTIFACT_VERSION}" ;;
  *)
    echo "::error::no Play release for publish kind $PUBLISH_KIND" >&2
    exit 1
    ;;
esac

aab="$RUNNER_TEMP/play/$("$here/../artifact_name.sh" android-arm64-v8a.aab)"
"$here/jarsign.sh" PLAY_UPLOAD "$unsigned/unsigned.aab" "$aab"

python3 -I scripts/android/play_upload.py \
  --package "$ANDROID_PACKAGE" \
  --bundle "$aab" \
  --track internal \
  --release-name "$name" \
  --version-code "$BUILD_NUMBER" \
  --only-if-newer
