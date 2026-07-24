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
# Regenerate the RFC 9580 known-answer test vector corpus for the rPGP engine.
#
# The rPGP engine cannot *emit* the non-compliant artifacts the test-suite must
# prove it *rejects* (a legacy Symmetrically Encrypted Data packet, a SHA-1
# signature, a foreign AEAD container, an expired/revoked-key signature). This
# script produces those known-answer vectors offline using:
#   * sq  (Sequoia PGP) -- RFC9580/v6 keys, v2-SEIPD/AEAD-OCB, cleartext/inline/
#                          detached signing, symmetric encryption.
#   * gpg (GnuPG 2.4)   -- SHA-1 signatures and the legacy Tag-9 SED container.
#
# The generated corpus is *committed* to the repository; tests never invoke
# sq/gpg at run time. Re-run this script only to refresh or extend the corpus,
# then commit the results. Every vector's expected structure is documented as a
# comment beside the command that produces it.
#
# The corpus is encrypted to / signed by:
#   * the canonical fixture keys resource/lfs/test/rpgp/key{1,2,3}.asc (v4,
#     passphrase "123456"); the RpgpCoreTest keyring already holds their
#     secrets, so it can decrypt the "encrypt-to-fixture" vectors, and
#   * a small set of purpose-built auxiliary "adversarial" keys written to
#     resource/lfs/test/rpgp_aux_keys/; the RpgpRfc9580Test fixture imports
#     these so signature verification can resolve the adversarial signers.
#
# Usage:  bash scripts/gen_rpgp_test_vectors.sh
# ---------------------------------------------------------------------------

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
FIXTURE_DIR="$REPO_ROOT/resource/lfs/test/rpgp"
VEC_DIR="$REPO_ROOT/resource/lfs/test/rpgp_vectors"
AUX_DIR="$REPO_ROOT/resource/lfs/test/rpgp_aux_keys"

PASSPHRASE="123456"

# Fixture fingerprints (see resource/lfs/test/rpgp/key*.asc).
KEY1_FPR="3B20B337A988D2C9917D0F33BDB8BB6BDDFA8497"  # Ed25519 cert/sign + ECDH
KEY2_FPR="3BEDAB48EAAAA195006330414DD9733454846D0C"  # ECDSA + ECDH
KEY3_FPR="C54DF5E9E6AD3278C77F5438DA6A97C428EC96C8"  # Ed25519 + ECDH

command -v sq  >/dev/null || { echo "error: sq not found"  >&2; exit 1; }
command -v gpg >/dev/null || { echo "error: gpg not found" >&2; exit 1; }

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
export SQ_HOME="$WORK/sqhome"
export GNUPGHOME="$WORK/gnupg"; mkdir -p "$GNUPGHOME"; chmod 700 "$GNUPGHOME"

# Operate on files only: never touch the shared cert/key stores.
SQ() { sq --batch --cert-store=none --key-store=none "$@"; }

PW="$WORK/pw.txt"; printf '%s' "$PASSPHRASE" > "$PW"

rm -rf "$VEC_DIR" "$AUX_DIR"
mkdir -p "$VEC_DIR" "$AUX_DIR"

# The single fixed plaintext every signature covers / every ciphertext hides.
PAYLOAD="$VEC_DIR/payload.txt"
printf 'GpgFrontend rPGP engine RFC 9580 known-answer test vector.\n' > "$PAYLOAD"

echo ">> extracting fixture public certificates"
# key1 is a transferable *secret* key; key2/key3 are already public certs.
# Strip secret key material where present so we have a public cert to encrypt to.
extract_cert() {  # <infile> <outfile>
  if head -1 "$1" | grep -q 'PRIVATE KEY'; then
    SQ key delete --cert-file="$1" --output="$2" --overwrite
  else
    cp "$1" "$2"
  fi
}
for n in 1 2 3; do
  extract_cert "$FIXTURE_DIR/key${n}.asc" "$WORK/key${n}.cert"
done

# ===========================================================================
# Auxiliary "adversarial" keys (committed to resource/lfs/test/rpgp_aux_keys/)
# ===========================================================================

echo ">> generating auxiliary keys"

# aux_v6: RFC 9580 (v6) key, passphrase-protected with the fixture passphrase.
#   * Its SECRET key is committed so the fixture can decrypt the v2-SEIPD/AEAD
#     vector encrypted to it.
#   * Also used as a v6 signer (native Ed25519, v6 signature packets).
SQ key generate --profile rfc9580 --name 'Aux V6' --email 'aux-v6@test.example' \
  --own-key --new-password-file "$PW" \
  --output "$AUX_DIR/aux_v6.asc" --rev-cert "$WORK/aux_v6.rev"
