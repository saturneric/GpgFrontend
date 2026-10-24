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
# Regenerate the demo key databases and fixtures used by the screenshot mode.
#
# The generated files are *committed*; the screenshot pipeline never runs this
# script. Key material is random, so re-running it changes every fingerprint
# and invalidates every manual screenshot. Run it only to change the demo data
# on purpose, then commit resource/screenshot/ as a whole.
#
# Every timestamp is pinned (gpg --faked-system-time, sq --time), at 12:00 UTC
# so a date-only display cannot shift a day in any timezone:
#
#   2025-01-01  all keys created
#   2025-01-01  13:00 UTC: Alice adds her work user ID
#   2025-01-02  Alice certifies Bob and Erin; Bob certifies Alice;
#               Grace certifies Heidi
#   2025-02-01  decrypt-verify fixture: encrypted to Alice, signed by Bob
#   2025-03-01  clock probe fixture: signed by Dave
#   2025-04-01  Carol's key revoked
#   2025-06-01  Dave's key expires
#   2026-02-01  Erin's key expires: "expiring soon" at the default clock
#               (2026-01-15), inside the 30-day window
#
# Built-in database (GnuPG, resource/screenshot/demo-gnupghome), as the main
# window lists it:
#   Alice  own key pair, RSA 3072 certify-only primary + sign and encrypt
#          subkeys, no expiry; a second user ID for her work address;
#          certified by Bob
#   Bob    public key, ed25519 + cv25519, certified by Alice
#   Carol  public key, revoked
#   Dave   public key, expired
#   Erin   public key, certified by Alice, expiring soon
#
# Work database (GnuPG, resource/screenshot/demo-dbs/work):
#   Grace  own key pair, ed25519 + cv25519, no expiry
#   Heidi  public key, RSA 3072, certified by Grace
#
# Lab database (rPGP, resource/screenshot/demo-dbs/lab): armored RFC 9580
# (OpenPGP v6) keys made with sq, imported by the screenshot mode at run time
# so the committed data never depends on the engine's own storage format:
#   ivan.key   Ivan, own key pair, no expiry
#   judy.cert  Judy, public key, no expiry
#
# No key carries a passphrase; this is demo data, not a secret.
#
# Usage:  bash scripts/screenshot/gen_demo_keyring.sh   (needs gpg and sq)
# ---------------------------------------------------------------------------

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
OUT_DIR="$REPO_ROOT/resource/screenshot"

# Short on purpose: the gpg-agent socket lives in the homedir when
# /run/user is unavailable, and a long path exceeds sun_path.
WORK="$(mktemp -d /tmp/gf-demo.XXXXXX)"
BUILTIN_HOME="$WORK/h"
WORK_HOME="$WORK/w"
mkdir -m 700 "$BUILTIN_HOME" "$WORK_HOME"
cleanup() {
  for home in "$BUILTIN_HOME" "$WORK_HOME"; do
    gpgconf --homedir "$home" --kill all >/dev/null 2>&1 || true
  done
  rm -rf "$WORK"
}
trap cleanup EXIT

# at <homedir> <time> <gpg args...>
at() {
  local home="$1" when="$2"
  shift 2
  gpg --homedir "$home" --batch --yes --quiet --pinentry-mode loopback \
    --passphrase '' --faked-system-time "${when}!" "$@"
}

fpr_of() {
  gpg --homedir "$1" --batch --with-colons --list-keys "$2" |
    awk -F: '$1=="fpr"{print $10; exit}'
}

T_CREATE=20250101T120000
T_ADD_UID=20250101T130000
T_CERTIFY=20250102T120000
T_FIXTURE=20250201T120000
T_PROBE=20250301T120000
T_REVOKE=20250401T120000
# Expiry in seconds from creation, so it lands exactly at 12:00 UTC.
DAVE_EXPIRE_SECONDS=$((151 * 86400))  # 2025-06-01
ERIN_EXPIRE_SECONDS=$((396 * 86400))  # 2026-02-01

