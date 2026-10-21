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
    crypto::hash::HashAlgorithm,
    packet::{Signature, SignatureConfig, SignatureType, Subpacket, SubpacketData},
    types::{
        KeyDetails, KeyVersion, PacketHeaderVersion, SecretParams, SignedUser, Tag, Timestamp,
    },
};
use zeroize::Zeroizing;

use crate::{
    cache::{PASSWORD_CACHE, PasswordCachePolicy},
    err::IntoGfrResult,
    key::{is_user_id_revoked, names_primary_as_issuer, uid_reissue_template},
    types::{GfrPasswordFetchCb, GfrRevocationCode, GfrStatus},
    utils::{
        PassphraseStateInternal, build_revocation_reason_subpacket, fetch_password_with_cache,
        is_property_self_sig, key_armor_opts, password_from_zeroizing_bytes, push_issuer,
        refresh_reissued_config,
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
            .to_armored_string(key_armor_opts(skey.primary_key.version()))
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
            .to_armored_string(key_armor_opts(pkey.primary_key.version()))
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

    let pk = skey.primary_key.public_key();

    // Use the certification that speaks for the key now as the template, so
    // the new UID inherits the key's key-flags and algorithm-preference
    // subpackets (rather than rPGP's bare `UserId::sign`, which emits none).
    // `build_updated_self_sig_config` clones the template's subpackets and
    // refreshes the issuer/creation-time.
    let template_sig: Option<Signature> =
        crate::key::uid_certification_in_force(&pk, &skey.details, None).cloned();

    let cfg = build_updated_self_sig_config(
        template_sig.as_ref(),
        &skey.primary_key,
        pgp::types::SigningKey::hash_alg(&skey.primary_key),
        false,
    )?;

    let new_sig = cfg
        .sign_certification(&skey.primary_key, &pk, &pwd, Tag::UserId, &new_uid)
        .into_gfr()?;

    skey.details
        .users
        .push(SignedUser::new(new_uid, vec![new_sig]));

    skey.to_armored_string(key_armor_opts(skey.primary_key.version()))
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
    // Adding then deleting by text would delete the only copy.
    if old_uid == new_uid {
        let (skey, _) = SignedSecretKey::from_string(secret_key_block).into_gfr()?;
        if !skey
            .details
            .users
            .iter()
            .any(|u| u.id.id() == old_uid.as_bytes())
        {
            return Err(GfrStatus::ErrorInvalidInput);
        }
        return Ok(Zeroizing::new(secret_key_block.to_string()));
    }
    let block_with_new = add_user_id_internal(channel, secret_key_block, new_uid, fetch_cb)?;
    delete_user_id_internal(block_with_new.as_str(), old_uid)
}

fn build_updated_self_sig_config(
    template_sig: Option<&Signature>,
    primary: &impl pgp::types::KeyDetails,
    primary_key_hash: HashAlgorithm,
    make_primary: bool,
) -> Result<SignatureConfig, GfrStatus> {
    let primary_key_algo = primary.algorithm();
    let primary_key_version = primary.version();
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
        Some(mut cfg) => {
            refresh_reissued_config(&mut cfg, primary_key_hash)?;
            cfg
        }
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

    push_issuer(&mut cfg, primary)?;

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
    let primary_hash = pgp::types::SigningKey::hash_alg(&skey.primary_key);

    // Templates come from verified self-certifications only (see
    // `uid_reissue_template`). The target keeps its own properties; it falls
    // back to the certification in force when it has none of its own.
    let in_force = crate::key::uid_certification_in_force(&pk, &skey.details, None).cloned();
    let plan: Vec<(bool, Option<Signature>, bool)> = skey
        .details
        .users
        .iter()
        .map(|user| {
            let template = uid_reissue_template(&pk, user).cloned();
            let marked_primary = template.as_ref().is_some_and(|sig| sig.is_primary());
            (is_user_id_revoked(&pk, user), template, marked_primary)
        })
        .collect();

    // A revoked User ID cannot become primary: certifying it anew would
    // un-revoke it for GnuPG and Sequoia.
    if plan[target_idx].0 {
        return Err(GfrStatus::ErrorInvalidInput);
    }

    for (i, user) in skey.details.users.iter_mut().enumerate() {
        let is_target = i == target_idx;
        let (revoked, own_template, marked_primary) = &plan[i];

        if *revoked || (!is_target && !*marked_primary) {
            continue;
        }

        let template_sig = if is_target {
            own_template.as_ref().or(in_force.as_ref())
        } else {
            own_template.as_ref()
        };

        let cfg = build_updated_self_sig_config(
            template_sig,
            &skey.primary_key,
            primary_hash,
            is_target,
        )?;

        let new_sig = cfg
            .sign_certification(&skey.primary_key, &pk, &pwd, Tag::UserId, &user.id)
            .into_gfr()?;

        // Replace the certifications only: a 0x30 revocation must survive.
        user.signatures
            .retain(|sig| !(names_primary_as_issuer(sig, &pk) && is_property_self_sig(sig)));

        user.signatures.push(new_sig);
    }

    if target_idx != 0 {
        let primary_user = skey.details.users.remove(target_idx);
        skey.details.users.insert(0, primary_user);
    }

    skey.to_armored_string(key_armor_opts(skey.primary_key.version()))
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

    // Version-aware: a v6 key MUST emit a v6 revocation signature, else conforming
    // verifiers ignore it and the revocation silently fails (RFC 9580 §5.2.5).
    let mut rng = rand::thread_rng();
    let mut cfg =
        SignatureConfig::from_key(&mut rng, &skey.primary_key, SignatureType::CertRevocation)
            .map_err(|_| GfrStatus::ErrorInternal)?;

    let user = skey
        .details
        .users
        .get_mut(target_idx)
        .ok_or(GfrStatus::ErrorInternal)?;

    push_issuer(&mut cfg, &skey.primary_key)?;

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

    skey.to_armored_string(key_armor_opts(skey.primary_key.version()))
        .into_gfr()
        .map(Zeroizing::new)
}

#[cfg(test)]
mod user_id_tests {
    //! User ID lifecycle: add, delete, update, set-primary and revoke.
    //!
    //! The load-bearing RFC 9580 requirements here are §5.2.3.10 (a new
    //! self-signature must carry the key's flags and preferences rather than
    //! silently dropping them), §10.3.2.2 (the self-signature version must
    //! match the key version) and §5.2.3.27 (the Primary User ID flag).

    use super::*;
    use crate::testutil::keys;

    /// Parse an armored secret key block and list its user IDs.
    fn uids_of(block: &str) -> Vec<String> {
        let (key, _) = SignedSecretKey::from_string(block).expect("parses");
        key.details
            .users
            .iter()
            .map(|u| String::from_utf8_lossy(u.id.id()).into_owned())
            .collect()
    }

    fn parse(block: &str) -> SignedSecretKey {
        SignedSecretKey::from_string(block).expect("parses").0
    }

    /// A fresh unprotected key with a single user ID, safe to mutate.
    fn base_key() -> String {
        keys::V4_SIGN.secret_armored.clone()
    }

    // -- re-issue invariants (round two) --------------------------------------
    //
    // A re-issued self-signature is signed for real by the keyholder, so its
    // subpackets must come only from a self-signature the keyholder verifiably
    // made, and it must change only what the user asked to change.

    const PLANTED: &str = "planted@attacker.example";

    fn has_notation(sig: &Signature, name: &str) -> bool {
        sig.notations()
            .iter()
            .any(|n| n.name.as_ref() == name.as_bytes())
    }

    fn now_secs() -> u32 {
        Timestamp::now().as_secs()
    }

    /// `block` with a forged "self-signature" on its first User ID: it names
    /// the primary as issuer, is dated in the future (so it looks newest) and
    /// carries a notation the keyholder never signed. Its signature bytes are
    /// borrowed from a genuine self-signature, so it does not verify.
    fn with_forged_template(block: &str) -> String {
        let mut sk = parse(block);
        let genuine = sk.details.users[0].signatures[0].clone();
        let mut cfg = genuine.config().unwrap().clone();
        cfg.hashed_subpackets
            .retain(|sp| !matches!(sp.data, SubpacketData::SignatureCreationTime(_)));
        cfg.hashed_subpackets.push(
            Subpacket::regular(SubpacketData::SignatureCreationTime(Timestamp::from_secs(
                now_secs() + 100_000,
            )))
            .unwrap(),
        );
        cfg.hashed_subpackets.push(
            Subpacket::regular(SubpacketData::Notation(pgp::packet::Notation {
                readable: true,
                name: PLANTED.into(),
                value: "1".into(),
            }))
            .unwrap(),
        );
        let forged =
            Signature::from_config(cfg, [0, 0], genuine.signature().unwrap().clone()).unwrap();
        sk.details.users[0].signatures.push(forged);
        sk.to_armored_string(crate::utils::armor_opts()).unwrap()
    }

    /// True if any User ID of `block` carries a *valid* certification that
    /// contains the planted notation, i.e. the keyholder was made to sign it.
    fn keyholder_signed_planted(block: &str) -> bool {
        let sk = parse(block);
        let pk = sk.primary_key.public_key();
        sk.details.users.iter().any(|u| {
            u.signatures.iter().any(|s| {
                has_notation(s, PLANTED) && s.verify_certification(&pk, Tag::UserId, &u.id).is_ok()
            })
        })
    }

    #[test]
    fn re_issuing_never_signs_subpackets_from_an_unverified_signature() {
        let block = with_forged_template(&base_key());
        let uid = base_uid();
        let expiry = (now_secs() + 365 * 86_400) as u64;
        let results = [
            (
                "expiry change",
                crate::key::update_key_expiration_internal(0, &block, None, expiry, None)
                    .map(|k| k.secret),
            ),
            (
                "add user id",
                add_user_id_internal(0, &block, "New <new@test.example>", None),
            ),
            (
                "set primary",
                set_primary_user_id_internal(0, &block, &uid, None),
            ),
        ];
        for (op, out) in results {
            let out = out.unwrap_or_else(|e| panic!("{op}: {e:?}"));
            assert!(
                !keyholder_signed_planted(&out),
                "{op} signed a forged template"
            );
        }
    }

    /// A self-certification of `sk`'s User ID `idx` by its primary key,
    /// carrying Key Flags and `extra` hashed subpackets. With `key_id_only` it
    /// names its issuer only by Key ID (unhashed), as GnuPG 1.x and old PGP
    /// versions did.
    fn self_cert(
        sk: &SignedSecretKey,
        idx: usize,
        key_id_only: bool,
        extra: Vec<Subpacket>,
    ) -> Signature {
        use pgp::types::Password;
        let primary = &sk.primary_key;
        let mut cfg =
            SignatureConfig::from_key(rand::thread_rng(), primary, SignatureType::CertPositive)
                .unwrap();
        let mut flags = pgp::packet::KeyFlags::default();
        flags.set_sign(true);
        flags.set_certify(true);
        cfg.hashed_subpackets = vec![
            Subpacket::regular(SubpacketData::SignatureCreationTime(Timestamp::now())).unwrap(),
            Subpacket::regular(SubpacketData::KeyFlags(flags)).unwrap(),
        ];
        cfg.hashed_subpackets.extend(extra);
        if key_id_only {
            cfg.unhashed_subpackets = vec![
                Subpacket::regular(SubpacketData::IssuerKeyId(primary.legacy_key_id())).unwrap(),
            ];
        } else {
            cfg.hashed_subpackets.push(
                Subpacket::regular(SubpacketData::IssuerFingerprint(primary.fingerprint()))
                    .unwrap(),
            );
        }
        cfg.sign_certification(
            primary,
            primary.public_key(),
            &Password::empty(),
            Tag::UserId,
            &sk.details.users[idx].id,
        )
        .unwrap()
    }

    #[test]
    fn a_legacy_key_id_only_self_signature_can_be_re_issued() {
        // RFC 9580 §5.2.3.12/§5.2.3.35: a v4 signature may name its issuer by
        // Key ID alone. Such a key must still accept an expiry change and keep
        // its Key Flags.
        let mut sk = parse(&base_key());
        let legacy = self_cert(&sk, 0, true, vec![]);
        sk.details.users[0].signatures = vec![legacy];
        let block = sk.to_armored_string(crate::utils::armor_opts()).unwrap();
        let expiry = (now_secs() + 365 * 86_400) as u64;
        let out = crate::key::update_key_expiration_internal(0, &block, None, expiry, None)
            .expect("expiry change on a key-ID-only self-signature");
        let cert = SignedPublicKey::from_string(&out.public).unwrap().0;
        let sig = crate::key::preference_self_signature(&cert).expect("a self-signature in force");
        assert!(sig.key_flags().sign(), "Key Flags survive the re-issue");
        assert_eq!(
            crate::key::primary_key_expires_at(&cert) as u64,
            expiry,
            "the new expiry applies"
        );
    }

    /// The creation time of `user`'s certification revocation, if any.
    fn revoked_at(user: &SignedUser) -> Option<u32> {
        user.signatures
            .iter()
            .filter(|s| matches!(s.typ(), Some(SignatureType::CertRevocation)))
            .filter_map(|s| s.created().map(|t| t.as_secs()))
            .max()
    }

    #[test]
    fn a_revoked_user_id_is_never_re_certified() {
        // A certification newer than a User ID's revocation un-revokes it in
        // GnuPG ("we allow invalidation of cert revocations by a newer
        // signature") and, for soft reasons, in Sequoia. Re-issuing must
        // leave revoked User IDs alone.
        let second = "Second <second@test.example>";
        let block = add_user_id_internal(0, &base_key(), second, None).unwrap();
        let block = revoke_user_id_internal(
            0,
            &block,
            second,
            GfrRevocationCode::UserIdInvalid,
            None,
            None,
        )
        .unwrap();
        std::thread::sleep(std::time::Duration::from_millis(1100));
        let expiry = (now_secs() + 365 * 86_400) as u64;
        let out = crate::key::update_key_expiration_internal(0, &block, None, expiry, None)
            .unwrap()
            .secret;
        let sk = parse(&out);
        let user = sk
            .details
            .users
            .iter()
            .find(|u| u.id.id() == second.as_bytes())
            .unwrap();
        let revoked = revoked_at(user).expect("still carries its revocation");
        assert!(
            user.signatures
                .iter()
                .filter(|s| is_property_self_sig(s))
                .all(|s| s.created().map(|t| t.as_secs()).unwrap_or(0) <= revoked),
            "no certification newer than the revocation"
        );
        assert_eq!(
            set_primary_user_id_internal(0, &block, second, None).err(),
            Some(GfrStatus::ErrorInvalidInput),
            "a revoked User ID cannot become primary"
        );
    }

    #[test]
    fn setting_the_primary_user_id_keeps_the_targets_own_properties() {
        // Only the Primary User ID flag changes. The target's own
        // self-signature (here: its notation) is the template, not whatever
        // User ID happens to be stored first.
        let second = "Second <second@test.example>";
        let mut sk = parse(&add_user_id_internal(0, &base_key(), second, None).unwrap());
        let own = Subpacket::regular(SubpacketData::Notation(pgp::packet::Notation {
            readable: true,
            name: "own@second.example".into(),
            value: "1".into(),
        }))
        .unwrap();
        let cert = self_cert(&sk, 1, false, vec![own]);
        sk.details.users[1].signatures = vec![cert];
        let block = sk.to_armored_string(crate::utils::armor_opts()).unwrap();
        let out = parse(&set_primary_user_id_internal(0, &block, second, None).unwrap());
        let target = out
            .details
            .users
            .iter()
            .find(|u| u.id.id() == second.as_bytes())
            .unwrap();
        let newest = target
            .signatures
            .iter()
            .filter(|s| is_property_self_sig(s))
            .max_by_key(|s| s.created().map(|t| t.as_secs()))
            .unwrap();
        assert!(newest.is_primary());
        assert!(has_notation(newest, "own@second.example"));
    }

    #[test]
    fn renaming_a_user_id_to_itself_keeps_it() {
        let uid = base_uid();
        let out = update_user_id_internal(0, &base_key(), &uid, &uid, None).expect("no-op rename");
        assert_eq!(uids_of(&out), vec![uid]);
    }

    #[test]
    fn setting_the_primary_user_id_keeps_another_user_ids_revocation() {
        // RFC 9580 §5.2.1.13: the 0x30 revocation retires the User ID.
        // Re-signing the former primary (which still carries IsPrimary) must
        // not drop it.
        let first = uids_of(&base_key()).remove(0);
        let second = "Second <second@test.example>";
        let block = add_user_id_internal(0, &base_key(), second, None).unwrap();
        let block = set_primary_user_id_internal(0, &block, second, None).unwrap();
        let block = revoke_user_id_internal(
            0,
            &block,
            second,
            GfrRevocationCode::UserIdInvalid,
            None,
            None,
        )
        .unwrap();
        let block = set_primary_user_id_internal(0, &block, &first, None).unwrap();

        let sk = parse(&block);
        let user = sk
            .details
            .users
            .iter()
            .find(|u| u.id.id() == second.as_bytes())
            .expect("the second User ID survives");
        assert!(user.signatures.iter().any(|sig| {
            matches!(sig.typ(), Some(SignatureType::CertRevocation))
                && sig
                    .verify_certification(sk.primary_key.public_key(), Tag::UserId, &user.id)
                    .is_ok()
        }));
    }

    fn base_uid() -> String {
        uids_of(&base_key()).remove(0)
    }

    // -- delete_user_id_internal -------------------------------------------

    #[test]
    fn deleting_a_user_id_removes_it() {
        let block = base_key();
        let uid = base_uid();
        let out = delete_user_id_internal(&block, &uid).expect("delete");
        assert!(!uids_of(&out).contains(&uid));
    }

    #[test]
    fn deleting_an_unknown_user_id_is_invalid_input() {
        let block = base_key();
        assert_eq!(
            delete_user_id_internal(&block, "nobody <nobody@example.test>").err(),
            Some(GfrStatus::ErrorInvalidInput)
        );
    }

    #[test]
    fn deleting_matches_the_whole_user_id_not_a_substring() {
        // A partial match would delete the wrong identity.
        let block = base_key();
        let uid = base_uid();
        let prefix = &uid[..uid.len() / 2];
        assert_eq!(
            delete_user_id_internal(&block, prefix).err(),
            Some(GfrStatus::ErrorInvalidInput)
        );
    }

    #[test]
    fn deleting_works_on_a_public_key_block() {
        // No signing is involved, so a public block is enough.
        let block = keys::V4_SIGN.public_armored.clone();
        let uid = base_uid();
        let out = delete_user_id_internal(&block, &uid).expect("delete");
        assert!(out.contains("BEGIN PGP PUBLIC KEY BLOCK"));
    }

    #[test]
    fn deleting_from_garbage_is_a_data_error() {
        assert_eq!(
            delete_user_id_internal("not a key block", "x").err(),
            Some(GfrStatus::ErrorInvalidData)
        );
    }

    #[test]
    fn deleting_from_an_empty_string_is_a_data_error() {
        assert_eq!(
            delete_user_id_internal("", "x").err(),
            Some(GfrStatus::ErrorInvalidData)
        );
    }

    #[test]
    fn deleting_the_last_user_id_still_produces_a_parseable_key() {
        // §10.1.5 only *recommends* at least one user ID; a key without one
        // must still serialise and re-parse.
        let block = base_key();
        let uid = base_uid();
        let out = delete_user_id_internal(&block, &uid).expect("delete");
        let key = parse(&out);
        assert!(key.details.users.is_empty());
    }

    #[test]
    fn a_delete_preserves_the_primary_key_and_subkeys() {
        let block = base_key();
        let uid = base_uid();
        let out = delete_user_id_internal(&block, &uid).expect("delete");
        let key = parse(&out);
        assert_eq!(
            key.primary_key.fingerprint().to_string().to_uppercase(),
            keys::V4_SIGN.primary_fpr
        );
        assert_eq!(key.secret_subkeys.len(), 2);
    }

    #[test]
    fn a_delete_on_a_v4_key_emits_armor_with_a_crc24_footer() {
        // RFC 9580 §6.1 interoperability exception: v4-only armor carries the
        // CRC24 footer (GnuPG 2.4); v6 armor never does (utils::crc24_policy_tests).
        let block = base_key();
        let uid = base_uid();
        let out = delete_user_id_internal(&block, &uid).expect("delete");
        crate::testutil::assert::armor_has_crc24(&out);
    }

    // -- add_user_id_internal -----------------------------------------------

    #[test]
    fn adding_a_user_id_appends_it() {
        let block = base_key();
        let out =
            add_user_id_internal(0, &block, "Second <second@example.test>", None).expect("add");
        let uids = uids_of(&out);
        assert!(uids.iter().any(|u| u == "Second <second@example.test>"));
        assert_eq!(uids.len(), 2);
    }

    #[test]
    fn adding_preserves_the_existing_user_id() {
        let block = base_key();
        let original = base_uid();
        let out =
            add_user_id_internal(0, &block, "Second <second@example.test>", None).expect("add");
        assert!(uids_of(&out).contains(&original));
    }

    #[test]
    fn a_new_self_signature_carries_the_key_flags() {
        // §5.2.3.10: subpackets on a certification self-signature describe the
        // key. Dropping the flags would make the new UID advertise nothing.
        let block = base_key();
        let out = add_user_id_internal(0, &block, "Flags <flags@example.test>", None).expect("add");
        let key = parse(&out);
        let added = key
            .details
            .users
            .iter()
            .find(|u| String::from_utf8_lossy(u.id.id()) == "Flags <flags@example.test>")
            .expect("the new uid");
        let sig = added.signatures.first().expect("a self-signature");
        assert!(
            sig.key_flags().certify() || sig.key_flags().sign(),
            "the new self-signature must inherit the key flags"
        );
    }

    #[test]
    fn a_new_self_signature_is_issued_by_the_primary_key() {
        let block = base_key();
        let out =
            add_user_id_internal(0, &block, "Issuer <issuer@example.test>", None).expect("add");
        let key = parse(&out);
        let added = key
            .details
            .users
            .iter()
            .find(|u| String::from_utf8_lossy(u.id.id()) == "Issuer <issuer@example.test>")
            .expect("the new uid");
        let sig = added.signatures.first().expect("a self-signature");
        assert!(names_primary_as_issuer(sig, &key.primary_key.public_key()));
    }

    #[test]
    fn a_new_self_signature_actually_verifies() {
        // The strongest check: the signature is cryptographically sound under
        // the primary key, not merely present.
        let block = base_key();
        let out =
            add_user_id_internal(0, &block, "Verify <verify@example.test>", None).expect("add");
        let key = parse(&out);
        let primary = key.primary_key.public_key();
        let added = key
            .details
            .users
            .iter()
            .find(|u| String::from_utf8_lossy(u.id.id()) == "Verify <verify@example.test>")
            .expect("the new uid");
        let sig = added.signatures.first().expect("a self-signature");
        assert!(
            sig.verify_certification(primary, Tag::UserId, &added.id)
                .is_ok(),
            "the generated self-signature must verify under the primary key"
        );
    }

    #[test]
    fn a_v4_key_gets_a_v4_self_signature() {
        // §10.3.2.2 Table 27: a v4 key produces v4 signatures.
        let block = base_key();
        let out = add_user_id_internal(0, &block, "V4 <v4@example.test>", None).expect("add");
        let key = parse(&out);
        let added = key
            .details
            .users
            .iter()
            .find(|u| String::from_utf8_lossy(u.id.id()) == "V4 <v4@example.test>")
            .expect("the new uid");
        assert_eq!(
            added.signatures[0].version(),
            pgp::packet::SignatureVersion::V4
        );
    }

    #[test]
    fn a_v6_key_gets_a_v6_self_signature() {
        // §10.3.2.2: "When a version 6 key produces a Signature packet, it
        // MUST produce a version 6 Signature packet."
        let block = keys::V6_SIGN.secret_armored.clone();
        let out = add_user_id_internal(0, &block, "V6 <v6@example.test>", None).expect("add");
        let key = parse(&out);
        let added = key
            .details
            .users
            .iter()
            .find(|u| String::from_utf8_lossy(u.id.id()) == "V6 <v6@example.test>")
            .expect("the new uid");
        assert_eq!(
            added.signatures[0].version(),
            pgp::packet::SignatureVersion::V6,
            "a v6 key must not emit a v4 self-signature"
        );
    }

    #[test]
    fn a_unicode_user_id_round_trips() {
        // §3.4 / §5.11: user IDs are UTF-8 text.
        let block = base_key();
        let uid = "Renée Übel 日本 <renee@example.test>";
        let out = add_user_id_internal(0, &block, uid, None).expect("add");
        assert!(uids_of(&out).iter().any(|u| u == uid));
    }

    #[test]
    fn an_empty_user_id_is_accepted_as_written() {
        // §5.11 places "no restrictions on its content", so an empty user ID
        // is unusual but not malformed.
        let block = base_key();
        let out = add_user_id_internal(0, &block, "", None).expect("add");
        assert!(uids_of(&out).iter().any(|u| u.is_empty()));
    }

    #[test]
    fn adding_to_a_public_only_block_fails() {
        // Signing the new self-signature needs the secret primary key.
        let block = keys::V4_SIGN.public_armored.clone();
        assert!(add_user_id_internal(0, &block, "X <x@example.test>", None).is_err());
    }

    #[test]
    fn adding_to_garbage_fails() {
        assert!(add_user_id_internal(0, "not a key", "X <x@example.test>", None).is_err());
    }

    #[test]
    fn adding_twice_yields_three_user_ids() {
        let block = base_key();
        let once = add_user_id_internal(0, &block, "A <a@example.test>", None).expect("add a");
        let twice = add_user_id_internal(0, &once, "B <b@example.test>", None).expect("add b");
        assert_eq!(uids_of(&twice).len(), 3);
    }

    #[test]
    fn adding_to_a_v4_key_emits_armor_with_a_crc24_footer() {
        // RFC 9580 §6.1 interoperability exception: v4-only armor carries the
        // CRC24 footer (GnuPG 2.4); v6 armor never does (utils::crc24_policy_tests).
        let block = base_key();
        let out = add_user_id_internal(0, &block, "C <c@example.test>", None).expect("add");
        crate::testutil::assert::armor_has_crc24(&out);
    }

    // -- update_user_id_internal --------------------------------------------

    #[test]
    fn updating_replaces_the_old_user_id_with_the_new_one() {
        let block = base_key();
        let old = base_uid();
        let out = update_user_id_internal(0, &block, &old, "New <new@example.test>", None)
            .expect("update");
        let uids = uids_of(&out);
        assert!(uids.iter().any(|u| u == "New <new@example.test>"));
        assert!(!uids.contains(&old));
    }

    #[test]
    fn updating_keeps_the_user_id_count_stable() {
        let block = base_key();
        let old = base_uid();
        let before = uids_of(&block).len();
        let out = update_user_id_internal(0, &block, &old, "New <new@example.test>", None)
            .expect("update");
        assert_eq!(uids_of(&out).len(), before);
    }

    #[test]
    fn updating_an_unknown_user_id_fails_after_adding_the_new_one() {
        // The operation is add-then-delete, so a bad `old` surfaces from the
        // delete step as invalid input.
        let block = base_key();
        assert_eq!(
            update_user_id_internal(
                0,
                &block,
                "ghost <ghost@example.test>",
                "N <n@example.test>",
                None
            )
            .err(),
            Some(GfrStatus::ErrorInvalidInput)
        );
    }

    #[test]
    fn the_updated_user_id_carries_a_verifying_self_signature() {
        let block = base_key();
        let old = base_uid();
        let out = update_user_id_internal(0, &block, &old, "Fresh <fresh@example.test>", None)
            .expect("update");
        let key = parse(&out);
        let primary = key.primary_key.public_key();
        let user = key.details.users.first().expect("a user id");
        let sig = user.signatures.first().expect("a self-signature");
        assert!(
            sig.verify_certification(primary, Tag::UserId, &user.id)
                .is_ok()
        );
    }

    // -- set_primary_user_id_internal ---------------------------------------

    #[test]
    fn setting_the_primary_user_id_marks_the_target() {
        // §5.2.3.27: the flag nominates the main user ID for a key.
        let block = base_key();
        let with_second =
            add_user_id_internal(0, &block, "Second <second@example.test>", None).expect("add");
        let out =
            set_primary_user_id_internal(0, &with_second, "Second <second@example.test>", None)
                .expect("set primary");

        let key = parse(&out);
        let target = key
            .details
            .users
            .iter()
            .find(|u| String::from_utf8_lossy(u.id.id()) == "Second <second@example.test>")
            .expect("the target uid");
        assert!(
            target.signatures.iter().any(|s| s.is_primary()),
            "the nominated user ID must carry the Primary User ID flag"
        );
    }

    #[test]
    fn setting_the_primary_user_id_clears_it_on_the_others() {
        // §5.2.3.27 allows more than one to be flagged but leaves the tie-break
        // undefined, so the engine keeps exactly one flagged.
        let block = base_key();
        let with_second =
            add_user_id_internal(0, &block, "Second <second@example.test>", None).expect("add");
        let out =
            set_primary_user_id_internal(0, &with_second, "Second <second@example.test>", None)
                .expect("set primary");

        let key = parse(&out);
        let flagged = key
            .details
            .users
            .iter()
            .filter(|u| u.signatures.iter().any(|s| s.is_primary()))
            .count();
        assert_eq!(flagged, 1, "exactly one user ID may be primary");
    }

    #[test]
    fn setting_an_unknown_primary_user_id_is_invalid_input() {
        let block = base_key();
        assert_eq!(
            set_primary_user_id_internal(0, &block, "ghost <ghost@example.test>", None).err(),
            Some(GfrStatus::ErrorInvalidInput)
        );
    }

    #[test]
    fn setting_primary_preserves_the_key_flags() {
        let block = base_key();
        let uid = base_uid();
        let out = set_primary_user_id_internal(0, &block, &uid, None).expect("set primary");
        let key = parse(&out);
        let user = key.details.users.first().expect("a user id");
        let sig = user.signatures.first().expect("a self-signature");
        assert!(sig.key_flags().certify() || sig.key_flags().sign());
    }

    #[test]
    fn setting_primary_leaves_a_verifying_self_signature() {
        let block = base_key();
        let uid = base_uid();
        let out = set_primary_user_id_internal(0, &block, &uid, None).expect("set primary");
        let key = parse(&out);
        let primary = key.primary_key.public_key();
        let user = key.details.users.first().expect("a user id");
        let sig = user.signatures.first().expect("a self-signature");
        assert!(
            sig.verify_certification(primary, Tag::UserId, &user.id)
                .is_ok()
        );
    }

    #[test]
    fn setting_primary_on_a_public_block_fails() {
        let block = keys::V4_SIGN.public_armored.clone();
        let uid = base_uid();
        assert!(set_primary_user_id_internal(0, &block, &uid, None).is_err());
    }

    #[test]
    fn setting_primary_on_a_v6_key_emits_a_v6_signature() {
        let block = keys::V6_SIGN.secret_armored.clone();
        let uid = uids_of(&block).remove(0);
        let out = set_primary_user_id_internal(0, &block, &uid, None).expect("set primary");
        let key = parse(&out);
        let user = key.details.users.first().expect("a user id");
        assert!(
            user.signatures
                .iter()
                .all(|s| s.version() == pgp::packet::SignatureVersion::V6)
        );
    }

    // -- revoke_user_id_internal --------------------------------------------

    #[test]
    fn revoking_a_user_id_adds_a_certification_revocation() {
        // §5.2.1.13: a Certification Revocation signature (type 0x30) revokes
        // an earlier user ID certification.
        let block = base_key();
        let uid = base_uid();
        let out = revoke_user_id_internal(
            0,
            &block,
            &uid,
            GfrRevocationCode::UserIdInvalid,
            Some("no longer used"),
            None,
        )
        .expect("revoke");

        let key = parse(&out);
        let user = key.details.users.first().expect("a user id");
        assert!(
            user.signatures
                .iter()
                .any(|s| matches!(s.typ(), Some(SignatureType::CertRevocation))),
            "a certification revocation signature must be present"
        );
    }

    #[test]
    fn a_user_id_revocation_verifies_under_the_primary_key() {
        let block = base_key();
        let uid = base_uid();
        let out = revoke_user_id_internal(
            0,
            &block,
            &uid,
            GfrRevocationCode::UserIdInvalid,
            None,
            None,
        )
        .expect("revoke");

        let key = parse(&out);
        let primary = key.primary_key.public_key();
        let user = key.details.users.first().expect("a user id");
        let rev = user
            .signatures
            .iter()
            .find(|s| matches!(s.typ(), Some(SignatureType::CertRevocation)))
            .expect("a revocation signature");
        assert!(
            rev.verify_certification(primary, Tag::UserId, &user.id)
                .is_ok(),
            "a forged revocation would be worthless; this one must verify"
        );
    }

    #[test]
    fn revoking_an_unknown_user_id_is_invalid_input() {
        let block = base_key();
        assert_eq!(
            revoke_user_id_internal(
                0,
                &block,
                "ghost <ghost@example.test>",
                GfrRevocationCode::UserIdInvalid,
                None,
                None
            )
            .err(),
            Some(GfrStatus::ErrorInvalidInput)
        );
    }

    #[test]
    fn a_user_id_revocation_records_its_reason() {
        // §5.2.3.31: "Such a signature revocation SHOULD include a Reason for
        // Revocation subpacket containing code 32."
        let block = base_key();
        let uid = base_uid();
        let out = revoke_user_id_internal(
            0,
            &block,
            &uid,
            GfrRevocationCode::UserIdInvalid,
            Some("address retired"),
            None,
        )
        .expect("revoke");

        let key = parse(&out);
        let user = key.details.users.first().expect("a user id");
        let rev = user
            .signatures
            .iter()
            .find(|s| matches!(s.typ(), Some(SignatureType::CertRevocation)))
            .expect("a revocation signature");
        let has_reason = rev
            .config()
            .map(|c| {
                c.hashed_subpackets
                    .iter()
                    .any(|sp| matches!(sp.data, SubpacketData::RevocationReason(..)))
            })
            .unwrap_or(false);
        assert!(has_reason);
    }

    #[test]
    fn revoking_without_a_reason_string_works() {
        let block = base_key();
        let uid = base_uid();
        assert!(
            revoke_user_id_internal(0, &block, &uid, GfrRevocationCode::NoReason, None, None)
                .is_ok()
        );
    }

    #[test]
    fn revoking_on_a_v6_key_emits_a_v6_revocation() {
        let block = keys::V6_SIGN.secret_armored.clone();
        let uid = uids_of(&block).remove(0);
        let out = revoke_user_id_internal(
            0,
            &block,
            &uid,
            GfrRevocationCode::UserIdInvalid,
            None,
            None,
        )
        .expect("revoke");

        let key = parse(&out);
        let user = key.details.users.first().expect("a user id");
        let rev = user
            .signatures
            .iter()
            .find(|s| matches!(s.typ(), Some(SignatureType::CertRevocation)))
            .expect("a revocation signature");
        assert_eq!(rev.version(), pgp::packet::SignatureVersion::V6);
    }

    #[test]
    fn revoking_a_user_id_does_not_delete_it() {
        // Revocation is a statement, not a removal: third parties need the
        // user ID present to see that it was revoked.
        let block = base_key();
        let uid = base_uid();
        let out = revoke_user_id_internal(
            0,
            &block,
            &uid,
            GfrRevocationCode::UserIdInvalid,
            None,
            None,
        )
        .expect("revoke");
        assert!(uids_of(&out).contains(&uid));
    }

    #[test]
    fn revoking_on_a_public_block_fails() {
        let block = keys::V4_SIGN.public_armored.clone();
        let uid = base_uid();
        assert!(
            revoke_user_id_internal(
                0,
                &block,
                &uid,
                GfrRevocationCode::UserIdInvalid,
                None,
                None
            )
            .is_err()
        );
    }

    #[test]
    fn every_user_id_operation_rejects_garbage_without_panicking() {
        for op in ["delete", "add", "update", "primary", "revoke"] {
            let outcome = std::panic::catch_unwind(|| match op {
                "delete" => delete_user_id_internal("junk", "x").is_err(),
                "add" => add_user_id_internal(0, "junk", "x", None).is_err(),
                "update" => update_user_id_internal(0, "junk", "x", "y", None).is_err(),
                "primary" => set_primary_user_id_internal(0, "junk", "x", None).is_err(),
                _ => {
                    revoke_user_id_internal(0, "junk", "x", GfrRevocationCode::NoReason, None, None)
                        .is_err()
                }
            });
            assert_eq!(outcome.ok(), Some(true), "{op} must fail cleanly");
        }
    }
}
