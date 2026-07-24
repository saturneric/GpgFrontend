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

//! FFI entry points for key block manipulation.
//!
//! Covers metadata extraction, public-key derivation, recipient sniffing,
//! passphrase changes, subkey deletion/revocation, revocation certificate
//! generation and import, and key block merging.

use crate::crypto::sniff_recipients;
use crate::err::clear_last_error;
use crate::key::{
    delete_subkey_internal, extract_rev_cert_target_fpr_internal, generate_key_rev_cert_internal,
    get_ecdh_kdf_params_internal, import_rev_cert_internal, merge_key_block_internal,
    modify_key_password_internal, revoke_subkey_internal, update_key_expiration_internal,
};
use crate::key::{
    export_merged_public_keys, export_merged_secret_keys, extract_public_key_internal,
};
use crate::types::{
    GfrBuffer, GfrKeyMetadataC, GfrPasswordFetchCb, GfrRecipientResultC, GfrRevocationCode,
    GfrStatus, GfrSubkeyMetadataC, GfrUserIdC,
};
use std::slice;
use std::{
    ffi::{CStr, CString, c_char},
    panic::catch_unwind,
};

/// Parse an armored key block and return an array of key metadata structs.
///
/// On success `*out_metadata` points to a heap-allocated array of
/// `*out_metadata_count` `GfrKeyMetadataC` structs. Free the array with
/// `gfr_free_metadata_array`.
///
/// # Safety
/// All pointer arguments must be non-null.
#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_extract_metadata(
    key_block: GfrBuffer,
    out_metadata: *mut *mut GfrKeyMetadataC,
    out_metadata_count: *mut usize,
) -> GfrStatus {
    if out_metadata.is_null() || out_metadata_count.is_null() {
        return GfrStatus::ErrorInvalidInput;
    }

    let result = std::panic::catch_unwind(|| -> Result<(), GfrStatus> {
        let block_str = unsafe { key_block.as_str() }?;

        // Get the list of metadata from the internal function
        let meta_list = crate::key::extract_metadata_many_internal(block_str)?;

        // Prepare a vector to hold the C-compatible metadata structs
        let mut c_metadata_list = Vec::with_capacity(meta_list.len());

        for meta in meta_list {
            // Use `unwrap_or_default` rather than `?` here: a fallible early
            // return mid-loop would leak every C string and array already built
            // (including secret key blocks). Interior NULs cannot occur in
            // fingerprints/key IDs/armor; an empty string is a safe fallback.
            let c_fpr = CString::new(meta.fpr).unwrap_or_default();
            let c_key_id = CString::new(meta.key_id).unwrap_or_default();

            // Convert the new armored key blocks
            let c_pub_block = CString::new(meta.public_key_block)
                .unwrap_or_default()
                .into_raw();

            // `secret_str` is `Zeroizing<String>`: copy its bytes into the C
            // string, then let the source wipe itself on drop at the arm's end.
            let c_sec_block = match meta.secret_key_block {
                Some(secret_str) => CString::new(secret_str.as_bytes())
                    .unwrap_or_default()
                    .into_raw(),
                None => std::ptr::null_mut(), // Safe to use NULL for Option::None in C
            };

            // Convert the subkeys Vec into a C-compatible array
            let mut c_subkeys = Vec::with_capacity(meta.subkeys.len());
            for sub in meta.subkeys {
                c_subkeys.push(GfrSubkeyMetadataC {
                    ver: sub.ver,
                    fpr: CString::new(sub.fpr).unwrap_or_default().into_raw(),
                    key_id: CString::new(sub.key_id).unwrap_or_default().into_raw(),
                    algo: sub.algo,
                    created_at: sub.created_at,
                    expires_at: sub.expires_at,
                    has_secret: sub.has_secret,
                    is_revoked: sub.is_revoked,
                    can_sign: sub.can_sign,
                    can_encrypt: sub.can_encrypt,
                    can_auth: sub.can_auth,
                    can_certify: sub.can_certify,
                    key_length: sub.key_length,
                });
            }

            // Prevent Rust from deallocating the subkeys array, transfer ownership to C
            let mut boxed_subkeys = c_subkeys.into_boxed_slice();
            let subkeys_ptr = boxed_subkeys.as_mut_ptr();
            let subkey_count = boxed_subkeys.len();
            std::mem::forget(boxed_subkeys); // Leak it deliberately

            let mut c_user_ids = Vec::with_capacity(meta.user_ids.len());
            for uid in meta.user_ids {
                c_user_ids.push(GfrUserIdC {
                    user_id: CString::new(uid.user_id).unwrap_or_default().into_raw(),
                    is_primary: uid.is_primary,
                    is_revoked: uid.is_revoked,
                });
            }

            let mut boxed_user_ids = c_user_ids.into_boxed_slice();
            let user_ids_ptr = boxed_user_ids.as_mut_ptr();
            let user_id_count = boxed_user_ids.len();
            std::mem::forget(boxed_user_ids); // Leak it deliberately

            // Assemble the C-compatible metadata struct
            c_metadata_list.push(GfrKeyMetadataC {
                ver: meta.ver,
                fpr: c_fpr.into_raw(),
                key_id: c_key_id.into_raw(),
                user_ids: user_ids_ptr,
                user_id_count,
                algo: meta.algo,
                key_length: meta.key_length,
                created_at: meta.created_at,
                expires_at: meta.expires_at,
                has_secret: meta.has_secret,
                is_revoked: meta.is_revoked,
                can_sign: meta.can_sign,
                can_encrypt: meta.can_encrypt,
                can_auth: meta.can_auth,
                can_certify: meta.can_certify,
                subkeys: subkeys_ptr,
                subkey_count,
                public_key_block: c_pub_block,
                secret_key_block: c_sec_block,
            });
        }

        // Prevent Rust from deallocating the outer metadata array
        let mut boxed_metadata = c_metadata_list.into_boxed_slice();
        unsafe {
            // Write to output pointers
            *out_metadata = boxed_metadata.as_mut_ptr();
            *out_metadata_count = boxed_metadata.len();
        }
        std::mem::forget(boxed_metadata); // Leak outer array to FFI

        Ok(())
    });

    match result {
        Ok(Ok(_)) => GfrStatus::Success, // Replace with your actual success enum variant
        Ok(Err(e)) => e,
        Err(_) => GfrStatus::ErrorPanic,
    }
}

