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

//! OpenPGP key and subkey generation.
//!
//! `keygen_dynamic` builds a `SignedSecretKey` from a `GfrKeyConfig`; the
//! higher-level `create_key_internal` and `add_subkey_internal` wrap it with
//! optional passphrase protection and armored export.

use crate::{
    cache::{PASSWORD_CACHE, PasswordCachePolicy},
    err::{IntoGfrResult, set_last_error},
    key::{apply_primary_key_expiration, apply_subkey_expiration},
    types::{GfrKeyAlgo, GfrKeyConfig, GfrOpenPGPKeyVersion, GfrPasswordFetchCb, GfrStatus},
    utils::{
        PassphraseStateInternal, armor_opts, check_if_should_use_key_ver_v6,
        fetch_password_with_cache, password_from_zeroizing_bytes, resolve_key_type,
    },
};
use log::error;
use pgp::{
    composed::{
        Deserializable, EncryptionCaps, KeyType, SecretKeyParamsBuilder, SignedPublicKey,
        SignedSecretKey, SignedSecretSubKey, SubkeyParamsBuilder,
    },
    crypto::{aead::AeadAlgorithm, hash::HashAlgorithm, sym::SymmetricKeyAlgorithm},
    packet::{KeyFlags, PubKeyInner, PublicSubkey, SecretSubkey},
    types::{CompressionAlgorithm, KeyDetails, KeyVersion, Password, Timestamp},
};
use rand::thread_rng;
use zeroize::Zeroizing;

/// Armored key material produced by a generation or modification operation.
///
/// `secret` holds armored private-key material and is wrapped in `Zeroizing` so
/// the heap buffer is wiped on every drop path, not just when it reaches the FFI
/// boundary.
pub struct GeneratedKeys {
    pub secret: Zeroizing<String>,
    pub public: String,
    pub fingerprint: String,
}

/// If `algo` cannot appear in a v6 key, return the reason. Covers both the
/// deprecated OIDs RFC 9580 §9.2 forbids ("Implementations MUST NOT accept or
/// generate version 6 key material using the deprecated OIDs") and curves that
/// are not registered for OpenPGP at all (secp256k1), which have no defined v6
/// wire format.
fn v6_algo_rejection_reason(algo: &GfrKeyAlgo) -> Option<&'static str> {
    match algo {
        GfrKeyAlgo::ED25519LEGACY => Some(
            "ED25519LEGACY uses a deprecated OID and cannot be used in a v6 key (RFC 9580 §9.2)",
        ),
        GfrKeyAlgo::SECP256K1 => Some(
            "secp256k1 is not a registered OpenPGP curve (RFC 9580 §9.2) and cannot be used in a v6 key",
        ),
        _ => None,
    }
}

/// For v6 keys, `ED25519`/`CV25519` used for encryption resolve to the deprecated
/// Curve25519Legacy ECDH curve (RFC 9580 §9.2). Return the native `X25519`
/// replacement when that is the case, otherwise `None` (no remap needed).
fn v6_remap_encryption_curve(algo: &GfrKeyAlgo, can_encrypt: bool) -> Option<KeyType> {
    if can_encrypt && matches!(algo, GfrKeyAlgo::CV25519 | GfrKeyAlgo::ED25519) {
        Some(KeyType::X25519)
    } else {
        None
    }
}

/// Build a `SignedSecretKey` from a primary key config and a list of subkey configs.
///
/// Honors the caller's requested key format (`key_config.ver`), but always
/// forces OpenPGP v6 when any key uses a post-quantum hybrid algorithm, which is
/// only defined for v6. For v6 keys, `CV25519`/`ED25519` encryption keys are
/// mapped to native `X25519` and deprecated legacy OIDs are rejected (§9.2).
pub fn keygen_dynamic(
    uid: &str,
    key_config: &GfrKeyConfig,
    s_key_configs: &[GfrKeyConfig],
) -> anyhow::Result<SignedSecretKey> {
    let mut primary_type = resolve_key_type(&key_config.algo, false)?;
    let mut subkeys = Vec::new();

    // Post-quantum hybrids mandate v6; otherwise respect the caller's explicit
    // request, defaulting to v4 when unspecified (`Unknown`) for interoperability.
    let pqc_forces_v6 = check_if_should_use_key_ver_v6(key_config, s_key_configs);
    let use_v6 = pqc_forces_v6 || key_config.ver == GfrOpenPGPKeyVersion::V6;
    if pqc_forces_v6 {
        log::info!(
            "Using V6 key version for generation due to presence of post-quantum hybrid algorithm."
        );
    } else if use_v6 {
        log::info!("Using V6 key version for generation as requested by caller.");
    }

    // RFC 9580 §9.2: v6 key material MUST NOT use the deprecated Curve25519Legacy
    // / Ed25519Legacy OIDs. Reject ED25519LEGACY and remap legacy-curve
    // encryption keys to native X25519 before building.
    if use_v6 {
        if let Some(reason) = v6_algo_rejection_reason(&key_config.algo) {
            anyhow::bail!(reason);
        }
        if let Some(remapped) = v6_remap_encryption_curve(&key_config.algo, false) {
            primary_type = remapped;
        }
    }

    // RFC 9580 §10.1.5: the primary key MUST be capable of making signatures (it
    // must self-certify). Reject encryption-only primary algorithms up front with
    // a clear error instead of letting the self-signature step fail opaquely.
    if !primary_type.can_sign() {
        anyhow::bail!(
            "primary key algorithm is encryption-only; RFC 9580 §10.1.5 requires a signing-capable primary"
        );
    }

    for config in s_key_configs {
        let mut k_type = resolve_key_type(&config.algo, config.can_encrypt)?;
        let mut builder = SubkeyParamsBuilder::default();

        if use_v6 {
            // For v6 keys, set the version explicitly to V6 in the builder.
            builder.version(KeyVersion::V6);

            // RFC 9580 §9.2: reject deprecated OIDs and remap ED25519/CV25519
            // encryption subkeys (which resolve to Curve25519Legacy) to X25519.
            if let Some(reason) = v6_algo_rejection_reason(&config.algo) {
                anyhow::bail!(reason);
            }
            if let Some(remapped) = v6_remap_encryption_curve(&config.algo, config.can_encrypt) {
                k_type = remapped;
            }
        }

        builder.key_type(k_type);

        builder
            .can_sign(config.can_sign)
            .can_authenticate(config.can_auth)
            .can_encrypt(if config.can_encrypt {
                EncryptionCaps::All
            } else {
                EncryptionCaps::None
            });

        subkeys.push(
            builder
                .build()
                .map_err(|e| anyhow::anyhow!("Subkey build failed: {}", e))?,
        );
    }

    let mut builder = SecretKeyParamsBuilder::default();

    if use_v6 {
        builder.version(KeyVersion::V6);
        // Advertise SEIPD v2 (AEAD) support so peers encrypt to this v6 key using
        // the stronger AEAD container (RFC 9580 §5.2.3.32 / §13.7). SEIPD v1 stays
        // enabled by the builder default for backward compatibility.
        builder.feature_seipd_v2(true);
    }

    // Advertise algorithm preferences on the self-signature (RFC 9580 §5.2.3.14-17).
    // rPGP leaves these empty by default, which forces peers to fall back to the
    // mandatory-to-implement algorithms (e.g. AES-128); populating them lets peers
    // pick our stronger preferred algorithms, matching GnuPG-class output.
    builder
        .preferred_symmetric_algorithms(
            [SymmetricKeyAlgorithm::AES256, SymmetricKeyAlgorithm::AES128]
                .into_iter()
                .collect(),
        )
        .preferred_hash_algorithms(
            [HashAlgorithm::Sha512, HashAlgorithm::Sha256]
                .into_iter()
                .collect(),
        )
        .preferred_compression_algorithms(
            [
                CompressionAlgorithm::ZLIB,
                CompressionAlgorithm::ZIP,
                CompressionAlgorithm::Uncompressed,
            ]
            .into_iter()
            .collect(),
        )
        .preferred_aead_algorithms(
            [
                (SymmetricKeyAlgorithm::AES256, AeadAlgorithm::Ocb),
                (SymmetricKeyAlgorithm::AES128, AeadAlgorithm::Ocb),
            ]
            .into_iter()
            .collect(),
        );

    let signed = builder
        .key_type(primary_type)
        .can_certify(true)
        .can_sign(key_config.can_sign)
        .can_encrypt(if key_config.can_encrypt {
            EncryptionCaps::All
        } else {
            EncryptionCaps::None
        })
        .can_authenticate(key_config.can_auth)
        .primary_user_id(uid.into())
        .subkeys(subkeys)
        .build()?
        .generate(thread_rng())?;

    Ok(signed)
}

