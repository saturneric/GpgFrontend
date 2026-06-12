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

//! Key block manipulation: metadata extraction, merging, password change,
//! subkey deletion/revocation, and revocation certificate generation/import.
//!
//! All functions operate on armored OpenPGP key blocks (strings) and return
//! updated armored blocks. No key material is stored; the caller is responsible
//! for persisting the returned blocks.

use crate::cache::{PASSWORD_CACHE, PasswordCachePolicy};
use crate::err::IntoGfrResult;
use crate::keygen::GeneratedKeys;
use crate::types::{
    GfrFreeCb, GfrKeyAlgo, GfrOpenPGPKeyVersion, GfrPasswordFetchCb, GfrRevocationCode, GfrStatus,
};
use crate::utils::{
    PassphraseStateInternal, build_revocation_reason_subpacket, determine_algo, extract_key_length,
    fetch_password_with_cache, password_from_zeroizing_bytes,
};
use pgp::armor::{self, BlockType};
use pgp::composed::SignedPublicSubKey;
use pgp::crypto::hash::HashAlgorithm;
use pgp::packet::{Packet, PacketHeader, SignatureConfig, SignatureType, Subpacket, SubpacketData};
use pgp::types::{KeyDetails, KeyVersion, Password, SecretParams, SignedUser};
use pgp::{
    composed::{ArmorOptions, Deserializable, SignedPublicKey, SignedSecretKey},
    packet::Signature,
    ser::Serialize,
};
use std::collections::{BTreeMap, HashMap, HashSet};
use std::io::{self, BufReader, Cursor};
use zeroize::Zeroizing;

/// A single user ID extracted from a key block.
pub struct ExtractUserId {
    pub user_id: String,
    pub is_primary: bool,
    pub is_revoked: bool,
}

