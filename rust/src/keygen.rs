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
    err::IntoGfrResult,
    types::{GfrKeyAlgo, GfrKeyConfig, GfrPasswordFetchCb, GfrStatus},
    utils::{
        PassphraseStateInternal, check_if_should_use_key_ver_v6, fetch_password_with_cache,
        password_from_zeroizing_bytes, resolve_key_type,
    },
};
use log::error;
use pgp::{
    composed::{
        ArmorOptions, Deserializable, EncryptionCaps, KeyType, SecretKeyParamsBuilder,
        SignedPublicKey, SignedSecretKey, SignedSecretSubKey, SubkeyParamsBuilder,
    },
    packet::{KeyFlags, PubKeyInner, PublicSubkey, SecretSubkey},
    types::{KeyDetails, KeyVersion, Password, Timestamp},
};
use rand::thread_rng;
use zeroize::Zeroizing;

/// Armored key material produced by a generation or modification operation.
pub struct GeneratedKeys {
    pub secret: String,
    pub public: String,
    pub fingerprint: String,
}

/// Build a `SignedSecretKey` from a primary key config and a list of subkey configs.
///
/// Forces OpenPGP v6 format when any key uses a post-quantum hybrid algorithm.
/// For v6 keys, `CV25519` is mapped to `X25519` and `ED25519` to `Ed25519` per
/// the v6 key type naming convention in the rPGP crate.
pub fn keygen_dynamic(
    uid: &str,
    key_config: &GfrKeyConfig,
    s_key_configs: &[GfrKeyConfig],
) -> anyhow::Result<SignedSecretKey> {
    let primary_type = resolve_key_type(&key_config.algo, false)?;
    let mut subkeys = Vec::new();

    let use_v6 = check_if_should_use_key_ver_v6(key_config, s_key_configs);
    if use_v6 {
        log::info!(
            "Using V6 key version for generation due to presence of post-quantum hybrid algorithm."
        );
    }

    for config in s_key_configs {
        let k_type = resolve_key_type(&config.algo, config.can_encrypt)?;
        let mut builder = SubkeyParamsBuilder::default();
        builder.key_type(k_type);

        if use_v6 {
            // For v6 keys, we need to set the version explicitly to V6 in the builder
            builder.version(KeyVersion::V6);

            // For certain algorithms like CV25519, we also need to set the key
            // type to X25519 for encryption capabilities in v6 keys
            if config.algo == GfrKeyAlgo::CV25519 {
                builder.key_type(KeyType::X25519);
            }
        }

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
    }

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
        .to_armored_string(ArmorOptions::default())
        .map_err(|_| GfrStatus::ErrorArmorFailed)?;

    let public_key = SignedPublicKey::from(secret_key);
    let armored_p_key = public_key
        .to_armored_string(ArmorOptions::default())
        .map_err(|_| GfrStatus::ErrorArmorFailed)?;

    Ok(GeneratedKeys {
        secret: armored_s_key,
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
        // For v6 keys, we need to set the version explicitly to V6 in the public key inner
        // and also ensure the key type is set correctly for certain algorithms like CV25519
        if config.algo == GfrKeyAlgo::CV25519 {
            sub_k_type = KeyType::X25519;
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

    // 9. Armor the updated keys for export
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
        fingerprint: fingerprint_str,
    })
}