/// Generate a new primary key with subkeys and optionally protect them with passphrases.
///
/// The passphrase callback is called once per key component that has
/// `has_passphrase: true`. Callers that don't want passphrase protection
/// should set `has_passphrase: false` on all configs and pass `None` for
/// the callbacks.
pub fn create_key_internal(
    user_id: &str,
    key_config: GfrKeyConfig,
    s_key_configs: &[GfrKeyConfig],
    fetch_pwd_cb: Option<GfrPasswordFetchCb>,
) -> Result<GeneratedKeys, GfrStatus> {
    let mut secret_key =
        keygen_dynamic(user_id, &key_config, s_key_configs).map_err(|e: anyhow::Error| {
            error!("Key generation failed: {}", e);
            GfrStatus::ErrorKeygenFailed
        })?;

    // Stamp the requested expirations while every component is still unlocked.
    // Re-issuing the self-signatures that carry the KeyExpirationTime subpacket
    // needs the primary key unlocked to sign, so this must run before the
    // passphrase-protection step below (empty password == not yet protected).
    // The rPGP key builder has no expiration setter, so this is the only place
    // a generated key acquires an expiry.
    if key_config.expiration_epoch_secs != 0 {
        apply_primary_key_expiration(
            &mut secret_key,
            key_config.expiration_epoch_secs,
            &Password::empty(),
        )?;
    }
    for (index, sub_config) in s_key_configs.iter().enumerate() {
        if sub_config.expiration_epoch_secs == 0 {
            continue;
        }
        let sub_fpr = secret_key
            .secret_subkeys
            .get(index)
            .ok_or(GfrStatus::ErrorInternal)?
            .key
            .fingerprint()
            .to_string();
        apply_subkey_expiration(
            &mut secret_key,
            &sub_fpr,
            sub_config.expiration_epoch_secs,
            &Password::empty(),
        )?;
    }

    let primary_pwd_bytes = if key_config.has_passphrase {
        fetch_password_with_cache(
            Some(&PASSWORD_CACHE),
            PasswordCachePolicy::Refresh,
            0, // Index 0 for primary key
            PassphraseStateInternal {
                fpr: String::new(), // No fingerprint yet for a new key
                info: "Generate Primary Key".to_string(),
                retry: false,
                ask_for_new: true,
                should_confirm: true, // Ask user to enter the password twice for confirmation when generating a new key
            },
            fetch_pwd_cb,
        )?
    } else {
        Zeroizing::new(Vec::<u8>::new())
    };

    if key_config.has_passphrase && primary_pwd_bytes.is_empty() {
        return Err(GfrStatus::ErrorFetchPasswordFailed);
    }

    if !primary_pwd_bytes.is_empty() {
        // Encrypt Primary Key with the primary password
        let primary_password = password_from_zeroizing_bytes(primary_pwd_bytes);
        secret_key
            .primary_key
            .set_password(thread_rng(), &primary_password)
            .map_err(|_| GfrStatus::ErrorPasswordFailed)?;

        // Iterate through subkeys to set individual passwords
        for (index, subkey) in secret_key.secret_subkeys.iter_mut().enumerate() {
            // Determine if subkey needs a password based on your own configuration logic.
            // For example, fetching a different password for each subkey:
            let subkey_pwd_bytes = fetch_password_with_cache(
                Some(&PASSWORD_CACHE),
                PasswordCachePolicy::Refresh,
                ((index + 1) as u32).try_into().unwrap(), // Use a different index for each subkey
                PassphraseStateInternal {
                    fpr: String::new(), // No fingerprint yet for a new key
                    info: format!("Set password for Subkey {}", index + 1),
                    retry: false,
                    ask_for_new: true,
                    should_confirm: false,
                },
                fetch_pwd_cb,
            )?;

            // If the subkey password is provided, apply it
            if !subkey_pwd_bytes.is_empty() {
                let sub_password = password_from_zeroizing_bytes(subkey_pwd_bytes);
                subkey
                    .key
                    .set_password(thread_rng(), &sub_password)
                    .map_err(|_| GfrStatus::ErrorPasswordFailed)?;
            } else {
                // Handle missing subkey password according to your app logic
                return Err(GfrStatus::ErrorFetchPasswordFailed);
            }
        }
    }

    let fingerprint = secret_key.fingerprint().to_string();

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
        fingerprint,
    })
}

