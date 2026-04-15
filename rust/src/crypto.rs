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
    types::{
        GfrFreeCb, GfrPasswordFetchCb, GfrPublicKeyFetchCb, GfrRecipientStatus,
        GfrSecretKeyFetchCb, GfrSignMode, GfrSignatureStatus, GfrStatus,
    },
    utils::fetch_password_internal,
};
use log::debug;
use pgp::{
    armor::Dearmor,
    composed::{
        ArmorOptions, CleartextSignedMessage, Deserializable, DetachedSignature, Message,
        MessageBuilder, SignedPublicKey, SignedSecretKey,
    },
    crypto::{hash::HashAlgorithm, public_key::PublicKeyAlgorithm, sym::SymmetricKeyAlgorithm},
    packet::{Packet, PacketParser, SecretKey, SecretSubkey},
    ser::Serialize,
    types::{KeyDetails, Password, SecretParams},
};
use rand::thread_rng;
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

// Helper to sniff all intended recipients from the encrypted data
fn sniff_recipients(data: &[u8]) -> Vec<RecipientResultInternal> {
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
                    status: GfrRecipientStatus::NoKey, // Default to NoKey until proven otherwise
                });
            }
        }
    }
    results
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

pub fn get_message_recipients_internal(data: &[u8]) -> Result<String, GfrStatus> {
    let recipients = sniff_recipients(data);
    let key_ids: Vec<String> = recipients.iter().map(|r| r.key_id.clone()).collect();

    if key_ids.is_empty() {
        return Err(GfrStatus::ErrorInvalidInput); // Not a valid encrypted message
    }

    // Join them with commas for easy FFI transfer (e.g., "A1B2C3D4E5F6G7H8,8H7G6F5E4D3C2B1A")
    Ok(key_ids.join(","))
}

pub struct SignResultInternal {
    pub data: Vec<u8>,
    pub signatures: Vec<SignatureResultInternal>,
}