# --- Built-in database ------------------------------------------------------
H="$BUILTIN_HOME"
at "$H" "$T_CREATE" --quick-gen-key 'Alice Example <alice@example.com>' rsa3072 cert never
ALICE="$(fpr_of "$H" alice@example.com)"
at "$H" "$T_CREATE" --quick-add-key "$ALICE" rsa3072 sign never
at "$H" "$T_CREATE" --quick-add-key "$ALICE" rsa3072 encr never
# An hour after creation, so the key details show the work address as the
# later of the two user IDs.
at "$H" "$T_ADD_UID" --quick-add-uid "$ALICE" 'Alice Example <alice@work.example.com>'
at "$H" "$T_CERTIFY" --quick-set-primary-uid "$ALICE" 'Alice Example <alice@example.com>'

at "$H" "$T_CREATE" --quick-gen-key 'Bob Example <bob@example.com>' ed25519 cert,sign never
BOB="$(fpr_of "$H" bob@example.com)"
at "$H" "$T_CREATE" --quick-add-key "$BOB" cv25519 encr never

at "$H" "$T_CREATE" --quick-gen-key 'Carol Example <carol@example.com>' ed25519 default never
CAROL="$(fpr_of "$H" carol@example.com)"

at "$H" "$T_CREATE" --quick-gen-key 'Dave Example <dave@example.com>' ed25519 default \
  "seconds=$DAVE_EXPIRE_SECONDS"
DAVE="$(fpr_of "$H" dave@example.com)"

at "$H" "$T_CREATE" --quick-gen-key 'Erin Example <erin@example.com>' ed25519 default \
  "seconds=$ERIN_EXPIRE_SECONDS"
ERIN="$(fpr_of "$H" erin@example.com)"

at "$H" "$T_CERTIFY" --local-user "$ALICE" --quick-sign-key "$BOB"
at "$H" "$T_CERTIFY" --local-user "$ALICE" --quick-sign-key "$ERIN"
at "$H" "$T_CERTIFY" --local-user "$BOB" --quick-sign-key "$ALICE" \
  'Alice Example <alice@example.com>'

mkdir -p "$OUT_DIR/fixtures"
printf 'Hello Alice,\n\nthe quarterly report is attached as agreed.\n\nBob\n' |
  at "$H" "$T_FIXTURE" --armor --local-user "$BOB" --recipient "$ALICE" \
    --sign --encrypt >"$OUT_DIR/fixtures/decrypt-verify.asc"
printf 'Clock probe message signed by Dave.\n' |
  at "$H" "$T_PROBE" --local-user "$DAVE" --clearsign \
    >"$OUT_DIR/fixtures/clock-probe-dave.asc"

# The stored certificate is prefixed with ':' to stop accidental imports.
sed 's/^:-----/-----/' "$H/openpgp-revocs.d/$CAROL.rev" |
  at "$H" "$T_REVOKE" --import

# Bob, Carol, Dave and Erin are contacts: keep only their public keys.
# Ownertrust drops back to unknown, so validity comes from certifications.
for fpr in "$BOB" "$CAROL" "$DAVE" "$ERIN"; do
  at "$H" "$T_REVOKE" --delete-secret-keys "$fpr"
done
printf '%s:6:\n' "$ALICE" | gpg --homedir "$H" --batch --import-ownertrust
for fpr in "$BOB" "$CAROL" "$DAVE" "$ERIN"; do
  printf '%s:2:\n' "$fpr" | gpg --homedir "$H" --batch --import-ownertrust
done
at "$H" "$T_REVOKE" --check-trustdb

# --- Work database ----------------------------------------------------------
W="$WORK_HOME"
at "$W" "$T_CREATE" --quick-gen-key 'Grace Example <grace@example.org>' ed25519 cert,sign never
GRACE="$(fpr_of "$W" grace@example.org)"
at "$W" "$T_CREATE" --quick-add-key "$GRACE" cv25519 encr never
at "$W" "$T_CREATE" --quick-gen-key 'Heidi Example <heidi@example.org>' rsa3072 default never
HEIDI="$(fpr_of "$W" heidi@example.org)"
at "$W" "$T_CERTIFY" --local-user "$GRACE" --quick-sign-key "$HEIDI"
at "$W" "$T_REVOKE" --delete-secret-keys "$HEIDI"
printf '%s:6:\n' "$GRACE" | gpg --homedir "$W" --batch --import-ownertrust
printf '%s:2:\n' "$HEIDI" | gpg --homedir "$W" --batch --import-ownertrust
at "$W" "$T_REVOKE" --check-trustdb

for home in "$H" "$W"; do gpgconf --homedir "$home" --kill all; done