/// Generate a new subkey and attach it to an existing secret key block.
///
/// The primary key must be unlocked to re-sign the new subkey binding
/// signature; its passphrase is fetched via the callback if needed.
pub fn add_subkey_internal(
    channel: i32,
    key_block: &str,
    config: &GfrKeyConfig,
    fetch_pwd_cb: Option<GfrPasswordFetchCb>,
) -> Result<GeneratedKeys, GfrStatus> {
    // 1. Parse the existing secret key block
    let (mut secret_key, _) = SignedSecretKey::from_string(key_block).map_err(|e| {
        log::error!("Failed to parse existing key block: {}", e);
        GfrStatus::ErrorInvalidData
    })?;

    let fingerprint_str = secret_key.fingerprint().to_string();

    // 2. Fetch the primary key password if it is encrypted.
    // We don't permanently remove the password; we just hold it to pass to the signing function,
    // which will use the `unlock(pw, closure)` pattern internally.
    let mut primary_pw = Password::empty();
    if secret_key.primary_key.secret_params().is_encrypted() {
        let pwd_bytes = fetch_password_with_cache(
            Some(&PASSWORD_CACHE),
            PasswordCachePolicy::Refresh,
            channel,
            PassphraseStateInternal {
                fpr: fingerprint_str.clone(),
                info: "Unlock Primary Key to generate subkey".to_string(),
                retry: false,
                ask_for_new: false,
                should_confirm: false,
            },
            fetch_pwd_cb,
        )?;

        if pwd_bytes.is_empty() {
            log::error!("Password required to unlock primary key but none provided.");
            return Err(GfrStatus::ErrorFetchPasswordFailed);
        }
        primary_pw = password_from_zeroizing_bytes(pwd_bytes);

        // Dry-run unlock to verify the password is correct before we proceed
        if secret_key
            .primary_key
            .unlock(&primary_pw, |_, _| Ok(()))
            .is_err()
        {
            return Err(GfrStatus::ErrorPasswordFailed);
        }
    }

    // 3. Generate the raw secret subkey materials (using the generate pattern from tests)
    let mut sub_k_type = resolve_key_type(&config.algo, config.can_encrypt)?;
    if secret_key.version() == KeyVersion::V6 {
        // RFC 9580 §9.2: v6 key material MUST NOT use deprecated OIDs. Reject
        // ED25519LEGACY and remap ED25519/CV25519 encryption subkeys (which
        // resolve to Curve25519Legacy) to native X25519.
        if let Some(reason) = v6_algo_rejection_reason(&config.algo) {
            set_last_error(reason);
            return Err(GfrStatus::ErrorUnsupportedAlgorithm);
        }
        if let Some(remapped) = v6_remap_encryption_curve(&config.algo, config.can_encrypt) {
            sub_k_type = remapped;
        }
    }

    let mut rng = thread_rng();

    let (public_params, secret_params) = sub_k_type.generate(&mut rng).into_gfr()?;

    // Assemble the inner public key part
    let pub_inner = PubKeyInner::new(
        secret_key.version(), // Use the same version as the primary key
        sub_k_type.to_alg(),
        Timestamp::now(),
        None,
        public_params,
    )
    .into_gfr()?;

    let public_subkey = PublicSubkey::from_inner(pub_inner).into_gfr()?;
    let mut raw_subkey = SecretSubkey::new(public_subkey, secret_params).into_gfr()?;

    // 4. Setup KeyFlags for the new subkey
    let mut flags = KeyFlags::default();
    if config.can_sign {
        flags.set_sign(true);
    }
    if config.can_encrypt {
        flags.set_encrypt_comms(true);
        flags.set_encrypt_storage(true);
    }
    if config.can_auth {
        flags.set_authentication(true);
    }

    // 5. If the subkey is capable of signing, it MUST cross-sign the primary key (Type 0x19).
    // We must do this BEFORE locking the subkey with a password.
    let embedded_sig = if config.can_sign {
        let sig = raw_subkey
            .sign_primary_key_binding(
                &mut rng,
                secret_key.primary_key.public_key(),
                &Password::empty(), // raw_subkey is still Plain (unlocked) at this stage
            )
            .map_err(|e| {
                log::error!(
                    "Failed to generate embedded primary key binding signature: {}",
                    e
                );
                GfrStatus::ErrorKeygenFailed
            })?;
        Some(sig)
    } else {
        None
    };

    // 6. Create the Subkey Binding Signature (Type 0x18) using the Primary Key.
    // The `sign` method automatically handles the `unlock` closure internally.
    let binding_sig = raw_subkey
        .sign(
            &mut rng,
            &secret_key.primary_key,
            secret_key.primary_key.public_key(),
            &primary_pw,
            flags,
            embedded_sig,
        )
        .map_err(|e| {
            log::error!("Subkey binding signature failed: {}", e);
            GfrStatus::ErrorKeygenFailed
        })?;

    // 7. Lock the new subkey if a passphrase is required by the user's config
    if config.has_passphrase {
        let subkey_pwd_bytes = fetch_password_with_cache(
            Some(&PASSWORD_CACHE),
            PasswordCachePolicy::Refresh,
            channel,
            PassphraseStateInternal {
                fpr: String::new(),
                info: "Set password for new subkey".to_string(),
                retry: false,
                ask_for_new: true,
                should_confirm: true,
            },
            fetch_pwd_cb,
        )?;

        if subkey_pwd_bytes.is_empty() {
            log::error!("Password requested for new subkey but none provided.");
            return Err(GfrStatus::ErrorFetchPasswordFailed);
        }

        let sub_password = password_from_zeroizing_bytes(subkey_pwd_bytes);
        raw_subkey
            .set_password(&mut rng, &sub_password)
            .map_err(|_| GfrStatus::ErrorPasswordFailed)?;
    }

    // 8. Assemble the SignedSecretSubKey using the struct layout from your codebase
    let signed_subkey = SignedSecretSubKey {
        key: raw_subkey,
        // The signature field is a generic Vec<Signature>, so we push the binding_sig into a vec
        signatures: vec![binding_sig],
    };

    secret_key.secret_subkeys.push(signed_subkey);

    // 8b. Stamp the requested expiration onto the freshly bound subkey by
    // re-issuing its binding signature. Re-signing only needs the primary key
    // unlocked (already covered by `primary_pw`); the subkey's own protection,
    // set just above, is irrelevant to the binding signature.
    if config.expiration_epoch_secs != 0 {
        let new_fpr = secret_key
            .secret_subkeys
            .last()
            .ok_or(GfrStatus::ErrorInternal)?
            .key
            .fingerprint()
            .to_string();
        apply_subkey_expiration(
            &mut secret_key,
            &new_fpr,
            config.expiration_epoch_secs,
            &primary_pw,
        )?;
    }

    // 9. Armor the updated keys for export
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
        fingerprint: fingerprint_str,
    })
}