/// Extract the public key from an armored secret key block.
///
/// On success `*out_public_block` is set to a heap-allocated armored public
/// key string. Free it with `gfr_crypto_free_string`.
///
/// # Safety
/// `secret_block` and `out_public_block` must be non-null.
#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_extract_public_key(
    secret_block: GfrBuffer,
    out_public_block: *mut *mut c_char,
) -> GfrStatus {
    let result = catch_unwind(|| -> Result<(), GfrStatus> {
        if out_public_block.is_null() {
            return Err(GfrStatus::ErrorInvalidInput);
        }

        let block_str = unsafe { secret_block.as_str() }?;

        // Perform the extraction
        let pub_key_str = extract_public_key_internal(block_str)?;

        // Convert the result back to CString
        let c_pub = CString::new(pub_key_str).map_err(|_| GfrStatus::ErrorInternal)?;

        // Transfer ownership to C++
        unsafe {
            *out_public_block = c_pub.into_raw();
        }

        Ok(())
    });

    match result {
        Ok(Ok(_)) => GfrStatus::Success,
        Ok(Err(e)) => e,
        Err(_) => GfrStatus::ErrorPanic,
    }
}

/// Sniff the recipient key IDs from an encrypted data buffer without decrypting.
///
/// On success `*out_recipients` points to a heap-allocated array of
/// `*out_count` `GfrRecipientResultC` structs. Free with
/// `gfr_crypto_free_decrypt_result` or manually via `gfr_crypto_free_string`.
///
/// Returns `ErrorInvalidData` if no recipients are found.
///
/// # Safety
/// `in_data`, `out_recipients`, and `out_count` must be non-null;
/// `in_data` must point to at least `in_len` bytes.
#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_get_recipients(
    in_data: *const u8,
    in_len: usize,
    out_recipients: *mut *mut GfrRecipientResultC, // Changed to return structured array
    out_count: *mut usize,                         // Added to return array length
) -> GfrStatus {
    let result = std::panic::catch_unwind(|| -> Result<(), GfrStatus> {
        if in_data.is_null() || out_recipients.is_null() || out_count.is_null() {
            return Err(GfrStatus::ErrorInvalidInput);
        }

        let data_slice = unsafe { std::slice::from_raw_parts(in_data, in_len) };

        // 2. Call sniff_recipients directly to get the structured list
        // Note: sniff_recipients does not return a Result, it returns Vec directly
        let recipients = sniff_recipients(data_slice);

        if recipients.is_empty() {
            return Err(GfrStatus::ErrorInvalidData);
        }

        // 3. Map to C structures
        let mut c_recipients = Vec::with_capacity(recipients.len());
        for rec in recipients {
            c_recipients.push(GfrRecipientResultC {
                key_id: std::ffi::CString::new(rec.key_id)
                    .unwrap_or_default()
                    .into_raw(),
                pub_algo: std::ffi::CString::new(rec.pub_algo)
                    .unwrap_or_default()
                    .into_raw(),
                status: rec.status, // Usually defaults to NoKey at this stage
            });
        }

        // 4. Leak the array to C
        let mut boxed_recs = c_recipients.into_boxed_slice();
        unsafe {
            *out_recipients = boxed_recs.as_mut_ptr();
            *out_count = boxed_recs.len();
        }
        std::mem::forget(boxed_recs);

        Ok(())
    });

    match result {
        Ok(Ok(_)) => GfrStatus::Success,
        Ok(Err(e)) => e,
        Err(_) => GfrStatus::ErrorPanic,
    }
}

/// Merge multiple armored key blocks and export the combined result.
///
/// `keys_ptr` is an array of `keys_len` armored key block C strings. If
/// `secret` is true, the merged secret key is returned; otherwise only the
/// public key is included. On success `*out_armored_ptr` is set to a
/// heap-allocated armored key string. Free with `gfr_crypto_free_string`.
///
/// # Safety
/// `keys_ptr` must point to a valid array of `keys_len` non-null C strings;
/// `out_armored_ptr` must be non-null.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn gfr_export_merged_keys(
    keys_ptr: *const *const c_char,
    keys_len: usize,
    secret: bool,
    out_armored_ptr: *mut *mut c_char,
) -> GfrStatus {
    // Parsing caller-supplied armored key blocks runs through rPGP, which can
    // panic on malformed packets. Guard the whole body in `catch_unwind` (as the
    // sibling parsing entry points do) so a panic never unwinds across the
    // `extern "C"` boundary, which is undefined behavior.
    let result = catch_unwind(|| -> Result<(), GfrStatus> {
        if keys_ptr.is_null() || out_armored_ptr.is_null() {
            return Err(GfrStatus::ErrorInvalidInput);
        }

        let c_str_ptrs = unsafe { slice::from_raw_parts(keys_ptr, keys_len) };
        let mut rust_strs = Vec::with_capacity(keys_len);

        for &ptr in c_str_ptrs {
            if ptr.is_null() {
                return Err(GfrStatus::ErrorInvalidInput);
            }
            match unsafe { CStr::from_ptr(ptr).to_str() } {
                Ok(s) => rust_strs.push(s),
                Err(_) => return Err(GfrStatus::ErrorInvalidInput),
            }
        }

        let armored_string = if secret {
            export_merged_secret_keys(&rust_strs)?
        } else {
            export_merged_public_keys(&rust_strs)?
        };

        let c_str = CString::new(armored_string).map_err(|_| GfrStatus::ErrorArmorFailed)?;
        unsafe { *out_armored_ptr = c_str.into_raw() };
        Ok(())
    });

    match result {
        Ok(Ok(_)) => GfrStatus::Success,
        Ok(Err(e)) => e,
        Err(_) => GfrStatus::ErrorPanic,
    }
}

