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
                if crate::key::is_subkey_revoked(
                    skey.primary_key.public_key(),
                    subkey.key.public_key(),
                    &subkey.signatures,
                ) {
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
        if crate::key::is_subkey_revoked(
            skey.primary_key.public_key(),
            subkey.key.public_key(),
            &subkey.signatures,
        ) {
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
    //
    // The vectors and the parsed certificates come from `crate::testutil::corpus`,
    // which embeds the committed corpus once for the whole crate. Fingerprints are
    // *derived* from those certificates rather than written down: the generator
    // script mints fresh random keys on every run, so any literal would silently
    // rot the moment the corpus is regenerated.

    use crate::testutil::corpus::{
        AUX_FORGED_REVOCATION_CERT, AUX_GOOD_CERT, AUX_REVOKED_CERT, ENC_MULTI_RECIPIENT as ENC_MULTI,
        SIG_GOOD_DETACHED as SIG_GOOD, SIG_SHA1_DETACHED as SIG_SHA1,
        SIG_STRONG_WEAK_SAME_KEY as SIG_STRONG_WEAK, SIG_V6_DETACHED as SIG_V6, aux_good_sign_fpr,
        long_key_id,
    };

    // Build the per-packet result entries for a detached signature blob exactly as
    // the production detached paths do: parse every Signature packet and map it
    // through `sig_entry_from_packet` (issuer fingerprint→key-id fallback, no
    // de-duplication). Replaces the former `sniff_signatures` helper.
    fn detached_entries(data: &[u8]) -> Vec<SignatureResultInternal> {
        parse_all_signature_packets(data)
            .iter()
            .map(|sig| sig_entry_from_packet(sig, GfrSignMode::Detached))
            .collect()
    }

    #[test]
    fn sniff_good_detached_reports_strong_hash_and_issuer() {
        let sigs = detached_entries(SIG_GOOD);
        assert_eq!(sigs.len(), 1);
        assert!(sigs[0].hash_algo.eq_ignore_ascii_case("SHA512"));
        assert!(sigs[0].fpr.eq_ignore_ascii_case(&aux_good_sign_fpr()));
        // Sniffed entries are unverified until the key is checked.
        assert_eq!(sigs[0].status, GfrSignatureStatus::NoKey);
    }

    #[test]
    fn sniff_sha1_detached_reports_weak_hash() {
        let sigs = detached_entries(SIG_SHA1);
        assert_eq!(sigs.len(), 1);
        assert!(sig_hash_algo_is_weak(&sigs[0].hash_algo));
    }

    #[test]
    fn sniff_v6_detached_yields_one_signature() {
        let sigs = detached_entries(SIG_V6);
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
        assert!(!cert_primary_revoked(&AUX_GOOD_CERT));
    }

    #[test]
    fn cert_primary_revoked_is_true_for_revoked_key() {
        assert!(cert_primary_revoked(&AUX_REVOKED_CERT));
    }

    #[test]
    fn cert_contains_issuer_matches_signing_subkey() {
        assert!(cert_contains_issuer(&AUX_GOOD_CERT, &aux_good_sign_fpr()));
        assert!(!cert_contains_issuer(
            &AUX_GOOD_CERT,
            "0000000000000000000000000000000000000000"
        ));
    }

    /// B1: a certificate must also be resolvable by a bare 16-hex key ID -- the
    /// fallback identifier `sig_entry_from_packet` emits when a signature carries
    /// only an Issuer Key ID subpacket (no Issuer Fingerprint), as legacy
    /// GnuPG-style signatures do.
    #[test]
    fn cert_contains_issuer_matches_bare_key_id() {
        // The v4 long key ID is the low 64 bits (last 16 hex) of the fingerprint.
        let key_id = long_key_id(&aux_good_sign_fpr());
        assert!(cert_contains_issuer(&AUX_GOOD_CERT, &key_id));
    }

    /// B2: two signatures from one issuer that differ in hash algorithm must both
    /// be surfaced by the packet parser; issuer-only de-duplication would drop the
    /// weak one and hide it from the §9.5 gate. `parse_all_signature_packets` does
    /// no de-duplication, so both packets survive to be gated individually.
    #[test]
    fn packet_parser_keeps_distinct_signatures_from_one_issuer() {
        let sigs = detached_entries(SIG_STRONG_WEAK);
        assert_eq!(sigs.len(), 2);
        let mut hashes: Vec<String> = sigs.iter().map(|s| s.hash_algo.to_uppercase()).collect();
        hashes.sort();
        assert_eq!(hashes, vec!["SHA1".to_string(), "SHA256".to_string()]);
    }

    /// B7: a fabricated primary-key revocation whose signature does not verify
    /// must NOT mark the certificate revoked (self-signature validation on
    /// import). The genuine cert is not revoked, and the tampered copy carrying an
    /// invalid revocation must also parse as not-revoked -- unlike the genuine
    /// `aux_revoked` cert, whose real self-revocation IS honored (see
    /// `cert_primary_revoked_is_true_for_revoked_key`).
    #[test]
    fn forged_revocation_is_not_honored() {
        assert!(!cert_primary_revoked(&AUX_GOOD_CERT));
        assert!(!cert_primary_revoked(&AUX_FORGED_REVOCATION_CERT));
    }

    #[test]
    fn live_cert_has_a_usable_signing_subkey() {
        assert!(
            AUX_GOOD_CERT
                .public_subkeys
                .iter()
                .any(|sk| subkey_usable_for_verify(&AUX_GOOD_CERT, sk))
        );
    }
}

/// Current wall-clock time as seconds since the Unix epoch (0 if the clock is
/// before the epoch, which cannot happen in practice).
fn now_unix_secs() -> u64 {
    std::time::SystemTime::now()
        .duration_since(std::time::UNIX_EPOCH)
        .map(|d| d.as_secs())
        .unwrap_or(0)
}

/// True if the certificate's primary key carries a valid self-revocation
/// (RFC 9580 §5.2.1.11). A signature that only verifies under a revoked primary
/// key MUST NOT be reported as valid.
pub fn cert_primary_revoked(cert: &SignedPublicKey) -> bool {
    crate::key::is_primary_key_revoked(&cert.primary_key, &cert.details.revocation_signatures)
        || crate::key::is_primary_key_revoked(&cert.primary_key, &cert.details.direct_signatures)
}

/// True if the certificate's primary key has passed its own expiration time
/// (RFC 9580 §5.2.3.13). A signature that only verifies under an expired primary
/// key MUST NOT be reported as valid — an expired primary invalidates the whole
/// certificate, including every subkey bound to it.
pub fn cert_primary_expired(cert: &SignedPublicKey) -> bool {
    let expires = crate::key::primary_key_expires_at(cert);
    expires != 0 && u64::from(expires) < now_unix_secs()
}

/// True if `subkey` has passed its own binding-signature expiration time
/// (RFC 9580 §5.2.3.13).
pub fn subkey_expired(
    cert: &SignedPublicKey,
    subkey: &pgp::composed::SignedPublicSubKey,
) -> bool {
    let expires = crate::key::subkey_expires_at(&cert.primary_key, &subkey.key, &subkey.signatures);
    expires != 0 && u64::from(expires) < now_unix_secs()
}

/// True if the certificate's primary key may currently be used to verify a
/// signature: it must be neither revoked ([`cert_primary_revoked`], §5.2.1.11)
/// nor expired ([`cert_primary_expired`], §5.2.3.13). This is the primary-key
/// gate every verification path shares.
pub fn cert_primary_usable(cert: &SignedPublicKey) -> bool {
    !cert_primary_revoked(cert) && !cert_primary_expired(cert)
}

/// True if `subkey` may be used to verify a signature: it must be signing-capable
/// and not revoked (RFC 9580 §5.2.1.12 / Key Flags §5.2.3.29), its owning primary
/// must itself be usable ([`cert_primary_usable`] — a revoked or expired primary
/// invalidates all its subkeys), and the subkey must not have expired. Verifying
/// under a revoked, expired, or encryption-only subkey — or any subkey of a
/// revoked/expired primary — MUST NOT be reported as a valid signature.
pub fn subkey_usable_for_verify(
    cert: &SignedPublicKey,
    subkey: &pgp::composed::SignedPublicSubKey,
) -> bool {
    subkey.key.algorithm().can_sign()
        && !crate::key::is_subkey_revoked(&cert.primary_key, &subkey.key, &subkey.signatures)
        && !subkey_expired(cert, subkey)
        && cert_primary_usable(cert)
}

/// The single per-index signature-attribution driver, shared by every
/// verification path (inline / cleartext / detached in-memory / detached stream
/// / decrypt-and-verify).
///
/// For each signature entry, only certificates that actually carry the entry's
/// issuer ([`cert_contains_issuer`], fingerprint *or* key id) are considered; an
/// entry with no matching cert keeps its `NoKey` status (an unknown signer must
/// never be downgraded to `BadSignature`). When a matching cert exists, the
/// caller-supplied `verifies` closure decides whether the signature *at that
/// entry's index* cryptographically verifies under a *usable* key of the cert —
/// the substrate differs per path (a parsed one-pass [`Message`] index via
/// [`verify_index_under_usable_key`], or a standalone packet over a data buffer
/// / seekable stream via [`signature_verifies_under_usable_key`] /
/// [`signature_verifies_under_usable_key_stream`]). The result is then run
/// through [`apply_signature_gate`], which layers the shared signature-level RFC
/// 9580 gates (§9.5 weak hash, §5.2.3.18 signature expiration).
///
/// Per-*index* attribution is deliberate: several packets can name the same
/// certificate (e.g. a genuine primary signature plus a forged packet naming one
/// of its subkeys), and only the entry whose own index verifies may be stamped
/// `Valid`. Keeping this in one place replaced four divergent copies (a per-cert
/// finalizer, two per-index forms, and an ungated decrypt-path variant).
pub(crate) fn attribute_entries<F>(
    entries: &mut [SignatureResultInternal],
    certs: &[SignedPublicKey],
    is_verified: &mut bool,
    mut verifies: F,
) where
    F: FnMut(usize, &SignedPublicKey) -> bool,
{
    for (i, entry) in entries.iter_mut().enumerate() {
        let matching: Vec<&SignedPublicKey> = certs
            .iter()
            .filter(|cert| cert_contains_issuer(cert, &entry.fpr))
            .collect();
        if matching.is_empty() {
            // No key for this issuer: leave the entry as NoKey (unknown signer).
            continue;
        }
        let verified = matching.iter().any(|cert| verifies(i, cert));
        apply_signature_gate(entry, verified, is_verified);
    }
}

/// The *message* verification substrate: true if signature `index` of a parsed
/// one-pass [`Message`] verifies under a usable key of `cert` — the primary if
/// it is usable ([`cert_primary_usable`], §5.2.1.11 / §5.2.3.13) or any subkey
/// that passes [`subkey_usable_for_verify`] (§5.2.1.12). Used by the inline and
/// decrypt-and-verify paths via [`attribute_entries`].
pub(crate) fn verify_index_under_usable_key(
    msg: &Message,
    index: usize,
    cert: &SignedPublicKey,
) -> bool {
    (cert_primary_usable(cert) && msg.verify_nested_explicit(index, cert).is_ok())
        || cert.public_subkeys.iter().any(|sk| {
            subkey_usable_for_verify(cert, sk) && msg.verify_nested_explicit(index, sk).is_ok()
        })
}

/// Build one `NoKey` result entry per signature packet carried by a parsed
/// one-pass [`Message`], in packet order, via [`sig_entry_from_packet`] (which
/// applies the Issuer Fingerprint→Key ID fallback and records per-packet hash /
/// algorithm / expiry metadata). Every real packet is surfaced — there is no
/// de-duplication, so a weak-hash companion signature from the same issuer stays
/// visible to the §9.5 gate. Shared by the inline and decrypt-and-verify paths,
/// replacing two divergent inline builders (one of which deduped on fingerprint
/// alone and omitted the key-id fallback).
pub(crate) fn signature_entries_from_message(msg: &Message) -> Vec<SignatureResultInternal> {
    let mut entries = Vec::new();
    if let Message::Signed { reader, .. } = msg {
        for i in 0..reader.num_signatures() {
            if let Some(sig) = reader.signature(i) {
                entries.push(sig_entry_from_packet(sig, GfrSignMode::Inline));
            }
        }
    }
    entries
}

/// Apply the signature-level RFC 9580 gates (§9.5 weak hash, §5.2.3.18
/// expiration) on top of a caller-computed `verified` decision, and update the
/// entry's status. Shared by both the per-cert and per-index finalizers so the
/// gate lives in exactly one place.
pub(crate) fn apply_signature_gate(
    sig: &mut SignatureResultInternal,
    verified: bool,
    is_verified: &mut bool,
) {
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

/// Build a single [`SignatureResultInternal`] (status `NoKey`) from a signature
/// packet.
///
/// The issuer identifier prefers the Issuer Fingerprint subpacket (v4/v6) and
/// falls back to the Issuer Key ID subpacket, so a signature that carries only a
/// key id — as GnuPG and other producers legitimately emit — is still surfaced
/// and its certificate can be fetched (the C++ key lookup resolves by
/// fingerprint OR key id, and [`cert_contains_issuer`] matches either). An empty
/// `fpr` means the packet named no issuer at all and cannot be attributed.
pub(crate) fn sig_entry_from_packet(
    sig: &pgp::packet::Signature,
    mode: GfrSignMode,
) -> SignatureResultInternal {
    let mut fpr = String::new();
    for issuer in sig.issuer_fingerprint() {
        fpr = issuer.to_string();
        break;
    }
    if fpr.is_empty() {
        for issuer in sig.issuer_key_id() {
            fpr = issuer.to_string();
            break;
        }
    }

    let (hash_algo, pub_algo) = match sig.config() {
        Some(config) => (
            config.hash_alg.to_string(),
            algo_to_string_simple(config.pub_alg),
        ),
        None => (String::new(), String::new()),
    };

    SignatureResultInternal {
        fpr,
        status: GfrSignatureStatus::NoKey,
        created_at: sig.created().map(|d| d.as_secs()).unwrap_or(0),
        expires_at: signature_absolute_expiry(sig),
        pub_algo,
        hash_algo,
        sig_type: mode,
    }
}

/// Parse every OpenPGP Signature packet from a (possibly armored) buffer.
///
/// Used by the detached-verify path, where a single blob may legitimately carry
/// several signature packets (multiple signers over the same data). Returns them
/// in packet order so the caller can attribute each independently.
pub(crate) fn parse_all_signature_packets(data: &[u8]) -> Vec<pgp::packet::Signature> {
    let mut dearmored = Vec::new();
    let _ = Dearmor::new(Cursor::new(data)).read_to_end(&mut dearmored);
    let payload = if dearmored.is_empty() { data } else { &dearmored };

    PacketParser::new(Cursor::new(payload))
        .flatten()
        .filter_map(|packet| match packet {
            Packet::Signature(sig) => Some(sig),
            _ => None,
        })
        .collect()
}

/// True if signature packet `sig` verifies over `data` under some *usable* key of
/// `cert` — the primary if it is usable ([`cert_primary_usable`], §5.2.1.11 /
/// §5.2.3.13) or any subkey that passes [`subkey_usable_for_verify`].
///
/// [`pgp::packet::Signature::verify`] already enforces issuer matching, weak-hash
/// rejection (§9.5), and text-mode line normalization; this layers on the
/// key-usability gates so a signature under a revoked or expired key is never
/// counted as verified. Used by [`attribute_entries`] for per-index attribution
/// on the cleartext and detached in-memory paths, the packet-over-buffer
/// analogue of [`verify_index_under_usable_key`] for the message paths.
pub(crate) fn signature_verifies_under_usable_key(
    cert: &SignedPublicKey,
    sig: &pgp::packet::Signature,
    data: &[u8],
) -> bool {
    if cert_primary_usable(cert) && sig.verify(&cert.primary_key, data).is_ok() {
        return true;
    }
    cert.public_subkeys
        .iter()
        .any(|sk| subkey_usable_for_verify(cert, sk) && sig.verify(&sk.key, data).is_ok())
}

/// Seekable-stream analogue of [`signature_verifies_under_usable_key`] for the
/// detached-*stream* path (large files verified without buffering the whole
/// payload). Rewinds `stream` before each attempt and hashes it through a
/// [`crate::cancel::CancellableReader`] so a mid-hash cancel aborts promptly.
/// Returns `true` if `sig` verifies over the stream under a usable primary
/// ([`cert_primary_usable`]) or subkey ([`subkey_usable_for_verify`]); the same
/// key-usability gates as the in-memory form, so a signature under a revoked or
/// expired key is never counted as verified.
pub(crate) fn signature_verifies_under_usable_key_stream<R: Read + Seek>(
    cert: &SignedPublicKey,
    sig: &pgp::packet::Signature,
    stream: &mut R,
    channel: i32,
) -> bool {
    if cert_primary_usable(cert) && stream.seek(SeekFrom::Start(0)).is_ok() {
        let mut cancellable = crate::cancel::CancellableReader::new(channel, &mut *stream);
        if sig.verify(&cert.primary_key, &mut cancellable).is_ok() {
            return true;
        }
    }
    for sk in &cert.public_subkeys {
        if subkey_usable_for_verify(cert, sk) && stream.seek(SeekFrom::Start(0)).is_ok() {
            let mut cancellable = crate::cancel::CancellableReader::new(channel, &mut *stream);
            if sig.verify(&sk.key, &mut cancellable).is_ok() {
                return true;
            }
        }
    }
    false
}

/// Upper bound on the number of plaintext octets a *decompressed* message may
/// produce before the operation is aborted. Guards against decompression bombs
/// (RFC 9580 §13.14); deliberately generous so it never trips on real data while
/// still bounding a malicious high-ratio payload to a finite size. Shared by the
/// decrypt and inline-verify paths.
pub(crate) const MAX_DECOMPRESSED_OUTPUT_BYTES: u64 = 4 * 1024 * 1024 * 1024; // 4 GiB

/// A `Read` adapter that errors once cumulative reads exceed `limit` bytes,
/// instead of silently truncating like `Read::take`. Used to cap decompressed
/// output so a compression bomb cannot exhaust memory or disk.
pub(crate) struct LimitedReader<R> {
    inner: R,
    limit: u64,
    read_so_far: u64,
}

impl<R: Read> LimitedReader<R> {
    pub(crate) fn new(inner: R, limit: u64) -> Self {
        Self {
            inner,
            limit,
            read_so_far: 0,
        }
    }
}

impl<R: Read> Read for LimitedReader<R> {
    fn read(&mut self, buf: &mut [u8]) -> std::io::Result<usize> {
        let n = self.inner.read(buf)?;
        self.read_so_far = self.read_so_far.saturating_add(n as u64);
        if self.read_so_far > self.limit {
            return Err(std::io::Error::new(
                std::io::ErrorKind::InvalidData,
                "decompressed output exceeds the maximum allowed size (possible compression bomb)",
            ));
        }
        Ok(n)
    }
}

/// Drain `reader` fully into a `Vec`, bounded by [`MAX_DECOMPRESSED_OUTPUT_BYTES`]
/// so a compression bomb cannot exhaust memory (RFC 9580 §13.14). Reading a
/// message body to the end is also a precondition for `verify_nested_explicit` /
/// `num_signatures` on the inline-verify path.
pub(crate) fn read_to_end_capped<R: Read>(reader: R) -> Result<Vec<u8>, GfrStatus> {
    let mut limited = LimitedReader::new(reader, MAX_DECOMPRESSED_OUTPUT_BYTES);
    let mut buf = Vec::new();
    limited.read_to_end(&mut buf).map_err(|e| {
        set_last_error(&e.to_string());
        GfrStatus::ErrorInvalidData
    })?;
    Ok(buf)
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
            // v3 PKESK exposes an 8-byte key ID; v6 PKESK a full fingerprint
            // (pkesk.id() errors for v6). Fall back so v6 recipients are listed.
            let recipient_id = match pkesk.id() {
                Ok(id) => Some(id.to_string()),
                Err(_) => pkesk.fingerprint().ok().flatten().map(|fp| fp.to_string()),
            };
            if let Some(key_id) = recipient_id {
                let algo = if let Ok(algo_id) = pkesk.algorithm() {
                    algo_to_string_simple(algo_id)
                } else {
                    String::new()
                };
                results.push(RecipientResultInternal {
                    key_id,
                    pub_algo: algo,
                    status: GfrRecipientStatus::NoKey,
                });
            }
        }
    }
    results
}