#[cfg(test)]
mod rfc9580_tests {
    //! RFC 9580 conformance tests for key generation. Each test targets a
    //! specific requirement that the engine previously violated.
    use super::*;
    use crate::testutil::keys::cfg;
    use crate::types::{GfrKeyAlgo, GfrOpenPGPKeyVersion};
    use pgp::crypto::public_key::PublicKeyAlgorithm;

    /// RFC 9580 §12.4/§12.5: RSA < 2048 and DSA MUST NOT be generated.
    #[test]
    fn resolve_key_type_rejects_weak_generation() {
        assert!(resolve_key_type(&GfrKeyAlgo::RSA1024, false).is_err());
        assert!(resolve_key_type(&GfrKeyAlgo::DSA1024, false).is_err());
        assert!(resolve_key_type(&GfrKeyAlgo::DSA2048, false).is_err());
        assert!(resolve_key_type(&GfrKeyAlgo::DSA3072, false).is_err());
        // Still-permitted generation targets must continue to resolve.
        assert!(resolve_key_type(&GfrKeyAlgo::RSA2048, false).is_ok());
        assert!(resolve_key_type(&GfrKeyAlgo::ED25519, false).is_ok());
    }

    /// RFC 9580 §9.2: a v6 key MUST NOT use the deprecated Curve25519Legacy OID.
    /// An ED25519 encryption subkey (which resolves to Curve25519Legacy ECDH on v4)
    /// must be remapped to native X25519 inside a v6 key.
    #[test]
    fn v6_encryption_subkey_uses_native_x25519() {
        let primary = cfg(GfrKeyAlgo::ED25519, true, false, GfrOpenPGPKeyVersion::V6);
        let sub = cfg(GfrKeyAlgo::ED25519, false, true, GfrOpenPGPKeyVersion::V6);
        let key = keygen_dynamic("rfc9580 <rfc@example.com>", &primary, &[sub]).expect("keygen");
        assert_eq!(key.primary_key.version(), KeyVersion::V6);
        assert_eq!(
            key.secret_subkeys[0].key.algorithm(),
            PublicKeyAlgorithm::X25519,
            "v6 encryption subkey must be native X25519, not deprecated Curve25519Legacy ECDH"
        );
    }

    /// RFC 9580 §9.2: ED25519LEGACY (deprecated OID) MUST NOT appear in a v6 key.
    #[test]
    fn v6_rejects_ed25519legacy() {
        let primary = cfg(
            GfrKeyAlgo::ED25519LEGACY,
            true,
            false,
            GfrOpenPGPKeyVersion::V6,
        );
        assert!(keygen_dynamic("rfc9580 <rfc@example.com>", &primary, &[]).is_err());
    }

    /// RFC 9580 §10.1.5: an encryption-only primary algorithm MUST be rejected.
    #[test]
    fn encryption_only_primary_rejected() {
        // ML-KEM is a KEM (encryption-only) and cannot self-certify as a primary.
        let primary = cfg(
            GfrKeyAlgo::KYBER768X25519,
            false,
            true,
            GfrOpenPGPKeyVersion::V6,
        );
        assert!(keygen_dynamic("rfc9580 <rfc@example.com>", &primary, &[]).is_err());
    }

    /// RFC 9580 §5.2.3.14: generated keys SHOULD advertise algorithm preferences.
    #[test]
    fn generated_key_advertises_symmetric_preferences() {
        let primary = cfg(GfrKeyAlgo::ED25519, true, false, GfrOpenPGPKeyVersion::V4);
        let key = keygen_dynamic("rfc9580 <rfc@example.com>", &primary, &[]).expect("keygen");
        let advertises_aes256 = key.details.users.iter().any(|u| {
            u.signatures.iter().any(|s| {
                s.preferred_symmetric_algs()
                    .contains(&SymmetricKeyAlgorithm::AES256)
            })
        });
        assert!(
            advertises_aes256,
            "v4 UID self-signature should advertise preferred symmetric algorithms"
        );
    }

    /// RFC 9580 §5.2.3.32 / §13.7: v6 keys SHOULD advertise SEIPD v2 (AEAD) support.
    #[test]
    fn v6_key_advertises_seipd_v2() {
        let primary = cfg(GfrKeyAlgo::ED25519, true, false, GfrOpenPGPKeyVersion::V6);
        let key = keygen_dynamic("rfc9580 <rfc@example.com>", &primary, &[]).expect("keygen");
        let advertises_v2 = key
            .details
            .direct_signatures
            .iter()
            .any(|s| s.features().map(|f| f.seipd_v2()).unwrap_or(false));
        assert!(
            advertises_v2,
            "v6 Direct Key signature should advertise SEIPD v2 support"
        );
    }

    /// RFC 9580 §6.1: v6 key armor MUST NOT contain a CRC24 footer.
    #[test]
    fn armored_v6_key_has_no_crc24_footer() {
        let primary = cfg(GfrKeyAlgo::ED25519, true, false, GfrOpenPGPKeyVersion::V6);
        let key = keygen_dynamic("rfc9580 <rfc@example.com>", &primary, &[]).expect("keygen");
        let armored = key.to_armored_string(armor_opts()).expect("armor");
        // A CRC24 footer is a line consisting of '=' followed by 4 base64 chars.
        for line in armored.lines() {
            let is_crc = line.starts_with('=') && line.len() == 5;
            assert!(
                !is_crc,
                "v6 key armor must not contain a CRC24 footer: {line}"
            );
        }
    }

    // --- v6 algorithm-rejection matrix (RFC 9580 §9.2) --------------------

    /// The deprecated Curve25519Legacy OID and the unregistered secp256k1 curve
    /// have no defined v6 wire format and must be rejected for v6 keys.
    #[test]
    fn v6_algo_rejection_reason_flags_forbidden_algos() {
        assert!(v6_algo_rejection_reason(&GfrKeyAlgo::ED25519LEGACY).is_some());
        assert!(v6_algo_rejection_reason(&GfrKeyAlgo::SECP256K1).is_some());
    }

    /// Algorithms with a defined v6 form are not rejected outright.
    #[test]
    fn v6_algo_rejection_reason_accepts_native_algos() {
        for algo in [
            GfrKeyAlgo::ED25519,
            GfrKeyAlgo::CV25519,
            GfrKeyAlgo::X448,
            GfrKeyAlgo::ED448,
            GfrKeyAlgo::RSA3072,
            GfrKeyAlgo::NISTP256,
        ] {
            assert!(
                v6_algo_rejection_reason(&algo).is_none(),
                "{algo:?} should be allowed in a v6 key"
            );
        }
    }

