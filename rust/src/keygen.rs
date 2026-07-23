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
        PassphraseStateInternal, armor_opts, check_if_should_use_key_ver_v6, fetch_password_with_cache,
        password_from_zeroizing_bytes, resolve_key_type,
    },
};
use log::error;
use pgp::{
    composed::{
        Deserializable, EncryptionCaps, KeyType, SecretKeyParamsBuilder, SignedPublicKey,
        SignedSecretKey, SignedSecretSubKey, SubkeyParamsBuilder,
    },
    crypto::{
        aead::AeadAlgorithm, hash::HashAlgorithm, sym::SymmetricKeyAlgorithm,
    },
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

/// True if `algo` maps to a deprecated OID that RFC 9580 §9.2 forbids in v6 keys
/// ("Implementations MUST NOT accept or generate version 6 key material using the
/// deprecated OIDs").
fn is_v6_forbidden_legacy(algo: &GfrKeyAlgo) -> bool {
    matches!(algo, GfrKeyAlgo::ED25519LEGACY)
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
        if is_v6_forbidden_legacy(&key_config.algo) {
            anyhow::bail!(
                "ED25519LEGACY uses a deprecated OID and cannot be used in a v6 key (RFC 9580 §9.2)"
            );
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
            if is_v6_forbidden_legacy(&config.algo) {
                anyhow::bail!(
                    "ED25519LEGACY uses a deprecated OID and cannot be used in a v6 key (RFC 9580 §9.2)"
                );
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
            [
                SymmetricKeyAlgorithm::AES256,
                SymmetricKeyAlgorithm::AES128,
            ]
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
        if is_v6_forbidden_legacy(&config.algo) {
            set_last_error(
                "ED25519LEGACY uses a deprecated OID and cannot be added to a v6 key (RFC 9580 §9.2)",
            );
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
    use crate::types::{GfrKeyAlgo, GfrKeyConfig, GfrOpenPGPKeyVersion};
    use pgp::crypto::public_key::PublicKeyAlgorithm;

    fn cfg(
        algo: GfrKeyAlgo,
        can_sign: bool,
        can_encrypt: bool,
        ver: GfrOpenPGPKeyVersion,
    ) -> GfrKeyConfig {
        GfrKeyConfig {
            algo,
            can_sign,
            can_encrypt,
            can_auth: false,
            has_passphrase: false,
            ver,
            expiration_epoch_secs: 0,
        }
    }

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
        let primary = cfg(GfrKeyAlgo::ED25519LEGACY, true, false, GfrOpenPGPKeyVersion::V6);
        assert!(keygen_dynamic("rfc9580 <rfc@example.com>", &primary, &[]).is_err());
    }

    /// RFC 9580 §10.1.5: an encryption-only primary algorithm MUST be rejected.
    #[test]
    fn encryption_only_primary_rejected() {
        // ML-KEM is a KEM (encryption-only) and cannot self-certify as a primary.
        let primary = cfg(GfrKeyAlgo::KYBER768X25519, false, true, GfrOpenPGPKeyVersion::V6);
        assert!(keygen_dynamic("rfc9580 <rfc@example.com>", &primary, &[]).is_err());
    }

    /// RFC 9580 §5.2.3.14: generated keys SHOULD advertise algorithm preferences.
    #[test]
    fn generated_key_advertises_symmetric_preferences() {
        let primary = cfg(GfrKeyAlgo::ED25519, true, false, GfrOpenPGPKeyVersion::V4);
        let key = keygen_dynamic("rfc9580 <rfc@example.com>", &primary, &[]).expect("keygen");
        let advertises_aes256 = key.details.users.iter().any(|u| {
            u.signatures
                .iter()
                .any(|s| s.preferred_symmetric_algs().contains(&SymmetricKeyAlgorithm::AES256))
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
            assert!(!is_crc, "v6 key armor must not contain a CRC24 footer: {line}");
        }
    }
}
