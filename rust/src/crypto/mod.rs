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

//! Unified OpenPGP crypto module.
//!
//! `mod.rs` defines all shared types, helper functions, and re-exports used by
//! the sub-modules. Each sub-module handles one operation family:
//! - [`encrypt`] — public-key and symmetric encryption, encrypt-and-sign (streaming + in-memory)
//! - [`sign`]    — inline, clear-text, and detached signing (streaming + in-memory)
//! - [`verify`]  — detached-signature verification over seekable streams + in-memory verify
//! - [`decrypt`] — decryption with optional integrated signature verification (streaming + in-memory)
//!
//! All internal result types carry only Rust-owned data; the FFI layer in
//! `ffi::crypto` converts them into the `GfrXxxResultC` structs expected by C++.

use crate::host::gfc_secure_free_cstr;
pub(crate) use crate::{
    cache::{PASSWORD_CACHE, PasswordCachePolicy},
    err::{IntoGfrResult, RecordErr, set_last_error},
    tar::build_tar_tempfile_from_directory,
    types::{
        GfrPasswordFetchCb, GfrPublicKeyFetchCb, GfrRecipientStatus, GfrSecretKeyFetchCb,
        GfrSignMode, GfrSignatureStatus, GfrStatus,
    },
    utils::{PassphraseStateInternal, fetch_password_with_cache},
};
pub(crate) use core::fmt;
pub(crate) use log::debug;
pub(crate) use pgp::{
    armor::Dearmor,
    composed::{
        CleartextSignedMessage, Deserializable, DetachedSignature, Esk, Message, MessageBuilder,
        SignedPublicKey, SignedSecretKey,
    },
    crypto::{hash::HashAlgorithm, sym::SymmetricKeyAlgorithm},
    packet::{Packet, PacketParser, SecretKey, SecretSubkey},
    ser::Serialize,
    types::{KeyDetails, Password, SecretParams, StringToKey, Tag},
};
pub(crate) use rand::thread_rng;
pub(crate) use std::{
    ffi::{CStr, CString, c_void},
    fs::File,
    io::{BufReader, Cursor, Read, Seek, SeekFrom, Write},
    path::Path,
};

mod decrypt;
mod encrypt;
mod sign;
mod verify;

pub use decrypt::*;
pub use encrypt::*;
pub use sign::*;
pub use verify::*;

/// A recipient whose session key could not be encrypted.
pub struct InvalidRecipientInternal {
    pub fpr: String,
    pub reason: GfrStatus,
}

/// Result of an in-memory encryption operation.
pub struct EncryptResultInternal {
    pub data: Vec<u8>,
    pub invalid_recipients: Vec<InvalidRecipientInternal>,
    /// Subkeys the session key was actually encrypted to (one per valid recipient).
    pub recipients: Vec<RecipientResultInternal>,
}

/// A single decryption recipient key as found in the encrypted message.
pub struct RecipientResultInternal {
    /// 16-character hex key ID (OpenPGP PKESK packets expose the key ID, not the full fingerprint).
    pub key_id: String,
    pub pub_algo: String,
    pub status: GfrRecipientStatus,
}

/// Result of an in-memory decryption operation.
pub struct DecryptResultInternal {
    pub data: Vec<u8>,
    /// Filename from the OpenPGP literal data packet (may be empty).
    pub filename: String,
    pub recipients: Vec<RecipientResultInternal>,
}

/// Result of an in-memory signing operation.
pub struct SignResultInternal {
    pub data: Vec<u8>,
    pub signatures: Vec<SignatureResultInternal>,
}

/// Result of an in-memory verification operation.
pub struct VerifyResultInternal {
    /// Extracted plaintext (inline/clear-text modes); empty for detached.
    pub data: Vec<u8>,
    /// True if at least one signature verified successfully.
    pub is_verified: bool,
    pub signatures: Vec<SignatureResultInternal>,
}

/// Per-signature verification result, shared by all crypto operations.
pub struct SignatureResultInternal {
    /// Issuer fingerprint (uppercase hex). May be empty for anonymous signers.
    pub fpr: String,
    pub status: GfrSignatureStatus,
    /// Unix timestamp from the Signature Creation Time subpacket (0 if absent).
    pub created_at: u32,
    /// Absolute Unix expiry from the Signature Expiration Time subpacket
    /// (0 = never expires). Consulted by [`signature_result_expired`].
    pub expires_at: u32,
    pub pub_algo: String,
    pub hash_algo: String,
    pub sig_type: GfrSignMode,
}

/// Result of a combined in-memory encrypt-and-sign operation.
pub struct EncryptAndSignResultInternal {
    pub data: Vec<u8>,
    pub signatures: Vec<SignatureResultInternal>,
    pub invalid_recipients: Vec<InvalidRecipientInternal>,
    /// Subkeys the session key was actually encrypted to (one per valid recipient).
    pub recipients: Vec<RecipientResultInternal>,
}