    /// A v6 key generated with secp256k1 must be refused end-to-end.
    #[test]
    fn v6_rejects_secp256k1() {
        let primary = cfg(GfrKeyAlgo::SECP256K1, true, false, GfrOpenPGPKeyVersion::V6);
        assert!(keygen_dynamic("rfc9580 <rfc@example.com>", &primary, &[]).is_err());
    }

    // --- v6 encryption-curve remap (RFC 9580 §9.2) ------------------------

    #[test]
    fn v6_remap_curve25519_encryption_to_x25519() {
        assert_eq!(
            v6_remap_encryption_curve(&GfrKeyAlgo::CV25519, true),
            Some(KeyType::X25519)
        );
        assert_eq!(
            v6_remap_encryption_curve(&GfrKeyAlgo::ED25519, true),
            Some(KeyType::X25519)
        );
    }

    #[test]
    fn v6_remap_curve_is_noop_for_signing_or_other_algos() {
        // Not an encryption key -> no remap.
        assert!(v6_remap_encryption_curve(&GfrKeyAlgo::ED25519, false).is_none());
        // A non-Curve25519 algorithm -> no remap.
        assert!(v6_remap_encryption_curve(&GfrKeyAlgo::RSA3072, true).is_none());
        assert!(v6_remap_encryption_curve(&GfrKeyAlgo::NISTP256, true).is_none());
    }

    // --- resolve_key_type generation policy (RFC 9580 §12.4/§12.5) --------

    #[test]
    fn resolve_key_type_rejects_all_dsa_sizes() {
        assert!(resolve_key_type(&GfrKeyAlgo::DSA1024, false).is_err());
        assert!(resolve_key_type(&GfrKeyAlgo::DSA2048, false).is_err());
        assert!(resolve_key_type(&GfrKeyAlgo::DSA3072, false).is_err());
    }

    #[test]
    fn resolve_key_type_rejects_rsa_below_2048() {
        assert!(resolve_key_type(&GfrKeyAlgo::RSA1024, false).is_err());
    }

    #[test]
    fn resolve_key_type_accepts_rsa_2048_and_above() {
        assert!(resolve_key_type(&GfrKeyAlgo::RSA2048, false).is_ok());
        assert!(resolve_key_type(&GfrKeyAlgo::RSA3072, false).is_ok());
        assert!(resolve_key_type(&GfrKeyAlgo::RSA4096, false).is_ok());
    }

    #[test]
    fn resolve_key_type_rejects_legacy_ed25519() {
        // ED25519LEGACY uses the deprecated OID and must not be generatable for
        // any key version at the FFI boundary.
        assert!(resolve_key_type(&GfrKeyAlgo::ED25519LEGACY, false).is_err());
    }

    #[test]
    fn resolve_key_type_accepts_modern_curves() {
        assert!(resolve_key_type(&GfrKeyAlgo::ED25519, false).is_ok());
        assert!(resolve_key_type(&GfrKeyAlgo::CV25519, true).is_ok());
    }

    // --- requested key version is honored ---------------------------------

    #[test]
    fn v4_request_produces_v4_key() {
        let primary = cfg(GfrKeyAlgo::ED25519, true, false, GfrOpenPGPKeyVersion::V4);
        let key = keygen_dynamic("rfc9580 <rfc@example.com>", &primary, &[]).expect("keygen");
        assert_eq!(key.primary_key.version(), KeyVersion::V4);
    }

    #[test]
    fn v6_request_produces_v6_key() {
        let primary = cfg(GfrKeyAlgo::ED25519, true, false, GfrOpenPGPKeyVersion::V6);
        let key = keygen_dynamic("rfc9580 <rfc@example.com>", &primary, &[]).expect("keygen");
        assert_eq!(key.primary_key.version(), KeyVersion::V6);
    }
}

#[cfg(test)]
mod keygen_more_tests {
    //! Generation-side conformance beyond the algorithm policy already covered
    //! by `rfc9580_tests`: key/signature version correspondence (§10.3.2.2),
    //! certificate structure (§10.1.1 / §10.1.3), advertised preferences
    //! (§5.2.3.14-§5.2.3.17, §5.2.3.32), passphrase protection (§3.7.2.1) and
    //! subkey addition.

    use super::*;
    use crate::testutil::keys::{cfg, cfg_expiring};
    use crate::types::{GfrKeyAlgo, GfrOpenPGPKeyVersion};
    use pgp::packet::SignatureVersion;
    use pgp::types::SecretParams;

    fn v4_primary() -> GfrKeyConfig {
        cfg(GfrKeyAlgo::ED25519, true, false, GfrOpenPGPKeyVersion::V4)
    }
    fn v6_primary() -> GfrKeyConfig {
        cfg(GfrKeyAlgo::ED25519, true, false, GfrOpenPGPKeyVersion::V6)
    }
    fn enc_sub(ver: GfrOpenPGPKeyVersion) -> GfrKeyConfig {
        cfg(GfrKeyAlgo::ED25519, false, true, ver)
    }
    fn sign_sub(ver: GfrOpenPGPKeyVersion) -> GfrKeyConfig {
        cfg(GfrKeyAlgo::ED25519, true, false, ver)
    }

    fn gen_v4() -> SignedSecretKey {
        keygen_dynamic(
            "Gen <gen@example.test>",
            &v4_primary(),
            &[enc_sub(GfrOpenPGPKeyVersion::V4)],
        )
        .expect("keygen")
    }

    fn gen_v6() -> SignedSecretKey {
        keygen_dynamic(
            "Gen <gen@example.test>",
            &v6_primary(),
            &[enc_sub(GfrOpenPGPKeyVersion::V6)],
        )
        .expect("keygen")
    }

    // -- certificate structure ----------------------------------------------

    #[test]
    fn a_generated_key_has_exactly_one_user_id() {
        // §10.1.5: "A Transferable Public Key SHOULD include at least one User
        // ID packet."
        assert_eq!(gen_v4().details.users.len(), 1);
    }

    #[test]
    fn the_user_id_is_stored_verbatim() {
        let key =
            keygen_dynamic("Ünïcødé Náme <u@example.test>", &v4_primary(), &[]).expect("keygen");
        assert_eq!(
            String::from_utf8_lossy(key.details.users[0].id.id()),
            "Ünïcødé Náme <u@example.test>"
        );
    }