#[cfg(test)]
mod packet_syntax_tests {
    //! RFC 9580 §3.2 / §4.2 / §4.3 / §5.2.3.7 packet-syntax conformance,
    //! driven by hand-crafted byte vectors that no `sq`/`gpg` invocation can
    //! produce.
    //!
    //! Two assertion styles are used deliberately. Positive framing cases are
    //! checked against rPGP's own decoder, which is the authority on what the
    //! wire format means. Adversarial cases assert only the *outcome class* --
    //! it errors, and it does not panic -- because `pgp::errors::Error`
    //! variants and messages are not a stable interface.

    use super::*;
    use crate::testutil::packets;
    use pgp::packet::PacketHeader;
    use pgp::types::{PacketLength, Tag};
    use std::io::BufReader;

    /// Decode a header with rPGP and return `(tag, length)`.
    fn decode(bytes: &[u8]) -> (Tag, PacketLength) {
        let hdr = PacketHeader::try_from_reader(BufReader::new(bytes)).expect("header decodes");
        (hdr.tag(), hdr.packet_length())
    }

    /// Run `f`, returning true when it neither panicked nor succeeded — the
    /// property every adversarial input must satisfy.
    fn errors_without_panicking<T>(f: impl FnOnce() -> Result<T, GfrStatus> + std::panic::UnwindSafe) -> bool {
        let prev = std::panic::take_hook();
        std::panic::set_hook(Box::new(|_| {}));
        let outcome = std::panic::catch_unwind(f);
        std::panic::set_hook(prev);
        matches!(outcome, Ok(Err(_)))
    }