/// Change the passphrase protecting `secret_key_block`.
///
/// `target_fpr` selects the scope. A null or empty `target_fpr` re-protects the
/// whole key — primary and every subkey — under one new passphrase, matching what
/// `gpg --passwd` does. Otherwise only the key at `target_fpr` is re-protected,
/// which may be the primary or any subkey.
///
/// The current passphrase and the new passphrase are both fetched via
/// `fetch_pwd_cb`. On success `*out_secret_block` is set to a
/// heap-allocated updated armored secret key block. Free with `gfr_crypto_free_string`.
///
/// # Safety
/// `secret_key_block` and `out_secret_block` must be non-null. `target_fpr` may be
/// null to request a whole-key change.
#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_modify_key_password(
    channel: i32,
    secret_key_block: GfrBuffer,
    target_fpr: *const c_char,
    fetch_pwd_cb: GfrPasswordFetchCb,
    out_secret_block: *mut *mut c_char,
) -> GfrStatus {
    clear_last_error();

    if !out_secret_block.is_null() {
        unsafe {
            *out_secret_block = std::ptr::null_mut();
        }
    }

    let result = catch_unwind(|| -> Result<(), GfrStatus> {
        if out_secret_block.is_null() {
            return Err(GfrStatus::ErrorInvalidInput);
        }

        let block_str = unsafe { secret_key_block.as_str() }?;

        let fpr_str = if target_fpr.is_null() {
            None
        } else {
            let s = unsafe { CStr::from_ptr(target_fpr) }
                .to_str()
                .map_err(|_| GfrStatus::ErrorInvalidInput)?;
            if s.is_empty() { None } else { Some(s) }
        };

        let generated =
            modify_key_password_internal(channel, block_str, fpr_str, Some(fetch_pwd_cb))?;

        let c_secret =
            CString::new(generated.secret.as_bytes()).map_err(|_| GfrStatus::ErrorInternal)?;

        unsafe {
            *out_secret_block = c_secret.into_raw();
        }

        Ok(())
    });

    match result {
        Ok(Ok(())) => GfrStatus::Success,
        Ok(Err(e)) => e,
        Err(_) => GfrStatus::ErrorPanic,
    }
}

/// Remove the subkey identified by `target_subkey_fpr` from `secret_key_block`.
///
/// On success `*out_secret_block` is set to a heap-allocated updated armored
/// secret key block. Free with `gfr_crypto_free_string`.
///
/// # Safety
/// `secret_key_block`, `target_subkey_fpr`, and `out_secret_block` must be non-null.
#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_delete_subkey(
    secret_key_block: GfrBuffer,
    target_subkey_fpr: *const c_char,
    out_secret_block: *mut *mut c_char,
) -> GfrStatus {
    clear_last_error();

    if !out_secret_block.is_null() {
        unsafe {
            *out_secret_block = std::ptr::null_mut();
        }
    }

    let result = catch_unwind(|| -> Result<(), GfrStatus> {
        if target_subkey_fpr.is_null() || out_secret_block.is_null() {
            return Err(GfrStatus::ErrorInvalidInput);
        }

        let block_str = unsafe { secret_key_block.as_str() }?;

        let fpr_str = unsafe { CStr::from_ptr(target_subkey_fpr) }
            .to_str()
            .map_err(|_| GfrStatus::ErrorInvalidInput)?;

        let result = delete_subkey_internal(block_str, fpr_str)?;

        let secret_cstr =
            CString::new(result.secret.as_bytes()).map_err(|_| GfrStatus::ErrorInternal)?;

        unsafe {
            *out_secret_block = secret_cstr.into_raw();
        }

        Ok(())
    });

    match result {
        Ok(Ok(())) => GfrStatus::Success,
        Ok(Err(e)) => e,
        Err(_) => GfrStatus::ErrorPanic,
    }
}

/// Extract the ECDH KDF parameters of the (sub)key `target_fpr` from
/// `public_key_block`, as the hexified octet-string `03 01 <hash> <cipher>`.
///
/// Needed to build the gpg-agent `KEYTOCARD ... <ecdh>` command when moving an
/// ECDH encryption subkey onto a smart card. On success `*out_hex` is set to a
/// heap-allocated C string. Free with `gfr_crypto_free_string`.
///
/// # Safety
/// `target_fpr` and `out_hex` must be non-null.
#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_get_ecdh_kdf_params(
    public_key_block: GfrBuffer,
    target_fpr: *const c_char,
    out_hex: *mut *mut c_char,
) -> GfrStatus {
    clear_last_error();

    if !out_hex.is_null() {
        unsafe {
            *out_hex = std::ptr::null_mut();
        }
    }

    let result = catch_unwind(|| -> Result<(), GfrStatus> {
        if target_fpr.is_null() || out_hex.is_null() {
            return Err(GfrStatus::ErrorInvalidInput);
        }

        let block_str = unsafe { public_key_block.as_str() }?;

        let fpr_str = unsafe { CStr::from_ptr(target_fpr) }
            .to_str()
            .map_err(|_| GfrStatus::ErrorInvalidInput)?;

        let hex = get_ecdh_kdf_params_internal(block_str, fpr_str)?;

        let hex_cstr = CString::new(hex.as_bytes()).map_err(|_| GfrStatus::ErrorInternal)?;

        unsafe {
            *out_hex = hex_cstr.into_raw();
        }

        Ok(())
    });

    match result {
        Ok(Ok(())) => GfrStatus::Success,
        Ok(Err(e)) => e,
        Err(_) => GfrStatus::ErrorPanic,
    }
}