SQ --keyring "$AUX_DIR/aux_v6.asc" cert export --all > "$WORK/aux_v6.cert"
AUX_V6_FPR="$(SQ inspect "$AUX_DIR/aux_v6.asc" | awk '/Fingerprint:/{print $2; exit}')"

# aux_good: plain v4 signer, alive, no passphrase. Only its PUBLIC cert is
# committed; the secret stays local to this run for producing good signatures.
SQ key generate --profile rfc4880 --name 'Aux Good' --email 'aux-good@test.example' \
  --own-key --without-password \
  --output "$WORK/aux_good.key" --rev-cert "$WORK/aux_good.rev"
SQ key delete --cert-file="$WORK/aux_good.key" --output="$AUX_DIR/aux_good.asc"
AUX_GOOD_FPR="$(SQ inspect "$AUX_DIR/aux_good.asc" | awk '/Fingerprint:/{print $2; exit}')"

# aux_expired: v4 key created 2020-01-01, expired 2020-06-01. A detached
# signature is made 2020-03-01 (within validity) so the signature itself is
# cryptographically sound but the signing key is expired *now*.
SQ --time 2020-01-01T00:00:00Z key generate --profile rfc4880 \
  --name 'Aux Expired' --email 'aux-expired@test.example' \
  --own-key --without-password --expiration 2020-06-01T00:00:00Z \
  --output "$WORK/aux_expired.key" --rev-cert "$WORK/aux_expired.rev"
SQ key delete --cert-file="$WORK/aux_expired.key" --output="$AUX_DIR/aux_expired.asc"
AUX_EXP_FPR="$(SQ inspect "$AUX_DIR/aux_expired.asc" | awk '/Fingerprint:/{print $2; exit}')"

# aux_revoked: v4 key whose committed cert carries a primary-key revocation.
# A signature is produced *before* merging the revocation certificate.
SQ key generate --profile rfc4880 --name 'Aux Revoked' --email 'aux-revoked@test.example' \
  --own-key --without-password \
  --output "$WORK/aux_revoked.key" --rev-cert "$WORK/aux_revoked.rev"
SQ key delete --cert-file="$WORK/aux_revoked.key" --output="$WORK/aux_revoked_clean.cert"
SQ keyring merge "$WORK/aux_revoked_clean.cert" "$WORK/aux_revoked.rev" \
  --output "$AUX_DIR/aux_revoked.asc"
AUX_REV_FPR="$(SQ inspect "$AUX_DIR/aux_revoked.asc" | awk '/Fingerprint:/{print $2; exit}')"

# aux_sha1: gpg Ed25519 key used to produce SHA-1 signatures (sq refuses the
# weak digest, gpg does not). Only its public cert is committed.
gpg --batch --pinentry-mode loopback --passphrase '' \
  --quick-gen-key 'Aux SHA1 <aux-sha1@test.example>' ed25519 sign never >/dev/null 2>&1
AUX_SHA1_FPR="$(gpg --list-keys --with-colons aux-sha1@test.example \
  | awk -F: '/^fpr/{print $10; exit}')"
gpg --export --armor aux-sha1@test.example > "$AUX_DIR/aux_sha1.asc"

# ===========================================================================
# Encryption containers (decrypted by the fixture / aux keyring)
# ===========================================================================

echo ">> generating encryption vectors"

# enc_v1seipd_mdc.pgp: v3 PKESK + v1 SEIPD (MDC). key1 advertises no AEAD
# feature, so sq falls back to the legacy integrity-protected container. RFC
# 9580 sec 5.13.1. Decrypts to payload with MessageIntegrityProtected()==true.
SQ encrypt --without-signature --for-file "$WORK/key1.cert" \
  --output "$VEC_DIR/enc_v1seipd_mdc.pgp" --binary "$PAYLOAD"

# enc_v2seipd_ocb.pgp: v6 PKESK + v2 SEIPD (AEAD, OCB). Encrypted to the aux_v6
# key which advertises SEIPDv2. RFC 9580 sec 5.13.2. Exercises AEAD decrypt.
SQ encrypt --without-signature --for-file "$WORK/aux_v6.cert" \
  --output "$VEC_DIR/enc_v2seipd_ocb.pgp" --binary "$PAYLOAD"

# enc_multi_recipient.pgp: three PKESK packets (key1+key2+key3). Any one of the
# fixture secrets decrypts it.
SQ encrypt --without-signature \
  --for-file "$WORK/key1.cert" --for-file "$WORK/key2.cert" --for-file "$WORK/key3.cert" \
  --output "$VEC_DIR/enc_multi_recipient.pgp" --binary "$PAYLOAD"