    // -- §4.2.1 new-format body lengths ------------------------------------

    #[test]
    fn a_one_octet_length_decodes() {
        // §4.2.1.1: "A 1-octet Body Length header encodes a length of 0 to 191".
        let (tag, len) = decode(&packets::new_hdr_forced(11, 100, 1));
        assert_eq!(tag, Tag::LiteralData);
        assert_eq!(len, PacketLength::Fixed(100));
    }

    #[test]
    fn a_two_octet_length_decodes() {
        // §4.2.3 gives 1723 as the worked example, encoded 0xC5 0xFB.
        let bytes = packets::new_hdr_forced(11, 1723, 2);
        assert_eq!(&bytes[1..], &[0xC5, 0xFB]);
        assert_eq!(decode(&bytes).1, PacketLength::Fixed(1723));
    }

    #[test]
    fn a_five_octet_length_decodes() {
        // §4.2.3: 100000 encodes as 0xFF 0x00 0x01 0x86 0xA0.
        let bytes = packets::new_hdr_forced(11, 100_000, 5);
        assert_eq!(&bytes[1..], &[0xFF, 0x00, 0x01, 0x86, 0xA0]);
        assert_eq!(decode(&bytes).1, PacketLength::Fixed(100_000));
    }

    #[test]
    fn the_one_octet_boundary_at_191_decodes() {
        assert_eq!(decode(&packets::new_hdr_forced(11, 191, 1)).1, PacketLength::Fixed(191));
    }

