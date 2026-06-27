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

//! Shared utility functions and types used across the crypto and key modules.
//!
//! Covers: passphrase fetching (with and without cache), algorithm resolution,
//! key-version selection, signature helpers, and revocation subpacket building.

use pgp::{
    bytes::Bytes,
    composed::{DsaKeySize, KeyType},
    crypto::ecc_curve::ECCCurve,
    packet::{RevocationCode, Signature, Subpacket, SubpacketData},
    types::{Password, PublicParams, Timestamp},
};
use rsa::traits::PublicKeyParts;
use zeroize::Zeroizing;

use crate::{
    cache::{PasswordCache, PasswordCacheKey, PasswordCachePolicy},
    host::gfc_secure_free_cstr,
    types::{
        GfrKeyAlgo, GfrKeyConfig, GfrPassphraseState, GfrPasswordFetchCb, GfrRevocationCode,
        GfrStatus,
    },
};
use std::{
    ffi::{CString, c_char},
    ptr::null_mut,
};

/// Describes the passphrase request context passed to the UI callback.
pub struct PassphraseStateInternal {
    /// Fingerprint of the key being unlocked; empty for symmetric operations.
    pub fpr: String,
    /// Human-readable purpose shown in the dialog (e.g. "Decryption", "Signing").
    pub info: String,
    /// True when retrying after a wrong passphrase.
    pub retry: bool,
    /// True when the user should create a new passphrase rather than unlock an existing key.
    pub ask_for_new: bool,
    /// True when the user must type the passphrase twice to confirm (e.g. when setting).
    pub should_confirm: bool,
}

/// Invoke the C++ passphrase callback and return the passphrase as owned bytes.
///
/// The callback returns `ret > 0` (number of bytes) on success; `ret <= 0` or
/// a null pointer means the user cancelled or an error occurred. The C++ buffer
/// is copied into Rust memory before the free callback is called, so the order
/// is: copy → free.
pub fn fetch_password_internal(
    channel: i32,
    state: PassphraseStateInternal,
    fetch_cb: Option<GfrPasswordFetchCb>,
) -> Result<Zeroizing<Vec<u8>>, GfrStatus> {
    let Some(fetch_fn) = fetch_cb else {
        return Err(GfrStatus::ErrorInvalidInput); // Free callback is required if fetch callback is provided
    };

    let info_c = CString::new(state.info).unwrap_or_default();
    let fpr_c = CString::new(state.fpr.to_uppercase()).unwrap_or_default();

    let passphrase_state = GfrPassphraseState {
        fpr: fpr_c.as_ptr() as *mut c_char,
        info: info_c.as_ptr() as *mut c_char,
        retry: state.retry,
        ask_for_new: state.ask_for_new,
        should_confirm: state.should_confirm,
    };

    let mut pwd_ptr: *mut u8 = null_mut();

    // If a password fetch callback is provided, use it to get the password.
    // The callback returns the password length: `ret > 0` indicates success and
    // is the number of bytes written to `*out_pwd`; `ret <= 0` means the user
    // cancelled or an error occurred.
    let ret = fetch_fn(channel, passphrase_state, &mut pwd_ptr, null_mut());

    // Callback indicated failure or no password provided. Defensively free any
    // buffer the callback may still have allocated (e.g. a buggy callback that
    // returns a non-positive length but writes a pointer), so secret bytes are
    // never leaked unwiped on the error path.
    if ret <= 0 || pwd_ptr.is_null() {
        if !pwd_ptr.is_null() {
            unsafe { gfc_secure_free_cstr(pwd_ptr as *mut std::ffi::c_char) };
        }
        return Err(GfrStatus::ErrorFetchPasswordFailed);
    }

    // safely create a slice from the returned pointer and length
    let pwd_slice = unsafe { std::slice::from_raw_parts(pwd_ptr, ret as usize) };

    if pwd_slice.is_empty() {
        unsafe { gfc_secure_free_cstr(pwd_ptr as *mut std::ffi::c_char) };
        return Err(GfrStatus::ErrorInvalidInput); // Empty password provided
    }

    // 1. COPY THE DATA FIRST to Rust's owned Vec
    let mut password = Vec::with_capacity(ret as usize);
    password.extend_from_slice(pwd_slice);

    // 2. NOW FREE THE MEMORY in C++
    unsafe { gfc_secure_free_cstr(pwd_ptr as *mut std::ffi::c_char) };

    log::debug!("Fetched password via callback");
    Ok(Zeroizing::new(password))
}

