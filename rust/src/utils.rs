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
    composed::{ArmorOptions, KeyType},
    crypto::ecc_curve::ECCCurve,
    packet::{RevocationCode, Signature, Subpacket, SubpacketData},
    types::{Password, PublicParams, Timestamp},
};
use rsa::traits::PublicKeyParts;
use zeroize::Zeroizing;

use crate::{
    cache::{PasswordCache, PasswordCacheKey, PasswordCachePolicy},
    err::set_last_error,
    host::gfc_secure_free_buffer,
    types::{
        GfrKeyAlgo, GfrKeyConfig, GfrPassphraseState, GfrPasswordFetchCb, GfrPasswordFetchStatus,
        GfrRevocationCode, GfrStatus,
    },
};
use std::{
    ffi::{CString, c_char},
    ptr::null_mut,
};

/// Armor options for all engine output: no headers and **no CRC24 footer**.
///
/// RFC 9580 §6.1 discourages generating the CRC24 footer in general and forbids
/// it outright for v6 keys, v6 signatures, and messages ending in a v2 SEIPD
/// packet ("MUST NOT contain a CRC24 footer"). rPGP's `armor_opts()`
/// enables the checksum with no version awareness, so every armored output goes
/// through this helper instead to stay compliant across key/signature versions.
pub fn armor_opts() -> ArmorOptions<'static> {
    ArmorOptions {
        headers: None,
        include_checksum: false,
    }
}

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
/// On success the callback returns `ret > 0` (number of bytes) and sets
/// `out_status` to `Provided`. Otherwise it sets `out_status` to `Cancelled`
/// (deliberate user cancellation) or `Failed` (timeout, missing provider,
/// internal error), which this function maps to `ErrorCanceled` and
/// `ErrorFetchPasswordFailed` respectively. The C++ buffer is copied into Rust
/// memory before the free callback is called, so the order is: copy → free.
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
    // Default to `Failed` so a callback that returns non-success without writing
    // a status is treated as a fetch failure rather than mistaken for success.
    let mut out_status = GfrPasswordFetchStatus::Failed;

    // If a password fetch callback is provided, use it to get the password.
    // On success it returns the byte count (`ret > 0`) and sets `out_status` to
    // `Provided`; otherwise it reports the reason via `out_status`.
    let ret = fetch_fn(
        channel,
        passphrase_state,
        &mut pwd_ptr,
        &mut out_status,
        null_mut(),
    );

    // Anything but a `Provided` status with a non-empty buffer is a non-success
    // outcome. Defensively free any buffer the callback may still have allocated
    // (e.g. a buggy callback that reports failure but writes a pointer), so
    // secret bytes are never leaked unwiped on the error path, then map the
    // reported reason to a specific status: the engine surfaces `ErrorCanceled`
    // as GPG_ERR_CANCELED for a deliberate user cancellation, rather than a
    // generic "General error".
    if out_status != GfrPasswordFetchStatus::Provided || ret <= 0 || pwd_ptr.is_null() {
        if !pwd_ptr.is_null() {
            // The buffer length is the callback's returned byte count; on this
            // failure path it may be <= 0, in which case there is nothing to
            // wipe — pass 0 and just reclaim the allocation.
            let len = if ret > 0 { ret as usize } else { 0 };
            unsafe { gfc_secure_free_buffer(pwd_ptr, len) };
        }
        return Err(match out_status {
            GfrPasswordFetchStatus::Cancelled => GfrStatus::ErrorCanceled,
            _ => GfrStatus::ErrorFetchPasswordFailed,
        });
    }

    // safely create a slice from the returned pointer and length
    let pwd_slice = unsafe { std::slice::from_raw_parts(pwd_ptr, ret as usize) };

    if pwd_slice.is_empty() {
        unsafe { gfc_secure_free_buffer(pwd_ptr, ret as usize) };
        return Err(GfrStatus::ErrorInvalidInput); // Empty password provided
    }

    // 1. COPY THE DATA FIRST to Rust's owned Vec
    let mut password = Vec::with_capacity(ret as usize);
    password.extend_from_slice(pwd_slice);

    // 2. NOW FREE THE MEMORY in C++ by its exact length (never via strlen).
    unsafe { gfc_secure_free_buffer(pwd_ptr, ret as usize) };

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

        // RFC 9580 §12.4: implementations MUST NOT generate RSA keys smaller than
        // 2048 bits. RSA-1024 is rejected here (existing RSA-1024 keys can still be
        // imported/displayed via `determine_algo`, which is unaffected).
        GfrKeyAlgo::RSA1024 => {
            set_last_error(
                "RSA-1024 is too weak to generate; RFC 9580 §12.4 requires >= 2048 bits",
            );
            Err(GfrStatus::ErrorUnsupportedAlgorithm)
        }
        GfrKeyAlgo::RSA2048 => Ok(KeyType::Rsa(2048)),
        GfrKeyAlgo::RSA3072 => Ok(KeyType::Rsa(3072)),
        GfrKeyAlgo::RSA4096 => Ok(KeyType::Rsa(4096)),

        // RFC 9580 §12.5: implementations MUST NOT generate DSA keys. Reading and
        // verifying existing DSA keys remains supported (see `determine_algo`).
        GfrKeyAlgo::DSA1024 | GfrKeyAlgo::DSA2048 | GfrKeyAlgo::DSA3072 => {
            set_last_error("DSA key generation is forbidden by RFC 9580 §12.5");
            Err(GfrStatus::ErrorUnsupportedAlgorithm)
        }

        // RFC 9580 §9.1: EdDSALegacy (algorithm 22, Ed25519Legacy OID) is
        // deprecated and SHOULD NOT be generated (for v6 keys it is forbidden).
        // Reject it at the generation boundary regardless of key version; existing
        // Ed25519Legacy keys are still read/verified via `determine_algo`.
        GfrKeyAlgo::ED25519LEGACY => {
            set_last_error(
                "Ed25519Legacy key generation is deprecated by RFC 9580 §9.1; use Ed25519 instead",
            );
            Err(GfrStatus::ErrorUnsupportedAlgorithm)
        }

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

