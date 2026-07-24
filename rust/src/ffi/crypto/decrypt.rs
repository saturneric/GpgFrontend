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
    clear_last_error();
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
    clear_last_error();
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
    clear_last_error();
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
    clear_last_error();
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
    clear_last_error();
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

#[cfg(test)]
mod ffi_decrypt_tests {
    //! The six decrypt / decrypt-and-verify FFI entry points.

    use super::*;
    use crate::testutil::{cb, corpus, keys};
    use crate::types::GfrBuffer;
    use std::ffi::CString;
    use std::mem::MaybeUninit;

    fn kbuf(s: &str) -> GfrBuffer {
        GfrBuffer {
            data: s.as_ptr(),
            len: s.len(),
        }
    }

    /// Encrypt through the FFI so there is something real to decrypt.
    fn sealed_for(key: &keys::Fixture, payload: &[u8]) -> Vec<u8> {
        let name = CString::new("").expect("no NUL");
        let certs = [kbuf(&key.public_armored)];
        let mut out = MaybeUninit::<crate::types::GfrEncryptResultC>::zeroed();
        assert_eq!(
            crate::ffi::crypto::encrypt::gfr_crypto_encrypt_data(
                0,
                name.as_ptr(),
                payload.as_ptr(),
                payload.len(),
                certs.as_ptr(),
                certs.len(),
                false,
                out.as_mut_ptr(),
            ),
            GfrStatus::Success
        );
        let mut result = unsafe { out.assume_init() };
        let bytes =
            unsafe { std::slice::from_raw_parts(result.data, result.data_len) }.to_vec();
        crate::ffi::mem::gfr_crypto_free_encrypt_result(&mut result);
        bytes
    }

    #[test]
    fn decrypt_data_rejects_null_arguments() {
        let mut out = MaybeUninit::<GfrDecryptResultC>::zeroed();
        assert_eq!(
            gfr_crypto_decrypt_data(
                0,
                std::ptr::null(),
                0,
                cb::seckey_none,
                cb::pwd_correct,
                std::ptr::null_mut(),
                out.as_mut_ptr(),
            ),
            GfrStatus::ErrorInvalidInput
        );
        assert_eq!(
            gfr_crypto_decrypt_data(
                0,
                corpus::ENC_V1SEIPD_MDC.as_ptr(),
                corpus::ENC_V1SEIPD_MDC.len(),
                cb::seckey_none,
                cb::pwd_correct,
                std::ptr::null_mut(),
                std::ptr::null_mut(),
            ),
            GfrStatus::ErrorInvalidInput
        );
    }

    #[test]
    fn decrypt_data_round_trips_through_its_free() {
        let key = &*keys::V4_SIGN;
        let sealed = sealed_for(key, b"ffi decryption payload");
        cb::set_seckey_answer(&key.secret_armored);

        let mut out = MaybeUninit::<GfrDecryptResultC>::zeroed();
        assert_eq!(
            gfr_crypto_decrypt_data(
                0,
                sealed.as_ptr(),
                sealed.len(),
                cb::seckey_fetch,
                cb::pwd_correct,
                std::ptr::null_mut(),
                out.as_mut_ptr(),
            ),
            GfrStatus::Success
        );
        let mut result = unsafe { out.assume_init() };
        let plain = unsafe { std::slice::from_raw_parts(result.data, result.data_len) };
        assert_eq!(plain, b"ffi decryption payload");
        crate::ffi::mem::gfr_crypto_free_decrypt_result(&mut result);
    }

    #[test]
    fn decrypt_data_without_the_secret_key_fails() {
        let key = &*keys::V4_SIGN;
        let sealed = sealed_for(key, b"payload");
        cb::clear_seckey_answer();
        let mut out = MaybeUninit::<GfrDecryptResultC>::zeroed();
        assert_ne!(
            gfr_crypto_decrypt_data(
                0,
                sealed.as_ptr(),
                sealed.len(),
                cb::seckey_none,
                cb::pwd_correct,
                std::ptr::null_mut(),
                out.as_mut_ptr(),
            ),
            GfrStatus::Success
        );
    }

    #[test]
    fn decrypt_data_refuses_a_legacy_sed_packet() {
        // §13.7: unauthenticated plaintext must never be released.
        cb::set_seckey_answer(corpus::KEY1_SECRET);
        let mut out = MaybeUninit::<GfrDecryptResultC>::zeroed();
        assert_ne!(
            gfr_crypto_decrypt_data(
                0,
                corpus::ENC_SED_TAG9.as_ptr(),
                corpus::ENC_SED_TAG9.len(),
                cb::seckey_fetch,
                cb::pwd_corpus,
                std::ptr::null_mut(),
                out.as_mut_ptr(),
            ),
            GfrStatus::Success
        );
    }

