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
# Create a temporary signing keychain, or delete it again.
#
#   keychain.sh create     import the certificates from the environment
#   keychain.sh delete     remove the keychain and any leftover credential file
#
# Environment for create: APP_CERT, APP_CERT_PWD (base64 .p12 for codesign),
# and optionally INSTALLER_CERT, INSTALLER_CERT_PWD (for productbuild).

set -euo pipefail
: "${RUNNER_TEMP:?}"
KEYCHAIN_PATH="$RUNNER_TEMP/app-signing.keychain-db"
APP_P12="$RUNNER_TEMP/application.p12"
INSTALLER_P12="$RUNNER_TEMP/installer.p12"

case "${1:-}" in
  delete)
    security delete-keychain "$KEYCHAIN_PATH" || true
    rm -f "$APP_P12" "$INSTALLER_P12" "$RUNNER_TEMP/asc_api_key.p8" \
          "$RUNNER_TEMP/notary-result.json"
    exit 0
    ;;
  create) ;;
  *) echo "usage: keychain.sh create|delete" >&2; exit 2 ;;
esac

: "${APP_CERT:?}" "${APP_CERT_PWD:?}"

# Random per run: a hardcoded keychain password is a credential checked into
# the repository. It never leaves this script: the keychain stays unlocked and
# in the search list, so no later step needs it.
KEYCHAIN_PWD="$(openssl rand -hex 24)"

umask 077
security create-keychain -p "$KEYCHAIN_PWD" "$KEYCHAIN_PATH"
security set-keychain-settings -lut 3600 "$KEYCHAIN_PATH"
security unlock-keychain -p "$KEYCHAIN_PWD" "$KEYCHAIN_PATH"

# Each certificate is granted only the tool that uses it, and its file is gone
# as soon as the key is in the keychain.
printf '%s' "$APP_CERT" | base64 --decode -o "$APP_P12"
security import "$APP_P12" -k "$KEYCHAIN_PATH" \
  -P "$APP_CERT_PWD" -t cert -f pkcs12 -T /usr/bin/codesign
rm -f "$APP_P12"
if [ -n "${INSTALLER_CERT:-}" ]; then
  printf '%s' "$INSTALLER_CERT" | base64 --decode -o "$INSTALLER_P12"
  security import "$INSTALLER_P12" -k "$KEYCHAIN_PATH" \
    -P "${INSTALLER_CERT_PWD:?}" -t cert -f pkcs12 -T /usr/bin/productbuild
  rm -f "$INSTALLER_P12"
fi

# Without this codesign and productbuild block on a UI prompt no runner can
# answer. productbuild needs no partition of its own: it is an Apple-signed
# tool, so apple-tool:/apple: already cover it.
security set-key-partition-list -S apple-tool:,apple:,codesign: \
  -s -k "$KEYCHAIN_PWD" "$KEYCHAIN_PATH" > /dev/null

# Prepend rather than replace: the Apple intermediate CAs live in the system
# keychain and codesign must still find them. `security list-keychains` prints
# one indented, double-quoted path per line; strip only the indent and the two
# surrounding quotes, and collect into the positional parameters (macOS bash
# is 3.2, no mapfile).
set --
while IFS= read -r line; do
  kc=$(printf '%s\n' "$line" | sed -e 's/^[[:space:]]*"//' -e 's/"[[:space:]]*$//')
  if [ -n "$kc" ]; then
    set -- "$@" "$kc"
  fi
done < <(security list-keychains -d user)
security list-keychains -d user -s "$KEYCHAIN_PATH" "$@"
