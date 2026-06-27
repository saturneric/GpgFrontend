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

use crate::crypto::{self, decrypt_and_verify_archive_internal};
use crate::err::clear_last_error;
use crate::types::{
    GfrDecryptAndVerifyResultC, GfrDecryptResultC, GfrPasswordFetchCb, GfrRecipientResultC,
    GfrSecretKeyFetchCb, GfrSignatureResultC, GfrStatus,
};
use std::fs::File;
use std::slice;
use std::{
    ffi::{CStr, CString, c_char},
    panic::catch_unwind,
};

/// Decrypt an in-memory ciphertext buffer.
///
/// Secret keys and passphrases are fetched via the provided callbacks.
/// On success `out_result` is populated with the plaintext and recipient info.
/// Free with `gfr_crypto_free_decrypt_result`.
///
/// # Safety
/// `in_data` and `out_result` must be non-null; `in_data` must point to at least `in_len` bytes.
#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_decrypt_data(
    channel: i32,
    in_data: *const u8,
    in_len: usize,
    fetch_sec_key_cb: GfrSecretKeyFetchCb,
    fetch_pwd_cb: GfrPasswordFetchCb,
    user_data: *mut std::ffi::c_void,
    out_result: *mut GfrDecryptResultC,
) -> GfrStatus {
    let result = catch_unwind(|| -> Result<(), GfrStatus> {
        if in_data.is_null() || out_result.is_null() {
            return Err(GfrStatus::ErrorInvalidInput);
        }

        let data_slice = unsafe { slice::from_raw_parts(in_data, in_len) };

        // Perform decryption
        let internal_result = crate::crypto::decrypt_internal(
            channel,
            data_slice,
            Some(fetch_sec_key_cb),
            Some(fetch_pwd_cb),
            user_data,
        )?;

        // 1. Process Payload
        // Convert to a boxed slice so the backing allocation's capacity is
        // guaranteed to equal its length; `gfr_crypto_free_buffer` rebuilds the
        // Vec with `capacity == len`, which would be UB if `shrink_to_fit` left
        // excess capacity.
        let mut data_boxed = internal_result.data.into_boxed_slice();
        let data_ptr = data_boxed.as_mut_ptr();
        let data_len = data_boxed.len();
        std::mem::forget(data_boxed);

        // 2. Process Filename
        let c_filename = CString::new(internal_result.filename)
            .unwrap_or_default()
            .into_raw();

        // 3. Process Recipients
        let mut c_recipients = Vec::with_capacity(internal_result.recipients.len());
        for rec in internal_result.recipients {
            c_recipients.push(GfrRecipientResultC {
                key_id: CString::new(rec.key_id).unwrap_or_default().into_raw(),
                pub_algo: CString::new(rec.pub_algo).unwrap_or_default().into_raw(),
                status: rec.status,
            });
        }

        let mut boxed_recs = c_recipients.into_boxed_slice();
        let recs_ptr = boxed_recs.as_mut_ptr();
        let recs_count = boxed_recs.len();
        std::mem::forget(boxed_recs);

        // 4. Assign to C Struct
        unsafe {
            (*out_result).data = data_ptr;
            (*out_result).data_len = data_len;
            (*out_result).meta.filename = c_filename;
            (*out_result).meta.recipients = recs_ptr;
            (*out_result).meta.recipient_count = recs_count;
        }

        Ok(())
    });

    match result {
        Ok(Ok(_)) => GfrStatus::Success,
        Ok(Err(e)) => e,
        Err(_) => GfrStatus::ErrorPanic,
    }
}

