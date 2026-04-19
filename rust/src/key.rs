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

use crate::cache::{PASSWORD_CACHE, PasswordCachePolicy};
use crate::err::IntoGfrResult;
use crate::keygen::GeneratedKeys;
use crate::types::{GfrFreeCb, GfrKeyAlgo, GfrPasswordFetchCb, GfrRevocationCode, GfrStatus};
use crate::utils::{
    build_revocation_reason_subpacket, determine_algo, extract_key_length,
    fetch_password_with_cache,
};
use pgp::armor::{self, BlockType};
use pgp::crypto::hash::HashAlgorithm;
use pgp::packet::{SignatureConfig, SignatureType, Subpacket, SubpacketData};
use pgp::types::{Password, SecretParams, SignedUser};
use pgp::{
    composed::{ArmorOptions, Deserializable, SignedPublicKey, SignedSecretKey},
    packet::Signature,
    ser::Serialize,
    types::KeyDetails,
};
use std::collections::HashSet;
use std::io;

pub struct ExtractUserId {
    pub user_id: String,
    pub is_primary: bool,
    pub is_revoked: bool,
}

pub struct ExtractedSubkey {
    pub fpr: String,
    pub key_id: String,
    pub algo: GfrKeyAlgo,
    pub key_length: u32,
    pub created_at: u32,
    pub has_secret: bool,
    pub is_revoked: bool,
    pub can_sign: bool,
    pub can_encrypt: bool,
    pub can_certify: bool,
    pub can_auth: bool,
}

pub struct ExtractedMetadata {
    pub fpr: String,
    pub key_id: String,
    pub algo: GfrKeyAlgo,
    pub key_length: u32,
    pub created_at: u32,
    pub has_secret: bool,
    pub can_sign: bool,
    pub can_encrypt: bool,
    pub can_auth: bool,
    pub can_certify: bool,
    pub user_ids: Vec<ExtractUserId>,
    pub subkeys: Vec<ExtractedSubkey>,
    pub public_key_block: String,
    pub secret_key_block: Option<String>,
}

fn is_self_signature_from_primary(sig: &Signature, primary_fpr_bytes: &[u8]) -> bool {
    sig.issuer_fingerprint()
        .iter()
        .any(|f| f.as_bytes() == primary_fpr_bytes)
}

fn is_self_uid_revocation(sig: &Signature, primary_fpr_bytes: &[u8]) -> bool {
    is_self_signature_from_primary(sig, primary_fpr_bytes)
        && matches!(sig.typ(), Some(SignatureType::CertRevocation))
}

fn is_user_id_revoked(user: &SignedUser, primary_fpr_bytes: &[u8]) -> bool {
    user.signatures
        .iter()
        .any(|sig| is_self_uid_revocation(sig, primary_fpr_bytes))
}

// Helper to extract metadata flags from a list of signatures. This is used for both primary and subkeys.
fn extract_capabilities(signatures: &[Signature]) -> (bool, bool, bool, bool) {
    let mut can_sign = false;
    let mut can_encrypt = false;
    let mut can_auth = false;
    let mut can_certify = false;

    for sig in signatures {
        // Get the KeyFlags struct directly
        let flags = sig.key_flags();

        // Call the boolean methods provided by the KeyFlags struct
        if flags.sign() {
            can_sign = true;
        }

        // PGP defines two types of encryption flags, checking either is usually sufficient
        if flags.encrypt_comms() || flags.encrypt_storage() {
            can_encrypt = true;
        }

        if flags.authentication() {
            can_auth = true;
        }

        if flags.certify() {
            can_certify = true;
        }
    }

    (can_sign, can_encrypt, can_auth, can_certify)
}

fn is_self_subkey_revocation(sig: &Signature, primary_fpr_bytes: &[u8]) -> bool {
    is_self_signature_from_primary(sig, primary_fpr_bytes)
        && matches!(sig.typ(), Some(SignatureType::SubkeyRevocation))
}

fn is_subkey_revoked(signatures: &[Signature], primary_fpr_bytes: &[u8]) -> bool {
    signatures
        .iter()
        .any(|sig| is_self_subkey_revocation(sig, primary_fpr_bytes))
}

