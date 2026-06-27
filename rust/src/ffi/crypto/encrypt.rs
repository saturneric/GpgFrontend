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

use crate::crypto::{self, encrypt_and_sign_directory_internal};
use crate::types::{
    GfrBuffer, GfrEncryptAndSignResultC, GfrEncryptResultC, GfrInvalidRecipientC,
    GfrPasswordFetchCb, GfrSignatureResultC, GfrStatus,
};
use std::fs::File;
use std::path::Path;
use std::slice;
use std::{
    ffi::{CStr, CString, c_char},
    panic::catch_unwind,
};

/// Encrypt an in-memory buffer with the given public keys.
///
/// `name` is embedded as the filename hint in the literal data packet.
/// `pub_keys` is an array of `pub_keys_count` armored public key block strings.
/// On success `out_result` is populated; free with `gfr_crypto_free_encrypt_result`.
///
/// # Safety
/// `name`, `in_data`, `pub_keys`, and `out_result` must be non-null;
/// `in_data` must point to at least `in_len` bytes.
#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_encrypt_data(
    channel: i32,
    name: *const c_char,
    in_data: *const u8,
    in_len: usize,
    pub_keys: *const GfrBuffer,
    pub_keys_count: usize,
    ascii: bool,
    out_result: *mut GfrEncryptResultC,
) -> GfrStatus {
    let result = catch_unwind(|| -> Result<(), GfrStatus> {
        // Null pointer checks
        if name.is_null() || in_data.is_null() || pub_keys.is_null() || out_result.is_null() {
            return Err(GfrStatus::ErrorInvalidInput);
        }

        let name_str = unsafe { CStr::from_ptr(name) }
            .to_str()
            .map_err(|_| GfrStatus::ErrorInvalidInput)?;

        // Convert the plaintext C string to a Rust string slice
        let data_slice = unsafe { slice::from_raw_parts(in_data, in_len) };

        // Convert the C array of GfrBuffer into a Rust Vec<&str>
        let mut key_blocks = Vec::with_capacity(pub_keys_count);
        unsafe {
            let keys_slice = std::slice::from_raw_parts(pub_keys, pub_keys_count);
            for gfr_buf in keys_slice {
                let key_str = gfr_buf.as_str()?;
                key_blocks.push(key_str);
            }
        }

        // Perform the encryption using the updated internal function
        let internal_result =
            crate::crypto::encrypt_internal(channel, name_str, data_slice, &key_blocks, ascii)?;

        // 1. Process output payload data.
        // Boxed slice guarantees `capacity == len` for the allocation handed to
        // C; `gfr_crypto_free_buffer` reconstructs the Vec with `capacity == len`.
        let mut data_boxed = internal_result.data.into_boxed_slice();
        let data_ptr = data_boxed.as_mut_ptr();
        let data_len = data_boxed.len();
        std::mem::forget(data_boxed); // Leak payload to C

        // 2. Process invalid recipients array
        let mut c_invalid_recs = Vec::with_capacity(internal_result.invalid_recipients.len());
        for rec in internal_result.invalid_recipients {
            c_invalid_recs.push(GfrInvalidRecipientC {
                fpr: CString::new(rec.fpr).unwrap_or_default().into_raw(),
                reason: rec.reason,
            });
        }

        let mut boxed_recs = c_invalid_recs.into_boxed_slice();
        let recs_ptr = boxed_recs.as_mut_ptr();
        let recs_count = boxed_recs.len();
        std::mem::forget(boxed_recs); // Leak array to C

        // 3. Populate the output struct safely
        unsafe {
            (*out_result).data = data_ptr;
            (*out_result).data_len = data_len;
            (*out_result).meta.invalid_recipients = recs_ptr;
            (*out_result).meta.invalid_recipient_count = recs_count;
        }

        Ok(())
    });

    match result {
        Ok(Ok(_)) => GfrStatus::Success,
        Ok(Err(e)) => e,
        Err(_) => GfrStatus::ErrorPanic,
    }
}