/// Fetch a passphrase, consulting the in-memory cache according to `policy`.
///
/// If `fpr` is empty the cache is automatically bypassed regardless of policy,
/// because there is no key to look up by.
pub fn fetch_password_with_cache(
    cache: Option<&PasswordCache>,
    policy: PasswordCachePolicy,
    channel: i32,
    state: PassphraseStateInternal,
    fetch_cb: Option<GfrPasswordFetchCb>,
) -> Result<Zeroizing<Vec<u8>>, GfrStatus> {
    let fpr = state.fpr.to_uppercase();
    let key = PasswordCacheKey {
        channel,
        fpr: fpr.to_string(),
        info: state.info.to_string().to_uppercase(),
    };

    let mut policy = policy;
    if fpr.is_empty() {
        policy = PasswordCachePolicy::Bypass; // If no FPR, we cannot cache, so bypass cache
    }

    match policy {
        PasswordCachePolicy::Default => {
            if let Some(cache) = cache {
                if let Some(pwd) = cache.get(&key) {
                    log::debug!("Password cache hit");
                    return Ok(Zeroizing::new(pwd));
                }
            }

            let pwd = fetch_password_internal(channel, state, fetch_cb)?;

            if let Some(cache) = cache {
                cache.put(key, pwd.to_vec());
            }

            Ok(pwd)
        }

        PasswordCachePolicy::Bypass => fetch_password_internal(channel, state, fetch_cb),

        PasswordCachePolicy::Refresh => {
            if let Some(cache) = cache {
                cache.remove(&key);
            }

            let pwd = fetch_password_internal(channel, state, fetch_cb)?;

            if let Some(cache) = cache {
                cache.put(key, pwd.to_vec());
            }

            Ok(pwd)
        }
    }
}

/// Map a `GfrKeyAlgo` and encryption intent to the rPGP `KeyType`.
///
/// For Curve25519-family algorithms, `can_encrypt` selects the curve variant:
/// `false` → Ed25519 (signing), `true` → ECDH Curve25519 (encryption).
/// Both `ED25519` and `CV25519` map to the same underlying curves — the
/// distinction is only in the intended usage expressed by `can_encrypt`.
pub fn resolve_key_type(algo: &GfrKeyAlgo, can_encrypt: bool) -> Result<KeyType, GfrStatus> {
    match algo {
        GfrKeyAlgo::ED25519 | GfrKeyAlgo::CV25519 => {
            if can_encrypt {
                Ok(KeyType::ECDH(ECCCurve::Curve25519Legacy))
            } else {
                Ok(KeyType::Ed25519)
            }
        }

        GfrKeyAlgo::NISTP256 => {
            if can_encrypt {
                Ok(KeyType::ECDH(ECCCurve::P256))
            } else {
                Ok(KeyType::ECDSA(ECCCurve::P256))
            }
        }
        GfrKeyAlgo::NISTP384 => {
            if can_encrypt {
                Ok(KeyType::ECDH(ECCCurve::P384))
            } else {
                Ok(KeyType::ECDSA(ECCCurve::P384))
            }
        }
        GfrKeyAlgo::NISTP521 => {
            if can_encrypt {
                Ok(KeyType::ECDH(ECCCurve::P521))
            } else {
                Ok(KeyType::ECDSA(ECCCurve::P521))
            }
        }
        GfrKeyAlgo::BRAINPOOLP256 => {
            if can_encrypt {
                Ok(KeyType::ECDH(ECCCurve::BrainpoolP256r1))
            } else {
                Ok(KeyType::ECDSA(ECCCurve::BrainpoolP256r1))
            }
        }
        GfrKeyAlgo::BRAINPOOLP384 => {
            if can_encrypt {
                Ok(KeyType::ECDH(ECCCurve::BrainpoolP384r1))
            } else {
                Ok(KeyType::ECDSA(ECCCurve::BrainpoolP384r1))
            }
        }
        GfrKeyAlgo::BRAINPOOLP512 => {
            if can_encrypt {
                Ok(KeyType::ECDH(ECCCurve::BrainpoolP512r1))
            } else {
                Ok(KeyType::ECDSA(ECCCurve::BrainpoolP512r1))
            }
        }
        GfrKeyAlgo::ED448 | GfrKeyAlgo::X448 => {
            if can_encrypt {
                Ok(KeyType::X448)
            } else {
                Ok(KeyType::Ed448)
            }
        }
        GfrKeyAlgo::SECP256K1 => {
            if can_encrypt {
                Err(GfrStatus::ErrorUnsupportedAlgorithm)
            } else {
                Ok(KeyType::ECDSA(ECCCurve::Secp256k1))
            }
        }

        GfrKeyAlgo::RSA1024 => Ok(KeyType::Rsa(1024)),
        GfrKeyAlgo::RSA2048 => Ok(KeyType::Rsa(2048)),
        GfrKeyAlgo::RSA3072 => Ok(KeyType::Rsa(3072)),
        GfrKeyAlgo::RSA4096 => Ok(KeyType::Rsa(4096)),

        GfrKeyAlgo::DSA1024 => Ok(KeyType::Dsa(DsaKeySize::B1024)),
        GfrKeyAlgo::DSA2048 => Ok(KeyType::Dsa(DsaKeySize::B2048)),
        GfrKeyAlgo::DSA3072 => Ok(KeyType::Dsa(DsaKeySize::B3072)),

        GfrKeyAlgo::ED25519LEGACY => Ok(KeyType::Ed25519Legacy),

        GfrKeyAlgo::KYBER768X25519 => Ok(KeyType::MlKem768X25519),
        GfrKeyAlgo::KYBER1024X448 => Ok(KeyType::MlKem1024X448),
        GfrKeyAlgo::MLDSA65ED25519 => Ok(KeyType::MlDsa65Ed25519),
        GfrKeyAlgo::MLDSA87ED448 => Ok(KeyType::MlDsa87Ed448),
        GfrKeyAlgo::SLHDSASHAKE128S => Ok(KeyType::SlhDsaShake128s),
        GfrKeyAlgo::SLHDSASHAKE128F => Ok(KeyType::SlhDsaShake128f),
        GfrKeyAlgo::SLHDSASHAKE256S => Ok(KeyType::SlhDsaShake256s),

        GfrKeyAlgo::Unknown => Err(GfrStatus::ErrorUnsupportedAlgorithm),
    }
}