// Helper: Extract metadata from a secret key
fn build_secret_metadata(sk: &SignedSecretKey) -> ExtractedMetadata {
    let pk = SignedPublicKey::from(sk.clone());
    let mut subs = Vec::new();

    let primary_fpr_bytes = sk.primary_key.fingerprint().as_bytes().to_vec();

    for sub in &sk.secret_subkeys {
        let (can_sign, can_encrypt, can_auth, can_certify) = extract_capabilities(&sub.signatures);
        let key_length = extract_key_length(sub.key.public_params());
        let is_revoked = is_subkey_revoked(&sub.signatures, &primary_fpr_bytes);
        subs.push(ExtractedSubkey {
            fpr: sub.key.fingerprint().to_string(),
            key_id: sub.key.legacy_key_id().to_string(),
            algo: determine_algo(sub.key.public_params()),
            created_at: sub.key.created_at().as_secs(),
            has_secret: true,
            can_sign,
            can_encrypt,
            can_auth,
            can_certify,
            key_length: key_length.unwrap_or(0),
            is_revoked: is_revoked,
        });
    }

    let users = &pk.details.users;

    // find primary user ID index (the one with the IsPrimary flag set). If
    // multiple have it, take the first. If none have it, default to index 0.
    let mut primary_idx = 0;
    for (i, user) in users.iter().enumerate() {
        let has_primary_flag = user.is_primary();

        if has_primary_flag {
            primary_idx = i;
            break;
        }
    }

    // extract all user IDs into a vector of strings
    let primary_fpr_bytes = pk.fingerprint().as_bytes().to_vec();
    let mut user_ids: Vec<ExtractUserId> = users
        .iter()
        .map(|u| {
            let is_revoked = is_user_id_revoked(u, &primary_fpr_bytes);

            ExtractUserId {
                user_id: String::from_utf8_lossy(u.id.id()).into_owned(),
                is_primary: !is_revoked && u.is_primary(),
                is_revoked,
            }
        })
        .collect();

    // if the detected primary UID is not already the first one, reorder the
    // vector so that it is. This ensures the primary UID is always at index 0
    // in our output, which simplifies capability extraction later.
    if primary_idx != 0 && primary_idx < user_ids.len() {
        let primary_uid = user_ids.remove(primary_idx);
        user_ids.insert(0, primary_uid);
    }

    // use the signatures of the actual primary user ID (after reordering) to determine capabilities
    let primary_user_sigs: &[pgp::packet::Signature] = users
        .get(primary_idx)
        .map(|u| u.signatures.as_slice())
        .unwrap_or(&[]);
    let (can_sign, can_encrypt, can_auth, can_certify) = extract_capabilities(primary_user_sigs);
    let key_length = extract_key_length(pk.primary_key.public_params());
    ExtractedMetadata {
        fpr: pk.primary_key.fingerprint().to_string(),
        key_id: pk.primary_key.legacy_key_id().to_string(),
        algo: determine_algo(pk.primary_key.public_params()),
        created_at: pk.primary_key.created_at().as_secs(),
        has_secret: true,
        can_sign,
        can_encrypt,
        can_auth,
        can_certify,
        user_ids, // Assign the collected vector here
        subkeys: subs,
        public_key_block: pk
            .to_armored_string(ArmorOptions::default())
            .unwrap_or_default(),
        secret_key_block: sk.to_armored_string(ArmorOptions::default()).ok(),
        key_length: key_length.unwrap_or(0),
    }
}