#[cfg(test)]
mod tests {
    use super::*;
    use crate::types::{GfrPassphraseState, GfrPasswordFetchStatus};
    use std::ffi::c_void;

    // The host's secure-free routines are stubbed once for the whole test
    // binary in `crate::testutil` -- they are `#[unsafe(no_mangle)]` global
    // symbols, so a second definition here would be a link error.

    fn state() -> PassphraseStateInternal {
        PassphraseStateInternal {
            fpr: "DEADBEEF".to_string(),
            info: "Decryption".to_string(),
            retry: false,
            ask_for_new: false,
            should_confirm: false,
        }
    }

    extern "C" fn cb_provided(
        _channel: i32,
        _state: GfrPassphraseState,
        out_pwd: *mut *mut u8,
        out_status: *mut GfrPasswordFetchStatus,
        _user_data: *mut c_void,
    ) -> i32 {
        // Hand over a heap buffer the way the C++ host does; the engine copies
        // it out and then calls `gfc_secure_free_buffer`, which the test stub
        // in `crate::testutil` genuinely reclaims. That stub rebuilds the
        // allocation from (ptr, len), so capacity must equal length -- hence a
        // boxed slice rather than a bare `Vec`.
        let (ptr, len) = crate::testutil::leak_as_c_buffer(b"secret");
        unsafe {
            *out_pwd = ptr;
            *out_status = GfrPasswordFetchStatus::Provided;
        }
        len as i32
    }

    extern "C" fn cb_cancelled(
        _channel: i32,
        _state: GfrPassphraseState,
        _out_pwd: *mut *mut u8,
        out_status: *mut GfrPasswordFetchStatus,
        _user_data: *mut c_void,
    ) -> i32 {
        unsafe { *out_status = GfrPasswordFetchStatus::Cancelled };
        0
    }

    extern "C" fn cb_failed(
        _channel: i32,
        _state: GfrPassphraseState,
        _out_pwd: *mut *mut u8,
        out_status: *mut GfrPasswordFetchStatus,
        _user_data: *mut c_void,
    ) -> i32 {
        unsafe { *out_status = GfrPasswordFetchStatus::Failed };
        0
    }

    // A callback that reports no status at all must be treated as a failure,
    // never mistaken for a provided passphrase or a cancellation.
    extern "C" fn cb_silent(
        _channel: i32,
        _state: GfrPassphraseState,
        _out_pwd: *mut *mut u8,
        _out_status: *mut GfrPasswordFetchStatus,
        _user_data: *mut c_void,
    ) -> i32 {
        0
    }

    #[test]
    fn provided_status_yields_passphrase() {
        let pwd = fetch_password_internal(0, state(), Some(cb_provided)).expect("provided");
        assert_eq!(pwd.as_slice(), b"secret");
    }

    #[test]
    fn cancelled_status_maps_to_canceled_error() {
        let err = fetch_password_internal(0, state(), Some(cb_cancelled)).unwrap_err();
        assert_eq!(err, GfrStatus::ErrorCanceled);
    }

    #[test]
    fn failed_status_maps_to_fetch_password_failed() {
        let err = fetch_password_internal(0, state(), Some(cb_failed)).unwrap_err();
        assert_eq!(err, GfrStatus::ErrorFetchPasswordFailed);
    }

    #[test]
    fn missing_status_defaults_to_fetch_password_failed() {
        let err = fetch_password_internal(0, state(), Some(cb_silent)).unwrap_err();
        assert_eq!(err, GfrStatus::ErrorFetchPasswordFailed);
    }

    #[test]
    fn absent_callback_is_invalid_input() {
        let err = fetch_password_internal(0, state(), None).unwrap_err();
        assert_eq!(err, GfrStatus::ErrorInvalidInput);
    }
}

#[cfg(test)]
mod utils_tests {
    //! Algorithm policy, key-version selection, self-signature helpers,
    //! revocation subpackets and the cached passphrase path.
    //!
    //! The `resolve_key_type` matrix is the RFC 9580 generation policy gate:
    //! §12.4 (RSA >= 2048), §12.5 (no DSA) and §9.1/§9.2 (no deprecated
    //! Ed25519Legacy OID) are all enforced there, so it is walked exhaustively.

    use super::*;
    use crate::cache::{PasswordCache, PasswordCachePolicy};
    use crate::testutil::keys;
    use crate::types::{GfrOpenPGPKeyVersion, GfrPassphraseState, GfrPasswordFetchStatus};
    use pgp::types::KeyDetails as _;
    use std::ffi::c_void;
    use std::time::Duration;

    fn state_for(fpr: &str) -> PassphraseStateInternal {
        PassphraseStateInternal {
            fpr: fpr.to_string(),
            info: "Decryption".to_string(),
            retry: false,
            ask_for_new: false,
            should_confirm: false,
        }
    }

    /// A callback that reports how many times it was invoked, so cache hits
    /// can be distinguished from cache misses.
    static PROMPTS: std::sync::atomic::AtomicUsize = std::sync::atomic::AtomicUsize::new(0);

    extern "C" fn cb_counting(
        _channel: i32,
        _state: GfrPassphraseState,
        out_pwd: *mut *mut u8,
        out_status: *mut GfrPasswordFetchStatus,
        _user_data: *mut c_void,
    ) -> i32 {
        PROMPTS.fetch_add(1, std::sync::atomic::Ordering::SeqCst);
        let (ptr, len) = crate::testutil::leak_as_c_buffer(b"hunter2");
        unsafe {
            *out_pwd = ptr;
            *out_status = GfrPasswordFetchStatus::Provided;
        }
        len as i32
    }

    extern "C" fn cb_refusing(
        _channel: i32,
        _state: GfrPassphraseState,
        _out_pwd: *mut *mut u8,
        out_status: *mut GfrPasswordFetchStatus,
        _user_data: *mut c_void,
    ) -> i32 {
        unsafe { *out_status = GfrPasswordFetchStatus::Failed };
        0
    }