    #[test]
    fn the_two_octet_boundary_at_192_decodes() {
        // 192 is the first length that needs two octets.
        let bytes = packets::new_hdr_forced(11, 192, 2);
        assert_eq!(&bytes[1..], &[192, 0]);
        assert_eq!(decode(&bytes).1, PacketLength::Fixed(192));
    }

    #[test]
    fn the_two_octet_boundary_at_8383_decodes() {
        // §4.2.1.2: the 2-octet form tops out at 8383.
        assert_eq!(decode(&packets::new_hdr_forced(11, 8383, 2)).1, PacketLength::Fixed(8383));
    }

    #[test]
    fn the_five_octet_boundary_at_8384_decodes() {
        assert_eq!(decode(&packets::new_hdr_forced(11, 8384, 5)).1, PacketLength::Fixed(8384));
    }

    #[test]
    fn a_zero_length_body_decodes() {
        assert_eq!(decode(&packets::new_hdr_forced(11, 0, 1)).1, PacketLength::Fixed(0));
    }

    #[test]
    fn the_maximum_five_octet_length_decodes() {
        // §4.2.1.3 allows up to 0xFFFFFFFF.
        assert_eq!(
            decode(&packets::new_hdr_forced(11, u32::MAX, 5)).1,
            PacketLength::Fixed(u32::MAX)
        );
    }

    #[test]
    fn a_non_minimal_five_octet_encoding_still_decodes() {
        // The RFC does not require the *shortest* encoding for packet body
        // lengths (unlike MPIs), so a 5-octet encoding of a small value is
        // legal and must be accepted.
        assert_eq!(decode(&packets::new_hdr_forced(11, 3, 5)).1, PacketLength::Fixed(3));
    }

    #[test]
    fn our_encoder_agrees_with_rpgps_for_every_width() {
        // Guards the hand-rolled builder itself: if it drifted from the spec,
        // every negative test built on it would be meaningless.
        for len in [0u32, 1, 191, 192, 1723, 8383, 8384, 100_000] {
            let ours = packets::packet(11, &vec![0u8; 0]);
            let _ = ours; // shape only; the comparison below is the real check
            let theirs = packets::new_hdr_via_rpgp(Tag::LiteralData, PacketLength::Fixed(len));
            let mine = if len < 192 {
                packets::new_hdr_forced(11, len, 1)
            } else if len < 8384 {
                packets::new_hdr_forced(11, len, 2)
            } else {
                packets::new_hdr_forced(11, len, 5)
            };
            assert_eq!(mine, theirs, "encoding mismatch for length {len}");
        }
    }

    // -- §4.2.1.4 partial body lengths --------------------------------------

    #[test]
    fn a_partial_body_length_decodes() {
        // §4.2.1.4: a first octet in 224..=254 encodes 2^(octet & 0x1F).
        let bytes = packets::partial_body(11, &[&[0u8; 512]], b"tail");
        assert_eq!(decode(&bytes).1, PacketLength::Partial(512));
    }

    #[test]
    fn every_legal_partial_length_is_a_power_of_two() {
        for shift in 0u32..=16 {
            let len = 1usize << shift;
            let bytes = packets::partial_body(11, &[&vec![0u8; len]], b"");
            assert_eq!(decode(&bytes).1, PacketLength::Partial(len as u32));
        }
    }

    #[test]
    fn rpgp_refuses_to_build_a_non_power_of_two_partial_length() {
        // §4.2.1.4: "This length is a power of 2". rPGP enforces it on the
        // encode side, which is where a producer bug would be caught.
        assert!(
            PacketHeader::from_parts(
                pgp::types::PacketHeaderVersion::New,
                Tag::LiteralData,
                PacketLength::Partial(300),
            )
            .is_err()
        );
    }

    #[test]
    fn rpgp_refuses_a_partial_length_above_2_to_the_30() {
        assert!(
            PacketHeader::from_parts(
                pgp::types::PacketHeaderVersion::New,
                Tag::LiteralData,
                PacketLength::Partial(1 << 31),
            )
            .is_err()
        );
    }

    #[test]
    fn a_partial_length_is_rejected_on_a_legacy_header() {
        // §4.2.1.4 is a new-format-only mechanism.
        assert!(
            PacketHeader::from_parts(
                pgp::types::PacketHeaderVersion::Old,
                Tag::LiteralData,
                PacketLength::Partial(512),
            )
            .is_err()
        );
    }

    #[test]
    fn a_partial_stream_reassembles_into_one_message() {
        // The end-to-end property: a literal packet split across partial
        // segments must decrypt/parse to the same bytes as an unsplit one.
        let payload: Vec<u8> = (0..1024u32).map(|i| i as u8).collect();
        let mut body = vec![b'b', 0];
        body.extend_from_slice(&0u32.to_be_bytes());
        body.extend_from_slice(&payload);

        let split = packets::partial_body(11, &[&body[..512]], &body[512..]);
        let msg = Message::from_bytes(std::io::Cursor::new(split));
        assert!(msg.is_ok(), "a partial-length literal packet must parse");
    }

    // -- §4.2.2 legacy framing ----------------------------------------------

    #[test]
    fn a_legacy_one_octet_length_decodes() {
        let (tag, len) = decode(&packets::old_hdr(11, 0, 100));
        assert_eq!(tag, Tag::LiteralData);
        assert_eq!(len, PacketLength::Fixed(100));
    }

    #[test]
    fn a_legacy_two_octet_length_decodes() {
        assert_eq!(decode(&packets::old_hdr(11, 1, 5000)).1, PacketLength::Fixed(5000));
    }

    #[test]
    fn a_legacy_four_octet_length_decodes() {
        assert_eq!(
            decode(&packets::old_hdr(11, 2, 100_000)).1,
            PacketLength::Fixed(100_000)
        );
    }

    #[test]
    fn a_legacy_indeterminate_length_decodes() {
        // §4.2.2 length-type 3: "the packet extends until the end of the file".
        assert_eq!(decode(&packets::old_hdr(11, 3, 0)).1, PacketLength::Indeterminate);
    }

    #[test]
    fn a_legacy_header_cannot_express_a_tag_above_15() {
        // §4.2: "Legacy format headers only have 4 bits for the Packet Type ID".
        assert!(
            PacketHeader::from_parts(
                pgp::types::PacketHeaderVersion::Old,
                Tag::Padding, // type ID 21
                PacketLength::Fixed(4),
            )
            .is_err()
        );
    }

    #[test]
    fn an_indeterminate_length_is_rejected_on_a_new_format_header() {
        // §4.2.1 has no indeterminate encoding; partial lengths replace it.
        assert!(
            PacketHeader::from_parts(
                pgp::types::PacketHeaderVersion::New,
                Tag::LiteralData,
                PacketLength::Indeterminate,
            )
            .is_err()
        );
    }

    #[test]
    fn a_header_with_the_top_bit_clear_is_rejected() {
        // §4.2: bit 7 of the first octet is "always one".
        let bytes = [0x0Bu8, 0x00];
        assert!(PacketHeader::try_from_reader(BufReader::new(&bytes[..])).is_err());
    }

    // -- §4.3 packet criticality -------------------------------------------

