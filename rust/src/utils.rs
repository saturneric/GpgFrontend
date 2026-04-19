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

use pgp::types::PublicParams;
use rsa::traits::PublicKeyParts;

use crate::types::{GfrFreeCb, GfrPasswordFetchCb, GfrStatus};
use std::{
    ffi::{CString, c_char},
    ptr::null_mut,
};

pub fn fetch_password_internal(
    channel: i32,
    fpr: &str,
    info: &str,
    fetch_cb: Option<GfrPasswordFetchCb>,
    free_cb: Option<GfrFreeCb>,
) -> Result<Vec<u8>, GfrStatus> {
    let Some(fetch_fn) = fetch_cb else {
        return Err(GfrStatus::ErrorInvalidInput); // Free callback is required if fetch callback is provided
    };

    let Some(free_fn) = free_cb else {
        return Err(GfrStatus::ErrorFetchPasswordFailed); // Password required but no callback provided
    };

    let info_c = CString::new(info).unwrap_or_default();
    let fpr_c = CString::new(fpr).unwrap_or_default();

    let mut pwd_ptr: *mut u8 = null_mut();

    // If a password fetch callback is provided, use it to get the password
    // ret > 0 indicates success, and it shows the length of the password returned
    let ret = fetch_fn(
        channel,
        fpr_c.as_ptr() as *const c_char,
        info_c.as_ptr() as *const c_char,
        &mut pwd_ptr,
        null_mut(),
    );

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

pub fn extract_key_length(public_params: &PublicParams) -> Option<u32> {
    match public_params {
        PublicParams::RSA(p) => Some(p.key.n().bits() as u32),

        PublicParams::Ed25519(_) => Some(255),
        PublicParams::Ed448(_) => Some(448),
        PublicParams::X448(_) => Some(448),

        PublicParams::ECDH(p) => match p.curve() {
            pgp::crypto::ecc_curve::ECCCurve::Curve25519 => Some(255),
            pgp::crypto::ecc_curve::ECCCurve::P256 => Some(256),
            pgp::crypto::ecc_curve::ECCCurve::P384 => Some(384),
            pgp::crypto::ecc_curve::ECCCurve::P521 => Some(521),
            pgp::crypto::ecc_curve::ECCCurve::BrainpoolP256r1 => Some(256),
            pgp::crypto::ecc_curve::ECCCurve::BrainpoolP384r1 => Some(384),
            pgp::crypto::ecc_curve::ECCCurve::BrainpoolP512r1 => Some(512),
            pgp::crypto::ecc_curve::ECCCurve::Secp256k1 => Some(256),
            _ => None,
        },

        PublicParams::ECDSA(p) => match p.curve() {
            pgp::crypto::ecc_curve::ECCCurve::P256 => Some(256),
            pgp::crypto::ecc_curve::ECCCurve::P384 => Some(384),
            pgp::crypto::ecc_curve::ECCCurve::P521 => Some(521),
            pgp::crypto::ecc_curve::ECCCurve::BrainpoolP256r1 => Some(256),
            pgp::crypto::ecc_curve::ECCCurve::BrainpoolP384r1 => Some(384),
            pgp::crypto::ecc_curve::ECCCurve::BrainpoolP512r1 => Some(512),
            pgp::crypto::ecc_curve::ECCCurve::Secp256k1 => Some(256),
            _ => None,
        },

        _ => None,
    }
}