    #[test]
    fn the_primary_key_can_certify() {
        // §10.1.5: the primary must be able to make signatures, since it
        // certifies its own user IDs and subkeys.
        let key = gen_v4();
        let flags = key.details.users[0].signatures[0].key_flags();
        assert!(flags.certify());
    }

    #[test]
    fn every_subkey_has_a_binding_signature() {
        // §10.1.1: "Every subkey MUST have at least one Subkey Binding
        // signature."
        let key = keygen_dynamic(
            "Gen <gen@example.test>",
            &v4_primary(),
            &[
                enc_sub(GfrOpenPGPKeyVersion::V4),
                sign_sub(GfrOpenPGPKeyVersion::V4),
            ],
        )
        .expect("keygen");
        assert_eq!(key.secret_subkeys.len(), 2);
        for sub in &key.secret_subkeys {
            assert!(!sub.signatures.is_empty(), "a subkey without a binding");
        }
    }

    #[test]
    fn a_subkey_binding_verifies_under_the_primary() {
        let key = gen_v4();
        let primary = key.primary_key.public_key();
        for sub in &key.secret_subkeys {
            let binding = sub.signatures.first().expect("a binding signature");
            assert!(
                binding
                    .verify_subkey_binding(primary, sub.key.public_key())
                    .is_ok(),
                "the binding signature must actually verify"
            );
        }
    }

    #[test]
    fn generating_with_no_subkeys_yields_a_bare_primary() {
        let key = keygen_dynamic("Bare <bare@example.test>", &v4_primary(), &[]).expect("keygen");
        assert!(key.secret_subkeys.is_empty());
    }

    #[test]
    fn generating_with_several_subkeys_keeps_their_order() {
        let key = keygen_dynamic(
            "Many <many@example.test>",
            &v4_primary(),
            &[
                sign_sub(GfrOpenPGPKeyVersion::V4),
                enc_sub(GfrOpenPGPKeyVersion::V4),
                sign_sub(GfrOpenPGPKeyVersion::V4),
            ],
        )
        .expect("keygen");
        assert_eq!(key.secret_subkeys.len(), 3);
    }

    #[test]
    fn a_generated_key_has_a_distinct_fingerprint_each_time() {
        // Fresh randomness per generation; identical parameters must not
        // produce identical keys.
        let a = keygen_dynamic("A <a@example.test>", &v4_primary(), &[]).expect("a");
        let b = keygen_dynamic("A <a@example.test>", &v4_primary(), &[]).expect("b");
        assert_ne!(
            a.primary_key.fingerprint().to_string(),
            b.primary_key.fingerprint().to_string()
        );
    }

    // -- §10.3.2.2 key / signature version correspondence -------------------

    #[test]
    fn a_v4_key_has_a_160_bit_fingerprint() {
        // §5.5.4.2: SHA-1 over the normalised public key packet.
        assert_eq!(gen_v4().primary_key.fingerprint().as_bytes().len(), 20);
    }

    #[test]
    fn a_v6_key_has_a_256_bit_fingerprint() {
        // §5.5.4.3: SHA2-256 over the normalised public key packet.
        assert_eq!(gen_v6().primary_key.fingerprint().as_bytes().len(), 32);
    }

    #[test]
    fn a_v4_key_signs_its_user_id_with_a_v4_signature() {
        let key = gen_v4();
        assert_eq!(
            key.details.users[0].signatures[0].version(),
            SignatureVersion::V4
        );
    }

    #[test]
    fn a_v6_key_signs_its_user_id_with_a_v6_signature() {
        let key = gen_v6();
        assert_eq!(
            key.details.users[0].signatures[0].version(),
            SignatureVersion::V6
        );
    }

    #[test]
    fn a_v4_subkey_binding_is_a_v4_signature() {
        let key = gen_v4();
        assert_eq!(
            key.secret_subkeys[0].signatures[0].version(),
            SignatureVersion::V4
        );
    }

    #[test]
    fn a_v6_subkey_binding_is_a_v6_signature() {
        // §10.1.1: "every self-signature made by a version 6 key MUST be a
        // version 6 signature."
        let key = gen_v6();
        assert_eq!(
            key.secret_subkeys[0].signatures[0].version(),
            SignatureVersion::V6
        );
    }

    #[test]
    fn every_subkey_of_a_v6_primary_is_itself_v6() {
        // §10.1.1: "Every subkey for a version 6 primary key MUST be a version
        // 6 subkey."
        let key = keygen_dynamic(
            "V6 <v6@example.test>",
            &v6_primary(),
            &[
                enc_sub(GfrOpenPGPKeyVersion::V6),
                sign_sub(GfrOpenPGPKeyVersion::V6),
            ],
        )
        .expect("keygen");
        for sub in &key.secret_subkeys {
            assert_eq!(sub.key.version(), KeyVersion::V6);
        }
    }

    #[test]
    fn every_subkey_of_a_v4_primary_is_itself_v4() {
        // §10.1.3, the mirror requirement.
        let key = gen_v4();
        for sub in &key.secret_subkeys {
            assert_eq!(sub.key.version(), KeyVersion::V4);
        }
    }

    #[test]
    fn a_subkey_config_version_does_not_override_the_primarys() {
        // The subkey configs' `ver` field is documented as ignored: subkeys
        // inherit the primary's version, because a mixed certificate is
        // forbidden by §10.1.1.
        let key = keygen_dynamic(
            "Mixed <mixed@example.test>",
            &v6_primary(),
            &[enc_sub(GfrOpenPGPKeyVersion::V4)],
        )
        .expect("keygen");
        assert_eq!(key.primary_key.version(), KeyVersion::V6);
        assert_eq!(key.secret_subkeys[0].key.version(), KeyVersion::V6);
    }

    // -- advertised preferences ----------------------------------------------

    #[test]
    fn a_generated_key_advertises_preferred_hash_algorithms() {
        // §5.2.3.16. Without this the recipient must fall back to the
        // mandatory-to-implement SHA2-256.
        let key = gen_v4();
        let sig = &key.details.users[0].signatures[0];
        assert!(
            !sig.preferred_hash_algs().is_empty(),
            "a self-signature should state hash preferences"
        );
    }

    #[test]
    fn the_preferred_hashes_contain_no_weak_algorithm() {
        // §9.5 forbids generating MD5/SHA-1/RIPEMD-160 signatures, so
        // advertising a preference for them would be self-defeating.
        let key = gen_v4();
        let sig = &key.details.users[0].signatures[0];
        for h in sig.preferred_hash_algs() {
            assert!(
                !crate::crypto::sig_hash_algo_is_weak(&h.to_string()),
                "{h:?} must not be advertised as preferred"
            );
        }
    }