/// Decrypt a ciphertext file, writing the plaintext to `out_file_path`.
///
/// `out_result.data` is null; only metadata is populated.
/// Free with `gfr_crypto_free_decrypt_result`.
///
/// # Safety
/// `in_file_path`, `out_file_path`, and `out_result` must be non-null.
#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_decrypt_file(
    channel: i32,
    in_file_path: *const c_char,
    out_file_path: *const c_char,
    ascii: bool,
    fetch_sec_key_cb: GfrSecretKeyFetchCb,
    fetch_pwd_cb: GfrPasswordFetchCb,
    user_data: *mut std::ffi::c_void,
    out_result: *mut GfrDecryptResultC,
) -> GfrStatus {
    let result = catch_unwind(|| -> Result<(), GfrStatus> {
        // 1. Check null pointers
        if in_file_path.is_null() || out_file_path.is_null() || out_result.is_null() {
            return Err(GfrStatus::ErrorInvalidInput);
        }

        // 2. Convert C strings to Rust string slices
        let in_path_str = unsafe { CStr::from_ptr(in_file_path) }
            .to_str()
            .map_err(|_| GfrStatus::ErrorInvalidInput)?;

        let out_path_str = unsafe { CStr::from_ptr(out_file_path) }
            .to_str()
            .map_err(|_| GfrStatus::ErrorInvalidInput)?;

        // 3. Open input and output files
        let in_file = File::open(in_path_str).map_err(|e| {
            log::error!("Failed to open input file: {}", e);
            GfrStatus::ErrorInvalidInput
        })?;

        let out_file = File::create(out_path_str).map_err(|e| {
            log::error!("Failed to create output file: {}", e);
            GfrStatus::ErrorInvalidInput
        })?;

        let stream_result = crypto::decrypt_and_verify_stream_internal(
            channel,
            in_file,
            out_file,
            ascii,
            Some(fetch_sec_key_cb),
            Some(fetch_pwd_cb),
            None, // fetch_pubkey_cb is not needed for decryption-only
            user_data,
        )?;

        // 5. Process Filename (extract from metadata)
        let c_filename = CString::new(stream_result.filename)
            .unwrap_or_default()
            .into_raw();

        // 6. Process Recipients array and leak it to C
        let mut c_recipients = Vec::with_capacity(stream_result.recipients.len());
        for rec in stream_result.recipients {
            c_recipients.push(GfrRecipientResultC {
                key_id: CString::new(rec.key_id).unwrap_or_default().into_raw(),
                pub_algo: CString::new(rec.pub_algo).unwrap_or_default().into_raw(),
                status: rec.status,
            });
        }

        let mut boxed_recs = c_recipients.into_boxed_slice();
        let recs_ptr = boxed_recs.as_mut_ptr();
        let recs_count = boxed_recs.len();
        std::mem::forget(boxed_recs); // Leak array to C

        // 7. Populate the output struct safely
        // For file streaming, we don't output byte buffers, so data is null.
        unsafe {
            (*out_result).data = std::ptr::null_mut(); // No in-memory payload
            (*out_result).data_len = 0;
            (*out_result).meta.filename = c_filename;
            (*out_result).meta.recipients = recs_ptr;
            (*out_result).meta.recipient_count = recs_count;
        }

        Ok(())
    });

    match result {
        Ok(Ok(_)) => GfrStatus::Success,
        Ok(Err(e)) => e,
        Err(_) => GfrStatus::ErrorPanic,
    }
}