// Helper: Extract metadata from a public key
fn build_public_metadata(pk: &SignedPublicKey) -> ExtractedMetadata {
    let mut subs = Vec::new();

    let primary_fpr_bytes = pk.primary_key.fingerprint().as_bytes().to_vec();

    for sub in &pk.public_subkeys {
        let (can_sign, can_encrypt, can_auth, can_certify) = extract_capabilities(&sub.signatures);
        let key_length = extract_key_length(sub.key.public_params()).unwrap_or(0);
        let is_revoked = is_subkey_revoked(&sub.signatures, &primary_fpr_bytes);
        subs.push(ExtractedSubkey {
            fpr: sub.key.fingerprint().to_string(),
            key_id: sub.key.legacy_key_id().to_string(),
            algo: determine_algo(sub.key.public_params()),
            created_at: sub.key.created_at().as_secs(),
            has_secret: false,
            can_sign,
            can_encrypt,
            can_auth,
            can_certify,
            key_length: key_length,
            is_revoked,
        });
    }

    let users = &pk.details.users;

    // find primary user ID index (the one with the IsPrimary flag set). If
    // multiple have it, take the first. If none have it, default to index 0.
    let mut primary_idx = 0;
    for (i, user) in users.iter().enumerate() {
        let has_primary_flag = user.is_primary();

        if has_primary_flag {
            primary_idx = i;
            break;
        }
    }

    // extract all user IDs into a vector of strings
    let primary_fpr_bytes = pk.fingerprint().as_bytes().to_vec();
    let mut user_ids: Vec<ExtractUserId> = users
        .iter()
        .map(|u| {
            let is_revoked = is_user_id_revoked(u, &primary_fpr_bytes);

            ExtractUserId {
                user_id: String::from_utf8_lossy(u.id.id()).into_owned(),
                is_primary: !is_revoked && u.is_primary(),
                is_revoked,
            }
        })
        .collect();

    // if the detected primary UID is not already the first one, reorder the
    // vector so that it is. This ensures the primary UID is always at index 0
    // in our output, which simplifies capability extraction later.
    if primary_idx != 0 && primary_idx < user_ids.len() {
        let primary_uid = user_ids.remove(primary_idx);
        user_ids.insert(0, primary_uid);
    }

    // use the signatures of the actual primary user ID (after reordering) to determine capabilities
    let primary_user_sigs: &[pgp::packet::Signature] = users
        .get(primary_idx)
        .map(|u| u.signatures.as_slice())
        .unwrap_or(&[]);
    let (can_sign, can_encrypt, can_auth, can_certify) = extract_capabilities(primary_user_sigs);
    let key_length = extract_key_length(pk.primary_key.public_params());
    ExtractedMetadata {
        fpr: pk.primary_key.fingerprint().to_string(),
        key_id: pk.primary_key.legacy_key_id().to_string(),
        user_ids,
        algo: determine_algo(pk.primary_key.public_params()),
        key_length: key_length.unwrap_or(0),
        created_at: pk.primary_key.created_at().as_secs(),
        has_secret: false,
        subkeys: subs,
        can_sign,
        can_encrypt,
        can_auth,
        can_certify,
        public_key_block: pk
            .to_armored_string(ArmorOptions::default())
            .unwrap_or_default(),
        secret_key_block: None,
    }
}

// Main extraction function supporting multiple keys
pub fn extract_metadata_many_internal(
    key_blocks: &str,
) -> Result<Vec<ExtractedMetadata>, GfrStatus> {
    let mut results = Vec::new();
    let mut processed_fprs = HashSet::new();

    // 1. Parse all available secret keys first
    if let Ok((sk_iter, _)) = SignedSecretKey::from_string_many(key_blocks) {
        for sk_res in sk_iter {
            if let Ok(sk) = sk_res {
                let metadata = build_secret_metadata(&sk);
                // Record the fingerprint to prevent duplicate processing later
                processed_fprs.insert(metadata.fpr.clone());
                results.push(metadata);
            }
        }
    }

    // 2. Parse all public keys and filter out the ones already processed as secret keys
    if let Ok((pk_iter, _)) = SignedPublicKey::from_string_many(key_blocks) {
        for pk_res in pk_iter {
            if let Ok(pk) = pk_res {
                let fpr = pk.fingerprint().to_string();

                // Skip if this key was already extracted with its secret material
                if !processed_fprs.contains(&fpr) {
                    let metadata = build_public_metadata(&pk);
                    processed_fprs.insert(fpr);
                    results.push(metadata);
                }
            }
        }
    }

    // 3. If no keys were successfully parsed, return an error
    if results.is_empty() {
        return Err(GfrStatus::ErrorInvalidInput);
    }

    Ok(results)
}

// Extract a public key armored string from a secret key armored string
pub fn extract_public_key_internal(secret_block: &str) -> Result<String, GfrStatus> {
    // 1. Parse the armored secret key block
    let (secret_key, _) =
        SignedSecretKey::from_string(secret_block).map_err(|_| GfrStatus::ErrorInvalidInput)?;

    // 2. Convert to public key (this strips the secret mathematical materials)
    let public_key = SignedPublicKey::from(secret_key);

    // 3. Export back to ASCII Armor format
    let armored_p_key = public_key
        .to_armored_string(ArmorOptions::default())
        .map_err(|_| GfrStatus::ErrorArmorFailed)?;

    Ok(armored_p_key)
}

struct MergedRawKeys {
    packet_bytes: Vec<u8>,
}