/// Revoke the subkey at `target_subkey_fpr` within `secret_key_block`.
///
/// `reason_code` and `reason_text` (may be null) describe the revocation.
/// The passphrase is fetched via `fetch_pwd_cb`. On success
/// `*out_secret_block` is set to a heap-allocated updated armored secret key block.
///
/// # Safety
/// `secret_key_block`, `target_subkey_fpr`, and `out_secret_block` must be non-null.
/// `reason_text` may be null.
#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_revoke_subkey(
    channel: i32,
    secret_key_block: GfrBuffer,
    target_subkey_fpr: *const c_char,
    reason_code: GfrRevocationCode,
    reason_text: *const c_char,
    fetch_pwd_cb: GfrPasswordFetchCb,
    out_secret_block: *mut *mut c_char,
) -> GfrStatus {
    clear_last_error();

    if !out_secret_block.is_null() {
        unsafe {
            *out_secret_block = std::ptr::null_mut();
        }
    }

    let result = catch_unwind(|| -> Result<(), GfrStatus> {
        if target_subkey_fpr.is_null() || out_secret_block.is_null() {
            return Err(GfrStatus::ErrorInvalidInput);
        }

        let block_str = unsafe { secret_key_block.as_str() }?;

        let subkey_fpr_str = unsafe { CStr::from_ptr(target_subkey_fpr) }
            .to_str()
            .map_err(|_| GfrStatus::ErrorInvalidInput)?;

        let reason_text = if reason_text.is_null() {
            None
        } else {
            Some(
                unsafe { CStr::from_ptr(reason_text) }
                    .to_str()
                    .map_err(|_| GfrStatus::ErrorInvalidInput)?,
            )
        };

        let result = revoke_subkey_internal(
            channel,
            block_str,
            subkey_fpr_str,
            reason_code,
            reason_text,
            Some(fetch_pwd_cb),
        )?;

        let secret_cstr =
            CString::new(result.secret.as_bytes()).map_err(|_| GfrStatus::ErrorInternal)?;

        unsafe {
            *out_secret_block = secret_cstr.into_raw();
        }

        Ok(())
    });

    match result {
        Ok(Ok(())) => GfrStatus::Success,
        Ok(Err(e)) => e,
        Err(_) => GfrStatus::ErrorPanic,
    }
}

/// Change the expiration of a primary key or a single subkey in `secret_key_block`.
///
/// `target_fpr` selects the scope: null, empty, or the primary key fingerprint
/// targets the primary key; any other fingerprint targets that subkey.
/// `expiration_epoch_secs` is an absolute Unix time in seconds, or `0` for
/// "never expires". The passphrase is fetched via `fetch_pwd_cb`. On success
/// `*out_secret_block` is set to a heap-allocated updated armored secret key
/// block. Free with `gfr_crypto_free_string`.
///
/// # Safety
/// `secret_key_block` and `out_secret_block` must be non-null. `target_fpr` may
/// be null to target the primary key.
#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_update_key_expiration(
    channel: i32,
    secret_key_block: GfrBuffer,
    target_fpr: *const c_char,
    expiration_epoch_secs: u64,
    fetch_pwd_cb: GfrPasswordFetchCb,
    out_secret_block: *mut *mut c_char,
) -> GfrStatus {
    clear_last_error();

    if !out_secret_block.is_null() {
        unsafe {
            *out_secret_block = std::ptr::null_mut();
        }
    }

    let result = catch_unwind(|| -> Result<(), GfrStatus> {
        if out_secret_block.is_null() {
            return Err(GfrStatus::ErrorInvalidInput);
        }

        let block_str = unsafe { secret_key_block.as_str() }?;

        let fpr_str = if target_fpr.is_null() {
            None
        } else {
            let s = unsafe { CStr::from_ptr(target_fpr) }
                .to_str()
                .map_err(|_| GfrStatus::ErrorInvalidInput)?;
            if s.is_empty() { None } else { Some(s) }
        };

        let result = update_key_expiration_internal(
            channel,
            block_str,
            fpr_str,
            expiration_epoch_secs,
            Some(fetch_pwd_cb),
        )?;

        let secret_cstr =
            CString::new(result.secret.as_bytes()).map_err(|_| GfrStatus::ErrorInternal)?;

        unsafe {
            *out_secret_block = secret_cstr.into_raw();
        }

        Ok(())
    });

    match result {
        Ok(Ok(())) => GfrStatus::Success,
        Ok(Err(e)) => e,
        Err(_) => GfrStatus::ErrorPanic,
    }
}

/// Generate a revocation certificate for the primary key in `secret_key_block`.
///
/// `reason_code` and `reason_text` (may be null) describe the revocation reason.
/// The passphrase is fetched via `fetch_pwd_cb`. On success
/// `*out_cert_block` is set to a heap-allocated armored revocation certificate.
/// Free with `gfr_crypto_free_string`.
///
/// # Safety
/// `secret_key_block` and `out_cert_block` must be non-null.
/// `reason_text` may be null.
#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_generate_key_rev_cert(
    channel: i32,
    secret_key_block: GfrBuffer,
    reason_code: GfrRevocationCode,
    reason_text: *const c_char,
    fetch_pwd_cb: GfrPasswordFetchCb,
    out_cert_block: *mut *mut c_char,
) -> GfrStatus {
    clear_last_error();

    if !out_cert_block.is_null() {
        unsafe {
            *out_cert_block = std::ptr::null_mut();
        }
    }

    let result = catch_unwind(|| -> Result<(), GfrStatus> {
        if out_cert_block.is_null() {
            return Err(GfrStatus::ErrorInvalidInput);
        }

        let block_str = unsafe { secret_key_block.as_str() }?;

        let reason_text = if reason_text.is_null() {
            None
        } else {
            Some(
                unsafe { CStr::from_ptr(reason_text) }
                    .to_str()
                    .map_err(|_| GfrStatus::ErrorInvalidInput)?,
            )
        };

        let cert = generate_key_rev_cert_internal(
            channel,
            block_str,
            reason_code,
            reason_text,
            Some(fetch_pwd_cb),
        )?;

        let cert_cstr = CString::new(cert).map_err(|_| GfrStatus::ErrorInternal)?;

        unsafe {
            *out_cert_block = cert_cstr.into_raw();
        }

        Ok(())
    });

    match result {
        Ok(Ok(())) => GfrStatus::Success,
        Ok(Err(e)) => e,
        Err(_) => GfrStatus::ErrorPanic,
    }
}