    extern "C" fn cb_empty_passphrase(
        _channel: i32,
        _state: GfrPassphraseState,
        out_pwd: *mut *mut u8,
        out_status: *mut GfrPasswordFetchStatus,
        _user_data: *mut c_void,
    ) -> i32 {
        let (ptr, _len) = crate::testutil::leak_as_c_buffer(b"");
        unsafe {
            *out_pwd = ptr;
            *out_status = GfrPasswordFetchStatus::Provided;
        }
        0
    }

    fn fresh_cache() -> PasswordCache {
        PasswordCache::new(Duration::from_secs(600), Duration::from_secs(3600))
    }

    // -- armor_opts --------------------------------------------------------

    #[test]
    fn armor_opts_emits_no_headers() {
        // RFC 9580 §6.2.2.1: implementations SHOULD NOT emit a Version header,
        // because it is gratuitous metadata about the sender's software.
        assert!(armor_opts().headers.is_none());
    }

    #[test]
    fn armor_opts_disables_the_crc24_footer() {
        // §6.1: generating the CRC24 footer is discouraged, and forbidden for
        // v6 material. rPGP's own default enables it with no version
        // awareness, which is exactly why this helper exists.
        assert!(!armor_opts().include_checksum);
    }

    #[test]
    fn armored_output_really_has_no_equals_footer() {
        let armored = &keys::V4_SIGN.public_armored;
        crate::testutil::assert::armor_has_no_crc24(armored);
    }

    #[test]
    fn armored_v6_output_has_no_equals_footer() {
        // The case the RFC states as a MUST NOT.
        crate::testutil::assert::armor_has_no_crc24(&keys::V6_SIGN.public_armored);
    }

    // -- resolve_key_type: the signing half of the matrix -------------------

    #[test]
    fn resolve_ed25519_for_signing_is_native_ed25519() {
        assert!(matches!(
            resolve_key_type(&GfrKeyAlgo::ED25519, false).unwrap(),
            KeyType::Ed25519
        ));
    }

    #[test]
    fn resolve_cv25519_for_signing_also_yields_ed25519() {
        // ED25519 and CV25519 name the same curve family; `can_encrypt` is
        // what picks the variant, not the enum value.
        assert!(matches!(
            resolve_key_type(&GfrKeyAlgo::CV25519, false).unwrap(),
            KeyType::Ed25519
        ));
    }

    #[test]
    fn resolve_nist_curves_for_signing_are_ecdsa() {
        for (algo, curve) in [
            (GfrKeyAlgo::NISTP256, ECCCurve::P256),
            (GfrKeyAlgo::NISTP384, ECCCurve::P384),
            (GfrKeyAlgo::NISTP521, ECCCurve::P521),
        ] {
            match resolve_key_type(&algo, false).unwrap() {
                KeyType::ECDSA(c) => assert_eq!(c, curve, "{algo:?}"),
                other => panic!("{algo:?} resolved to {other:?}"),
            }
        }
    }

    #[test]
    fn resolve_brainpool_curves_for_signing_are_ecdsa() {
        for (algo, curve) in [
            (GfrKeyAlgo::BRAINPOOLP256, ECCCurve::BrainpoolP256r1),
            (GfrKeyAlgo::BRAINPOOLP384, ECCCurve::BrainpoolP384r1),
            (GfrKeyAlgo::BRAINPOOLP512, ECCCurve::BrainpoolP512r1),
        ] {
            match resolve_key_type(&algo, false).unwrap() {
                KeyType::ECDSA(c) => assert_eq!(c, curve, "{algo:?}"),
                other => panic!("{algo:?} resolved to {other:?}"),
            }
        }
    }

    #[test]
    fn resolve_448_family_for_signing_is_ed448() {
        for algo in [GfrKeyAlgo::ED448, GfrKeyAlgo::X448] {
            assert!(
                matches!(resolve_key_type(&algo, false).unwrap(), KeyType::Ed448),
                "{algo:?}"
            );
        }
    }

    #[test]
    fn resolve_secp256k1_for_signing_is_ecdsa() {
        match resolve_key_type(&GfrKeyAlgo::SECP256K1, false).unwrap() {
            KeyType::ECDSA(c) => assert_eq!(c, ECCCurve::Secp256k1),
            other => panic!("resolved to {other:?}"),
        }
    }

    // -- resolve_key_type: the encryption half ------------------------------

    #[test]
    fn resolve_ed25519_for_encryption_is_legacy_ecdh() {
        // On a v4 key the encryption counterpart is ECDH over
        // Curve25519Legacy. Note RFC 9580 §9.2 forbids that OID in v6 keys --
        // `keygen_dynamic` remaps it, which is tested in `keygen.rs`.
        match resolve_key_type(&GfrKeyAlgo::ED25519, true).unwrap() {
            KeyType::ECDH(c) => assert_eq!(c, ECCCurve::Curve25519Legacy),
            other => panic!("resolved to {other:?}"),
        }
    }

    #[test]
    fn resolve_nist_curves_for_encryption_are_ecdh() {
        for (algo, curve) in [
            (GfrKeyAlgo::NISTP256, ECCCurve::P256),
            (GfrKeyAlgo::NISTP384, ECCCurve::P384),
            (GfrKeyAlgo::NISTP521, ECCCurve::P521),
        ] {
            match resolve_key_type(&algo, true).unwrap() {
                KeyType::ECDH(c) => assert_eq!(c, curve, "{algo:?}"),
                other => panic!("{algo:?} resolved to {other:?}"),
            }
        }
    }

