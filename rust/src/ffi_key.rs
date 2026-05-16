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

use crate::crypto_stream::sniff_recipients;
use crate::err::clear_last_error;
use crate::key::{
    delete_subkey_internal, extract_rev_cert_target_fpr_internal, generate_key_rev_cert_internal,
    import_rev_cert_internal, merge_key_block_internal, modify_key_password_internal,
    revoke_subkey_internal,
};
use crate::key::{
    export_merged_public_keys, export_merged_secret_keys, extract_public_key_internal,
};
use crate::types::{
    GfrFreeCb, GfrKeyMetadataC, GfrPasswordFetchCb, GfrRecipientResultC, GfrRevocationCode,
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
    key_block: *const std::os::raw::c_char,
    out_metadata: *mut *mut GfrKeyMetadataC,
    out_metadata_count: *mut usize,
) -> GfrStatus {
    // Check for null pointers
    if key_block.is_null() || out_metadata.is_null() || out_metadata_count.is_null() {
        return GfrStatus::ErrorInvalidInput;
    }

    let result = std::panic::catch_unwind(|| -> Result<(), GfrStatus> {
        let block_str = unsafe { CStr::from_ptr(key_block) }
            .to_str()
            .map_err(|_| GfrStatus::ErrorInvalidInput)?;

        // Get the list of metadata from the internal function
        let meta_list = crate::key::extract_metadata_many_internal(block_str)?;

        // Prepare a vector to hold the C-compatible metadata structs
        let mut c_metadata_list = Vec::with_capacity(meta_list.len());

        for meta in meta_list {
            let c_fpr = CString::new(meta.fpr).map_err(|_| GfrStatus::ErrorInternal)?;
            let c_key_id = CString::new(meta.key_id).map_err(|_| GfrStatus::ErrorInternal)?;

            // Convert the new armored key blocks
            let c_pub_block = CString::new(meta.public_key_block)
                .map_err(|_| GfrStatus::ErrorInternal)?
                .into_raw();

            let c_sec_block = match meta.secret_key_block {
                Some(secret_str) => CString::new(secret_str)
                    .map_err(|_| GfrStatus::ErrorInternal)?
                    .into_raw(),
                None => std::ptr::null_mut(), // Safe to use NULL for Option::None in C
            };

            // Convert the subkeys Vec into a C-compatible array
            let mut c_subkeys = Vec::with_capacity(meta.subkeys.len());
            for sub in meta.subkeys {
                c_subkeys.push(GfrSubkeyMetadataC {
                    ver: sub.ver,
                    fpr: CString::new(sub.fpr)
                        .map_err(|_| GfrStatus::ErrorInternal)?
                        .into_raw(),
                    key_id: CString::new(sub.key_id)
                        .map_err(|_| GfrStatus::ErrorInternal)?
                        .into_raw(),
                    algo: sub.algo,
                    created_at: sub.created_at,
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
                    user_id: CString::new(uid.user_id)
                        .map_err(|_| GfrStatus::ErrorInternal)?
                        .into_raw(),
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
                user_id_count: user_id_count,
                algo: meta.algo,
                key_length: meta.key_length,
                created_at: meta.created_at,
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
    secret_block: *const c_char,
    out_public_block: *mut *mut c_char,
) -> GfrStatus {
    let result = catch_unwind(|| -> Result<(), GfrStatus> {
        // Null pointer check
        if secret_block.is_null() || out_public_block.is_null() {
            return Err(GfrStatus::ErrorInvalidInput);
        }

        // Safely convert C string to Rust string slice
        let block_str = unsafe { CStr::from_ptr(secret_block) }
            .to_str()
            .map_err(|_| GfrStatus::ErrorInvalidInput)?;

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
    // crate::error_handler::clear_last_error(); // Uncomment if using the new error system

    let result = std::panic::catch_unwind(|| -> Result<(), GfrStatus> {
        // 1. Check for null pointers
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
    // 1. Check for null pointers to prevent segmentation faults
    if keys_ptr.is_null() || out_armored_ptr.is_null() {
        return GfrStatus::ErrorInvalidInput;
    }

    // 2. Convert the C array of pointers into a Rust slice of pointers
    let c_str_ptrs = unsafe { slice::from_raw_parts(keys_ptr, keys_len) };
    let mut rust_strs = Vec::with_capacity(keys_len);

    // 3. Iterate through pointers, convert each to a Rust &str
    for &ptr in c_str_ptrs {
        if ptr.is_null() {
            return GfrStatus::ErrorInvalidInput;
        }

        match unsafe { CStr::from_ptr(ptr).to_str() } {
            Ok(s) => rust_strs.push(s),
            Err(_) => return GfrStatus::ErrorInvalidInput, // Fails if not valid UTF-8
        }
    }

    if secret {
        // 4. Call the core Rust function
        return match export_merged_secret_keys(&rust_strs) {
            Ok(armored_string) => {
                // 5. Convert the resulting Rust String into a null-terminated CString
                match CString::new(armored_string) {
                    Ok(c_str) => {
                        // Transfer ownership of the memory to C (prevents Rust from dropping it)
                        unsafe { *out_armored_ptr = c_str.into_raw() };

                        // Assuming GfrStatus has a Success variant.
                        // If your enum uses a different name for success, adjust this.
                        GfrStatus::Success
                    }
                    // Handle cases where the output string somehow contains a null byte
                    Err(_) => GfrStatus::ErrorArmorFailed,
                }
            }
            Err(status) => status, // Return the exact error status from the core function
        };
    }

    // 4. Call the core Rust function
    match export_merged_public_keys(&rust_strs) {
        Ok(armored_string) => {
            // 5. Convert the resulting Rust String into a null-terminated CString
            match CString::new(armored_string) {
                Ok(c_str) => {
                    // Transfer ownership of the memory to C (prevents Rust from dropping it)
                    unsafe { *out_armored_ptr = c_str.into_raw() };

                    // Assuming GfrStatus has a Success variant.
                    // If your enum uses a different name for success, adjust this.
                    GfrStatus::Success
                }
                // Handle cases where the output string somehow contains a null byte
                Err(_) => GfrStatus::ErrorArmorFailed,
            }
        }
        Err(status) => status, // Return the exact error status from the core function
    }
}

/// Change the passphrase protecting the subkey at `target_fpr` within `secret_key_block`.
///
/// The current passphrase and the new passphrase are both fetched via
/// `fetch_pwd_cb`/`free_cb`. On success `*out_secret_block` is set to a
/// heap-allocated updated armored secret key block. Free with `gfr_crypto_free_string`.
///
/// # Safety
/// `secret_key_block`, `target_fpr`, and `out_secret_block` must be non-null.
#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_modify_key_password(
    channel: i32,
    secret_key_block: *const c_char,
    target_fpr: *const c_char,
    fetch_pwd_cb: GfrPasswordFetchCb,
    free_cb: GfrFreeCb,
    out_secret_block: *mut *mut c_char,
) -> GfrStatus {
    clear_last_error();

    if !out_secret_block.is_null() {
        unsafe {
            *out_secret_block = std::ptr::null_mut();
        }
    }

    let result = catch_unwind(|| -> Result<(), GfrStatus> {
        if secret_key_block.is_null() || target_fpr.is_null() || out_secret_block.is_null() {
            return Err(GfrStatus::ErrorInvalidInput);
        }

        let block_str = unsafe { CStr::from_ptr(secret_key_block) }
            .to_str()
            .map_err(|_| GfrStatus::ErrorInvalidInput)?;

        let fpr_str = unsafe { CStr::from_ptr(target_fpr) }
            .to_str()
            .map_err(|_| GfrStatus::ErrorInvalidInput)?;

        let generated = modify_key_password_internal(
            channel,
            block_str,
            fpr_str,
            Some(fetch_pwd_cb),
            Some(free_cb),
        )?;

        let c_secret = CString::new(generated.secret).map_err(|_| GfrStatus::ErrorInternal)?;

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
    secret_key_block: *const c_char,
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
        if secret_key_block.is_null() || target_subkey_fpr.is_null() || out_secret_block.is_null() {
            return Err(GfrStatus::ErrorInvalidInput);
        }

        let block_str = unsafe { CStr::from_ptr(secret_key_block) }
            .to_str()
            .map_err(|_| GfrStatus::ErrorInvalidInput)?;

        let fpr_str = unsafe { CStr::from_ptr(target_subkey_fpr) }
            .to_str()
            .map_err(|_| GfrStatus::ErrorInvalidInput)?;

        let result = delete_subkey_internal(block_str, fpr_str)?;

        let secret_cstr = CString::new(result.secret).map_err(|_| GfrStatus::ErrorInternal)?;

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

/// Revoke the subkey at `target_subkey_fpr` within `secret_key_block`.
///
/// `reason_code` and `reason_text` (may be null) describe the revocation.
/// The passphrase is fetched via `fetch_pwd_cb`/`free_cb`. On success
/// `*out_secret_block` is set to a heap-allocated updated armored secret key block.
///
/// # Safety
/// `secret_key_block`, `target_subkey_fpr`, and `out_secret_block` must be non-null.
/// `reason_text` may be null.
#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_revoke_subkey(
    channel: i32,
    secret_key_block: *const c_char,
    target_subkey_fpr: *const c_char,
    reason_code: GfrRevocationCode,
    reason_text: *const c_char,
    fetch_pwd_cb: GfrPasswordFetchCb,
    free_cb: GfrFreeCb,
    out_secret_block: *mut *mut c_char,
) -> GfrStatus {
    clear_last_error();

    if !out_secret_block.is_null() {
        unsafe {
            *out_secret_block = std::ptr::null_mut();
        }
    }

    let result = catch_unwind(|| -> Result<(), GfrStatus> {
        if secret_key_block.is_null() || target_subkey_fpr.is_null() || out_secret_block.is_null() {
            return Err(GfrStatus::ErrorInvalidInput);
        }

        let block_str = unsafe { CStr::from_ptr(secret_key_block) }
            .to_str()
            .map_err(|_| GfrStatus::ErrorInvalidInput)?;

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
            Some(free_cb),
        )?;

        let secret_cstr = CString::new(result.secret).map_err(|_| GfrStatus::ErrorInternal)?;

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
/// The passphrase is fetched via `fetch_pwd_cb`/`free_cb`. On success
/// `*out_cert_block` is set to a heap-allocated armored revocation certificate.
/// Free with `gfr_crypto_free_string`.
///
/// # Safety
/// `secret_key_block` and `out_cert_block` must be non-null.
/// `reason_text` may be null.
#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_generate_key_rev_cert(
    channel: i32,
    secret_key_block: *const c_char,
    reason_code: GfrRevocationCode,
    reason_text: *const c_char,
    fetch_pwd_cb: GfrPasswordFetchCb,
    free_cb: GfrFreeCb,
    out_cert_block: *mut *mut c_char,
) -> GfrStatus {
    clear_last_error();

    if !out_cert_block.is_null() {
        unsafe {
            *out_cert_block = std::ptr::null_mut();
        }
    }

    let result = catch_unwind(|| -> Result<(), GfrStatus> {
        if secret_key_block.is_null() || out_cert_block.is_null() {
            return Err(GfrStatus::ErrorInvalidInput);
        }

        let block_str = unsafe { CStr::from_ptr(secret_key_block) }
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

        let cert = generate_key_rev_cert_internal(
            channel,
            block_str,
            reason_code,
            reason_text,
            Some(fetch_pwd_cb),
            Some(free_cb),
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
    base_block: *const c_char,
    incoming_block: *const c_char,
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
        if base_block.is_null()
            || incoming_block.is_null()
            || out_secret_block.is_null()
            || out_public_block.is_null()
        {
            return Err(GfrStatus::ErrorInvalidInput);
        }

        let base_str = unsafe { CStr::from_ptr(base_block) }
            .to_str()
            .map_err(|_| GfrStatus::ErrorInvalidInput)?;

        let incoming_str = unsafe { CStr::from_ptr(incoming_block) }
            .to_str()
            .map_err(|_| GfrStatus::ErrorInvalidInput)?;

        let merged = merge_key_block_internal(base_str, incoming_str)?;

        // public block 必须有
        let public_cstr = CString::new(merged.public).map_err(|_| GfrStatus::ErrorInternal)?;

        unsafe {
            *out_public_block = public_cstr.into_raw();
        }

        // secret block 可能为空；为空时保持 null，便于 C/C++ 判断
        if !merged.secret.is_empty() {
            let secret_cstr = CString::new(merged.secret).map_err(|_| GfrStatus::ErrorInternal)?;
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
    base_key_block: *const c_char,
    rev_cert_block: *const c_char,
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
        if base_key_block.is_null()
            || rev_cert_block.is_null()
            || out_secret_block.is_null()
            || out_public_block.is_null()
        {
            return Err(GfrStatus::ErrorInvalidInput);
        }

        let base_str = unsafe { CStr::from_ptr(base_key_block) }
            .to_str()
            .map_err(|_| GfrStatus::ErrorInvalidInput)?;

        let cert_str = unsafe { CStr::from_ptr(rev_cert_block) }
            .to_str()
            .map_err(|_| GfrStatus::ErrorInvalidInput)?;

        let merged = import_rev_cert_internal(base_str, cert_str)?;

        if !merged.secret.is_empty() {
            let secret_cstr = CString::new(merged.secret).map_err(|_| GfrStatus::ErrorInternal)?;
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
    rev_cert_block: *const c_char,
    out_fpr: *mut *mut c_char,
) -> GfrStatus {
    clear_last_error();

    if !out_fpr.is_null() {
        unsafe {
            *out_fpr = std::ptr::null_mut();
        }
    }

    let result = catch_unwind(|| -> Result<(), GfrStatus> {
        if rev_cert_block.is_null() || out_fpr.is_null() {
            return Err(GfrStatus::ErrorInvalidInput);
        }

        let cert_str = unsafe { CStr::from_ptr(rev_cert_block) }
            .to_str()
            .map_err(|_| GfrStatus::ErrorInvalidInput)?;

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