/// Merge `incoming_block` into `base_block`, returning updated public and secret blocks.
///
/// Useful for importing certification signatures or subkeys from an external source.
/// On success `*out_public_block` is always populated; `*out_secret_block` is
/// populated only if a secret key is present. Free with `gfr_crypto_free_string`.
///
/// # Safety
/// All four pointer arguments must be non-null.
#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_merge_key_blocks(
    base_block: GfrBuffer,
    incoming_block: GfrBuffer,
    out_secret_block: *mut *mut c_char,
    out_public_block: *mut *mut c_char,
) -> GfrStatus {
    clear_last_error();

    if !out_secret_block.is_null() {
        unsafe {
            *out_secret_block = std::ptr::null_mut();
        }
    }

    if !out_public_block.is_null() {
        unsafe {
            *out_public_block = std::ptr::null_mut();
        }
    }

    let result = catch_unwind(|| -> Result<(), GfrStatus> {
        if out_secret_block.is_null() || out_public_block.is_null() {
            return Err(GfrStatus::ErrorInvalidInput);
        }

        let base_str = unsafe { base_block.as_str() }?;

        let incoming_str = unsafe { incoming_block.as_str() }?;

        let merged = merge_key_block_internal(base_str, incoming_str)?;

        let public_cstr = CString::new(merged.public).map_err(|_| GfrStatus::ErrorInternal)?;

        unsafe {
            *out_public_block = public_cstr.into_raw();
        }

        if !merged.secret.is_empty() {
            let secret_cstr =
                CString::new(merged.secret.as_bytes()).map_err(|_| GfrStatus::ErrorInternal)?;
            unsafe {
                *out_secret_block = secret_cstr.into_raw();
            }
        }

        Ok(())
    });

    match result {
        Ok(Ok(())) => GfrStatus::Success,
        Ok(Err(e)) => e,
        Err(_) => GfrStatus::ErrorPanic,
    }
}

/// Apply a revocation certificate to a key block.
///
/// Merges `rev_cert_block` into `base_key_block`. On success both output
/// pointers are populated with updated armored key blocks (secret may remain
/// null if no secret key is present). Free with `gfr_crypto_free_string`.
///
/// # Safety
/// All four pointer arguments must be non-null.
#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_import_rev_cert(
    base_key_block: GfrBuffer,
    rev_cert_block: GfrBuffer,
    out_secret_block: *mut *mut c_char,
    out_public_block: *mut *mut c_char,
) -> GfrStatus {
    clear_last_error();

    if !out_secret_block.is_null() {
        unsafe {
            *out_secret_block = std::ptr::null_mut();
        }
    }

    if !out_public_block.is_null() {
        unsafe {
            *out_public_block = std::ptr::null_mut();
        }
    }

    let result = catch_unwind(|| -> Result<(), GfrStatus> {
        if out_secret_block.is_null() || out_public_block.is_null() {
            return Err(GfrStatus::ErrorInvalidInput);
        }

        let base_str = unsafe { base_key_block.as_str() }?;

        let cert_str = unsafe { rev_cert_block.as_str() }?;

        let merged = import_rev_cert_internal(base_str, cert_str)?;

        if !merged.secret.is_empty() {
            let secret_cstr =
                CString::new(merged.secret.as_bytes()).map_err(|_| GfrStatus::ErrorInternal)?;
            unsafe {
                *out_secret_block = secret_cstr.into_raw();
            }
        }

        let public_cstr = CString::new(merged.public).map_err(|_| GfrStatus::ErrorInternal)?;
        unsafe {
            *out_public_block = public_cstr.into_raw();
        }

        Ok(())
    });

    match result {
        Ok(Ok(())) => GfrStatus::Success,
        Ok(Err(e)) => e,
        Err(_) => GfrStatus::ErrorPanic,
    }
}

/// Extract the fingerprint of the key targeted by a revocation certificate.
///
/// On success `*out_fpr` is set to a heap-allocated fingerprint string.
/// Free with `gfr_crypto_free_string`.
///
/// # Safety
/// `rev_cert_block` and `out_fpr` must be non-null.
#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_extract_rev_cert_target_fpr(
    rev_cert_block: GfrBuffer,
    out_fpr: *mut *mut c_char,
) -> GfrStatus {
    clear_last_error();

    if !out_fpr.is_null() {
        unsafe {
            *out_fpr = std::ptr::null_mut();
        }
    }

    let result = catch_unwind(|| -> Result<(), GfrStatus> {
        if out_fpr.is_null() {
            return Err(GfrStatus::ErrorInvalidInput);
        }

        let cert_str = unsafe { rev_cert_block.as_str() }?;

        let fpr = extract_rev_cert_target_fpr_internal(cert_str)?;

        let fpr_cstr = CString::new(fpr).map_err(|_| GfrStatus::ErrorInternal)?;
        unsafe {
            *out_fpr = fpr_cstr.into_raw();
        }

        Ok(())
    });

    match result {
        Ok(Ok(())) => GfrStatus::Success,
        Ok(Err(e)) => e,
        Err(_) => GfrStatus::ErrorPanic,
    }
}

#[cfg(test)]
mod ffi_key_tests {
    //! The 13 key-block FFI entry points.
    //!
    //! Rules every test here follows, because the alternative is undefined
    //! behaviour rather than a failing assertion:
    //!   * no pointer is ever fabricated -- every non-null one comes from a
    //!     live Rust allocation;
    //!   * C strings are bound to a local, never built inline as a temporary
    //!     whose backing storage dies before the call;
    //!   * out-params are zeroed `MaybeUninit` and only read on `Success`;
    //!   * everything allocated is released with its matching
    //!     `gfr_crypto_free_*`.

    use super::*;
    use crate::testutil::{corpus, keys};
    use std::ffi::CString;