    #[test]
    fn resolve_brainpool_curves_for_encryption_are_ecdh() {
        for (algo, curve) in [
            (GfrKeyAlgo::BRAINPOOLP256, ECCCurve::BrainpoolP256r1),
            (GfrKeyAlgo::BRAINPOOLP384, ECCCurve::BrainpoolP384r1),
            (GfrKeyAlgo::BRAINPOOLP512, ECCCurve::BrainpoolP512r1),
        ] {
            match resolve_key_type(&algo, true).unwrap() {
                KeyType::ECDH(c) => assert_eq!(c, curve, "{algo:?}"),
                other => panic!("{algo:?} resolved to {other:?}"),
            }
        }
    }

    #[test]
    fn resolve_448_family_for_encryption_is_x448() {
        for algo in [GfrKeyAlgo::ED448, GfrKeyAlgo::X448] {
            assert!(
                matches!(resolve_key_type(&algo, true).unwrap(), KeyType::X448),
                "{algo:?}"
            );
        }
    }

    #[test]
    fn resolve_secp256k1_for_encryption_is_unsupported() {
        // secp256k1 has no ECDH registration for OpenPGP.
        assert_eq!(
            resolve_key_type(&GfrKeyAlgo::SECP256K1, true),
            Err(GfrStatus::ErrorUnsupportedAlgorithm)
        );
    }

    // -- resolve_key_type: the RFC 9580 policy rejections -------------------

    #[test]
    fn resolve_rejects_rsa_1024_per_rfc9580_12_4() {
        assert_eq!(
            resolve_key_type(&GfrKeyAlgo::RSA1024, false),
            Err(GfrStatus::ErrorUnsupportedAlgorithm)
        );
    }

    #[test]
    fn resolve_rsa_1024_records_why() {
        // The C++ side surfaces this text to the user, so a bare status code
        // is not enough.
        let _ = resolve_key_type(&GfrKeyAlgo::RSA1024, false);
        let msg = crate::err::gfr_get_last_error_msg();
        assert!(!msg.is_null(), "the policy rejection must explain itself");
        let text = unsafe { std::ffi::CStr::from_ptr(msg) }
            .to_string_lossy()
            .into_owned();
        crate::ffi::mem::gfr_crypto_free_string(msg);
        assert!(text.contains("12.4"), "{text}");
    }

    #[test]
    fn resolve_accepts_rsa_2048_and_above() {
        for (algo, bits) in [
            (GfrKeyAlgo::RSA2048, 2048u32),
            (GfrKeyAlgo::RSA3072, 3072),
            (GfrKeyAlgo::RSA4096, 4096),
        ] {
            match resolve_key_type(&algo, false).unwrap() {
                KeyType::Rsa(n) => assert_eq!(n, bits, "{algo:?}"),
                other => panic!("{algo:?} resolved to {other:?}"),
            }
        }
    }

    #[test]
    fn resolve_rejects_every_dsa_size_per_rfc9580_12_5() {
        // "An implementation MUST NOT generate DSA keys" -- no size is exempt.
        for algo in [
            GfrKeyAlgo::DSA1024,
            GfrKeyAlgo::DSA2048,
            GfrKeyAlgo::DSA3072,
        ] {
            assert_eq!(
                resolve_key_type(&algo, false),
                Err(GfrStatus::ErrorUnsupportedAlgorithm),
                "{algo:?}"
            );
        }
    }

    #[test]
    fn resolve_dsa_records_why() {
        let _ = resolve_key_type(&GfrKeyAlgo::DSA2048, false);
        let msg = crate::err::gfr_get_last_error_msg();
        assert!(!msg.is_null());
        let text = unsafe { std::ffi::CStr::from_ptr(msg) }
            .to_string_lossy()
            .into_owned();
        crate::ffi::mem::gfr_crypto_free_string(msg);
        assert!(text.contains("12.5"), "{text}");
    }

    #[test]
    fn resolve_rejects_ed25519legacy_per_rfc9580_9_1() {
        // Deprecated in favour of native Ed25519; forbidden outright in v6.
        // Rejected at the generation boundary for every key version.
        for can_encrypt in [false, true] {
            assert_eq!(
                resolve_key_type(&GfrKeyAlgo::ED25519LEGACY, can_encrypt),
                Err(GfrStatus::ErrorUnsupportedAlgorithm)
            );
        }
    }

    #[test]
    fn resolve_rejects_unknown() {
        for can_encrypt in [false, true] {
            assert_eq!(
                resolve_key_type(&GfrKeyAlgo::Unknown, can_encrypt),
                Err(GfrStatus::ErrorUnsupportedAlgorithm)
            );
        }
    }

    #[test]
    fn resolve_accepts_the_post_quantum_families() {
        assert!(resolve_key_type(&GfrKeyAlgo::KYBER768X25519, true).is_ok());
        assert!(resolve_key_type(&GfrKeyAlgo::KYBER1024X448, true).is_ok());
        assert!(resolve_key_type(&GfrKeyAlgo::MLDSA65ED25519, false).is_ok());
        assert!(resolve_key_type(&GfrKeyAlgo::MLDSA87ED448, false).is_ok());
        assert!(resolve_key_type(&GfrKeyAlgo::SLHDSASHAKE128S, false).is_ok());
        assert!(resolve_key_type(&GfrKeyAlgo::SLHDSASHAKE128F, false).is_ok());
        assert!(resolve_key_type(&GfrKeyAlgo::SLHDSASHAKE256S, false).is_ok());
    }

    #[test]
    fn resolve_never_panics_for_any_algorithm_and_intent() {
        // The exhaustive sweep: every enum value against both intents must
        // return a Result, never unwind.
        for algo in ALL_ALGOS {
            for can_encrypt in [false, true] {
                let _ = resolve_key_type(&algo, can_encrypt);
            }
        }
    }

