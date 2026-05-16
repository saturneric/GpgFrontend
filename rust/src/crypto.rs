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

use crate::{
    crypto_stream::{
        decrypt_and_verify_stream_internal, encrypt_and_sign_stream_internal,
        encrypt_stream_internal, sign_stream_internal,
    },
    err::IntoGfrResult,
    types::{
        GfrFreeCb, GfrPasswordFetchCb, GfrPublicKeyFetchCb, GfrRecipientStatus,
        GfrSecretKeyFetchCb, GfrSignMode, GfrSignatureStatus, GfrStatus,
    },
};
use pgp::{
    armor::Dearmor,
    composed::{
        ArmorOptions, CleartextSignedMessage, Deserializable, DetachedSignature, Message,
        SignedPublicKey, SignedSecretKey,
    },
    crypto::public_key::PublicKeyAlgorithm,
    packet::{Packet, PacketParser, SecretKey, SecretSubkey},
    types::{KeyDetails, SecretParams},
};
use std::{
    ffi::c_void,
    io::{Cursor, Read},
};

pub struct InvalidRecipientInternal {
    pub fpr: String,
    pub reason: GfrStatus,
}

pub struct EncryptResultInternal {
    pub data: Vec<u8>,
    pub invalid_recipients: Vec<InvalidRecipientInternal>,
}

pub fn encrypt_internal(
    name: &str,
    data: &[u8],
    public_key_blocks: &[&str],
    ascii_armor: bool,
) -> Result<EncryptResultInternal, GfrStatus> {
    let mut output = Vec::new();

    let stream_result =
        encrypt_stream_internal(name, data, &mut output, public_key_blocks, ascii_armor)?;

    Ok(EncryptResultInternal {
        data: output,
        invalid_recipients: stream_result.invalid_recipients,
    })
}

pub struct RecipientResultInternal {
    pub key_id: String, // PGP PKESK only exposes 16-char Key ID, not full Fingerprint
    pub pub_algo: String,
    pub status: GfrRecipientStatus,
}

pub struct DecryptResultInternal {
    pub data: Vec<u8>,
    pub filename: String,
    pub recipients: Vec<RecipientResultInternal>,
}

pub fn decrypt_internal(
    channel: i32,
    encrypted_data: &[u8],
    fetch_seckey_cb: Option<GfrSecretKeyFetchCb>,
    fetch_pwd_cb: Option<GfrPasswordFetchCb>,
    free_cb: Option<GfrFreeCb>,
    user_data: *mut c_void,
) -> Result<DecryptResultInternal, GfrStatus> {
    let mut output_data = Vec::new();

    // Auto-detect ASCII armor by checking for the standard PGP header.
    // If it's not valid UTF-8, it's definitely a binary packet.
    let is_armor = std::str::from_utf8(encrypted_data)
        .map(|s| s.trim_start().starts_with("-----BEGIN PGP MESSAGE-----"))
        .unwrap_or(false);

    // Wrap the in-memory byte slice into a Read stream
    let input_cursor = Cursor::new(encrypted_data);

    // Delegate all the heavy lifting to the stream implementation
    let stream_result = decrypt_and_verify_stream_internal(
        channel,
        input_cursor,
        &mut output_data, // Passes as Write stream
        is_armor,
        fetch_seckey_cb,
        fetch_pwd_cb,
        None, // fetch_pubkey_cb is not needed for decryption-only
        free_cb,
        user_data,
    )?;

    Ok(DecryptResultInternal {
        data: output_data,
        filename: stream_result.filename,
        recipients: stream_result.recipients,
    })
}

pub struct SignResultInternal {
    pub data: Vec<u8>,
    pub signatures: Vec<SignatureResultInternal>,
}

/// Helper to extract exact target (!) from the armored string prefix
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

