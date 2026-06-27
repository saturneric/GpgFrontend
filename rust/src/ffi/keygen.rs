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

//! FFI entry points for key and subkey generation.

use crate::keygen::{GeneratedKeys, add_subkey_internal, create_key_internal};
use crate::types::{GfrBuffer, GfrKeyConfig, GfrKeyGenerateResult, GfrPasswordFetchCb, GfrStatus};

use std::{
    ffi::{CStr, CString, c_char},
    panic::catch_unwind,
};

/// Generate a new primary key with optional subkeys.
///
/// Creates a primary key for `user_id` using `key_config`, then appends
/// `s_key_count` subkeys described by `s_key_configs`. If the key or any
/// subkey requires a passphrase the `fetch_pwd_cb` is used
/// to obtain and release it.
///
/// On success the result fields of `o_result` are populated with
/// heap-allocated armored key blocks and the fingerprint. Free them with
/// `gfr_crypto_free_key_generate_result`.
///
/// # Safety
/// `user_id` and `o_result` must be non-null. `s_key_configs` must be
/// non-null when `s_key_count > 0`.
#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_generate_key(
    user_id: *const c_char,
    key_config: GfrKeyConfig,
    s_key_configs: *const GfrKeyConfig,
    s_key_count: usize,
    fetch_pwd_cb: GfrPasswordFetchCb,
    o_result: *mut GfrKeyGenerateResult,
) -> GfrStatus {
    let result = catch_unwind(|| -> Result<GeneratedKeys, GfrStatus> {
        if user_id.is_null() || o_result.is_null() {
            return Err(GfrStatus::ErrorInvalidInput);
        }

        let user_id_str = unsafe { CStr::from_ptr(user_id) }
            .to_str()
            .map_err(|_| GfrStatus::ErrorInvalidInput)?;

        if s_key_configs.is_null() && s_key_count > 0 {
            return Err(GfrStatus::ErrorInvalidInput);
        }

        let mut configs: &[GfrKeyConfig] = &[];
        if s_key_count > 0 {
            configs = unsafe { std::slice::from_raw_parts(s_key_configs, s_key_count) };
        }

        let keys = create_key_internal(user_id_str, key_config, configs, Some(fetch_pwd_cb))?;

        Ok(keys)
    });

    match result {
        Ok(inner_result) => match inner_result {
            Ok(keys) => {
                let Ok(c_s) = CString::new(keys.secret.as_bytes()) else {
                    return GfrStatus::ErrorInternal;
                };
                let Ok(c_p) = CString::new(keys.public) else {
                    return GfrStatus::ErrorInternal;
                };
                let Ok(c_f) = CString::new(keys.fingerprint) else {
                    return GfrStatus::ErrorInternal;
                };

                unsafe {
                    *o_result = GfrKeyGenerateResult {
                        secret_key: c_s.into_raw(),
                        public_key: c_p.into_raw(),
                        fingerprint: c_f.into_raw(),
                    };
                }
                GfrStatus::Success
            }

            Err(status) => status,
        },
        Err(_) => GfrStatus::ErrorPanic,
    }
}

/// Add a new subkey to an existing key block.
///
/// Reads the primary key from the armored `key_block`, generates a new
/// subkey according to `config`, and returns updated armored key blocks in
/// `o_result`. The passphrase protecting the primary key is fetched via the
/// `fetch_pwd_cb`.
///
/// On success `o_result` is populated with heap-allocated strings. Free them
/// with `gfr_crypto_free_key_generate_result`.
///
/// # Safety
/// `o_result` must be non-null.
#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_add_subkey(
    channel: i32,
    key_block: GfrBuffer,
    config: GfrKeyConfig,
    fetch_pwd_cb: GfrPasswordFetchCb,
    o_result: *mut GfrKeyGenerateResult,
) -> GfrStatus {
    let result = catch_unwind(|| -> Result<GeneratedKeys, GfrStatus> {
        if o_result.is_null() {
            return Err(GfrStatus::ErrorInvalidInput);
        }

        let key_block_str = unsafe { key_block.as_str() }?;

        let keys = add_subkey_internal(channel, key_block_str, &config, Some(fetch_pwd_cb))?;

        Ok(keys)
    });

    match result {
        Ok(inner_result) => match inner_result {
            Ok(keys) => {
                let Ok(c_s) = CString::new(keys.secret.as_bytes()) else {
                    return GfrStatus::ErrorInternal;
                };
                let Ok(c_p) = CString::new(keys.public) else {
                    return GfrStatus::ErrorInternal;
                };
                let Ok(c_f) = CString::new(keys.fingerprint) else {
                    return GfrStatus::ErrorInternal;
                };

                unsafe {
                    *o_result = GfrKeyGenerateResult {
                        secret_key: c_s.into_raw(),
                        public_key: c_p.into_raw(),
                        fingerprint: c_f.into_raw(),
                    };
                }
                GfrStatus::Success
            }
            Err(status) => status,
        },
        Err(_) => GfrStatus::ErrorPanic,
    }
}