/// Reverse-map rPGP public-key parameters to a `GfrKeyAlgo` variant.
///
/// RSA and DSA bit-lengths are bucketed to the nearest standard size.
pub fn determine_algo(public_params: &PublicParams) -> GfrKeyAlgo {
    match public_params {
        PublicParams::RSA(p) => {
            let bits = p.key.n().bits() as u32;
            if bits >= 4096 {
                GfrKeyAlgo::RSA4096
            } else if bits >= 3072 {
                GfrKeyAlgo::RSA3072
            } else if bits >= 2048 {
                GfrKeyAlgo::RSA2048
            } else {
                GfrKeyAlgo::RSA1024
            }
        }
        PublicParams::DSA(p) => {
            let bits = p.key.components().p().bits() as u32;
            if bits >= 3072 {
                GfrKeyAlgo::DSA3072
            } else if bits >= 2048 {
                GfrKeyAlgo::DSA2048
            } else {
                GfrKeyAlgo::DSA1024
            }
        }
        PublicParams::Ed25519(_) => GfrKeyAlgo::ED25519,
        PublicParams::X25519(_) => GfrKeyAlgo::CV25519,
        PublicParams::Ed448(_) => GfrKeyAlgo::ED448,
        PublicParams::X448(_) => GfrKeyAlgo::X448,
        PublicParams::ECDH(p) => match p.curve() {
            ECCCurve::Curve25519Legacy => GfrKeyAlgo::CV25519,
            ECCCurve::P256 => GfrKeyAlgo::NISTP256,
            ECCCurve::P384 => GfrKeyAlgo::NISTP384,
            ECCCurve::P521 => GfrKeyAlgo::NISTP521,
            ECCCurve::Secp256k1 => GfrKeyAlgo::SECP256K1,
            _ => GfrKeyAlgo::Unknown,
        },
        PublicParams::ECDSA(p) => match p.curve() {
            ECCCurve::P256 => GfrKeyAlgo::NISTP256,
            ECCCurve::P384 => GfrKeyAlgo::NISTP384,
            ECCCurve::P521 => GfrKeyAlgo::NISTP521,
            ECCCurve::Secp256k1 => GfrKeyAlgo::SECP256K1,
            _ => GfrKeyAlgo::Unknown,
        },
        PublicParams::EdDSALegacy(_) => GfrKeyAlgo::ED25519LEGACY,
        PublicParams::MlKem768X25519(_) => GfrKeyAlgo::KYBER768X25519,
        PublicParams::MlKem1024X448(_) => GfrKeyAlgo::KYBER1024X448,
        PublicParams::MlDsa65Ed25519(_) => GfrKeyAlgo::MLDSA65ED25519,
        PublicParams::MlDsa87Ed448(_) => GfrKeyAlgo::MLDSA87ED448,
        PublicParams::SlhDsaShake128s(_) => GfrKeyAlgo::SLHDSASHAKE128S,
        PublicParams::SlhDsaShake128f(_) => GfrKeyAlgo::SLHDSASHAKE128F,
        PublicParams::SlhDsaShake256s(_) => GfrKeyAlgo::SLHDSASHAKE256S,
        _ => GfrKeyAlgo::Unknown, // Fallback
    }
}

