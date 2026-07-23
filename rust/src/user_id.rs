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

//! User ID management: add, delete, update, set-primary, and revoke.
//!
//! All functions accept and return armored key blocks. They try to parse the
//! block as a secret key first; if that fails they fall back to a public key.
//! Operations that require signing (add, update, revoke) only work on secret keys.

use pgp::{
    composed::{Deserializable, SignedPublicKey, SignedSecretKey},
    crypto::{hash::HashAlgorithm, public_key},
    packet::{Signature, SignatureConfig, SignatureType, Subpacket, SubpacketData},
    types::{
        KeyDetails, KeyVersion, PacketHeaderVersion, SecretParams, SignedUser, Tag, Timestamp,
    },
};
use zeroize::Zeroizing;

use crate::{
    cache::{PASSWORD_CACHE, PasswordCachePolicy},
    err::IntoGfrResult,
    types::{GfrPasswordFetchCb, GfrRevocationCode, GfrStatus},
    utils::{
        PassphraseStateInternal, armor_opts, build_revocation_reason_subpacket, choose_template_self_sig,
        fetch_password_with_cache, has_is_primary_true, is_self_signature_from_primary,
        password_from_zeroizing_bytes,
    },
};

/// Remove the user ID matching `target_uid` from the key block.
///
/// Works on both secret and public keys (tries secret first). Returns
/// `ErrorInvalidInput` when the UID is not found.
pub fn delete_user_id_internal(
    key_block: &str,
    target_uid: &str,
) -> Result<Zeroizing<String>, GfrStatus> {
    if let Ok((mut skey, _)) = SignedSecretKey::from_string(key_block) {
        let initial_len = skey.details.users.len();
        skey.details
            .users
            .retain(|u| String::from_utf8_lossy(u.id.id()) != target_uid);

        if skey.details.users.len() == initial_len {
            return Err(GfrStatus::ErrorInvalidInput);
        }
        return skey
            .to_armored_string(armor_opts())
            .into_gfr()
            .map(Zeroizing::new);
    }

    if let Ok((mut pkey, _)) = SignedPublicKey::from_string(key_block) {
        let initial_len = pkey.details.users.len();
        pkey.details
            .users
            .retain(|u| String::from_utf8_lossy(u.id.id()) != target_uid);

        if pkey.details.users.len() == initial_len {
            return Err(GfrStatus::ErrorInvalidInput);
        }
        return pkey
            .to_armored_string(armor_opts())
            .into_gfr()
            .map(Zeroizing::new);
    }

    Err(GfrStatus::ErrorInvalidData)
}