/// Decrypt an encrypted archive file and extract its contents to `out_file_path`.
///
/// `out_result.data` is null; only metadata is populated.
/// Free with `gfr_crypto_free_decrypt_result`.
///
/// # Safety
/// `in_file_path`, `out_file_path`, and `out_result` must be non-null.
#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_decrypt_archive(
    channel: i32,
    in_file_path: *const c_char,
    out_file_path: *const c_char,
    ascii: bool,
    fetch_sec_key_cb: GfrSecretKeyFetchCb,
    fetch_pwd_cb: GfrPasswordFetchCb,
    user_data: *mut std::ffi::c_void,
    out_result: *mut GfrDecryptResultC,
) -> GfrStatus {
    let result = catch_unwind(|| -> Result<(), GfrStatus> {
        // 1. Check null pointers
        if in_file_path.is_null() || out_file_path.is_null() || out_result.is_null() {
            return Err(GfrStatus::ErrorInvalidInput);
        }

        // 2. Convert C strings to Rust string slices
        let in_path_str = unsafe { CStr::from_ptr(in_file_path) }
            .to_str()
            .map_err(|_| GfrStatus::ErrorInvalidInput)?;

        let out_path_str = unsafe { CStr::from_ptr(out_file_path) }
            .to_str()
            .map_err(|_| GfrStatus::ErrorInvalidInput)?;

        let stream_result = crypto::decrypt_and_verify_archive_internal(
            channel,
            in_path_str,
            out_path_str,
            ascii,
            Some(fetch_sec_key_cb),
            Some(fetch_pwd_cb),
            None, // fetch_pubkey_cb is not needed for decryption-only
            user_data,
        )?;

        // 5. Process Filename (extract from metadata)
        let c_filename = CString::new(stream_result.filename)
            .unwrap_or_default()
            .into_raw();

        // 6. Process Recipients array and leak it to C
        let mut c_recipients = Vec::with_capacity(stream_result.recipients.len());
        for rec in stream_result.recipients {
            c_recipients.push(GfrRecipientResultC {
                key_id: CString::new(rec.key_id).unwrap_or_default().into_raw(),
                pub_algo: CString::new(rec.pub_algo).unwrap_or_default().into_raw(),
                status: rec.status,
            });
        }

        let mut boxed_recs = c_recipients.into_boxed_slice();
        let recs_ptr = boxed_recs.as_mut_ptr();
        let recs_count = boxed_recs.len();
        std::mem::forget(boxed_recs); // Leak array to C

        // 7. Populate the output struct safely
        // For file streaming, we don't output byte buffers, so data is null.
        unsafe {
            (*out_result).data = std::ptr::null_mut(); // No in-memory payload
            (*out_result).data_len = 0;
            (*out_result).meta.filename = c_filename;
            (*out_result).meta.recipients = recs_ptr;
            (*out_result).meta.recipient_count = recs_count;
        }

        Ok(())
    });

    match result {
        Ok(Ok(_)) => GfrStatus::Success,
        Ok(Err(e)) => e,
        Err(_) => GfrStatus::ErrorPanic,
    }
}

