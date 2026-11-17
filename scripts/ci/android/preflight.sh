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
# Fail fast, and helpfully, when an Android signing environment is half
# configured.
#
#   preflight.sh <android-signing|google-play>
#
# Only tests for emptiness; no value is printed, and an unset secret arrives
# as the empty string.

set -euo pipefail

environment="${1:-}"
case "$environment" in
  android-signing)
    names=(ANDROID_KEYSTORE ANDROID_KEYSTORE_PASSWORD ANDROID_KEY_ALIAS
           ANDROID_KEY_PASSWORD)
    help="ANDROID_KEYSTORE is the keystore file (PKCS#12 or JKS), base64
encoded on one line (base64 -w0 release.p12). Keep that file:
an installed app only takes updates signed by the same key."
    ;;
  google-play)
    names=(PLAY_UPLOAD_KEYSTORE PLAY_UPLOAD_KEYSTORE_PASSWORD
           PLAY_UPLOAD_KEY_ALIAS PLAY_UPLOAD_KEY_PASSWORD
           PLAY_SERVICE_ACCOUNT_JSON)
    help="PLAY_UPLOAD_KEYSTORE is the upload keystore (PKCS#12 or JKS),
base64 encoded on one line. PLAY_SERVICE_ACCOUNT_JSON is the
service account's JSON key file, as Google Cloud gives it."
    ;;
  *)
    echo "usage: $0 <android-signing|google-play>" >&2
    exit 2
    ;;
esac

missing=()
for name in "${names[@]}"; do
  [ -n "${!name:-}" ] || missing+=("$name")
done
if [ "${#missing[@]}" -gt 0 ]; then
  echo "The '$environment' environment is missing:" >&2
  for name in "${missing[@]}"; do echo "  - $name" >&2; done
  echo >&2
  echo "Set them under Settings > Environments > $environment." >&2
  echo "$help" >&2
  exit 1
fi
echo "all ${#names[@]} credentials of '$environment' are present"