/// Encrypt a file with the given public keys, writing ciphertext to `out_file_path`.
///
/// `out_result.data` is null on success; only metadata (invalid recipients) is populated.
/// Free with `gfr_crypto_free_encrypt_result`.
///
/// # Safety
/// `in_file_path`, `out_file_path`, `pub_keys`, and `out_result` must be non-null.
#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_encrypt_file(
    channel: i32,
    in_file_path: *const c_char,
    out_file_path: *const c_char,
    pub_keys: *const GfrBuffer,
    pub_keys_count: usize,
    ascii: bool,
    out_result: *mut GfrEncryptResultC,
) -> GfrStatus {
    let result = catch_unwind(|| -> Result<(), GfrStatus> {
        // 1. Check null pointers
        if in_file_path.is_null()
            || out_file_path.is_null()
            || pub_keys.is_null()
            || out_result.is_null()
        {
            return Err(GfrStatus::ErrorInvalidInput);
        }

        // 2. Convert C strings to Rust string slices
        let in_path_str = unsafe { CStr::from_ptr(in_file_path) }
            .to_str()
            .map_err(|_| GfrStatus::ErrorInvalidInput)?;

        let out_path_str = unsafe { CStr::from_ptr(out_file_path) }
            .to_str()
            .map_err(|_| GfrStatus::ErrorInvalidInput)?;

        // 3. Extract the filename from the input path to use as a PGP hint
        let filename_hint = Path::new(in_path_str)
            .file_name()
            .unwrap_or_default()
            .to_string_lossy();

        // 4. Convert the C array of GfrBuffer into a Rust Vec<&str>
        let mut key_blocks = Vec::with_capacity(pub_keys_count);
        unsafe {
            let keys_slice = std::slice::from_raw_parts(pub_keys, pub_keys_count);
            for gfr_buf in keys_slice {
                let key_str = gfr_buf.as_str()?;
                key_blocks.push(key_str);
            }
        }

        // 5. Open input and output files
        let in_file = File::open(in_path_str).map_err(|e| {
            log::error!("Failed to open input file: {}", e);
            GfrStatus::ErrorInvalidInput
        })?;

        let out_file = File::create(out_path_str).map_err(|e| {
            log::error!("Failed to create output file: {}", e);
            GfrStatus::ErrorInvalidInput
        })?;

        // 6. Perform the streaming encryption
        let stream_result = crypto::encrypt_stream_internal(
            channel,
            &filename_hint,
            in_file,
            out_file,
            &key_blocks,
            ascii,
        )?;

        // 7. Process invalid recipients array and leak it to C
        let mut c_invalid_recs = Vec::with_capacity(stream_result.invalid_recipients.len());
        for rec in stream_result.invalid_recipients {
            c_invalid_recs.push(GfrInvalidRecipientC {
                fpr: CString::new(rec.fpr).unwrap_or_default().into_raw(),
                reason: rec.reason,
            });
        }

        let mut boxed_recs = c_invalid_recs.into_boxed_slice();
        let recs_ptr = boxed_recs.as_mut_ptr();
        let recs_count = boxed_recs.len();
        std::mem::forget(boxed_recs); // Leak array to C

        // 8. Populate the output metadata struct safely
        // Note: For files, we only need to pass back the metadata.
        unsafe {
            (*out_result).data = std::ptr::null_mut(); // No in-memory data for file encryption
            (*out_result).data_len = 0;
            (*out_result).meta.invalid_recipients = recs_ptr;
            (*out_result).meta.invalid_recipient_count = recs_count;
        }

        Ok(())
    });

    match result {
        Ok(Ok(_)) => GfrStatus::Success,
        Ok(Err(e)) => e,
        Err(_) => GfrStatus::ErrorPanic,
    }
}