/// Decrypt and verify an in-memory combined encrypt+sign buffer.
///
/// Keys and passphrases are fetched via the provided callbacks.
/// On success `out_result` contains plaintext, decrypt metadata, and verify metadata.
/// Free with `gfr_crypto_free_decrypt_and_verify_result`.
///
/// # Safety
/// `in_data` and `out_result` must be non-null; `in_data` must be at least `in_len` bytes.
#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_decrypt_and_verify_data(
    channel: i32,
    in_data: *const u8,
    in_len: usize,
    fetch_sec_key_cb: GfrSecretKeyFetchCb,
    fetch_pwd_cb: GfrPasswordFetchCb,
    fetch_pubkey_cb: crate::types::GfrPublicKeyFetchCb,
    user_data: *mut std::ffi::c_void,
    out_result: *mut GfrDecryptAndVerifyResultC,
) -> GfrStatus {
    let result = std::panic::catch_unwind(|| -> Result<(), GfrStatus> {
        if in_data.is_null() || out_result.is_null() {
            return Err(GfrStatus::ErrorInvalidInput);
        }

        let data_slice = unsafe { std::slice::from_raw_parts(in_data, in_len) };

        let internal_result = crate::crypto::decrypt_and_verify_internal(
            channel,
            data_slice,
            Some(fetch_sec_key_cb),
            Some(fetch_pwd_cb),
            Some(fetch_pubkey_cb),
            user_data,
        )?;

        // Convert to a boxed slice so the backing allocation's capacity is
        // guaranteed to equal its length; `gfr_crypto_free_buffer` rebuilds the
        // Vec with `capacity == len`, which would be UB if `shrink_to_fit` left
        // excess capacity.
        let mut data_boxed = internal_result.data.into_boxed_slice();
        let data_ptr = data_boxed.as_mut_ptr();
        let data_len = data_boxed.len();
        std::mem::forget(data_boxed);

        let c_filename = std::ffi::CString::new(internal_result.filename)
            .unwrap_or_default()
            .into_raw();

        let mut c_recipients = Vec::with_capacity(internal_result.recipients.len());
        for rec in internal_result.recipients {
            c_recipients.push(GfrRecipientResultC {
                key_id: std::ffi::CString::new(rec.key_id)
                    .unwrap_or_default()
                    .into_raw(),
                pub_algo: std::ffi::CString::new(rec.pub_algo)
                    .unwrap_or_default()
                    .into_raw(),
                status: rec.status,
            });
        }
        let mut boxed_recs = c_recipients.into_boxed_slice();
        let recs_ptr = boxed_recs.as_mut_ptr();
        let recs_count = boxed_recs.len();
        std::mem::forget(boxed_recs);

        let mut c_signatures = Vec::with_capacity(internal_result.signatures.len());
        for sig in internal_result.signatures {
            c_signatures.push(GfrSignatureResultC {
                issuer_fpr: std::ffi::CString::new(sig.fpr)
                    .unwrap_or_default()
                    .into_raw(),
                status: sig.status,
                created_at: sig.created_at,
                pub_algo: std::ffi::CString::new(sig.pub_algo)
                    .unwrap_or_default()
                    .into_raw(),
                hash_algo: std::ffi::CString::new(sig.hash_algo)
                    .unwrap_or_default()
                    .into_raw(),
                sig_type: sig.sig_type,
            });
        }
        let mut boxed_sigs = c_signatures.into_boxed_slice();
        let sigs_ptr = boxed_sigs.as_mut_ptr();
        let sigs_count = boxed_sigs.len();
        std::mem::forget(boxed_sigs);

        unsafe {
            (*out_result).data = data_ptr;
            (*out_result).data_len = data_len;
            (*out_result).decrypt_meta.filename = c_filename;
            (*out_result).decrypt_meta.recipients = recs_ptr;
            (*out_result).decrypt_meta.recipient_count = recs_count;
            (*out_result).verify_meta.is_verified = internal_result.is_verified;
            (*out_result).verify_meta.signatures = sigs_ptr;
            (*out_result).verify_meta.signature_count = sigs_count;
        }

        Ok(())
    });

    match result {
        Ok(Ok(_)) => GfrStatus::Success,
        Ok(Err(e)) => e,
        Err(_) => GfrStatus::ErrorPanic,
    }
}

