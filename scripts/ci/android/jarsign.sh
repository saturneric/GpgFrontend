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
# Sign an Android App Bundle with jarsigner, the only signer for a bundle,
# and check it.
#
#   jarsign.sh <PREFIX> <unsigned.aab> <signed.aab>
#
# The key is the one the environment holds under <PREFIX>: <PREFIX>_KEYSTORE
# (the keystore, base64 on one line), <PREFIX>_KEYSTORE_PASSWORD,
# <PREFIX>_KEY_ALIAS and <PREFIX>_KEY_PASSWORD; ANDROID for the release key,
# PLAY_UPLOAD for Play's upload key. The keystore is written to $RUNNER_TEMP
# for the run of this script only; the passwords never reach a command line.
# Without -strict: a self-signed release certificate is normal on Android,
# and -strict fails on its chain warning.

set -euo pipefail

prefix="${1:?usage: $0 <PREFIX> <unsigned.aab> <signed.aab>}"
in="${2:?}"
out="${3:?}"
keystore_var="${prefix}_KEYSTORE"
alias_var="${prefix}_KEY_ALIAS"
: "${!keystore_var:?}" "${!alias_var:?}" "${RUNNER_TEMP:?}"

export JAVA_HOME="${JAVA_HOME_17_X64:-$JAVA_HOME}"
export PATH="$JAVA_HOME/bin:$PATH"
ks="$(mktemp "$RUNNER_TEMP/jarsign.XXXXXX")"
trap 'rm -f "$ks"' EXIT
printf '%s' "${!keystore_var}" | base64 -d > "$ks"

mkdir -p "$(dirname "$out")"
jarsigner -keystore "$ks" \
  -storepass:env "${prefix}_KEYSTORE_PASSWORD" \
  -keypass:env "${prefix}_KEY_PASSWORD" \
  -digestalg SHA-256 \
  -signedjar "$out" "$in" \
  "${!alias_var}"
jarsigner -verify "$out" | grep -x "jar verified."
keytool -printcert -jarfile "$out" | grep -E "^[[:space:]]*SHA256:"