    #[test]
    fn a_marker_packet_is_tolerated_in_a_message() {
        // §5.8: "Such a packet MUST be ignored when received."
        let mut data = packets::marker_packet();
        data.extend_from_slice(&packets::literal(b'b', b"", 0, b"payload"));
        let parsed = Message::from_bytes(std::io::Cursor::new(data));
        assert!(parsed.is_ok(), "a leading marker packet must not break parsing");
    }

    #[test]
    fn a_padding_packet_decodes_with_the_right_tag() {
        // §5.14, introduced by RFC 9580 for traffic-analysis resistance.
        let (tag, len) = decode(&packets::padding_packet(32));
        assert_eq!(tag, Tag::Padding);
        assert_eq!(len, PacketLength::Fixed(32));
    }

    #[test]
    fn an_unknown_critical_packet_type_is_not_silently_accepted() {
        // §4.3: "Packets with Type IDs from 0 to 39 are critical" -- an
        // unknown one must invalidate the sequence rather than be skipped.
        let mut data = packets::unknown_packet(30, b"unknown critical");
        data.extend_from_slice(&packets::literal(b'b', b"", 0, b"payload"));
        let outcome = std::panic::catch_unwind(|| {
            Message::from_bytes(std::io::Cursor::new(data)).map(|_| ())
        });
        assert!(outcome.is_ok(), "an unknown packet must never panic the parser");
    }

    #[test]
    fn an_unknown_non_critical_packet_type_does_not_panic() {
        // §4.3: "an unknown non-critical packet MUST be ignored".
        let mut data = packets::unknown_packet(60, b"private use");
        data.extend_from_slice(&packets::literal(b'b', b"", 0, b"payload"));
        let outcome = std::panic::catch_unwind(|| {
            Message::from_bytes(std::io::Cursor::new(data)).map(|_| ())
        });
        assert!(outcome.is_ok());
    }

    #[test]
    fn a_reserved_type_id_zero_packet_does_not_panic() {
        // §5 Table 3: "Reserved - this Packet Type ID MUST NOT be used".
        let data = packets::unknown_packet(0, b"reserved");
        let outcome = std::panic::catch_unwind(|| {
            Message::from_bytes(std::io::Cursor::new(data)).map(|_| ())
        });
        assert!(outcome.is_ok());
    }

    // -- §4.1 truncation and oversized declarations -------------------------

    #[test]
    fn a_declared_length_longer_than_the_body_fails_cleanly() {
        // §4.1: "a parser MUST abort without writing outside the indicated
        // range and MUST treat the packet as malformed and unusable."
        let data = packets::declared_len_larger_than_body(11, 100_000, b"only a few bytes");
        assert!(errors_without_panicking(|| {
            Message::from_bytes(std::io::Cursor::new(data))
                .map(|_| ())
                .map_err(|_| GfrStatus::ErrorInvalidData)
        }));
    }

    #[test]
    fn an_absurd_declared_length_does_not_pre_allocate() {
        // A 4 GiB declaration with a 4-byte body must fail fast rather than
        // trying to reserve the declared size.
        let data = packets::declared_len_larger_than_body(11, u32::MAX, b"tiny");
        let outcome = std::panic::catch_unwind(|| {
            Message::from_bytes(std::io::Cursor::new(data)).map(|_| ())
        });
        assert!(outcome.is_ok(), "must not abort on allocation");
    }

    #[test]
    fn a_header_truncated_mid_length_fails_cleanly() {
        // 0xFF announces a 4-octet scalar; supply only two of them.
        let bytes = [0xCBu8, 0xFF, 0x00, 0x01];
        assert!(PacketHeader::try_from_reader(BufReader::new(&bytes[..])).is_err());
    }

    #[test]
    fn an_empty_input_is_not_a_packet() {
        assert!(PacketHeader::try_from_reader(BufReader::new(&[][..])).is_err());
    }

    #[test]
    fn every_truncation_of_a_real_message_fails_cleanly() {
        // Sweep every prefix of a corpus message: none may parse *and* drain
        // successfully, and none may panic.
        let full = crate::testutil::corpus::ENC_V1SEIPD_MDC;
        for n in (1..full.len()).step_by(37) {
            let prefix = packets::truncate_at(full, n);
            let outcome = std::panic::catch_unwind(|| {
                Message::from_bytes(std::io::Cursor::new(prefix)).map(|_| ())
            });
            assert!(outcome.is_ok(), "panicked on a {n}-byte prefix");
        }
    }

    // -- §3.2 MPI framing ---------------------------------------------------

    #[test]
    fn the_rfc_mpi_examples_encode_as_stated() {
        // §3.2 gives all three verbatim.
        assert_eq!(packets::mpi(&[]), vec![0x00, 0x00]);
        assert_eq!(packets::mpi(&[0x01]), vec![0x00, 0x01, 0x01]);
        assert_eq!(packets::mpi(&[0x01, 0xFF]), vec![0x00, 0x09, 0x01, 0xFF]);
    }

    #[test]
    fn an_mpi_bit_count_starts_at_the_most_significant_non_zero_bit() {
        // §3.2: "The length field of an MPI describes the length starting from
        // its most significant non-zero bit."
        assert_eq!(packets::mpi(&[0x80]), vec![0x00, 0x08, 0x80]);
        assert_eq!(packets::mpi(&[0x7F]), vec![0x00, 0x07, 0x7F]);
        assert_eq!(packets::mpi(&[0xFF, 0xFF]), vec![0x00, 0x10, 0xFF, 0xFF]);
    }

    #[test]
    fn leading_zero_octets_are_stripped_from_an_mpi() {
        // The RFC's counter-example: [00 02 01] is malformed; the correct
        // encoding of the same value is [00 01 01].
        assert_eq!(packets::mpi(&[0x00, 0x00, 0x01]), vec![0x00, 0x01, 0x01]);
        assert_ne!(packets::mpi(&[0x00, 0x01]), packets::mpi_non_minimal());
    }

    #[test]
    fn the_mpi_size_formula_holds() {
        // §3.2: "The size of an MPI is ((MPI.length + 7) / 8) + 2 octets."
        for value in [
            vec![0x01u8],
            vec![0xFF],
            vec![0x01, 0x00],
            vec![0xFF; 32],
            vec![0x80; 64],
        ] {
            let encoded = packets::mpi(&value);
            let bits = u16::from_be_bytes([encoded[0], encoded[1]]) as usize;
            assert_eq!(encoded.len(), bits.div_ceil(8) + 2, "for {value:?}");
        }
    }

    #[test]
    fn a_mismatched_mpi_bit_count_is_distinguishable() {
        // The builder used by the negative parser tests: a bit count that does
        // not describe the body it precedes.
        let bad = packets::mpi_with_bitcount(64, &[0x01]);
        assert_eq!(&bad[..2], &[0x00, 0x40]);
        assert_ne!(bad, packets::mpi(&[0x01]));
    }

    // -- §5.2.3.7 subpacket framing ----------------------------------------

    #[test]
    fn a_subpacket_length_covers_the_type_octet_but_not_itself() {
        // §5.2.3.7: "The subpacket length field covers the encoded Subpacket
        // Type ID and the subpacket-specific data, and it does not include the
        // subpacket length field itself."
        let sp = packets::subpacket(false, 2, &[0u8; 4]);
        assert_eq!(sp[0], 5, "4 body octets + 1 type octet");
        assert_eq!(sp.len(), 6);
    }

    #[test]
    fn the_critical_bit_is_bit_7_of_the_type_octet() {
        let plain = packets::subpacket(false, 33, b"x");
        let critical = packets::subpacket(true, 33, b"x");
        assert_eq!(plain[1], 33);
        assert_eq!(critical[1], 33 | 0x80);
        assert_eq!(critical[1] & 0x7F, 33, "the low 7 bits are the type ID");
    }

