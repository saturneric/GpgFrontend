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
# Fail fast, and helpfully, when a signing environment is half configured.
#
#   preflight.sh <developer-id|app-store>
#
# Only tests for emptiness; no value is printed, and an unset secret arrives
# as the empty string.

set -euo pipefail

case "${1:-}" in
  developer-id)
    environment=macos-signing
    required=(DEVELOP_ID_CERT DEVELOP_ID_CERT_PWD DEVELOPER_ID_CODE_SIGN_IDENTITY
              ASC_API_KEY_P8 ASC_KEY_ID ASC_ISSUER_ID)
    help=(
      "The ASC_* trio comes from an App Store Connect API key:"
      "  App Store Connect > Users and Access > Integrations >"
      "  App Store Connect API > Team Keys > generate a key with the"
      "  Developer role. ASC_API_KEY_P8 is the whole .p8 file including"
      "  its BEGIN/END PRIVATE KEY lines (downloadable only once),"
      "  ASC_KEY_ID is the Key ID column, ASC_ISSUER_ID the Issuer ID"
      "  shown above the table."
    )
    ;;
  app-store)
    environment=mas-signing
    required=(MAS_APPLICATION_CERT MAS_APPLICATION_CERT_PWD
              MAS_APPLICATION_CODE_SIGN_IDENTITY MAS_INSTALLER_CERT
              MAS_INSTALLER_CERT_PWD MAS_INSTALLER_CODE_SIGN_IDENTITY
              MAS_PROVISIONING_PROFILE_DATA)
    help=(
      "The two certificates are exported from Keychain Access as .p12"
      "and base64-encoded; the profile is the App Store"
      ".provisionprofile, also base64-encoded."
    )
    ;;
  *) echo "usage: preflight.sh developer-id|app-store" >&2; exit 2 ;;
esac

missing=()
for name in "${required[@]}"; do
  [ -n "${!name:-}" ] || missing+=("$name")
done

if [ "${#missing[@]}" -gt 0 ]; then
  echo "The '$environment' environment is missing:" >&2
  printf '  - %s\n' "${missing[@]}" >&2
  echo >&2
  echo "Set them under Settings > Environments > $environment." >&2
  printf '%s\n' "${help[@]}" >&2
  exit 1
fi
echo "all ${#required[@]} signing credentials are present"