pub fn extract_key_length(public_params: &PublicParams) -> Option<u32> {
    match public_params {
        PublicParams::RSA(p) => Some(p.key.n().bits() as u32),
        PublicParams::DSA(p) => Some(p.key.components().p().bits() as u32),

        PublicParams::Ed25519(_) => Some(255),
        PublicParams::Ed448(_) => Some(448),
        PublicParams::X448(_) => Some(448),

        PublicParams::ECDH(p) => Some(p.curve().nbits() as u32),
        PublicParams::ECDSA(p) => Some(p.curve().nbits() as u32),

        PublicParams::EdDSALegacy(_) => Some(255),
        PublicParams::X25519(_) => Some(255),

        PublicParams::MlKem768X25519(_) => Some(768),
        PublicParams::MlKem1024X448(_) => Some(1024),

        PublicParams::MlDsa65Ed25519(_) => Some(65),
        PublicParams::MlDsa87Ed448(_) => Some(87),

        PublicParams::SlhDsaShake128s(_) => Some(128),
        PublicParams::SlhDsaShake128f(_) => Some(128),
        PublicParams::SlhDsaShake256s(_) => Some(256),

        _ => None,
    }
}

/// Return true if `algo` is a post-quantum hybrid algorithm.
///
/// PQC algorithms require OpenPGP v6 keys; this is checked during key generation
/// to enforce the correct version byte in the generated packet.
pub fn check_if_quantum_hybrid_algo(algo: &GfrKeyAlgo) -> bool {
    matches!(
        algo,
        GfrKeyAlgo::KYBER768X25519
            | GfrKeyAlgo::KYBER1024X448
            | GfrKeyAlgo::MLDSA65ED25519
            | GfrKeyAlgo::MLDSA87ED448
            | GfrKeyAlgo::SLHDSASHAKE128S
            | GfrKeyAlgo::SLHDSASHAKE128F
            | GfrKeyAlgo::SLHDSASHAKE256S
    )
}

/// Return true when any key in the request requires OpenPGP v6 format.
pub fn check_if_should_use_key_ver_v6(
    primary_algo: &GfrKeyConfig,
    sub_algos: &[GfrKeyConfig],
) -> bool {
    // If the primary key is a post-quantum hybrid, we must use V6 keys to ensure correct version byte
    if check_if_quantum_hybrid_algo(&primary_algo.algo) {
        return true;
    }

    // If any subkey is a post-quantum hybrid, we must also use V6 keys
    for sub in sub_algos {
        if check_if_quantum_hybrid_algo(&sub.algo) {
            return true;
        }
    }

    false
}

pub fn is_self_signature_from_primary(sig: &Signature, primary_fpr_bytes: &[u8]) -> bool {
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

pub fn has_is_primary_true(sig: &Signature) -> bool {
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

/// Pick the best self-signature to use as a template when re-signing a user ID.
///
/// Prefers the most-recent signature that carries a `KeyFlags` subpacket, so
/// the re-generated signature preserves the original capability flags. Falls
/// back to the most-recent signature without key flags when none have them.
pub fn choose_template_self_sig<'a>(self_sigs: &[&'a Signature]) -> Option<&'a Signature> {
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

pub fn build_revocation_reason_subpacket(
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

pub fn password_from_zeroizing_bytes(bytes: Zeroizing<Vec<u8>>) -> Password {
    Password::from(move || Zeroizing::new(bytes.as_slice().to_vec()))
}
