#!/usr/bin/env bash
#
# Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
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
# ---------------------------------------------------------------------------
# Regenerate the email fixtures used by the screenshot mode, from the demo
# keyring already committed in resource/screenshot/demo-gnupghome.
#
# Separate from gen_demo_keyring.sh on purpose: that script makes new keys,
# which changes every fingerprint and invalidates every manual screenshot.
# This one only signs with the existing keys, so it can be run on its own.
# The generated files are committed; the pipeline never runs this script.
#
# Every timestamp is pinned to 2025-02-01 12:00 UTC, before the default clock
# (2026-01-15), the same day as the text fixtures:
#
#   signed-contract.eml   PGP/MIME (RFC 3156) message from Alice to Bob,
#                         signed by Alice. Alice holds the only secret key in
#                         the built-in database, so she is the one who signs.
#   draft-contract.eml    the same message, unsigned: a draft in the editor
#   newsletter.eml        an HTML-only message, for the HTML preview notice
#
# Usage:  bash scripts/screenshot/gen_email_fixtures.sh   (needs gpg)
# ---------------------------------------------------------------------------

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
OUT_DIR="$REPO_ROOT/resource/screenshot/fixtures"

# Short on purpose: see gen_demo_keyring.sh.
WORK="$(mktemp -d /tmp/gf-mail.XXXXXX)"
H="$WORK/h"
cleanup() {
  gpgconf --homedir "$H" --kill all >/dev/null 2>&1 || true
  rm -rf "$WORK"
}
trap cleanup EXIT
cp -r "$REPO_ROOT/resource/screenshot/demo-gnupghome" "$H"
chmod 700 "$H"

T_FIXTURE=20250201T120000
DATE='Sat, 01 Feb 2025 12:00:00 +0000'
BOUNDARY='gf-demo-signed'

# CRLF throughout: the signature covers the part exactly as it travels.
crlf() { sed 's/$/\r/'; }

headers() {
  printf 'From: Alice Example <alice@example.com>\n'
  printf 'To: Bob Example <bob@example.com>\n'
  printf 'Subject: Signed contract\n'
  printf 'Date: %s\n' "$DATE"
  printf 'Message-ID: <%s@example.com>\n' "$1"
  printf 'MIME-Version: 1.0\n'
}

body() {
  printf 'Hello Bob,\n\nplease find the signed contract terms below.\n\n'
  printf 'Term: 12 months, starting 1 March 2025.\n\nAlice\n'
}

# The signed entity: the first part, up to but not including the CRLF that
# comes before the next boundary (RFC 3156, section 5).
{
  printf 'Content-Type: text/plain; charset=utf-8\n'
  printf 'Content-Transfer-Encoding: 7bit\n\n'
  body
} | crlf >"$WORK/part"

gpg --homedir "$H" --batch --yes --quiet --pinentry-mode loopback \
  --passphrase '' --faked-system-time "${T_FIXTURE}!" \
  --local-user alice@example.com --digest-algo SHA256 \
  --armor --detach-sign --output "$WORK/part.asc" "$WORK/part"

{
  headers signed-contract-20250201 | crlf
  printf 'Content-Type: multipart/signed; micalg=pgp-sha256;\r\n'
  printf ' protocol="application/pgp-signature"; boundary="%s"\r\n\r\n' \
    "$BOUNDARY"
  printf -- '--%s\r\n' "$BOUNDARY"
  cat "$WORK/part"
  printf '\r\n--%s\r\n' "$BOUNDARY"
  printf 'Content-Type: application/pgp-signature; name="signature.asc"\r\n'
  printf 'Content-Description: OpenPGP digital signature\r\n\r\n'
  crlf <"$WORK/part.asc"
  printf '\r\n--%s--\r\n' "$BOUNDARY"
} >"$OUT_DIR/signed-contract.eml"

{
  headers draft-contract-20250201
  printf 'Content-Type: text/plain; charset=utf-8\n'
  printf 'Content-Transfer-Encoding: 7bit\n\n'
  body
} | crlf >"$OUT_DIR/draft-contract.eml"

{
  printf 'From: Example News <news@example.org>\n'
  printf 'To: Bob Example <bob@example.com>\n'
  printf 'Subject: Example News for February\n'
  printf 'Date: %s\n' "$DATE"
  printf 'Message-ID: <news-20250201@example.org>\n'
  printf 'MIME-Version: 1.0\n'
  printf 'Content-Type: text/html; charset=utf-8\n'
  printf 'Content-Transfer-Encoding: 7bit\n\n'
  printf '<html><body>\n<h1>Example News</h1>\n'
  printf '<p>Welcome to the February issue.</p>\n'
  printf '<p><a href="https://example.org/news">Read it online</a></p>\n'
  printf '</body></html>\n'
} | crlf >"$OUT_DIR/newsletter.eml"

echo "email fixtures written to $OUT_DIR"