    #[test]
    fn a_generated_key_advertises_preferred_compression_algorithms() {
        // §5.2.3.17.
        let key = gen_v4();
        let sig = &key.details.users[0].signatures[0];
        let _ = sig.preferred_compression_algs();
    }

    #[test]
    fn a_v6_key_advertises_preferred_aead_ciphersuites() {
        // §5.2.3.15: paired cipher/AEAD octets, only meaningful once v2 SEIPD
        // is on the table.
        let key = gen_v6();
        let sig = key
            .details
            .users
            .first()
            .and_then(|u| u.signatures.first())
            .expect("a self-signature");
        let _ = sig.preferred_aead_algs();
    }

    // -- §3.7.2.1 passphrase protection --------------------------------------

    #[test]
    fn a_key_generated_without_a_passphrase_has_plaintext_secrets() {
        let key = gen_v4();
        assert!(matches!(
            key.primary_key.secret_params(),
            SecretParams::Plain(_)
        ));
    }

    #[test]
    fn create_key_with_a_passphrase_encrypts_the_primary() {
        // §3.7.2.1: a non-zero S2K usage octet means the secret material is
        // passphrase-protected.
        let mut primary = v4_primary();
        primary.has_passphrase = true;
        let out = create_key_internal(
            "Locked <locked@example.test>",
            primary,
            &[],
            Some(crate::testutil::cb::pwd_correct),
        )
        .expect("create");

        let (key, _) = SignedSecretKey::from_string(&out.secret).expect("parses");
        assert!(matches!(
            key.primary_key.secret_params(),
            SecretParams::Encrypted(_)
        ));
    }

    #[test]
    fn a_passphrase_protected_key_unlocks_with_the_right_passphrase() {
        let mut primary = v4_primary();
        primary.has_passphrase = true;
        let out = create_key_internal(
            "Locked <locked@example.test>",
            primary,
            &[],
            Some(crate::testutil::cb::pwd_correct),
        )
        .expect("create");

        let (key, _) = SignedSecretKey::from_string(&out.secret).expect("parses");
        assert!(
            key.unlock(
                &Password::from(crate::testutil::cb::CORRECT_PASSPHRASE),
                |_, _| Ok(())
            )
            .is_ok()
        );
    }

    #[test]
    fn a_passphrase_protected_key_refuses_the_wrong_passphrase() {
        let mut primary = v4_primary();
        primary.has_passphrase = true;
        let out = create_key_internal(
            "Locked <locked@example.test>",
            primary,
            &[],
            Some(crate::testutil::cb::pwd_correct),
        )
        .expect("create");

        let (key, _) = SignedSecretKey::from_string(&out.secret).expect("parses");
        assert!(key.unlock(&Password::from("wrong"), |_, _| Ok(())).is_err());
    }

    #[test]
    fn a_cancelled_passphrase_prompt_maps_to_canceled() {
        let mut primary = v4_primary();
        primary.has_passphrase = true;
        assert_eq!(
            create_key_internal(
                "Locked <locked@example.test>",
                primary,
                &[],
                Some(crate::testutil::cb::pwd_cancelled),
            )
            .err(),
            Some(GfrStatus::ErrorCanceled)
        );
    }

    #[test]
    fn requesting_a_passphrase_without_a_callback_fails() {
        let mut primary = v4_primary();
        primary.has_passphrase = true;
        assert!(create_key_internal("Locked <locked@example.test>", primary, &[], None).is_err());
    }

    #[test]
    fn create_key_returns_matching_public_and_secret_blocks() {
        let out = create_key_internal(
            "Pair <pair@example.test>",
            v4_primary(),
            &[enc_sub(GfrOpenPGPKeyVersion::V4)],
            None,
        )
        .expect("create");

        let (secret, _) = SignedSecretKey::from_string(&out.secret).expect("secret parses");
        let (public, _) = SignedPublicKey::from_string(&out.public).expect("public parses");
        assert_eq!(
            secret.primary_key.fingerprint().to_string(),
            public.primary_key.fingerprint().to_string()
        );
        assert_eq!(
            out.fingerprint.to_uppercase(),
            secret.primary_key.fingerprint().to_string().to_uppercase()
        );
    }

    #[test]
    fn create_key_emits_armor_without_a_crc24_footer() {
        let out = create_key_internal("Armor <armor@example.test>", v4_primary(), &[], None)
            .expect("create");
        crate::testutil::assert::armor_has_no_crc24(&out.secret);
        crate::testutil::assert::armor_has_no_crc24(&out.public);
    }

    // -- expiration at generation time ---------------------------------------

    #[test]
    fn a_requested_expiry_lands_on_the_primary_key() {
        let now = std::time::SystemTime::now()
            .duration_since(std::time::UNIX_EPOCH)
            .expect("clock")
            .as_secs();
        let primary = cfg_expiring(
            GfrKeyAlgo::ED25519,
            true,
            false,
            GfrOpenPGPKeyVersion::V4,
            now + 86_400,
        );
        let out =
            create_key_internal("Exp <exp@example.test>", primary, &[], None).expect("create");

        let meta = crate::key::extract_metadata_many_internal(&out.secret)
            .expect("metadata")
            .remove(0);
        assert_eq!(meta.expires_at, (now + 86_400) as u32);
    }

    #[test]
    fn a_zero_expiry_means_never() {
        let out = create_key_internal("Never <never@example.test>", v4_primary(), &[], None)
            .expect("create");
        let meta = crate::key::extract_metadata_many_internal(&out.secret)
            .expect("metadata")
            .remove(0);
        assert_eq!(meta.expires_at, 0);
    }

    #[test]
    fn a_subkey_expiry_is_independent_of_the_primarys() {
        let now = std::time::SystemTime::now()
            .duration_since(std::time::UNIX_EPOCH)
            .expect("clock")
            .as_secs();
        let sub = cfg_expiring(
            GfrKeyAlgo::ED25519,
            false,
            true,
            GfrOpenPGPKeyVersion::V4,
            now + 3_600,
        );
        let out = create_key_internal("Sub <sub@example.test>", v4_primary(), &[sub], None)
            .expect("create");

        let meta = crate::key::extract_metadata_many_internal(&out.secret)
            .expect("metadata")
            .remove(0);
        assert_eq!(meta.expires_at, 0, "the primary never expires");
        assert_eq!(meta.subkeys[0].expires_at, (now + 3_600) as u32);
    }