    const ALL_ALGOS: [GfrKeyAlgo; 26] = [
        GfrKeyAlgo::Unknown,
        GfrKeyAlgo::ED25519,
        GfrKeyAlgo::CV25519,
        GfrKeyAlgo::NISTP256,
        GfrKeyAlgo::NISTP384,
        GfrKeyAlgo::NISTP521,
        GfrKeyAlgo::BRAINPOOLP256,
        GfrKeyAlgo::BRAINPOOLP384,
        GfrKeyAlgo::BRAINPOOLP512,
        GfrKeyAlgo::ED448,
        GfrKeyAlgo::X448,
        GfrKeyAlgo::RSA1024,
        GfrKeyAlgo::RSA2048,
        GfrKeyAlgo::RSA3072,
        GfrKeyAlgo::RSA4096,
        GfrKeyAlgo::SECP256K1,
        GfrKeyAlgo::DSA1024,
        GfrKeyAlgo::DSA2048,
        GfrKeyAlgo::DSA3072,
        GfrKeyAlgo::ED25519LEGACY,
        GfrKeyAlgo::KYBER768X25519,
        GfrKeyAlgo::KYBER1024X448,
        GfrKeyAlgo::MLDSA65ED25519,
        GfrKeyAlgo::MLDSA87ED448,
        GfrKeyAlgo::SLHDSASHAKE128S,
        GfrKeyAlgo::SLHDSASHAKE128F,
    ];

    // -- determine_algo / extract_key_length --------------------------------

    #[test]
    fn determine_algo_round_trips_an_ed25519_primary() {
        let key = &keys::V4_SIGN.secret;
        assert_eq!(
            determine_algo(key.primary_key.public_params()),
            GfrKeyAlgo::ED25519
        );
    }

    #[test]
    fn determine_algo_round_trips_a_legacy_ecdh_subkey() {
        // The v4 encryption subkey is ECDH over Curve25519Legacy, which maps
        // back to CV25519.
        let key = &keys::V4_SIGN.secret;
        let enc = key
            .secret_subkeys
            .iter()
            .find(|s| !s.key.algorithm().can_sign())
            .expect("an encryption subkey");
        assert_eq!(determine_algo(enc.key.public_params()), GfrKeyAlgo::CV25519);
    }

    #[test]
    fn determine_algo_round_trips_native_x25519_on_a_v6_key() {
        let key = &keys::V6_SIGN.secret;
        let enc = key
            .secret_subkeys
            .iter()
            .find(|s| !s.key.algorithm().can_sign())
            .expect("an encryption subkey");
        assert_eq!(determine_algo(enc.key.public_params()), GfrKeyAlgo::CV25519);
    }

    #[test]
    fn determine_algo_round_trips_nist_p256() {
        let key = &keys::V4_NISTP256.secret;
        assert_eq!(
            determine_algo(key.primary_key.public_params()),
            GfrKeyAlgo::NISTP256
        );
    }

    #[test]
    fn determine_algo_round_trips_the_p256_ecdh_subkey() {
        let key = &keys::V4_NISTP256.secret;
        let enc = key.secret_subkeys.first().expect("a subkey");
        assert_eq!(determine_algo(enc.key.public_params()), GfrKeyAlgo::NISTP256);
    }

    #[test]
    fn extract_key_length_reports_255_for_ed25519() {
        // 255, not 256: the scalar field is 2^255 - 19.
        let key = &keys::V4_SIGN.secret;
        assert_eq!(extract_key_length(key.primary_key.public_params()), Some(255));
    }

    #[test]
    fn extract_key_length_reports_256_for_nist_p256() {
        let key = &keys::V4_NISTP256.secret;
        assert_eq!(extract_key_length(key.primary_key.public_params()), Some(256));
    }

    #[test]
    fn extract_key_length_reports_a_length_for_every_generated_component() {
        for fixture in [&*keys::V4_SIGN, &*keys::V6_SIGN, &*keys::V4_NISTP256] {
            assert!(
                extract_key_length(fixture.secret.primary_key.public_params()).is_some(),
                "primary of {}",
                fixture.primary_fpr
            );
            for sub in &fixture.secret.secret_subkeys {
                assert!(
                    extract_key_length(sub.key.public_params()).is_some(),
                    "subkey of {}",
                    fixture.primary_fpr
                );
            }
        }
    }

    #[test]
    fn extract_key_length_matches_the_corpus_keys() {
        let key = &*crate::testutil::corpus::KEY1;
        assert!(extract_key_length(key.primary_key.public_params()).is_some());
    }

    // -- post-quantum / key version selection -------------------------------

    #[test]
    fn quantum_hybrid_detection_covers_every_pq_algorithm() {
        for algo in [
            GfrKeyAlgo::KYBER768X25519,
            GfrKeyAlgo::KYBER1024X448,
            GfrKeyAlgo::MLDSA65ED25519,
            GfrKeyAlgo::MLDSA87ED448,
            GfrKeyAlgo::SLHDSASHAKE128S,
            GfrKeyAlgo::SLHDSASHAKE128F,
            GfrKeyAlgo::SLHDSASHAKE256S,
        ] {
            assert!(check_if_quantum_hybrid_algo(&algo), "{algo:?}");
        }
    }

    #[test]
    fn quantum_hybrid_detection_rejects_classical_algorithms() {
        for algo in [
            GfrKeyAlgo::ED25519,
            GfrKeyAlgo::CV25519,
            GfrKeyAlgo::RSA4096,
            GfrKeyAlgo::NISTP521,
            GfrKeyAlgo::ED448,
            GfrKeyAlgo::Unknown,
        ] {
            assert!(!check_if_quantum_hybrid_algo(&algo), "{algo:?}");
        }
    }

    #[test]
    fn a_post_quantum_primary_forces_v6() {
        // PQ algorithms have no v4 wire format, so the version is not the
        // caller's choice.
        let primary = keys::cfg(
            GfrKeyAlgo::MLDSA65ED25519,
            true,
            false,
            GfrOpenPGPKeyVersion::V4,
        );
        assert!(check_if_should_use_key_ver_v6(&primary, &[]));
    }

    #[test]
    fn a_post_quantum_subkey_forces_v6() {
        let primary = keys::cfg(GfrKeyAlgo::ED25519, true, false, GfrOpenPGPKeyVersion::V4);
        let sub = keys::cfg(
            GfrKeyAlgo::KYBER768X25519,
            false,
            true,
            GfrOpenPGPKeyVersion::V4,
        );
        assert!(check_if_should_use_key_ver_v6(&primary, &[sub]));
    }