    #[test]
    fn decrypt_data_never_panics_on_adversarial_input() {
        for vector in [
            corpus::GARBAGE,
            corpus::EMPTY,
            corpus::PKESK_NO_SEIPD,
            corpus::SIG_GOOD_DETACHED,
            corpus::TRUNCATED_ARMOR.as_bytes(),
        ] {
            let outcome = std::panic::catch_unwind(|| {
                cb::set_seckey_answer(corpus::KEY1_SECRET);
                let mut out = MaybeUninit::<GfrDecryptResultC>::zeroed();
                let status = gfr_crypto_decrypt_data(
                    0,
                    vector.as_ptr(),
                    vector.len(),
                    cb::seckey_fetch,
                    cb::pwd_corpus,
                    std::ptr::null_mut(),
                    out.as_mut_ptr(),
                );
                if status == GfrStatus::Success {
                    let mut r = unsafe { out.assume_init() };
                    crate::ffi::mem::gfr_crypto_free_decrypt_result(&mut r);
                }
            });
            assert!(outcome.is_ok(), "panicked on an adversarial vector");
        }
    }

    #[test]
    fn decrypt_and_verify_data_rejects_null_arguments() {
        let mut out = MaybeUninit::<GfrDecryptAndVerifyResultC>::zeroed();
        assert_eq!(
            gfr_crypto_decrypt_and_verify_data(
                0,
                std::ptr::null(),
                0,
                cb::seckey_none,
                cb::pwd_correct,
                cb::pubkey_none,
                std::ptr::null_mut(),
                out.as_mut_ptr(),
            ),
            GfrStatus::ErrorInvalidInput
        );
    }

    #[test]
    fn decrypt_and_verify_data_reports_both_halves() {
        let key = &*keys::V4_SIGN;
        let name = CString::new("").expect("no NUL");
        let certs = [kbuf(&key.public_armored)];
        let signers = [kbuf(&key.secret_armored)];
        let payload = b"signed and sealed via ffi";
        let mut enc_out = MaybeUninit::<crate::types::GfrEncryptAndSignResultC>::zeroed();

        assert_eq!(
            crate::ffi::crypto::encrypt::gfr_crypto_encrypt_and_sign_data(
                0,
                name.as_ptr(),
                payload.as_ptr(),
                payload.len(),
                certs.as_ptr(),
                1,
                signers.as_ptr(),
                1,
                cb::pwd_correct,
                false,
                enc_out.as_mut_ptr(),
            ),
            GfrStatus::Success
        );
        let mut sealed = unsafe { enc_out.assume_init() };
        let bytes =
            unsafe { std::slice::from_raw_parts(sealed.data, sealed.data_len) }.to_vec();
        crate::ffi::mem::gfr_crypto_free_encrypt_and_sign_result(&mut sealed);

        cb::set_seckey_answer(&key.secret_armored);
        cb::set_pubkey_answer(&key.public_armored);
        let mut out = MaybeUninit::<GfrDecryptAndVerifyResultC>::zeroed();
        assert_eq!(
            gfr_crypto_decrypt_and_verify_data(
                0,
                bytes.as_ptr(),
                bytes.len(),
                cb::seckey_fetch,
                cb::pwd_correct,
                cb::pubkey_fetch,
                std::ptr::null_mut(),
                out.as_mut_ptr(),
            ),
            GfrStatus::Success
        );
        let mut result = unsafe { out.assume_init() };
        let plain = unsafe { std::slice::from_raw_parts(result.data, result.data_len) };
        assert_eq!(plain, payload);
        assert_eq!(result.verify_meta.signature_count, 1);
        let sig = unsafe { &*result.verify_meta.signatures };
        assert_eq!(sig.status, crate::types::GfrSignatureStatus::Valid);
        crate::ffi::mem::gfr_crypto_free_decrypt_and_verify_result(&mut result);
    }

    #[test]
    fn decrypt_file_rejects_null_paths() {
        let mut out = MaybeUninit::<GfrDecryptResultC>::zeroed();
        assert_eq!(
            gfr_crypto_decrypt_file(
                0,
                std::ptr::null(),
                std::ptr::null(),
                false,
                cb::seckey_none,
                cb::pwd_correct,
                std::ptr::null_mut(),
                out.as_mut_ptr(),
            ),
            GfrStatus::ErrorInvalidInput
        );
    }

