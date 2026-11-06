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
# Check what the signature on a signed bundle actually carries.
#
#   verify_signature.sh <developer-id|app-store> <app> <entitlements.plist>

set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
TARGET="${1:?target}"
APP="${2:?app bundle}"
EXPECTED="${3:?entitlements plist}"
ACTUAL="$(mktemp)"
trap 'rm -f "$ACTUAL"' EXIT

codesign -d --entitlements - --xml "$APP" > "$ACTUAL" 2>/dev/null

# Equality in both directions: an entitlement that appears only in the signed
# result is as much a defect as a missing one. For the App Store build this
# catches a lost app-sandbox key or a wrong application-identifier, either of
# which App Store Connect rejects on upload.
python3 "$HERE/compare_entitlements.py" "$EXPECTED" "$ACTUAL"

# Entitlements are inert without the hardened runtime, and that is a property
# of the signature, not of the entitlement dictionary.
if ! codesign -d --verbose=2 "$APP" 2>&1 | grep -q 'flags=.*runtime'; then
  echo "hardened runtime is not enabled on the signed app" >&2
  exit 1
fi

case "$TARGET" in
  app-store)
    # The profile has to have survived signing, sealed inside the bundle.
    test -s "$APP/Contents/embedded.provisionprofile"
    echo "embedded.provisionprofile present after signing"
    ;;
  developer-id)
    # Named explicitly, not left to the equality check above, which would pass
    # just as happily if someone re-added this one to the policy file as well.
    # Modules ship inside Contents/Frameworks, signed with the app's own
    # identity, which is precisely the case Library Validation exists to
    # permit. If modules stop loading, the fix is a module signed with the
    # wrong Team ID -- not this exception.
    if grep -q 'disable-library-validation' "$ACTUAL"; then
      echo "the signed app carries disable-library-validation;" >&2
      echo "module code is signed with the app's identity and does not" >&2
      echo "need it -- see resource/entitlements/Normal.entitlements" >&2
      exit 1
    fi
    echo "library validation is ON: no disable-library-validation entitlement"
    ;;
  *) echo "unknown target $TARGET" >&2; exit 2 ;;
esac