# enc_symmetric_v1.pgp: password-based (SKESK) + v1 SEIPD/MDC (gpg default).
printf '%s' "$PASSPHRASE" | gpg --batch --yes --pinentry-mode loopback \
  --passphrase-fd 0 --cipher-algo AES256 --symmetric \
  --output "$VEC_DIR/enc_symmetric_v1.pgp" "$PAYLOAD" 2>/dev/null

# enc_symmetric_v2.pgp: password-based + v2 SEIPD/AEAD (sq rfc9580 profile).
SQ encrypt --profile rfc9580 --without-signature --with-password-file "$PW" \
  --output "$VEC_DIR/enc_symmetric_v2.pgp" --binary "$PAYLOAD"

# enc_sed_tag9.pgp: LEGACY Symmetrically Encrypted Data (Tag 9), NO integrity
# protection. RFC 9580 sec 13.7 -- the engine MUST refuse to decrypt this
# (finding H2). Produced with gpg --rfc2440 --disable-mdc.
printf '%s' "$PASSPHRASE" | gpg --batch --yes --pinentry-mode loopback \
  --passphrase-fd 0 --s2k-mode 0 --disable-mdc --rfc2440 \
  --cipher-algo AES --symmetric \
  --output "$VEC_DIR/enc_sed_tag9.pgp" "$PAYLOAD" 2>/dev/null

# ===========================================================================
# Signature vectors (verified against the aux keyring)
# ===========================================================================

echo ">> generating signature vectors"

# sig_good_detached.sig: alive v4 signer, SHA-512. Verifies as Valid.
SQ sign --signature-file="$VEC_DIR/sig_good_detached.sig" \
  --signer-file="$WORK/aux_good.key" "$PAYLOAD"

# sig_good_cleartext.asc: cleartext-signed message, alive v4 signer.
SQ sign --cleartext --signer-file="$WORK/aux_good.key" \
  --output "$VEC_DIR/sig_good_cleartext.asc" "$PAYLOAD"

# sig_good_inline_v6.pgp: inline (one-pass) signed message from the v6 key.
SQ sign --message --signer-file="$AUX_DIR/aux_v6.asc" --password-file="$PW" \
  --output "$VEC_DIR/sig_good_inline_v6.pgp" "$PAYLOAD"

# sig_v6_detached.sig: detached v6 signature (native Ed25519, v6 sig packet).
SQ sign --signature-file="$VEC_DIR/sig_v6_detached.sig" \
  --signer-file="$AUX_DIR/aux_v6.asc" --password-file="$PW" "$PAYLOAD"

# sig_sha1_detached.sig: SHA-1 detached signature. RFC 9580 sec 9.5 lists SHA-1
# as legacy/weak -- the engine must NOT report this as Valid (weak-hash gate).
gpg --batch --yes --digest-algo SHA1 --detach-sign --armor \
  -u aux-sha1@test.example --output "$VEC_DIR/sig_sha1_detached.sig" "$PAYLOAD" 2>/dev/null

# sig_expired.sig: detached signature from a now-expired key. Must not be Valid
# (finding M3, signature/key expiration enforcement).
SQ --time 2020-03-01T00:00:00Z sign \
  --signature-file="$VEC_DIR/sig_expired.sig" \
  --signer-file="$WORK/aux_expired.key" "$PAYLOAD"

# sig_revokedkey.sig: detached signature whose signer's committed cert is
# revoked. Must not be Valid (revoked-primary gate).
SQ sign --signature-file="$VEC_DIR/sig_revokedkey.sig" \
  --signer-file="$WORK/aux_revoked.key" "$PAYLOAD"

# two_signer.pgp: inline message signed by BOTH aux_good and aux_v6. Exercises
# per-index signer attribution (finding B-Verify): each signature must be
# attributed to the exact key that made it.
SQ sign --message \
  --signer-file="$WORK/aux_good.key" \
  --signer-file="$AUX_DIR/aux_v6.asc" --password-file="$PW" \
  --output "$VEC_DIR/two_signer.pgp" "$PAYLOAD"

# sig_bad_mutated.sig: a good detached signature with one byte of the signature
# body flipped -> cryptographically invalid. Must not be Valid.
cp "$VEC_DIR/sig_good_detached.sig" "$WORK/mut.sig"
python3 - "$WORK/mut.sig" "$VEC_DIR/sig_bad_mutated.sig" <<'PY'
import sys
src, dst = sys.argv[1], sys.argv[2]
data = bytearray(open(src, 'rb').read())
# Flip a byte near the middle of the armored body (well past the header).
i = len(data) // 2
data[i] ^= 0x01
open(dst, 'wb').write(data)
PY

# ===========================================================================
# Malformed / edge inputs (must fail cleanly -- never crash)
# ===========================================================================

echo ">> generating malformed inputs"

# garbage.bin: not OpenPGP data at all.
printf 'this is definitely not an OpenPGP message at all\n' > "$VEC_DIR/garbage.bin"

