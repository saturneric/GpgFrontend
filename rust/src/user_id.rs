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

use pgp::{
    bytes::Bytes,
    composed::{ArmorOptions, Deserializable, SignedPublicKey, SignedSecretKey},
    crypto::{hash::HashAlgorithm, public_key},
    packet::{RevocationCode, Signature, SignatureConfig, SignatureType, Subpacket, SubpacketData},
    types::{KeyDetails, PacketHeaderVersion, Password, SecretParams, Tag, Timestamp},
};

use crate::{
    err::IntoGfrResult,
    types::{GfrFreeCb, GfrPasswordFetchCb, GfrRevocationCode, GfrStatus},
    utils::fetch_password_internal,
};

pub fn delete_user_id_internal(key_block: &str, target_uid: &str) -> Result<String, GfrStatus> {
    if let Ok((mut skey, _)) = SignedSecretKey::from_string(key_block) {
        let initial_len = skey.details.users.len();
        skey.details
            .users
            .retain(|u| String::from_utf8_lossy(u.id.id()) != target_uid);

        if skey.details.users.len() == initial_len {
            return Err(GfrStatus::ErrorInvalidInput);
        }
        return skey.to_armored_string(ArmorOptions::default()).into_gfr();
    }

    if let Ok((mut pkey, _)) = SignedPublicKey::from_string(key_block) {
        let initial_len = pkey.details.users.len();
        pkey.details
            .users
            .retain(|u| String::from_utf8_lossy(u.id.id()) != target_uid);

        if pkey.details.users.len() == initial_len {
            return Err(GfrStatus::ErrorInvalidInput);
        }
        return pkey.to_armored_string(ArmorOptions::default()).into_gfr();
    }

    Err(GfrStatus::ErrorInvalidData)
}

pub fn add_user_id_internal(
    channel: i32,
    secret_key_block: &str,
    new_uid_str: &str,
    fetch_cb: Option<GfrPasswordFetchCb>,
    free_cb: Option<GfrFreeCb>,
) -> Result<String, GfrStatus> {
    let (mut skey, _) = SignedSecretKey::from_string(secret_key_block).into_gfr()?;
    let fpr = skey.primary_key.fingerprint().to_string();

    let is_enc = matches!(skey.primary_key.secret_params(), SecretParams::Encrypted(_));
    let pwd_bytes = if is_enc {
        fetch_password_internal(channel, &fpr, "Add User ID", fetch_cb, free_cb)?
    } else {
        Vec::new()
    };
    let pwd = Password::from(pwd_bytes.as_slice());

    let new_uid = pgp::packet::UserId::from_str(PacketHeaderVersion::New, new_uid_str)
        .map_err(|_| GfrStatus::ErrorInternal)?;

    let mut rng = rand::thread_rng();

    let pk = skey.primary_key.public_key();
    let signed_user = new_uid
        .sign(&mut rng, &skey.primary_key, &pk, &pwd)
        .into_gfr()?;

    skey.details.users.push(signed_user);

    skey.to_armored_string(ArmorOptions::default()).into_gfr()
}

pub fn update_user_id_internal(
    channel: i32,
    secret_key_block: &str,
    old_uid: &str,
    new_uid: &str,
    fetch_cb: Option<GfrPasswordFetchCb>,
    free_cb: Option<GfrFreeCb>,
) -> Result<String, GfrStatus> {
    let block_with_new =
        add_user_id_internal(channel, secret_key_block, new_uid, fetch_cb, free_cb)?;
    delete_user_id_internal(&block_with_new, old_uid)
}

fn is_self_signature_from_primary(sig: &Signature, primary_fpr_bytes: &[u8]) -> bool {
    sig.issuer_fingerprint()
        .iter()
        .any(|f| f.as_bytes() == primary_fpr_bytes)
}

fn sig_creation_time(sig: &Signature) -> Option<Timestamp> {
    sig.config().and_then(|c| {
        c.hashed_subpackets
            .iter()
            .chain(c.unhashed_subpackets.iter())
            .find_map(|sp| match &sp.data {
                SubpacketData::SignatureCreationTime(ts) => Some(*ts),
                _ => None,
            })
    })
}

fn has_key_flags(sig: &Signature) -> bool {
    sig.config()
        .map(|c| {
            c.hashed_subpackets
                .iter()
                .chain(c.unhashed_subpackets.iter())
                .any(|sp| matches!(sp.data, SubpacketData::KeyFlags(_)))
        })
        .unwrap_or(false)
}

fn has_is_primary_true(sig: &Signature) -> bool {
    sig.config()
        .map(|c| {
            c.hashed_subpackets
                .iter()
                .chain(c.unhashed_subpackets.iter())
                .any(|sp| matches!(sp.data, SubpacketData::IsPrimary(true)))
        })
        .unwrap_or(false)
}