/// Metadata for a single subkey within a key block.
pub struct ExtractedSubkey {
    pub ver: GfrOpenPGPKeyVersion,
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

/// Full metadata for a primary key, including its subkeys and user IDs.
///
/// `secret_key_block` is `None` for public-only keys. Both armored blocks
/// are always re-exported from the parsed key objects (not taken verbatim from
/// the input) to ensure they are canonical.
pub struct ExtractedMetadata {
    pub ver: GfrOpenPGPKeyVersion,
    pub fpr: String,
    pub key_id: String,
    pub algo: GfrKeyAlgo,
    pub key_length: u32,
    pub created_at: u32,
    pub has_secret: bool,
    pub is_revoked: bool,
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

fn is_self_key_revocation(sig: &Signature, primary_fpr_bytes: &[u8]) -> bool {
    is_self_signature_from_primary(sig, primary_fpr_bytes)
        && matches!(sig.typ(), Some(SignatureType::KeyRevocation))
}

fn is_primary_key_revoked(signatures: &[Signature], primary_fpr_bytes: &[u8]) -> bool {
    signatures
        .iter()
        .any(|sig| is_self_key_revocation(sig, primary_fpr_bytes))
}

fn extract_capabilities<'a, I>(signatures: I) -> (bool, bool, bool, bool)
where
    I: IntoIterator<Item = &'a Signature>,
{
    let mut can_sign = false;
    let mut can_encrypt = false;
    let mut can_auth = false;
    let mut can_certify = false;

    for sig in signatures {
        let flags = sig.key_flags();

        if flags.sign() {
            can_sign = true;
        }

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

impl From<KeyVersion> for GfrOpenPGPKeyVersion {
    fn from(version: KeyVersion) -> Self {
        match version {
            KeyVersion::V4 => GfrOpenPGPKeyVersion::V4,
            KeyVersion::V6 => GfrOpenPGPKeyVersion::V6,
            _ => GfrOpenPGPKeyVersion::Unknown,
        }
    }
}

// Helper: Extract metadata from a secret key
fn build_secret_metadata(sk: &SignedSecretKey) -> ExtractedMetadata {
    let pk = SignedPublicKey::from(sk.clone());
    let mut subs = Vec::new();

    let primary_fpr_bytes = sk.primary_key.fingerprint().as_bytes().to_vec();

    for sub in &sk.secret_subkeys {
        let (can_sign, can_encrypt, can_auth, can_certify) =
            extract_capabilities(sub.signatures.iter());
        let key_length = extract_key_length(sub.key.public_params());
        let is_revoked = is_subkey_revoked(&sub.signatures, &primary_fpr_bytes);
        subs.push(ExtractedSubkey {
            ver: sub.version().into(),
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

    let primary_user_sigs = users
        .get(primary_idx)
        .map(|u| u.signatures.as_slice())
        .unwrap_or(&[]);
    let (can_sign, can_encrypt, can_auth, can_certify) = extract_capabilities(
        pk.details
            .direct_signatures
            .iter()
            .chain(primary_user_sigs.iter()),
    );
    let key_length = extract_key_length(pk.primary_key.public_params());
    let is_revoked = is_primary_key_revoked(&pk.details.revocation_signatures, &primary_fpr_bytes)
        || is_primary_key_revoked(&pk.details.direct_signatures, &primary_fpr_bytes);

    ExtractedMetadata {
        ver: pk.version().into(),
        fpr: pk.primary_key.fingerprint().to_string(),
        key_id: pk.primary_key.legacy_key_id().to_string(),
        algo: determine_algo(pk.primary_key.public_params()),
        created_at: pk.primary_key.created_at().as_secs(),
        has_secret: true,
        is_revoked,
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
        let (can_sign, can_encrypt, can_auth, can_certify) =
            extract_capabilities(sub.signatures.iter());
        let key_length = extract_key_length(sub.key.public_params()).unwrap_or(0);
        let is_revoked = is_subkey_revoked(&sub.signatures, &primary_fpr_bytes);
        subs.push(ExtractedSubkey {
            ver: sub.version().into(),
            fpr: sub.key.fingerprint().to_string(),
            key_id: sub.key.legacy_key_id().to_string(),
            algo: determine_algo(sub.key.public_params()),
            created_at: sub.key.created_at().as_secs(),
            has_secret: false,
            can_sign,
            can_encrypt,
            can_auth,
            can_certify,
            key_length,
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
    let primary_user_sigs = users
        .get(primary_idx)
        .map(|u| u.signatures.as_slice())
        .unwrap_or(&[]);

    let (can_sign, can_encrypt, can_auth, can_certify) = extract_capabilities(
        pk.details
            .direct_signatures
            .iter()
            .chain(primary_user_sigs.iter()),
    );
    let key_length = extract_key_length(pk.primary_key.public_params());
    let is_revoked = is_primary_key_revoked(&pk.details.revocation_signatures, &primary_fpr_bytes)
        || is_primary_key_revoked(&pk.details.direct_signatures, &primary_fpr_bytes);
    ExtractedMetadata {
        ver: pk.version().into(),
        fpr: pk.primary_key.fingerprint().to_string(),
        key_id: pk.primary_key.legacy_key_id().to_string(),
        user_ids,
        algo: determine_algo(pk.primary_key.public_params()),
        key_length: key_length.unwrap_or(0),
        created_at: pk.primary_key.created_at().as_secs(),
        has_secret: false,
        is_revoked,
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

fn split_pgp_blocks(input: &str) -> Vec<String> {
    let mut blocks = Vec::new();
    let mut current_block = String::new();
    let mut in_block = false;

    for line in input.lines() {
        if line.contains("-----BEGIN PGP") {
            in_block = true;
            current_block.clear();
        }

        if in_block {
            current_block.push_str(line);
            current_block.push('\n');
        }

        if line.contains("-----END PGP") {
            in_block = false;
            blocks.push(current_block.clone());
        }
    }

    blocks
}

/// Parse one or more armored PGP key blocks and return metadata for each unique key.
///
/// The input may contain concatenated armor blocks. If the same fingerprint
/// appears as both a secret and a public block, the secret key entry wins
/// (the public-only block is skipped via the `entry().or_insert_with()` pattern).
pub fn extract_metadata_many_internal(
    key_blocks: &str,
) -> Result<Vec<ExtractedMetadata>, GfrStatus> {
    let mut results_map: HashMap<String, ExtractedMetadata> = HashMap::new();

    let individual_blocks = split_pgp_blocks(key_blocks);

    if individual_blocks.is_empty() {
        return Err(GfrStatus::ErrorInvalidInput);
    }

    for block in individual_blocks {
        if let Ok((sk_iter, _)) = SignedSecretKey::from_string_many(&block) {
            for sk_res in sk_iter {
                if let Ok(sk) = sk_res {
                    let fpr = sk.fingerprint().to_string();
                    let mut metadata = build_secret_metadata(&sk);

                    metadata.secret_key_block = sk.to_armored_string(ArmorOptions::default()).ok();

                    let public_key = SignedPublicKey::from(sk.clone());
                    metadata.public_key_block = public_key
                        .to_armored_string(ArmorOptions::default())
                        .unwrap_or_default();

                    results_map.insert(fpr, metadata);
                } else {
                    log::error!(
                        "Failed to parse a secret key from block: {}",
                        sk_res.err().unwrap()
                    );
                }
            }
        }

        if let Ok((pk_iter, _)) = SignedPublicKey::from_string_many(&block) {
            for pk_res in pk_iter {
                if let Ok(pk) = pk_res {
                    let fpr = pk.fingerprint().to_string();

                    results_map.entry(fpr).or_insert_with(|| {
                        let mut metadata = build_public_metadata(&pk);
                        metadata.public_key_block = pk
                            .to_armored_string(ArmorOptions::default())
                            .unwrap_or_default();
                        metadata.secret_key_block = None;
                        metadata
                    });
                } else {
                    log::error!(
                        "Failed to parse a public key from block: {}",
                        pk_res.err().unwrap()
                    );
                }
            }
        }
    }

    log::info!(
        "Completed processing all blocks. Total unique keys found: {}",
        results_map.len()
    );

    let results: Vec<ExtractedMetadata> = results_map.into_values().collect();

    if results.is_empty() {
        return Err(GfrStatus::ErrorInvalidInput);
    }

    Ok(results)
}

/// Derive the public key from an armored secret key block.
///
/// Strips all secret key material and re-exports only the public components.
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

/// Merge multiple armored key blocks into a single public key armor block.
///
/// If an input block is a secret key it is accepted and its public component
/// is extracted. The merge is done at the raw packet level (no re-signing).
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

/// Merge multiple armored secret key blocks into a single secret key armor block.
///
/// All input blocks must be secret keys; a public-key block returns `ErrorInvalidInput`.
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
            PassphraseStateInternal {
                fpr: target_fpr.to_string(),
                info: unlock_purpose.to_string(),
                retry: false,
                ask_for_new: false,
                should_confirm: false,
            },
            fetch_pwd_cb,
            free_cb,
        )?;

        if old_pwd_bytes.is_empty() {
            return Err(GfrStatus::ErrorFetchPasswordFailed);
        }

        Some(password_from_zeroizing_bytes(old_pwd_bytes))
    } else {
        None
    };

    let new_pwd_bytes = fetch_password_with_cache(
        Some(&PASSWORD_CACHE),
        PasswordCachePolicy::Bypass,
        channel,
        PassphraseStateInternal {
            fpr: target_fpr.to_string(),
            info: new_purpose.to_string(),
            retry: false,
            ask_for_new: true,
            should_confirm: true,
        },
        fetch_pwd_cb,
        free_cb,
    )?;

    if new_pwd_bytes.is_empty() {
        return Err(GfrStatus::ErrorFetchPasswordFailed);
    }

    let new_pw = password_from_zeroizing_bytes(new_pwd_bytes);

    Ok((old_pw, new_pw))
}

/// Change the passphrase protecting the key at `target_fpr` within a secret key block.
///
/// `target_fpr` may refer to the primary key or any subkey. The entire key block
/// is re-exported after the change, so the returned block contains all keys.
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

/// Remove a subkey from a secret key block.
///
/// Attempting to delete the primary key fingerprint returns `ErrorInvalidInput`.
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

/// Add a revocation self-signature to the subkey at `target_subkey_fpr`.
///
/// Requires unlocking the primary key to sign the revocation; targeting the
/// primary key fingerprint is rejected with `ErrorInvalidInput`.
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
            PassphraseStateInternal {
                fpr: fpr.clone(),
                info: "Unlock Primary Key to revoke subkey".to_string(),
                retry: false,
                ask_for_new: false,
                should_confirm: false,
            },
            fetch_cb,
            free_cb,
        )?
    } else {
        Zeroizing::new(Vec::new())
    };
    let pwd = password_from_zeroizing_bytes(pwd_bytes);

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

/// Generate a standalone revocation certificate for the primary key.
///
/// The certificate is a single v4 `KeyRevocation` signature packet wrapped in
/// a "PUBLIC KEY BLOCK" armor. It can be imported into any key block with the
/// matching fingerprint via `import_rev_cert_internal`. A v4 signature is used
/// even for v6 keys because revocation certificate interoperability matters more
/// than version consistency.
pub fn generate_key_rev_cert_internal(
    channel: i32,
    secret_key_block: &str,
    reason_code: GfrRevocationCode,
    reason_text: Option<&str>,
    fetch_cb: Option<GfrPasswordFetchCb>,
    free_cb: Option<GfrFreeCb>,
) -> Result<String, GfrStatus> {
    use pgp::packet::Packet;

    let (skey, _) = SignedSecretKey::from_string(secret_key_block).into_gfr()?;

    let fpr = skey.primary_key.fingerprint().to_string();
    let is_enc = matches!(skey.primary_key.secret_params(), SecretParams::Encrypted(_));

    let pwd_bytes = if is_enc {
        fetch_password_with_cache(
            Some(&PASSWORD_CACHE),
            PasswordCachePolicy::Default,
            channel,
            PassphraseStateInternal {
                fpr: fpr.clone(),
                info: "Generate Key Revocation Certificate".to_string(),
                retry: false,
                ask_for_new: false,
                should_confirm: false,
            },
            fetch_cb,
            free_cb,
        )?
    } else {
        Zeroizing::new(Vec::new())
    };
    let pwd = password_from_zeroizing_bytes(pwd_bytes);

    if is_enc {
        let inner = skey
            .primary_key
            .unlock(&pwd, |_, _| Ok(()))
            .map_err(|_| GfrStatus::ErrorPasswordFailed)?;
        inner.map_err(|_| GfrStatus::ErrorPasswordFailed)?;
    }

    let primary_fpr = skey.primary_key.fingerprint();
    let pk = skey.primary_key.public_key();

    let mut cfg = SignatureConfig::v4(
        SignatureType::KeyRevocation,
        skey.primary_key.algorithm(),
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

    let revoke_sig = cfg.sign_key(&skey.primary_key, &pwd, &pk).into_gfr()?;

    let mut out = Vec::new();

    Packet::Signature(revoke_sig)
        .to_writer(&mut out)
        .into_gfr()?;

    let source = MergedRawKeys { packet_bytes: out };

    let mut armored_output = Vec::new();
    let mut headers = BTreeMap::new();
    headers.insert(
        String::from("Comment"),
        vec![String::from("This is a revocation certificate.")],
    );

    armor::write(
        &source,
        BlockType::PublicKey,
        &mut armored_output,
        Some(&headers),
        true,
    )
    .into_gfr()?;

    String::from_utf8(armored_output).map_err(|_| GfrStatus::ErrorArmorFailed)
}

fn serialize_bytes<T: Serialize>(obj: &T) -> Result<Vec<u8>, GfrStatus> {
    let mut buf = Vec::new();
    obj.to_writer(&mut buf)
        .map_err(|_| GfrStatus::ErrorInternal)?;
    Ok(buf)
}

fn merge_signatures(dst: &mut Vec<Signature>, src: &[Signature]) -> Result<(), GfrStatus> {
    let mut seen: HashSet<Vec<u8>> = HashSet::new();

    for sig in dst.iter() {
        seen.insert(serialize_bytes(sig)?);
    }

    for sig in src {
        let key = serialize_bytes(sig)?;
        if seen.insert(key) {
            dst.push(sig.clone());
        }
    }

    Ok(())
}

fn merge_users(
    dst: &mut Vec<pgp::types::SignedUser>,
    src: &[pgp::types::SignedUser],
) -> Result<(), GfrStatus> {
    for src_user in src {
        let src_uid = String::from_utf8_lossy(src_user.id.id()).into_owned();

        if let Some(dst_user) = dst
            .iter_mut()
            .find(|u| String::from_utf8_lossy(u.id.id()) == src_uid)
        {
            merge_signatures(&mut dst_user.signatures, &src_user.signatures)?;
        } else {
            dst.push(src_user.clone());
        }
    }

    Ok(())
}

fn merge_public_subkeys(
    dst: &mut Vec<SignedPublicSubKey>,
    src: &[SignedPublicSubKey],
) -> Result<(), GfrStatus> {
    for src_sub in src {
        let src_fpr = src_sub.key.fingerprint().to_string();

        if let Some(dst_sub) = dst
            .iter_mut()
            .find(|s| s.key.fingerprint().to_string() == src_fpr)
        {
            merge_signatures(&mut dst_sub.signatures, &src_sub.signatures)?;
        } else {
            dst.push(src_sub.clone());
        }
    }

    Ok(())
}

fn dedup_signatures_in_place(sigs: &mut Vec<Signature>) -> Result<(), GfrStatus> {
    let mut seen = HashSet::new();
    let mut out = Vec::with_capacity(sigs.len());

    for sig in sigs.drain(..) {
        let key = serialize_bytes(&sig)?;
        if seen.insert(key) {
            out.push(sig);
        }
    }

    *sigs = out;
    Ok(())
}

fn export_secret_key(sk: SignedSecretKey) -> Result<GeneratedKeys, GfrStatus> {
    let fingerprint = sk.fingerprint().to_string();
    let secret = sk.to_armored_string(ArmorOptions::default()).into_gfr()?;
    let public = SignedPublicKey::from(sk)
        .to_armored_string(ArmorOptions::default())
        .into_gfr()?;

    Ok(GeneratedKeys {
        secret,
        public,
        fingerprint,
    })
}

fn export_public_key(pk: SignedPublicKey) -> Result<GeneratedKeys, GfrStatus> {
    let fingerprint = pk.fingerprint().to_string();
    let public = pk.to_armored_string(ArmorOptions::default()).into_gfr()?;

    Ok(GeneratedKeys {
        secret: String::new(),
        public,
        fingerprint,
    })
}

fn extract_key_revocation_signatures(block: &str) -> Result<Vec<Signature>, GfrStatus> {
    let rev_sigs: Vec<Signature> = parse_signatures_from_armor(block)?
        .into_iter()
        .filter(|sig| matches!(sig.typ(), Some(SignatureType::KeyRevocation)))
        .collect();

    if rev_sigs.is_empty() {
        log::error!("No valid KeyRevocation signatures found.");
        Err(GfrStatus::ErrorInvalidInput)
    } else {
        Ok(rev_sigs)
    }
}

fn merge_key_components(
    base_direct: &mut Vec<Signature>,
    base_revocation: &mut Vec<Signature>,
    base_users: &mut Vec<pgp::types::SignedUser>,
    incoming_direct: &[Signature],
    incoming_revocation: &[Signature],
    incoming_users: &[pgp::types::SignedUser],
) -> Result<(), GfrStatus> {
    merge_signatures(base_direct, incoming_direct)?;
    merge_signatures(base_revocation, incoming_revocation)?;
    merge_users(base_users, incoming_users)?;

    dedup_signatures_in_place(base_direct)?;
    dedup_signatures_in_place(base_revocation)?;
    Ok(())
}

/// Merge two armored key blocks for the same fingerprint.
///
/// Handles all four combinations of (secret, public) × (secret, public):
/// - secret + secret → merged secret
/// - secret + public → secret enriched with public signatures
/// - public + secret → secret enriched with public signatures
/// - public + public → merged public
///
/// Returns `ErrorInvalidInput` if the fingerprints don't match.
pub fn merge_key_block_internal(
    base_block: &str,
    incoming_block: &str,
) -> Result<GeneratedKeys, GfrStatus> {
    let base_sk = SignedSecretKey::from_string(base_block).ok();
    let base_pk = SignedPublicKey::from_string(base_block).ok();
    let inc_sk = SignedSecretKey::from_string(incoming_block).ok();
    let inc_pk = SignedPublicKey::from_string(incoming_block).ok();

    match (base_sk, inc_sk) {
        (Some((mut target_sk, _)), Some((other_sk, _))) => {
            if target_sk.fingerprint() != other_sk.fingerprint() {
                return Err(GfrStatus::ErrorInvalidInput);
            }
            merge_sk_into_sk(&mut target_sk, &other_sk)?;
            export_secret_key(target_sk)
        }

        (Some((mut target_sk, _)), None) => {
            if let Some((other_pk, _)) = inc_pk {
                if target_sk.fingerprint() != other_pk.fingerprint() {
                    return Err(GfrStatus::ErrorInvalidInput);
                }
                merge_pk_into_sk(&mut target_sk, &other_pk)?;
            }
            export_secret_key(target_sk)
        }

        (None, Some((mut target_sk, _))) => {
            if let Some((other_pk, _)) = base_pk {
                if target_sk.fingerprint() != other_pk.fingerprint() {
                    return Err(GfrStatus::ErrorInvalidInput);
                }
                merge_pk_into_sk(&mut target_sk, &other_pk)?;
            }
            export_secret_key(target_sk)
        }

        (None, None) => {
            if let (Some((mut target_pk, _)), Some((other_pk, _))) = (base_pk, inc_pk) {
                if target_pk.fingerprint() != other_pk.fingerprint() {
                    return Err(GfrStatus::ErrorInvalidInput);
                }
                merge_key_components(
                    &mut target_pk.details.direct_signatures,
                    &mut target_pk.details.revocation_signatures,
                    &mut target_pk.details.users,
                    &other_pk.details.direct_signatures,
                    &other_pk.details.revocation_signatures,
                    &other_pk.details.users,
                )?;
                merge_public_subkeys(&mut target_pk.public_subkeys, &other_pk.public_subkeys)?;
                export_public_key(target_pk)
            } else {
                Err(GfrStatus::ErrorInvalidInput)
            }
        }
    }
}

fn merge_sk_into_sk(
    target: &mut SignedSecretKey,
    source: &SignedSecretKey,
) -> Result<(), GfrStatus> {
    merge_key_components(
        &mut target.details.direct_signatures,
        &mut target.details.revocation_signatures,
        &mut target.details.users,
        &source.details.direct_signatures,
        &source.details.revocation_signatures,
        &source.details.users,
    )
}

fn merge_pk_into_sk(
    target: &mut SignedSecretKey,
    source: &SignedPublicKey,
) -> Result<(), GfrStatus> {
    merge_key_components(
        &mut target.details.direct_signatures,
        &mut target.details.revocation_signatures,
        &mut target.details.users,
        &source.details.direct_signatures,
        &source.details.revocation_signatures,
        &source.details.users,
    )?;

    for incoming_sub in &source.public_subkeys {
        let incoming_fpr = incoming_sub.key.fingerprint().to_string();
        if let Some(dst_sub) = target
            .secret_subkeys
            .iter_mut()
            .find(|s| s.key.fingerprint().to_string() == incoming_fpr)
        {
            merge_signatures(&mut dst_sub.signatures, &incoming_sub.signatures)?;
        }
    }
    Ok(())
}

fn parse_signatures_from_armor(block: &str) -> Result<Vec<Signature>, GfrStatus> {
    let cursor = Cursor::new(block.as_bytes());
    let dearmor = armor::Dearmor::new(cursor);
    let mut buf_reader = BufReader::new(dearmor);

    let mut sigs = Vec::new();

    while let Ok(packet_header) = PacketHeader::try_from_reader(&mut buf_reader) {
        if let Ok(Packet::Signature(sig)) = Packet::from_reader(packet_header, &mut buf_reader) {
            sigs.push(sig);
        } else {
            // If we encounter a non-signature packet, we can choose to ignore it or break.
            // For now, let's just ignore it and continue parsing.
            continue;
        }
    }

    if sigs.is_empty() {
        Err(GfrStatus::ErrorInvalidInput)
    } else {
        Ok(sigs)
    }
}

/// Apply a revocation certificate to a key block.
///
/// The certificate's issuer fingerprint must match the base key's primary key;
/// mismatched fingerprints return `ErrorInvalidInput`. Duplicate revocation
/// signatures are deduplicated before the key is re-exported.
pub fn import_rev_cert_internal(
    base_key_block: &str,
    rev_cert_block: &str,
) -> Result<GeneratedKeys, GfrStatus> {
    let rev_sigs = extract_key_revocation_signatures(rev_cert_block)?;

    if let Ok((mut base_sk, _)) = SignedSecretKey::from_string(base_key_block) {
        let base_fpr_bytes = base_sk.primary_key.fingerprint().as_bytes().to_vec();

        if !rev_sigs
            .iter()
            .any(|sig| is_self_signature_from_primary(sig, &base_fpr_bytes))
        {
            return Err(GfrStatus::ErrorInvalidInput);
        }

        base_sk.details.revocation_signatures.extend(rev_sigs);
        dedup_signatures_in_place(&mut base_sk.details.revocation_signatures)?;
        dedup_signatures_in_place(&mut base_sk.details.direct_signatures)?;

        return export_secret_key(base_sk);
    }

    if let Ok((mut base_pk, _)) = SignedPublicKey::from_string(base_key_block) {
        let base_fpr_bytes = base_pk.primary_key.fingerprint().as_bytes().to_vec();

        if !rev_sigs
            .iter()
            .any(|sig| is_self_signature_from_primary(sig, &base_fpr_bytes))
        {
            return Err(GfrStatus::ErrorInvalidInput);
        }

        base_pk.details.revocation_signatures.extend(rev_sigs);
        dedup_signatures_in_place(&mut base_pk.details.revocation_signatures)?;
        dedup_signatures_in_place(&mut base_pk.details.direct_signatures)?;

        return export_public_key(base_pk);
    }

    Err(GfrStatus::ErrorInvalidInput)
}

/// Extract the fingerprint of the key targeted by a revocation certificate.
pub fn extract_rev_cert_target_fpr_internal(rev_cert_block: &str) -> Result<String, GfrStatus> {
    let rev_sigs = extract_key_revocation_signatures(rev_cert_block)?;
    let sig = rev_sigs.first().ok_or(GfrStatus::ErrorInvalidInput)?;

    let binding = sig.issuer_fingerprint();
    let issuer_fp = binding.first().ok_or(GfrStatus::ErrorInvalidInput)?;

    Ok(issuer_fp.to_string())
}