    #[test]
    fn a_file_round_trips_through_encrypt_and_decrypt() {
        let dir = tempfile::tempdir().expect("tempdir");
        let plain_path = dir.path().join("plain.bin");
        let cipher_path = dir.path().join("cipher.pgp");
        let out_path = dir.path().join("recovered.bin");
        std::fs::write(&plain_path, b"file round trip via ffi").expect("write");

        let in_c = CString::new(plain_path.to_string_lossy().as_ref()).expect("no NUL");
        let cipher_c = CString::new(cipher_path.to_string_lossy().as_ref()).expect("no NUL");
        let out_c = CString::new(out_path.to_string_lossy().as_ref()).expect("no NUL");
        let key = &*keys::V4_SIGN;
        let certs = [kbuf(&key.public_armored)];

        let mut enc_out = MaybeUninit::<crate::types::GfrEncryptResultC>::zeroed();
        assert_eq!(
            crate::ffi::crypto::encrypt::gfr_crypto_encrypt_file(
                0,
                in_c.as_ptr(),
                cipher_c.as_ptr(),
                certs.as_ptr(),
                1,
                false,
                enc_out.as_mut_ptr(),
            ),
            GfrStatus::Success
        );
        let mut enc = unsafe { enc_out.assume_init() };
        crate::ffi::mem::gfr_crypto_free_encrypt_result(&mut enc);

        cb::set_seckey_answer(&key.secret_armored);
        let mut dec_out = MaybeUninit::<GfrDecryptResultC>::zeroed();
        assert_eq!(
            gfr_crypto_decrypt_file(
                0,
                cipher_c.as_ptr(),
                out_c.as_ptr(),
                false,
                cb::seckey_fetch,
                cb::pwd_correct,
                std::ptr::null_mut(),
                dec_out.as_mut_ptr(),
            ),
            GfrStatus::Success
        );
        let mut dec = unsafe { dec_out.assume_init() };
        crate::ffi::mem::gfr_crypto_free_decrypt_result(&mut dec);

        assert_eq!(
            std::fs::read(&out_path).expect("read"),
            b"file round trip via ffi"
        );
    }

    #[test]
    fn decrypt_file_with_a_missing_input_fails() {
        let dir = tempfile::tempdir().expect("tempdir");
        let in_c = CString::new("/nonexistent/definitely/not/here").expect("no NUL");
        let out_c =
            CString::new(dir.path().join("o.bin").to_string_lossy().as_ref()).expect("no NUL");
        let mut out = MaybeUninit::<GfrDecryptResultC>::zeroed();
        assert_ne!(
            gfr_crypto_decrypt_file(
                0,
                in_c.as_ptr(),
                out_c.as_ptr(),
                false,
                cb::seckey_none,
                cb::pwd_correct,
                std::ptr::null_mut(),
                out.as_mut_ptr(),
            ),
            GfrStatus::Success
        );
    }

    #[test]
    fn decrypt_archive_rejects_null_paths() {
        let mut out = MaybeUninit::<GfrDecryptResultC>::zeroed();
        assert_eq!(
            gfr_crypto_decrypt_archive(
                0,
                std::ptr::null(),
                std::ptr::null(),
                false,
                cb::seckey_none,
                cb::pwd_correct,
                std::ptr::null_mut(),
                out.as_mut_ptr(),
            ),
            GfrStatus::ErrorInvalidInput
        );
    }

    #[test]
    fn a_directory_round_trips_through_encrypt_and_decrypt_archive() {
        let dir = tempfile::tempdir().expect("tempdir");
        let src = dir.path().join("src-dir");
        std::fs::create_dir(&src).expect("mkdir");
        std::fs::write(src.join("a.txt"), b"alpha").expect("write");
        let cipher_path = dir.path().join("dir.pgp");
        let dest = dir.path().join("out-dir");
        std::fs::create_dir(&dest).expect("mkdir");

        let src_c = CString::new(src.to_string_lossy().as_ref()).expect("no NUL");
        let cipher_c = CString::new(cipher_path.to_string_lossy().as_ref()).expect("no NUL");
        let dest_c = CString::new(dest.to_string_lossy().as_ref()).expect("no NUL");
        let key = &*keys::V4_SIGN;
        let certs = [kbuf(&key.public_armored)];

        let mut enc_out = MaybeUninit::<crate::types::GfrEncryptResultC>::zeroed();
        assert_eq!(
            crate::ffi::crypto::encrypt::gfr_crypto_encrypt_directory(
                0,
                src_c.as_ptr(),
                cipher_c.as_ptr(),
                certs.as_ptr(),
                1,
                false,
                enc_out.as_mut_ptr(),
            ),
            GfrStatus::Success
        );
        let mut enc = unsafe { enc_out.assume_init() };
        crate::ffi::mem::gfr_crypto_free_encrypt_result(&mut enc);

        cb::set_seckey_answer(&key.secret_armored);
        let mut dec_out = MaybeUninit::<GfrDecryptResultC>::zeroed();
        assert_eq!(
            gfr_crypto_decrypt_archive(
                0,
                cipher_c.as_ptr(),
                dest_c.as_ptr(),
                false,
                cb::seckey_fetch,
                cb::pwd_correct,
                std::ptr::null_mut(),
                dec_out.as_mut_ptr(),
            ),
            GfrStatus::Success
        );
        let mut dec = unsafe { dec_out.assume_init() };
        crate::ffi::mem::gfr_crypto_free_decrypt_result(&mut dec);

        assert_eq!(
            std::fs::read(dest.join("a.txt")).expect("read"),
            b"alpha",
            "the archive must be unpacked into the destination"
        );
    }
}