/// Add a new user ID to a secret key block and sign it with the primary key.
///
/// The primary key must be unlocked; its passphrase is fetched via the callback.
/// The new self-signature's version matches the primary key (v4 or v6), and its
/// key flags / algorithm preferences are copied from the most-recent existing
/// self-signature so the new UID advertises the same capabilities (RFC 9580
/// §5.2.3.10) instead of silently dropping them.
pub fn add_user_id_internal(
    channel: i32,
    secret_key_block: &str,
    new_uid_str: &str,
    fetch_cb: Option<GfrPasswordFetchCb>,
) -> Result<Zeroizing<String>, GfrStatus> {
    let (mut skey, _) = SignedSecretKey::from_string(secret_key_block).into_gfr()?;
    let fpr = skey.primary_key.fingerprint().to_string();

    let is_enc = matches!(skey.primary_key.secret_params(), SecretParams::Encrypted(_));
    let pwd_bytes = if is_enc {
        fetch_password_with_cache(
            Some(&PASSWORD_CACHE),
            PasswordCachePolicy::Default,
            channel,
            PassphraseStateInternal {
                fpr,
                info: "Add User ID".to_string(),
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

    let new_uid = pgp::packet::UserId::from_str(PacketHeaderVersion::New, new_uid_str)
        .map_err(|_| GfrStatus::ErrorInternal)?;

    let primary_fpr = skey.primary_key.fingerprint();
    let primary_fpr_bytes = primary_fpr.as_bytes().to_vec();
    let primary_algo = skey.primary_key.algorithm();
    let primary_version = skey.primary_key.version();

    // Use an existing self-signature as a template so the new UID inherits the
    // key's key-flags and algorithm-preference subpackets (rather than rPGP's
    // bare `UserId::sign`, which emits none). `build_updated_self_sig_config`
    // clones the template's subpackets and refreshes the issuer/creation-time.
    let template_sig: Option<Signature> = skey.details.users.iter().find_map(|u| {
        let self_sigs: Vec<&Signature> = u
            .signatures
            .iter()
            .filter(|s| is_self_signature_from_primary(s, &primary_fpr_bytes))
            .collect();
        choose_template_self_sig(&self_sigs).cloned()
    });

    let cfg = build_updated_self_sig_config(
        template_sig.as_ref(),
        primary_algo,
        primary_version,
        primary_fpr.clone(),
        false,
    )?;

    let pk = skey.primary_key.public_key();
    let new_sig = cfg
        .sign_certification(&skey.primary_key, &pk, &pwd, Tag::UserId, &new_uid)
        .into_gfr()?;

    skey.details.users.push(SignedUser::new(new_uid, vec![new_sig]));

    skey.to_armored_string(armor_opts())
        .into_gfr()
        .map(Zeroizing::new)
}

/// Replace one user ID string with another and re-sign it.
///
/// Creates a new `PositiveCertification` self-signature for `new_uid_str` and
/// drops the old user ID (with its old signatures). The primary key must be
/// unlocked.
pub fn update_user_id_internal(
    channel: i32,
    secret_key_block: &str,
    old_uid: &str,
    new_uid: &str,
    fetch_cb: Option<GfrPasswordFetchCb>,
) -> Result<Zeroizing<String>, GfrStatus> {
    let block_with_new = add_user_id_internal(channel, secret_key_block, new_uid, fetch_cb)?;
    delete_user_id_internal(block_with_new.as_str(), old_uid)
}

fn build_updated_self_sig_config(
    template_sig: Option<&Signature>,
    primary_key_algo: public_key::PublicKeyAlgorithm,
    primary_key_version: KeyVersion,
    primary_fpr: pgp::types::Fingerprint,
    make_primary: bool,
) -> Result<SignatureConfig, GfrStatus> {
    // Cloning an existing self-signature's config preserves its version (and key
    // flags / preference subpackets). When no template exists, build a config
    // whose version matches the primary key: a v6 key MUST produce v6 signatures
    // (RFC 9580 §10.3.2.2). `build_fallback` is only reached in that case.
    let build_fallback = || -> Result<SignatureConfig, GfrStatus> {
        match primary_key_version {
            KeyVersion::V6 => {
                let mut rng = rand::thread_rng();
                SignatureConfig::v6(
                    &mut rng,
                    SignatureType::CertPositive,
                    primary_key_algo,
                    HashAlgorithm::Sha512,
                )
                .map_err(|_| GfrStatus::ErrorInternal)
            }
            _ => Ok(SignatureConfig::v4(
                SignatureType::CertPositive,
                primary_key_algo,
                HashAlgorithm::Sha512,
            )),
        }
    };

    let mut cfg = match template_sig.and_then(|sig| sig.config().cloned()) {
        Some(cfg) => cfg,
        None => build_fallback()?,
    };

    let filter_subpackets = |subpackets: &mut Vec<Subpacket>| {
        subpackets.retain(|sp| {
            !matches!(
                sp.data,
                SubpacketData::SignatureCreationTime(_)
                    | SubpacketData::IssuerFingerprint(_)
                    | SubpacketData::IssuerKeyId(_)
                    | SubpacketData::IsPrimary(_)
            )
        });
    };

    filter_subpackets(&mut cfg.hashed_subpackets);
    filter_subpackets(&mut cfg.unhashed_subpackets);

    cfg.hashed_subpackets.push(
        Subpacket::regular(SubpacketData::IssuerFingerprint(primary_fpr))
            .map_err(|_| GfrStatus::ErrorInternal)?,
    );

    cfg.hashed_subpackets.push(
        Subpacket::regular(SubpacketData::SignatureCreationTime(Timestamp::now()))
            .map_err(|_| GfrStatus::ErrorInternal)?,
    );

    if make_primary {
        cfg.hashed_subpackets.push(
            Subpacket::regular(SubpacketData::IsPrimary(true))
                .map_err(|_| GfrStatus::ErrorInternal)?,
        );
    }

    Ok(cfg)
}

/// Mark a user ID as the primary one by adding an `IsPrimary(true)` self-signature.
///
/// OpenPGP clients treat the most-recent self-signature with `IsPrimary(true)`
/// as the primary UID; this function adds such a signature to `target_uid`
/// without touching the other UIDs.
pub fn set_primary_user_id_internal(
    channel: i32,
    secret_key_block: &str,
    target_uid_str: &str,
    fetch_cb: Option<GfrPasswordFetchCb>,
) -> Result<Zeroizing<String>, GfrStatus> {
    let (mut skey, _) = SignedSecretKey::from_string(secret_key_block).into_gfr()?;

    let target_idx = skey
        .details
        .users
        .iter()
        .position(|u| String::from_utf8_lossy(u.id.id()) == target_uid_str)
        .ok_or(GfrStatus::ErrorInvalidInput)?;

    let fpr = skey.primary_key.fingerprint().to_string();
    let is_enc = matches!(skey.primary_key.secret_params(), SecretParams::Encrypted(_));

    let pwd_bytes = if is_enc {
        fetch_password_with_cache(
            Some(&PASSWORD_CACHE),
            PasswordCachePolicy::Default,
            channel,
            PassphraseStateInternal {
                fpr,
                info: "Set Primary User ID".to_string(),
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

    let pk = skey.primary_key.public_key();
    let primary_fpr = skey.primary_key.fingerprint();
    let primary_fpr_bytes = primary_fpr.as_bytes().to_vec();
    let primary_algo = skey.primary_key.algorithm();
    let primary_version = skey.primary_key.version();

    let old_primary_idx = 0;

    let current_primary_template_sig: Option<Signature> =
        skey.details.users.get(old_primary_idx).and_then(|user| {
            let self_sigs: Vec<&Signature> = user
                .signatures
                .iter()
                .filter(|sig| is_self_signature_from_primary(sig, &primary_fpr_bytes))
                .collect();

            choose_template_self_sig(&self_sigs).cloned()
        });

    for (i, user) in skey.details.users.iter_mut().enumerate() {
        let is_target = i == target_idx;

        let self_sigs: Vec<&Signature> = user
            .signatures
            .iter()
            .filter(|sig| is_self_signature_from_primary(sig, &primary_fpr_bytes))
            .collect();

        let has_primary_true = self_sigs.iter().any(|sig| has_is_primary_true(sig));

        if !is_target && !has_primary_true {
            continue;
        }

        let own_template_sig = choose_template_self_sig(&self_sigs);

        let template_sig = if is_target {
            current_primary_template_sig.as_ref().or(own_template_sig)
        } else {
            own_template_sig.or(current_primary_template_sig.as_ref())
        };

        let cfg = build_updated_self_sig_config(
            template_sig,
            primary_algo,
            primary_version,
            primary_fpr.clone(),
            is_target,
        )?;

        let new_sig = cfg
            .sign_certification(&skey.primary_key, &pk, &pwd, Tag::UserId, &user.id)
            .into_gfr()?;

        user.signatures
            .retain(|sig| !is_self_signature_from_primary(sig, &primary_fpr_bytes));

        user.signatures.push(new_sig);
    }

    if target_idx != 0 {
        let primary_user = skey.details.users.remove(target_idx);
        skey.details.users.insert(0, primary_user);
    }

    skey.to_armored_string(armor_opts())
        .into_gfr()
        .map(Zeroizing::new)
}

/// Revoke a user ID by appending a `CertRevocation` self-signature.
///
/// Does not delete the UID; it remains in the key block but is marked
/// revoked so conforming implementations hide it from normal display.
pub fn revoke_user_id_internal(
    channel: i32,
    secret_key_block: &str,
    target_uid_str: &str,
    reason_code: GfrRevocationCode,
    reason_text: Option<&str>,
    fetch_cb: Option<GfrPasswordFetchCb>,
) -> Result<Zeroizing<String>, GfrStatus> {
    let (mut skey, _) = SignedSecretKey::from_string(secret_key_block).into_gfr()?;

    let target_idx = skey
        .details
        .users
        .iter()
        .position(|u| String::from_utf8_lossy(u.id.id()) == target_uid_str)
        .ok_or(GfrStatus::ErrorInvalidInput)?;

    let fpr = skey.primary_key.fingerprint().to_string();
    let is_enc = matches!(skey.primary_key.secret_params(), SecretParams::Encrypted(_));

    let pwd_bytes = if is_enc {
        fetch_password_with_cache(
            Some(&PASSWORD_CACHE),
            PasswordCachePolicy::Default,
            channel,
            PassphraseStateInternal {
                fpr,
                info: "Revoke User ID".to_string(),
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

    let pk = skey.primary_key.public_key();
    let primary_fpr = skey.primary_key.fingerprint();

    // Version-aware: a v6 key MUST emit a v6 revocation signature, else conforming
    // verifiers ignore it and the revocation silently fails (RFC 9580 §5.2.5).
    let mut rng = rand::thread_rng();
    let mut cfg = SignatureConfig::from_key(
        &mut rng,
        &skey.primary_key,
        SignatureType::CertRevocation,
    )
    .map_err(|_| GfrStatus::ErrorInternal)?;

    let user = skey
        .details
        .users
        .get_mut(target_idx)
        .ok_or(GfrStatus::ErrorInternal)?;

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
        .sign_certification(&skey.primary_key, &pk, &pwd, Tag::UserId, &user.id)
        .into_gfr()?;

    user.signatures.push(revoke_sig);

    skey.to_armored_string(armor_opts())
        .into_gfr()
        .map(Zeroizing::new)
}