/// Pack a directory into a tar archive and encrypt it with the given public keys.
///
/// Output is written to `out_file_path`. `out_result.data` is null on success.
/// Free with `gfr_crypto_free_encrypt_result`.
///
/// # Safety
/// `in_dir_path`, `out_file_path`, `pub_keys`, and `out_result` must be non-null.
#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_encrypt_directory(
    channel: i32,
    in_dir_path: *const c_char,
    out_file_path: *const c_char,
    pub_keys: *const GfrBuffer,
    pub_keys_count: usize,
    ascii: bool,
    out_result: *mut GfrEncryptResultC,
) -> GfrStatus {
    let result = catch_unwind(|| -> Result<(), GfrStatus> {
        // 1. Check null pointers
        if in_dir_path.is_null()
            || out_file_path.is_null()
            || pub_keys.is_null()
            || out_result.is_null()
        {
            return Err(GfrStatus::ErrorInvalidInput);
        }

        // 2. Convert C strings to Rust string slices
        let in_path_str = unsafe { CStr::from_ptr(in_dir_path) }
            .to_str()
            .map_err(|_| GfrStatus::ErrorInvalidInput)?;

        let out_path_str = unsafe { CStr::from_ptr(out_file_path) }
            .to_str()
            .map_err(|_| GfrStatus::ErrorInvalidInput)?;

        // 4. Convert the C array of GfrBuffer into a Rust Vec<&str>
        let mut key_blocks = Vec::with_capacity(pub_keys_count);
        unsafe {
            let keys_slice = std::slice::from_raw_parts(pub_keys, pub_keys_count);
            for gfr_buf in keys_slice {
                let key_str = gfr_buf.as_str()?;
                key_blocks.push(key_str);
            }
        }

        // 6. Perform the streaming encryption
        let stream_result = crypto::encrypt_directory_internal(
            channel,
            in_path_str,
            out_path_str,
            &key_blocks,
            ascii,
        )?;

        // 7. Process invalid recipients array and leak it to C
        let mut c_invalid_recs = Vec::with_capacity(stream_result.invalid_recipients.len());
        for rec in stream_result.invalid_recipients {
            c_invalid_recs.push(GfrInvalidRecipientC {
                fpr: CString::new(rec.fpr).unwrap_or_default().into_raw(),
                reason: rec.reason,
            });
        }

        let mut boxed_recs = c_invalid_recs.into_boxed_slice();
        let recs_ptr = boxed_recs.as_mut_ptr();
        let recs_count = boxed_recs.len();
        std::mem::forget(boxed_recs); // Leak array to C

        // 8. Populate the output metadata struct safely
        // Note: For files, we only need to pass back the metadata.
        unsafe {
            (*out_result).data = std::ptr::null_mut(); // No in-memory data for file encryption
            (*out_result).data_len = 0;
            (*out_result).meta.invalid_recipients = recs_ptr;
            (*out_result).meta.invalid_recipient_count = recs_count;
        }

        Ok(())
    });

    match result {
        Ok(Ok(_)) => GfrStatus::Success,
        Ok(Err(e)) => e,
        Err(_) => GfrStatus::ErrorPanic,
    }
}