    fn buf(s: &str) -> GfrBuffer {
        GfrBuffer {
            data: s.as_ptr(),
            len: s.len(),
        }
    }

    /// Consume an out-param C string and free it.
    fn take(ptr: *mut c_char) -> String {
        assert!(!ptr.is_null());
        let s = unsafe { CStr::from_ptr(ptr) }.to_string_lossy().into_owned();
        crate::ffi::mem::gfr_crypto_free_string(ptr);
        s
    }

    // -- extract_metadata -----------------------------------------------------

    #[test]
    fn extract_metadata_rejects_null_out_params() {
        let key = &keys::V4_SIGN;
        let mut count = 0usize;
        assert_eq!(
            gfr_crypto_extract_metadata(buf(&key.public_armored), std::ptr::null_mut(), &mut count),
            GfrStatus::ErrorInvalidInput
        );
        let mut list = std::ptr::null_mut();
        assert_eq!(
            gfr_crypto_extract_metadata(buf(&key.public_armored), &mut list, std::ptr::null_mut()),
            GfrStatus::ErrorInvalidInput
        );
    }

    #[test]
    fn extract_metadata_rejects_an_empty_buffer() {
        let mut list = std::ptr::null_mut();
        let mut count = 0usize;
        assert_eq!(
            gfr_crypto_extract_metadata(GfrBuffer::empty(), &mut list, &mut count),
            GfrStatus::ErrorInvalidInput
        );
    }

    #[test]
    fn extract_metadata_rejects_invalid_utf8() {
        // `GfrBuffer::as_str` is the gate; a key block is text by definition.
        let bytes = [0xC3u8, 0x28, 0x00, 0xFF];
        let b = GfrBuffer {
            data: bytes.as_ptr(),
            len: bytes.len(),
        };
        let mut list = std::ptr::null_mut();
        let mut count = 0usize;
        assert_eq!(
            gfr_crypto_extract_metadata(b, &mut list, &mut count),
            GfrStatus::ErrorInvalidInput
        );
    }

    #[test]
    fn extract_metadata_round_trips_a_real_key() {
        let key = &keys::V4_SIGN;
        let mut list = std::ptr::null_mut();
        let mut count = 0usize;
        assert_eq!(
            gfr_crypto_extract_metadata(buf(&key.secret_armored), &mut list, &mut count),
            GfrStatus::Success
        );
        assert_eq!(count, 1);
        let meta = unsafe { &*list };
        let fpr = unsafe { CStr::from_ptr(meta.fpr) }.to_string_lossy().into_owned();
        assert_eq!(fpr.to_uppercase(), key.primary_fpr);
        assert_eq!(meta.subkey_count, 2);
        unsafe { crate::ffi::mem::gfr_free_metadata_array(list, count) };
    }

    #[test]
    fn extract_metadata_of_garbage_fails_without_leaking_an_array() {
        let mut list = std::ptr::null_mut();
        let mut count = 0usize;
        let status = gfr_crypto_extract_metadata(buf("not a key block"), &mut list, &mut count);
        assert_ne!(status, GfrStatus::Success);
        assert!(list.is_null(), "no array may be handed back on failure");
    }

    // -- extract_public_key ----------------------------------------------------

    #[test]
    fn extract_public_key_rejects_a_null_out_param() {
        assert_eq!(
            gfr_crypto_extract_public_key(
                buf(&keys::V4_SIGN.secret_armored),
                std::ptr::null_mut()
            ),
            GfrStatus::ErrorInvalidInput
        );
    }

    #[test]
    fn extract_public_key_strips_the_secret_material() {
        let mut out = std::ptr::null_mut();
        assert_eq!(
            gfr_crypto_extract_public_key(buf(&keys::V4_SIGN.secret_armored), &mut out),
            GfrStatus::Success
        );
        let block = take(out);
        assert!(block.contains("BEGIN PGP PUBLIC KEY BLOCK"));
        assert!(!block.contains("PRIVATE KEY BLOCK"));
    }

    #[test]
    fn extract_public_key_rejects_an_empty_buffer() {
        let mut out = std::ptr::null_mut();
        assert_eq!(
            gfr_crypto_extract_public_key(GfrBuffer::empty(), &mut out),
            GfrStatus::ErrorInvalidInput
        );
    }

    // -- get_recipients ---------------------------------------------------------

    #[test]
    fn get_recipients_rejects_null_arguments() {
        let mut list = std::ptr::null_mut();
        let mut count = 0usize;
        assert_eq!(
            gfr_crypto_get_recipients(std::ptr::null(), 0, &mut list, &mut count),
            GfrStatus::ErrorInvalidInput
        );
        assert_eq!(
            gfr_crypto_get_recipients(
                corpus::ENC_MULTI_RECIPIENT.as_ptr(),
                corpus::ENC_MULTI_RECIPIENT.len(),
                std::ptr::null_mut(),
                &mut count
            ),
            GfrStatus::ErrorInvalidInput
        );
    }

    #[test]
    fn get_recipients_lists_them_and_frees_cleanly() {
        let data = corpus::ENC_MULTI_RECIPIENT;
        let mut list = std::ptr::null_mut();
        let mut count = 0usize;
        assert_eq!(
            gfr_crypto_get_recipients(data.as_ptr(), data.len(), &mut list, &mut count),
            GfrStatus::Success
        );
        assert!(count >= 3, "the vector is encrypted to three certificates");
        crate::ffi::mem::gfr_crypto_free_recipients(list, count);
    }

    #[test]
    fn get_recipients_of_garbage_does_not_crash() {
        let data = corpus::GARBAGE;
        let mut list = std::ptr::null_mut();
        let mut count = 0usize;
        let status = gfr_crypto_get_recipients(data.as_ptr(), data.len(), &mut list, &mut count);
        if status == GfrStatus::Success {
            crate::ffi::mem::gfr_crypto_free_recipients(list, count);
        }
    }

    // -- export_merged_keys ------------------------------------------------------