// 2. Implement the Serialize trait so rpgp's armor::write can consume it.
impl Serialize for MergedRawKeys {
    fn to_writer<W: io::Write>(&self, writer: &mut W) -> pgp::errors::Result<()> {
        writer.write_all(&self.packet_bytes)?;
        Ok(())
    }

    fn write_len(&self) -> usize {
        self.packet_bytes.len()
    }
}

pub fn export_merged_public_keys(key_blocks: &[&str]) -> Result<String, GfrStatus> {
    let mut combined_bytes = Vec::new();

    for block in key_blocks {
        // 3. Try parsing as secret key first, fallback to public key.
        let pub_key = if let Ok((sk, _)) = SignedSecretKey::from_string(block) {
            SignedPublicKey::from(sk)
        } else if let Ok((pk, _)) = SignedPublicKey::from_string(block) {
            pk
        } else {
            return Err(GfrStatus::ErrorInvalidInput);
        };

        // 4. Serialize the underlying packets of this public key into our buffer.
        pub_key
            .to_writer(&mut combined_bytes)
            .map_err(|_| GfrStatus::ErrorInvalidInput)?;
    }

    if combined_bytes.is_empty() {
        return Err(GfrStatus::ErrorInvalidInput);
    }

    // 5. Wrap the merged bytes into our custom struct.
    let merged_source = MergedRawKeys {
        packet_bytes: combined_bytes,
    };

    let mut armored_output = Vec::new();

    // 6. Call the armor::write function you found in the source code.
    // The arguments are: source, block_type, writer, headers, include_checksum.
    armor::write(
        &merged_source,
        BlockType::PublicKey,
        &mut armored_output,
        None, // Or Some(&headers) if you need custom headers like "Version"
        true, // Standard PGP includes the CRC checksum
    )
    .map_err(|_| GfrStatus::ErrorArmorFailed)?;

    // 7. Convert the armored bytes back to a String.
    String::from_utf8(armored_output).map_err(|_| GfrStatus::ErrorArmorFailed)
}

pub fn export_merged_secret_keys(key_blocks: &[&str]) -> Result<String, GfrStatus> {
    let mut combined_bytes = Vec::new();

    for block in key_blocks {
        // 1. Only accept valid secret keys. Fallback to public key is NOT possible here.
        let sec_key = if let Ok((sk, _)) = SignedSecretKey::from_string(block) {
            sk
        } else {
            // Fails if the block is a public key or invalid data
            return Err(GfrStatus::ErrorInvalidInput);
        };

        // 2. Serialize the underlying packets of the secret key into our buffer.
        sec_key
            .to_writer(&mut combined_bytes)
            .map_err(|_| GfrStatus::ErrorInvalidInput)?;
    }

    if combined_bytes.is_empty() {
        return Err(GfrStatus::ErrorInvalidInput);
    }

    // 3. Wrap the merged bytes into our custom struct (reusing the one from before).
    let merged_source = MergedRawKeys {
        packet_bytes: combined_bytes,
    };

    let mut armored_output = Vec::new();

    // 4. Call armor::write, but use BlockType::PrivateKey this time.
    armor::write(
        &merged_source,
        BlockType::PrivateKey, // Crucial change: PRIVATE KEY BLOCK
        &mut armored_output,
        None,
        true,
    )
    .map_err(|_| GfrStatus::ErrorArmorFailed)?;

    // 5. Convert the armored bytes back to a String.
    String::from_utf8(armored_output).map_err(|_| GfrStatus::ErrorArmorFailed)
}

fn fetch_old_and_new_passwords(
    channel: i32,
    target_fpr: &str,
    is_encrypted: bool,
    unlock_purpose: &str,
    new_purpose: &str,
    fetch_pwd_cb: Option<GfrPasswordFetchCb>,
    free_cb: Option<GfrFreeCb>,
) -> Result<(Option<Password>, Password), GfrStatus> {
    let old_pw = if is_encrypted {
        let old_pwd_bytes = fetch_password_with_cache(
            Some(&PASSWORD_CACHE),
            PasswordCachePolicy::Default,
            channel,
            target_fpr,
            unlock_purpose,
            fetch_pwd_cb,
            free_cb,
        )?;

        if old_pwd_bytes.is_empty() {
            return Err(GfrStatus::ErrorFetchPasswordFailed);
        }

        Some(Password::from(old_pwd_bytes.as_slice()))
    } else {
        None
    };

    let new_pwd_bytes = fetch_password_with_cache(
        Some(&PASSWORD_CACHE),
        PasswordCachePolicy::Bypass,
        channel,
        target_fpr,
        new_purpose,
        fetch_pwd_cb,
        free_cb,
    )?;

    if new_pwd_bytes.is_empty() {
        return Err(GfrStatus::ErrorFetchPasswordFailed);
    }

    let new_pw = Password::from(new_pwd_bytes.as_slice());

    Ok((old_pw, new_pw))
}