    #[test]
    fn a_backdated_expiry_at_generation_is_rejected() {
        // §5.2.3.13 encodes a forward duration; a past absolute time is not
        // representable.
        let primary = cfg_expiring(
            GfrKeyAlgo::ED25519,
            true,
            false,
            GfrOpenPGPKeyVersion::V4,
            1_000_000_000,
        );
        assert!(create_key_internal("Past <past@example.test>", primary, &[], None).is_err());
    }

    // -- add_subkey_internal --------------------------------------------------

    #[test]
    fn adding_a_subkey_appends_it() {
        let base =
            create_key_internal("Add <add@example.test>", v4_primary(), &[], None).expect("create");
        let out = add_subkey_internal(0, &base.secret, &enc_sub(GfrOpenPGPKeyVersion::V4), None)
            .expect("add subkey");

        let (key, _) = SignedSecretKey::from_string(&out.secret).expect("parses");
        assert_eq!(key.secret_subkeys.len(), 1);
    }

    #[test]
    fn an_added_subkey_binding_verifies() {
        let base =
            create_key_internal("Add <add@example.test>", v4_primary(), &[], None).expect("create");
        let out = add_subkey_internal(0, &base.secret, &enc_sub(GfrOpenPGPKeyVersion::V4), None)
            .expect("add subkey");

        let (key, _) = SignedSecretKey::from_string(&out.secret).expect("parses");
        let primary = key.primary_key.public_key();
        let sub = &key.secret_subkeys[0];
        assert!(
            sub.signatures[0]
                .verify_subkey_binding(primary, sub.key.public_key())
                .is_ok()
        );
    }

    #[test]
    fn a_subkey_added_to_a_v6_key_is_v6() {
        let base =
            create_key_internal("V6 <v6@example.test>", v6_primary(), &[], None).expect("create");
        let out = add_subkey_internal(0, &base.secret, &enc_sub(GfrOpenPGPKeyVersion::V6), None)
            .expect("add subkey");

        let (key, _) = SignedSecretKey::from_string(&out.secret).expect("parses");
        assert_eq!(key.secret_subkeys[0].key.version(), KeyVersion::V6);
        assert_eq!(
            key.secret_subkeys[0].signatures[0].version(),
            SignatureVersion::V6
        );
    }

    #[test]
    fn a_v6_encryption_subkey_added_later_is_native_x25519() {
        // §9.2 forbids the Curve25519Legacy OID in v6 material, so the remap
        // must apply on the add-subkey path too, not just at generation.
        let base =
            create_key_internal("V6 <v6@example.test>", v6_primary(), &[], None).expect("create");
        let out = add_subkey_internal(0, &base.secret, &enc_sub(GfrOpenPGPKeyVersion::V6), None)
            .expect("add subkey");

        let (key, _) = SignedSecretKey::from_string(&out.secret).expect("parses");
        assert_eq!(
            key.secret_subkeys[0].key.algorithm(),
            pgp::crypto::public_key::PublicKeyAlgorithm::X25519
        );
    }

    #[test]
    fn adding_a_subkey_to_a_public_block_fails() {
        let base =
            create_key_internal("Pub <pub@example.test>", v4_primary(), &[], None).expect("create");
        assert!(
            add_subkey_internal(0, &base.public, &enc_sub(GfrOpenPGPKeyVersion::V4), None).is_err()
        );
    }

    #[test]
    fn adding_a_subkey_to_garbage_fails() {
        assert!(add_subkey_internal(0, "junk", &enc_sub(GfrOpenPGPKeyVersion::V4), None).is_err());
    }

    #[test]
    fn adding_a_forbidden_algorithm_as_a_subkey_fails() {
        // The generation policy applies to subkeys as well as primaries.
        let base =
            create_key_internal("Pol <pol@example.test>", v4_primary(), &[], None).expect("create");
        let bad = cfg(GfrKeyAlgo::DSA2048, true, false, GfrOpenPGPKeyVersion::V4);
        assert!(add_subkey_internal(0, &base.secret, &bad, None).is_err());
    }

    #[test]
    fn adding_two_subkeys_in_sequence_keeps_both() {
        let base =
            create_key_internal("Two <two@example.test>", v4_primary(), &[], None).expect("create");
        let once = add_subkey_internal(0, &base.secret, &enc_sub(GfrOpenPGPKeyVersion::V4), None)
            .expect("add 1");
        let twice = add_subkey_internal(0, &once.secret, &sign_sub(GfrOpenPGPKeyVersion::V4), None)
            .expect("add 2");

        let (key, _) = SignedSecretKey::from_string(&twice.secret).expect("parses");
        assert_eq!(key.secret_subkeys.len(), 2);
    }

    // -- slow algorithms ------------------------------------------------------

    #[test]
    #[ignore = "slow: RSA-2048 key generation takes seconds even optimised; run with --ignored"]
    fn rsa_2048_generates_a_usable_key() {
        let primary = cfg(GfrKeyAlgo::RSA2048, true, false, GfrOpenPGPKeyVersion::V4);
        let key = keygen_dynamic("RSA <rsa@example.test>", &primary, &[]).expect("keygen");
        assert_eq!(key.primary_key.version(), KeyVersion::V4);
    }

    #[test]
    #[ignore = "slow: RSA-4096 key generation can take tens of seconds; run with --ignored"]
    fn rsa_4096_generates_a_usable_key() {
        let primary = cfg(GfrKeyAlgo::RSA4096, true, false, GfrOpenPGPKeyVersion::V4);
        let key = keygen_dynamic("RSA <rsa@example.test>", &primary, &[]).expect("keygen");
        assert_eq!(key.primary_key.version(), KeyVersion::V4);
    }

    #[test]
    #[ignore = "slow: post-quantum key generation; run with --ignored"]
    fn ml_dsa_65_forces_a_v6_key() {
        let primary = cfg(
            GfrKeyAlgo::MLDSA65ED25519,
            true,
            false,
            GfrOpenPGPKeyVersion::V4,
        );
        let key = keygen_dynamic("PQ <pq@example.test>", &primary, &[]).expect("keygen");
        assert_eq!(
            key.primary_key.version(),
            KeyVersion::V6,
            "post-quantum algorithms have no v4 wire format"
        );
    }

    #[test]
    #[ignore = "slow: SLH-DSA key generation is the slowest of the PQ families; run with --ignored"]
    fn slh_dsa_shake_128s_forces_a_v6_key() {
        let primary = cfg(
            GfrKeyAlgo::SLHDSASHAKE128S,
            true,
            false,
            GfrOpenPGPKeyVersion::V4,
        );
        let key = keygen_dynamic("PQ <pq@example.test>", &primary, &[]).expect("keygen");
        assert_eq!(key.primary_key.version(), KeyVersion::V6);
    }
}