    #[test]
    fn export_merged_keys_rejects_null_arguments() {
        let mut out = std::ptr::null_mut();
        assert_eq!(
            unsafe { gfr_export_merged_keys(std::ptr::null(), 0, false, &mut out) },
            GfrStatus::ErrorInvalidInput
        );
        let blocks = [CString::new("x").expect("no NUL")];
        let ptrs: Vec<*const c_char> = blocks.iter().map(|c| c.as_ptr()).collect();
        assert_eq!(
            unsafe { gfr_export_merged_keys(ptrs.as_ptr(), 1, false, std::ptr::null_mut()) },
            GfrStatus::ErrorInvalidInput
        );
    }

    #[test]
    fn export_merged_keys_combines_two_certificates() {
        let a = CString::new(keys::V4_SIGN.public_armored.as_str()).expect("no NUL");
        let b = CString::new(keys::V6_SIGN.public_armored.as_str()).expect("no NUL");
        let ptrs: Vec<*const c_char> = vec![a.as_ptr(), b.as_ptr()];
        let mut out = std::ptr::null_mut();
        assert_eq!(
            unsafe { gfr_export_merged_keys(ptrs.as_ptr(), ptrs.len(), false, &mut out) },
            GfrStatus::Success
        );
        let merged = take(out);
        assert_eq!(
            crate::key::extract_metadata_many_internal(&merged)
                .expect("metadata")
                .len(),
            2
        );
    }

    #[test]
    fn export_merged_keys_with_a_null_element_is_rejected() {
        let a = CString::new(keys::V4_SIGN.public_armored.as_str()).expect("no NUL");
        let ptrs: Vec<*const c_char> = vec![a.as_ptr(), std::ptr::null()];
        let mut out = std::ptr::null_mut();
        let status = unsafe { gfr_export_merged_keys(ptrs.as_ptr(), ptrs.len(), false, &mut out) };
        assert_ne!(status, GfrStatus::Success);
    }

    // -- delete_subkey / revoke_subkey ---------------------------------------------

    #[test]
    fn delete_subkey_rejects_null_arguments() {
        let fpr = CString::new("AABB").expect("no NUL");
        assert_eq!(
            gfr_crypto_delete_subkey(
                buf(&keys::V4_SIGN.secret_armored),
                std::ptr::null(),
                &mut std::ptr::null_mut()
            ),
            GfrStatus::ErrorInvalidInput
        );
        assert_eq!(
            gfr_crypto_delete_subkey(
                buf(&keys::V4_SIGN.secret_armored),
                fpr.as_ptr(),
                std::ptr::null_mut()
            ),
            GfrStatus::ErrorInvalidInput
        );
    }

    #[test]
    fn delete_subkey_removes_the_named_subkey() {
        let key = &keys::V4_SIGN;
        let fpr = CString::new(key.enc_subkey_fpr()).expect("no NUL");
        let mut out = std::ptr::null_mut();
        assert_eq!(
            gfr_crypto_delete_subkey(buf(&key.secret_armored), fpr.as_ptr(), &mut out),
            GfrStatus::Success
        );
        let block = take(out);
        let meta = crate::key::extract_metadata_many_internal(&block).expect("meta");
        assert_eq!(meta[0].subkeys.len(), 1);
    }

    #[test]
    fn delete_subkey_with_an_unknown_fingerprint_fails_and_nulls_the_out_param() {
        let fpr = CString::new("0000000000000000").expect("no NUL");
        let mut out = std::ptr::null_mut();
        let status =
            gfr_crypto_delete_subkey(buf(&keys::V4_SIGN.secret_armored), fpr.as_ptr(), &mut out);
        assert_ne!(status, GfrStatus::Success);
        assert!(out.is_null(), "a failure must not hand back a block");
    }

    // -- ecdh kdf params -------------------------------------------------------------

    #[test]
    fn get_ecdh_kdf_params_rejects_null_arguments() {
        let fpr = CString::new("AABB").expect("no NUL");
        assert_eq!(
            gfr_crypto_get_ecdh_kdf_params(
                buf(&keys::V4_SIGN.public_armored),
                fpr.as_ptr(),
                std::ptr::null_mut()
            ),
            GfrStatus::ErrorInvalidInput
        );
    }

    #[test]
    fn get_ecdh_kdf_params_returns_four_hex_octets() {
        let key = &keys::V4_SIGN;
        let fpr = CString::new(key.enc_subkey_fpr()).expect("no NUL");
        let mut out = std::ptr::null_mut();
        assert_eq!(
            gfr_crypto_get_ecdh_kdf_params(buf(&key.public_armored), fpr.as_ptr(), &mut out),
            GfrStatus::Success
        );
        let hex = take(out);
        assert_eq!(hex.len(), 8);
        assert!(hex.starts_with("0301"));
    }

    // -- revocation certificates -------------------------------------------------------

    #[test]
    fn generate_and_extract_a_revocation_certificate_round_trip() {
        let key = &keys::V4_SIGN;
        let mut cert_out = std::ptr::null_mut();
        assert_eq!(
            gfr_crypto_generate_key_rev_cert(
                0,
                buf(&key.secret_armored),
                GfrRevocationCode::Superseded,
                std::ptr::null(),
                crate::testutil::cb::pwd_correct,
                &mut cert_out,
            ),
            GfrStatus::Success
        );
        let cert = take(cert_out);

        let mut fpr_out = std::ptr::null_mut();
        assert_eq!(
            gfr_crypto_extract_rev_cert_target_fpr(buf(&cert), &mut fpr_out),
            GfrStatus::Success
        );
        assert_eq!(take(fpr_out).to_uppercase(), key.primary_fpr);
    }

    #[test]
    fn extract_rev_cert_target_fpr_rejects_a_non_revocation_block() {
        let mut out = std::ptr::null_mut();
        let status =
            gfr_crypto_extract_rev_cert_target_fpr(buf(&keys::V4_SIGN.public_armored), &mut out);
        assert_ne!(status, GfrStatus::Success);
        assert!(out.is_null());
    }