/// Encrypt and sign an in-memory buffer in a single operation.
///
/// `pub_keys` are the recipient keys; `secret_keys` are the signing keys.
/// On success `out_result` contains the ciphertext and combined metadata.
/// Free with `gfr_crypto_free_encrypt_and_sign_result`.
///
/// # Safety
/// `name`, `in_data`, `pub_keys`, `secret_keys`, and `out_result` must be non-null.
#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_encrypt_and_sign_data(
    channel: i32,
    name: *const c_char,
    in_data: *const u8,
    in_len: usize,
    pub_keys: *const GfrBuffer,
    pub_keys_count: usize,
    secret_keys: *const GfrBuffer,
    signers_count: usize,
    fetch_pwd_cb: GfrPasswordFetchCb,
    ascii: bool,
    out_result: *mut GfrEncryptAndSignResultC,
) -> GfrStatus {
    let result = catch_unwind(|| -> Result<(), GfrStatus> {
        if name.is_null()
            || in_data.is_null()
            || pub_keys.is_null()
            || secret_keys.is_null()
            || out_result.is_null()
        {
            return Err(GfrStatus::ErrorInvalidInput);
        }

        let name_str = unsafe { CStr::from_ptr(name) }.to_str().unwrap_or("");
        let data_slice = unsafe { slice::from_raw_parts(in_data, in_len) };

        // Safely extract public keys
        let mut pkey_blocks = Vec::with_capacity(pub_keys_count);
        unsafe {
            let pkeys_slice = slice::from_raw_parts(pub_keys, pub_keys_count);
            for gfr_buf in pkeys_slice {
                pkey_blocks.push(gfr_buf.as_str()?);
            }
        }

        let mut skey_blocks = Vec::with_capacity(signers_count);
        unsafe {
            let sk_slice = slice::from_raw_parts(secret_keys, signers_count);
            for gfr_buf in sk_slice {
                skey_blocks.push(gfr_buf.as_str()?);
            }
        }

        // Execute core logic
        let internal_result = crate::crypto::encrypt_and_sign_internal(
            channel,
            name_str,
            data_slice,
            &pkey_blocks,
            &skey_blocks,
            Some(fetch_pwd_cb),
            ascii,
        )?;

        // 1. Process data.
        // Boxed slice guarantees `capacity == len` for the allocation handed to C.
        let mut data_boxed = internal_result.data.into_boxed_slice();
        let data_ptr = data_boxed.as_mut_ptr();
        let data_len = data_boxed.len();
        std::mem::forget(data_boxed);

        // 2. Process Signatures
        let mut c_signatures = Vec::with_capacity(internal_result.signatures.len());
        for sig in internal_result.signatures {
            c_signatures.push(GfrSignatureResultC {
                issuer_fpr: CString::new(sig.fpr).unwrap_or_default().into_raw(),
                status: sig.status,
                created_at: sig.created_at,
                pub_algo: CString::new(sig.pub_algo).unwrap_or_default().into_raw(),
                hash_algo: CString::new(sig.hash_algo).unwrap_or_default().into_raw(),
                sig_type: sig.sig_type,
            });
        }
        let mut boxed_sigs = c_signatures.into_boxed_slice();
        let sigs_ptr = boxed_sigs.as_mut_ptr();
        let sigs_count = boxed_sigs.len();
        std::mem::forget(boxed_sigs);

        // 3. Process Invalid Recipients
        let mut c_invalid_recs = Vec::with_capacity(internal_result.invalid_recipients.len());
        for rec in internal_result.invalid_recipients {
            c_invalid_recs.push(GfrInvalidRecipientC {
                fpr: CString::new(rec.fpr).unwrap_or_default().into_raw(),
                reason: rec.reason,
            });
        }
        let mut boxed_recs = c_invalid_recs.into_boxed_slice();
        let recs_ptr = boxed_recs.as_mut_ptr();
        let recs_count = boxed_recs.len();
        std::mem::forget(boxed_recs);

        // 4. Assign to C struct
        unsafe {
            (*out_result).data = data_ptr;
            (*out_result).data_len = data_len;
            (*out_result).sign_meta.signatures = sigs_ptr;
            (*out_result).sign_meta.signature_count = sigs_count;
            (*out_result).encrypt_meta.invalid_recipients = recs_ptr;
            (*out_result).encrypt_meta.invalid_recipient_count = recs_count;
        }

        Ok(())
    });

    match result {
        Ok(Ok(_)) => GfrStatus::Success,
        Ok(Err(e)) => e,
        Err(_) => GfrStatus::ErrorPanic,
    }
}

