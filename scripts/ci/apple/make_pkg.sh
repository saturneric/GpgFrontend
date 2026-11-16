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
# Embed the provisioning profile, or build the App Store installer package.
#
#   make_pkg.sh embed-profile <app>   MAS_PROVISIONING_PROFILE_DATA in env;
#                                     run BEFORE signing
#   make_pkg.sh package <app> <out.pkg>
#                                     INSTALLER_IDENTITY in env; run AFTER
#
# No notarization and no `spctl --assess` for an App Store package. Both would
# be wrong: notarytool is for Developer ID distribution, App Store Connect
# validates on upload, and Gatekeeper deliberately rejects an Apple
# Distribution signature outside the App Store, so a correct package would
# fail the assessment.

set -euo pipefail

case "${1:-}" in
  embed-profile)
    APP="${2:?app bundle}"
    : "${MAS_PROVISIONING_PROFILE_DATA:?}"
    # Must happen BEFORE signing: the profile is sealed into the bundle's
    # signature, so embedding it afterwards would invalidate the seal.
    PROFILE="$APP/Contents/embedded.provisionprofile"
    printf '%s' "$MAS_PROVISIONING_PROFILE_DATA" | base64 --decode -o "$PROFILE"
    test -s "$PROFILE"

    # Explicitly 0644. The certificates and the notary key are secret files in
    # $RUNNER_TEMP; this one is written INTO the bundle, so it ships inside
    # every copy of the application. `productbuild` installs the package as
    # root, a 0600 file in it is then readable only by root, and App Store
    # Connect refuses the upload:
    #
    #   ERROR ITMS-90255: the installer package includes files that are only
    #   readable by the root user.
    chmod 0644 "$PROFILE"
    ;;

  package)
    APP="${2:?app bundle}"
    PKG="${3:?output pkg}"
    : "${INSTALLER_IDENTITY:?}"

    # Every file the installer will lay down must be readable by an ordinary
    # user, because `productbuild` installs as root and macOS then verifies
    # the code signature as whoever runs the app. App Store Connect refuses
    # the upload otherwise (ITMS-90255), and it refuses it AFTER a green CI
    # run, a successful signature and a completed upload -- the slowest
    # possible place to learn it. -004 on files: readable by "other". -005 on
    # directories: also traversable.
    UNREADABLE="$( { find "$APP" -type f ! -perm -004
                     find "$APP" -type d ! -perm -005 ; } | sort )"
    if [ -n "$UNREADABLE" ]; then
      echo "these would be installed unreadable by ordinary users:" >&2
      while IFS= read -r path; do
        [ -n "$path" ] || continue
        printf '  %s  %s\n' \
          "$(stat -f%Sp "$path")" "${path#"$APP"/}" >&2
      done <<< "$UNREADABLE"
      echo "App Store Connect refuses this package (ITMS-90255)" >&2
      exit 1
    fi
    echo "every file and directory in the bundle is readable by others"

    mkdir -p "$(dirname "$PKG")"
    productbuild --component "$APP" /Applications \
      --sign "$INSTALLER_IDENTITY" "$PKG"
    # The acceptance test available offline for an App Store package.
    pkgutil --check-signature "$PKG"
    ;;

  *) echo "usage: make_pkg.sh embed-profile <app> | package <app> <out.pkg>" >&2; exit 2 ;;
esac