/// Decrypt and verify a combined encrypt+sign file, writing plaintext to `out_file_path`.
///
/// `out_result.data` is null; metadata is populated. Keys and passphrases are
/// fetched via the provided callbacks. Free with `gfr_crypto_free_decrypt_and_verify_result`.
///
/// # Safety
/// `in_file_path`, `out_file_path`, and `out_result` must be non-null.
#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_decrypt_and_verify_file(
    channel: i32,
    in_file_path: *const c_char,
    out_file_path: *const c_char,
    ascii: bool,
    fetch_sec_key_cb: GfrSecretKeyFetchCb,
    fetch_pwd_cb: GfrPasswordFetchCb,
    fetch_pubkey_cb: crate::types::GfrPublicKeyFetchCb,
    user_data: *mut std::ffi::c_void,
    out_result: *mut GfrDecryptAndVerifyResultC,
) -> GfrStatus {
    let result = catch_unwind(|| -> Result<(), GfrStatus> {
        // 1. Check for null pointers
        if in_file_path.is_null() || out_file_path.is_null() || out_result.is_null() {
            return Err(GfrStatus::ErrorInvalidInput);
        }

        // 2. Convert C string paths
        let in_path_str = unsafe { std::ffi::CStr::from_ptr(in_file_path) }
            .to_str()
            .map_err(|_| GfrStatus::ErrorInvalidInput)?;

        let out_path_str = unsafe { std::ffi::CStr::from_ptr(out_file_path) }
            .to_str()
            .map_err(|_| GfrStatus::ErrorInvalidInput)?;

        // 3. Open input and output files for streaming
        let in_file = std::fs::File::open(in_path_str).map_err(|e| {
            log::error!("Failed to open input file: {}", e);
            GfrStatus::ErrorInvalidInput
        })?;

        let out_file = std::fs::File::create(out_path_str).map_err(|e| {
            log::error!("Failed to create output file: {}", e);
            GfrStatus::ErrorInvalidInput
        })?;

        // 4. Execute stream logic
        // This will read the encrypted file in chunks, decrypt it, decompress it,
        // verify inline signatures, and write the plaintext straight to out_file.
        let stream_result = crate::crypto::decrypt_and_verify_stream_internal(
            channel,
            in_file,
            out_file,
            ascii,
            Some(fetch_sec_key_cb),
            Some(fetch_pwd_cb),
            Some(fetch_pubkey_cb),
            user_data,
        )?;

        // 5. Process Filename (Extracted from literal data header)
        let c_filename = std::ffi::CString::new(stream_result.filename)
            .unwrap_or_default()
            .into_raw();

        // 6. Process Decrypt Meta (Recipients array)
        let mut c_recipients = Vec::with_capacity(stream_result.recipients.len());
        for rec in stream_result.recipients {
            c_recipients.push(GfrRecipientResultC {
                key_id: std::ffi::CString::new(rec.key_id)
                    .unwrap_or_default()
                    .into_raw(),
                pub_algo: std::ffi::CString::new(rec.pub_algo)
                    .unwrap_or_default()
                    .into_raw(),
                status: rec.status,
            });
        }
        let mut boxed_recs = c_recipients.into_boxed_slice();
        let recs_ptr = boxed_recs.as_mut_ptr();
        let recs_count = boxed_recs.len();
        std::mem::forget(boxed_recs); // Leak array to C

        // 7. Process Verify Meta (Signatures array)
        let mut c_signatures = Vec::with_capacity(stream_result.signatures.len());
        for sig in stream_result.signatures {
            c_signatures.push(GfrSignatureResultC {
                issuer_fpr: std::ffi::CString::new(sig.fpr)
                    .unwrap_or_default()
                    .into_raw(),
                status: sig.status,
                created_at: sig.created_at,
                pub_algo: std::ffi::CString::new(sig.pub_algo)
                    .unwrap_or_default()
                    .into_raw(),
                hash_algo: std::ffi::CString::new(sig.hash_algo)
                    .unwrap_or_default()
                    .into_raw(),
                sig_type: sig.sig_type,
            });
        }
        let mut boxed_sigs = c_signatures.into_boxed_slice();
        let sigs_ptr = boxed_sigs.as_mut_ptr();
        let sigs_count = boxed_sigs.len();
        std::mem::forget(boxed_sigs); // Leak array to C

        // 8. Populate C Struct safely
        unsafe {
            (*out_result).data = std::ptr::null_mut(); // No in-memory payload for file ops
            (*out_result).data_len = 0;

            // Fill decryption metadata
            (*out_result).decrypt_meta.filename = c_filename;
            (*out_result).decrypt_meta.recipients = recs_ptr;
            (*out_result).decrypt_meta.recipient_count = recs_count;

            // Fill verification metadata
            (*out_result).verify_meta.is_verified = stream_result.is_verified;
            (*out_result).verify_meta.signatures = sigs_ptr;
            (*out_result).verify_meta.signature_count = sigs_count;
        }

        Ok(())
    });

    match result {
        Ok(Ok(_)) => GfrStatus::Success,
        Ok(Err(e)) => e,
        Err(_) => GfrStatus::ErrorPanic,
    }
}