/// Encrypt and sign a file in a single streaming operation.
///
/// Output is written to `out_file_path`. `out_result.data` is null.
/// Free with `gfr_crypto_free_encrypt_and_sign_result`.
///
/// # Safety
/// `in_file_path`, `out_file_path`, `pub_keys`, `secret_keys`, and `out_result` must be non-null.
#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_encrypt_and_sign_file(
    channel: i32,
    in_file_path: *const c_char,
    out_file_path: *const c_char,
    pub_keys: *const GfrBuffer,
    pub_keys_count: usize,
    secret_keys: *const GfrBuffer,
    signers_count: usize,
    fetch_pwd_cb: GfrPasswordFetchCb,
    ascii: bool,
    out_result: *mut GfrEncryptAndSignResultC,
) -> GfrStatus {
    let result = catch_unwind(|| -> Result<(), GfrStatus> {
        // 1. Check for null pointers
        if in_file_path.is_null()
            || out_file_path.is_null()
            || pub_keys.is_null()
            || secret_keys.is_null()
            || out_result.is_null()
        {
            return Err(GfrStatus::ErrorInvalidInput);
        }

        // 2. Convert C string paths
        let in_path_str = unsafe { std::ffi::CStr::from_ptr(in_file_path) }
            .to_str()
            .map_err(|_| GfrStatus::ErrorInvalidInput)?;

        let out_path_str = unsafe { std::ffi::CStr::from_ptr(out_file_path) }
            .to_str()
            .map_err(|_| GfrStatus::ErrorInvalidInput)?;

        // 3. Extract the filename from the input path to use as a PGP hint
        let filename_hint = std::path::Path::new(in_path_str)
            .file_name()
            .unwrap_or_default()
            .to_string_lossy();

        // 4. Safely extract public keys (Recipients)
        let mut pkey_blocks = Vec::with_capacity(pub_keys_count);
        unsafe {
            let pkeys_slice = std::slice::from_raw_parts(pub_keys, pub_keys_count);
            for gfr_buf in pkeys_slice {
                pkey_blocks.push(gfr_buf.as_str()?);
            }
        }

        // 5. Safely extract secret keys (Signers)
        let mut skey_blocks = Vec::with_capacity(signers_count);
        unsafe {
            let sk_slice = std::slice::from_raw_parts(secret_keys, signers_count);
            for gfr_buf in sk_slice {
                skey_blocks.push(gfr_buf.as_str()?);
            }
        }

        // 6. Open input and output files for streaming
        let in_file = std::fs::File::open(in_path_str).map_err(|e| {
            log::error!("Failed to open input file: {}", e);
            GfrStatus::ErrorInvalidInput
        })?;

        let out_file = std::fs::File::create(out_path_str).map_err(|e| {
            log::error!("Failed to create output file: {}", e);
            GfrStatus::ErrorInvalidInput
        })?;

        // 7. Execute core stream logic
        let stream_result = crate::crypto::encrypt_and_sign_stream_internal(
            channel,
            &filename_hint,
            in_file,
            out_file,
            &pkey_blocks,
            &skey_blocks,
            Some(fetch_pwd_cb),
            ascii,
        )?;

        // 8. Process Signatures array and leak it to C
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
        std::mem::forget(boxed_sigs);

        // 9. Process Invalid Recipients array and leak it to C
        let mut c_invalid_recs = Vec::with_capacity(stream_result.invalid_recipients.len());
        for rec in stream_result.invalid_recipients {
            c_invalid_recs.push(GfrInvalidRecipientC {
                fpr: std::ffi::CString::new(rec.fpr)
                    .unwrap_or_default()
                    .into_raw(),
                reason: rec.reason,
            });
        }
        let mut boxed_recs = c_invalid_recs.into_boxed_slice();
        let recs_ptr = boxed_recs.as_mut_ptr();
        let recs_count = boxed_recs.len();
        std::mem::forget(boxed_recs);

        // 10. Populate the output struct safely
        unsafe {
            (*out_result).data = std::ptr::null_mut(); // No in-memory payload for file ops
            (*out_result).data_len = 0;
            (*out_result).sign_meta.signatures = sigs_ptr;
            (*out_result).sign_meta.signature_count = sigs_count;
            (*out_result).encrypt_meta.invalid_recipients = recs_ptr;
            (*out_result).encrypt_meta.invalid_recipient_count = recs_count;
        }

        Ok(())
    });

    match result {
        Ok(Ok(_)) => GfrStatus::Success,
        Ok(Err(e)) => e,
        Err(_) => GfrStatus::ErrorPanic,
    }
}