fn sig_creation_time_value(sig: &Signature) -> u64 {
    sig_creation_time(sig)
        .map(|ts| ts.as_secs() as u64)
        .unwrap_or(0)
}

fn choose_template_self_sig<'a>(self_sigs: &[&'a Signature]) -> Option<&'a Signature> {
    self_sigs
        .iter()
        .copied()
        .filter(|sig| has_key_flags(sig))
        .max_by(|a, b| sig_creation_time_value(a).cmp(&sig_creation_time_value(b)))
        .or_else(|| {
            self_sigs
                .iter()
                .copied()
                .max_by(|a, b| sig_creation_time_value(a).cmp(&sig_creation_time_value(b)))
        })
}
fn build_updated_self_sig_config(
    template_sig: Option<&Signature>,
    primary_key_algo: public_key::PublicKeyAlgorithm,
    primary_fpr: pgp::types::Fingerprint,
    make_primary: bool,
) -> Result<SignatureConfig, GfrStatus> {
    let mut cfg = if let Some(sig) = template_sig {
        sig.config().cloned().unwrap_or_else(|| {
            SignatureConfig::v4(
                SignatureType::CertPositive,
                primary_key_algo,
                HashAlgorithm::Sha512,
            )
        })
    } else {
        SignatureConfig::v4(
            SignatureType::CertPositive,
            primary_key_algo,
            HashAlgorithm::Sha512,
        )
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

pub fn set_primary_user_id_internal(
    channel: i32,
    secret_key_block: &str,
    target_uid_str: &str,
    fetch_cb: Option<GfrPasswordFetchCb>,
    free_cb: Option<GfrFreeCb>,
) -> Result<String, GfrStatus> {
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
        fetch_password_internal(channel, &fpr, "Set Primary User ID", fetch_cb, free_cb)?
    } else {
        Vec::new()
    };
    let pwd = Password::from(pwd_bytes.as_slice());

    let pk = skey.primary_key.public_key();
    let primary_fpr = skey.primary_key.fingerprint();
    let primary_fpr_bytes = primary_fpr.as_bytes().to_vec();
    let primary_algo = skey.primary_key.algorithm();

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

    skey.to_armored_string(ArmorOptions::default()).into_gfr()
}

fn build_revocation_reason_subpacket(
    code: GfrRevocationCode,
    text: Option<&str>,
) -> Result<Subpacket, GfrStatus> {
    let reason_text = text.unwrap_or("").to_string();

    let sp = match code {
        GfrRevocationCode::NoReason => Subpacket::regular(SubpacketData::RevocationReason(
            RevocationCode::NoReason,
            Bytes::from(reason_text),
        )),
        GfrRevocationCode::Superseded => Subpacket::regular(SubpacketData::RevocationReason(
            RevocationCode::KeySuperseded,
            Bytes::from(reason_text),
        )),
        GfrRevocationCode::Compromised => Subpacket::regular(SubpacketData::RevocationReason(
            RevocationCode::KeyCompromised,
            Bytes::from(reason_text),
        )),
        GfrRevocationCode::Retired => Subpacket::regular(SubpacketData::RevocationReason(
            RevocationCode::KeyRetired,
            Bytes::from(reason_text),
        )),
        GfrRevocationCode::UserIdInvalid => Subpacket::regular(SubpacketData::RevocationReason(
            RevocationCode::CertUserIdInvalid,
            Bytes::from(reason_text),
        )),
    };

    sp.map_err(|_| GfrStatus::ErrorInternal)
}

pub fn revoke_user_id_internal(
    channel: i32,
    secret_key_block: &str,
    target_uid_str: &str,
    reason_code: GfrRevocationCode,
    reason_text: Option<&str>,
    fetch_cb: Option<GfrPasswordFetchCb>,
    free_cb: Option<GfrFreeCb>,
) -> Result<String, GfrStatus> {
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
        fetch_password_internal(channel, &fpr, "Revoke User ID", fetch_cb, free_cb)?
    } else {
        Vec::new()
    };
    let pwd = Password::from(pwd_bytes.as_slice());

    let pk = skey.primary_key.public_key();
    let primary_fpr = skey.primary_key.fingerprint();

    let user = skey
        .details
        .users
        .get_mut(target_idx)
        .ok_or(GfrStatus::ErrorInternal)?;

    let mut cfg = SignatureConfig::v4(
        SignatureType::CertRevocation,
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

    let revoke_sig = cfg
        .sign_certification(&skey.primary_key, &pk, &pwd, Tag::UserId, &user.id)
        .into_gfr()?;

    user.signatures.push(revoke_sig);

    skey.to_armored_string(ArmorOptions::default()).into_gfr()
}