/// Helper to extract exact target (!) from the armored string prefix
pub fn parse_signer_block(raw: &str) -> (Option<String>, &str) {
    if let Some(idx) = raw.find("-----BEGIN PGP") {
        let prefix = raw[..idx].trim();
        if prefix.ends_with('!') {
            let target = prefix.trim_end_matches('!').trim().to_uppercase();
            return (Some(target), &raw[idx..]);
        }
        return (None, &raw[idx..]);
    }
    (None, raw)
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

/// Helper to find the signing key (either primary or subkey) that matches the
/// specified target (if any), and execute the provided action with that key if
/// found.
/// Core routing mechanism for exact target matching (!) or automatic fallback
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
        for subkey in &skey.secret_subkeys {
            let fpr = subkey.key.fingerprint().to_string().to_uppercase();
            let kid = subkey.key.legacy_key_id().to_string().to_uppercase();

            // Match exact subkey AND ensure it has signing capabilities
            if (fpr.ends_with(target) || kid.ends_with(target)) && subkey.key.algorithm().can_sign()
            {
                return action(SelectedKey::Sub(&subkey.key));
            }
        }

        let primary_fpr = skey.primary_key.fingerprint().to_string().to_uppercase();
        let primary_kid = skey.primary_key.legacy_key_id().to_string().to_uppercase();
        if (primary_fpr.ends_with(target) || primary_kid.ends_with(target))
            && skey.primary_key.algorithm().can_sign()
        {
            return action(SelectedKey::Primary(&skey.primary_key));
        }

        return Err(crate::types::GfrStatus::ErrorInvalidInput); // Target not found
    }

    // ==========================================
    // NORMAL MODE (Auto Fallback)
    // ==========================================
    for subkey in &skey.secret_subkeys {
        if subkey.key.algorithm().can_sign() {
            return action(SelectedKey::Sub(&subkey.key));
        }
    }

    if skey.primary_key.algorithm().can_sign() {
        return action(SelectedKey::Primary(&skey.primary_key));
    }

    Err(crate::types::GfrStatus::ErrorInvalidInput) // No signing capabilities found
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
    if cert
        .primary_key
        .fingerprint()
        .to_string()
        .eq_ignore_ascii_case(issuer_hex)
    {
        return true;
    }
    for subkey in &cert.public_subkeys {
        if subkey
            .key
            .fingerprint()
            .to_string()
            .eq_ignore_ascii_case(issuer_hex)
        {
            return true;
        }
    }
    false
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
                        fpr: fpr,
                        status: GfrSignatureStatus::NoKey,
                        created_at: sig.created().map(|d| d.as_secs() as u32).unwrap_or(0),
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
                .map_err(|_| GfrStatus::ErrorInvalidInput)?;

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

            let clear_data = msg.as_data_vec().map_err(|_| GfrStatus::ErrorInternal)?;
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
            let (msg, _) = CleartextSignedMessage::from_string(text_str)
                .map_err(|_| GfrStatus::ErrorInvalidInput)?;

            // Sniff directly from the parsed message
            let mut signatures = Vec::new();
            for sig in msg.signatures().into_iter() {
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
                            created_at: sig.created().map(|d| d.as_secs() as u32).unwrap_or(0),
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
                .map_err(|_| GfrStatus::ErrorInvalidInput)?;

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
            for sig in msg.signatures().into_iter() {
                for issuer in sig.issuer_key_id() {
                    issuers.push(issuer.to_string());
                }
            }
            // Cleartext messages only contain signatures, not encrypted recipients
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

    for packet_result in parser {
        if let Ok(packet) = packet_result {
            match packet {
                // Sniff Recipient (for Encryption)
                Packet::PublicKeyEncryptedSessionKey(pkesk) => {
                    if let Ok(id) = pkesk.id() {
                        recipients.push(id.to_string());
                    }
                }
                // Sniff Signer from OnePassSignature (Appears at the start of inline signatures)
                Packet::OnePassSignature(ops) => {
                    // Match on the version-specific enum to extract the identifier
                    match ops.version_specific() {
                        pgp::packet::OpsVersionSpecific::V3 { key_id } => {
                            // V3 OPS directly contains a KeyId
                            issuers.push(key_id.to_string());
                        }
                        pgp::packet::OpsVersionSpecific::V6 { fingerprint, .. } => {
                            // V6 OPS contains a 32-byte fingerprint. Format it safely as an uppercase HEX string.
                            let fp_str: String =
                                fingerprint.iter().map(|b| format!("{:02X}", b)).collect();
                            issuers.push(fp_str);
                        }
                        _ => {}
                    }
                }
                // Sniff Signer from Signature packet (Used in detached signatures or end of inline)
                Packet::Signature(sig) => {
                    for issuer in sig.issuer_key_id() {
                        issuers.push(issuer.to_string());
                    }
                }
                _ => {}
            }
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
    secret_key_block: &str,
    fetch_pwd_cb: Option<GfrPasswordFetchCb>,
    fetch_pubkey_cb: Option<GfrPublicKeyFetchCb>,
    free_cb: Option<GfrFreeCb>,
    user_data: *mut c_void,
) -> Result<DecryptAndVerifyResultInternal, GfrStatus> {
    // 1. Parse secret key
    let (skey, _) =
        SignedSecretKey::from_string(secret_key_block).map_err(|_| GfrStatus::ErrorInvalidInput)?;

    // 2. Sniff recipients from outer layer
    let mut recipients = sniff_recipients(encrypted_data);

    let parsed_message = if let Ok((msg, _)) = Message::from_armor(Cursor::new(encrypted_data)) {
        msg
    } else if let Ok(msg) = Message::from_bytes(encrypted_data) {
        msg
    } else {
        return Err(GfrStatus::ErrorInvalidInput);
    };

    // 3. Determine if we need to fetch a password
    let mut needs_password = false;
    let primary_id = skey.primary_key.legacy_key_id().to_string();
    let mut target_fpr_for_pwd = skey.primary_key.fingerprint().to_string();

    let is_anonymous = |id: &str| id == "0000000000000000";

    if recipients
        .iter()
        .any(|r| r.key_id == primary_id || is_anonymous(&r.key_id))
        && matches!(skey.primary_key.secret_params(), SecretParams::Encrypted(_))
    {
        needs_password = true;
    }

    if !needs_password {
        for subkey in &skey.secret_subkeys {
            let subkey_id = subkey.key.legacy_key_id().to_string();
            if recipients
                .iter()
                .any(|r| r.key_id == subkey_id || is_anonymous(&r.key_id))
                && matches!(subkey.key.secret_params(), SecretParams::Encrypted(_))
            {
                needs_password = true;
                target_fpr_for_pwd = subkey.key.fingerprint().to_string();
                break;
            }
        }
    }

    let mut password = Vec::<u8>::new();

    if needs_password {
        password = fetch_password_internal(
            channel,
            &target_fpr_for_pwd,
            "Decryption & Verification",
            fetch_pwd_cb,
            free_cb,
        )?;
    } else {
        debug!("Target secret key is unlocked. Bypassing password callback.");
    }

    // 4. Decrypt outer layer
    let pwd_fn = Password::from(password.as_slice());
    let mut decrypted = parsed_message
        .decrypt(&pwd_fn, &skey)
        .map_err(|_| GfrStatus::ErrorInternal)?;

    // Update recipients
    let subkey_ids: Vec<String> = skey
        .secret_subkeys
        .iter()
        .map(|s| s.key.legacy_key_id().to_string())
        .collect();

    for rec in &mut recipients {
        if rec.key_id == primary_id || subkey_ids.contains(&rec.key_id) {
            rec.status = GfrRecipientStatus::Success;
        }
    }

    // 5. Decompress if necessary
    if decrypted.is_compressed() {
        decrypted = decrypted
            .decompress()
            .map_err(|_| GfrStatus::ErrorInternal)?;
    }

    // 6. Extract filename from headers BEFORE consuming payload
    let mut filename = String::new();
    if let Some(header) = decrypted.literal_data_header() {
        filename = String::from_utf8_lossy(header.file_name()).to_string();
    }

    let is_signed = decrypted.is_signed();

    // 7. Consume the payload to unlock trailing signatures
    let payload = decrypted
        .as_data_vec()
        .map_err(|_| GfrStatus::ErrorInternal)?;

    // 8. Extract parsed signatures
    let mut signatures = Vec::new();
    if let pgp::composed::Message::Signed { ref reader, .. } = decrypted {
        for i in 0..reader.num_signatures() {
            if let Some(sig) = reader.signature(i) {
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

                    if !signatures
                        .iter()
                        .any(|r: &SignatureResultInternal| r.fpr == fpr)
                    {
                        signatures.push(SignatureResultInternal {
                            fpr,
                            status: GfrSignatureStatus::NoKey, // Default to NoKey
                            created_at: sig.created().map(|d| d.as_secs() as u32).unwrap_or(0),
                            pub_algo: pub_algo_id,
                            hash_algo: hash_algo_id,
                            sig_type: GfrSignMode::Inline,
                        });
                    }
                }
            }
        }
    }

    // 9. Dynamically Fetch Public Keys via C++ Callback
    let mut certs = Vec::new();
    if let Some(cb) = fetch_pubkey_cb {
        for sig in &signatures {
            let c_fpr = std::ffi::CString::new(sig.fpr.clone()).unwrap_or_default();
            let c_key_block = cb(c_fpr.as_ptr(), user_data);

            if !c_key_block.is_null() {
                if let Ok(key_str) = unsafe { std::ffi::CStr::from_ptr(c_key_block) }.to_str() {
                    if let Ok((cert, _)) = SignedPublicKey::from_string(key_str) {
                        certs.push(cert);
                    }
                }
                if let Some(f_cb) = free_cb {
                    f_cb(c_key_block as *mut c_void, user_data);
                }
            }
        }
    }

    // 10. Verify Signatures with fetched certs
    let mut is_verified = false;
    if is_signed {
        // Helper closure to update signature statuses uniformly
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

        for cert in &certs {
            let is_cert_valid = decrypted.verify(cert).is_ok()
                || cert
                    .public_subkeys
                    .iter()
                    .any(|sk| decrypted.verify(sk).is_ok());

            update_signatures(cert, is_cert_valid, &mut signatures, &mut is_verified);
        }
    }

    Ok(DecryptAndVerifyResultInternal {
        data: payload,
        filename,
        recipients,
        is_verified,
        signatures,
    })
}