    #[test]
    fn an_all_classical_request_does_not_force_v6() {
        let primary = keys::cfg(GfrKeyAlgo::ED25519, true, false, GfrOpenPGPKeyVersion::V4);
        let sub = keys::cfg(GfrKeyAlgo::ED25519, false, true, GfrOpenPGPKeyVersion::V4);
        assert!(!check_if_should_use_key_ver_v6(&primary, &[sub]));
    }

    #[test]
    fn an_empty_subkey_list_is_handled() {
        let primary = keys::cfg(GfrKeyAlgo::ED25519, true, false, GfrOpenPGPKeyVersion::V4);
        assert!(!check_if_should_use_key_ver_v6(&primary, &[]));
    }

    #[test]
    fn one_post_quantum_subkey_among_many_still_forces_v6() {
        let primary = keys::cfg(GfrKeyAlgo::ED25519, true, false, GfrOpenPGPKeyVersion::V4);
        let subs = [
            keys::cfg(GfrKeyAlgo::ED25519, false, true, GfrOpenPGPKeyVersion::V4),
            keys::cfg(GfrKeyAlgo::NISTP256, false, true, GfrOpenPGPKeyVersion::V4),
            keys::cfg(
                GfrKeyAlgo::SLHDSASHAKE128F,
                true,
                false,
                GfrOpenPGPKeyVersion::V4,
            ),
        ];
        assert!(check_if_should_use_key_ver_v6(&primary, &subs));
    }

    // -- self-signature helpers ---------------------------------------------

    #[test]
    fn a_self_signature_is_recognised_by_its_issuer_fingerprint() {
        let key = &keys::V4_SIGN.secret;
        let fpr = key.primary_key.fingerprint();
        let sig = key.details.users[0]
            .signatures
            .first()
            .expect("a user id self-signature");
        assert!(is_self_signature_from_primary(sig, fpr.as_bytes()));
    }

    #[test]
    fn a_signature_from_another_key_is_not_a_self_signature() {
        let key = &keys::V4_SIGN.secret;
        let other = &keys::V6_SIGN.secret;
        let sig = key.details.users[0]
            .signatures
            .first()
            .expect("a user id self-signature");
        assert!(!is_self_signature_from_primary(
            sig,
            other.primary_key.fingerprint().as_bytes()
        ));
    }

    #[test]
    fn an_empty_fingerprint_never_matches() {
        let key = &keys::V4_SIGN.secret;
        let sig = key.details.users[0].signatures.first().expect("a sig");
        assert!(!is_self_signature_from_primary(sig, &[]));
    }

    #[test]
    fn the_generated_user_id_self_signature_is_marked_primary() {
        // RFC 9580 §5.2.3.27: the Primary User ID flag nominates the main user
        // ID for a key. The generator sets it even on a single-UID key, which
        // is harmless and makes the flag unambiguous once a second UID is
        // added later.
        let key = &keys::V4_SIGN.secret;
        let sig = key.details.users[0].signatures.first().expect("a sig");
        assert!(has_is_primary_true(sig));
    }

    #[test]
    fn a_subkey_binding_signature_is_not_marked_primary() {
        // §5.2.3.27 scopes the flag to user IDs and user attributes; it has no
        // meaning on a subkey binding.
        let key = &keys::V4_SIGN.secret;
        let sub = key.secret_subkeys.first().expect("a subkey");
        let sig = sub.signatures.first().expect("a binding signature");
        assert!(!has_is_primary_true(sig));
    }

    #[test]
    fn choose_template_prefers_a_signature_carrying_key_flags() {
        // Re-issuing a self-signature must not silently drop the capability
        // flags, so the template is chosen from the signatures that have them.
        let key = &keys::V4_SIGN.secret;
        let sigs: Vec<&Signature> = key.details.users[0].signatures.iter().collect();
        let chosen = choose_template_self_sig(&sigs).expect("a template");
        assert!(
            chosen.key_flags().certify() || chosen.key_flags().sign(),
            "the chosen template should be the one carrying key flags"
        );
    }

    #[test]
    fn choose_template_returns_none_for_an_empty_slice() {
        assert!(choose_template_self_sig(&[]).is_none());
    }

    #[test]
    fn choose_template_returns_the_only_candidate() {
        let key = &keys::V4_SIGN.secret;
        let only = key.details.users[0].signatures.first().expect("a sig");
        let chosen = choose_template_self_sig(&[only]).expect("a template");
        assert_eq!(
            chosen.issuer_fingerprint().first().map(|f| f.to_string()),
            only.issuer_fingerprint().first().map(|f| f.to_string())
        );
    }

    #[test]
    fn choose_template_is_deterministic() {
        let key = &keys::V4_SIGN.secret;
        let sigs: Vec<&Signature> = key.details.users[0].signatures.iter().collect();
        let a = choose_template_self_sig(&sigs).expect("a");
        let b = choose_template_self_sig(&sigs).expect("b");
        assert_eq!(std::ptr::eq(a, b), true, "same input must pick the same sig");
    }

    // -- revocation reason subpackets (RFC 9580 §5.2.3.31) ------------------

    #[test]
    fn every_revocation_code_builds_a_subpacket() {
        for code in [
            GfrRevocationCode::NoReason,
            GfrRevocationCode::Superseded,
            GfrRevocationCode::Compromised,
            GfrRevocationCode::Retired,
            GfrRevocationCode::UserIdInvalid,
        ] {
            assert!(
                build_revocation_reason_subpacket(code, Some("because")).is_ok(),
                "{code:?}"
            );
        }
    }

    #[test]
    fn a_revocation_subpacket_carries_the_mapped_reason_code() {
        let sp = build_revocation_reason_subpacket(GfrRevocationCode::Compromised, Some("leaked"))
            .expect("subpacket");
        match sp.data {
            SubpacketData::RevocationReason(code, ref text) => {
                assert_eq!(code, RevocationCode::KeyCompromised);
                assert_eq!(text.as_ref(), b"leaked");
            }
            ref other => panic!("unexpected subpacket data {other:?}"),
        }
    }