/// Result of a combined in-memory decrypt-and-verify operation.
pub struct DecryptAndVerifyResultInternal {
    pub data: Vec<u8>,
    pub filename: String,
    pub recipients: Vec<RecipientResultInternal>,
    pub is_verified: bool,
    pub signatures: Vec<SignatureResultInternal>,
}

/// Holds a reference to either a primary signing key or a signing subkey.
///
/// Using an enum instead of `&dyn` avoids virtual dispatch overhead and lets
/// rpgp's generic builder methods receive the concrete key type they expect.
pub enum SelectedKey<'a> {
    Primary(&'a SecretKey),
    Sub(&'a SecretSubkey),
}

impl<'a> SelectedKey<'a> {
    pub fn fpr(&self) -> String {
        match self {
            SelectedKey::Primary(k) => k.fingerprint().to_string(),
            SelectedKey::Sub(k) => k.fingerprint().to_string(),
        }
    }

    pub fn is_encrypted(&self) -> bool {
        match self {
            SelectedKey::Primary(k) => matches!(k.secret_params(), SecretParams::Encrypted(_)),
            SelectedKey::Sub(k) => matches!(k.secret_params(), SecretParams::Encrypted(_)),
        }
    }

    pub fn algorithm(&self) -> pgp::crypto::public_key::PublicKeyAlgorithm {
        match self {
            SelectedKey::Primary(k) => k.algorithm(),
            SelectedKey::Sub(k) => k.algorithm(),
        }
    }
}

/// Result of a public-key encryption stream operation.
pub struct EncryptStreamResultInternal {
    /// Recipients whose session key could not be encrypted (parse or algorithm failure).
    pub invalid_recipients: Vec<InvalidRecipientInternal>,
    /// Subkeys the session key was actually encrypted to (one per valid recipient).
    pub recipients: Vec<RecipientResultInternal>,
}

/// Result of a signing stream operation.
pub struct SignStreamResultInternal {
    /// One entry per signing key that produced a signature packet.
    pub signatures: Vec<SignatureResultInternal>,
}

/// Result of a detached-signature verification operation.
pub struct VerifyStreamResultInternal {
    /// True if at least one signature verified successfully against a fetched key.
    pub is_verified: bool,
    pub signatures: Vec<SignatureResultInternal>,
}

/// Result of a combined encrypt-and-sign stream operation.
pub struct EncryptAndSignStreamResultInternal {
    pub signatures: Vec<SignatureResultInternal>,
    pub invalid_recipients: Vec<InvalidRecipientInternal>,
    /// Subkeys the session key was actually encrypted to (one per valid recipient).
    pub recipients: Vec<RecipientResultInternal>,
}

/// Result of a combined decrypt-and-verify stream operation.
pub struct DecryptAndVerifyStreamResultInternal {
    /// Filename embedded in the OpenPGP literal data packet (may be empty).
    pub filename: String,
    pub recipients: Vec<RecipientResultInternal>,
    /// True if at least one embedded signature verified successfully.
    pub is_verified: bool,
    pub signatures: Vec<SignatureResultInternal>,
}

/// Placeholder result for symmetric encryption; carries no additional data.
pub struct SymmetricEncryptStreamResultInternal {}

/// Extract an optional target subkey fingerprint from the armored block prefix.
///
/// The caller may prepend a fingerprint (or key ID) followed by `!` to the
/// armor block to force signing with a specific subkey, e.g.:
/// `"AABBCCDD!\n-----BEGIN PGP PRIVATE KEY BLOCK-----\n..."`.
/// Returns `(Some(fingerprint), armor_block)` if a valid `!`-terminated prefix
/// is present, otherwise `(None, full_block)`.
pub fn parse_signer_block(block: &str) -> (Option<String>, &str) {
    let trimmed = block.trim_start();

    if let Some(pos) = trimmed.find("-----BEGIN PGP") {
        let prefix = trimmed[..pos].trim();

        if let Some(target) = prefix.strip_suffix('!') {
            let target = normalize_key_identifier(target);

            if !target.is_empty() {
                return (Some(target), &trimmed[pos..]);
            }
        }

        return (None, &trimmed[pos..]);
    }

    (None, trimmed)
}

fn normalize_key_identifier(s: &str) -> String {
    s.chars()
        .filter(|c| !c.is_whitespace())
        .collect::<String>()
        .to_uppercase()
}

fn key_identifier_matches(fpr: &str, key_id: &str, target: &str) -> bool {
    let fpr = normalize_key_identifier(fpr);
    let key_id = normalize_key_identifier(key_id);
    let target = normalize_key_identifier(target);

    if target.is_empty() {
        return false;
    }

    // Full fingerprint match
    if fpr == target {
        return true;
    }

    // Long key id match
    if key_id == target {
        return true;
    }

    // Allow shortened suffix matching only for shorter user-provided identifiers.
    // For example: last 16 hex chars.
    fpr.ends_with(&target) || key_id.ends_with(&target)
}

/// Helper to find the signing key (either primary or subkey) that matches the
/// specified target (if any), and execute the provided action with that key if
/// found.
///
/// Core routing mechanism for exact target matching (!) or automatic fallback.
pub fn with_signing_key<'a, F, R>(
    skey: &'a SignedSecretKey,
    target_fpr: Option<&str>,
    mut action: F,
) -> Result<R, crate::types::GfrStatus>
where
    F: FnMut(SelectedKey<'a>) -> Result<R, crate::types::GfrStatus>,
{
    // ==========================================
    // EXACT MATCH MODE (!)
    // ==========================================
    let primary_fpr_bytes = skey.primary_key.fingerprint().as_bytes().to_vec();

    if let Some(target) = target_fpr {
        let target = normalize_key_identifier(target);

        if target.is_empty() {
            return Err(crate::types::GfrStatus::ErrorInvalidInput);
        }

        log::info!("Requested signing target: {}", target);

        for subkey in &skey.secret_subkeys {
            let fpr = subkey.key.fingerprint().to_string();
            let kid = subkey.key.legacy_key_id().to_string();

            log::info!(
                "Available signing subkey: fpr={}, keyid={}, algo={:?}, can_sign_algo={}",
                fpr,
                kid,
                subkey.key.algorithm(),
                subkey.key.algorithm().can_sign(),
            );

            if key_identifier_matches(&fpr, &kid, &target) {
                if crate::key::is_subkey_revoked(&subkey.signatures, &primary_fpr_bytes) {
                    log::error!("Requested signing subkey is revoked: fpr={}", fpr);
                    return Err(crate::types::GfrStatus::ErrorInvalidInput);
                }
                if !subkey.key.algorithm().can_sign() {
                    log::error!(
                        "Requested subkey is not signing-capable: fpr={}, keyid={}, algo={:?}",
                        fpr,
                        kid,
                        subkey.key.algorithm(),
                    );
                    return Err(crate::types::GfrStatus::ErrorInvalidInput);
                }

                log::info!("Selected marked signing subkey: fpr={}, keyid={}", fpr, kid,);

                return action(SelectedKey::Sub(&subkey.key));
            }
        }

        let primary_fpr = skey.primary_key.fingerprint().to_string();
        let primary_kid = skey.primary_key.legacy_key_id().to_string();

        if key_identifier_matches(&primary_fpr, &primary_kid, &target) {
            if !skey.primary_key.algorithm().can_sign() {
                log::error!(
                    "Requested primary key is not signing-capable: fpr={}, keyid={}, algo={:?}",
                    primary_fpr,
                    primary_kid,
                    skey.primary_key.algorithm(),
                );
                return Err(crate::types::GfrStatus::ErrorInvalidInput);
            }

            log::info!(
                "Selected marked primary signing key: fpr={}, keyid={}",
                primary_fpr,
                primary_kid,
            );

            return action(SelectedKey::Primary(&skey.primary_key));
        }

        log::error!("Requested signing target not found: {}", target);
        return Err(crate::types::GfrStatus::ErrorInvalidInput);
    }

    // ==========================================
    // NORMAL MODE (Auto Fallback) — skip revoked
    // ==========================================
    for subkey in &skey.secret_subkeys {
        if crate::key::is_subkey_revoked(&subkey.signatures, &primary_fpr_bytes) {
            log::info!(
                "Skipping revoked signing subkey: fpr={}",
                subkey.key.fingerprint(),
            );
            continue;
        }
        if subkey.key.algorithm().can_sign() {
            log::info!(
                "No target specified. Selected first signing-capable subkey: fpr={}",
                subkey.key.fingerprint(),
            );
            return action(SelectedKey::Sub(&subkey.key));
        }
    }

    if skey.primary_key.algorithm().can_sign() {
        log::info!(
            "No target specified. Selected primary signing key: fpr={}",
            skey.primary_key.fingerprint(),
        );
        return action(SelectedKey::Primary(&skey.primary_key));
    }

    Err(crate::types::GfrStatus::ErrorInvalidInput)
}

/// Return true if `cert`'s primary key or any subkey matches `issuer_hex`.
///
/// `issuer_hex` may be a full fingerprint, a 16-char long key ID, or a suffix
/// thereof. Comparison is normalised to uppercase with whitespace stripped.
pub fn cert_contains_issuer(cert: &SignedPublicKey, issuer_hex: &str) -> bool {
    if key_identifier_matches(
        &cert.primary_key.fingerprint().to_string(),
        &cert.primary_key.legacy_key_id().to_string(),
        issuer_hex,
    ) {
        return true;
    }

    cert.public_subkeys.iter().any(|subkey| {
        key_identifier_matches(
            &subkey.key.fingerprint().to_string(),
            &subkey.key.legacy_key_id().to_string(),
            issuer_hex,
        )
    })
}

pub fn algo_to_string_simple(algo: pgp::crypto::public_key::PublicKeyAlgorithm) -> String {
    // Uses the derived Debug trait to get the variant name as a String
    format!("{:?}", algo)
}

/// RFC 9580 §9.5: an implementation MUST NOT validate a signature that depends on
/// MD5, SHA-1, or RIPEMD-160. `hash_algo` is the display string recorded on a
/// [`SignatureResultInternal`] (rPGP renders these exactly as "MD5", "SHA1",
/// "RIPEMD160").
pub fn sig_hash_algo_is_weak(hash_algo: &str) -> bool {
    matches!(hash_algo, "MD5" | "SHA1" | "RIPEMD160")
}

#[cfg(test)]
mod rfc9580_policy_tests {
    use super::*;

    /// RFC 9580 §9.5: MD5/SHA-1/RIPEMD-160 signatures must be treated as weak and
    /// never reported as valid; SHA-2/SHA-3 hashes are acceptable.
    #[test]
    fn weak_hash_policy() {
        assert!(sig_hash_algo_is_weak("MD5"));
        assert!(sig_hash_algo_is_weak("SHA1"));
        assert!(sig_hash_algo_is_weak("RIPEMD160"));
        assert!(!sig_hash_algo_is_weak("SHA256"));
        assert!(!sig_hash_algo_is_weak("SHA512"));
        assert!(!sig_hash_algo_is_weak("SHA3-512"));
    }

    fn sig_with_expiry(expires_at: u32) -> SignatureResultInternal {
        SignatureResultInternal {
            fpr: "AA".to_string(),
            status: GfrSignatureStatus::NoKey,
            created_at: 1_000,
            expires_at,
            pub_algo: "ED25519".to_string(),
            hash_algo: "SHA512".to_string(),
            sig_type: GfrSignMode::Inline,
        }
    }

    /// RFC 9580 §5.2.3.18: a signature whose own expiration time has passed must
    /// not be reported valid; `expires_at == 0` means it never expires.
    #[test]
    fn signature_expiry_policy() {
        // Never-expires sentinel.
        assert!(!signature_result_expired(&sig_with_expiry(0)));
        // Far past (year 2001) -> expired.
        assert!(signature_result_expired(&sig_with_expiry(1_000_000_000)));
        // Far future (year 2096) -> not expired.
        assert!(!signature_result_expired(&sig_with_expiry(4_000_000_000)));
    }

    // --- RFC 9580 §9.5 weak-hash gate: exhaustive -------------------------

    #[test]
    fn weak_hash_flags_every_forbidden_digest() {
        for h in ["MD5", "SHA1", "RIPEMD160"] {
            assert!(sig_hash_algo_is_weak(h), "{h} must be weak");
        }
    }

    #[test]
    fn weak_hash_accepts_modern_digests() {
        for h in [
            "SHA224", "SHA256", "SHA384", "SHA512", "SHA3-256", "SHA3-512",
        ] {
            assert!(!sig_hash_algo_is_weak(h), "{h} must be acceptable");
        }
    }

    #[test]
    fn weak_hash_match_is_exact() {
        // The gate matches rPGP's exact display strings; unknown or differently
        // cased spellings are not silently treated as weak.
        assert!(!sig_hash_algo_is_weak("sha1"));
        assert!(!sig_hash_algo_is_weak(""));
        assert!(!sig_hash_algo_is_weak("SHA-1"));
        assert!(!sig_hash_algo_is_weak("MD-5"));
    }

    // --- signature expiration boundaries ----------------------------------

    #[test]
    fn signature_expiry_zero_never_expires() {
        assert!(!signature_result_expired(&sig_with_expiry(0)));
    }

    #[test]
    fn signature_expiry_one_second_is_past() {
        // 1970-01-01T00:00:01Z is unambiguously in the past.
        assert!(signature_result_expired(&sig_with_expiry(1)));
    }

    #[test]
    fn signature_expiry_max_u32_is_future() {
        // u32::MAX is 2106-02-07, comfortably in the future.
        assert!(!signature_result_expired(&sig_with_expiry(u32::MAX)));
    }

    // --- apply_signature_gate truth table ---------------------------------

    fn gated(verified: bool, hash: &str, expires_at: u32) -> (GfrSignatureStatus, bool) {
        let mut sig = sig_with_expiry(expires_at);
        sig.hash_algo = hash.to_string();
        sig.status = GfrSignatureStatus::NoKey;
        let mut is_verified = false;
        apply_signature_gate(&mut sig, verified, &mut is_verified);
        (sig.status, is_verified)
    }

    #[test]
    fn gate_marks_valid_when_verified_strong_unexpired() {
        assert_eq!(gated(true, "SHA512", 0), (GfrSignatureStatus::Valid, true));
    }

    #[test]
    fn gate_rejects_weak_hash_even_when_verified() {
        // A cryptographically valid SHA-1 signature must never be Valid (§9.5).
        let (status, is_verified) = gated(true, "SHA1", 0);
        assert_ne!(status, GfrSignatureStatus::Valid);
        assert!(!is_verified);
    }

    #[test]
    fn gate_rejects_expired_even_when_verified() {
        let (status, is_verified) = gated(true, "SHA512", 1);
        assert_ne!(status, GfrSignatureStatus::Valid);
        assert!(!is_verified);
    }

    #[test]
    fn gate_rejects_unverified_signature() {
        let (status, is_verified) = gated(false, "SHA512", 0);
        assert_eq!(status, GfrSignatureStatus::BadSignature);
        assert!(!is_verified);
    }

    // --- signer-block prefix parsing --------------------------------------

    #[test]
    fn parse_signer_block_without_prefix() {
        let (target, rest) = parse_signer_block("-----BEGIN PGP MESSAGE-----\nbody");
        assert!(target.is_none());
        assert!(rest.starts_with("-----BEGIN PGP MESSAGE-----"));
    }

    #[test]
    fn parse_signer_block_with_pinned_prefix() {
        let (target, rest) = parse_signer_block("DEADBEEF!\n-----BEGIN PGP MESSAGE-----\nbody");
        assert_eq!(target.as_deref(), Some("DEADBEEF"));
        assert!(rest.starts_with("-----BEGIN PGP MESSAGE-----"));
    }

    #[test]
    fn parse_signer_block_non_pinned_prefix_is_ignored() {
        // A prefix without a trailing '!' is not a pin request.
        let (target, _rest) = parse_signer_block("DEADBEEF\n-----BEGIN PGP MESSAGE-----");
        assert!(target.is_none());
    }

    // --- corpus-backed sniffing (see scripts/gen_rpgp_test_vectors.sh) -----

    const SIG_GOOD: &[u8] =
        include_bytes!("../../../resource/lfs/test/rpgp_vectors/sig_good_detached.sig");
    const SIG_SHA1: &[u8] =
        include_bytes!("../../../resource/lfs/test/rpgp_vectors/sig_sha1_detached.sig");
    const SIG_V6: &[u8] =
        include_bytes!("../../../resource/lfs/test/rpgp_vectors/sig_v6_detached.sig");
    const ENC_MULTI: &[u8] =
        include_bytes!("../../../resource/lfs/test/rpgp_vectors/enc_multi_recipient.pgp");
    const AUX_GOOD_CERT: &str =
        include_str!("../../../resource/lfs/test/rpgp_aux_keys/aux_good.asc");
    const AUX_REVOKED_CERT: &str =
        include_str!("../../../resource/lfs/test/rpgp_aux_keys/aux_revoked.asc");

    // aux_good's signing-subkey issuer fingerprint (see MANIFEST.txt).
    const AUX_GOOD_SIGN_FPR: &str = "d3abb909adde4a9d9ea30e0a8c871bb1d239c773";

    #[test]
    fn sniff_good_detached_reports_strong_hash_and_issuer() {
        let sigs = sniff_signatures(SIG_GOOD, GfrSignMode::Detached);
        assert_eq!(sigs.len(), 1);
        assert!(sigs[0].hash_algo.eq_ignore_ascii_case("SHA512"));
        assert!(sigs[0].fpr.eq_ignore_ascii_case(AUX_GOOD_SIGN_FPR));
        // Sniffed entries are unverified until the key is checked.
        assert_eq!(sigs[0].status, GfrSignatureStatus::NoKey);
    }

    #[test]
    fn sniff_sha1_detached_reports_weak_hash() {
        let sigs = sniff_signatures(SIG_SHA1, GfrSignMode::Detached);
        assert_eq!(sigs.len(), 1);
        assert!(sig_hash_algo_is_weak(&sigs[0].hash_algo));
    }

    #[test]
    fn sniff_v6_detached_yields_one_signature() {
        let sigs = sniff_signatures(SIG_V6, GfrSignMode::Detached);
        assert_eq!(sigs.len(), 1);
        assert!(!sigs[0].fpr.is_empty());
    }

    #[test]
    fn sniff_multi_recipient_lists_all_recipients() {
        // enc_multi_recipient.pgp is encrypted to key1+key2+key3; sq emits one
        // PKESK per encryption subkey, so there are at least three recipients
        // (key1 alone carries three ECDH encryption subkeys).
        let recipients = sniff_recipients(ENC_MULTI);
        assert!(recipients.len() >= 3, "got {}", recipients.len());
        assert!(recipients.iter().all(|r| !r.key_id.is_empty()));
    }

    #[test]
    fn get_signature_issuers_from_good_detached_signature() {
        let res = get_signature_issuers_internal(SIG_GOOD);
        assert!(res.is_ok());
    }

    // --- certificate revocation / issuer gates ----------------------------

    #[test]
    fn cert_primary_revoked_is_false_for_live_key() {
        use pgp::composed::Deserializable;
        let (cert, _) = pgp::composed::SignedPublicKey::from_string(AUX_GOOD_CERT).unwrap();
        assert!(!cert_primary_revoked(&cert));
    }

    #[test]
    fn cert_primary_revoked_is_true_for_revoked_key() {
        use pgp::composed::Deserializable;
        let (cert, _) = pgp::composed::SignedPublicKey::from_string(AUX_REVOKED_CERT).unwrap();
        assert!(cert_primary_revoked(&cert));
    }

    #[test]
    fn cert_contains_issuer_matches_signing_subkey() {
        use pgp::composed::Deserializable;
        let (cert, _) = pgp::composed::SignedPublicKey::from_string(AUX_GOOD_CERT).unwrap();
        assert!(cert_contains_issuer(&cert, AUX_GOOD_SIGN_FPR));
        assert!(!cert_contains_issuer(
            &cert,
            "0000000000000000000000000000000000000000"
        ));
    }

    #[test]
    fn live_cert_has_a_usable_signing_subkey() {
        use pgp::composed::Deserializable;
        let (cert, _) = pgp::composed::SignedPublicKey::from_string(AUX_GOOD_CERT).unwrap();
        assert!(
            cert.public_subkeys
                .iter()
                .any(|sk| subkey_usable_for_verify(&cert, sk))
        );
    }
}

/// True if the certificate's primary key carries a valid self-revocation
/// (RFC 9580 §5.2.1.11). A signature that only verifies under a revoked primary
/// key MUST NOT be reported as valid.
pub fn cert_primary_revoked(cert: &SignedPublicKey) -> bool {
    let primary_fpr_bytes = cert.primary_key.fingerprint().as_bytes().to_vec();
    crate::key::is_primary_key_revoked(&cert.details.revocation_signatures, &primary_fpr_bytes)
        || crate::key::is_primary_key_revoked(&cert.details.direct_signatures, &primary_fpr_bytes)
}

/// True if `subkey` may be used to verify a signature: it must be signing-capable
/// and not revoked (RFC 9580 §5.2.1.12 / Key Flags §5.2.3.29). Verifying under a
/// revoked or encryption-only subkey MUST NOT be reported as a valid signature.
pub fn subkey_usable_for_verify(
    cert: &SignedPublicKey,
    subkey: &pgp::composed::SignedPublicSubKey,
) -> bool {
    let primary_fpr_bytes = cert.primary_key.fingerprint().as_bytes().to_vec();
    subkey.key.algorithm().can_sign()
        && !crate::key::is_subkey_revoked(&subkey.signatures, &primary_fpr_bytes)
}

/// Apply the final signature status decision for one certificate uniformly
/// across every verification path (inline / cleartext / detached / decrypt-and-
/// verify).
///
/// `is_cert_valid` must already encode the key-usability gates that depend on
/// the rPGP objects in scope at the call site: the primary key must not be
/// revoked ([`cert_primary_revoked`]) and any subkey used must pass
/// [`subkey_usable_for_verify`]. This routine then layers the remaining
/// signature-level gates that only need the recorded metadata:
///
/// * RFC 9580 §9.5 — a signature made with a weak hash (MD5/SHA-1/RIPEMD-160)
///   is never reported `Valid`, even if it verifies cryptographically.
/// * RFC 9580 §5.2.3.10 — a signature whose own Signature Expiration Time has
///   passed is never reported `Valid`.
///
/// Returns `true` if any signature entry was attributed to this certificate's
/// issuer. Keeping this in one place is deliberate: the decrypt-and-verify path
/// previously carried a divergent copy that omitted every gate, which allowed
/// weak-hash / revoked-key signatures to be reported valid.
pub fn finalize_signature_statuses(
    cert: &SignedPublicKey,
    is_cert_valid: bool,
    signatures: &mut [SignatureResultInternal],
    is_verified: &mut bool,
) -> bool {
    let mut found = false;
    for sig in signatures.iter_mut() {
        if cert_contains_issuer(cert, &sig.fpr) {
            found = true;
            apply_signature_gate(sig, is_cert_valid, is_verified);
        }
    }
    found
}

/// Multi-signature variant of [`finalize_signature_statuses`] that ties each
/// `Valid` decision to the *specific* signature index that verified, rather than
/// to the certificate as a whole.
///
/// A message can carry several signature packets whose issuer subpackets all
/// name the same certificate (e.g. a genuine primary-key signature plus a
/// forged packet naming one of its subkeys). The per-cert form would stamp
/// every issuer-matching entry `Valid` as soon as *any* index verified,
/// mis-attributing a never-verified signature to a real key. This form only
/// marks an entry `Valid` when a signature index whose issuer matches that
/// entry actually verifies under a usable key.
pub fn finalize_signature_statuses_by_index(
    msg: &Message,
    num_sigs: usize,
    cert: &SignedPublicKey,
    signatures: &mut [SignatureResultInternal],
    is_verified: &mut bool,
) -> bool {
    let verified_fprs = verified_issuer_fprs_for_cert(msg, num_sigs, cert);
    let mut found = false;
    for sig in signatures.iter_mut() {
        if cert_contains_issuer(cert, &sig.fpr) {
            found = true;
            // Both `verified_fprs` and `sig.fpr` are issuer fingerprints rendered
            // via `issuer_fingerprint().to_string()`, so an exact match is correct.
            let this_verified = verified_fprs.iter().any(|f| f == &sig.fpr);
            apply_signature_gate(sig, this_verified, is_verified);
        }
    }
    found
}

/// Collect the issuer fingerprints of every signature *index* in `msg` that
/// verifies under a usable key of `cert` (usable = primary not revoked
/// §5.2.1.11, or a signing-capable non-revoked subkey §5.2.1.12).
fn verified_issuer_fprs_for_cert(
    msg: &Message,
    num_sigs: usize,
    cert: &SignedPublicKey,
) -> Vec<String> {
    let mut out = Vec::new();
    let Message::Signed { reader, .. } = msg else {
        return out;
    };
    let primary_usable = !cert_primary_revoked(cert);
    for i in 0..num_sigs {
        let verified = (primary_usable && msg.verify_nested_explicit(i, cert).is_ok())
            || cert.public_subkeys.iter().any(|sk| {
                subkey_usable_for_verify(cert, sk) && msg.verify_nested_explicit(i, sk).is_ok()
            });
        if verified {
            if let Some(sig) = reader.signature(i) {
                for issuer in sig.issuer_fingerprint() {
                    out.push(issuer.to_string());
                }
            }
        }
    }
    out
}

/// Apply the signature-level RFC 9580 gates (§9.5 weak hash, §5.2.3.18
/// expiration) on top of a caller-computed `verified` decision, and update the
/// entry's status. Shared by both the per-cert and per-index finalizers so the
/// gate lives in exactly one place.
fn apply_signature_gate(sig: &mut SignatureResultInternal, verified: bool, is_verified: &mut bool) {
    if verified && !sig_hash_algo_is_weak(&sig.hash_algo) && !signature_result_expired(sig) {
        sig.status = GfrSignatureStatus::Valid;
        *is_verified = true;
    } else if sig.status == GfrSignatureStatus::NoKey {
        sig.status = GfrSignatureStatus::BadSignature;
    }
}

/// True if the signature's own Signature Expiration Time (RFC 9580 §5.2.3.18)
/// has passed. `expires_at` is the absolute Unix expiry recorded at sniff time
/// (0 means the signature never expires).
pub fn signature_result_expired(sig: &SignatureResultInternal) -> bool {
    if sig.expires_at == 0 {
        return false;
    }
    let now = std::time::SystemTime::now()
        .duration_since(std::time::UNIX_EPOCH)
        .map(|d| d.as_secs())
        .unwrap_or(0);
    (sig.expires_at as u64) < now
}

/// Absolute Unix expiry (seconds) for a signature, or 0 when it carries no
/// Signature Expiration Time subpacket. Used to populate
/// [`SignatureResultInternal::expires_at`] at sniff time.
pub fn signature_absolute_expiry(sig: &pgp::packet::Signature) -> u32 {
    match (sig.created(), sig.signature_expiration_time()) {
        (Some(created), Some(dur)) => created.as_secs().saturating_add(dur.as_secs() as u32),
        _ => 0,
    }
}

/// Extract issuer fingerprints and algorithm metadata from signature packets
/// without verifying them. All returned entries have status `NoKey`; the caller
/// updates statuses after fetching and checking the actual public keys.
pub fn sniff_signatures(data: &[u8], mode: GfrSignMode) -> Vec<SignatureResultInternal> {
    let mut results = Vec::new();
    let mut dearmored = Vec::new();
    let _ = Dearmor::new(Cursor::new(data)).read_to_end(&mut dearmored);
    let payload = if dearmored.is_empty() {
        data
    } else {
        &dearmored
    };

    let parser = PacketParser::new(Cursor::new(payload));
    for packet_result in parser {
        if let Ok(Packet::Signature(sig)) = packet_result {
            for issuer in sig.issuer_fingerprint() {
                let fpr = issuer.to_string();
                let (hash_algo_id, pub_algo_id) = if let Some(config) = sig.config() {
                    (
                        config.hash_alg.to_string(),
                        algo_to_string_simple(config.pub_alg),
                    )
                } else {
                    (String::new(), String::new())
                };
                if !results
                    .iter()
                    .any(|r: &SignatureResultInternal| r.fpr == fpr)
                {
                    results.push(SignatureResultInternal {
                        fpr,
                        status: GfrSignatureStatus::NoKey,
                        created_at: sig.created().map(|d| d.as_secs()).unwrap_or(0),
                        expires_at: signature_absolute_expiry(&sig),
                        pub_algo: pub_algo_id,
                        hash_algo: hash_algo_id,
                        sig_type: mode,
                    });
                }
            }
        }
    }
    results
}

// Shared helper to dynamically fetch certs for sniffing results
fn fetch_certs_for_signatures(
    signatures: &[SignatureResultInternal],
    fetch_pubkey_cb: Option<GfrPublicKeyFetchCb>,
    user_data: *mut std::ffi::c_void,
) -> Vec<SignedPublicKey> {
    let mut certs = Vec::new();
    if let Some(cb) = fetch_pubkey_cb {
        for sig in signatures {
            let c_fpr = std::ffi::CString::new(sig.fpr.clone()).unwrap_or_default();
            let c_key_block = cb(c_fpr.as_ptr(), user_data);

            if !c_key_block.is_null() {
                if let Ok(key_str) = unsafe { std::ffi::CStr::from_ptr(c_key_block) }.to_str() {
                    if let Ok((cert, _)) = SignedPublicKey::from_string(key_str) {
                        certs.push(cert);
                    }
                }

                unsafe {
                    gfc_secure_free_cstr(c_key_block);
                }
            }
        }
    }
    certs
}

/// Extract comma-separated recipient key IDs and signer fingerprints from a
/// signed or encrypted data buffer.
///
/// Returns `(recipients_csv, issuers_csv)`. Clear-text messages are handled as
/// a special case before packet parsing; no decryption is performed.
pub fn get_signature_issuers_internal(data: &[u8]) -> Result<(String, String), GfrStatus> {
    let mut recipients = Vec::new();
    let mut issuers = Vec::new();

    // 1. First, attempt to parse as a Cleartext Signed Message
    if let Ok(text_str) = std::str::from_utf8(data) {
        if let Ok((msg, _)) = CleartextSignedMessage::from_string(text_str) {
            for sig in msg.signatures().iter() {
                for issuer in sig.issuer_key_id() {
                    issuers.push(issuer.to_string());
                }
            }
            issuers.sort();
            issuers.dedup();
            return Ok((recipients.join(","), issuers.join(",")));
        }
    }

    // 2. Un-armor if necessary for standard encrypted or detached/inline signed data
    let mut dearmored = Vec::new();
    let _ = Dearmor::new(Cursor::new(data)).read_to_end(&mut dearmored);

    let payload = if dearmored.is_empty() {
        data
    } else {
        &dearmored
    };

    // 3. Parse standard PGP packets
    let parser = PacketParser::new(Cursor::new(payload));

    for packet in parser.flatten() {
        match packet {
            Packet::PublicKeyEncryptedSessionKey(pkesk) => {
                if let Ok(id) = pkesk.id() {
                    recipients.push(id.to_string());
                }
            }
            Packet::OnePassSignature(ops) => {
                match ops.version_specific() {
                    pgp::packet::OpsVersionSpecific::V3 { key_id } => {
                        issuers.push(key_id.to_string());
                    }
                    pgp::packet::OpsVersionSpecific::V6 { fingerprint, .. } => {
                        // V6 OPS uses a 32-byte fingerprint encoded as uppercase hex
                        let fp_str: String =
                            fingerprint.iter().map(|b| format!("{:02X}", b)).collect();
                        issuers.push(fp_str);
                    }
                    _ => {}
                }
            }
            Packet::Signature(sig) => {
                for issuer in sig.issuer_key_id() {
                    issuers.push(issuer.to_string());
                }
            }
            _ => {}
        }
    }

    // 4. Deduplicate the collected IDs to avoid repeating the same Key ID
    recipients.sort();
    recipients.dedup();

    issuers.sort();
    issuers.dedup();

    Ok((recipients.join(","), issuers.join(",")))
}

pub(crate) fn create_output_file(out_file_path: &str) -> Result<File, GfrStatus> {
    File::create(out_file_path).map_err(|e| {
        log::error!("Failed to create output file: {}", e);
        set_last_error(&e.to_string());
        GfrStatus::ErrorIo
    })
}

/// A parsed signing key paired with an optional target subkey fingerprint.
///
/// The second element selects which subkey to sign with. `None` means "use
/// whatever `with_signing_key` selects by capability".
pub(crate) type ParsedSigner = (SignedSecretKey, Option<String>);

/// Parse armored secret key blocks into [`ParsedSigner`] pairs.
///
/// Each block may be prefixed with a fingerprint hint (see `parse_signer_block`)
/// to pin a specific subkey for signing.
pub(crate) fn parse_secret_signers(
    secret_key_blocks: &[&str],
) -> Result<Vec<ParsedSigner>, GfrStatus> {
    let mut parsed_keys = Vec::with_capacity(secret_key_blocks.len());

    for block in secret_key_blocks {
        let (target, armor_block) = parse_signer_block(block);
        let (skey, _) =
            SignedSecretKey::from_string(armor_block).map_err(|_| GfrStatus::ErrorInvalidInput)?;
        parsed_keys.push((skey, target));
    }

    Ok(parsed_keys)
}

/// Extract recipient key IDs from an encrypted buffer without decrypting it.
///
/// Reads only the `PublicKeyEncryptedSessionKey` (PKESK) packets from the
/// OpenPGP envelope. No session key is recovered; all returned entries have
/// status `NoKey` — the caller updates statuses after decryption.
pub fn sniff_recipients(data: &[u8]) -> Vec<RecipientResultInternal> {
    let mut results = Vec::new();
    let mut dearmored = Vec::new();
    let _ = Dearmor::new(Cursor::new(data)).read_to_end(&mut dearmored);
    let payload = if dearmored.is_empty() {
        data
    } else {
        &dearmored
    };

    let parser = PacketParser::new(Cursor::new(payload));
    for packet_result in parser {
        if let Ok(Packet::PublicKeyEncryptedSessionKey(pkesk)) = packet_result {
            if let Ok(id) = pkesk.id() {
                let algo = if let Ok(algo_id) = pkesk.algorithm() {
                    algo_to_string_simple(algo_id)
                } else {
                    String::new()
                };
                results.push(RecipientResultInternal {
                    key_id: id.to_string(),
                    pub_algo: algo,
                    status: GfrRecipientStatus::NoKey,
                });
            }
        }
    }
    results
}