pub fn modify_key_password_internal(
    channel: i32,
    secret_key_block: &str,
    target_fpr: &str,
    fetch_pwd_cb: Option<GfrPasswordFetchCb>,
    free_cb: Option<GfrFreeCb>,
) -> Result<GeneratedKeys, GfrStatus> {
    let (mut secret_key, _) = SignedSecretKey::from_string(secret_key_block).map_err(|e| {
        log::error!("Failed to parse secret key block: {}", e);
        GfrStatus::ErrorInvalidData
    })?;

    let whole_fpr = secret_key.fingerprint().to_string();
    let primary_fpr = secret_key
        .primary_key
        .fingerprint()
        .to_string()
        .to_uppercase();

    if primary_fpr == target_fpr {
        let mut rng = rand::thread_rng();

        let (old_pw, new_pw) = fetch_old_and_new_passwords(
            channel,
            target_fpr,
            secret_key.primary_key.secret_params().is_encrypted(),
            "Unlock Primary Key to change password",
            "Set new password for Primary Key",
            fetch_pwd_cb,
            free_cb,
        )?;

        if let Some(old_pw) = old_pw {
            secret_key.primary_key.remove_password(&old_pw).into_gfr()?;
        }

        // Invalidate cache entries for this key
        PASSWORD_CACHE.remove_by_fpr(&primary_fpr);

        secret_key
            .primary_key
            .set_password(&mut rng, &new_pw)
            .into_gfr()?;

        let armored_s_key = secret_key
            .to_armored_string(ArmorOptions::default())
            .into_gfr()?;

        let public_key = SignedPublicKey::from(secret_key);
        let armored_p_key = public_key
            .to_armored_string(ArmorOptions::default())
            .into_gfr()?;

        return Ok(GeneratedKeys {
            secret: armored_s_key,
            public: armored_p_key,
            fingerprint: whole_fpr,
        });
    }

    for subkey in secret_key.secret_subkeys.iter_mut() {
        let sub_fpr = subkey.key.fingerprint().to_string().to_uppercase();

        if sub_fpr != target_fpr {
            continue;
        }

        let mut rng = rand::thread_rng();

        let (old_pw, new_pw) = fetch_old_and_new_passwords(
            channel,
            target_fpr,
            subkey.key.secret_params().is_encrypted(),
            "Unlock Subkey to change password",
            "Set new password for Subkey",
            fetch_pwd_cb,
            free_cb,
        )?;

        if let Some(old_pw) = old_pw {
            subkey.key.remove_password(&old_pw).into_gfr()?;
        }

        subkey.key.set_password(&mut rng, &new_pw).into_gfr()?;

        let armored_s_key = secret_key
            .to_armored_string(ArmorOptions::default())
            .into_gfr()?;

        let public_key = SignedPublicKey::from(secret_key);
        let armored_p_key = public_key
            .to_armored_string(ArmorOptions::default())
            .into_gfr()?;

        return Ok(GeneratedKeys {
            secret: armored_s_key,
            public: armored_p_key,
            fingerprint: whole_fpr,
        });
    }

    Err(GfrStatus::ErrorInvalidInput)
}

pub fn delete_subkey_internal(
    secret_key_block: &str,
    target_subkey_fpr: &str,
) -> Result<GeneratedKeys, GfrStatus> {
    let (mut secret_key, _) = SignedSecretKey::from_string(secret_key_block).map_err(|e| {
        log::error!("Failed to parse secret key block: {}", e);
        GfrStatus::ErrorInvalidData
    })?;

    let fingerprint_str = secret_key.fingerprint().to_string().to_uppercase();
    let initial_len = secret_key.secret_subkeys.len();

    if secret_key
        .primary_key
        .fingerprint()
        .to_string()
        .to_uppercase()
        == target_subkey_fpr
    {
        return Err(GfrStatus::ErrorInvalidInput);
    }

    secret_key
        .secret_subkeys
        .retain(|sub| sub.key.fingerprint().to_string().to_uppercase() != target_subkey_fpr);

    if secret_key.secret_subkeys.len() == initial_len {
        return Err(GfrStatus::ErrorInvalidInput);
    }

    let armored_s_key = secret_key
        .to_armored_string(ArmorOptions::default())
        .into_gfr()?;

    let public_key = SignedPublicKey::from(secret_key);
    let armored_p_key = public_key
        .to_armored_string(ArmorOptions::default())
        .into_gfr()?;

    Ok(GeneratedKeys {
        secret: armored_s_key,
        public: armored_p_key,
        fingerprint: fingerprint_str,
    })
}