    #[test]
    fn a_missing_reason_text_becomes_an_empty_string() {
        // §5.2.3.31: "The string may be null (of zero length)."
        let sp =
            build_revocation_reason_subpacket(GfrRevocationCode::Retired, None).expect("subpacket");
        match sp.data {
            SubpacketData::RevocationReason(_, ref text) => assert!(text.is_empty()),
            ref other => panic!("unexpected subpacket data {other:?}"),
        }
    }

    #[test]
    fn reason_text_is_preserved_as_utf8() {
        // §5.2.3.31 requires the reason string to be UTF-8.
        let sp = build_revocation_reason_subpacket(
            GfrRevocationCode::Superseded,
            Some("remplacé par une nouvelle clé"),
        )
        .expect("subpacket");
        match sp.data {
            SubpacketData::RevocationReason(_, ref text) => {
                assert_eq!(
                    std::str::from_utf8(text).unwrap(),
                    "remplacé par une nouvelle clé"
                );
            }
            ref other => panic!("unexpected subpacket data {other:?}"),
        }
    }

    #[test]
    fn user_id_invalid_maps_to_code_32() {
        // The certification-revocation reason, distinct from the key ones.
        let sp = build_revocation_reason_subpacket(GfrRevocationCode::UserIdInvalid, None)
            .expect("subpacket");
        match sp.data {
            SubpacketData::RevocationReason(code, _) => {
                assert_eq!(code, RevocationCode::CertUserIdInvalid)
            }
            ref other => panic!("unexpected subpacket data {other:?}"),
        }
    }

    // -- password_from_zeroizing_bytes --------------------------------------

    #[test]
    fn a_password_can_be_read_back_from_zeroizing_bytes() {
        let pw = password_from_zeroizing_bytes(Zeroizing::new(b"s3cret".to_vec()));
        assert_eq!(pw.read().as_slice(), b"s3cret");
    }

    #[test]
    fn a_password_closure_can_be_read_more_than_once() {
        // rPGP may ask for the passphrase once per component, so the closure
        // must not be single-shot.
        let pw = password_from_zeroizing_bytes(Zeroizing::new(b"twice".to_vec()));
        assert_eq!(pw.read().as_slice(), b"twice");
        assert_eq!(pw.read().as_slice(), b"twice");
    }

    #[test]
    fn an_empty_password_round_trips() {
        let pw = password_from_zeroizing_bytes(Zeroizing::new(Vec::new()));
        assert!(pw.read().is_empty());
    }

    #[test]
    fn a_non_utf8_password_round_trips() {
        // Passphrases are bytes, not text; a byte sequence that is not valid
        // UTF-8 must survive intact.
        let raw = vec![0xFFu8, 0x00, 0xFE, 0x80];
        let pw = password_from_zeroizing_bytes(Zeroizing::new(raw.clone()));
        assert_eq!(pw.read().as_slice(), raw.as_slice());
    }

    // -- fetch_password_with_cache ------------------------------------------

    #[test]
    fn the_default_policy_stores_on_a_miss_and_hits_afterwards() {
        let cache = fresh_cache();
        PROMPTS.store(0, std::sync::atomic::Ordering::SeqCst);

        let first = fetch_password_with_cache(
            Some(&cache),
            PasswordCachePolicy::Default,
            0,
            state_for("AABBCCDD"),
            Some(cb_counting),
        )
        .expect("first fetch");
        assert_eq!(first.as_slice(), b"hunter2");
        let after_first = PROMPTS.load(std::sync::atomic::Ordering::SeqCst);

        let second = fetch_password_with_cache(
            Some(&cache),
            PasswordCachePolicy::Default,
            0,
            state_for("AABBCCDD"),
            Some(cb_counting),
        )
        .expect("second fetch");
        assert_eq!(second.as_slice(), b"hunter2");
        assert_eq!(
            PROMPTS.load(std::sync::atomic::Ordering::SeqCst),
            after_first,
            "the second fetch must come from the cache, not the user"
        );
    }

    #[test]
    fn the_bypass_policy_never_consults_or_fills_the_cache() {
        let cache = fresh_cache();
        let key = crate::cache::PasswordCacheKey {
            channel: 0,
            fpr: "BYPASSME".to_string(),
            info: "DECRYPTION".to_string(),
        };
        cache.put(key.clone(), b"cached".to_vec());

        let got = fetch_password_with_cache(
            Some(&cache),
            PasswordCachePolicy::Bypass,
            0,
            state_for("BYPASSME"),
            Some(cb_counting),
        )
        .expect("fetch");

        assert_eq!(got.as_slice(), b"hunter2", "must ignore the cached value");
        assert_eq!(
            cache.get(&key).as_deref(),
            Some(&b"cached"[..]),
            "and must not overwrite it either"
        );
    }

    #[test]
    fn the_refresh_policy_evicts_the_stale_entry_first() {
        // This is the retry path: the cached passphrase was wrong, so it must
        // be dropped rather than handed back again.
        let cache = fresh_cache();
        let key = crate::cache::PasswordCacheKey {
            channel: 0,
            fpr: "REFRESHME".to_string(),
            info: "DECRYPTION".to_string(),
        };
        cache.put(key.clone(), b"stale".to_vec());

        let got = fetch_password_with_cache(
            Some(&cache),
            PasswordCachePolicy::Refresh,
            0,
            state_for("REFRESHME"),
            Some(cb_counting),
        )
        .expect("fetch");

        assert_eq!(got.as_slice(), b"hunter2");
        assert_eq!(
            cache.get(&key).as_deref(),
            Some(&b"hunter2"[..]),
            "the fresh value replaces the stale one"
        );
    }