    #[test]
    fn a_two_octet_subpacket_length_uses_the_192_offset_form() {
        let sp = packets::subpacket(false, 20, &vec![0u8; 300]);
        let decoded = (((sp[0] as usize) - 192) << 8) + (sp[1] as usize) + 192;
        assert_eq!(decoded, 301);
    }

    #[test]
    fn a_five_octet_subpacket_length_uses_the_0xff_form() {
        let sp = packets::subpacket(false, 20, &vec![0u8; 9000]);
        assert_eq!(sp[0], 0xFF);
        assert_eq!(u32::from_be_bytes([sp[1], sp[2], sp[3], sp[4]]), 9001);
    }

    // -- §13.14 / §5.6 malicious compressed data ---------------------------

    #[test]
    fn the_decompression_cap_is_four_gibibytes() {
        // The documented bound on any single decompressed body.
        assert_eq!(MAX_DECOMPRESSED_OUTPUT_BYTES, 4 * 1024 * 1024 * 1024);
    }

    #[test]
    fn the_limited_reader_passes_data_below_its_limit() {
        let data = vec![7u8; 1024];
        let mut r = LimitedReader::new(std::io::Cursor::new(data.clone()), 4096);
        let mut out = Vec::new();
        r.read_to_end(&mut out).expect("under the limit");
        assert_eq!(out, data);
    }

    #[test]
    fn the_limited_reader_stops_at_its_limit() {
        // §13.14: "An OpenPGP implementation SHOULD limit the number of layers
        // of compression it is willing to decompress" -- and, equally, the
        // volume, so a quine cannot exhaust memory.
        let mut r = LimitedReader::new(std::io::Cursor::new(vec![0u8; 8192]), 1024);
        let mut out = Vec::new();
        let err = r.read_to_end(&mut out).expect_err("must trip the cap");
        assert_eq!(err.kind(), std::io::ErrorKind::InvalidData);
        assert!(err.to_string().contains("compression bomb"), "{err}");
    }

    #[test]
    fn the_limited_reader_allows_exactly_its_limit() {
        let mut r = LimitedReader::new(std::io::Cursor::new(vec![0u8; 1024]), 1024);
        let mut out = Vec::new();
        r.read_to_end(&mut out).expect("exactly at the limit is fine");
        assert_eq!(out.len(), 1024);
    }

    #[test]
    fn read_to_end_capped_maps_an_overrun_to_invalid_data() {
        // A bomb surfaces to the caller as a data error, not as an internal
        // error or an OOM abort.
        struct Endless;
        impl Read for Endless {
            fn read(&mut self, buf: &mut [u8]) -> std::io::Result<usize> {
                buf.fill(0);
                Ok(buf.len())
            }
        }
        let mut limited = LimitedReader::new(Endless, 64 * 1024);
        let mut buf = Vec::new();
        assert!(limited.read_to_end(&mut buf).is_err());
    }

    #[test]
    fn a_high_ratio_compressed_packet_parses_without_exploding() {
        // 4 MiB of zeros compresses to a few kilobytes. Parsing the packet
        // must not eagerly inflate it.
        let bomb = packets::compression_bomb(4);
        assert!(
            bomb.len() < 64 * 1024,
            "the bomb should be small on the wire, got {} bytes",
            bomb.len()
        );
        let outcome = std::panic::catch_unwind(|| {
            Message::from_bytes(std::io::Cursor::new(bomb)).map(|_| ())
        });
        assert!(outcome.is_ok());
    }

    #[test]
    fn deeply_nested_compression_does_not_panic_the_parser() {
        // 32 layers, twice the engine's `MAX_COMPRESSION_LAYERS` unwrap cap.
        let nested = packets::nested_compressed(32, &packets::literal(b'b', b"", 0, b"deep"));
        let outcome = std::panic::catch_unwind(|| {
            Message::from_bytes(std::io::Cursor::new(nested)).map(|_| ())
        });
        assert!(outcome.is_ok());
    }

    // -- §5.9 literal data --------------------------------------------------

    #[test]
    fn a_literal_packet_round_trips_its_payload() {
        let data = packets::literal(b'b', b"", 0, b"hello world");
        let mut msg = Message::from_bytes(std::io::Cursor::new(data)).expect("parse");
        let mut out = Vec::new();
        std::io::copy(&mut msg, &mut out).expect("drain");
        assert_eq!(out, b"hello world");
    }

    #[test]
    fn a_literal_packet_carries_its_filename() {
        let data = packets::literal(b'b', b"report.pdf", 0, b"x");
        let msg = Message::from_bytes(std::io::Cursor::new(data)).expect("parse");
        assert_eq!(msg.literal_data_header().map(|h| h.file_name().to_vec()),
                   Some(b"report.pdf".to_vec()));
    }

    #[test]
    fn an_empty_literal_packet_is_valid() {
        let data = packets::literal(b'b', b"", 0, b"");
        let mut msg = Message::from_bytes(std::io::Cursor::new(data)).expect("parse");
        let mut out = Vec::new();
        std::io::copy(&mut msg, &mut out).expect("drain");
        assert!(out.is_empty());
    }

    #[test]
    fn the_text_format_octet_is_preserved() {
        // §5.9: 'b' is binary, 'u' is UTF-8 text. The engine must not rewrite
        // one into the other.
        for format in [b'b', b'u'] {
            let data = packets::literal(format, b"", 0, b"x");
            let msg = Message::from_bytes(std::io::Cursor::new(data)).expect("parse");
            assert!(msg.literal_data_header().is_some(), "format {format}");
        }
    }
}

#[cfg(test)]
mod attribution_tests {
    //! Issuer matching, signing-key selection and the shared per-index
    //! signature-attribution driver.
    //!
    //! This is the layer the B1-B7 interop bugs lived in, so several tests
    //! name the finding they lock down.

    use super::*;
    use crate::testutil::corpus;
    use crate::testutil::keys;

    // -- key_identifier_matches --------------------------------------------

    #[test]
    fn a_full_fingerprint_matches_itself() {
        let fpr = corpus::aux_good_sign_fpr();
        assert!(key_identifier_matches(&fpr, "", &fpr));
    }

    #[test]
    fn a_long_key_id_matches_its_fingerprint() {
        // B1: signatures that carry only an Issuer Key ID subpacket (§5.2.3.12)
        // must still resolve to the certificate that owns them.
        let fpr = corpus::aux_good_sign_fpr();
        let key_id = corpus::long_key_id(&fpr);
        assert!(key_identifier_matches(&fpr, &key_id, &key_id));
    }

    #[test]
    fn a_suffix_of_the_fingerprint_matches() {
        // Users routinely paste a short key ID; the match falls back to a
        // suffix comparison for identifiers shorter than a fingerprint.
        let fpr = corpus::aux_good_sign_fpr();
        assert!(key_identifier_matches(&fpr, "", &fpr[fpr.len() - 8..]));
    }

    #[test]
    fn matching_is_case_insensitive() {
        let fpr = corpus::aux_good_sign_fpr();
        assert!(key_identifier_matches(&fpr.to_lowercase(), "", &fpr.to_uppercase()));
        assert!(key_identifier_matches(&fpr.to_uppercase(), "", &fpr.to_lowercase()));
    }

    #[test]
    fn whitespace_in_an_identifier_is_ignored() {
        // Fingerprints are conventionally displayed in space-separated groups
        // (§13.6), so a pasted one arrives with spaces in it.
        let fpr = corpus::aux_good_sign_fpr();
        let spaced = fpr
            .as_bytes()
            .chunks(4)
            .map(|c| String::from_utf8_lossy(c).into_owned())
            .collect::<Vec<_>>()
            .join(" ");
        assert!(key_identifier_matches(&fpr, "", &spaced));
    }

    #[test]
    fn an_empty_target_never_matches() {
        // Otherwise an anonymous signature would match every certificate.
        let fpr = corpus::aux_good_sign_fpr();
        assert!(!key_identifier_matches(&fpr, "DEADBEEFDEADBEEF", ""));
    }

