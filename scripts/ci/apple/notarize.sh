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
# Notarize one file, then staple the ticket to what is shipped.
#
#   notarize.sh <submission> [<staple-target>]
#
# notarytool only accepts an archive, a disk image or a package, so a bundle
# travels as a throwaway zip and the ticket is stapled to the bundle itself:
# pass the zip as <submission> and the bundle as <staple-target>. Without a
# second argument the submission is stapled.
#
# Environment: ASC_API_KEY_P8, ASC_KEY_ID, ASC_ISSUER_ID, RUNNER_TEMP.

set -euo pipefail
SUBMISSION="${1:?file to notarize}"
STAPLE_TARGET="${2:-$SUBMISSION}"
# Guarded again here: this script is what actually spends the credential, and
# a clear message beats "parameter null or not set".
: "${ASC_API_KEY_P8:?not set - add it to the macos-signing environment}"
: "${ASC_KEY_ID:?not set - add it to the macos-signing environment}"
: "${ASC_ISSUER_ID:?not set - add it to the macos-signing environment}"
: "${RUNNER_TEMP:?}"

KEY_PATH="$RUNNER_TEMP/asc_api_key.p8"
RESULT="$RUNNER_TEMP/notary-result.json"
# The key exists on disk only for the length of this one submission.
trap 'rm -f "$KEY_PATH"' EXIT
(umask 077 && printf '%s\n' "$ASC_API_KEY_P8" > "$KEY_PATH")

xcrun notarytool submit "$SUBMISSION" \
  --key "$KEY_PATH" --key-id "$ASC_KEY_ID" --issuer "$ASC_ISSUER_ID" \
  --wait --output-format json > "$RESULT"

# --wait exits 0 even when the verdict is Invalid, so read the verdict back
# explicitly rather than trusting the exit status.
STATUS="$(plutil -extract status raw -o - "$RESULT")"
if [ "$STATUS" != "Accepted" ]; then
  SUBMISSION_ID="$(plutil -extract id raw -o - "$RESULT")"
  echo "notarization of $(basename "$SUBMISSION") returned: $STATUS" >&2
  xcrun notarytool log "$SUBMISSION_ID" \
    --key "$KEY_PATH" --key-id "$ASC_KEY_ID" --issuer "$ASC_ISSUER_ID" >&2 || true
  exit 1
fi
echo "notarization accepted: $(basename "$SUBMISSION")"
rm -f "$KEY_PATH"

# Both staple and validate fetch the ticket from Apple's CloudKit service, and
# a dropped connection there (NSURLError -1005, stapler exit 68) says nothing
# about the ticket itself. By this point the notarization is accepted, so
# retry rather than lose the job. Each verb retries on its own: a staple that
# already landed is not redone just because the validate after it hit the
# network. The window (~5.5 min per verb) is sized for CloudKit outages that
# outlast a couple of minutes: a ~100s window was seen to lose all five
# attempts to one.
MAX_ATTEMPTS=8
for verb in staple validate; do
  for attempt in $(seq 1 "$MAX_ATTEMPTS"); do
    if xcrun stapler "$verb" "$STAPLE_TARGET"; then
      break
    fi
    if [ "$attempt" -eq "$MAX_ATTEMPTS" ]; then
      echo "stapler $verb $(basename "$STAPLE_TARGET") failed after $MAX_ATTEMPTS attempts" >&2
      exit 1
    fi
    echo "stapler $verb failed (attempt $attempt/$MAX_ATTEMPTS), retrying" >&2
    sleep $(( attempt * 15 < 60 ? attempt * 15 : 60 ))
  done
done
