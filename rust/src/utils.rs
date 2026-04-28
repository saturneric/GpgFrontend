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
    composed::{DsaKeySize, KeyType},
    crypto::ecc_curve::ECCCurve,
    packet::{RevocationCode, Signature, Subpacket, SubpacketData},
    types::{PublicParams, Timestamp},
};
use rsa::traits::PublicKeyParts;

use crate::{
    cache::{PasswordCache, PasswordCacheKey, PasswordCachePolicy},
    types::{
        GfrFreeCb, GfrKeyAlgo, GfrKeyConfig, GfrPassphraseState, GfrPasswordFetchCb,
        GfrRevocationCode, GfrStatus,
    },
};
use std::{
    ffi::{CString, c_char},
    ptr::null_mut,
};

pub struct PassphraseStateInternal {
    pub fpr: String,
    pub info: String,
    pub retry: bool,
    pub ask_for_new: bool,
    pub should_confirm: bool, // User will be asked to confirm the new password by entering it twice if this is true
}

pub fn fetch_password_internal(
    channel: i32,
    state: PassphraseStateInternal,
    fetch_cb: Option<GfrPasswordFetchCb>,
    free_cb: Option<GfrFreeCb>,
) -> Result<Vec<u8>, GfrStatus> {
    let Some(fetch_fn) = fetch_cb else {
        return Err(GfrStatus::ErrorInvalidInput); // Free callback is required if fetch callback is provided
    };

    let Some(free_fn) = free_cb else {
        return Err(GfrStatus::ErrorFetchPasswordFailed); // Password required but no callback provided
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

    // If a password fetch callback is provided, use it to get the password
    // ret > 0 indicates success, and it shows the length of the password returned
    let ret = fetch_fn(channel, passphrase_state, &mut pwd_ptr, null_mut());

    // Callback indicated failure or no password provided
    if ret <= 0 || pwd_ptr.is_null() {
        return Err(GfrStatus::ErrorFetchPasswordFailed);
    }

    // safely create a slice from the returned pointer and length
    let pwd_slice = unsafe { std::slice::from_raw_parts(pwd_ptr, ret as usize) };

    if pwd_slice.is_empty() {
        free_fn(pwd_ptr as *mut std::ffi::c_void, null_mut());
        return Err(GfrStatus::ErrorInvalidInput); // Empty password provided
    }

    // 1. COPY THE DATA FIRST to Rust's owned Vec
    let mut password = Vec::with_capacity(ret as usize);
    password.extend_from_slice(pwd_slice);

    // 2. NOW FREE THE MEMORY in C++
    free_fn(pwd_ptr as *mut std::ffi::c_void, null_mut());

    log::debug!("Fetched password via callback");
    Ok(password)
}

pub fn fetch_password_with_cache(
    cache: Option<&PasswordCache>,
    policy: PasswordCachePolicy,
    channel: i32,
    state: PassphraseStateInternal,
    fetch_cb: Option<GfrPasswordFetchCb>,
    free_cb: Option<GfrFreeCb>,
) -> Result<Vec<u8>, GfrStatus> {
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
                    return Ok(pwd);
                }
            }

            let pwd = fetch_password_internal(channel, state, fetch_cb, free_cb)?;

            if let Some(cache) = cache {
                cache.put(key, pwd.clone());
            }

            Ok(pwd)
        }

        PasswordCachePolicy::Bypass => fetch_password_internal(channel, state, fetch_cb, free_cb),

        PasswordCachePolicy::Refresh => {
            if let Some(cache) = cache {
                cache.remove(&key);
            }

            let pwd = fetch_password_internal(channel, state, fetch_cb, free_cb)?;

            if let Some(cache) = cache {
                cache.put(key, pwd.clone());
            }

            Ok(pwd)
        }
    }
}

pub fn resolve_key_type(algo: &GfrKeyAlgo, can_encrypt: bool) -> Result<KeyType, GfrStatus> {
    match algo {
        GfrKeyAlgo::ED25519 | GfrKeyAlgo::CV25519 => {
            if can_encrypt {
                Ok(KeyType::ECDH(ECCCurve::Curve25519))
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
            ECCCurve::Curve25519 => GfrKeyAlgo::CV25519,
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