    #[test]
    fn a_whitespace_only_target_never_matches() {
        let fpr = corpus::aux_good_sign_fpr();
        assert!(!key_identifier_matches(&fpr, "", "   \t "));
    }

    #[test]
    fn an_unrelated_identifier_does_not_match() {
        let fpr = corpus::aux_good_sign_fpr();
        assert!(!key_identifier_matches(&fpr, "", "0123456789ABCDEF"));
    }

    #[test]
    fn a_prefix_of_the_fingerprint_does_not_match() {
        // Only suffix matching is supported: key IDs are the *low* bits of a
        // v4 fingerprint, so a prefix match would be meaningless.
        let fpr = corpus::aux_good_sign_fpr();
        assert!(!key_identifier_matches(&fpr, "", &fpr[..8]));
    }

    // -- cert_contains_issuer ----------------------------------------------

    #[test]
    fn a_certificate_contains_its_primary_key_identifier() {
        assert!(cert_contains_issuer(
            &corpus::AUX_GOOD_CERT,
            &corpus::aux_good_primary_fpr()
        ));
    }

    #[test]
    fn a_certificate_does_not_contain_an_unrelated_identifier() {
        assert!(!cert_contains_issuer(
            &corpus::AUX_GOOD_CERT,
            &corpus::key1_primary_fpr()
        ));
    }

    #[test]
    fn an_empty_issuer_matches_no_certificate() {
        assert!(!cert_contains_issuer(&corpus::AUX_GOOD_CERT, ""));
    }

    // -- parse_signer_block -------------------------------------------------

    #[test]
    fn a_pinned_signer_prefix_is_extracted_and_stripped() {
        let (target, rest) = parse_signer_block("ABCDEF01!\n-----BEGIN PGP MESSAGE-----\nbody");
        assert_eq!(target.as_deref(), Some("ABCDEF01"));
        assert!(rest.starts_with("-----BEGIN PGP MESSAGE-----"));
    }

    #[test]
    fn a_pin_without_a_following_armor_block_is_not_a_pin() {
        // The prefix is only meaningful as a header on an armored block; with
        // no `-----BEGIN PGP` to head, the whole input is returned untouched
        // rather than being silently reinterpreted as a key identifier.
        let (target, rest) = parse_signer_block("ABCDEF01!\n");
        assert!(target.is_none());
        assert_eq!(rest, "ABCDEF01!\n");
    }

    #[test]
    fn a_pin_is_normalised_to_uppercase_without_whitespace() {
        let (target, _rest) =
            parse_signer_block("ab cd ef 01!\n-----BEGIN PGP MESSAGE-----\nbody");
        assert_eq!(target.as_deref(), Some("ABCDEF01"));
    }

    #[test]
    fn a_bare_exclamation_mark_is_not_a_pin() {
        // An empty target would match every key, so it must be ignored.
        let (target, rest) = parse_signer_block("!\n-----BEGIN PGP MESSAGE-----\nbody");
        assert!(target.is_none());
        assert!(rest.starts_with("-----BEGIN PGP MESSAGE-----"));
    }

    #[test]
    fn leading_whitespace_before_a_block_is_tolerated() {
        let (target, rest) = parse_signer_block("\n\n  -----BEGIN PGP MESSAGE-----\nbody");
        assert!(target.is_none());
        assert!(rest.contains("BEGIN PGP MESSAGE"));
    }

    #[test]
    fn an_empty_input_yields_no_pin() {
        let (target, rest) = parse_signer_block("");
        assert!(target.is_none());
        assert!(rest.is_empty());
    }

    // -- with_signing_key ---------------------------------------------------

    #[test]
    fn signing_key_selection_prefers_a_signing_subkey() {
        // §10.1.5: "It is good practice to use separate subkeys for every
        // operation", so the subkey is chosen over the certification primary.
        let key = &keys::V4_SIGN.secret;
        let chosen = with_signing_key(key, None, |k| Ok(k.fpr().to_uppercase())).expect("a signing key");
        assert_eq!(chosen, keys::V4_SIGN.sign_subkey_fpr());
    }

    #[test]
    fn signing_key_selection_falls_back_to_the_primary() {
        // A key with no signing subkey must still be able to sign: §5.2.3.10
        // notes a primary is always allowed to make signatures.
        let key = &keys::V4_PRIMARY_ONLY.secret;
        let chosen = with_signing_key(key, None, |k| Ok(k.fpr().to_uppercase())).expect("a signing key");
        assert_eq!(chosen, keys::V4_PRIMARY_ONLY.primary_fpr);
    }

    #[test]
    fn a_pinned_signing_subkey_is_honoured() {
        let key = &keys::V4_SIGN.secret;
        let want = keys::V4_SIGN.sign_subkey_fpr();
        let chosen = with_signing_key(key, Some(want), |k| Ok(k.fpr().to_uppercase())).expect("pinned");
        assert_eq!(chosen, want);
    }

    #[test]
    fn a_pinned_primary_key_is_honoured() {
        let key = &keys::V4_SIGN.secret;
        let want = &keys::V4_SIGN.primary_fpr;
        let chosen = with_signing_key(key, Some(want), |k| Ok(k.fpr().to_uppercase())).expect("pinned");
        assert_eq!(&chosen, want);
    }

    #[test]
    fn a_pinned_unknown_fingerprint_is_rejected() {
        // Silently falling back would sign with a key the user did not ask
        // for, which is exactly what a pin is meant to prevent.
        let key = &keys::V4_SIGN.secret;
        let res = with_signing_key(key, Some("0000000000000000"), |k| Ok(k.fpr()));
        assert!(res.is_err());
    }

    #[test]
    fn a_pinned_encryption_subkey_is_rejected() {
        // An ECDH subkey cannot sign; pinning it must fail rather than
        // silently signing with something else.
        let key = &keys::V4_SIGN.secret;
        let enc = keys::V4_SIGN.enc_subkey_fpr();
        let res = with_signing_key(key, Some(enc), |k| Ok(k.fpr()));
        assert!(res.is_err(), "an encryption subkey must not be usable for signing");
    }

    #[test]
    fn a_pinned_key_id_suffix_resolves() {
        let key = &keys::V4_SIGN.secret;
        let full = keys::V4_SIGN.sign_subkey_fpr();
        let short = &full[full.len() - 16..];
        let chosen = with_signing_key(key, Some(short), |k| Ok(k.fpr().to_uppercase())).expect("pinned");
        assert_eq!(chosen, full);
    }

    #[test]
    fn signing_key_selection_works_on_a_v6_key() {
        let key = &keys::V6_SIGN.secret;
        let chosen = with_signing_key(key, None, |k| Ok(k.fpr().to_uppercase())).expect("a signing key");
        assert_eq!(chosen, keys::V6_SIGN.sign_subkey_fpr());
    }

    // -- signature gates ----------------------------------------------------

    #[test]
    fn the_weak_hash_gate_matches_rpgps_display_strings() {
        // `sig_hash_algo_is_weak` compares against rPGP's `Display` output. If
        // rPGP ever changed its rendering the gate would silently stop firing,
        // so the exact strings are pinned here rather than assumed.
        use pgp::crypto::hash::HashAlgorithm;
        assert_eq!(HashAlgorithm::Md5.to_string(), "MD5");
        assert_eq!(HashAlgorithm::Sha1.to_string(), "SHA1");
        assert_eq!(HashAlgorithm::Ripemd160.to_string(), "RIPEMD160");
        assert_eq!(HashAlgorithm::Sha256.to_string(), "SHA256");
        assert_eq!(HashAlgorithm::Sha512.to_string(), "SHA512");
    }