/// A typed wrapper to avoid Trait Object (&dyn) issues when passing to builder methods
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

    pub fn algorithm(&self) -> PublicKeyAlgorithm {
        match self {
            SelectedKey::Primary(k) => k.algorithm(),
            SelectedKey::Sub(k) => k.algorithm(),
        }
    }
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
    // NORMAL MODE (Auto Fallback)
    // ==========================================
    for subkey in &skey.secret_subkeys {
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

pub fn sign_internal(
    channel: i32,
    name: &str,
    data: &[u8],
    secret_key_blocks: &[&str],
    fetch_cb: Option<GfrPasswordFetchCb>,
    free_cb: Option<GfrFreeCb>,
    mode: GfrSignMode,
    ascii_armor: bool,
) -> Result<SignResultInternal, GfrStatus> {
    let mut output_data = Vec::new();
    let input_cursor = Cursor::new(data);

    let stream_result = sign_stream_internal(
        channel,
        name,
        input_cursor,
        &mut output_data,
        secret_key_blocks,
        fetch_cb,
        free_cb,
        mode,
        ascii_armor,
    )?;

    Ok(SignResultInternal {
        data: output_data,
        signatures: stream_result.signatures,
    })
}

pub struct VerifyResultInternal {
    pub data: Vec<u8>,
    pub is_verified: bool,
    pub signatures: Vec<SignatureResultInternal>,
}

pub struct SignatureResultInternal {
    pub fpr: String,
    pub status: GfrSignatureStatus,
    pub created_at: u32,
    pub pub_algo: String,
    pub hash_algo: String,
    pub sig_type: GfrSignMode,
}

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

pub fn algo_to_string_simple(algo: PublicKeyAlgorithm) -> String {
    // Uses the derived Debug trait to get the variant name as a String
    format!("{:?}", algo)
}

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
    free_cb: Option<GfrFreeCb>,
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
                if let Some(f_cb) = free_cb {
                    f_cb(c_key_block as *mut std::ffi::c_void, user_data);
                }
            }
        }
    }
    certs
}

pub fn verify_internal(
    data: &[u8],
    sig_data: &[u8], // Used only for Detached mode
    mode: GfrSignMode,
    fetch_pubkey_cb: Option<GfrPublicKeyFetchCb>,
    free_cb: Option<GfrFreeCb>,
    user_data: *mut std::ffi::c_void,
) -> Result<VerifyResultInternal, GfrStatus> {
    // Helper closure to update signature statuses uniformly across all modes.
    let update_signatures = |cert: &SignedPublicKey,
                             is_cert_valid: bool,
                             signatures: &mut Vec<SignatureResultInternal>,
                             is_verified: &mut bool|
     -> bool {
        let mut found = false;
        for sig in signatures.iter_mut() {
            if cert_contains_issuer(cert, &sig.fpr) {
                found = true;
                if is_cert_valid {
                    sig.status = GfrSignatureStatus::Valid;
                    *is_verified = true;
                } else if sig.status == GfrSignatureStatus::NoKey {
                    sig.status = GfrSignatureStatus::BadSignature;
                }
            }
        }
        found
    };

    match mode {
        // ---------------------------------------------------------
        // MODE 0: INLINE SIGNATURE
        // ---------------------------------------------------------
        GfrSignMode::Inline => {
            let mut msg = Message::from_armor(Cursor::new(data))
                .map(|(m, _)| m)
                .or_else(|_| Message::from_bytes(data))
                .into_gfr()?;

            let mut signatures = sniff_signatures(data, mode);
            let certs =
                fetch_certs_for_signatures(&signatures, fetch_pubkey_cb, free_cb, user_data);
            let mut is_verified = false;

            for cert in &certs {
                let is_cert_valid = msg.verify(cert).is_ok()
                    || cert.public_subkeys.iter().any(|sk| msg.verify(sk).is_ok());

                let found =
                    update_signatures(cert, is_cert_valid, &mut signatures, &mut is_verified);

                // Special fallback if verification succeeded but issuer wasn't caught by sniff_signatures
                if is_cert_valid && !found {
                    signatures.push(SignatureResultInternal {
                        fpr: cert.primary_key.fingerprint().to_string(),
                        status: GfrSignatureStatus::Valid,
                        created_at: 0,
                        pub_algo: "None".to_string(),
                        hash_algo: "None".to_string(),
                        sig_type: mode,
                    });
                    is_verified = true;
                }
            }

            let clear_data = msg.as_data_vec().into_gfr()?;
            Ok(VerifyResultInternal {
                data: clear_data,
                is_verified,
                signatures,
            })
        }

        // ---------------------------------------------------------
        // MODE 1: CLEARTEXT SIGNATURE
        // ---------------------------------------------------------
        GfrSignMode::ClearText => {
            let text_str = std::str::from_utf8(data).map_err(|_| GfrStatus::ErrorInvalidInput)?;
            let (msg, _) = CleartextSignedMessage::from_string(text_str).into_gfr()?;

            // Sniff directly from the parsed message
            let mut signatures = Vec::new();
            for sig in msg.signatures().iter() {
                for issuer in sig.issuer_fingerprint() {
                    let fpr = issuer.to_string();
                    if !signatures
                        .iter()
                        .any(|r: &SignatureResultInternal| r.fpr == fpr)
                    {
                        let (hash_algo, pub_algo) = sig
                            .config()
                            .map(|c| (c.hash_alg.to_string(), algo_to_string_simple(c.pub_alg)))
                            .unwrap_or_default();

                        signatures.push(SignatureResultInternal {
                            fpr,
                            status: GfrSignatureStatus::NoKey,
                            created_at: sig.created().map(|d| d.as_secs()).unwrap_or(0),
                            pub_algo,
                            hash_algo,
                            sig_type: mode,
                        });
                    }
                }
            }

            let certs =
                fetch_certs_for_signatures(&signatures, fetch_pubkey_cb, free_cb, user_data);
            let mut is_verified = false;

            for cert in &certs {
                let is_cert_valid = msg.verify(cert).is_ok()
                    || cert.public_subkeys.iter().any(|sk| msg.verify(sk).is_ok());

                update_signatures(cert, is_cert_valid, &mut signatures, &mut is_verified);
            }

            let clear_data = msg
                .to_armored_bytes(ArmorOptions::default())
                .unwrap_or_default();
            Ok(VerifyResultInternal {
                data: clear_data,
                is_verified,
                signatures,
            })
        }

        // ---------------------------------------------------------
        // MODE 2: DETACHED SIGNATURE
        // ---------------------------------------------------------
        GfrSignMode::Detached => {
            if sig_data.is_empty() {
                return Err(GfrStatus::ErrorInvalidInput);
            }

            let sig_msg = DetachedSignature::from_armor_single(Cursor::new(sig_data))
                .map(|(s, _)| s)
                .or_else(|_| DetachedSignature::from_bytes(sig_data))
                .into_gfr()?;

            let mut signatures = sniff_signatures(sig_data, mode);
            let certs =
                fetch_certs_for_signatures(&signatures, fetch_pubkey_cb, free_cb, user_data);
            let mut is_verified = false;

            for cert in &certs {
                let is_cert_valid = sig_msg.verify(cert, data).is_ok()
                    || cert
                        .public_subkeys
                        .iter()
                        .any(|sk| sig_msg.verify(sk, data).is_ok());

                update_signatures(cert, is_cert_valid, &mut signatures, &mut is_verified);
            }

            Ok(VerifyResultInternal {
                data: Vec::new(),
                is_verified,
                signatures,
            })
        }
    }
}

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

