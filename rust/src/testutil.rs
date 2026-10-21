/*
 * Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
 *
 * This file is part of GpgFrontend.
 *
 * GpgFrontend is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GpgFrontend is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GpgFrontend. If not, see <https://www.gnu.org/licenses/>.
 *
 * The initial version of the source code is inherited from
 * the gpg4usb project, which is under GPL-3.0-or-later.
 *
 * All the source code of GpgFrontend was modified and released by
 * Saturneric <eric@bktus.com> starting on May 12, 2021.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

//! Shared scaffolding for the crate's unit tests.
//!
//! This module is `#[cfg(test)]`-only and lives *inside* the crate on purpose:
//! `Cargo.toml` declares `crate-type = ["staticlib"]`, so there is no rlib for
//! an external `tests/` target to link against. Every test in this crate is an
//! inline `#[cfg(test)] mod`, and everything they share lives here.
//!
//! Contents:
//! - [`corpus`] — the committed RFC 9580 known-answer vectors, embedded at
//!   compile time, plus parse-once certificates and *derived* fingerprints.
//! - [`rfc9580`] — the self-contained test vectors from RFC 9580 Appendix A.
//! - [`keys`] — lazily generated key fixtures shared by the whole test binary.
//! - [`assert`] — assertion helpers used across modules.
//!
//! It also provides the two host symbols the crate links against
//! (`gfc_secure_free_cstr` / `gfc_secure_free_buffer`). In a production build
//! those come from the C++ core; in the test binary they must be defined
//! exactly once, and this is that one place.

#![allow(dead_code)]

use std::ffi::{CString, c_char};

// ---------------------------------------------------------------------------
// Host stubs
// ---------------------------------------------------------------------------

/// Test stand-in for the host's secure C-string free.
///
/// Mirrors the production contract documented in [`crate::host`]: the pointer
/// is a genuine NUL-terminated C string whose length is recovered by scanning.
/// Unlike the previous no-op stub this genuinely reclaims, so the callback
/// stubs below (and the FFI tests that drive them) do not leak.
///
/// # Safety contract for callers
/// Every pointer handed to this function must originate from
/// `CString::into_raw`.
#[unsafe(no_mangle)]
extern "C" fn gfc_secure_free_cstr(ptr: *mut c_char) {
    if ptr.is_null() {
        return;
    }
    // SAFETY: by the contract above the pointer came from `CString::into_raw`.
    unsafe { drop(CString::from_raw(ptr)) };
}

/// Test stand-in for the host's secure length-delimited buffer free.
///
/// The length is explicit, so freeing never depends on a NUL terminator.
///
/// # Safety contract for callers
/// Every pointer handed to this function must originate from
/// `Box<[u8]>::into_raw` (or an equivalent allocation whose capacity equals
/// `len`), which is why the callback stubs hand out `into_boxed_slice()`
/// allocations rather than raw `Vec`s.
#[unsafe(no_mangle)]
extern "C" fn gfc_secure_free_buffer(ptr: *mut u8, len: usize) {
    if ptr.is_null() || len == 0 {
        return;
    }
    // SAFETY: by the contract above `ptr` is a `Box<[u8]>` of exactly `len`
    // bytes, so capacity == length and reconstructing the Vec is sound.
    unsafe { drop(Vec::from_raw_parts(ptr, len, len)) };
}

/// Hand a byte slice to C in the shape [`gfc_secure_free_buffer`] expects:
/// a boxed slice, whose capacity is guaranteed equal to its length.
pub fn leak_as_c_buffer(bytes: &[u8]) -> (*mut u8, usize) {
    let boxed = bytes.to_vec().into_boxed_slice();
    let len = boxed.len();
    (Box::into_raw(boxed).cast::<u8>(), len)
}

// ---------------------------------------------------------------------------
// Corpus
// ---------------------------------------------------------------------------

/// The committed known-answer corpus produced by
/// `scripts/gen_rpgp_test_vectors.sh` (see `MANIFEST.txt` in the same
/// directory for the provenance of each file).
///
/// Nothing here is regenerated at test time. Crucially, **no fingerprint is
/// hardcoded**: the script re-mints random keys on every run, so every
/// fingerprint a test needs is derived from the committed certificate itself.
pub mod corpus {
    use once_cell::sync::Lazy;
    use pgp::composed::{Deserializable, SignedPublicKey, SignedSecretKey};
    use pgp::types::KeyDetails;

    macro_rules! vector {
        ($name:ident, $file:literal) => {
            pub const $name: &[u8] =
                include_bytes!(concat!("../../resource/lfs/test/rpgp_vectors/", $file));
        };
    }
    macro_rules! vector_str {
        ($name:ident, $file:literal) => {
            pub const $name: &str =
                include_str!(concat!("../../resource/lfs/test/rpgp_vectors/", $file));
        };
    }
    macro_rules! aux_key {
        ($name:ident, $file:literal) => {
            pub const $name: &str =
                include_str!(concat!("../../resource/lfs/test/rpgp_aux_keys/", $file));
        };
    }

    // --- plaintext -------------------------------------------------------
    vector!(PAYLOAD, "payload.txt");

    // --- encrypted messages (RFC 9580 §5.1 / §5.3 / §5.13) ---------------
    vector!(ENC_V1SEIPD_MDC, "enc_v1seipd_mdc.pgp");
    vector!(ENC_V2SEIPD_OCB, "enc_v2seipd_ocb.pgp");
    vector!(ENC_MULTI_RECIPIENT, "enc_multi_recipient.pgp");
    vector!(ENC_SYMMETRIC_V1, "enc_symmetric_v1.pgp");
    vector!(ENC_SYMMETRIC_V2, "enc_symmetric_v2.pgp");
    vector!(ENC_SED_TAG9, "enc_sed_tag9.pgp");

    // --- signatures (RFC 9580 §5.2 / §7) ---------------------------------
    vector!(SIG_GOOD_DETACHED, "sig_good_detached.sig");
    vector!(SIG_V6_DETACHED, "sig_v6_detached.sig");
    vector!(SIG_SHA1_DETACHED, "sig_sha1_detached.sig");
    vector!(SIG_EXPIRED, "sig_expired.sig");
    vector!(SIG_REVOKEDKEY, "sig_revokedkey.sig");
    vector!(SIG_BAD_MUTATED, "sig_bad_mutated.sig");
    vector!(SIG_GOOD_INLINE_V6, "sig_good_inline_v6.pgp");
    vector!(SIG_INLINE_COMPRESSED, "sig_inline_compressed.pgp");
    vector!(TWO_SIGNER, "two_signer.pgp");
    vector!(SIG_STRONG_WEAK_SAME_KEY, "sig_strong_weak_same_key.sig");
    vector!(
        SIG_TWO_SAME_ISSUER_ONE_VALID,
        "sig_two_same_issuer_one_valid.sig"
    );
    vector_str!(SIG_GOOD_CLEARTEXT, "sig_good_cleartext.asc");
    vector_str!(SIG_TWO_SIGNER_CLEARTEXT, "sig_two_signer_cleartext.asc");

    // --- malformed / adversarial ------------------------------------------
    vector!(GARBAGE, "garbage.bin");
    vector!(EMPTY, "empty.bin");
    vector!(PKESK_NO_SEIPD, "pkesk_no_seipd.pgp");
    vector_str!(TRUNCATED_ARMOR, "truncated_armor.asc");
    vector_str!(CORRUPT_CRC, "corrupt_crc.asc");

    // --- auxiliary signer certificates -------------------------------------
    aux_key!(AUX_GOOD, "aux_good.asc");
    aux_key!(AUX_EXPIRED, "aux_expired.asc");
    aux_key!(AUX_REVOKED, "aux_revoked.asc");
    aux_key!(AUX_SHA1, "aux_sha1.asc");
    aux_key!(AUX_FORGED_REVOCATION, "aux_forged_revocation.asc");
    // `aux_v6.asc` is a v6 key in the RFC 9580 profile. Unlike the other aux
    // keys it is a *secret* key block, protected with `CORPUS_PASSPHRASE`.
    aux_key!(AUX_V6_SECRET, "aux_v6.asc");

    // --- fixture keyring ----------------------------------------------------
    pub const KEY1_SECRET: &str = include_str!("../../resource/lfs/test/rpgp/key1.asc");
    pub const KEY2_PUBLIC: &str = include_str!("../../resource/lfs/test/rpgp/key2.asc");
    pub const KEY3_PUBLIC: &str = include_str!("../../resource/lfs/test/rpgp/key3.asc");

    /// Every committed secret key in the corpus uses this passphrase.
    pub const CORPUS_PASSPHRASE: &[u8] = b"123456";

    // --- parse-once certificates -------------------------------------------

    fn public(block: &str) -> SignedPublicKey {
        SignedPublicKey::from_string(block)
            .expect("committed corpus certificate must parse")
            .0
    }
    fn secret(block: &str) -> SignedSecretKey {
        SignedSecretKey::from_string(block)
            .expect("committed corpus secret key must parse")
            .0
    }

    pub static AUX_GOOD_CERT: Lazy<SignedPublicKey> = Lazy::new(|| public(AUX_GOOD));
    pub static AUX_EXPIRED_CERT: Lazy<SignedPublicKey> = Lazy::new(|| public(AUX_EXPIRED));
    pub static AUX_REVOKED_CERT: Lazy<SignedPublicKey> = Lazy::new(|| public(AUX_REVOKED));
    pub static AUX_SHA1_CERT: Lazy<SignedPublicKey> = Lazy::new(|| public(AUX_SHA1));
    pub static AUX_FORGED_REVOCATION_CERT: Lazy<SignedPublicKey> =
        Lazy::new(|| public(AUX_FORGED_REVOCATION));
    pub static AUX_V6_KEY: Lazy<SignedSecretKey> = Lazy::new(|| secret(AUX_V6_SECRET));
    pub static KEY1: Lazy<SignedSecretKey> = Lazy::new(|| secret(KEY1_SECRET));
    pub static KEY2_CERT: Lazy<SignedPublicKey> = Lazy::new(|| public(KEY2_PUBLIC));
    pub static KEY3_CERT: Lazy<SignedPublicKey> = Lazy::new(|| public(KEY3_PUBLIC));

    // --- derived fingerprints ----------------------------------------------
    //
    // Deriving rather than hardcoding is what makes the corpus regenerable:
    // `gen_rpgp_test_vectors.sh` mints fresh random keys on every run, so any
    // fingerprint literal in a test would silently rot.

    /// The fingerprint of the subkey a certificate actually signs *data* with.
    ///
    /// Selecting on `algorithm().can_sign()` alone is not enough: `sq key
    /// generate --own-key` emits both a signing subkey and an authentication
    /// subkey, and both are EdDSALegacy. The distinguishing property is the Key
    /// Flags subpacket (RFC 9580 §5.2.3.29 bit 0x02, "may be used to sign
    /// data") on the subkey binding signature, which is what this reads.
    ///
    /// Note this deliberately inspects the flags directly rather than calling
    /// the engine's own capability helpers — a test fixture that derived its
    /// expectations from the code under test would not be much of a test.
    fn fpr_of_data_signing_subkey(cert: &SignedPublicKey) -> String {
        cert.public_subkeys
            .iter()
            .find(|sub| sub.signatures.iter().any(|sig| sig.key_flags().sign()))
            .map(|sub| sub.key.fingerprint().to_string().to_uppercase())
            .expect("certificate must carry a data-signing subkey")
    }

    /// Primary-key fingerprint of `aux_good.asc`, uppercase hex.
    pub fn aux_good_primary_fpr() -> String {
        AUX_GOOD_CERT
            .primary_key
            .fingerprint()
            .to_string()
            .to_uppercase()
    }

    /// Fingerprint of the signing subkey that issued `sig_good_detached.sig`.
    pub fn aux_good_sign_fpr() -> String {
        fpr_of_data_signing_subkey(&AUX_GOOD_CERT)
    }

    /// Primary-key fingerprint of the v6 aux key.
    pub fn aux_v6_primary_fpr() -> String {
        AUX_V6_KEY
            .primary_key
            .fingerprint()
            .to_string()
            .to_uppercase()
    }

    /// Fingerprint of the v6 aux key's signing subkey, falling back to the
    /// primary when the key has no dedicated signing subkey.
    pub fn aux_v6_sign_fpr() -> String {
        AUX_V6_KEY
            .secret_subkeys
            .iter()
            .find(|sub| sub.signatures.iter().any(|sig| sig.key_flags().sign()))
            .map(|sub| sub.key.fingerprint().to_string().to_uppercase())
            .unwrap_or_else(aux_v6_primary_fpr)
    }

    /// Primary-key fingerprint of the `key1.asc` fixture.
    pub fn key1_primary_fpr() -> String {
        KEY1.primary_key.fingerprint().to_string().to_uppercase()
    }

    /// Long key ID (low 64 bits, 16 hex digits) of a full fingerprint.
    pub fn long_key_id(fpr: &str) -> String {
        fpr[fpr.len() - 16..].to_string()
    }
}

// ---------------------------------------------------------------------------
// RFC 9580 Appendix A
// ---------------------------------------------------------------------------

/// Known-answer vectors transcribed verbatim from RFC 9580 Appendix A.
///
/// These are the highest-value vectors in the suite: they are normative,
/// stable across tool versions, and require no `sq`/`gpg` at generation time.
/// They are also the *only* coverage the engine has for Argon2 (§3.7.1.4),
/// AEAD-EAX (§5.13.3) and AEAD-GCM (§5.13.5), none of which the engine can
/// produce itself.
pub mod rfc9580 {
    /// A.1 — sample version 4 Ed25519Legacy public key.
    pub const A1_V4_ED25519LEGACY_CERT: &str = "\
-----BEGIN PGP PUBLIC KEY BLOCK-----

xjMEU/NfCxYJKwYBBAHaRw8BAQdAPwmJlL3ZFu1AUxl5NOSofIBzOhKA1i+AEJku
Q+47JAY=
-----END PGP PUBLIC KEY BLOCK-----
";

    /// A.1 — the fingerprint the RFC states for [`A1_V4_ED25519LEGACY_CERT`].
    pub const A1_FINGERPRINT: &str = "C959BDBAFA32A2F89A153B678CFDE12197965A9A";

    /// A.2 — detached signature over [`A2_PLAINTEXT`] by the A.1 key.
    pub const A2_V4_ED25519LEGACY_SIG: &str = "\
-----BEGIN PGP SIGNATURE-----

iF4EABYIAAYFAlX5X5UACgkQjP3hIZeWWpr2IgD/VvkMypjiECY3vZg/2xbBMd/S
ftgr9N3lYG4NdWrtM2YBANCcT6EVJ/A44PV/IgHYLy6iyQMyZfps60iehUuuYbQE
-----END PGP SIGNATURE-----
";

    /// A.2 — the signed data. The RFC signs the literal string `OpenPGP`.
    pub const A2_PLAINTEXT: &[u8] = b"OpenPGP";

    /// A.3 — sample version 6 certificate (transferable public key).
    pub const A3_V6_CERT: &str = "\
-----BEGIN PGP PUBLIC KEY BLOCK-----

xioGY4d/4xsAAAAg+U2nu0jWCmHlZ3BqZYfQMxmZu52JGggkLq2EVD34laPCsQYf
GwoAAABCBYJjh3/jAwsJBwUVCg4IDAIWAAKbAwIeCSIhBssYbE8GCaaX5NUt+mxy
KwwfHifBilZwj2Ul7Ce62azJBScJAgcCAAAAAK0oIBA+LX0ifsDm185Ecds2v8lw
gyU2kCcUmKfvBXbAf6rhRYWzuQOwEn7E/aLwIwRaLsdry0+VcallHhSu4RN6HWaE
QsiPlR4zxP/TP7mhfVEe7XWPxtnMUMtf15OyA51YBM4qBmOHf+MZAAAAIIaTJINn
+eUBXbki+PSAld2nhJh/LVmFsS+60WyvXkQ1wpsGGBsKAAAALAWCY4d/4wKbDCIh
BssYbE8GCaaX5NUt+mxyKwwfHifBilZwj2Ul7Ce62azJAAAAAAQBIKbpGG2dWTX8
j+VjFM21J0hqWlEg+bdiojWnKfA5AQpWUWtnNwDEM0g12vYxoWM8Y81W+bHBw805
I8kWVkXU6vFOi+HWvv/ira7ofJu16NnoUkhclkUrk0mXubZvyl4GBg==
-----END PGP PUBLIC KEY BLOCK-----
";

    /// A.3 — primary-key fingerprint stated by the RFC (§5.5.4.3, SHA2-256).
    pub const A3_PRIMARY_FINGERPRINT: &str =
        "CB186C4F0609A697E4D52DFA6C722B0C1F1E27C18A56708F6525EC27BAD9ACC9";

    /// A.3 — subkey fingerprint stated by the RFC.
    pub const A3_SUBKEY_FINGERPRINT: &str =
        "12C83F1E706F6308FE151A417743A1F033790E93E9978488D1DB378DA9930885";

    /// A.4 — sample version 6 secret key, unprotected.
    pub const A4_V6_SECRET_UNLOCKED: &str = "\
-----BEGIN PGP PRIVATE KEY BLOCK-----

xUsGY4d/4xsAAAAg+U2nu0jWCmHlZ3BqZYfQMxmZu52JGggkLq2EVD34laMAGXKB
exK+cH6NX1hs5hNhIB00TrJmosgv3mg1ditlsLfCsQYfGwoAAABCBYJjh3/jAwsJ
BwUVCg4IDAIWAAKbAwIeCSIhBssYbE8GCaaX5NUt+mxyKwwfHifBilZwj2Ul7Ce6
2azJBScJAgcCAAAAAK0oIBA+LX0ifsDm185Ecds2v8lwgyU2kCcUmKfvBXbAf6rh
RYWzuQOwEn7E/aLwIwRaLsdry0+VcallHhSu4RN6HWaEQsiPlR4zxP/TP7mhfVEe
7XWPxtnMUMtf15OyA51YBMdLBmOHf+MZAAAAIIaTJINn+eUBXbki+PSAld2nhJh/
LVmFsS+60WyvXkQ1AE1gCk95TUR3XFeibg/u/tVY6a//1q0NWC1X+yui3O24wpsG
GBsKAAAALAWCY4d/4wKbDCIhBssYbE8GCaaX5NUt+mxyKwwfHifBilZwj2Ul7Ce6
2azJAAAAAAQBIKbpGG2dWTX8j+VjFM21J0hqWlEg+bdiojWnKfA5AQpWUWtnNwDE
M0g12vYxoWM8Y81W+bHBw805I8kWVkXU6vFOi+HWvv/ira7ofJu16NnoUkhclkUr
k0mXubZvyl4GBg==
-----END PGP PRIVATE KEY BLOCK-----
";

    /// A.5 — the same secret key, locked with Argon2 + AEAD-OCB.
    pub const A5_V6_SECRET_LOCKED: &str = "\
-----BEGIN PGP PRIVATE KEY BLOCK-----

xYIGY4d/4xsAAAAg+U2nu0jWCmHlZ3BqZYfQMxmZu52JGggkLq2EVD34laP9JgkC
FARdb9ccngltHraRe25uHuyuAQQVtKipJ0+r5jL4dacGWSAheCWPpITYiyfyIOPS
3gIDyg8f7strd1OB4+LZsUhcIjOMpVHgmiY/IutJkulneoBYwrEGHxsKAAAAQgWC
Y4d/4wMLCQcFFQoOCAwCFgACmwMCHgkiIQbLGGxPBgmml+TVLfpscisMHx4nwYpW
cI9lJewnutmsyQUnCQIHAgAAAACtKCAQPi19In7A5tfORHHbNr/JcIMlNpAnFJin
7wV2wH+q4UWFs7kDsBJ+xP2i8CMEWi7Ha8tPlXGpZR4UruETeh1mhELIj5UeM8T/
0z+5oX1RHu11j8bZzFDLX9eTsgOdWATHggZjh3/jGQAAACCGkySDZ/nlAV25Ivj0
gJXdp4SYfy1ZhbEvutFsr15ENf0mCQIUBA5hhGgp2oaavg6mFUXcFMwBBBUuE8qf
9Ock+xwusd+GAglBr5LVyr/lup3xxQvHXFSjjA2haXfoN6xUGRdDEHI6+uevKjVR
v5oAxgu7eJpaXNjCmwYYGwoAAAAsBYJjh3/jApsMIiEGyxhsTwYJppfk1S36bHIr
DB8eJ8GKVnCPZSXsJ7rZrMkAAAAABAEgpukYbZ1ZNfyP5WMUzbUnSGpaUSD5t2Ki
Nacp8DkBClZRa2c3AMQzSDXa9jGhYzxjzVb5scHDzTkjyRZWRdTq8U6L4da+/+Kt
ruh8m7Xo2ehSSFyWRSuTSZe5tm/KXgYG
-----END PGP PRIVATE KEY BLOCK-----
";

    /// A.5 — passphrase for [`A5_V6_SECRET_LOCKED`].
    pub const A5_PASSPHRASE: &[u8] = b"correct horse battery staple";

    /// A.6 — cleartext-signed message (§7), verifiable with [`A3_V6_CERT`].
    pub const A6_CLEARTEXT: &str = "\
-----BEGIN PGP SIGNED MESSAGE-----

What we need from the grocery store:

- - tofu
- - vegetables
- - noodles

-----BEGIN PGP SIGNATURE-----

wpgGARsKAAAAKQWCY5ijYyIhBssYbE8GCaaX5NUt+mxyKwwfHifBilZwj2Ul7Ce6
2azJAAAAAGk2IHZJX1AhiJD39eLuPBgiUU9wUA9VHYblySHkBONKU/usJ9BvuAqo
/FvLFuGWMbKAdA+epq7V4HOtAPlBWmU8QOd6aud+aSunHQaaEJ+iTFjP2OMW0KBr
NK2ay45cX1IVAQ==
-----END PGP SIGNATURE-----
";

    /// A.6/A.7 — the plaintext both messages carry, with the canonical
    /// `<CR><LF>` line endings a text signature (§5.2.1.2) is computed over.
    pub const A6_CANONICAL_TEXT: &[u8] =
        b"What we need from the grocery store:\r\n\r\n- tofu\r\n- vegetables\r\n- noodles\r\n";

    /// A.6 — the SHA2-512 digest the RFC states for the signed data.
    pub const A6_DIGEST_HEX: &str = concat!(
        "69365bf44a97af1f0844f1f6ab83fdf6b36f26692efaa621a8aac91c4e29ea07",
        "e894cabc6e2f20eedfce6c03b89141a2cc7cbe245e6e7a5654addbec5000b89b",
    );

    /// A.7 — the same message and signature, inline-signed.
    pub const A7_INLINE_SIGNED: &str = "\
-----BEGIN PGP MESSAGE-----

xEYGAQobIHZJX1AhiJD39eLuPBgiUU9wUA9VHYblySHkBONKU/usyxhsTwYJppfk
1S36bHIrDB8eJ8GKVnCPZSXsJ7rZrMkBy0p1AAAAAABXaGF0IHdlIG5lZWQgZnJv
bSB0aGUgZ3JvY2VyeSBzdG9yZToKCi0gdG9mdQotIHZlZ2V0YWJsZXMKLSBub29k
bGVzCsKYBgEbCgAAACkFgmOYo2MiIQbLGGxPBgmml+TVLfpscisMHx4nwYpWcI9l
JewnutmsyQAAAABpNiB2SV9QIYiQ9/Xi7jwYIlFPcFAPVR2G5ckh5ATjSlP7rCfQ
b7gKqPxbyxbhljGygHQPnqau1eBzrQD5QVplPEDnemrnfmkrpx0GmhCfokxYz9jj
FtCgazStmsuOXF9SFQE=
-----END PGP MESSAGE-----
";

    /// The plaintext every Appendix A encryption sample carries.
    pub const A_PLAINTEXT_HELLO: &[u8] = b"Hello, world!";

    /// A.8 — X25519 + AEAD-OCB encrypted message for the A.3/A.4 key.
    pub const A8_X25519_AEAD_OCB: &str = "\
-----BEGIN PGP MESSAGE-----

wV0GIQYSyD8ecG9jCP4VGkF3Q6HwM3kOk+mXhIjR2zeNqZMIhRmHzxjV8bU/gXzO
WgBM85PMiVi93AZfJfhK9QmxfdNnZBjeo1VDeVZheQHgaVf7yopqR6W1FT6NOrfS
aQIHAgZhZBZTW+CwcW1g4FKlbExAf56zaw76/prQoN+bAzxpohup69LA7JW/Vp0l
yZnuSj3hcFj0DfqLTGgr4/u717J+sPWbtQBfgMfG9AOIwwrUBqsFE9zW+f1zdlYo
bhF30A+IitsxxA==
-----END PGP MESSAGE-----
";

    /// The passphrase used by the A.9/A.10/A.11/A.12 symmetric samples.
    pub const A_SYMMETRIC_PASSPHRASE: &[u8] = b"password";

    /// A.9 — AEAD-EAX, AES-128, v6 SKESK + v2 SEIPD.
    pub const A9_EAX_SKESK: &str = "\
-----BEGIN PGP MESSAGE-----

w0AGHgcBCwMIpa5XnR/F2Cv/aSJPkZmTs1Bvo7WaanPP+MXvxfQcV/tU4cImgV14
KPX5LEVOtl6+AKtZhsaObnxV0mkCBwEGn/kOOzIZZPOkKRPI3MZhkyUBUifvt+rq
pJ8EwuZ0F11KPSJu1q/LnKmsEiwUcOEcY9TAqyQcapOK1Iv5mlqZuQu6gyXeYQR1
QCWKt5Wala0FHdqW6xVDHf719eIlXKeCYVRuM5o=
-----END PGP MESSAGE-----
";

    /// A.10 — AEAD-OCB, AES-128, v6 SKESK + v2 SEIPD.
    pub const A10_OCB_SKESK: &str = "\
-----BEGIN PGP MESSAGE-----

wz8GHQcCCwMIVqKY0vXjZFP/z8xcEWZO2520JZDX3EawckG2EsOBLP/76gDyNHsl
ZBEj+IeuYNT9YU4IN9gZ02zSaQIHAgYgpmH3MfyaMDK1YjMmAn46XY21dI6+/wsM
WRDQns3WQf+f04VidYA1vEl1TOG/P/+n2tCjuBBPUTPPQqQQCoPu9MobSAGohGv0
K82nyM6dZeIS8wHLzZj9yt5pSod61CRzI/boVw==
-----END PGP MESSAGE-----
";

    /// A.11 — AEAD-GCM, AES-128, v6 SKESK + v2 SEIPD.
    pub const A11_GCM_SKESK: &str = "\
-----BEGIN PGP MESSAGE-----

wzwGGgcDCwMI6dOXhbIHAAj/tC58SD70iERXyzcmubPbn/d25fTZpAlS4kRymIUa
v/91Jt8t1VRBdXmneZ/SaQIHAwb8uUSQvLmLvcnRBsYJAmaUD3LontwhtVlrFXax
Ae0Pn/xvxtZbv9JNzQeQlm5tHoWjAFN4TLHYtqBpnvEhVaeyrWJYUxtXZR/Xd3kS
+pXjXZtAIW9ppMJI2yj/QzHxYykHOZ5v+Q==
-----END PGP MESSAGE-----
";

    /// A.12.1 — v4 SKESK using Argon2 (t=1, p=4, m=2^21) with AES-128.
    pub const A12_ARGON2_AES128: &str = "\
-----BEGIN PGP MESSAGE-----

wycEBwScUvg8J/leUNU1RA7N/zE2AQQVnlL8rSLPP5VlQsunlO+ECxHSPgGYGKY+
YJz4u6F+DDlDBOr5NRQXt/KJIf4m4mOlKyC/uqLbpnLJZMnTq3o79GxBTdIdOzhH
XfA3pqV4mTzF
-----END PGP MESSAGE-----
";

    /// A.12.2 — the same, AES-192.
    pub const A12_ARGON2_AES192: &str = "\
-----BEGIN PGP MESSAGE-----

wy8ECAThTKxHFTRZGKli3KNH4UP4AQQVhzLJ2va3FG8/pmpIPd/H/mdoVS5VBLLw
F9I+AdJ1Sw56PRYiKZjCvHg+2bnq02s33AJJoyBexBI4QKATFRkyez2gldJldRys
LVg77Mwwfgl2n/d572WciAM=
-----END PGP MESSAGE-----
";

    /// A.12.3 — the same, AES-256.
    pub const A12_ARGON2_AES256: &str = "\
-----BEGIN PGP MESSAGE-----

wzcECQS4eJUgIG/3mcaILEJFpmJ8AQQVnZ9l7KtagdClm9UaQ/Z6M/5roklSGpGu
623YmaXezGj80j4B+Ku1sgTdJo87X1Wrup7l0wJypZls21Uwd67m9koF60eefH/K
95D1usliXOEm8ayQJQmZrjf6K6v9PWwqMQ==
-----END PGP MESSAGE-----
";
}

// ---------------------------------------------------------------------------
// Key fixtures
// ---------------------------------------------------------------------------

/// Lazily generated key material shared by the entire test binary.
///
/// The test harness runs as one process, so a `Lazy` static is generated once
/// no matter how many tests read it. That is the difference between a suite
/// that generates hundreds of keys and one that generates eight.
pub mod keys {
    use once_cell::sync::Lazy;
    use pgp::composed::{Deserializable, SignedPublicKey, SignedSecretKey};
    use pgp::types::KeyDetails;

    use crate::key::{
        generate_key_rev_cert_internal, import_rev_cert_internal, revoke_subkey_internal,
        update_key_expiration_internal,
    };
    use crate::keygen::keygen_dynamic;
    use crate::types::{GfrKeyAlgo, GfrKeyConfig, GfrOpenPGPKeyVersion, GfrRevocationCode};

    /// A generated key in every representation the tests need.
    pub struct Fixture {
        pub secret: SignedSecretKey,
        pub secret_armored: String,
        pub public_armored: String,
        pub primary_fpr: String,
        /// Fingerprints of the signing-capable subkeys, in key order.
        pub sign_subkey_fprs: Vec<String>,
        /// Fingerprints of the encryption-capable subkeys, in key order.
        pub enc_subkey_fprs: Vec<String>,
    }

    impl Fixture {
        /// First signing subkey fingerprint. Panics if the fixture has none.
        pub fn sign_subkey_fpr(&self) -> &str {
            self.sign_subkey_fprs
                .first()
                .expect("fixture has no signing subkey")
        }

        /// First encryption subkey fingerprint. Panics if the fixture has none.
        pub fn enc_subkey_fpr(&self) -> &str {
            self.enc_subkey_fprs
                .first()
                .expect("fixture has no encryption subkey")
        }
    }

    /// Build a [`GfrKeyConfig`] with the fields tests actually vary.
    pub fn cfg(
        algo: GfrKeyAlgo,
        can_sign: bool,
        can_encrypt: bool,
        ver: GfrOpenPGPKeyVersion,
    ) -> GfrKeyConfig {
        GfrKeyConfig {
            algo,
            can_sign,
            can_encrypt,
            can_auth: false,
            has_passphrase: false,
            ver,
            expiration_epoch_secs: 0,
        }
    }

    /// [`cfg`] with an absolute expiry stamped on the component.
    pub fn cfg_expiring(
        algo: GfrKeyAlgo,
        can_sign: bool,
        can_encrypt: bool,
        ver: GfrOpenPGPKeyVersion,
        expiration_epoch_secs: u64,
    ) -> GfrKeyConfig {
        let mut c = cfg(algo, can_sign, can_encrypt, ver);
        c.expiration_epoch_secs = expiration_epoch_secs;
        c
    }

    /// Generate a key directly. Prefer the `Lazy` fixtures below; reach for
    /// this only when a test needs a shape none of them provide.
    ///
    /// (Named `generate` rather than `gen` because `gen` is a reserved keyword
    /// in edition 2024.)
    pub fn generate(uid: &str, primary: &GfrKeyConfig, subs: &[GfrKeyConfig]) -> SignedSecretKey {
        keygen_dynamic(uid, primary, subs).expect("fixture key generation must succeed")
    }

    fn fixture_from(secret: SignedSecretKey) -> Fixture {
        let secret_armored = secret
            .to_armored_string(crate::utils::armor_opts())
            .expect("fixture secret key must armor");
        let public_armored = SignedPublicKey::from(secret.clone())
            .to_armored_string(crate::utils::armor_opts())
            .expect("fixture public key must armor");
        let primary_fpr = secret.primary_key.fingerprint().to_string().to_uppercase();

        let mut sign_subkey_fprs = Vec::new();
        let mut enc_subkey_fprs = Vec::new();
        for sub in &secret.secret_subkeys {
            let fpr = sub.key.fingerprint().to_string().to_uppercase();
            if sub.key.algorithm().can_sign() {
                sign_subkey_fprs.push(fpr.clone());
            } else {
                enc_subkey_fprs.push(fpr);
            }
        }

        Fixture {
            secret,
            secret_armored,
            public_armored,
            primary_fpr,
            sign_subkey_fprs,
            enc_subkey_fprs,
        }
    }

    fn reparse(armored: &str) -> SignedSecretKey {
        SignedSecretKey::from_string(armored)
            .expect("derived fixture must re-parse")
            .0
    }

    /// v4 Ed25519 primary + Ed25519 signing subkey + encryption subkey.
    /// The workhorse fixture: unprotected, so no passphrase callback is needed.
    pub static V4_SIGN: Lazy<Fixture> = Lazy::new(|| {
        fixture_from(generate(
            "V4 Fixture <v4@example.test>",
            &cfg(GfrKeyAlgo::ED25519, true, false, GfrOpenPGPKeyVersion::V4),
            &[
                cfg(GfrKeyAlgo::ED25519, true, false, GfrOpenPGPKeyVersion::V4),
                cfg(GfrKeyAlgo::ED25519, false, true, GfrOpenPGPKeyVersion::V4),
            ],
        ))
    });

    /// v6 Ed25519 primary + Ed25519 signing subkey + native X25519 subkey.
    pub static V6_SIGN: Lazy<Fixture> = Lazy::new(|| {
        fixture_from(generate(
            "V6 Fixture <v6@example.test>",
            &cfg(GfrKeyAlgo::ED25519, true, false, GfrOpenPGPKeyVersion::V6),
            &[
                cfg(GfrKeyAlgo::ED25519, true, false, GfrOpenPGPKeyVersion::V6),
                cfg(GfrKeyAlgo::ED25519, false, true, GfrOpenPGPKeyVersion::V6),
            ],
        ))
    });

    /// v4 NIST P-256 ECDSA primary + ECDH subkey — exercises the SEC1 point
    /// wire format (§11.2.1) and the ECDH KDF parameters (§11.5.1).
    pub static V4_NISTP256: Lazy<Fixture> = Lazy::new(|| {
        fixture_from(generate(
            "P256 Fixture <p256@example.test>",
            &cfg(GfrKeyAlgo::NISTP256, true, false, GfrOpenPGPKeyVersion::V4),
            &[cfg(
                GfrKeyAlgo::NISTP256,
                false,
                true,
                GfrOpenPGPKeyVersion::V4,
            )],
        ))
    });

    /// v4 Ed25519 primary with no subkeys at all.
    pub static V4_PRIMARY_ONLY: Lazy<Fixture> = Lazy::new(|| {
        fixture_from(generate(
            "Bare Fixture <bare@example.test>",
            &cfg(GfrKeyAlgo::ED25519, true, false, GfrOpenPGPKeyVersion::V4),
            &[],
        ))
    });

    /// [`V4_SIGN`]'s shape with its encryption subkey revoked (§5.2.1.12).
    pub static V4_REVOKED_SUBKEY: Lazy<Fixture> = Lazy::new(|| {
        let base = &*V4_SIGN;
        let out = revoke_subkey_internal(
            0,
            &base.secret_armored,
            base.enc_subkey_fpr(),
            GfrRevocationCode::Retired,
            Some("fixture"),
            None,
        )
        .expect("subkey revocation fixture");
        fixture_from(reparse(&out.secret))
    });

    /// v6 Ed25519 primary + ML-KEM-768+X25519 encryption subkey: the
    /// post-quantum shape of the original 4.86 GB SEIPD v1 report.
    pub static V6_PQ: Lazy<Fixture> = Lazy::new(|| {
        fixture_from(generate(
            "V6 PQ Fixture <v6-pq@example.test>",
            &cfg(GfrKeyAlgo::ED25519, true, false, GfrOpenPGPKeyVersion::V6),
            &[cfg(
                GfrKeyAlgo::KYBER768X25519,
                false,
                true,
                GfrOpenPGPKeyVersion::V6,
            )],
        ))
    });

    /// [`V4_SIGN`]'s shape with the primary key revoked (§5.2.1.11), produced
    /// the way a user would: generate a revocation certificate, then import it.
    pub static V4_REVOKED_PRIMARY: Lazy<Fixture> = Lazy::new(|| {
        let base = &*V4_SIGN;
        let rev_cert = generate_key_rev_cert_internal(
            0,
            &base.secret_armored,
            GfrRevocationCode::Compromised,
            Some("fixture"),
            None,
        )
        .expect("revocation certificate fixture");
        let out = import_rev_cert_internal(&base.secret_armored, &rev_cert)
            .expect("revocation certificate import fixture");
        fixture_from(reparse(&out.secret))
    });

    /// [`V4_SIGN`]'s shape carrying a Key Expiration Time subpacket (§5.2.3.13).
    ///
    /// The subpacket stores seconds *after key creation*, so a freshly
    /// generated key cannot be given an expiry in the past — the engine
    /// correctly rejects that as invalid input. This fixture therefore expires
    /// one second after creation, which makes it useful for asserting that the
    /// subpacket is present and carries the expected absolute value, but
    /// **not** for "is this key expired right now" — that would race the clock.
    ///
    /// For already-expired behaviour use the committed
    /// [`crate::testutil::corpus::AUX_EXPIRED_CERT`], which was minted with a
    /// past expiry precisely for this purpose.
    /// v4 RSA-1024 primary (sign, certify) + RSA-1024 encryption subkey,
    /// generated once with GnuPG 2.4 (unprotected). Neither GF nor rPGP will
    /// generate such a key (RFC 9580 §12.4), so it stands in for an old
    /// imported key that GF must refuse to use.
    pub static V4_RSA1024: Lazy<Fixture> = Lazy::new(|| fixture_from(reparse(RSA1024_SECRET)));

    /// v4 RSA-2048 primary (sign, certify), generated once with GnuPG 2.4
    /// (unprotected). The smallest RSA key GF may still use (RFC 9580 §12.4)
    /// though it would not generate one; also the one fixture algorithm that
    /// can carry a SHA-1 self-signature.
    pub static V4_RSA2048: Lazy<Fixture> = Lazy::new(|| fixture_from(reparse(RSA2048_SECRET)));

    const RSA2048_SECRET: &str = r"-----BEGIN PGP PRIVATE KEY BLOCK-----

lQOYBGrYvAoBCADBgLoXib8v2DUUF00lenqt+Vd1qziqDZqHU+VLARfN3toWbH+R
fd8LsF5iKUmaxi7469wOWup3Wg+/1XTdMJBEvFZ1yFsHoyLn64C5GtCP+G5Ajguw
lkln2pQGEXcpfEFV0LAY4EwA++1+atBzuwLfZ0b1EvRrHjcPqqTMUkzElocZ+0qB
0iwc9w8YSSoGWdE33mSpC81X0ctmF9zikXjbL7rEJVoyRHw/uGl4gGzdwpsPX0Ct
VfZsZPAh77WXz0+EKUxu1AnRthLwVERk66O6bNB0sGCcdQes/AdjDOtx69C+lMxd
UliFQYesG6/4qBC+SmKcLfc1p5Xyu+QL6daXABEBAAEAB/sGBRzFAAfZjImGpWBE
08fCLdd1KtD0Qk7ApNSlkymTFyx1w+kGw0jWsrOHrH0oLDHMIn3nPG1MvGDF8fVx
dfrf7B6+QuGsyLTx9wyQmT2hnQIZ+V+O1xNwOUBmjwpSPMB+RWsjUFG+yMJWhEdO
lflK58L4Qq2XhqasxItrh+FFwNMmynlHzbMop7Cf5DCPHWXeumWorEw6Ym8VPhy5
p66xiqzIslOAc3E/5zJ9Pu5Bmg7DkGuQSbHZcoD5wlKbrW8xD8caTCYL/4vOpRdK
CAaqRc8VGHxHntzByJ9QWY8qD6uJsv+wud4Dz72Q5qQ5OckqC0klyevkRueEWACZ
VyzRBADEF/PqQjZfAVYQhBFdLZi1i2n5/IIfekfJ8E9ZPfEGiA1Rl/JjskXm1cC7
kWW/g0rd9YGTzNMm22gzs/9HbKUp4bO2iPsJm+fWxK9ivrW/Typ6rny46JVQnHL+
LAE8o2vt4qGqmYx+HgWSBnDGxRr6zdKvNvPd9436F7+zAUun7QQA/J4ovNkV5AbR
sKVNpYlZ1MJS/1cp1abLzziyB4JBl5ZmyREUGzuguzgwsH9wEE92fjOqgEjYevbB
yDLuWO9r28WT5O87pYnXaUqynhHGTwzrR0NlZh6tsgRWHeE/zw2XtXx1NBpaK5w5
BjlasPfBk3alrEj3tEvlLdlQyIwx4BMEAN7T/PvDoNHEEt5v66rxInjtV7uHm62h
IXiOGoq/hqVQMPdzAhlPGDDf99qj7zRA8JsUCYnna/mFTwJEuhs2bZp4nbLb6tzI
fEQcHwE00qSHftX4HYqm2sCN+gWcT9Sj1MLtfuI2ork9MJemi8vM4R4HdsiucjTr
qChjBAUkeKAoTTK0JlJTQTIwNDggRml4dHVyZSA8cnNhMjA0OEBleGFtcGxlLnRl
c3Q+iQFOBBMBCgA4FiEENCBJpe6EwrHehWUJUTz+uau+WjQFAmrYvAoCGwMFCwkI
BwIGFQoJCAsCBBYCAwECHgECF4AACgkQUTz+uau+WjTW9Qf+OznDu2Ztj0zZYZG8
ACypJDXOlXMomrWztYDAEkY5IklEKRf4YIzevTDidA5IgBild7s792Wp+97yDm4Y
9Eq3nxGGNZW6Fgn55PXy8AV49EVYUEmhoQzUHzikH9296eaoUMRxmM6xjsyJ8UMf
3qKK6Bgf7dxS6XH0MhC71ZCPhN+UfBZ4k9CZtpwY6jQqvu8FxSKqEFXA5a/2nVem
yKK43oDat6HCHbxXkVWYxY3nTHV6Sm1dmt0El/Qmdnjg2YIoA/c789G6Ch1F+pMm
hs7mfUNhyuty990EzqRdeh0Ez1Eu6GV3GGMniHTfsXrEIPtfgbxE+Xc4qeiX1GJq
HnIi6g==
=uDVC
-----END PGP PRIVATE KEY BLOCK-----";

    /// Public certificates GnuPG 2.4 made with an encryption subkey rPGP 0.21
    /// cannot encrypt to: ElGamal (algorithm 16), and ECDH over
    /// brainpoolP256r1. Each has a usable Ed25519 primary.
    pub const ELGAMAL_SUBKEY_CERT: &str = r"-----BEGIN PGP PUBLIC KEY BLOCK-----

mDMEati7MBYJKwYBBAHaRw8BAQdA5l4wYaiuVKIFP+YUsakWpCpz3n5UJ81X8OAJ
Zj5adDG0HUxlZ2FjeSBlbGcgPGVsZ0BleGFtcGxlLnRlc3Q+iJAEExYKADgWIQRD
cWqBcgA/GDOIXkR9B75O3xAFewUCati7MAIbAwULCQgHAgYVCgkICwIEFgIDAQIe
AQIXgAAKCRB9B75O3xAFe6Z2AQC7mbnzg6zlOOBvQ/aUIgdYYeyGXYzoM/ohBUu2
yu3eUAEAmziiNAij4m6hpQcdCnM5BxqJ48HgnTYLRmY3N5988gy5Ag0Eati7MBAI
APndSPwdbjS8EGm7vstIK6EcxrL/kyCmD17/pZSJ/Dk6TzujXkNVeGy3s8vRPGF9
U2j7GXJyC/L09jtYIq2NBGyvbefok7xziHy4S7QLYAP4i0nHVglaoYaPk/koOlmV
AoKbplLHG9Ay6F81nppQ3WF8MrVgKKFTDB5pyPRuk2xmns0SGA1Z7Jflp7ToNTqt
u+H0DniS3FvD3SE+0h2Az+KzvP4pBQnUvj8W11i22rgXp1n6+Tp0CF3WsUcqoEHc
4NSu+ZWXXPt8vD19NcM4J9mXPXis8oYbtfQ0A5XBbIoMw6yEh3pCP2yNvbCX23l1
akCE/2asERNse3XnMM1rdecAAwUH/ilQGqxGhoC0fgTKmGrOuqwr8bWnmN0b25G6
pDJhfY19qS534k+D+l+o59fA3lkx6Ew7tzaury/SHEe96iuPF8MFX1GGkcnVf66r
FlzGCfFXdYwIhBaRK+yL4Wt3LQhg/yLoD/xHyQevCGYWEAxls/Si0v8NLpL2FCCZ
KUPXw11wGKxy4qiQV5Vhslm+U4hpkmL1xHBPY4XrJaUKyKlwPQhlWmKaiJOqKzWI
sLj+1Ddku1lRAaeIvdI4cD5RwHH2A2JaCdN1ukIGTCGMn7jI72XQ2pjSHHfIBQ2b
Uh5sIq3IjAl/dNB+SCV0jst5Vir70qZzcmBjphfUwJazE0RsZViIeAQYFgoAIBYh
BENxaoFyAD8YM4heRH0Hvk7fEAV7BQJq2LswAhsMAAoJEH0Hvk7fEAV7mcsBALrv
iaN+VF091bEPaXuVIPshceigV6VJXqWqeGpanslZAP9BuE5/YHaYwa2jE+JMfaCd
g7BVNiKVd3rHvWBbvJ9DDQ==
=Bw5N
-----END PGP PUBLIC KEY BLOCK-----";

    pub const BRAINPOOL_SUBKEY_CERT: &str = r"-----BEGIN PGP PUBLIC KEY BLOCK-----

mDMEati7MBYJKwYBBAHaRw8BAQdAupk2O20ApKiqN4oaOUOowDyjAWxA+jdF9dNE
5PD1ZcO0G0xlZ2FjeSBicCA8YnBAZXhhbXBsZS50ZXN0PoiQBBMWCgA4FiEEd0T3
cwAahiMSCt93woHP5Ff6BDUFAmrYuzACGwMFCwkIBwIGFQoJCAsCBBYCAwECHgEC
F4AACgkQwoHP5Ff6BDVorQEApau/1M+nk/ne5+wORTH1ZoLzc8076tlz20gi8C3T
Q54BAPj20wzcrylGxrmWIlfLnKm1qlP2uptddDiQmtKkztEHuFcEati7MRIJKyQD
AwIIAQEHAgMELu3064lyaK21FPYFcqN8caY76AWxY7S5+uy5V37NufodlZ+TH0X2
3NqSxCaUhElRo6k7TE+HtRzPUgW8aTPm3AMBCAeIeAQYFgoAIBYhBHdE93MAGoYj
Egrfd8KBz+RX+gQ1BQJq2LsxAhsMAAoJEMKBz+RX+gQ1nGMBANnyJJd6UH2+hmRJ
C+T44fCznPD0inkVRckMMZKTe2X1AQD+saZPsBp+JXmsLKfoe9C7JzPXFV8YstWp
uYmpmBORCA==
=+OfU
-----END PGP PUBLIC KEY BLOCK-----";

    /// v4 DSA-2048 primary (sign, certify), generated once with GnuPG 2.4
    /// (unprotected). RFC 9580 §12.5: "An implementation MUST NOT sign or
    /// verify using DSA keys", so it stands in for an old imported key that GF
    /// must refuse to use.
    pub static V4_DSA2048: Lazy<Fixture> = Lazy::new(|| fixture_from(reparse(DSA2048_SECRET)));

    const DSA2048_SECRET: &str = r"-----BEGIN PGP PRIVATE KEY BLOCK-----

lQNTBGrYp2YRCACfWU+FqJE5Jb7UXH3JuDuZDjaPVrgXQQZJb8/OQem/hpWHsVJk
H0eS4qI1lYk3s/NM9J2kRum1Kjv5CDyo6JxEgBH/Cams7MnddU0ArdTms4xGyct7
kCYyWcnuaKxYRtymC+BGNW+H6o+PzrXn9657vRuP0aHs1bkO7Tokox3lnghi1w75
NrDcPCcUq/vGCNHxFL1FMzU/V7+/dWpCdNCXyatO1RA/N0HYbvaQBdYPO01IYkCo
a3dS7XDtkgnTMPI++1g1HESGT/+68Rl5Nqxwi51IkGNnMlMGHjhyc1JTQGbDpS1A
iyf9Qq9UMesAqP7O1Eub/I0Yh89OWRjtrzmPAQDtt07ZzYJdnitd4cIjx76o2Nsy
3GTc609dXM/mWHUSMQgAlqJWvo9TqUiTaQ7FSorLzE3YK6/YIMJyioBqQ0PugCSX
AZKg/iVrOWSxVuaMYVM93GpwDSqDB2aBo+Brcz5E7la8wzuXlEBVkOVo9cz2mUMw
HeCfBNe5VgnbL2NlPPMwzxuMnfW07wsB8bMScfNg8UyQ7tRk+Y0XAN4gBqVncY7n
HquJJ1rSbSasdaR2yy0Ej4NLL4de3J8LIDBj8LsOsOTN6p4NXTyUZZs346dJkhRk
zsaiKoU/r5tK9Xl8bel8IwRjvSHk02YkCGP8EhtJqgFFJFz/ihaoH372fzAuJz7t
raOsJg1vq1mj9FxRToZ+xIfh7fSzZ/Q8amAz6XmkBggAgeNW2tqvHWev5c+3DgAZ
lWWRLASNdlGW3nArsZbGcvRzRz0NeLNjUax/eanxY631RCPKdaC13HBr76VLty6I
bk0fQgEZDuNY+LeWJZC4O+arPmZTl6L14NwVDpcFiuTGK3pB02MWJkC33jbZpySI
WkpjNjIlKi/sNKGct2dNThp7JYTFfPC4YIDeIrp9p1uJ81HmdVQ7u4tgWRiXT7Bv
2OHP4pIlYocsYd6gFdrLC4OqG+vMN0vSNmVfSspWOFIPIm5pkqxWLy6TRUeuUzLH
mm/J/0rEUi/vUSH3uyMN0d3I71xlepQAnzmpAoCuv+i8LiaqDscKoGjBdb+3rbjn
NgABAJ2zxfLgniTseu5dEGDeTRg6fF/7qLm25qzELZa0oJSkEtq0HkRTQSBGaXh0
dXJlIDxkc2FAZXhhbXBsZS50ZXN0PoiQBBMRCAA4FiEEdhnc0C8kk1LhqaQhqDp/
95KXqisFAmrYp2YCGwMFCwkIBwIGFQoJCAsCBBYCAwECHgECF4AACgkQqDp/95KX
qitsnAD/R67DLPbT16JeuFXpeC/+SLqIU+QH+jPpmyFsWMYX/SgBAOz6HjQsiPda
6LHSH+VHrOlqoFIy+nqMQ7JTe/p1FPye
=PGwK
-----END PGP PRIVATE KEY BLOCK-----";

    const RSA1024_SECRET: &str = r"-----BEGIN PGP PRIVATE KEY BLOCK-----

lQHYBGrX0zUBBADD3X9GN8IMYaJYs82kNaX/i09AInAqPcyGzmuWajHGFrQcrv9+
QY1XVYJBywQmBMZfTyYQbKtLuF+Gd+2Cblr7W1pM4xqaCzSx3TeVkzCpWVeSj2gq
X8n1YbrhqJqKRzEE5EY1Cht5hXlnSiWvY2beZWUkijkI6kU23ogR1R4aJwARAQAB
AAP/VRR+GM5XqHEwyD8ycMYtGVg2ULGk73Rwx44bBfeTL2zlSCBsMzfeYbVvI41A
fBNO6E/FRRUvQ3EPbaXiseml8w1w+bVpPFsj6QbBbQMAvlzLRKRw/CM76HTttHHa
APX4ibwSdx50oUBazPhlqySyHma/PQ5Wo19fMfkPx/hjY8ECAMPtlr3WA3qqUt8P
3hOJqSLjmC4kwgTgLrCQ9H4g3LN/i8FWr2AeQKt9tF3Ro7BJveP468Oe9N7u+hGy
aA0IYukCAP/q+X8PHto0gjg5mJir/QiTjAajxEHOOcoJJSzva1HtHUyNJp9YHxaF
ReeXrIBP/zmu8sN5p1w19crplNhryo8CAO6cIgZwvLbaEv9jiYMV9eTi+VF9+A7X
WHMPhMx048TenPoeCn65tf9mxOL4n0lHsXsgGpJYD2paoezsCVe4+A6k+7QfUlNB
IDEwMjQgPHJzYTEwMjRAdGVzdC5leGFtcGxlPojOBBMBCgA4FiEE3k5RkBwVhBtJ
p57+cdGo3WXK0LoFAmrX0zUCGwMFCwkIBwIGFQoJCAsCBBYCAwECHgECF4AACgkQ
cdGo3WXK0LonVgQAv8cQl3rgP2ufnINqEqpUGzE0AGrRNDaTUfOvokKD3MNPGYol
A3Ga3W3TSWMM3+iFUuw66/iXGKMpkDhpAi1htTwi31R7H3uCA0fK9CiyepKOzQya
LIw1mWLtrArMB53aWTVLn0z0pQK4/kAQkFOkuJBjvut8ds/MLgX5qdnwR/ydAdgE
atfTNQEEAJnicjBprzlo+YXYJoNojlvnKxuC2VEyq98QzrMzOtcAGVi7luYCf9IC
7kwKPKgWSutxW17w3laP2X2nFzGygwTBZLENnBF1+OHQf8phV5jCYTeHUloxXcII
gKrQcQrbIJsYVZ+eUuZktYCoHqNW0aIwMl2wNsgbTwkYgZ5e0SyJABEBAAEAA/0R
nStf4kD1y8+/48Foyjd9SoUTJZOVaTzmEAjw1xzVjkPYdcRseH3D/qTjHErcHI0f
zP5d7Cku9FE7Qjxz6lB3tezzDY+xXHmccB7veUFmNH0QI/toyMp0tgUpwxXidZ+K
PJwoXVGdQkCvp6FL8IvIFF0ZNbUiJ9th7wOYFzJpzwIAwS5XrUM0VMzUhZ/RDI9k
pWFOlh4drSqCPDvbgUNFwO5EugfXgsnfiropEg0o/Gtt19QW22zRWC/qIe3iwAFk
ZwIAy+zOptgXDbiNwPlYAjegwkPisg0eSmG2Xw0bD8imzDezNFTy+KRBz0A6yin+
1myqyxhziyY2aJDJU4UJsR7RjwIAhrKxzr+JUIRTh/8LV5j+aYfVvzoMpNioPq45
b00ZSQpl5O7Yqc6Lo3puaCc/ExMATssxr486s4qsK5m/lNEszJ6YiLYEGAEKACAW
IQTeTlGQHBWEG0mnnv5x0ajdZcrQugUCatfTNQIbDAAKCRBx0ajdZcrQuonoA/9Y
xZ2RD21yzuAC5i2j+yyyqL/d1dwAAdbWXpha0dlPbvELHMUCZO9CXuku4kO+ye/L
sXEz2yrRo+4pNpIx9aVIxpu+BoVtMweoFb6XOgQwhm1mCywXRuqUImYCRlrI0WYB
kNl6eEN+fsTGbm+i2S1SGleCuh6yLEONt4TkhMjYIg==
=u2L9
-----END PGP PRIVATE KEY BLOCK-----";

    pub static V4_SHORT_EXPIRY: Lazy<Fixture> = Lazy::new(|| {
        let base = &*V4_SIGN;
        let created = u64::from(base.secret.primary_key.created_at().as_secs());
        let out = update_key_expiration_internal(0, &base.secret_armored, None, created + 1, None)
            .expect("short-expiry key fixture");
        fixture_from(reparse(&out.secret))
    });
}

// ---------------------------------------------------------------------------
// FFI callback stubs
// ---------------------------------------------------------------------------

/// `extern "C"` stand-ins for the callbacks the C++ core supplies.
///
/// Several FFI entry points take these as *non-nullable* function pointers
/// (`GfrPublicKeyFetchCb`, not `Option<...>`), so there is no "pass null" case
/// for them — a stub is mandatory to exercise those paths at all.
///
/// Every stub that hands memory to the engine allocates it the way the C++
/// side does, so the engine's matching free (routed to the host stubs at the
/// top of this file) reclaims it exactly: `CString::into_raw` for C strings,
/// `Box<[u8]>::into_raw` for length-delimited buffers.
pub mod cb {
    use std::cell::Cell;
    use std::ffi::{CString, c_char, c_void};

    use crate::types::{GfrPassphraseState, GfrPasswordFetchStatus};

    /// The passphrase every `pwd_correct`-style stub hands back, and the one
    /// keys generated in tests are locked with.
    pub const CORRECT_PASSPHRASE: &str = "test-passphrase";

    thread_local! {
        /// Counts how many times a prompting callback was invoked on this
        /// thread, so cache hits can be told apart from re-prompts. The engine
        /// prompts on the calling thread; a process-wide counter was bumped by
        /// tests running in parallel and made prompt-count assertions flaky.
        static PROMPT_COUNT: Cell<usize> = const { Cell::new(0) };
    }

    pub fn reset_prompt_count() {
        PROMPT_COUNT.with(|c| c.set(0));
    }

    pub fn prompt_count() -> usize {
        PROMPT_COUNT.with(Cell::get)
    }

    fn provide(out_pwd: *mut *mut u8, out_status: *mut GfrPasswordFetchStatus, pw: &[u8]) -> i32 {
        let (ptr, len) = super::leak_as_c_buffer(pw);
        unsafe {
            *out_pwd = ptr;
            *out_status = GfrPasswordFetchStatus::Provided;
        }
        len as i32
    }

    /// Supplies [`CORRECT_PASSPHRASE`].
    pub extern "C" fn pwd_correct(
        _channel: i32,
        _state: GfrPassphraseState,
        out_pwd: *mut *mut u8,
        out_status: *mut GfrPasswordFetchStatus,
        _user_data: *mut c_void,
    ) -> i32 {
        PROMPT_COUNT.with(|c| c.set(c.get() + 1));
        provide(out_pwd, out_status, CORRECT_PASSPHRASE.as_bytes())
    }

    /// Supplies the passphrase the committed corpus keys use.
    pub extern "C" fn pwd_corpus(
        _channel: i32,
        _state: GfrPassphraseState,
        out_pwd: *mut *mut u8,
        out_status: *mut GfrPasswordFetchStatus,
        _user_data: *mut c_void,
    ) -> i32 {
        PROMPT_COUNT.with(|c| c.set(c.get() + 1));
        provide(out_pwd, out_status, super::corpus::CORPUS_PASSPHRASE)
    }

    /// Supplies a passphrase that will not unlock anything.
    pub extern "C" fn pwd_wrong(
        _channel: i32,
        _state: GfrPassphraseState,
        out_pwd: *mut *mut u8,
        out_status: *mut GfrPasswordFetchStatus,
        _user_data: *mut c_void,
    ) -> i32 {
        PROMPT_COUNT.with(|c| c.set(c.get() + 1));
        provide(out_pwd, out_status, b"definitely-not-the-passphrase")
    }

    /// The user pressed Cancel. Must surface as `ErrorCanceled`, not a
    /// generic failure.
    pub extern "C" fn pwd_cancelled(
        _channel: i32,
        _state: GfrPassphraseState,
        _out_pwd: *mut *mut u8,
        out_status: *mut GfrPasswordFetchStatus,
        _user_data: *mut c_void,
    ) -> i32 {
        PROMPT_COUNT.with(|c| c.set(c.get() + 1));
        unsafe { *out_status = GfrPasswordFetchStatus::Cancelled };
        0
    }

    /// No passphrase could be obtained (timeout, no provider, ...).
    pub extern "C" fn pwd_failed(
        _channel: i32,
        _state: GfrPassphraseState,
        _out_pwd: *mut *mut u8,
        out_status: *mut GfrPasswordFetchStatus,
        _user_data: *mut c_void,
    ) -> i32 {
        PROMPT_COUNT.with(|c| c.set(c.get() + 1));
        unsafe { *out_status = GfrPasswordFetchStatus::Failed };
        0
    }

    /// Writes no status at all: must be treated as a failure rather than
    /// mistaken for success.
    pub extern "C" fn pwd_silent(
        _channel: i32,
        _state: GfrPassphraseState,
        _out_pwd: *mut *mut u8,
        _out_status: *mut GfrPasswordFetchStatus,
        _user_data: *mut c_void,
    ) -> i32 {
        PROMPT_COUNT.with(|c| c.set(c.get() + 1));
        0
    }

    // `pubkey_fetch` below answers from this thread-local override, so a test
    // can decide which certificate the engine will "find" for an issuer.
    thread_local! {
        static PUBKEY_ANSWER: std::cell::RefCell<Option<String>> =
            const { std::cell::RefCell::new(None) };
    }

    /// Make [`pubkey_fetch`] answer with `block` for the current thread.
    pub fn set_pubkey_answer(block: &str) {
        PUBKEY_ANSWER.with(|a| *a.borrow_mut() = Some(block.to_string()));
    }

    /// Make [`pubkey_fetch`] answer "key not found".
    pub fn clear_pubkey_answer() {
        PUBKEY_ANSWER.with(|a| *a.borrow_mut() = None);
    }

    pub extern "C" fn pubkey_fetch(_issuer: *const c_char, _user_data: *mut c_void) -> *mut c_char {
        PUBKEY_ANSWER.with(|a| match a.borrow().as_deref() {
            Some(block) => CString::new(block).unwrap_or_default().into_raw(),
            None => std::ptr::null_mut(),
        })
    }

    /// Always answers "key not found".
    pub extern "C" fn pubkey_none(_issuer: *const c_char, _user_data: *mut c_void) -> *mut c_char {
        std::ptr::null_mut()
    }

    /// Answers with a block that is not a key at all.
    pub extern "C" fn pubkey_garbage(
        _issuer: *const c_char,
        _user_data: *mut c_void,
    ) -> *mut c_char {
        CString::new("this is not an OpenPGP certificate")
            .expect("no interior NUL")
            .into_raw()
    }

    thread_local! {
        static SECKEY_ANSWER: std::cell::RefCell<Option<String>> =
            const { std::cell::RefCell::new(None) };
    }

    /// Make [`seckey_fetch`] answer with `block` for the current thread.
    pub fn set_seckey_answer(block: &str) {
        SECKEY_ANSWER.with(|a| *a.borrow_mut() = Some(block.to_string()));
    }

    pub fn clear_seckey_answer() {
        SECKEY_ANSWER.with(|a| *a.borrow_mut() = None);
    }

    pub extern "C" fn seckey_fetch(_key_id: *const c_char, _user_data: *mut c_void) -> *mut c_char {
        SECKEY_ANSWER.with(|a| match a.borrow().as_deref() {
            Some(block) => CString::new(block).unwrap_or_default().into_raw(),
            None => std::ptr::null_mut(),
        })
    }

    thread_local! {
        static SECKEYS_BY_ID: std::cell::RefCell<Vec<(Vec<String>, String)>> =
            const { std::cell::RefCell::new(Vec::new()) };
    }

    /// Make [`seckey_by_id`] answer each lookup with whichever of `blocks`
    /// has a component whose Key ID or fingerprint is the one asked for,
    /// like a host keyring holding several secret keys.
    pub fn set_seckey_keyring(blocks: &[&str]) {
        use pgp::composed::{Deserializable, SignedSecretKey};
        use pgp::types::KeyDetails;
        let entries = blocks
            .iter()
            .map(|block| {
                let key = SignedSecretKey::from_string(block)
                    .expect("keyring block")
                    .0;
                let mut ids = vec![
                    key.primary_key.legacy_key_id().to_string(),
                    key.primary_key.fingerprint().to_string(),
                ];
                for sub in &key.secret_subkeys {
                    ids.push(sub.key.legacy_key_id().to_string());
                    ids.push(sub.key.fingerprint().to_string());
                }
                (ids, block.to_string())
            })
            .collect();
        SECKEYS_BY_ID.with(|k| *k.borrow_mut() = entries);
    }

    pub extern "C" fn seckey_by_id(key_id: *const c_char, _user_data: *mut c_void) -> *mut c_char {
        let wanted = unsafe { std::ffi::CStr::from_ptr(key_id) }.to_string_lossy();
        SECKEYS_BY_ID.with(|k| {
            k.borrow()
                .iter()
                .find(|(ids, _)| ids.iter().any(|id| id.eq_ignore_ascii_case(&wanted)))
                .map(|(_, block)| CString::new(block.as_str()).unwrap_or_default().into_raw())
                .unwrap_or(std::ptr::null_mut())
        })
    }

    /// Always answers "secret key not found".
    pub extern "C" fn seckey_none(_key_id: *const c_char, _user_data: *mut c_void) -> *mut c_char {
        std::ptr::null_mut()
    }
}

// ---------------------------------------------------------------------------
// Hand-crafted packet vectors
// ---------------------------------------------------------------------------

/// Byte-level builders for OpenPGP packets, used to probe the parser at the
/// edges of RFC 9580's framing rules where no `sq`/`gpg` vector can reach.
///
/// The positive cases are cross-checked against rPGP's own decoder
/// (`PacketHeader::try_from_reader`); the negative cases assert only the
/// *outcome class* (an error, and no panic), never a specific
/// `pgp::errors::Error` variant, because those are not a stable interface.
pub mod packets {
    use flate2::Compression;
    use flate2::write::ZlibEncoder;
    use std::io::Write;

    /// New-format (RFC 9580 §4.2.1) header with the body length forced into a
    /// specific encoding width, independent of what the value would minimally
    /// require. That is what lets a test probe the 191/192/8383/8384
    /// boundaries and the non-minimal encodings on either side of them.
    ///
    /// `octets` must be 1, 2 or 5.
    pub fn new_hdr_forced(tag: u8, len: u32, octets: u8) -> Vec<u8> {
        let mut v = vec![0xC0 | (tag & 0x3F)];
        match octets {
            // §4.2.1.1 -- a single octet below 192.
            1 => v.push(u8::try_from(len).expect("1-octet length must fit in a u8")),
            // §4.2.1.2 -- first octet in 192..=223.
            2 => {
                let x = len - 192;
                v.push(((x >> 8) + 192) as u8);
                v.push((x & 0xFF) as u8);
            }
            // §4.2.1.3 -- 0xFF followed by a 4-octet scalar.
            5 => {
                v.push(0xFF);
                v.extend_from_slice(&len.to_be_bytes());
            }
            _ => panic!("octets must be 1, 2 or 5"),
        }
        v
    }

    /// The same header, encoded by rPGP itself — the authority to compare
    /// [`new_hdr_forced`] against.
    pub fn new_hdr_via_rpgp(tag: pgp::types::Tag, length: pgp::types::PacketLength) -> Vec<u8> {
        use pgp::ser::Serialize;
        let mut v = Vec::new();
        pgp::packet::PacketHeader::from_parts(pgp::types::PacketHeaderVersion::New, tag, length)
            .expect("valid header parts")
            .to_writer(&mut v)
            .expect("header serialises");
        v
    }

    /// Legacy-format (§4.2.2) header. `len_type`: 0 = 1-octet, 1 = 2-octet,
    /// 2 = 4-octet, 3 = indeterminate (body runs to EOF).
    pub fn old_hdr(tag: u8, len_type: u8, len: u32) -> Vec<u8> {
        let mut v = vec![0x80 | ((tag & 0x0F) << 2) | (len_type & 0x03)];
        match len_type {
            0 => v.push(len as u8),
            1 => v.extend_from_slice(&(len as u16).to_be_bytes()),
            2 => v.extend_from_slice(&len.to_be_bytes()),
            3 => {}
            _ => panic!("len_type is two bits"),
        }
        v
    }

    /// A complete new-format packet with a minimally encoded length.
    pub fn packet(tag: u8, body: &[u8]) -> Vec<u8> {
        let len = body.len() as u32;
        let octets = if len < 192 {
            1
        } else if len < 8384 {
            2
        } else {
            5
        };
        let mut v = new_hdr_forced(tag, len, octets);
        v.extend_from_slice(body);
        v
    }

    /// A packet whose declared length is longer than the body that follows —
    /// the truncation case from §4.1 ("a parser MUST abort without writing
    /// outside the indicated range").
    pub fn declared_len_larger_than_body(tag: u8, declared: u32, body: &[u8]) -> Vec<u8> {
        let mut v = new_hdr_forced(tag, declared, 5);
        v.extend_from_slice(body);
        v
    }

    /// Partial body lengths (§4.2.1.4). Each chunk's length MUST be a power of
    /// two, and the final segment MUST use a regular length header.
    pub fn partial_body(tag: u8, chunks: &[&[u8]], last: &[u8]) -> Vec<u8> {
        let mut v = vec![0xC0 | (tag & 0x3F)];
        for chunk in chunks {
            let len = chunk.len() as u32;
            assert!(
                len.count_ones() == 1 && (1..=(1 << 30)).contains(&len),
                "a partial length must be a power of two from 1 to 2^30, got {len}"
            );
            v.push(0xE0 | (len.trailing_zeros() as u8));
            v.extend_from_slice(chunk);
        }
        // §4.2.1.4: "The last length header in the packet MUST NOT be a
        // Partial Body Length header."
        let len = last.len() as u32;
        if len < 192 {
            v.push(len as u8);
        } else {
            v.push(0xFF);
            v.extend_from_slice(&len.to_be_bytes());
        }
        v.extend_from_slice(last);
        v
    }

    /// A partial-length header whose length is deliberately *not* a power of
    /// two — which the encoding cannot express, so this emits the nearest
    /// illegal shape a hostile producer might.
    pub fn partial_body_not_power_of_two(tag: u8, body: &[u8]) -> Vec<u8> {
        let mut v = vec![0xC0 | (tag & 0x3F)];
        // 0xE0..0xFE encode 2^0..2^30; there is no encoding for 3 octets, so a
        // producer wanting 3 must lie about the length. Claim 4, supply 3.
        v.push(0xE2); // 2^2 == 4 octets
        v.extend_from_slice(&body[..body.len().min(3)]);
        v
    }

    /// A Multiprecision Integer (§3.2): a 2-octet big-endian *bit* count
    /// followed by the minimal big-endian integer.
    pub fn mpi(value: &[u8]) -> Vec<u8> {
        let first_nonzero = value.iter().position(|b| *b != 0);
        let Some(start) = first_nonzero else {
            // §3.2: "The string of octets [00 00] forms an MPI with the value 0."
            return vec![0x00, 0x00];
        };
        let trimmed = &value[start..];
        let bits = (trimmed.len() as u16 - 1) * 8 + (8 - trimmed[0].leading_zeros() as u16);
        let mut v = bits.to_be_bytes().to_vec();
        v.extend_from_slice(trimmed);
        v
    }

    /// An MPI with a bit count that disagrees with its body — §3.2 requires a
    /// v6 parser to "check that the encoded length matches the length starting
    /// from the most significant non-zero bit".
    pub fn mpi_with_bitcount(bits: u16, body: &[u8]) -> Vec<u8> {
        let mut v = bits.to_be_bytes().to_vec();
        v.extend_from_slice(body);
        v
    }

    /// The malformed example the RFC gives verbatim: "the MPI [00 02 01] is
    /// not formed correctly. It should be [00 01 01]."
    pub fn mpi_non_minimal() -> Vec<u8> {
        vec![0x00, 0x02, 0x01]
    }

    /// A signature subpacket (§5.2.3.7): length prefix, then the type octet
    /// with bit 7 as the critical flag, then the body.
    pub fn subpacket(critical: bool, typ: u8, body: &[u8]) -> Vec<u8> {
        let len = body.len() + 1; // the type octet counts, the length does not
        let mut v = Vec::new();
        if len < 192 {
            v.push(len as u8);
        } else if len < 8384 {
            let x = len - 192;
            v.push(((x >> 8) + 192) as u8);
            v.push((x & 0xFF) as u8);
        } else {
            v.push(0xFF);
            v.extend_from_slice(&(len as u32).to_be_bytes());
        }
        v.push(if critical { typ | 0x80 } else { typ & 0x7F });
        v.extend_from_slice(body);
        v
    }

    /// A Marker packet (§5.8): the three octets spelling "PGP". MUST be
    /// ignored when received.
    pub fn marker_packet() -> Vec<u8> {
        packet(10, &[0x50, 0x47, 0x50])
    }

    /// A Padding packet (§5.14) of `n` bytes. MUST be ignored when received.
    pub fn padding_packet(n: usize) -> Vec<u8> {
        packet(21, &vec![0x5A; n])
    }

    /// A packet with an unassigned type ID. Tags 0..=39 are critical and 40..=63
    /// are not (§4.3), which is the distinction these probe.
    pub fn unknown_packet(tag: u8, body: &[u8]) -> Vec<u8> {
        packet(tag, body)
    }

    /// A Literal Data packet (§5.9).
    pub fn literal(format: u8, filename: &[u8], timestamp: u32, data: &[u8]) -> Vec<u8> {
        let mut body = vec![format];
        body.push(filename.len() as u8);
        body.extend_from_slice(filename);
        body.extend_from_slice(&timestamp.to_be_bytes());
        body.extend_from_slice(data);
        packet(11, &body)
    }

    /// A Compressed Data packet (§5.6) using ZLIB (algorithm 2).
    pub fn compressed_zlib(inner: &[u8]) -> Vec<u8> {
        let mut enc = ZlibEncoder::new(Vec::new(), Compression::best());
        enc.write_all(inner).expect("compress");
        let deflated = enc.finish().expect("finish");
        let mut body = vec![2u8]; // ZLIB
        body.extend_from_slice(&deflated);
        packet(8, &body)
    }

    /// `layers` nested Compressed Data packets wrapping `inner`.
    pub fn nested_compressed(layers: usize, inner: &[u8]) -> Vec<u8> {
        let mut out = inner.to_vec();
        for _ in 0..layers {
            out = compressed_zlib(&out);
        }
        out
    }

    /// A high-ratio compression bomb: `mib` mebibytes of zeros inside a single
    /// Compressed Data packet, which deflates to a few kilobytes.
    pub fn compression_bomb(mib: usize) -> Vec<u8> {
        let literal = literal(b'b', b"", 0, &vec![0u8; mib * 1024 * 1024]);
        compressed_zlib(&literal)
    }

    /// Truncate `data` after `n` bytes.
    pub fn truncate_at(data: &[u8], n: usize) -> Vec<u8> {
        data[..n.min(data.len())].to_vec()
    }

    /// Flip one bit, for tamper-detection tests.
    pub fn flip_bit(data: &[u8], byte: usize, bit: u8) -> Vec<u8> {
        let mut v = data.to_vec();
        v[byte] ^= 1 << bit;
        v
    }
}

// ---------------------------------------------------------------------------
// Assertions
// ---------------------------------------------------------------------------

/// Assertion helpers shared across test modules.
pub mod assert {
    use pgp::composed::{Deserializable, SignedSecretKey};
    use pgp::types::KeyDetails;

    use crate::crypto::SignatureResultInternal;
    use crate::types::{GfrSignatureStatus, GfrStatus};

    /// RFC 9580 §6.1: v6 material MUST NOT carry a CRC24 footer, and this
    /// engine never emits one at all (see [`crate::utils::armor_opts`]).
    pub fn armor_has_crc24(armored: &str) {
        assert!(
            armored
                .lines()
                .any(|line| line.starts_with('=') && line.len() == 5),
            "v4-only armor must carry a CRC24 footer:\n{armored}"
        );
    }

    pub fn armor_has_no_crc24(armored: &str) {
        for line in armored.lines() {
            assert!(
                !(line.starts_with('=') && line.len() == 5),
                "armor must not contain a CRC24 footer, found {line:?}"
            );
        }
    }

    /// A signature is valid for this engine iff the policy gate left it
    /// `Valid` — a verifying signature that fails the §9.5 weak-hash or
    /// §5.2.3.18 expiry gate is deliberately *not* valid.
    pub fn is_valid(sig: &SignatureResultInternal) {
        assert_eq!(
            sig.status,
            GfrSignatureStatus::Valid,
            "expected a valid signature from {}, got {:?}",
            sig.fpr,
            sig.status
        );
    }

    pub fn is_not_valid(sig: &SignatureResultInternal) {
        assert_ne!(
            sig.status,
            GfrSignatureStatus::Valid,
            "signature from {} must not be reported valid",
            sig.fpr
        );
    }

    /// Exactly one of `sigs` is `Valid`. This is the shape that the B4
    /// per-index attribution fix is about: a sibling signature naming the same
    /// certificate must not inherit its neighbour's verdict.
    pub fn exactly_one_valid(sigs: &[SignatureResultInternal]) {
        let valid = sigs
            .iter()
            .filter(|s| s.status == GfrSignatureStatus::Valid)
            .count();
        assert_eq!(
            valid,
            1,
            "expected exactly one valid signature, got {valid} of {} ({:?})",
            sigs.len(),
            sigs.iter().map(|s| s.status).collect::<Vec<_>>()
        );
    }

    /// No signature in the set is valid. The set must not be empty: a
    /// signature that was never found would otherwise pass vacuously.
    pub fn none_valid(sigs: &[SignatureResultInternal]) {
        assert!(
            !sigs.is_empty(),
            "expected at least one signature, got none"
        );
        for sig in sigs {
            is_not_valid(sig);
        }
    }

    /// Exactly one signature was found, and the policy gate gave it `want`.
    pub fn only_status(sigs: &[SignatureResultInternal], want: GfrSignatureStatus) {
        assert_eq!(
            sigs.iter().map(|s| s.status).collect::<Vec<_>>(),
            vec![want],
            "expected exactly one {want:?} signature"
        );
    }

    pub fn status_is(got: GfrStatus, want: GfrStatus) {
        assert_eq!(got, want, "expected {want:?}, got {got:?}");
    }

    /// Assert an armored secret key round-trips through parse -> armor -> parse
    /// with a stable fingerprint.
    pub fn roundtrips_through_armor(armored: &str) -> SignedSecretKey {
        let (key, _) = SignedSecretKey::from_string(armored).expect("armored secret key parses");
        let again = key
            .to_armored_string(crate::utils::armor_opts())
            .expect("re-armors");
        let (key2, _) = SignedSecretKey::from_string(&again).expect("re-armored key parses");
        assert_eq!(
            key.primary_key.fingerprint().to_string(),
            key2.primary_key.fingerprint().to_string(),
            "fingerprint must survive an armor round-trip"
        );
        key
    }
}

// ---------------------------------------------------------------------------
// SEIPD fixtures
// ---------------------------------------------------------------------------

/// Deterministic, reusable SEIPD v1 / v2 regression fixtures.
///
/// Payloads come from a seeded generator and are streamed, never held in
/// memory, so the same helpers serve a 1 KiB unit test and a 5 GiB large-file
/// test. A decrypted file is checked against the generator byte for byte.
pub mod seipd {
    use std::io::Read;
    use std::path::Path;

    use super::keys;

    /// A deterministic pseudo-random byte stream of exactly `len` bytes. The
    /// bytes depend only on `(len, seed)`, never on how the reader is called.
    pub struct Payload {
        remaining: u64,
        state: u64,
        word: [u8; 8],
        /// How many bytes of `word` are already handed out (8 = none left).
        used: usize,
    }

    impl Payload {
        pub fn new(len: u64, seed: u64) -> Self {
            Payload {
                remaining: len,
                // xorshift must not start at zero.
                state: seed ^ 0x9E37_79B9_7F4A_7C15,
                word: [0; 8],
                used: 8,
            }
        }
    }

    impl Read for Payload {
        fn read(&mut self, buf: &mut [u8]) -> std::io::Result<usize> {
            let n = buf
                .len()
                .min(self.remaining.min(usize::MAX as u64) as usize);
            for out in &mut buf[..n] {
                if self.used == 8 {
                    self.state ^= self.state << 13;
                    self.state ^= self.state >> 7;
                    self.state ^= self.state << 17;
                    self.word = self.state.to_le_bytes();
                    self.used = 0;
                }
                *out = self.word[self.used];
                self.used += 1;
            }
            self.remaining -= n as u64;
            Ok(n)
        }
    }

    /// `len` payload bytes in memory, for small tests.
    pub fn bytes(len: usize, seed: u64) -> Vec<u8> {
        let mut out = Vec::with_capacity(len);
        Payload::new(len as u64, seed)
            .read_to_end(&mut out)
            .expect("generator");
        out
    }

    /// Assert the file at `path` holds exactly the payload `(len, seed)`,
    /// streaming both sides.
    pub fn assert_file_is_payload(path: &Path, len: u64, seed: u64) {
        let mut file = std::io::BufReader::with_capacity(
            1 << 20,
            std::fs::File::open(path).expect("open decrypted output"),
        );
        let mut want = Payload::new(len, seed);
        let mut a = vec![0u8; 1 << 20];
        let mut b = vec![0u8; 1 << 20];
        let mut offset = 0u64;
        loop {
            let n = want.read(&mut a).unwrap();
            file.read_exact(&mut b[..n])
                .unwrap_or_else(|e| panic!("output is short at offset {offset}: {e}"));
            assert!(
                a[..n] == b[..n],
                "output differs within 1 MiB of offset {offset}"
            );
            offset += n as u64;
            if n == 0 {
                break;
            }
        }
        let mut rest = [0u8; 1];
        assert_eq!(
            file.read(&mut rest).unwrap(),
            0,
            "output is longer than {len} bytes"
        );
    }

    /// Encrypt the payload `(len, seed)` to `recipient` into `path`, streaming,
    /// optionally signed by the secret key `signer`. Our engine chooses the
    /// container from the recipient (see `encrypt::select_container`).
    pub fn encrypt_payload_to_file(
        path: &Path,
        recipient: &keys::Fixture,
        len: u64,
        seed: u64,
        signer: Option<&keys::Fixture>,
    ) {
        let out = std::io::BufWriter::new(std::fs::File::create(path).expect("create ciphertext"));
        let signers: Vec<&str> = signer.iter().map(|k| k.secret_armored.as_str()).collect();
        crate::crypto::encrypt_and_sign_stream_internal(
            0,
            "",
            Payload::new(len, seed),
            out,
            &[&recipient.public_armored],
            &signers,
            None,
            false,
        )
        .expect("encrypt payload");
    }

    /// The SEIPD container version of a binary message, read from its packet
    /// headers only: ESK packets are skipped by length and the SEIPD body is
    /// never loaded, so this is cheap on a multi-GB file.
    pub fn seipd_version_of_file(path: &Path) -> u32 {
        let mut r = std::io::BufReader::new(std::fs::File::open(path).expect("open"));
        let mut byte = || {
            let mut b = [0u8; 1];
            r.read_exact(&mut b).expect("truncated packet header");
            b[0]
        };
        loop {
            let ctb = byte();
            assert!(ctb & 0x80 != 0, "not a binary OpenPGP packet stream");
            let (tag, len) = if ctb & 0x40 != 0 {
                // RFC 9580 §4.2.1 OpenPGP format.
                let o1 = byte() as u64;
                let len = match o1 {
                    0..=191 => o1,
                    192..=223 => ((o1 - 192) << 8) + byte() as u64 + 192,
                    255 => (0..4).fold(0u64, |acc, _| (acc << 8) | byte() as u64),
                    _ => 0, // partial length: only the SEIPD body uses it
                };
                (ctb & 0x3F, len)
            } else {
                // §4.2.2 legacy format.
                let octets = [1usize, 2, 4, 0][(ctb & 0x03) as usize];
                let len = (0..octets).fold(0u64, |acc, _| (acc << 8) | byte() as u64);
                ((ctb >> 2) & 0x0F, len)
            };
            match tag {
                18 => return byte() as u32,
                1 | 3 => {
                    for _ in 0..len {
                        byte();
                    }
                }
                other => panic!("unexpected packet (tag {other}) before the SEIPD"),
            }
        }
    }

    /// A message in the format every GpgFrontend rPGP release before 0.3.2
    /// wrote to *any* recipient, v6 and post-quantum keys included: SEIPD v1
    /// with AES-256, v3 PKESKs and 512 KiB partial chunks. Today's engine only
    /// writes v1 when a recipient needs it, so legacy files are rebuilt here
    /// to keep their decryption under test.
    pub fn legacy_v1_message(
        recipient: &keys::Fixture,
        payload: impl Read + Send + Sync,
    ) -> Vec<u8> {
        let mut out = Vec::new();
        write_legacy_v1(recipient, payload, &mut out);
        out
    }

    /// [`legacy_v1_message`], streamed into `out` (for multi-GB files).
    pub fn write_legacy_v1(
        recipient: &keys::Fixture,
        payload: impl Read + Send + Sync,
        out: impl std::io::Write,
    ) {
        use pgp::composed::{Deserializable, MessageBuilder, SignedPublicKey};
        use pgp::crypto::sym::SymmetricKeyAlgorithm;
        use pgp::types::KeyDetails;
        let cert = SignedPublicKey::from_string(&recipient.public_armored)
            .expect("parses")
            .0;
        let subkey = cert
            .public_subkeys
            .iter()
            .find(|s| s.key.algorithm().can_encrypt())
            .expect("an encryption subkey");
        let mut rng = rand::thread_rng();
        let mut builder = MessageBuilder::from_reader("", payload);
        builder.partial_chunk_size(512 * 1024).expect("chunk size");
        let mut builder = builder.seipd_v1(&mut rng, SymmetricKeyAlgorithm::AES256);
        builder.encrypt_to_key(&mut rng, subkey).expect("v3 PKESK");
        builder.to_writer(&mut rng, out).expect("write");
    }

    /// A message encrypted both to `recipient`'s encryption subkey (PKESK) and
    /// with `password` (SKESK), in a SEIPD v2 (AES-256/OCB) container when
    /// `v2`, else SEIPD v1 (AES-256). GF never writes such messages; other
    /// implementations do (RFC 9580 §3.7.2.2).
    pub fn mixed_message(
        recipient: &keys::Fixture,
        password: &str,
        payload: &[u8],
        v2: bool,
    ) -> Vec<u8> {
        use pgp::composed::{Deserializable, MessageBuilder, SignedPublicKey};
        use pgp::crypto::aead::{AeadAlgorithm, ChunkSize};
        use pgp::crypto::hash::HashAlgorithm;
        use pgp::crypto::sym::SymmetricKeyAlgorithm;
        use pgp::types::{KeyDetails, Password, StringToKey};
        let cert = SignedPublicKey::from_string(&recipient.public_armored)
            .expect("parses")
            .0;
        let subkey = cert
            .public_subkeys
            .iter()
            .find(|s| s.key.algorithm().can_encrypt())
            .expect("an encryption subkey");
        let pw = Password::from(password);
        let mut rng = rand::thread_rng();
        let builder = MessageBuilder::from_bytes("", payload.to_vec());
        if v2 {
            let mut b = builder.seipd_v2(
                &mut rng,
                SymmetricKeyAlgorithm::AES256,
                AeadAlgorithm::Ocb,
                ChunkSize::default(),
            );
            b.encrypt_to_key(&mut rng, subkey).expect("v6 PKESK");
            // Cheap Argon2 (t=1, p=1, 2^8 KiB): tests, not storage.
            let s2k = StringToKey::new_argon2(&mut rng, 1, 1, 8);
            b.encrypt_with_password(&mut rng, s2k, &pw)
                .expect("v6 SKESK");
            b.to_vec(&mut rng).expect("write")
        } else {
            let mut b = builder.seipd_v1(&mut rng, SymmetricKeyAlgorithm::AES256);
            b.encrypt_to_key(&mut rng, subkey).expect("v3 PKESK");
            let s2k = StringToKey::new_iterated(&mut rng, HashAlgorithm::Sha256, 96);
            b.encrypt_with_password(s2k, &pw).expect("v4 SKESK");
            b.to_vec(&mut rng).expect("write")
        }
    }

    /// A v3 PKESK to `recipient` followed by a SEIPD v1 whose decrypted body
    /// is exactly `inner`: arbitrary, possibly malformed, packets that only
    /// become visible after the integrity check passed.
    pub fn v1_with_inner(recipient: &keys::Fixture, inner: &[u8]) -> Vec<u8> {
        v1_with_inner_cipher(
            recipient,
            inner,
            pgp::crypto::sym::SymmetricKeyAlgorithm::AES256,
        )
    }

    /// [`v1_with_inner`] with the session key for `alg`, whatever the
    /// recipient's algorithm allows: rPGP's low-level PKESK builder does not
    /// restrict the cipher, which lets a test produce what a non-conforming
    /// sender would.
    pub fn v1_with_inner_cipher(
        recipient: &keys::Fixture,
        inner: &[u8],
        alg: pgp::crypto::sym::SymmetricKeyAlgorithm,
    ) -> Vec<u8> {
        use pgp::composed::{Deserializable, SignedPublicKey};
        use pgp::packet::{Packet, PublicKeyEncryptedSessionKey, SymEncryptedProtectedData};
        use pgp::ser::Serialize;
        use pgp::types::KeyDetails;
        let cert = SignedPublicKey::from_string(&recipient.public_armored)
            .expect("parses")
            .0;
        let subkey = cert
            .public_subkeys
            .iter()
            .find(|s| s.key.algorithm().can_encrypt())
            .expect("an encryption subkey");
        let mut rng = rand::thread_rng();
        let session_key = alg.new_session_key(&mut rng);
        let pkesk =
            PublicKeyEncryptedSessionKey::from_session_key_v3(&mut rng, &session_key, alg, subkey)
                .expect("v3 PKESK");
        let seipd =
            SymEncryptedProtectedData::encrypt_seipdv1(&mut rng, alg, session_key.as_ref(), inner)
                .expect("SEIPD v1");
        let mut out = Packet::from(pkesk).to_bytes().expect("PKESK bytes");
        out.extend(Packet::from(seipd).to_bytes().expect("SEIPD bytes"));
        out
    }

    /// The message (in memory) encrypted to the certificate `recipient`.
    pub fn message(recipient: &str, payload: &[u8], signer: Option<&keys::Fixture>) -> Vec<u8> {
        let signers: Vec<&str> = signer.iter().map(|k| k.secret_armored.as_str()).collect();
        let mut out = Vec::new();
        crate::crypto::encrypt_and_sign_stream_internal(
            0,
            "",
            payload,
            &mut out,
            &[recipient],
            &signers,
            None,
            false,
        )
        .expect("encrypt");
        out
    }

    /// Flip every bit of the byte at `index` (negative counts from the end).
    pub fn flipped(data: &[u8], index: isize) -> Vec<u8> {
        let mut out = data.to_vec();
        let i = if index < 0 {
            (out.len() as isize + index) as usize
        } else {
            index as usize
        };
        out[i] ^= 0xFF;
        out
    }

    /// Flip the byte at `offset_from_end` of the file at `path` in place.
    pub fn flip_in_file(path: &Path, offset_from_end: u64) {
        use std::io::{Seek, SeekFrom, Write};
        let mut f = std::fs::OpenOptions::new()
            .read(true)
            .write(true)
            .open(path)
            .expect("open");
        let len = f.metadata().unwrap().len();
        let pos = len - offset_from_end;
        f.seek(SeekFrom::Start(pos)).unwrap();
        let mut byte = [0u8; 1];
        f.read_exact(&mut byte).unwrap();
        f.seek(SeekFrom::Start(pos)).unwrap();
        f.write_all(&[byte[0] ^ 0xFF]).unwrap();
    }
}

// ---------------------------------------------------------------------------
// External OpenPGP implementations
// ---------------------------------------------------------------------------

/// Drivers for GnuPG (`gpg`) and Sequoia (`sq`), for interop tests.
///
/// These checks are security relevant, so a missing tool is a **failure**, not
/// a silent skip. Set `GF_ALLOW_MISSING_INTEROP_TOOLS=1` to downgrade it to a
/// loudly logged skip on a machine that genuinely lacks one.
pub mod interop {
    use std::ffi::OsStr;
    use std::io::Write;
    use std::path::{Path, PathBuf};
    use std::process::{Command, Output, Stdio};

    pub const ALLOW_MISSING: &str = "GF_ALLOW_MISSING_INTEROP_TOOLS";

    /// Whether `tool` can be spawned. Only spawning is checked: `sq` has no
    /// `--version` flag and exits non-zero on it.
    fn spawnable(tool: &str) -> bool {
        Command::new(tool)
            .arg("--help")
            .stdout(Stdio::null())
            .stderr(Stdio::null())
            .status()
            .is_ok()
    }

    /// `true` if `tool` is available. Panics if it is not, unless
    /// [`ALLOW_MISSING`] is set, in which case it logs the skip and returns
    /// `false`.
    pub fn require(tool: &str) -> bool {
        if spawnable(tool) {
            return true;
        }
        if std::env::var(ALLOW_MISSING).as_deref() == Ok("1") {
            eprintln!("SKIPPED interop check: `{tool}` is not installed ({ALLOW_MISSING}=1)");
            return false;
        }
        panic!(
            "interop test needs `{tool}`, which is not installed; install it, or set \
             {ALLOW_MISSING}=1 to skip explicitly"
        );
    }

    fn check(out: Output, what: &str) -> Vec<u8> {
        assert!(
            out.status.success(),
            "{what} failed: {}",
            String::from_utf8_lossy(&out.stderr)
        );
        out.stdout
    }

    /// A throwaway GnuPG home with a freshly generated, unprotected default
    /// key (so its certificate is one GnuPG itself made).
    pub struct Gnupg {
        home: tempfile::TempDir,
        /// The generated key's public certificate, armored.
        pub cert: String,
        /// The generated key's fingerprint.
        pub fpr: String,
    }

    impl Gnupg {
        /// `None` only when `gpg` is missing and that was explicitly allowed.
        pub fn new() -> Option<Self> {
            if !require("gpg") {
                return None;
            }
            // A short path: gpg-agent's socket must fit in sun_path.
            let home = tempfile::Builder::new()
                .prefix("gf")
                .tempdir_in("/tmp")
                .or_else(|_| tempfile::tempdir())
                .expect("tempdir");
            #[cfg(unix)]
            {
                use std::os::unix::fs::PermissionsExt;
                let _ =
                    std::fs::set_permissions(home.path(), std::fs::Permissions::from_mode(0o700));
            }
            let mut this = Gnupg {
                home,
                cert: String::new(),
                fpr: String::new(),
            };
            check(
                this.gpg([
                    "--passphrase",
                    "",
                    "--quick-gen-key",
                    "GF Interop <gf-interop@example.test>",
                    "default",
                    "default",
                    "never",
                ]),
                "gpg --quick-gen-key",
            );
            this.cert = String::from_utf8(check(this.gpg(["--armor", "--export"]), "gpg --export"))
                .expect("armored");
            let listing = String::from_utf8(check(
                this.gpg(["--with-colons", "--list-secret-keys"]),
                "gpg --list-secret-keys",
            ))
            .unwrap();
            this.fpr = listing
                .lines()
                .find(|l| l.starts_with("fpr:"))
                .and_then(|l| l.split(':').nth(9))
                .expect("a fingerprint")
                .to_string();
            Some(this)
        }

        fn command(&self) -> Command {
            let mut cmd = Command::new("gpg");
            cmd.arg("--homedir").arg(self.home.path()).args([
                "--batch",
                "--yes",
                "--pinentry-mode",
                "loopback",
            ]);
            cmd
        }

        pub fn gpg<I, S>(&self, args: I) -> Output
        where
            I: IntoIterator<Item = S>,
            S: AsRef<OsStr>,
        {
            self.command().args(args).output().expect("run gpg")
        }

        /// Import a public certificate so it can be encrypted to.
        pub fn import(&self, armored_cert: &str) {
            let path = self.home.path().join("import.asc");
            std::fs::write(&path, armored_cert).unwrap();
            check(
                self.gpg([OsStr::new("--import"), path.as_os_str()]),
                "gpg --import",
            );
        }

        /// Encrypt `input` (streamed into gpg's stdin) to `recipient_fpr` into
        /// `out`. `compress` is a `--compress-algo` value; `sign` also signs
        /// with this home's own key.
        pub fn encrypt_stream(
            &self,
            recipient_fpr: &str,
            mut input: impl std::io::Read + Send + 'static,
            out: &Path,
            compress: &str,
            sign: bool,
        ) {
            let mut cmd = self.command();
            cmd.args(["--trust-model", "always", "--compress-algo", compress])
                .args(["--recipient", recipient_fpr, "--output"])
                .arg(out);
            if sign {
                cmd.args(["--passphrase", "", "--local-user", &self.fpr, "--sign"]);
            }
            let mut child = cmd
                .arg("--encrypt")
                .stdin(Stdio::piped())
                .stdout(Stdio::null())
                .stderr(Stdio::piped())
                .spawn()
                .expect("spawn gpg");
            let mut stdin = child.stdin.take().unwrap();
            let feeder = std::thread::spawn(move || {
                std::io::copy(&mut input, &mut stdin).expect("feed gpg");
                stdin.flush().unwrap();
            });
            let output = child.wait_with_output().expect("gpg");
            feeder.join().unwrap();
            check(output, "gpg --encrypt");
        }

        /// Encrypt `data` in memory; see [`Self::encrypt_stream`].
        pub fn encrypt(
            &self,
            recipient_fpr: &str,
            data: &[u8],
            compress: &str,
            sign: bool,
        ) -> Vec<u8> {
            let out = self.home.path().join("msg.pgp");
            self.encrypt_stream(
                recipient_fpr,
                std::io::Cursor::new(data.to_vec()),
                &out,
                compress,
                sign,
            );
            std::fs::read(&out).unwrap()
        }

        /// Decrypt `data` with this home's key.
        pub fn decrypt(&self, data: &[u8]) -> Vec<u8> {
            let msg = self.home.path().join("in.pgp");
            std::fs::write(&msg, data).unwrap();
            check(
                self.gpg([OsStr::new("--decrypt"), msg.as_os_str()]),
                "gpg --decrypt",
            )
        }

        /// Run `gpg --decrypt` with `password` as the message passphrase,
        /// returning the raw output so a refusal can be asserted too.
        pub fn decrypt_with_password(&self, password: &str, data: &[u8]) -> Output {
            let msg = self.home.path().join("in.pgp");
            std::fs::write(&msg, data).unwrap();
            self.gpg([
                OsStr::new("--passphrase"),
                OsStr::new(password),
                OsStr::new("--decrypt"),
                msg.as_os_str(),
            ])
        }

        /// Encrypt `data` both to `recipient_fpr` (PKESK) and with `password`
        /// (SKESK) in one message.
        pub fn encrypt_mixed(&self, recipient_fpr: &str, password: &str, data: &[u8]) -> Vec<u8> {
            let input = self.home.path().join("plain.bin");
            let out = self.home.path().join("mixed.pgp");
            std::fs::write(&input, data).unwrap();
            check(
                self.gpg([
                    OsStr::new("--trust-model"),
                    OsStr::new("always"),
                    OsStr::new("--passphrase"),
                    OsStr::new(password),
                    OsStr::new("--recipient"),
                    OsStr::new(recipient_fpr),
                    OsStr::new("--output"),
                    out.as_os_str(),
                    OsStr::new("--symmetric"),
                    OsStr::new("--encrypt"),
                    input.as_os_str(),
                ]),
                "gpg --symmetric --encrypt",
            );
            std::fs::read(&out).unwrap()
        }

        /// `gpg --verify` of a detached signature (`Some(data)`) or of an
        /// inline/cleartext message (`None`), against imported certificates.
        pub fn verify(&self, signature: &[u8], data: Option<&[u8]>) -> Output {
            let sig = self.home.path().join("sig.asc");
            std::fs::write(&sig, signature).unwrap();
            let mut args = vec![OsStr::new("--verify").to_os_string(), sig.into_os_string()];
            if let Some(data) = data {
                let path = self.home.path().join("signed.bin");
                std::fs::write(&path, data).unwrap();
                args.push(path.into_os_string());
            }
            self.gpg(args)
        }
    }

    impl Drop for Gnupg {
        fn drop(&mut self) {
            let _ = Command::new("gpgconf")
                .arg("--homedir")
                .arg(self.home.path())
                .args(["--kill", "gpg-agent"])
                .output();
        }
    }

    /// Sequoia's `sq`, stateless (no cert or key store).
    pub struct Sq {
        dir: tempfile::TempDir,
    }

    impl Sq {
        /// `None` only when `sq` is missing and that was explicitly allowed.
        pub fn new() -> Option<Self> {
            require("sq").then(|| Sq {
                dir: tempfile::tempdir().expect("tempdir"),
            })
        }

        fn file(&self, name: &str, contents: &[u8]) -> PathBuf {
            let path = self.dir.path().join(name);
            std::fs::write(&path, contents).unwrap();
            path
        }

        fn sq<I, S>(&self, args: I) -> Output
        where
            I: IntoIterator<Item = S>,
            S: AsRef<OsStr>,
        {
            Command::new("sq")
                .args(["--batch", "--cert-store=none", "--key-store=none"])
                .args(args)
                .output()
                .expect("run sq")
        }

        /// Encrypt `data` to the certificate `cert`; sq picks the container
        /// from the certificate's features. `signer` (secret key, armored, with
        /// `password`) also signs.
        pub fn encrypt(&self, cert: &str, data: &[u8], signer: Option<(&str, &[u8])>) -> Vec<u8> {
            let cert = self.file("cert.asc", cert.as_bytes());
            let input = self.file("plain.bin", data);
            let mut args: Vec<std::ffi::OsString> =
                vec!["encrypt".into(), "--for-file".into(), cert.into()];
            match signer {
                Some((key, password)) => {
                    args.push("--signer-file".into());
                    args.push(self.file("signer.asc", key.as_bytes()).into());
                    args.push("--password-file".into());
                    args.push(self.file("signer.pw", password).into());
                }
                None => args.push("--without-signature".into()),
            }
            args.push("--binary".into());
            args.push(input.into());
            check(self.sq(args), "sq encrypt")
        }

        /// Encrypt `data` both to the certificate `cert` (PKESK) and with
        /// `password` (SKESK) in one message.
        pub fn encrypt_mixed(&self, cert: &str, password: &str, data: &[u8]) -> Vec<u8> {
            let cert = self.file("cert.asc", cert.as_bytes());
            let pw = self.file("msg.pw", password.as_bytes());
            let input = self.file("plain.bin", data);
            check(
                self.sq([
                    OsStr::new("encrypt"),
                    OsStr::new("--for-file"),
                    cert.as_os_str(),
                    OsStr::new("--with-password-file"),
                    pw.as_os_str(),
                    OsStr::new("--without-signature"),
                    OsStr::new("--binary"),
                    input.as_os_str(),
                ]),
                "sq encrypt (mixed)",
            )
        }

        /// Decrypt `data` with the message `password` only.
        pub fn decrypt_with_password(&self, password: &str, data: &[u8]) -> Vec<u8> {
            let pw = self.file("msg.pw", password.as_bytes());
            let msg = self.file("msg.pgp", data);
            check(
                self.sq([
                    OsStr::new("decrypt"),
                    OsStr::new("--password-file"),
                    pw.as_os_str(),
                    msg.as_os_str(),
                ]),
                "sq decrypt (password)",
            )
        }

        /// `sq verify` against `cert`: a detached `signature` over `data`
        /// (`Some(data)`), or a cleartext signed message (`None`).
        pub fn verify(&self, cert: &str, signature: &[u8], data: Option<&[u8]>) -> Output {
            let cert = self.file("signer.cert", cert.as_bytes());
            let sig = self.file("sig.asc", signature);
            let mut args = vec![
                OsStr::new("verify").to_os_string(),
                OsStr::new("--signer-file").to_os_string(),
                cert.into_os_string(),
            ];
            match data {
                Some(data) => {
                    args.push(OsStr::new("--signature-file").to_os_string());
                    args.push(sig.into_os_string());
                    args.push(self.file("signed.bin", data).into_os_string());
                }
                None => {
                    args.push(OsStr::new("--cleartext").to_os_string());
                    args.push(sig.into_os_string());
                }
            }
            self.sq(args)
        }

        /// `sq inspect` of the certificate (or certificate fragment) `data`.
        pub fn inspect(&self, data: &[u8]) -> Output {
            let file = self.file("inspect.asc", data);
            self.sq([OsStr::new("inspect").to_os_string(), file.into_os_string()])
        }

        /// Decrypt `data` with the secret key `key` (armored) and `password`.
        pub fn decrypt(&self, key: &str, password: &[u8], data: &[u8]) -> Vec<u8> {
            let key = self.file("key.asc", key.as_bytes());
            let pw = self.file("key.pw", password);
            let msg = self.file("msg.pgp", data);
            check(
                self.sq([
                    OsStr::new("decrypt"),
                    OsStr::new("--recipient-file"),
                    key.as_os_str(),
                    OsStr::new("--password-file"),
                    pw.as_os_str(),
                    msg.as_os_str(),
                ]),
                "sq decrypt",
            )
        }
    }
}

// ---------------------------------------------------------------------------
// Memory measurement
// ---------------------------------------------------------------------------

/// Peak resident memory of this process, for the large-file tier's
/// bounded-memory assertions. Linux only: elsewhere [`PeakRss::start`] panics
/// rather than let a memory assertion pass vacuously.
pub mod memory {
    /// Bytes of a `/proc/self/status` field reported in kB.
    #[cfg(target_os = "linux")]
    fn status_kib(field: &str) -> u64 {
        let status = std::fs::read_to_string("/proc/self/status").expect("/proc/self/status");
        status
            .lines()
            .find_map(|l| l.strip_prefix(field))
            .and_then(|v| v.trim().trim_end_matches("kB").trim().parse().ok())
            .unwrap_or_else(|| panic!("no {field} in /proc/self/status"))
    }

    /// Measures how far peak RSS rose after [`start`](Self::start).
    pub struct PeakRss {
        #[cfg_attr(not(target_os = "linux"), allow(dead_code))]
        baseline_kib: u64,
    }

    impl PeakRss {
        #[cfg(target_os = "linux")]
        pub fn start() -> Self {
            // Writing "5" to clear_refs resets VmHWM to the current RSS, so the
            // measurement is not polluted by whatever ran before.
            let _ = std::fs::write("/proc/self/clear_refs", "5");
            PeakRss {
                baseline_kib: status_kib("VmRSS:"),
            }
        }

        #[cfg(not(target_os = "linux"))]
        pub fn start() -> Self {
            panic!(
                "the bounded-memory assertion needs Linux /proc; run the large-file tier on Linux"
            );
        }

        /// Peak RSS growth since `start`, in bytes.
        pub fn growth_bytes(&self) -> u64 {
            #[cfg(target_os = "linux")]
            {
                status_kib("VmHWM:").saturating_sub(self.baseline_kib) * 1024
            }
            #[cfg(not(target_os = "linux"))]
            unreachable!()
        }

        /// Assert peak RSS grew by less than `limit` bytes while handling a
        /// `handled`-byte file.
        pub fn assert_below(&self, limit: u64, handled: u64) {
            let growth = self.growth_bytes();
            eprintln!(
                "peak RSS growth: {} MiB while handling {} MiB",
                growth >> 20,
                handled >> 20
            );
            assert!(
                growth < limit,
                "peak RSS grew by {} MiB (limit {} MiB) while handling {} MiB: memory is not bounded",
                growth >> 20,
                limit >> 20,
                handled >> 20
            );
        }
    }
}

// ---------------------------------------------------------------------------
// Self-tests
// ---------------------------------------------------------------------------

#[cfg(test)]
mod selftest {
    //! The fixtures are load-bearing for ~600 downstream assertions, so they
    //! get their own tests. A broken fixture would otherwise surface as a
    //! confusing failure somewhere else entirely.
    use super::*;

    /// The bounded-memory assertion must be able to fail: a real 300 MiB
    /// allocation has to register, or a "0 MiB" reading proves nothing.
    #[cfg(target_os = "linux")]
    #[test]
    #[ignore = "large-file tier: run with scripts/run_tests.sh --large-file-only"]
    fn large_file_memory_probe_detects_a_real_allocation() {
        let peak = memory::PeakRss::start();
        let mut big = vec![0u8; 300 << 20];
        for page in big.chunks_mut(4096) {
            page[0] = 1;
        }
        std::hint::black_box(&big);
        let growth = peak.growth_bytes();
        drop(big);
        assert!(
            growth >= 290 << 20,
            "probe saw only {} MiB of a 300 MiB allocation",
            growth >> 20
        );
    }

    #[test]
    fn the_payload_generator_ignores_read_sizes() {
        use std::io::Read;
        let whole = seipd::bytes(10_007, 3);
        let mut pieces = Vec::new();
        let mut p = seipd::Payload::new(10_007, 3);
        for size in [1usize, 3, 7, 8, 9, 1000, 4096].iter().cycle() {
            let mut buf = vec![0u8; *size];
            let n = p.read(&mut buf).unwrap();
            if n == 0 {
                break;
            }
            pieces.extend_from_slice(&buf[..n]);
        }
        assert_eq!(pieces, whole);
        assert_ne!(seipd::bytes(64, 3), seipd::bytes(64, 4), "the seed matters");
    }

    #[test]
    fn corpus_files_are_embedded_and_non_empty() {
        assert!(!corpus::PAYLOAD.is_empty());
        assert!(!corpus::SIG_GOOD_DETACHED.is_empty());
        assert!(!corpus::ENC_V1SEIPD_MDC.is_empty());
        assert!(corpus::AUX_GOOD.contains("BEGIN PGP PUBLIC KEY BLOCK"));
        assert!(corpus::KEY1_SECRET.contains("BEGIN PGP PRIVATE KEY BLOCK"));
        // The empty vector is genuinely empty -- that is its whole purpose.
        assert!(corpus::EMPTY.is_empty());
    }

    #[test]
    fn corpus_certificates_parse() {
        assert!(!corpus::AUX_GOOD_CERT.public_subkeys.is_empty());
        assert!(!corpus::AUX_V6_KEY.secret_subkeys.is_empty());
        assert!(!corpus::KEY1.secret_subkeys.is_empty());
    }

    #[test]
    fn derived_fingerprints_are_well_formed() {
        for fpr in [
            corpus::aux_good_primary_fpr(),
            corpus::aux_good_sign_fpr(),
            corpus::aux_v6_primary_fpr(),
            corpus::aux_v6_sign_fpr(),
            corpus::key1_primary_fpr(),
        ] {
            assert!(
                fpr.len() == 40 || fpr.len() == 64,
                "a v4 fingerprint is 40 hex digits and a v6 one 64, got {} ({fpr})",
                fpr.len()
            );
            assert!(fpr.chars().all(|c| c.is_ascii_hexdigit()));
            assert_eq!(fpr, fpr.to_uppercase(), "fingerprints are normalised upper");
        }
    }

    #[test]
    fn long_key_id_is_the_low_64_bits() {
        let fpr = corpus::aux_good_sign_fpr();
        let id = corpus::long_key_id(&fpr);
        assert_eq!(id.len(), 16);
        assert!(fpr.ends_with(&id));
    }

    #[test]
    fn generated_fixtures_have_the_expected_shape() {
        let v4 = &*keys::V4_SIGN;
        assert_eq!(v4.primary_fpr.len(), 40, "v4 fingerprint is 160 bits");
        assert_eq!(v4.sign_subkey_fprs.len(), 1);
        assert_eq!(v4.enc_subkey_fprs.len(), 1);

        let v6 = &*keys::V6_SIGN;
        assert_eq!(v6.primary_fpr.len(), 64, "v6 fingerprint is 256 bits");
        assert_eq!(v6.sign_subkey_fprs.len(), 1);
        assert_eq!(v6.enc_subkey_fprs.len(), 1);

        assert!(keys::V4_PRIMARY_ONLY.sign_subkey_fprs.is_empty());
        assert!(keys::V4_PRIMARY_ONLY.enc_subkey_fprs.is_empty());
    }

    #[test]
    fn derived_fixtures_retain_the_base_primary_key() {
        // Revoking or expiring must not mint a new key: downstream tests
        // compare these against `V4_SIGN`.
        assert_eq!(
            keys::V4_REVOKED_SUBKEY.primary_fpr,
            keys::V4_SIGN.primary_fpr
        );
        assert_eq!(
            keys::V4_REVOKED_PRIMARY.primary_fpr,
            keys::V4_SIGN.primary_fpr
        );
        assert_eq!(keys::V4_SHORT_EXPIRY.primary_fpr, keys::V4_SIGN.primary_fpr);
    }

    #[test]
    fn fixture_armor_roundtrips_and_has_no_crc24() {
        assert::roundtrips_through_armor(&keys::V4_SIGN.secret_armored);
        assert::roundtrips_through_armor(&keys::V6_SIGN.secret_armored);
        assert::armor_has_no_crc24(&keys::V4_SIGN.secret_armored);
        assert::armor_has_no_crc24(&keys::V6_SIGN.public_armored);
    }

    #[test]
    fn appendix_a_constants_are_armored_blocks() {
        for (name, block) in [
            ("A.1", rfc9580::A1_V4_ED25519LEGACY_CERT),
            ("A.2", rfc9580::A2_V4_ED25519LEGACY_SIG),
            ("A.3", rfc9580::A3_V6_CERT),
            ("A.4", rfc9580::A4_V6_SECRET_UNLOCKED),
            ("A.5", rfc9580::A5_V6_SECRET_LOCKED),
            ("A.6", rfc9580::A6_CLEARTEXT),
            ("A.7", rfc9580::A7_INLINE_SIGNED),
            ("A.8", rfc9580::A8_X25519_AEAD_OCB),
            ("A.9", rfc9580::A9_EAX_SKESK),
            ("A.10", rfc9580::A10_OCB_SKESK),
            ("A.11", rfc9580::A11_GCM_SKESK),
            ("A.12.1", rfc9580::A12_ARGON2_AES128),
            ("A.12.2", rfc9580::A12_ARGON2_AES192),
            ("A.12.3", rfc9580::A12_ARGON2_AES256),
        ] {
            assert!(block.starts_with("-----BEGIN PGP "), "{name} armor header");
            assert!(block.trim_end().ends_with("-----"), "{name} armor tail");
        }
    }

    #[test]
    fn leaked_c_buffer_is_reclaimable_by_the_host_stub() {
        // The FFI callback stubs rely on this shape: capacity == length, so
        // `gfc_secure_free_buffer` can rebuild the allocation exactly.
        let (ptr, len) = leak_as_c_buffer(b"secret");
        assert_eq!(len, 6);
        gfc_secure_free_buffer(ptr, len);
        // Freeing null or a zero length must be a no-op, never a crash.
        gfc_secure_free_buffer(std::ptr::null_mut(), 0);
        gfc_secure_free_cstr(std::ptr::null_mut());
    }
}