# empty.bin: zero bytes.
: > "$VEC_DIR/empty.bin"

# An ASCII-armored ciphertext used as the basis for the truncation/CRC cases.
SQ encrypt --without-signature --for-file "$WORK/key1.cert" \
  --output "$WORK/enc_armored.asc" "$PAYLOAD"

# truncated_armor.asc: a valid armored ciphertext with its body/tail chopped off.
head -n 4 "$WORK/enc_armored.asc" > "$VEC_DIR/truncated_armor.asc"

# corrupt_crc.asc: a valid armored ciphertext with its CRC-24 footer corrupted.
cp "$WORK/enc_armored.asc" "$WORK/crc.asc"
python3 - "$WORK/crc.asc" "$VEC_DIR/corrupt_crc.asc" <<'PY'
import sys
src, dst = sys.argv[1], sys.argv[2]
lines = open(src).read().splitlines()
# The CRC-24 checksum is the line beginning with '=' just before the footer.
for i, ln in enumerate(lines):
    if ln.startswith('=') and len(ln) == 5:
        body = list(lines[i])
        body[1] = 'A' if body[1] != 'A' else 'B'
        lines[i] = ''.join(body)
        break
open(dst, 'w').write('\n'.join(lines) + '\n')
PY

# pkesk_no_seipd.pgp: a PKESK packet with no following encrypted-data packet.
SQ --force packet dump "$VEC_DIR/enc_v1seipd_mdc.pgp" >/dev/null 2>&1 || true
python3 - "$VEC_DIR/enc_v1seipd_mdc.pgp" "$VEC_DIR/pkesk_no_seipd.pgp" <<'PY'
import sys
src, dst = sys.argv[1], sys.argv[2]
data = open(src, 'rb').read()
# Keep only the first ~2/3 (drops the trailing SEIPD packet body), leaving a
# lone/short session-key packet with no complete encrypted-data packet.
open(dst, 'wb').write(data[: max(1, (len(data) * 2) // 3)])
PY

# ===========================================================================
# Manifest
# ===========================================================================

cat > "$VEC_DIR/MANIFEST.txt" <<EOF
RFC 9580 rPGP test-vector corpus -- generated by scripts/gen_rpgp_test_vectors.sh
Plaintext (payload.txt): "$(cat "$PAYLOAD")"
Symmetric / secret-key passphrase: $PASSPHRASE

Fixture recipients:
  key1 $KEY1_FPR
  key2 $KEY2_FPR
  key3 $KEY3_FPR

Auxiliary signers (resource/lfs/test/rpgp_aux_keys/):
  aux_v6      $AUX_V6_FPR   (v6, SECRET committed, pass $PASSPHRASE)
  aux_good    $AUX_GOOD_FPR (v4, public cert)
  aux_expired $AUX_EXP_FPR  (v4, public cert, expired 2020-06-01)
  aux_revoked $AUX_REV_FPR  (v4, public cert, primary revoked)
  aux_sha1    $AUX_SHA1_FPR (v4/gpg ed25519, public cert)

Encryption vectors:
  enc_v1seipd_mdc.pgp    v3 PKESK + v1 SEIPD/MDC -> key1
  enc_v2seipd_ocb.pgp    v6 PKESK + v2 SEIPD/AEAD-OCB -> aux_v6
  enc_multi_recipient.pgp  3x PKESK -> key1+key2+key3
  enc_symmetric_v1.pgp   SKESK + v1 SEIPD/MDC (password)
  enc_symmetric_v2.pgp   SKESK + v2 SEIPD/AEAD (password)
  enc_sed_tag9.pgp       LEGACY Tag-9 SED (no integrity) -- MUST be rejected

Signature vectors (over payload.txt):
  sig_good_detached.sig   valid, SHA-512, aux_good
  sig_good_cleartext.asc  valid cleartext, aux_good
  sig_good_inline_v6.pgp  valid inline, aux_v6
  sig_v6_detached.sig     valid detached v6, aux_v6
  sig_sha1_detached.sig   SHA-1 -> MUST NOT be Valid (weak hash)
  sig_expired.sig         expired key -> MUST NOT be Valid
  sig_revokedkey.sig      revoked key -> MUST NOT be Valid
  two_signer.pgp          inline, aux_good + aux_v6 (per-index attribution)
  sig_bad_mutated.sig     one byte flipped -> invalid

Malformed inputs (must fail cleanly, never crash):
  garbage.bin, empty.bin, truncated_armor.asc, corrupt_crc.asc, pkesk_no_seipd.pgp
EOF

echo ">> done. corpus written to:"
echo "   $VEC_DIR"
echo "   $AUX_DIR"
ls -1 "$VEC_DIR" "$AUX_DIR"