pub fn revoke_subkey_internal(
    channel: i32,
    secret_key_block: &str,
    target_subkey_fpr: &str,
    reason_code: GfrRevocationCode,
    reason_text: Option<&str>,
    fetch_cb: Option<GfrPasswordFetchCb>,
    free_cb: Option<GfrFreeCb>,
) -> Result<GeneratedKeys, GfrStatus> {
    let (mut secret_key, _) = SignedSecretKey::from_string(secret_key_block).into_gfr()?;

    let target_idx = secret_key
        .secret_subkeys
        .iter()
        .position(|sub| {
            sub.key.fingerprint().to_string().to_uppercase() == target_subkey_fpr.to_uppercase()
        })
        .ok_or(GfrStatus::ErrorInvalidInput)?;

    if secret_key
        .primary_key
        .fingerprint()
        .to_string()
        .to_uppercase()
        == target_subkey_fpr.to_uppercase()
    {
        return Err(GfrStatus::ErrorInvalidInput);
    }

    let fpr = secret_key.primary_key.fingerprint().to_string();
    let is_enc = matches!(
        secret_key.primary_key.secret_params(),
        SecretParams::Encrypted(_)
    );

    let pwd_bytes = if is_enc {
        fetch_password_with_cache(
            Some(&PASSWORD_CACHE),
            PasswordCachePolicy::Default,
            channel,
            &fpr,
            "Revoke Subkey",
            fetch_cb,
            free_cb,
        )?
    } else {
        Vec::new()
    };
    let pwd = Password::from(pwd_bytes.as_slice());

    let pk = secret_key.primary_key.public_key();
    let primary_fpr = secret_key.primary_key.fingerprint();
    let primary_fpr_bytes = primary_fpr.as_bytes().to_vec();

    let subkey = secret_key
        .secret_subkeys
        .get_mut(target_idx)
        .ok_or(GfrStatus::ErrorInternal)?;

    let already_revoked = subkey.signatures.iter().any(|sig| {
        sig.issuer_fingerprint()
            .iter()
            .any(|fp| fp.as_bytes() == primary_fpr_bytes)
            && matches!(sig.typ(), Some(SignatureType::SubkeyRevocation))
    });

    if already_revoked {
        return Err(GfrStatus::ErrorInvalidInput);
    }

    let mut cfg = SignatureConfig::v4(
        SignatureType::SubkeyRevocation,
        secret_key.primary_key.algorithm(),
        HashAlgorithm::Sha512,
    );

    cfg.hashed_subpackets.push(
        Subpacket::regular(SubpacketData::IssuerFingerprint(primary_fpr))
            .map_err(|_| GfrStatus::ErrorInternal)?,
    );

    cfg.hashed_subpackets.push(
        Subpacket::regular(SubpacketData::SignatureCreationTime(
            pgp::types::Timestamp::now(),
        ))
        .map_err(|_| GfrStatus::ErrorInternal)?,
    );

    cfg.hashed_subpackets
        .push(build_revocation_reason_subpacket(reason_code, reason_text)?);

    let revoke_sig = cfg
        .sign_subkey_binding(&secret_key.primary_key, &pk, &pwd, subkey.key.public_key())
        .into_gfr()?;

    subkey.signatures.push(revoke_sig);

    let armored_s_key = secret_key
        .to_armored_string(ArmorOptions::default())
        .map_err(|_| GfrStatus::ErrorArmorFailed)?;

    let public_key = SignedPublicKey::from(secret_key);
    let armored_p_key = public_key
        .to_armored_string(ArmorOptions::default())
        .map_err(|_| GfrStatus::ErrorArmorFailed)?;

    Ok(GeneratedKeys {
        secret: armored_s_key,
        public: armored_p_key,
        fingerprint: fpr,
    })
}