/// Pack a directory into a tar archive, then encrypt and sign it in a single operation.
///
/// Output is written to `out_file_path`. `out_result.data` is null.
/// Free with `gfr_crypto_free_encrypt_and_sign_result`.
///
/// # Safety
/// `in_dir_path`, `out_file_path`, `pub_keys`, `secret_keys`, and `out_result` must be non-null.
#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_encrypt_and_sign_directory(
    channel: i32,
    in_dir_path: *const std::os::raw::c_char,
    out_file_path: *const std::os::raw::c_char,
    pub_keys: *const GfrBuffer,
    pub_keys_count: usize,
    secret_keys: *const GfrBuffer,
    signers_count: usize,
    fetch_pwd_cb: crate::types::GfrPasswordFetchCb,
    ascii: bool,
    out_result: *mut GfrEncryptAndSignResultC,
) -> GfrStatus {
    let result = std::panic::catch_unwind(|| -> Result<(), GfrStatus> {
        if in_dir_path.is_null()
            || out_file_path.is_null()
            || pub_keys.is_null()
            || secret_keys.is_null()
            || out_result.is_null()
        {
            return Err(GfrStatus::ErrorInvalidInput);
        }

        let in_path_str = unsafe { std::ffi::CStr::from_ptr(in_dir_path) }
            .to_str()
            .map_err(|_| GfrStatus::ErrorInvalidInput)?;

        let out_path_str = unsafe { std::ffi::CStr::from_ptr(out_file_path) }
            .to_str()
            .map_err(|_| GfrStatus::ErrorInvalidInput)?;

        let mut pkey_blocks = Vec::with_capacity(pub_keys_count);
        unsafe {
            let pkeys_slice = std::slice::from_raw_parts(pub_keys, pub_keys_count);
            for gfr_buf in pkeys_slice {
                pkey_blocks.push(gfr_buf.as_str()?);
            }
        }

        let mut skey_blocks = Vec::with_capacity(signers_count);
        unsafe {
            let sk_slice = std::slice::from_raw_parts(secret_keys, signers_count);
            for gfr_buf in sk_slice {
                skey_blocks.push(gfr_buf.as_str()?);
            }
        }

        let internal_result = encrypt_and_sign_directory_internal(
            channel,
            in_path_str,
            out_path_str,
            &pkey_blocks,
            &skey_blocks,
            Some(fetch_pwd_cb),
            ascii,
        )?;

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

        let mut c_invalid_recs = Vec::with_capacity(internal_result.invalid_recipients.len());
        for rec in internal_result.invalid_recipients {
            c_invalid_recs.push(GfrInvalidRecipientC {
                fpr: std::ffi::CString::new(rec.fpr)
                    .unwrap_or_default()
                    .into_raw(),
                reason: rec.reason,
            });
        }
        let mut boxed_recs = c_invalid_recs.into_boxed_slice();
        let recs_ptr = boxed_recs.as_mut_ptr();
        let recs_count = boxed_recs.len();
        std::mem::forget(boxed_recs);

        unsafe {
            (*out_result).data = std::ptr::null_mut();
            (*out_result).data_len = 0;
            (*out_result).sign_meta.signatures = sigs_ptr;
            (*out_result).sign_meta.signature_count = sigs_count;
            (*out_result).encrypt_meta.invalid_recipients = recs_ptr;
            (*out_result).encrypt_meta.invalid_recipient_count = recs_count;
        }

        Ok(())
    });

    match result {
        Ok(Ok(_)) => GfrStatus::Success,
        Ok(Err(e)) => e,
        Err(_) => GfrStatus::ErrorPanic,
    }
}

/// Symmetrically encrypt an in-memory buffer using a password.
///
/// The password is fetched via `fetch_pwd_cb`/`free_cb`. On success
/// `out_result` contains the ciphertext; `meta.invalid_recipients` is null.
/// Free with `gfr_crypto_free_encrypt_result`.
///
/// # Safety
/// `name`, `in_data`, and `out_result` must be non-null; `in_data` must be at least `in_len` bytes.
#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_encrypt_data_symmetric(
    channel: i32,
    name: *const c_char,
    in_data: *const u8,
    in_len: usize,
    ascii: bool,
    fetch_pwd_cb: GfrPasswordFetchCb,
    out_result: *mut GfrEncryptResultC,
) -> GfrStatus {
    let result = catch_unwind(|| -> Result<(), GfrStatus> {
        if name.is_null() || in_data.is_null() || out_result.is_null() {
            return Err(GfrStatus::ErrorInvalidInput);
        }

        let name_str = unsafe { CStr::from_ptr(name) }
            .to_str()
            .map_err(|_| GfrStatus::ErrorInvalidInput)?;

        let data_slice = unsafe { slice::from_raw_parts(in_data, in_len) };

        let mut out_buf = Vec::new();

        crypto::encrypt_stream_with_password_internal(
            channel,
            name_str,
            data_slice,
            &mut out_buf,
            Some(fetch_pwd_cb),
            ascii,
        )?;

        // Boxed slice guarantees `capacity == len` for the allocation handed to C.
        let mut data_boxed = out_buf.into_boxed_slice();
        let data_ptr = data_boxed.as_mut_ptr();
        let data_len = data_boxed.len();
        std::mem::forget(data_boxed);

        unsafe {
            (*out_result).data = data_ptr;
            (*out_result).data_len = data_len;
            (*out_result).meta.invalid_recipients = std::ptr::null_mut();
            (*out_result).meta.invalid_recipient_count = 0;
        }

        Ok(())
    });

    match result {
        Ok(Ok(_)) => GfrStatus::Success,
        Ok(Err(e)) => e,
        Err(_) => GfrStatus::ErrorPanic,
    }
}

