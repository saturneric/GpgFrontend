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
    GfrKeyAlgo, GfrOpenPGPKeyVersion, GfrPasswordFetchCb, GfrRevocationCode, GfrStatus,
};
use crate::utils::{
    PassphraseStateInternal, armor_opts, build_revocation_reason_subpacket,
    choose_template_self_sig, determine_algo, extract_key_length, fetch_password_with_cache,
    password_from_zeroizing_bytes,
};
use pgp::armor::{self, BlockType};
use pgp::composed::{SignedPublicSubKey, SignedSecretSubKey};
use pgp::packet::{Packet, PacketHeader, SignatureConfig, SignatureType, Subpacket, SubpacketData};
use pgp::types::{Duration, KeyDetails, KeyVersion, Password, SecretParams, SignedUser, Tag};
use pgp::{
    composed::{Deserializable, SignedPublicKey, SignedSecretKey},
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
    /// Absolute expiration time (Unix epoch, seconds); `0` means never expires.
    pub expires_at: u32,
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
    /// Absolute expiration time (Unix epoch, seconds); `0` means never expires.
    pub expires_at: u32,
    pub has_secret: bool,
    pub is_revoked: bool,
    pub can_sign: bool,
    pub can_encrypt: bool,
    pub can_auth: bool,
    pub can_certify: bool,
    pub user_ids: Vec<ExtractUserId>,
    pub subkeys: Vec<ExtractedSubkey>,
    pub public_key_block: String,
    pub secret_key_block: Option<Zeroizing<String>>,
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

pub(crate) fn is_primary_key_revoked(signatures: &[Signature], primary_fpr_bytes: &[u8]) -> bool {
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

/// Resolve the absolute expiration time (Unix epoch, seconds) implied by a set
/// of self-signatures, given the target component's creation time.
///
/// The OpenPGP `KeyExpirationTime` subpacket encodes a duration measured from
/// the key's creation time (RFC 9580 §5.2.3.13), so the absolute value is
/// `created_at + duration`. Among the supplied signatures, the most-recently
/// created one that carries a non-zero `KeyExpirationTime` wins; a zero
/// duration or the absence of the subpacket both mean "never expires",
/// reported here as `0`.
fn expiration_from_self_sigs<'a, I>(sigs: I, created_at: u32) -> u32
where
    I: IntoIterator<Item = &'a Signature>,
{
    let mut best_sig_time: Option<u32> = None;
    let mut expires_at: u32 = 0;

    for sig in sigs {
        let Some(dur) = sig.key_expiration_time() else {
            continue;
        };
        let secs = dur.as_secs();
        if secs == 0 {
            continue;
        }

        let sig_time = sig.created().map(|t| t.as_secs()).unwrap_or(0);
        if best_sig_time.is_none_or(|prev| sig_time >= prev) {
            best_sig_time = Some(sig_time);
            expires_at = created_at.saturating_add(secs);
        }
    }

    expires_at
}

pub(crate) fn is_self_subkey_revocation(sig: &Signature, primary_fpr_bytes: &[u8]) -> bool {
    is_self_signature_from_primary(sig, primary_fpr_bytes)
        && matches!(sig.typ(), Some(SignatureType::SubkeyRevocation))
}

pub(crate) fn is_subkey_revoked(signatures: &[Signature], primary_fpr_bytes: &[u8]) -> bool {
    signatures
        .iter()
        .any(|sig| is_self_subkey_revocation(sig, primary_fpr_bytes))
}

/// Absolute expiration time (Unix epoch, seconds) of a certificate's primary
/// key, or `0` if it never expires.
///
/// Mirrors the primary-key expiry derivation in [`build_public_metadata`]: V4
/// keys carry the `KeyExpirationTime` in the primary User ID self-certification,
/// V6 keys in the direct-key signature (RFC 9580 §5.2.3.13). Shared with the
/// verification gates so a signature under an expired key is not reported valid.
pub(crate) fn primary_key_expires_at(cert: &SignedPublicKey) -> u32 {
    let users = &cert.details.users;
    let primary_idx = users.iter().position(|u| u.is_primary()).unwrap_or(0);
    let primary_user_sigs = users
        .get(primary_idx)
        .map(|u| u.signatures.as_slice())
        .unwrap_or(&[]);
    let created_at = cert.primary_key.created_at().as_secs();
    expiration_from_self_sigs(
        cert.details
            .direct_signatures
            .iter()
            .chain(primary_user_sigs.iter()),
        created_at,
    )
}

/// Absolute expiration time (Unix epoch, seconds) of a public subkey, from its
/// binding self-signatures, or `0` if it never expires (RFC 9580 §5.2.3.13).
pub(crate) fn subkey_expires_at(subkey: &SignedPublicSubKey) -> u32 {
    let created_at = subkey.key.created_at().as_secs();
    expiration_from_self_sigs(subkey.signatures.iter(), created_at)
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
        let created_at = sub.key.created_at().as_secs();
        let expires_at = expiration_from_self_sigs(sub.signatures.iter(), created_at);
        subs.push(ExtractedSubkey {
            ver: sub.version().into(),
            fpr: sub.key.fingerprint().to_string(),
            key_id: sub.key.legacy_key_id().to_string(),
            algo: determine_algo(sub.key.public_params()),
            created_at,
            expires_at,
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
    let created_at = pk.primary_key.created_at().as_secs();
    let expires_at = expiration_from_self_sigs(
        pk.details
            .direct_signatures
            .iter()
            .chain(primary_user_sigs.iter()),
        created_at,
    );

    ExtractedMetadata {
        ver: pk.version().into(),
        fpr: pk.primary_key.fingerprint().to_string(),
        key_id: pk.primary_key.legacy_key_id().to_string(),
        algo: determine_algo(pk.primary_key.public_params()),
        created_at,
        expires_at,
        has_secret: true,
        is_revoked,
        can_sign,
        can_encrypt,
        can_auth,
        can_certify,
        user_ids, // Assign the collected vector here
        subkeys: subs,
        public_key_block: pk.to_armored_string(armor_opts()).unwrap_or_default(),
        secret_key_block: sk.to_armored_string(armor_opts()).ok().map(Zeroizing::new),
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
        let created_at = sub.key.created_at().as_secs();
        let expires_at = expiration_from_self_sigs(sub.signatures.iter(), created_at);
        subs.push(ExtractedSubkey {
            ver: sub.version().into(),
            fpr: sub.key.fingerprint().to_string(),
            key_id: sub.key.legacy_key_id().to_string(),
            algo: determine_algo(sub.key.public_params()),
            created_at,
            expires_at,
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
    let created_at = pk.primary_key.created_at().as_secs();
    let expires_at = expiration_from_self_sigs(
        pk.details
            .direct_signatures
            .iter()
            .chain(primary_user_sigs.iter()),
        created_at,
    );
    ExtractedMetadata {
        ver: pk.version().into(),
        fpr: pk.primary_key.fingerprint().to_string(),
        key_id: pk.primary_key.legacy_key_id().to_string(),
        user_ids,
        algo: determine_algo(pk.primary_key.public_params()),
        key_length: key_length.unwrap_or(0),
        created_at,
        expires_at,
        has_secret: false,
        is_revoked,
        subkeys: subs,
        can_sign,
        can_encrypt,
        can_auth,
        can_certify,
        public_key_block: pk.to_armored_string(armor_opts()).unwrap_or_default(),
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

                    metadata.secret_key_block =
                        sk.to_armored_string(armor_opts()).ok().map(Zeroizing::new);

                    let public_key = SignedPublicKey::from(sk.clone());
                    metadata.public_key_block = public_key
                        .to_armored_string(armor_opts())
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
                        metadata.public_key_block =
                            pk.to_armored_string(armor_opts()).unwrap_or_default();
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
        .to_armored_string(armor_opts())
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
        None,
        false, // RFC 9580 §6.1: do not emit a CRC24 footer (forbidden for v6)
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
        false, // RFC 9580 §6.1: do not emit a CRC24 footer (forbidden for v6)
    )
    .map_err(|_| GfrStatus::ErrorArmorFailed)?;

    // 5. Convert the armored bytes back to a String.
    String::from_utf8(armored_output).map_err(|_| GfrStatus::ErrorArmorFailed)
}

/// Prompt for the key's current passphrase, to unlock it before re-protection.
fn fetch_old_password(
    channel: i32,
    target_fpr: &str,
    purpose: &str,
    fetch_pwd_cb: Option<GfrPasswordFetchCb>,
) -> Result<Password, GfrStatus> {
    let bytes = fetch_password_with_cache(
        Some(&PASSWORD_CACHE),
        PasswordCachePolicy::Default,
        channel,
        PassphraseStateInternal {
            fpr: target_fpr.to_string(),
            info: purpose.to_string(),
            retry: false,
            ask_for_new: false,
            should_confirm: false,
        },
        fetch_pwd_cb,
    )?;

    if bytes.is_empty() {
        return Err(GfrStatus::ErrorFetchPasswordFailed);
    }

    Ok(password_from_zeroizing_bytes(bytes))
}

/// Prompt for the new passphrase to re-protect the key with.
///
/// Only ever called once the key has actually been unlocked, so the user is not
/// asked to choose a new passphrase for a key they could not open.
fn fetch_new_password(
    channel: i32,
    target_fpr: &str,
    purpose: &str,
    fetch_pwd_cb: Option<GfrPasswordFetchCb>,
) -> Result<Password, GfrStatus> {
    let bytes = fetch_password_with_cache(
        Some(&PASSWORD_CACHE),
        PasswordCachePolicy::Bypass,
        channel,
        PassphraseStateInternal {
            fpr: target_fpr.to_string(),
            info: purpose.to_string(),
            retry: false,
            ask_for_new: true,
            should_confirm: true,
        },
        fetch_pwd_cb,
    )?;

    if bytes.is_empty() {
        return Err(GfrStatus::ErrorFetchPasswordFailed);
    }

    Ok(password_from_zeroizing_bytes(bytes))
}

/// Prompt for a key's current passphrase again, targeted at `fpr`.
///
/// Used when a passphrase already collected for the key block is rejected by an
/// individual subkey, which may legitimately carry a different one.
fn fetch_retry_password(
    channel: i32,
    fpr: &str,
    purpose: &str,
    fetch_pwd_cb: Option<GfrPasswordFetchCb>,
) -> Result<Password, GfrStatus> {
    let bytes = fetch_password_with_cache(
        Some(&PASSWORD_CACHE),
        PasswordCachePolicy::Bypass,
        channel,
        PassphraseStateInternal {
            fpr: fpr.to_string(),
            info: purpose.to_string(),
            retry: true,
            ask_for_new: false,
            should_confirm: false,
        },
        fetch_pwd_cb,
    )?;

    if bytes.is_empty() {
        return Err(GfrStatus::ErrorFetchPasswordFailed);
    }

    Ok(password_from_zeroizing_bytes(bytes))
}

/// How many times a rejected unlock passphrase may be re-entered before the
/// operation gives up, mirroring gpg-agent's default pinentry allowance.
const PASSPHRASE_RETRY_LIMIT: usize = 3;

/// Strip the passphrase from one key, re-prompting when the one supplied is
/// rejected.
///
/// `initial` is tried first when present — the passphrase already collected for
/// this key block, which is usually right and costs no extra prompt. Each
/// rejection spends one retry prompt targeted at `fpr`, so a typo (or a subkey
/// carrying a different passphrase) is recoverable instead of aborting the whole
/// operation. Exhausting the retries reports `ErrorBadPassphrase`, which the C++
/// side turns into `GPG_ERR_BAD_PASSPHRASE` — a bare `ErrorInvalidInput` from
/// the underlying unlock would surface to the user as "General error".
fn remove_password_with_retry<F>(
    channel: i32,
    fpr: &str,
    purpose: &str,
    initial: Option<&Password>,
    fetch_pwd_cb: Option<GfrPasswordFetchCb>,
    mut remove: F,
) -> Result<(), GfrStatus>
where
    F: FnMut(&Password) -> bool,
{
    if let Some(pw) = initial {
        if remove(pw) {
            return Ok(());
        }
        // Whatever is cached for this key was just proven wrong; leaving it in
        // place would feed the same rejected passphrase to the next operation.
        PASSWORD_CACHE.remove_by_fpr(fpr);
        log::warn!("passphrase rejected for key {}, re-prompting", fpr);
    }

    for _ in 0..PASSPHRASE_RETRY_LIMIT {
        let pw = fetch_retry_password(channel, fpr, purpose, fetch_pwd_cb)?;
        if remove(&pw) {
            return Ok(());
        }
        log::warn!("passphrase rejected for key {}, re-prompting", fpr);
    }

    log::error!("passphrase for key {} rejected after retries", fpr);
    Err(GfrStatus::ErrorBadPassphrase)
}

/// Re-protect every secret key in the block — primary and all subkeys — under a
/// single new passphrase.
///
/// This mirrors `gpg --passwd` (what `gpgme_op_passwd` runs), which re-protects
/// the primary and every subkey together. One unlock prompt and one new-password
/// prompt are issued against the primary fingerprint; a subkey that rejects the
/// shared unlock passphrase gets its own targeted retry prompt.
fn change_whole_key_password(
    channel: i32,
    secret_key: &mut SignedSecretKey,
    fetch_pwd_cb: Option<GfrPasswordFetchCb>,
) -> Result<(), GfrStatus> {
    let primary_fpr = secret_key
        .primary_key
        .fingerprint()
        .to_string()
        .to_uppercase();

    let any_encrypted = secret_key.primary_key.secret_params().is_encrypted()
        || secret_key
            .secret_subkeys
            .iter()
            .any(|s| s.key.secret_params().is_encrypted());

    // Unlock everything first, so the new passphrase is only asked for once the
    // key is proven to open — never prompt for a new password before the old one
    // is verified. The old passphrase is fetched once and reused across keys; a
    // key that rejects it is re-prompted on its own.
    let old_pw = if any_encrypted {
        Some(fetch_old_password(
            channel,
            &primary_fpr,
            "Unlock Key to change password",
            fetch_pwd_cb,
        )?)
    } else {
        None
    };

    if secret_key.primary_key.secret_params().is_encrypted() {
        let primary = &mut secret_key.primary_key;
        remove_password_with_retry(
            channel,
            &primary_fpr,
            "Unlock Primary Key to change password",
            old_pw.as_ref(),
            fetch_pwd_cb,
            |pw| primary.remove_password(pw).is_ok(),
        )?;
    }

    for subkey in secret_key.secret_subkeys.iter_mut() {
        if subkey.key.secret_params().is_encrypted() {
            let sub_fpr = subkey.key.fingerprint().to_string().to_uppercase();
            let key = &mut subkey.key;
            remove_password_with_retry(
                channel,
                &sub_fpr,
                "Unlock Subkey to change password",
                old_pw.as_ref(),
                fetch_pwd_cb,
                |pw| key.remove_password(pw).is_ok(),
            )?;
        }
    }

    // Everything is open; now collect the single new passphrase and apply it.
    let new_pw = fetch_new_password(
        channel,
        &primary_fpr,
        "Set new password for Key",
        fetch_pwd_cb,
    )?;

    let mut rng = rand::thread_rng();

    secret_key
        .primary_key
        .set_password(&mut rng, &new_pw)
        .into_gfr()?;

    // Invalidate cache entries for this key
    PASSWORD_CACHE.remove_by_fpr(&primary_fpr);

    for subkey in secret_key.secret_subkeys.iter_mut() {
        let sub_fpr = subkey.key.fingerprint().to_string().to_uppercase();
        subkey.key.set_password(&mut rng, &new_pw).into_gfr()?;

        // Decrypt and sign cache under the subkey fingerprint, so a stale entry
        // here would keep serving the passphrase that was just replaced.
        PASSWORD_CACHE.remove_by_fpr(&sub_fpr);
    }

    Ok(())
}

/// Re-protect exactly one key — the primary or a single subkey — identified by
/// `target_fpr`, leaving every other key in the block untouched.
fn change_single_key_password(
    channel: i32,
    secret_key: &mut SignedSecretKey,
    target_fpr: &str,
    fetch_pwd_cb: Option<GfrPasswordFetchCb>,
) -> Result<(), GfrStatus> {
    let primary_fpr = secret_key
        .primary_key
        .fingerprint()
        .to_string()
        .to_uppercase();

    let mut rng = rand::thread_rng();

    if primary_fpr == target_fpr {
        // Unlock the key first; only prompt for the new passphrase once it opens.
        if secret_key.primary_key.secret_params().is_encrypted() {
            let old_pw = fetch_old_password(
                channel,
                target_fpr,
                "Unlock Primary Key to change password",
                fetch_pwd_cb,
            )?;
            let primary = &mut secret_key.primary_key;
            remove_password_with_retry(
                channel,
                &primary_fpr,
                "Unlock Primary Key to change password",
                Some(&old_pw),
                fetch_pwd_cb,
                |pw| primary.remove_password(pw).is_ok(),
            )?;
        }

        let new_pw = fetch_new_password(
            channel,
            target_fpr,
            "Set new password for Primary Key",
            fetch_pwd_cb,
        )?;

        secret_key
            .primary_key
            .set_password(&mut rng, &new_pw)
            .into_gfr()?;

        // Invalidate cache entries for this key
        PASSWORD_CACHE.remove_by_fpr(&primary_fpr);

        return Ok(());
    }

    for subkey in secret_key.secret_subkeys.iter_mut() {
        let sub_fpr = subkey.key.fingerprint().to_string().to_uppercase();

        if sub_fpr != target_fpr {
            continue;
        }

        // Unlock the subkey first; only prompt for the new passphrase once it
        // opens, so a mistyped current passphrase never costs a new one.
        if subkey.key.secret_params().is_encrypted() {
            let old_pw = fetch_old_password(
                channel,
                target_fpr,
                "Unlock Subkey to change password",
                fetch_pwd_cb,
            )?;
            let key = &mut subkey.key;
            remove_password_with_retry(
                channel,
                &sub_fpr,
                "Unlock Subkey to change password",
                Some(&old_pw),
                fetch_pwd_cb,
                |pw| key.remove_password(pw).is_ok(),
            )?;
        }

        let new_pw = fetch_new_password(
            channel,
            target_fpr,
            "Set new password for Subkey",
            fetch_pwd_cb,
        )?;

        subkey.key.set_password(&mut rng, &new_pw).into_gfr()?;

        // Decrypt and sign cache under the subkey fingerprint, so a stale entry
        // here would keep serving the passphrase that was just replaced.
        PASSWORD_CACHE.remove_by_fpr(&sub_fpr);

        return Ok(());
    }

    Err(GfrStatus::ErrorInvalidInput)
}

/// Change the passphrase protecting a secret key block.
///
/// `target_fpr` selects the scope: `None` re-protects the whole key — primary and
/// every subkey — under one new passphrase, matching `gpg --passwd`. `Some(fpr)`
/// re-protects only that key, which may be the primary or any subkey. The entire
/// key block is re-exported either way, so the returned block contains all keys.
pub fn modify_key_password_internal(
    channel: i32,
    secret_key_block: &str,
    target_fpr: Option<&str>,
    fetch_pwd_cb: Option<GfrPasswordFetchCb>,
) -> Result<GeneratedKeys, GfrStatus> {
    let (mut secret_key, _) = SignedSecretKey::from_string(secret_key_block).map_err(|e| {
        log::error!("Failed to parse secret key block: {}", e);
        GfrStatus::ErrorInvalidData
    })?;

    match target_fpr {
        Some(fpr) => {
            change_single_key_password(channel, &mut secret_key, &fpr.to_uppercase(), fetch_pwd_cb)?
        }
        None => change_whole_key_password(channel, &mut secret_key, fetch_pwd_cb)?,
    }

    export_secret_key(secret_key)
}

/// Convert an absolute expiration time into the OpenPGP duration-from-creation
/// form used by the `KeyExpirationTime` subpacket.
///
/// `expiration_epoch_secs == 0` means "never expires" and yields `None`. An
/// expiration at or before the component's creation time is rejected as
/// `ErrorInvalidInput`, as is one so far in the future it overflows the 4-octet
/// duration field.
fn expiration_duration(
    created_at: u32,
    expiration_epoch_secs: u64,
) -> Result<Option<Duration>, GfrStatus> {
    if expiration_epoch_secs == 0 {
        return Ok(None);
    }

    let created = u64::from(created_at);
    if expiration_epoch_secs <= created {
        return Err(GfrStatus::ErrorInvalidInput);
    }

    let secs =
        u32::try_from(expiration_epoch_secs - created).map_err(|_| GfrStatus::ErrorInvalidInput)?;

    Ok(Some(Duration::from_secs(secs)))
}

/// Clone a template self-signature's config and rewrite it to carry `duration`
/// as the key expiration, preserving every other subpacket.
///
/// Key flags, algorithm preferences, features, the primary-UID flag, and any
/// embedded primary-key-binding back-signature (on signing subkeys) all survive
/// because the whole config — including its signature type and version — is
/// cloned from the existing signature. Only the issuer fingerprint, signature
/// creation time, and key expiration are refreshed. A `None` duration drops the
/// expiration entirely ("never expires").
fn build_expiration_sig_config(
    template: &Signature,
    primary_fpr: pgp::types::Fingerprint,
    duration: Option<Duration>,
) -> Result<SignatureConfig, GfrStatus> {
    let mut cfg = template.config().cloned().ok_or(GfrStatus::ErrorInternal)?;

    let strip = |subpackets: &mut Vec<Subpacket>| {
        subpackets.retain(|sp| {
            !matches!(
                sp.data,
                SubpacketData::SignatureCreationTime(_)
                    | SubpacketData::IssuerFingerprint(_)
                    | SubpacketData::IssuerKeyId(_)
                    | SubpacketData::KeyExpirationTime(_)
            )
        });
    };
    strip(&mut cfg.hashed_subpackets);
    strip(&mut cfg.unhashed_subpackets);

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
    if let Some(dur) = duration {
        cfg.hashed_subpackets.push(
            Subpacket::regular(SubpacketData::KeyExpirationTime(dur))
                .map_err(|_| GfrStatus::ErrorInternal)?,
        );
    }

    Ok(cfg)
}

/// Re-issue the primary key's self-signature(s) with a new expiration.
///
/// V6 keys carry key expiration in the direct-key signature (0x1F); V4 keys
/// carry it in each User ID's self-certification. The corresponding signatures
/// are rebuilt from the current ones and replaced in place, so a key with no
/// usable self-signature is rejected as `ErrorInvalidData`.
fn update_primary_expiration(
    secret_key: &mut SignedSecretKey,
    duration: Option<Duration>,
    pwd: &Password,
) -> Result<(), GfrStatus> {
    let primary_fpr = secret_key.primary_key.fingerprint();
    let primary_fpr_bytes = primary_fpr.as_bytes().to_vec();
    let pk = secret_key.primary_key.public_key();

    if secret_key.primary_key.version() == KeyVersion::V6 {
        let template = {
            let self_sigs: Vec<&Signature> = secret_key
                .details
                .direct_signatures
                .iter()
                .filter(|sig| is_self_signature_from_primary(sig, &primary_fpr_bytes))
                .collect();
            choose_template_self_sig(&self_sigs)
                .cloned()
                .ok_or(GfrStatus::ErrorInvalidData)?
        };

        let cfg = build_expiration_sig_config(&template, primary_fpr, duration)?;
        let new_sig = cfg.sign_key(&secret_key.primary_key, pwd, &pk).into_gfr()?;

        secret_key
            .details
            .direct_signatures
            .retain(|sig| !is_self_signature_from_primary(sig, &primary_fpr_bytes));
        secret_key.details.direct_signatures.push(new_sig);

        return Ok(());
    }

    // V4: re-sign the self-certification on every User ID that carries one.
    let mut updated_any = false;
    for user in secret_key.details.users.iter_mut() {
        let template = {
            let self_sigs: Vec<&Signature> = user
                .signatures
                .iter()
                .filter(|sig| is_self_signature_from_primary(sig, &primary_fpr_bytes))
                .collect();
            match choose_template_self_sig(&self_sigs) {
                Some(sig) => sig.clone(),
                None => continue,
            }
        };

        let cfg = build_expiration_sig_config(&template, primary_fpr.clone(), duration)?;
        let new_sig = cfg
            .sign_certification(&secret_key.primary_key, &pk, pwd, Tag::UserId, &user.id)
            .into_gfr()?;

        user.signatures
            .retain(|sig| !is_self_signature_from_primary(sig, &primary_fpr_bytes));
        user.signatures.push(new_sig);
        updated_any = true;
    }

    if updated_any {
        Ok(())
    } else {
        Err(GfrStatus::ErrorInvalidData)
    }
}

/// Re-issue a single subkey's binding signature with a new expiration.
///
/// The subkey binding signature (0x18) is rebuilt from the current one and
/// replaced in place; any revocation signatures on the subkey are preserved.
fn update_subkey_expiration(
    secret_key: &mut SignedSecretKey,
    target_fpr: &str,
    duration: Option<Duration>,
    pwd: &Password,
) -> Result<(), GfrStatus> {
    let target_idx = secret_key
        .secret_subkeys
        .iter()
        .position(|sub| sub.key.fingerprint().to_string().to_uppercase() == target_fpr)
        .ok_or(GfrStatus::ErrorInvalidInput)?;

    let primary_fpr = secret_key.primary_key.fingerprint();
    let primary_fpr_bytes = primary_fpr.as_bytes().to_vec();
    let pk = secret_key.primary_key.public_key();

    let subkey = secret_key
        .secret_subkeys
        .get_mut(target_idx)
        .ok_or(GfrStatus::ErrorInternal)?;

    let template = {
        let binding_sigs: Vec<&Signature> = subkey
            .signatures
            .iter()
            .filter(|sig| {
                is_self_signature_from_primary(sig, &primary_fpr_bytes)
                    && matches!(sig.typ(), Some(SignatureType::SubkeyBinding))
            })
            .collect();
        choose_template_self_sig(&binding_sigs)
            .cloned()
            .ok_or(GfrStatus::ErrorInvalidData)?
    };

    let cfg = build_expiration_sig_config(&template, primary_fpr, duration)?;
    let new_sig = cfg
        .sign_subkey_binding(&secret_key.primary_key, &pk, pwd, subkey.key.public_key())
        .into_gfr()?;

    subkey.signatures.retain(|sig| {
        !(is_self_signature_from_primary(sig, &primary_fpr_bytes)
            && matches!(sig.typ(), Some(SignatureType::SubkeyBinding)))
    });
    subkey.signatures.push(new_sig);

    Ok(())
}

/// Change the expiration of a primary key or a single subkey.
///
/// `target_fpr` selects the scope: `None`, an empty string, or the primary key
/// fingerprint targets the primary key; any other fingerprint targets that
/// subkey. `expiration_epoch_secs` is an absolute Unix time, or `0` for "never
/// expires". The primary key is unlocked once (its passphrase drives the
/// re-signing in every case) and the whole key block is re-exported.
pub fn update_key_expiration_internal(
    channel: i32,
    secret_key_block: &str,
    target_fpr: Option<&str>,
    expiration_epoch_secs: u64,
    fetch_cb: Option<GfrPasswordFetchCb>,
) -> Result<GeneratedKeys, GfrStatus> {
    let (mut secret_key, _) = SignedSecretKey::from_string(secret_key_block).map_err(|e| {
        log::error!("Failed to parse secret key block: {}", e);
        GfrStatus::ErrorInvalidData
    })?;

    let primary_fpr = secret_key
        .primary_key
        .fingerprint()
        .to_string()
        .to_uppercase();

    let normalized_target = target_fpr.map(|s| s.to_uppercase());
    let is_primary_target = match normalized_target.as_deref() {
        None | Some("") => true,
        Some(fpr) => fpr == primary_fpr,
    };

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
                fpr: primary_fpr.clone(),
                info: "Unlock Primary Key to change expiration".to_string(),
                retry: false,
                ask_for_new: false,
                should_confirm: false,
            },
            fetch_cb,
        )?
    } else {
        Zeroizing::new(Vec::new())
    };
    let pwd = password_from_zeroizing_bytes(pwd_bytes);

    if is_primary_target {
        let created_at = secret_key.primary_key.created_at().as_secs();
        let duration = expiration_duration(created_at, expiration_epoch_secs)?;
        update_primary_expiration(&mut secret_key, duration, &pwd)?;
    } else {
        let target = normalized_target.as_deref().unwrap_or_default();
        let created_at = secret_key
            .secret_subkeys
            .iter()
            .find(|sub| sub.key.fingerprint().to_string().to_uppercase() == target)
            .map(|sub| sub.key.created_at().as_secs())
            .ok_or(GfrStatus::ErrorInvalidInput)?;
        let duration = expiration_duration(created_at, expiration_epoch_secs)?;
        update_subkey_expiration(&mut secret_key, target, duration, &pwd)?;
    }

    export_secret_key(secret_key)
}

/// Stamp an absolute expiration onto the primary key of an in-memory secret key.
///
/// A thin wrapper over [`update_primary_expiration`] that resolves the
/// duration-from-creation form from the primary key's own creation time, so the
/// generation path can express expirations as absolute Unix times just like the
/// FFI. `expiration_epoch_secs == 0` clears the expiration. The primary key must
/// already be unlocked by `pwd` (an empty password for a freshly generated,
/// not-yet-protected key).
pub(crate) fn apply_primary_key_expiration(
    secret_key: &mut SignedSecretKey,
    expiration_epoch_secs: u64,
    pwd: &Password,
) -> Result<(), GfrStatus> {
    let created_at = secret_key.primary_key.created_at().as_secs();
    let duration = expiration_duration(created_at, expiration_epoch_secs)?;
    update_primary_expiration(secret_key, duration, pwd)
}

/// Stamp an absolute expiration onto a single subkey of an in-memory secret key.
///
/// The companion of [`apply_primary_key_expiration`] for subkeys. Re-issuing the
/// binding signature only needs the primary key unlocked by `pwd`; the subkey's
/// own protection is irrelevant. `expiration_epoch_secs == 0` clears the
/// expiration.
pub(crate) fn apply_subkey_expiration(
    secret_key: &mut SignedSecretKey,
    target_fpr: &str,
    expiration_epoch_secs: u64,
    pwd: &Password,
) -> Result<(), GfrStatus> {
    let normalized = target_fpr.to_uppercase();
    let created_at = secret_key
        .secret_subkeys
        .iter()
        .find(|sub| sub.key.fingerprint().to_string().to_uppercase() == normalized)
        .map(|sub| sub.key.created_at().as_secs())
        .ok_or(GfrStatus::ErrorInvalidInput)?;
    let duration = expiration_duration(created_at, expiration_epoch_secs)?;
    update_subkey_expiration(secret_key, &normalized, duration, pwd)
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

    let armored_s_key = secret_key.to_armored_string(armor_opts()).into_gfr()?;

    let public_key = SignedPublicKey::from(secret_key);
    let armored_p_key = public_key.to_armored_string(armor_opts()).into_gfr()?;

    Ok(GeneratedKeys {
        secret: Zeroizing::new(armored_s_key),
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

    // The signature version MUST match the primary key version: a v6 key MUST
    // produce a v6 signature (RFC 9580 §10.3.2.2), and a non-v6 signature made by
    // a v6 key MUST be ignored by conforming verifiers (§5.2.5). `from_key`
    // selects v4 for a v4 key and v6 (with the required salt) for a v6 key.
    let mut rng = rand::thread_rng();
    let mut cfg = SignatureConfig::from_key(
        &mut rng,
        &secret_key.primary_key,
        SignatureType::SubkeyRevocation,
    )
    .map_err(|_| GfrStatus::ErrorInternal)?;

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
        .to_armored_string(armor_opts())
        .map_err(|_| GfrStatus::ErrorArmorFailed)?;

    let public_key = SignedPublicKey::from(secret_key);
    let armored_p_key = public_key
        .to_armored_string(armor_opts())
        .map_err(|_| GfrStatus::ErrorArmorFailed)?;

    Ok(GeneratedKeys {
        secret: Zeroizing::new(armored_s_key),
        public: armored_p_key,
        fingerprint: fpr,
    })
}

/// Generate a standalone revocation certificate for the primary key.
///
/// The certificate is a single `KeyRevocation` signature packet wrapped in a
/// "PUBLIC KEY BLOCK" armor. It can be imported into any key block with the
/// matching fingerprint via `import_rev_cert_internal`. The signature version
/// matches the key version (v4 for v4 keys, v6 for v6 keys); a v6 key MUST NOT
/// emit a v4 signature, as conforming verifiers ignore it (RFC 9580 §5.2.5).
pub fn generate_key_rev_cert_internal(
    channel: i32,
    secret_key_block: &str,
    reason_code: GfrRevocationCode,
    reason_text: Option<&str>,
    fetch_cb: Option<GfrPasswordFetchCb>,
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

    // Version-aware: v4 key -> v4 signature, v6 key -> v6 signature (RFC 9580
    // §10.3.2.2 / §5.2.5). See `from_key` at pgp::packet::SignatureConfig.
    let mut rng = rand::thread_rng();
    let mut cfg =
        SignatureConfig::from_key(&mut rng, &skey.primary_key, SignatureType::KeyRevocation)
            .map_err(|_| GfrStatus::ErrorInternal)?;

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
        false, // RFC 9580 §6.1: do not emit a CRC24 footer (forbidden for v6)
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
    let secret = Zeroizing::new(sk.to_armored_string(armor_opts()).into_gfr()?);
    let public = SignedPublicKey::from(sk)
        .to_armored_string(armor_opts())
        .into_gfr()?;

    Ok(GeneratedKeys {
        secret,
        public,
        fingerprint,
    })
}

fn export_public_key(pk: SignedPublicKey) -> Result<GeneratedKeys, GfrStatus> {
    let fingerprint = pk.fingerprint().to_string();
    let public = pk.to_armored_string(armor_opts()).into_gfr()?;

    Ok(GeneratedKeys {
        secret: Zeroizing::new(String::new()),
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
    )?;

    merge_secret_subkeys(&mut target.secret_subkeys, &source.secret_subkeys)?;

    Ok(())
}

fn merge_secret_subkeys(
    dst: &mut Vec<SignedSecretSubKey>,
    src: &[SignedSecretSubKey],
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

#[cfg(test)]
mod rfc9580_tests {
    //! RFC 9580 conformance tests for revocation signatures and Appendix A
    //! known-answer vectors.
    use super::*;
    use crate::keygen::keygen_dynamic;
    use crate::types::{GfrKeyAlgo, GfrKeyConfig, GfrOpenPGPKeyVersion};

    fn cfg(algo: GfrKeyAlgo, sign: bool, enc: bool, ver: GfrOpenPGPKeyVersion) -> GfrKeyConfig {
        GfrKeyConfig {
            algo,
            can_sign: sign,
            can_encrypt: enc,
            can_auth: false,
            has_passphrase: false,
            ver,
            expiration_epoch_secs: 0,
        }
    }

    /// RFC 9580 §10.3.2.2 / §5.2.5: a v6 key MUST produce v6 signatures. A subkey
    /// revocation on a v6 key must be a v6 signature (a v4 one would be ignored by
    /// conforming verifiers, silently failing the revocation).
    #[test]
    fn v6_subkey_revocation_is_v6_signature() {
        let primary = cfg(GfrKeyAlgo::ED25519, true, false, GfrOpenPGPKeyVersion::V6);
        let sub = cfg(GfrKeyAlgo::ED25519, false, true, GfrOpenPGPKeyVersion::V6);
        let key = keygen_dynamic("rev <rev@example.com>", &primary, &[sub]).expect("keygen");
        assert_eq!(key.primary_key.version(), KeyVersion::V6);

        let sub_fpr = key.secret_subkeys[0].key.fingerprint().to_string();
        let armored = key.to_armored_string(armor_opts()).expect("armor");

        let result = revoke_subkey_internal(
            0,
            &armored,
            &sub_fpr,
            GfrRevocationCode::Compromised,
            None,
            None,
        )
        .expect("revoke");

        let (revoked, _) = SignedSecretKey::from_string(&result.secret).expect("parse");
        let rev_sig = revoked.secret_subkeys[0]
            .signatures
            .iter()
            .find(|s| matches!(s.typ(), Some(SignatureType::SubkeyRevocation)))
            .expect("subkey revocation signature present");
        assert_eq!(
            rev_sig.version(),
            pgp::packet::SignatureVersion::V6,
            "revocation signature on a v6 key must be a v6 signature"
        );
    }

    /// Sanity check: a v4 key still produces v4 revocation signatures (no regression).
    #[test]
    fn v4_subkey_revocation_is_v4_signature() {
        let primary = cfg(GfrKeyAlgo::ED25519, true, false, GfrOpenPGPKeyVersion::V4);
        let sub = cfg(GfrKeyAlgo::ED25519, false, true, GfrOpenPGPKeyVersion::V4);
        let key = keygen_dynamic("rev <rev@example.com>", &primary, &[sub]).expect("keygen");
        let sub_fpr = key.secret_subkeys[0].key.fingerprint().to_string();
        let armored = key.to_armored_string(armor_opts()).expect("armor");

        let result = revoke_subkey_internal(
            0,
            &armored,
            &sub_fpr,
            GfrRevocationCode::Retired,
            None,
            None,
        )
        .expect("revoke");
        let (revoked, _) = SignedSecretKey::from_string(&result.secret).expect("parse");
        let rev_sig = revoked.secret_subkeys[0]
            .signatures
            .iter()
            .find(|s| matches!(s.typ(), Some(SignatureType::SubkeyRevocation)))
            .expect("subkey revocation signature present");
        assert_eq!(rev_sig.version(), pgp::packet::SignatureVersion::V4);
    }

    // ---- RFC 9580 Appendix A known-answer vectors ----

    /// RFC 9580 Appendix A.3 — sample v6 certificate (transferable public key).
    const A3_V6_CERT: &str = "-----BEGIN PGP PUBLIC KEY BLOCK-----\n\
\n\
xioGY4d/4xsAAAAg+U2nu0jWCmHlZ3BqZYfQMxmZu52JGggkLq2EVD34laPCsQYf\n\
GwoAAABCBYJjh3/jAwsJBwUVCg4IDAIWAAKbAwIeCSIhBssYbE8GCaaX5NUt+mxy\n\
KwwfHifBilZwj2Ul7Ce62azJBScJAgcCAAAAAK0oIBA+LX0ifsDm185Ecds2v8lw\n\
gyU2kCcUmKfvBXbAf6rhRYWzuQOwEn7E/aLwIwRaLsdry0+VcallHhSu4RN6HWaE\n\
QsiPlR4zxP/TP7mhfVEe7XWPxtnMUMtf15OyA51YBM4qBmOHf+MZAAAAIIaTJINn\n\
+eUBXbki+PSAld2nhJh/LVmFsS+60WyvXkQ1wpsGGBsKAAAALAWCY4d/4wKbDCIh\n\
BssYbE8GCaaX5NUt+mxyKwwfHifBilZwj2Ul7Ce62azJAAAAAAQBIKbpGG2dWTX8\n\
j+VjFM21J0hqWlEg+bdiojWnKfA5AQpWUWtnNwDEM0g12vYxoWM8Y81W+bHBw805\n\
I8kWVkXU6vFOi+HWvv/ira7ofJu16NnoUkhclkUrk0mXubZvyl4GBg==\n\
-----END PGP PUBLIC KEY BLOCK-----\n";

    /// RFC 9580 Appendix A.6 — cleartext signed message, verifiable with A.3.
    const A6_CLEARTEXT: &str = "-----BEGIN PGP SIGNED MESSAGE-----\n\
\n\
What we need from the grocery store:\n\
\n\
- - tofu\n\
- - vegetables\n\
- - noodles\n\
\n\
-----BEGIN PGP SIGNATURE-----\n\
\n\
wpgGARsKAAAAKQWCY5ijYyIhBssYbE8GCaaX5NUt+mxyKwwfHifBilZwj2Ul7Ce6\n\
2azJAAAAAGk2IHZJX1AhiJD39eLuPBgiUU9wUA9VHYblySHkBONKU/usJ9BvuAqo\n\
/FvLFuGWMbKAdA+epq7V4HOtAPlBWmU8QOd6aud+aSunHQaaEJ+iTFjP2OMW0KBr\n\
NK2ay45cX1IVAQ==\n\
-----END PGP SIGNATURE-----\n";

    /// The v6 certificate parses and yields the fingerprint stated in the RFC.
    #[test]
    fn appendix_a3_v6_cert_fingerprint() {
        let (cert, _) = SignedPublicKey::from_string(A3_V6_CERT).expect("parse A.3 cert");
        assert_eq!(cert.primary_key.version(), KeyVersion::V6);
        assert_eq!(
            cert.primary_key.fingerprint().to_string().to_uppercase(),
            "CB186C4F0609A697E4D52DFA6C722B0C1F1E27C18A56708F6525EC27BAD9ACC9"
        );
    }

    /// The Appendix A.6 cleartext message verifies against the A.3 certificate,
    /// and its signature is a v6 signature — exercising the whole v6 verify path.
    #[test]
    fn appendix_a6_cleartext_verifies() {
        use pgp::composed::{CleartextSignedMessage, Deserializable};
        let (cert, _) = SignedPublicKey::from_string(A3_V6_CERT).expect("parse A.3 cert");
        let (msg, _) = CleartextSignedMessage::from_string(A6_CLEARTEXT).expect("parse A.6");
        msg.verify(&cert)
            .expect("A.6 cleartext signature must verify against A.3 cert");
        assert!(
            msg.signatures()
                .iter()
                .all(|s| s.version() == pgp::packet::SignatureVersion::V6),
            "Appendix A.6 signature must be a v6 signature"
        );
    }
}