copy_home() {
  local from="$1" to="$2"
  rm -rf "$to"
  mkdir -p "$to"
  cp "$from/pubring.kbx" "$from/trustdb.gpg" "$to/"
  cp -r "$from/private-keys-v1.d" "$to/"
}
copy_home "$H" "$OUT_DIR/demo-gnupghome"
copy_home "$W" "$OUT_DIR/demo-dbs/work"

# --- Lab database (rPGP, OpenPGP v6) ----------------------------------------
LAB="$OUT_DIR/demo-dbs/lab"
rm -rf "$LAB"
mkdir -p "$LAB"
sq_gen() {
  sq --home none --time 2025-01-01T12:00:00Z key generate --profile rfc9580 \
    --without-password --own-key --expiration never --userid "$1" \
    --output "$2" --rev-cert "$WORK/$(basename "$2").rev" >/dev/null 2>&1
}
sq_gen 'Ivan Example <ivan@example.net>' "$LAB/ivan.key"
sq_gen 'Judy Example <judy@example.net>' "$WORK/judy.key"
sq --home none key delete --cert-file "$WORK/judy.key" --output "$LAB/judy.cert" \
  >/dev/null 2>&1
sq_fpr() {
  sq --home none inspect "$1" | awk '/Fingerprint:/{print $2; exit}'
}
IVAN="$(sq_fpr "$LAB/ivan.key")"
JUDY="$(sq_fpr "$LAB/judy.cert")"

# --- demo-keys.json -----------------------------------------------------------
# What the screenshot mode's clock probe checks against, and the earliest
# --clock it accepts (newest_timestamp).
key_json() {
  local home="$1" name="$2" fpr="$3"
  gpg --homedir "$home" --batch --with-colons --fixed-list-mode --list-keys "$fpr" |
    awk -F: -v name="$name" -v fpr="$fpr" '
      $1=="pub" {
        printf "{\"name\": \"%s\", \"fingerprint\": \"%s\", ", name, fpr
        printf "\"created\": %s, \"expires\": %s, \"revoked\": %s}", \
          $6, ($7=="" ? "null" : $7), ($2=="r" ? "true" : "false")
        exit
      }'
}
epoch() { date -u -d "$1" +%s; }
CREATED="$(epoch 2025-01-01T12:00:00Z)"
{
  printf '{\n  "schema": 1,\n'
  printf '  "keys": [\n'
  printf '    %s,\n' "$(key_json "$H" Alice "$ALICE")"
  printf '    %s,\n' "$(key_json "$H" Bob "$BOB")"
  printf '    %s,\n' "$(key_json "$H" Carol "$CAROL")"
  printf '    %s,\n' "$(key_json "$H" Dave "$DAVE")"
  printf '    %s\n' "$(key_json "$H" Erin "$ERIN")"
  printf '  ],\n'
  printf '  "databases": {\n'
  printf '    "work": {"name": "Work", "backend": "gnupg", "keys": [\n'
  printf '      %s,\n' "$(key_json "$W" Grace "$GRACE")"
  printf '      %s\n' "$(key_json "$W" Heidi "$HEIDI")"
  printf '    ]},\n'
  printf '    "lab": {"name": "Lab", "backend": "rpgp", "import": ["ivan.key", "judy.cert"], "keys": [\n'
  printf '      {"name": "Ivan", "fingerprint": "%s", "created": %s, "expires": null, "revoked": false},\n' "$IVAN" "$CREATED"
  printf '      {"name": "Judy", "fingerprint": "%s", "created": %s, "expires": null, "revoked": false}\n' "$JUDY" "$CREATED"
  printf '    ]}\n'
  printf '  },\n'
  printf '  "fixtures": {\n'
  printf '    "decrypt-verify.asc": {"signer": "%s", "recipient": "%s", "signed": %s},\n' \
    "$BOB" "$ALICE" "$(epoch 2025-02-01T12:00:00Z)"
  printf '    "clock-probe-dave.asc": {"signer": "%s", "signed": %s}\n' \
    "$DAVE" "$(epoch 2025-03-01T12:00:00Z)"
  printf '  },\n'
  printf '  "newest_timestamp": %s\n}\n' "$(epoch 2025-04-01T12:00:00Z)"
} >"$OUT_DIR/demo-keys.json"

echo "demo key databases written to $OUT_DIR"