/// Symmetrically encrypt a file using a password, writing ciphertext to `out_file_path`.
///
/// `out_result.data` is null. Free with `gfr_crypto_free_encrypt_result`.
///
/// # Safety
/// `in_file_path`, `out_file_path`, and `out_result` must be non-null.
#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_encrypt_file_symmetric(
    channel: i32,
    in_file_path: *const c_char,
    out_file_path: *const c_char,
    ascii: bool,
    fetch_pwd_cb: GfrPasswordFetchCb,
    out_result: *mut GfrEncryptResultC,
) -> GfrStatus {
    let result = catch_unwind(|| -> Result<(), GfrStatus> {
        // 1. Null pointer checks
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

        // 3. Extract filename hint from input path
        let filename_hint = Path::new(in_path_str)
            .file_name()
            .unwrap_or_default()
            .to_string_lossy();

        // 4. Open input and output files
        let in_file = File::open(in_path_str).map_err(|e| {
            log::error!("Failed to open input file: {}", e);
            GfrStatus::ErrorInvalidInput
        })?;

        let out_file = File::create(out_path_str).map_err(|e| {
            log::error!("Failed to create output file: {}", e);
            GfrStatus::ErrorInvalidInput
        })?;

        // 5. Execute streaming symmetric encryption
        crypto::encrypt_stream_with_password_internal(
            channel,
            &filename_hint,
            in_file,
            out_file,
            Some(fetch_pwd_cb),
            ascii,
        )?;

        // 6. Populate output metadata
        // Symmetric encryption has no recipients, so return empty metadata
        unsafe {
            (*out_result).data = std::ptr::null_mut();
            (*out_result).data_len = 0;
            (*out_result).meta.invalid_recipients = std::ptr::null_mut();
            (*out_result).meta.invalid_recipient_count = 0;
        }

        Ok(())
    });

    match result {
        Ok(Ok(_)) => GfrStatus::Success,
        Ok(Err(e)) => e,
        Err(_) => GfrStatus::ErrorPanic,
    }
}

/// Pack a directory into a tar archive and symmetrically encrypt it with a password.
///
/// Output is written to `out_file_path`. `out_result.data` is null.
/// Free with `gfr_crypto_free_encrypt_result`.
///
/// # Safety
/// `in_dir_path`, `out_file_path`, and `out_result` must be non-null.
#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_encrypt_directory_symmetric(
    channel: i32,
    in_dir_path: *const c_char,
    out_file_path: *const c_char,
    ascii: bool,
    fetch_pwd_cb: GfrPasswordFetchCb,
    out_result: *mut GfrEncryptResultC,
) -> GfrStatus {
    let result = catch_unwind(|| -> Result<(), GfrStatus> {
        if in_dir_path.is_null() || out_file_path.is_null() || out_result.is_null() {
            return Err(GfrStatus::ErrorInvalidInput);
        }

        let in_path_str = unsafe { CStr::from_ptr(in_dir_path) }
            .to_str()
            .map_err(|_| GfrStatus::ErrorInvalidInput)?;

        let out_path_str = unsafe { CStr::from_ptr(out_file_path) }
            .to_str()
            .map_err(|_| GfrStatus::ErrorInvalidInput)?;

        crypto::encrypt_directory_with_password_internal(
            channel,
            in_path_str,
            out_path_str,
            Some(fetch_pwd_cb),
            ascii,
        )?;

        unsafe {
            (*out_result).data = std::ptr::null_mut();
            (*out_result).data_len = 0;
            (*out_result).meta.invalid_recipients = std::ptr::null_mut();
            (*out_result).meta.invalid_recipient_count = 0;
        }

        Ok(())
    });

    match result {
        Ok(Ok(_)) => GfrStatus::Success,
        Ok(Err(e)) => e,
        Err(_) => GfrStatus::ErrorPanic,
    }
}