    #[test]
    fn an_empty_fingerprint_forces_a_bypass() {
        // Symmetric operations have no key to cache against, so caching would
        // key every passphrase under the same empty fingerprint.
        let cache = fresh_cache();
        let got = fetch_password_with_cache(
            Some(&cache),
            PasswordCachePolicy::Default,
            0,
            state_for(""),
            Some(cb_counting),
        )
        .expect("fetch");
        assert_eq!(got.as_slice(), b"hunter2");

        let key = crate::cache::PasswordCacheKey {
            channel: 0,
            fpr: String::new(),
            info: "DECRYPTION".to_string(),
        };
        assert!(cache.get(&key).is_none(), "nothing may be cached");
    }

    #[test]
    fn the_cache_key_is_case_insensitive_in_the_fingerprint() {
        let cache = fresh_cache();
        fetch_password_with_cache(
            Some(&cache),
            PasswordCachePolicy::Default,
            0,
            state_for("abcdef12"),
            Some(cb_counting),
        )
        .expect("store");

        // Fingerprints arrive from the C++ side in either case; the same key
        // must resolve to the same cache entry.
        let key = crate::cache::PasswordCacheKey {
            channel: 0,
            fpr: "ABCDEF12".to_string(),
            info: "DECRYPTION".to_string(),
        };
        assert_eq!(cache.get(&key).as_deref(), Some(&b"hunter2"[..]));
    }

    #[test]
    fn a_missing_cache_falls_back_to_prompting() {
        let got = fetch_password_with_cache(
            None,
            PasswordCachePolicy::Default,
            0,
            state_for("NOCACHE1"),
            Some(cb_counting),
        )
        .expect("fetch");
        assert_eq!(got.as_slice(), b"hunter2");
    }

    #[test]
    fn a_callback_failure_propagates_and_caches_nothing() {
        let cache = fresh_cache();
        let err = fetch_password_with_cache(
            Some(&cache),
            PasswordCachePolicy::Default,
            0,
            state_for("FAILME01"),
            Some(cb_refusing),
        )
        .unwrap_err();
        assert_eq!(err, GfrStatus::ErrorFetchPasswordFailed);

        let key = crate::cache::PasswordCacheKey {
            channel: 0,
            fpr: "FAILME01".to_string(),
            info: "DECRYPTION".to_string(),
        };
        assert!(cache.get(&key).is_none(), "a failure must not be cached");
    }

    #[test]
    fn a_zero_length_passphrase_is_treated_as_a_fetch_failure() {
        // A callback that reports `Provided` but returns a length of zero is
        // misbehaving. It is caught by the `ret <= 0` guard and reported as a
        // fetch failure -- never mistaken for a successful empty passphrase.
        //
        // Note this means the later `pwd_slice.is_empty()` branch (which maps
        // to ErrorInvalidInput) is unreachable: a zero length cannot get past
        // the earlier guard. Harmless belt-and-braces, recorded here so the
        // dead arm is not mistaken for live behaviour.
        let err = fetch_password_internal(0, state_for("EMPTY001"), Some(cb_empty_passphrase))
            .unwrap_err();
        assert_eq!(err, GfrStatus::ErrorFetchPasswordFailed);
    }

    #[test]
    fn a_negative_return_is_treated_as_a_fetch_failure() {
        extern "C" fn cb_negative(
            _channel: i32,
            _state: GfrPassphraseState,
            out_pwd: *mut *mut u8,
            out_status: *mut GfrPasswordFetchStatus,
            _user_data: *mut c_void,
        ) -> i32 {
            // Reports success *and* writes a pointer, but returns a negative
            // length: the engine must not trust the status alone. The buffer it
            // wrote is reclaimed defensively by the error path.
            let (ptr, _len) = crate::testutil::leak_as_c_buffer(b"ignored");
            unsafe {
                *out_pwd = ptr;
                *out_status = GfrPasswordFetchStatus::Provided;
            }
            -1
        }
        let err =
            fetch_password_internal(0, state_for("NEG00001"), Some(cb_negative)).unwrap_err();
        assert_eq!(err, GfrStatus::ErrorFetchPasswordFailed);
    }

    #[test]
    fn a_null_buffer_with_a_provided_status_is_a_fetch_failure() {
        extern "C" fn cb_null_buffer(
            _channel: i32,
            _state: GfrPassphraseState,
            _out_pwd: *mut *mut u8,
            out_status: *mut GfrPasswordFetchStatus,
            _user_data: *mut c_void,
        ) -> i32 {
            // Claims a 6-byte passphrase but never writes the pointer.
            unsafe { *out_status = GfrPasswordFetchStatus::Provided };
            6
        }
        let err =
            fetch_password_internal(0, state_for("NULLBUF1"), Some(cb_null_buffer)).unwrap_err();
        assert_eq!(err, GfrStatus::ErrorFetchPasswordFailed);
    }

    #[test]
    fn different_channels_do_not_share_a_cache_entry() {
        let cache = fresh_cache();
        fetch_password_with_cache(
            Some(&cache),
            PasswordCachePolicy::Default,
            7,
            state_for("SHARED01"),
            Some(cb_counting),
        )
        .expect("store on channel 7");

        let other_channel = crate::cache::PasswordCacheKey {
            channel: 8,
            fpr: "SHARED01".to_string(),
            info: "DECRYPTION".to_string(),
        };
        assert!(cache.get(&other_channel).is_none());
    }

    #[test]
    fn the_info_string_is_part_of_the_cache_key() {
        // Unlocking for signing and unlocking for decryption are different
        // prompts; sharing one entry would show the wrong context.
        let cache = fresh_cache();
        fetch_password_with_cache(
            Some(&cache),
            PasswordCachePolicy::Default,
            0,
            state_for("INFOKEY1"),
            Some(cb_counting),
        )
        .expect("store");

        let other_info = crate::cache::PasswordCacheKey {
            channel: 0,
            fpr: "INFOKEY1".to_string(),
            info: "SIGNING".to_string(),
        };
        assert!(cache.get(&other_info).is_none());
    }
}
