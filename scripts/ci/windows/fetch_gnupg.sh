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
# Download the GnuPG binary release the Windows payload ships, verify it
# against a pinned signing key and its checksum, and extract it byte-exact
# into build/artifacts/gnupg.
#
# Environment: GNUPG_VERSION.

set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
: "${GNUPG_VERSION:?}"
cd "$ROOT"

URL="https://ftp.bktus.com/GnuPG/${GNUPG_VERSION}"
KEY_URL="https://bktus.com/pgp/saturneric-code-signing.asc"
KEY_FPR="12F7E8858CF15BEC9975FF3C5CA3DA246843FD03"
KEY_FILE="saturneric-code-signing.asc"
FILE="gnupg.zip"
CHECKSUM_FILE="SHA256SUMS.txt"
SIGNATURE_FILE="gnupg.zip.sig"
GNUPGHOME="$(mktemp -d)"
export GNUPGHOME

DL=build/downloads
mkdir -p "$DL"
curl -fL --retry 3 -o "$DL/$FILE" "$URL/$FILE"
curl -fL --retry 3 -o "$DL/$CHECKSUM_FILE" "$URL/$CHECKSUM_FILE"
curl -fL --retry 3 -o "$DL/$KEY_FILE" "$KEY_URL"
curl -fL --retry 3 -o "$DL/$SIGNATURE_FILE" "$URL/$SIGNATURE_FILE"

gpg --import "$DL/$KEY_FILE"

# Trust is pinned to this exact fingerprint, not to whatever the downloaded key
# file happens to contain.
if ! KEY_INFO=$(gpg --batch --with-colons --list-keys "$KEY_FPR"); then
  echo "Imported key does not match fingerprint $KEY_FPR!" >&2
  exit 1
fi

EXPIRES=$(echo "$KEY_INFO" | awk -F: '/^pub:/ {print $7; exit}')
if [ -n "$EXPIRES" ]; then
  echo "Signing key expires: $(date -u -d "@$EXPIRES")"
  if [ "$EXPIRES" -le "$(date +%s)" ]; then
    echo "Signing key has expired!" >&2
    exit 1
  fi
else
  echo "Signing key has no expiration date"
fi

# VALIDSIG carries the primary key fingerprint as its last field, so this also
# rejects a valid signature from any other imported key.
if ! gpg --status-fd 1 --verify "$DL/$SIGNATURE_FILE" "$DL/$FILE" \
    | grep "VALIDSIG" | grep -q "$KEY_FPR"; then
  echo "GnuPG signature verification failed!" >&2
  exit 1
fi

CHECKSUM=$(grep "$FILE\$" "$DL/$CHECKSUM_FILE" | awk '{print $1}')
ACTUAL_CHECKSUM=$(sha256sum "$DL/$FILE" | awk '{print $1}')
echo "Expected Checksum: $CHECKSUM"
echo "Actual Checksum: $ACTUAL_CHECKSUM"
if [ "$CHECKSUM" != "$ACTUAL_CHECKSUM" ]; then
  echo "Checksum verification failed!" >&2
  exit 1
fi

mkdir -p build/artifacts/gnupg

# Extraction has to be byte-exact. A text-mode extractor rewrites every LF as
# CRLF, which shifts each PE image away from the offset its e_lfanew field
# points at: the staged binaries then carry no readable Authenticode signature
# and Windows refuses to load them, while the archive-level OpenPGP and SHA256
# checks above still pass because the downloaded zip is intact. bsdtar has no
# text mode at all.
if command -v bsdtar >/dev/null 2>&1; then
  bsdtar -xf "$DL/$FILE" -C build/artifacts/gnupg
else
  unzip -o "$DL/$FILE" -d build/artifacts/gnupg/
fi

# Gate the staged payload rather than trusting the extractor: every image must
# still start with MZ and hold the PE signature exactly where e_lfanew points.
BROKEN=0
COUNT=0
while IFS= read -r pe; do
  COUNT=$((COUNT + 1))
  MZ=$(dd if="$pe" bs=1 count=2 2>/dev/null | od -An -tx1 | tr -d ' \n')
  OFF=$(od -An -tu4 -j 60 -N 4 "$pe" | tr -d ' ')
  SIG=$(dd if="$pe" bs=1 skip="$OFF" count=4 2>/dev/null | od -An -tx1 | tr -d ' \n')
  if [ "$MZ" != "4d5a" ] || [ "$SIG" != "50450000" ]; then
    echo "corrupt PE image: $pe (mz=$MZ e_lfanew=$OFF sig=$SIG)" >&2
    BROKEN=$((BROKEN + 1))
  fi
done < <(find build/artifacts/gnupg -type f \( -name '*.exe' -o -name '*.dll' \))

if [ "$BROKEN" -ne 0 ]; then
  echo "$BROKEN of $COUNT staged GnuPG images are not loadable PE files!" >&2
  type -a bsdtar unzip >&2 || true
  env | grep -iE '^(UNZIP|UNZIPOPT|ZIPOPT|MSYS|CYGWIN)=' >&2 || true
  exit 1
fi
echo "verified $COUNT staged GnuPG PE images"

ls -l build/artifacts/gnupg/