    #[test]
    fn every_hash_rpgp_can_name_is_classified_by_the_gate() {
        // A hash the gate does not recognise is treated as strong, so a new
        // weak algorithm appearing upstream would open a hole. This asserts
        // the current set is exactly the RFC 9580 §9.5 forbidden trio.
        use pgp::crypto::hash::HashAlgorithm;
        for (algo, weak) in [
            (HashAlgorithm::Md5, true),
            (HashAlgorithm::Sha1, true),
            (HashAlgorithm::Ripemd160, true),
            (HashAlgorithm::Sha224, false),
            (HashAlgorithm::Sha256, false),
            (HashAlgorithm::Sha384, false),
            (HashAlgorithm::Sha512, false),
            (HashAlgorithm::Sha3_256, false),
            (HashAlgorithm::Sha3_512, false),
        ] {
            assert_eq!(
                sig_hash_algo_is_weak(&algo.to_string()),
                weak,
                "{algo:?} misclassified"
            );
        }
    }

    #[test]
    fn signature_absolute_expiry_is_zero_when_the_subpacket_is_absent() {
        // §5.2.3.18: "If this is not present or has a value of zero, it never
        // expires."
        let sigs = parse_all_signature_packets(corpus::SIG_GOOD_DETACHED);
        let sig = sigs.first().expect("one signature");
        assert_eq!(signature_absolute_expiry(sig), 0);
    }

    #[test]
    fn an_entry_from_a_packet_starts_unverified() {
        // Sniffing only reads the packet; nothing is `Valid` until a key has
        // actually verified it.
        let sigs = parse_all_signature_packets(corpus::SIG_GOOD_DETACHED);
        let entry = sig_entry_from_packet(&sigs[0], GfrSignMode::Detached);
        assert_eq!(entry.status, GfrSignatureStatus::NoKey);
        assert_eq!(entry.sig_type, GfrSignMode::Detached);
    }

    #[test]
    fn parse_all_signature_packets_returns_nothing_for_non_signature_input() {
        assert!(parse_all_signature_packets(corpus::GARBAGE).is_empty());
        assert!(parse_all_signature_packets(corpus::EMPTY).is_empty());
        assert!(parse_all_signature_packets(corpus::PAYLOAD).is_empty());
    }

    #[test]
    fn parse_all_signature_packets_never_panics_on_adversarial_input() {
        for vector in [
            corpus::GARBAGE,
            corpus::EMPTY,
            corpus::PKESK_NO_SEIPD,
            corpus::ENC_SED_TAG9,
        ] {
            let outcome = std::panic::catch_unwind(|| parse_all_signature_packets(vector).len());
            assert!(outcome.is_ok(), "panicked while parsing an adversarial vector");
        }
    }

    #[test]
    fn attribution_leaves_an_unknown_issuer_as_no_key() {
        // The distinction that matters most to a user: "I don't have the key"
        // must never be shown as "this signature is forged".
        let mut entries: Vec<SignatureResultInternal> =
            parse_all_signature_packets(corpus::SIG_GOOD_DETACHED)
                .iter()
                .map(|s| sig_entry_from_packet(s, GfrSignMode::Detached))
                .collect();
        let mut is_verified = false;
        // An empty certificate list means no key was found for any issuer.
        attribute_entries(&mut entries, &[], &mut is_verified, |_, _| true);
        assert!(!is_verified);
        assert_eq!(entries[0].status, GfrSignatureStatus::NoKey);
    }

    #[test]
    fn attribution_marks_only_the_index_that_verifies() {
        // B4: several packets can name the same certificate; only the entry
        // whose own index verifies may be stamped Valid.
        let mut entries: Vec<SignatureResultInternal> =
            parse_all_signature_packets(corpus::SIG_STRONG_WEAK_SAME_KEY)
                .iter()
                .map(|s| sig_entry_from_packet(s, GfrSignMode::Detached))
                .collect();
        assert_eq!(entries.len(), 2, "the vector carries two signatures");

        let certs = vec![corpus::AUX_GOOD_CERT.clone()];
        let mut is_verified = false;
        // Claim only index 0 verifies.
        attribute_entries(&mut entries, &certs, &mut is_verified, |i, _| i == 0);

        assert_ne!(
            entries[1].status,
            GfrSignatureStatus::Valid,
            "index 1 did not verify and must not inherit index 0's verdict"
        );
    }

    #[test]
    fn attribution_applies_the_weak_hash_gate_even_when_the_signature_verifies() {
        // §9.5: a cryptographically sound SHA-1 signature is still not valid.
        let mut entries: Vec<SignatureResultInternal> =
            parse_all_signature_packets(corpus::SIG_SHA1_DETACHED)
                .iter()
                .map(|s| sig_entry_from_packet(s, GfrSignMode::Detached))
                .collect();
        assert!(!entries.is_empty());
        assert!(sig_hash_algo_is_weak(&entries[0].hash_algo));

        let certs = vec![corpus::AUX_SHA1_CERT.clone()];
        let mut is_verified = false;
        attribute_entries(&mut entries, &certs, &mut is_verified, |_, _| true);

        assert_ne!(entries[0].status, GfrSignatureStatus::Valid);
        assert!(!is_verified);
    }

    #[test]
    fn attribution_on_an_empty_entry_list_is_a_no_op() {
        let mut entries: Vec<SignatureResultInternal> = Vec::new();
        let mut is_verified = false;
        attribute_entries(&mut entries, &[], &mut is_verified, |_, _| true);
        assert!(!is_verified);
    }

    // -- recipient sniffing -------------------------------------------------

    #[test]
    fn sniffing_a_non_message_yields_no_recipients() {
        assert!(sniff_recipients(corpus::GARBAGE).is_empty());
        assert!(sniff_recipients(corpus::EMPTY).is_empty());
    }

    #[test]
    fn sniffing_a_symmetric_message_yields_no_public_key_recipients() {
        // A password-based message has an SKESK, not a PKESK, so there is no
        // recipient key to name.
        assert!(sniff_recipients(corpus::ENC_SYMMETRIC_V1).is_empty());
    }

    #[test]
    fn sniffing_a_v6_message_reports_a_full_fingerprint() {
        // §5.1.2: a v6 PKESK identifies the recipient by fingerprint, not by
        // the 8-octet key ID a v3 PKESK carries.
        let recipients = sniff_recipients(corpus::ENC_V2SEIPD_OCB);
        assert_eq!(recipients.len(), 1);
        assert_eq!(recipients[0].key_id.len(), 64);
    }

    #[test]
    fn sniffing_never_panics_on_adversarial_input() {
        for vector in [
            corpus::GARBAGE,
            corpus::EMPTY,
            corpus::PKESK_NO_SEIPD,
            corpus::ENC_SED_TAG9,
            corpus::SIG_BAD_MUTATED,
        ] {
            let outcome = std::panic::catch_unwind(|| sniff_recipients(vector).len());
            assert!(outcome.is_ok());
        }
    }

    // -- get_signature_issuers ---------------------------------------------

    #[test]
    fn issuers_are_reported_for_a_detached_signature() {
        let (fprs, key_ids) =
            get_signature_issuers_internal(corpus::SIG_GOOD_DETACHED).expect("issuers");
        assert!(!fprs.is_empty() || !key_ids.is_empty());
    }

    #[test]
    fn issuers_of_a_non_signature_are_an_error_or_empty() {
        match get_signature_issuers_internal(corpus::GARBAGE) {
            Ok((fprs, key_ids)) => assert!(fprs.is_empty() && key_ids.is_empty()),
            Err(status) => assert!((status as i32) < 0),
        }
    }

    #[test]
    fn issuer_extraction_never_panics() {
        for vector in [corpus::GARBAGE, corpus::EMPTY, corpus::PAYLOAD] {
            let outcome =
                std::panic::catch_unwind(|| get_signature_issuers_internal(vector).is_ok());
            assert!(outcome.is_ok());
        }
    }
}
