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

//! FFI entry points for OpenPGP message and file encrypt/decrypt/sign/verify operations.
//!
//! Each operation has an in-memory variant (`*_data`) and a file-streaming
//! variant (`*_file` / `*_directory` / `*_archive`). For file variants the
//! `data`/`data_len` fields of the result struct are null/0 and output is
//! written directly to disk. All result structs must be freed with the
//! corresponding `gfr_crypto_free_*` function from `ffi_mem`.

use crate::crypto::get_signature_issuers_internal;
use crate::crypto_stream::{
    self, decrypt_and_verify_archive_internal, encrypt_and_sign_directory_internal,
};
use crate::err::clear_last_error;
use crate::types::{
    GfrDecryptAndVerifyResultC, GfrDecryptResultC, GfrEncryptAndSignResultC, GfrEncryptResultC,
    GfrFreeCb, GfrInvalidRecipientC, GfrPasswordFetchCb, GfrPublicKeyFetchCb, GfrRecipientResultC,
    GfrSecretKeyFetchCb, GfrSignMode, GfrSignResultC, GfrSignatureResultC, GfrStatus,
    GfrVerifyResultC,
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
    name: *const c_char,
    in_data: *const u8,
    in_len: usize,
    pub_keys: *const *const c_char,
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

        // Convert the C array of strings into a Rust Vec<&str>
        let mut key_blocks = Vec::with_capacity(pub_keys_count);
        unsafe {
            let keys_slice = std::slice::from_raw_parts(pub_keys, pub_keys_count);
            for &key_ptr in keys_slice {
                if key_ptr.is_null() {
                    return Err(GfrStatus::ErrorInvalidInput);
                }
                let key_str = CStr::from_ptr(key_ptr)
                    .to_str()
                    .map_err(|_| GfrStatus::ErrorInvalidInput)?;
                key_blocks.push(key_str);
            }
        }

        // Perform the encryption using the updated internal function
        let mut internal_result =
            crate::crypto::encrypt_internal(name_str, data_slice, &key_blocks, ascii)?;

        // 1. Process output payload data
        internal_result.data.shrink_to_fit();
        let data_ptr = internal_result.data.as_mut_ptr();
        let data_len = internal_result.data.len();
        std::mem::forget(internal_result.data); // Leak payload to C

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
    in_file_path: *const c_char,
    out_file_path: *const c_char,
    pub_keys: *const *const c_char,
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

        // 4. Convert the C array of strings into a Rust Vec<&str>
        let mut key_blocks = Vec::with_capacity(pub_keys_count);
        unsafe {
            let keys_slice = std::slice::from_raw_parts(pub_keys, pub_keys_count);
            for &key_ptr in keys_slice {
                if key_ptr.is_null() {
                    return Err(GfrStatus::ErrorInvalidInput);
                }
                let key_str = CStr::from_ptr(key_ptr)
                    .to_str()
                    .map_err(|_| GfrStatus::ErrorInvalidInput)?;
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
        let stream_result = crypto_stream::encrypt_stream_internal(
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
    in_dir_path: *const c_char,
    out_file_path: *const c_char,
    pub_keys: *const *const c_char,
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

        // 4. Convert the C array of strings into a Rust Vec<&str>
        let mut key_blocks = Vec::with_capacity(pub_keys_count);
        unsafe {
            let keys_slice = std::slice::from_raw_parts(pub_keys, pub_keys_count);
            for &key_ptr in keys_slice {
                if key_ptr.is_null() {
                    return Err(GfrStatus::ErrorInvalidInput);
                }
                let key_str = CStr::from_ptr(key_ptr)
                    .to_str()
                    .map_err(|_| GfrStatus::ErrorInvalidInput)?;
                key_blocks.push(key_str);
            }
        }

        // 6. Perform the streaming encryption
        let stream_result = crypto_stream::encrypt_directory_internal(
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
    free_cb: crate::types::GfrFreeCb,
    user_data: *mut std::ffi::c_void,
    out_result: *mut GfrDecryptResultC,
) -> GfrStatus {
    let result = catch_unwind(|| -> Result<(), GfrStatus> {
        if in_data.is_null() || out_result.is_null() {
            return Err(GfrStatus::ErrorInvalidInput);
        }

        let data_slice = unsafe { slice::from_raw_parts(in_data, in_len) };

        // Perform decryption
        let mut internal_result = crate::crypto::decrypt_internal(
            channel,
            data_slice,
            Some(fetch_sec_key_cb),
            Some(fetch_pwd_cb),
            Some(free_cb),
            user_data,
        )?;

        // 1. Process Payload
        internal_result.data.shrink_to_fit();
        let data_ptr = internal_result.data.as_mut_ptr();
        let data_len = internal_result.data.len();
        std::mem::forget(internal_result.data);

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
    free_cb: crate::types::GfrFreeCb,
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

        let stream_result = crypto_stream::decrypt_and_verify_stream_internal(
            channel,
            in_file,
            out_file,
            ascii,
            Some(fetch_sec_key_cb),
            Some(fetch_pwd_cb),
            None, // fetch_pubkey_cb is not needed for decryption-only
            Some(free_cb),
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
    free_cb: crate::types::GfrFreeCb,
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

        let stream_result = crypto_stream::decrypt_and_verify_archive_internal(
            channel,
            in_path_str,
            out_path_str,
            ascii,
            Some(fetch_sec_key_cb),
            Some(fetch_pwd_cb),
            None, // fetch_pubkey_cb is not needed for decryption-only
            Some(free_cb),
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

/// Sign an in-memory buffer with the given secret keys.
///
/// `mode` selects inline, clear-text, or detached signature format.
/// On success `out_result` contains the signed data and per-signature results.
/// Free with `gfr_crypto_free_sign_result`.
///
/// # Safety
/// `name`, `in_data`, `secret_keys`, and `out_result` must be non-null;
/// `in_data` must point to at least `in_len` bytes.
#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_sign_data(
    channel: i32,
    name: *const c_char,
    in_data: *const u8,
    in_len: usize,
    secret_keys: *const *const c_char,
    signers_count: usize,
    fetch_pwd_cb: GfrPasswordFetchCb,
    free_cb: GfrFreeCb,
    mode: GfrSignMode,
    ascii: bool,
    out_result: *mut GfrSignResultC,
) -> GfrStatus {
    let result = catch_unwind(|| -> Result<(), GfrStatus> {
        if name.is_null() || in_data.is_null() || secret_keys.is_null() || out_result.is_null() {
            return Err(GfrStatus::ErrorInvalidInput);
        }

        let name_str = unsafe { CStr::from_ptr(name) }.to_str().unwrap_or("");
        let data_slice = unsafe { slice::from_raw_parts(in_data, in_len) };

        let mut skey_blocks = Vec::with_capacity(signers_count);

        unsafe {
            let sk_slice = slice::from_raw_parts(secret_keys, signers_count);
            for &sk_ptr in sk_slice {
                if sk_ptr.is_null() {
                    return Err(GfrStatus::ErrorInvalidInput);
                }
                skey_blocks.push(
                    CStr::from_ptr(sk_ptr)
                        .to_str()
                        .map_err(|_| GfrStatus::ErrorInvalidInput)?,
                );
            }
        }

        // Perform the multi-signature and get the structured report
        let mut internal_result = crate::crypto::sign_internal(
            channel,
            name_str,
            data_slice,
            &skey_blocks,
            Some(fetch_pwd_cb),
            Some(free_cb),
            mode,
            ascii,
        )?;

        // 1. Process the output payload (data)
        internal_result.data.shrink_to_fit();
        let data_ptr = internal_result.data.as_mut_ptr();
        let data_len = internal_result.data.len();
        std::mem::forget(internal_result.data); // Leak payload to C

        // 2. Process the signatures array
        let mut c_signatures = Vec::with_capacity(internal_result.signatures.len());
        for sig in internal_result.signatures {
            c_signatures.push(GfrSignatureResultC {
                sig_type: mode,
                issuer_fpr: CString::new(sig.fpr).unwrap_or_default().into_raw(),
                status: sig.status,
                created_at: sig.created_at,
                pub_algo: CString::new(sig.pub_algo).unwrap_or_default().into_raw(),
                hash_algo: CString::new(sig.hash_algo).unwrap_or_default().into_raw(),
            });
        }

        let mut boxed_sigs = c_signatures.into_boxed_slice();
        let sigs_ptr = boxed_sigs.as_mut_ptr();
        let sigs_count = boxed_sigs.len();
        std::mem::forget(boxed_sigs); // Leak array to C

        // 3. Populate the output struct safely
        unsafe {
            (*out_result).data = data_ptr;
            (*out_result).data_len = data_len;
            (*out_result).meta.signatures = sigs_ptr;
            (*out_result).meta.signature_count = sigs_count;
        }

        Ok(())
    });

    match result {
        Ok(Ok(_)) => GfrStatus::Success,
        Ok(Err(e)) => e,
        Err(_) => GfrStatus::ErrorPanic,
    }
}

/// Sign a file with the given secret keys, writing the signed output to `out_file_path`.
///
/// `out_result.data` is null; only signature metadata is populated.
/// Free with `gfr_crypto_free_sign_result`.
///
/// # Safety
/// `in_file_path`, `out_file_path`, `secret_keys`, and `out_result` must be non-null.
#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_sign_file(
    channel: i32,
    in_file_path: *const c_char,
    out_file_path: *const c_char,
    secret_keys: *const *const c_char,
    signers_count: usize,
    fetch_pwd_cb: GfrPasswordFetchCb,
    free_cb: GfrFreeCb,
    mode: GfrSignMode,
    ascii: bool,
    out_result: *mut GfrSignResultC,
) -> GfrStatus {
    let result = catch_unwind(|| -> Result<(), GfrStatus> {
        // 1. Check null pointers
        if in_file_path.is_null()
            || out_file_path.is_null()
            || secret_keys.is_null()
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

        // 4. Safely extract secret keys
        let mut skey_blocks = Vec::with_capacity(signers_count);
        unsafe {
            let sk_slice = slice::from_raw_parts(secret_keys, signers_count);
            for &sk_ptr in sk_slice {
                if sk_ptr.is_null() {
                    return Err(GfrStatus::ErrorInvalidInput);
                }
                skey_blocks.push(
                    CStr::from_ptr(sk_ptr)
                        .to_str()
                        .map_err(|_| GfrStatus::ErrorInvalidInput)?,
                );
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

        // 6. Perform the streaming signature
        let stream_result = crate::crypto_stream::sign_stream_internal(
            channel,
            &filename_hint,
            in_file,
            out_file,
            &skey_blocks,
            Some(fetch_pwd_cb),
            Some(free_cb),
            mode,
            ascii,
        )?;

        // 7. Process the signatures array
        let mut c_signatures = Vec::with_capacity(stream_result.signatures.len());
        for sig in stream_result.signatures {
            c_signatures.push(GfrSignatureResultC {
                sig_type: mode,
                issuer_fpr: CString::new(sig.fpr).unwrap_or_default().into_raw(),
                status: sig.status,
                created_at: sig.created_at,
                pub_algo: CString::new(sig.pub_algo).unwrap_or_default().into_raw(),
                hash_algo: CString::new(sig.hash_algo).unwrap_or_default().into_raw(),
            });
        }

        let mut boxed_sigs = c_signatures.into_boxed_slice();
        let sigs_ptr = boxed_sigs.as_mut_ptr();
        let sigs_count = boxed_sigs.len();
        std::mem::forget(boxed_sigs); // Leak array to C

        // 8. Populate the output struct safely
        // Note: For files, we only need to pass back the metadata.
        unsafe {
            (*out_result).data = std::ptr::null_mut(); // No in-memory data for file signing
            (*out_result).data_len = 0;
            (*out_result).meta.signatures = sigs_ptr;
            (*out_result).meta.signature_count = sigs_count;
        }

        Ok(())
    });

    match result {
        Ok(Ok(_)) => GfrStatus::Success,
        Ok(Err(e)) => e,
        Err(_) => GfrStatus::ErrorPanic,
    }
}

/// Verify the signature(s) on an in-memory data buffer.
///
/// For detached mode `sig_data`/`sig_len` must point to the separate signature.
/// Public keys are fetched on demand via `fetch_pubkey_cb`/`free_cb`.
/// On success `out_result` contains the verified payload and per-signature status.
/// Free with `gfr_crypto_free_verify_result`.
///
/// # Safety
/// `in_data` and `out_result` must be non-null. `sig_data` may be null for
/// inline/clear-text modes.
#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_verify_data(
    in_data: *const u8,
    in_len: usize,
    sig_data: *const u8,
    sig_len: usize,
    fetch_pubkey_cb: crate::types::GfrPublicKeyFetchCb,
    free_cb: crate::types::GfrFreeCb,
    user_data: *mut std::ffi::c_void,
    mode: GfrSignMode,
    out_result: *mut GfrVerifyResultC,
) -> GfrStatus {
    let result = catch_unwind(|| -> Result<(), GfrStatus> {
        // Check for null pointers
        if in_data.is_null() || out_result.is_null() {
            return Err(GfrStatus::ErrorInvalidInput);
        }

        // Convert raw pointers to Rust slices safely
        let data_slice = unsafe { std::slice::from_raw_parts(in_data, in_len) };
        let sig_slice = if !sig_data.is_null() && sig_len > 0 {
            unsafe { std::slice::from_raw_parts(sig_data, sig_len) }
        } else {
            &[]
        };

        // Call the updated internal function with callbacks instead of a static array
        let mut internal_result = crate::crypto::verify_internal(
            data_slice,
            sig_slice,
            mode,
            Some(fetch_pubkey_cb),
            Some(free_cb),
            user_data,
        )?;

        // 1. Process the extracted payload (data)
        internal_result.data.shrink_to_fit();
        let data_ptr = internal_result.data.as_mut_ptr();
        let data_len = internal_result.data.len();
        std::mem::forget(internal_result.data); // Leak payload to C

        // 2. Process the signatures array
        let mut c_signatures = Vec::with_capacity(internal_result.signatures.len());
        for sig in internal_result.signatures {
            let c_fpr = std::ffi::CString::new(sig.fpr)
                .unwrap_or_default()
                .into_raw();
            let c_pub_algo = std::ffi::CString::new(sig.pub_algo)
                .unwrap_or_default()
                .into_raw();
            let c_hash_algo = std::ffi::CString::new(sig.hash_algo)
                .unwrap_or_default()
                .into_raw();

            c_signatures.push(GfrSignatureResultC {
                sig_type: sig.sig_type,
                issuer_fpr: c_fpr,
                status: sig.status,
                created_at: sig.created_at,
                pub_algo: c_pub_algo,
                hash_algo: c_hash_algo,
            });
        }

        let mut boxed_sigs = c_signatures.into_boxed_slice();
        let sigs_ptr = boxed_sigs.as_mut_ptr();
        let sigs_count = boxed_sigs.len();
        std::mem::forget(boxed_sigs); // Leak array to C

        // 3. Populate the output struct safely
        unsafe {
            (*out_result).data = data_ptr;
            (*out_result).data_len = data_len;
            (*out_result).meta.signatures = sigs_ptr;
            (*out_result).meta.signature_count = sigs_count;
            (*out_result).meta.is_verified = internal_result.is_verified;
        }

        Ok(())
    });

    match result {
        Ok(Ok(_)) => GfrStatus::Success,
        Ok(Err(e)) => e,
        Err(_) => GfrStatus::ErrorPanic,
    }
}

/// Verify the signature(s) on a file.
///
/// For detached mode `sig_file_path` must point to the signature file.
/// For inline/clear-text modes `out_file_path` (may be null) receives the
/// extracted plaintext. Public keys are fetched via `fetch_pubkey_cb`/`free_cb`.
/// Free with `gfr_crypto_free_verify_result`.
///
/// # Safety
/// `in_file_path` and `out_result` must be non-null. `sig_file_path` and
/// `out_file_path` may be null depending on `mode`.
#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_verify_file(
    in_file_path: *const c_char,
    sig_file_path: *const c_char, // Used for Detached mode (.sig file)
    out_file_path: *const c_char, // Optional: Extracted plaintext output for Inline mode
    fetch_pubkey_cb: GfrPublicKeyFetchCb,
    free_cb: GfrFreeCb,
    user_data: *mut std::ffi::c_void,
    mode: GfrSignMode,
    out_result: *mut GfrVerifyResultC,
) -> GfrStatus {
    let result = catch_unwind(|| -> Result<(), GfrStatus> {
        if in_file_path.is_null() || out_result.is_null() {
            return Err(GfrStatus::ErrorInvalidInput);
        }

        let in_path_str = unsafe { std::ffi::CStr::from_ptr(in_file_path) }
            .to_str()
            .unwrap_or("");

        let sig_path_str = if !sig_file_path.is_null() {
            unsafe { std::ffi::CStr::from_ptr(sig_file_path) }
                .to_str()
                .unwrap_or("")
        } else {
            ""
        };

        let out_path_str = if !out_file_path.is_null() {
            unsafe { std::ffi::CStr::from_ptr(out_file_path) }
                .to_str()
                .unwrap_or("")
        } else {
            ""
        };

        let mut out_data_ptr: *mut u8 = std::ptr::null_mut();
        let mut out_data_len: usize = 0;
        let c_signatures;
        let is_verified;

        match mode {
            // ---------------------------------------------------------
            // Detached: Use stream implementation for large payloads
            // ---------------------------------------------------------
            GfrSignMode::Detached => {
                if sig_path_str.is_empty() {
                    return Err(GfrStatus::ErrorInvalidInput);
                }

                let in_file = File::open(in_path_str).map_err(|e| {
                    log::error!("Failed to open input file: {}", e);
                    GfrStatus::ErrorInvalidInput
                })?;

                let sig_data =
                    std::fs::read(sig_path_str).map_err(|_| GfrStatus::ErrorInvalidInput)?;

                let stream_result = crate::crypto_stream::verify_detached_stream_internal(
                    in_file,
                    &sig_data,
                    Some(fetch_pubkey_cb),
                    Some(free_cb),
                    user_data,
                )?;

                is_verified = stream_result.is_verified;
                c_signatures = stream_result.signatures;
            }

            // ---------------------------------------------------------
            // Inline / ClearText: Use non-stream fallback to extract payload
            // ---------------------------------------------------------
            GfrSignMode::Inline | GfrSignMode::ClearText => {
                let in_data =
                    std::fs::read(in_path_str).map_err(|_| GfrStatus::ErrorInvalidInput)?;

                let mut internal_result = crate::crypto::verify_internal(
                    &in_data,
                    &[], // sig_data is not needed for inline
                    mode,
                    Some(fetch_pubkey_cb),
                    Some(free_cb),
                    user_data,
                )?;

                is_verified = internal_result.is_verified;
                c_signatures = internal_result.signatures;

                if !out_path_str.is_empty() {
                    std::fs::write(out_path_str, &internal_result.data).map_err(|e| {
                        log::error!("Failed to write inline extracted data to file: {}", e);
                        GfrStatus::ErrorInternal
                    })?;
                } else {
                    internal_result.data.shrink_to_fit();
                    out_data_ptr = internal_result.data.as_mut_ptr();
                    out_data_len = internal_result.data.len();
                    std::mem::forget(internal_result.data);
                }
            }
        }

        // Convert signatures array for C
        let mut c_sigs_c = Vec::with_capacity(c_signatures.len());
        for sig in c_signatures {
            c_sigs_c.push(GfrSignatureResultC {
                sig_type: sig.sig_type,
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
            });
        }

        let mut boxed_sigs = c_sigs_c.into_boxed_slice();
        let sigs_ptr = boxed_sigs.as_mut_ptr();
        let sigs_count = boxed_sigs.len();
        std::mem::forget(boxed_sigs);

        unsafe {
            (*out_result).data = out_data_ptr;
            (*out_result).data_len = out_data_len;
            (*out_result).meta.signatures = sigs_ptr;
            (*out_result).meta.signature_count = sigs_count;
            (*out_result).meta.is_verified = is_verified;
        }

        Ok(())
    });

    match result {
        Ok(Ok(_)) => GfrStatus::Success,
        Ok(Err(e)) => e,
        Err(_) => GfrStatus::ErrorPanic,
    }
}

/// Extract issuer fingerprints from a signed data buffer as a comma-separated string.
///
/// On success `*out_issuers` is set to a heap-allocated CSV string of fingerprints.
/// Free with `gfr_crypto_free_string`.
///
/// # Safety
/// `in_data` and `out_issuers` must be non-null; `in_data` must be at least `in_len` bytes.
#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_get_signature_issuers(
    in_data: *const u8,
    in_len: usize,
    out_issuers: *mut *mut c_char,
) -> GfrStatus {
    let result = catch_unwind(|| -> Result<(), GfrStatus> {
        if in_data.is_null() || out_issuers.is_null() {
            return Err(GfrStatus::ErrorInvalidInput);
        }

        let data_slice = unsafe { std::slice::from_raw_parts(in_data, in_len) };
        let (_, issuers_csv) = get_signature_issuers_internal(data_slice)?;

        let c_str = CString::new(issuers_csv).map_err(|_| GfrStatus::ErrorInternal)?;
        unsafe {
            *out_issuers = c_str.into_raw();
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
    pub_keys: *const *const c_char,
    pub_keys_count: usize,
    secret_keys: *const *const c_char,
    signers_count: usize,
    fetch_pwd_cb: GfrPasswordFetchCb,
    free_cb: GfrFreeCb,
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
            for &key_ptr in pkeys_slice {
                if key_ptr.is_null() {
                    return Err(GfrStatus::ErrorInvalidInput);
                }
                pkey_blocks.push(CStr::from_ptr(key_ptr).to_str().unwrap_or(""));
            }
        }

        let mut skey_blocks = Vec::with_capacity(signers_count);
        unsafe {
            let sk_slice = slice::from_raw_parts(secret_keys, signers_count);
            for &sk_ptr in sk_slice {
                if sk_ptr.is_null() {
                    return Err(GfrStatus::ErrorInvalidInput);
                }
                skey_blocks.push(CStr::from_ptr(sk_ptr).to_str().unwrap_or(""));
            }
        }

        // Execute core logic
        let mut internal_result = crate::crypto::encrypt_and_sign_internal(
            channel,
            name_str,
            data_slice,
            &pkey_blocks,
            &skey_blocks,
            Some(fetch_pwd_cb),
            Some(free_cb),
            ascii,
        )?;

        // 1. Process data
        internal_result.data.shrink_to_fit();
        let data_ptr = internal_result.data.as_mut_ptr();
        let data_len = internal_result.data.len();
        std::mem::forget(internal_result.data);

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
    pub_keys: *const *const c_char,
    pub_keys_count: usize,
    secret_keys: *const *const c_char,
    signers_count: usize,
    fetch_pwd_cb: GfrPasswordFetchCb,
    free_cb: GfrFreeCb,
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
            for &key_ptr in pkeys_slice {
                if key_ptr.is_null() {
                    return Err(GfrStatus::ErrorInvalidInput);
                }
                pkey_blocks.push(std::ffi::CStr::from_ptr(key_ptr).to_str().unwrap_or(""));
            }
        }

        // 5. Safely extract secret keys (Signers)
        let mut skey_blocks = Vec::with_capacity(signers_count);
        unsafe {
            let sk_slice = std::slice::from_raw_parts(secret_keys, signers_count);
            for &sk_ptr in sk_slice {
                if sk_ptr.is_null() {
                    return Err(GfrStatus::ErrorInvalidInput);
                }
                skey_blocks.push(std::ffi::CStr::from_ptr(sk_ptr).to_str().unwrap_or(""));
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
        let stream_result = crate::crypto_stream::encrypt_and_sign_stream_internal(
            channel,
            &filename_hint,
            in_file,
            out_file,
            &pkey_blocks,
            &skey_blocks,
            Some(fetch_pwd_cb),
            Some(free_cb),
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
    pub_keys: *const *const std::os::raw::c_char,
    pub_keys_count: usize,
    secret_keys: *const *const std::os::raw::c_char,
    signers_count: usize,
    fetch_pwd_cb: crate::types::GfrPasswordFetchCb,
    free_cb: crate::types::GfrFreeCb,
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
            for &key_ptr in pkeys_slice {
                if key_ptr.is_null() {
                    return Err(GfrStatus::ErrorInvalidInput);
                }
                pkey_blocks.push(std::ffi::CStr::from_ptr(key_ptr).to_str().unwrap_or(""));
            }
        }

        let mut skey_blocks = Vec::with_capacity(signers_count);
        unsafe {
            let sk_slice = std::slice::from_raw_parts(secret_keys, signers_count);
            for &sk_ptr in sk_slice {
                if sk_ptr.is_null() {
                    return Err(GfrStatus::ErrorInvalidInput);
                }
                skey_blocks.push(std::ffi::CStr::from_ptr(sk_ptr).to_str().unwrap_or(""));
            }
        }

        let internal_result = encrypt_and_sign_directory_internal(
            channel,
            in_path_str,
            out_path_str,
            &pkey_blocks,
            &skey_blocks,
            Some(fetch_pwd_cb),
            Some(free_cb),
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
    free_cb: crate::types::GfrFreeCb,
    user_data: *mut std::ffi::c_void,
    out_result: *mut GfrDecryptAndVerifyResultC,
) -> GfrStatus {
    let result = std::panic::catch_unwind(|| -> Result<(), GfrStatus> {
        if in_data.is_null() || out_result.is_null() {
            return Err(GfrStatus::ErrorInvalidInput);
        }

        let data_slice = unsafe { std::slice::from_raw_parts(in_data, in_len) };

        let mut internal_result = crate::crypto::decrypt_and_verify_internal(
            channel,
            data_slice,
            Some(fetch_sec_key_cb),
            Some(fetch_pwd_cb),
            Some(fetch_pubkey_cb),
            Some(free_cb),
            user_data,
        )?;

        internal_result.data.shrink_to_fit();
        let data_ptr = internal_result.data.as_mut_ptr();
        let data_len = internal_result.data.len();
        std::mem::forget(internal_result.data);

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
    free_cb: crate::types::GfrFreeCb,
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
        let stream_result = crate::crypto_stream::decrypt_and_verify_stream_internal(
            channel,
            in_file,
            out_file,
            ascii,
            Some(fetch_sec_key_cb),
            Some(fetch_pwd_cb),
            Some(fetch_pubkey_cb),
            Some(free_cb),
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
    free_cb: crate::types::GfrFreeCb,
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
            Some(free_cb),
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
    free_cb: GfrFreeCb,
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

        crypto_stream::encrypt_stream_with_password_internal(
            channel,
            name_str,
            data_slice,
            &mut out_buf,
            Some(fetch_pwd_cb),
            Some(free_cb),
            ascii,
        )?;

        out_buf.shrink_to_fit();
        let data_ptr = out_buf.as_mut_ptr();
        let data_len = out_buf.len();
        std::mem::forget(out_buf);

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
    free_cb: GfrFreeCb,
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
        crypto_stream::encrypt_stream_with_password_internal(
            channel,
            &filename_hint,
            in_file,
            out_file,
            Some(fetch_pwd_cb),
            Some(free_cb),
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
    free_cb: GfrFreeCb,
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

        crypto_stream::encrypt_directory_with_password_internal(
            channel,
            in_path_str,
            out_path_str,
            Some(fetch_pwd_cb),
            Some(free_cb),
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