/// Decrypt and verify an encrypted archive, extracting its contents to `out_dir_path`.
///
/// `out_result.data` is null; metadata is populated. Free with
/// `gfr_crypto_free_decrypt_and_verify_result`.
///
/// # Safety
/// `in_file_path`, `out_dir_path`, and `out_result` must be non-null.
#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_decrypt_and_verify_archive(
    channel: i32,
    in_file_path: *const std::os::raw::c_char,
    out_dir_path: *const std::os::raw::c_char,
    ascii: bool,
    fetch_sec_key_cb: crate::types::GfrSecretKeyFetchCb,
    fetch_pwd_cb: crate::types::GfrPasswordFetchCb,
    fetch_pubkey_cb: crate::types::GfrPublicKeyFetchCb,
    user_data: *mut std::ffi::c_void,
    out_result: *mut GfrDecryptAndVerifyResultC,
) -> GfrStatus {
    clear_last_error();

    let result = std::panic::catch_unwind(|| -> Result<(), GfrStatus> {
        if in_file_path.is_null() || out_dir_path.is_null() || out_result.is_null() {
            return Err(GfrStatus::ErrorInvalidInput);
        }

        let in_path_str = unsafe { std::ffi::CStr::from_ptr(in_file_path) }
            .to_str()
            .map_err(|_| GfrStatus::ErrorInvalidInput)?;

        let out_dir_str = unsafe { std::ffi::CStr::from_ptr(out_dir_path) }
            .to_str()
            .map_err(|_| GfrStatus::ErrorInvalidInput)?;

        let internal_result = decrypt_and_verify_archive_internal(
            channel,
            in_path_str,
            out_dir_str,
            ascii,
            Some(fetch_sec_key_cb),
            Some(fetch_pwd_cb),
            Some(fetch_pubkey_cb),
            user_data,
        )?;

        let c_filename = std::ffi::CString::new(internal_result.filename)
            .unwrap_or_default()
            .into_raw();

        let mut c_recipients = Vec::with_capacity(internal_result.recipients.len());
        for rec in internal_result.recipients {
            c_recipients.push(GfrRecipientResultC {
                key_id: std::ffi::CString::new(rec.key_id)
                    .unwrap_or_default()
                    .into_raw(),
                pub_algo: std::ffi::CString::new(rec.pub_algo)
                    .unwrap_or_default()
                    .into_raw(),
                status: rec.status,
            });
        }
        let mut boxed_recs = c_recipients.into_boxed_slice();
        let recs_ptr = boxed_recs.as_mut_ptr();
        let recs_count = boxed_recs.len();
        std::mem::forget(boxed_recs);

        let mut c_signatures = Vec::with_capacity(internal_result.signatures.len());
        for sig in internal_result.signatures {
            c_signatures.push(GfrSignatureResultC {
                issuer_fpr: std::ffi::CString::new(sig.fpr)
                    .unwrap_or_default()
                    .into_raw(),
                status: sig.status,
                created_at: sig.created_at,
                pub_algo: std::ffi::CString::new(sig.pub_algo)
                    .unwrap_or_default()
                    .into_raw(),
                hash_algo: std::ffi::CString::new(sig.hash_algo)
                    .unwrap_or_default()
                    .into_raw(),
                sig_type: sig.sig_type,
            });
        }
        let mut boxed_sigs = c_signatures.into_boxed_slice();
        let sigs_ptr = boxed_sigs.as_mut_ptr();
        let sigs_count = boxed_sigs.len();
        std::mem::forget(boxed_sigs);

        unsafe {
            (*out_result).data = std::ptr::null_mut();
            (*out_result).data_len = 0;

            (*out_result).decrypt_meta.filename = c_filename;
            (*out_result).decrypt_meta.recipients = recs_ptr;
            (*out_result).decrypt_meta.recipient_count = recs_count;

            (*out_result).verify_meta.is_verified = internal_result.is_verified;
            (*out_result).verify_meta.signatures = sigs_ptr;
            (*out_result).verify_meta.signature_count = sigs_count;
        }

        Ok(())
    });

    match result {
        Ok(Ok(_)) => GfrStatus::Success,
        Ok(Err(e)) => e,
        Err(_) => GfrStatus::ErrorPanic,
    }
}