    #[test]
    fn extract_rev_cert_target_fpr_rejects_an_empty_buffer() {
        let mut out = std::ptr::null_mut();
        assert_eq!(
            gfr_crypto_extract_rev_cert_target_fpr(GfrBuffer::empty(), &mut out),
            GfrStatus::ErrorInvalidInput
        );
    }

    #[test]
    fn import_rev_cert_rejects_a_foreign_certificate() {
        // A revocation that does not verify under the base primary must never
        // be stored, however convincingly it names one.
        let foreign = crate::key::generate_key_rev_cert_internal(
            0,
            &keys::V6_SIGN.secret_armored,
            GfrRevocationCode::Compromised,
            None,
            None,
        )
        .expect("rev cert");

        let mut sec_out = std::ptr::null_mut();
        let mut pub_out = std::ptr::null_mut();
        let status = gfr_crypto_import_rev_cert(
            buf(&keys::V4_SIGN.secret_armored),
            buf(&foreign),
            &mut sec_out,
            &mut pub_out,
        );
        assert_ne!(status, GfrStatus::Success);
    }

    // -- merge_key_blocks -------------------------------------------------------------

    #[test]
    fn merge_key_blocks_rejects_null_out_params() {
        assert_eq!(
            gfr_crypto_merge_key_blocks(
                buf(&keys::V4_SIGN.secret_armored),
                buf(&keys::V4_SIGN.public_armored),
                std::ptr::null_mut(),
                std::ptr::null_mut(),
            ),
            GfrStatus::ErrorInvalidInput
        );
    }

    #[test]
    fn merge_key_blocks_keeps_the_secret_half() {
        let key = &keys::V4_SIGN;
        let mut sec_out = std::ptr::null_mut();
        let mut pub_out = std::ptr::null_mut();
        assert_eq!(
            gfr_crypto_merge_key_blocks(
                buf(&key.secret_armored),
                buf(&key.public_armored),
                &mut sec_out,
                &mut pub_out,
            ),
            GfrStatus::Success
        );
        let secret = take(sec_out);
        let public = take(pub_out);
        assert!(secret.contains("PRIVATE KEY BLOCK"));
        assert!(public.contains("PUBLIC KEY BLOCK"));
    }

    // -- modify_key_password -----------------------------------------------------------

    #[test]
    fn modify_key_password_rejects_a_null_out_param() {
        assert_eq!(
            gfr_crypto_modify_key_password(
                0,
                buf(&keys::V4_SIGN.secret_armored),
                std::ptr::null(),
                crate::testutil::cb::pwd_correct,
                std::ptr::null_mut(),
            ),
            GfrStatus::ErrorInvalidInput
        );
    }

    #[test]
    fn modify_key_password_rejects_an_empty_buffer() {
        let mut out = std::ptr::null_mut();
        assert_eq!(
            gfr_crypto_modify_key_password(
                0,
                GfrBuffer::empty(),
                std::ptr::null(),
                crate::testutil::cb::pwd_correct,
                &mut out,
            ),
            GfrStatus::ErrorInvalidInput
        );
    }

    // -- a sweep across every entry point -----------------------------------------------

    #[test]
    fn every_key_entry_point_rejects_an_empty_buffer_without_panicking() {
        // The systematic null/empty sweep: none of these may succeed, and
        // none may unwind across the FFI boundary.
        let fpr = CString::new("AABB").expect("no NUL");
        let mut out_c: *mut c_char = std::ptr::null_mut();
        let mut out_c2: *mut c_char = std::ptr::null_mut();

        let statuses = [
            gfr_crypto_extract_public_key(GfrBuffer::empty(), &mut out_c),
            gfr_crypto_delete_subkey(GfrBuffer::empty(), fpr.as_ptr(), &mut out_c),
            gfr_crypto_get_ecdh_kdf_params(GfrBuffer::empty(), fpr.as_ptr(), &mut out_c),
            gfr_crypto_extract_rev_cert_target_fpr(GfrBuffer::empty(), &mut out_c),
            gfr_crypto_merge_key_blocks(
                GfrBuffer::empty(),
                GfrBuffer::empty(),
                &mut out_c,
                &mut out_c2,
            ),
            gfr_crypto_modify_key_password(
                0,
                GfrBuffer::empty(),
                std::ptr::null(),
                crate::testutil::cb::pwd_correct,
                &mut out_c,
            ),
        ];
        for status in statuses {
            assert_ne!(status, GfrStatus::Success);
        }
    }

    #[test]
    fn key_entry_points_never_panic_on_adversarial_blocks() {
        let fpr = CString::new("AABB").expect("no NUL");
        for block in [
            "junk",
            corpus::TRUNCATED_ARMOR,
            corpus::CORRUPT_CRC,
            corpus::SIG_GOOD_CLEARTEXT,
        ] {
            let outcome = std::panic::catch_unwind(|| {
                let mut out: *mut c_char = std::ptr::null_mut();
                let mut list = std::ptr::null_mut();
                let mut count = 0usize;
                let _ = gfr_crypto_extract_metadata(buf(block), &mut list, &mut count);
                if !list.is_null() {
                    unsafe { crate::ffi::mem::gfr_free_metadata_array(list, count) };
                }
                let _ = gfr_crypto_extract_public_key(buf(block), &mut out);
                if !out.is_null() {
                    crate::ffi::mem::gfr_crypto_free_string(out);
                    out = std::ptr::null_mut();
                }
                let _ = gfr_crypto_delete_subkey(buf(block), fpr.as_ptr(), &mut out);
                if !out.is_null() {
                    crate::ffi::mem::gfr_crypto_free_string(out);
                    out = std::ptr::null_mut();
                }
                let _ = gfr_crypto_extract_rev_cert_target_fpr(buf(block), &mut out);
                if !out.is_null() {
                    crate::ffi::mem::gfr_crypto_free_string(out);
                }
            });
            assert!(outcome.is_ok(), "panicked on an adversarial block");
        }
    }
}