// Internal struct combining both signatures created and invalid recipients skipped
pub struct EncryptAndSignResultInternal {
    pub data: Vec<u8>,
    pub signatures: Vec<SignatureResultInternal>,
    pub invalid_recipients: Vec<InvalidRecipientInternal>,
}

pub fn encrypt_and_sign_internal(
    channel: i32,
    name: &str,
    data: &[u8],
    public_key_blocks: &[&str],
    secret_key_blocks: &[&str],
    fetch_cb: Option<GfrPasswordFetchCb>,
    free_cb: Option<GfrFreeCb>,
    ascii_armor: bool,
) -> Result<EncryptAndSignResultInternal, GfrStatus> {
    let mut output_data = Vec::new();
    let input_cursor = Cursor::new(data);

    // Delegate processing to the streaming pipeline
    let stream_result = encrypt_and_sign_stream_internal(
        channel,
        name,
        input_cursor,
        &mut output_data, // Safely handles the output as a Write stream
        public_key_blocks,
        secret_key_blocks,
        fetch_cb,
        free_cb,
        ascii_armor,
    )?;

    Ok(EncryptAndSignResultInternal {
        data: output_data,
        signatures: stream_result.signatures,
        invalid_recipients: stream_result.invalid_recipients,
    })
}
// Internal struct for the combined Decrypt + Verify operation
pub struct DecryptAndVerifyResultInternal {
    pub data: Vec<u8>,
    pub filename: String,
    pub recipients: Vec<RecipientResultInternal>,
    pub is_verified: bool,
    pub signatures: Vec<SignatureResultInternal>,
}

pub fn decrypt_and_verify_internal(
    channel: i32,
    encrypted_data: &[u8],
    fetch_seckey_cb: Option<GfrSecretKeyFetchCb>,
    fetch_pwd_cb: Option<GfrPasswordFetchCb>,
    fetch_pubkey_cb: Option<GfrPublicKeyFetchCb>,
    free_cb: Option<GfrFreeCb>,
    user_data: *mut std::ffi::c_void,
) -> Result<DecryptAndVerifyResultInternal, GfrStatus> {
    let mut output_data = Vec::new();

    let is_armor = std::str::from_utf8(encrypted_data)
        .map(|s| s.trim_start().starts_with("-----BEGIN PGP MESSAGE-----"))
        .unwrap_or(false);

    let input_cursor = Cursor::new(encrypted_data);

    let stream_result = crate::crypto_stream::decrypt_and_verify_stream_internal(
        channel,
        input_cursor,
        &mut output_data,
        is_armor,
        fetch_seckey_cb,
        fetch_pwd_cb,
        fetch_pubkey_cb,
        free_cb,
        user_data,
    )?;

    Ok(DecryptAndVerifyResultInternal {
        data: output_data,
        filename: stream_result.filename,
        recipients: stream_result.recipients,
        is_verified: stream_result.is_verified,
        signatures: stream_result.signatures,
    })
}
