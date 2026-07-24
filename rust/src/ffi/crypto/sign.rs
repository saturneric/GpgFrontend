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

use crate::crypto::get_signature_issuers_internal;
use crate::err::clear_last_error;
use crate::types::{
    GfrBuffer, GfrPasswordFetchCb, GfrPublicKeyFetchCb, GfrSignMode, GfrSignResultC,
    GfrSignatureResultC, GfrStatus, GfrVerifyResultC,
};
use std::fs::File;
use std::path::Path;
use std::slice;
use std::{
    ffi::{CStr, CString, c_char},
    panic::catch_unwind,
};

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
    secret_keys: *const GfrBuffer,
    signers_count: usize,
    fetch_pwd_cb: GfrPasswordFetchCb,
    mode: GfrSignMode,
    ascii: bool,
    out_result: *mut GfrSignResultC,
) -> GfrStatus {
    clear_last_error();
    let result = catch_unwind(|| -> Result<(), GfrStatus> {
        if name.is_null() || in_data.is_null() || secret_keys.is_null() || out_result.is_null() {
            return Err(GfrStatus::ErrorInvalidInput);
        }

        let name_str = unsafe { CStr::from_ptr(name) }.to_str().unwrap_or("");
        let data_slice = unsafe { slice::from_raw_parts(in_data, in_len) };

        let mut skey_blocks = Vec::with_capacity(signers_count);

        unsafe {
            let sk_slice = slice::from_raw_parts(secret_keys, signers_count);
            for gfr_buf in sk_slice {
                skey_blocks.push(gfr_buf.as_str()?);
            }
        }

        // Perform the multi-signature and get the structured report
        let internal_result = crate::crypto::sign_internal(
            channel,
            name_str,
            data_slice,
            &skey_blocks,
            Some(fetch_pwd_cb),
            mode,
            ascii,
        )?;

        // 1. Process the output payload (data)
        // Boxed slice guarantees `capacity == len` for the allocation handed to
        // C; `gfr_crypto_free_buffer` reconstructs the Vec with `capacity == len`.
        let mut data_boxed = internal_result.data.into_boxed_slice();
        let data_ptr = data_boxed.as_mut_ptr();
        let data_len = data_boxed.len();
        std::mem::forget(data_boxed); // Leak payload to C

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
    secret_keys: *const GfrBuffer,
    signers_count: usize,
    fetch_pwd_cb: GfrPasswordFetchCb,
    mode: GfrSignMode,
    ascii: bool,
    out_result: *mut GfrSignResultC,
) -> GfrStatus {
    clear_last_error();
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
            for gfr_buf in sk_slice {
                skey_blocks.push(gfr_buf.as_str()?);
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
        let stream_result = crate::crypto::sign_stream_internal(
            channel,
            &filename_hint,
            in_file,
            out_file,
            &skey_blocks,
            Some(fetch_pwd_cb),
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
    user_data: *mut std::ffi::c_void,
    mode: GfrSignMode,
    out_result: *mut GfrVerifyResultC,
) -> GfrStatus {
    clear_last_error();
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
        let internal_result = crate::crypto::verify_internal(
            data_slice,
            sig_slice,
            mode,
            Some(fetch_pubkey_cb),
            user_data,
        )?;

        // 1. Process the extracted payload (data)
        // Boxed slice guarantees `capacity == len` for the allocation handed to
        // C; `gfr_crypto_free_buffer` reconstructs the Vec with `capacity == len`.
        let mut data_boxed = internal_result.data.into_boxed_slice();
        let data_ptr = data_boxed.as_mut_ptr();
        let data_len = data_boxed.len();
        std::mem::forget(data_boxed); // Leak payload to C

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
    channel: i32,
    in_file_path: *const c_char,
    sig_file_path: *const c_char, // Used for Detached mode (.sig file)
    out_file_path: *const c_char, // Optional: Extracted plaintext output for Inline mode
    fetch_pubkey_cb: GfrPublicKeyFetchCb,
    user_data: *mut std::ffi::c_void,
    mode: GfrSignMode,
    out_result: *mut GfrVerifyResultC,
) -> GfrStatus {
    clear_last_error();
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

                let stream_result = crate::crypto::verify_detached_stream_internal(
                    channel,
                    in_file,
                    &sig_data,
                    Some(fetch_pubkey_cb),
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

                let internal_result = crate::crypto::verify_internal(
                    &in_data,
                    &[], // sig_data is not needed for inline
                    mode,
                    Some(fetch_pubkey_cb),
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
                    // Boxed slice guarantees `capacity == len` for the C handoff.
                    let mut data_boxed = internal_result.data.into_boxed_slice();
                    out_data_ptr = data_boxed.as_mut_ptr();
                    out_data_len = data_boxed.len();
                    std::mem::forget(data_boxed);
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
    clear_last_error();
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

#[cfg(test)]
mod ffi_sign_tests {
    //! The five sign/verify/issuer FFI entry points.

    use super::*;
    use crate::testutil::{cb, corpus, keys};
    use std::ffi::CString;
    use std::mem::MaybeUninit;

    fn kbuf(s: &str) -> GfrBuffer {
        GfrBuffer {
            data: s.as_ptr(),
            len: s.len(),
        }
    }

    // -- sign_data ------------------------------------------------------------

    #[test]
    fn sign_data_rejects_null_arguments() {
        let name = CString::new("").expect("no NUL");
        let key = &keys::V4_SIGN;
        let keys_arr = [kbuf(&key.secret_armored)];
        let payload = b"x";
        let mut out = MaybeUninit::<GfrSignResultC>::zeroed();

        assert_eq!(
            gfr_crypto_sign_data(
                0,
                std::ptr::null(),
                payload.as_ptr(),
                payload.len(),
                keys_arr.as_ptr(),
                1,
                cb::pwd_correct,
                GfrSignMode::Detached,
                true,
                out.as_mut_ptr(),
            ),
            GfrStatus::ErrorInvalidInput
        );
        assert_eq!(
            gfr_crypto_sign_data(
                0,
                name.as_ptr(),
                std::ptr::null(),
                0,
                keys_arr.as_ptr(),
                1,
                cb::pwd_correct,
                GfrSignMode::Detached,
                true,
                out.as_mut_ptr(),
            ),
            GfrStatus::ErrorInvalidInput
        );
        assert_eq!(
            gfr_crypto_sign_data(
                0,
                name.as_ptr(),
                payload.as_ptr(),
                payload.len(),
                std::ptr::null(),
                0,
                cb::pwd_correct,
                GfrSignMode::Detached,
                true,
                out.as_mut_ptr(),
            ),
            GfrStatus::ErrorInvalidInput
        );
        assert_eq!(
            gfr_crypto_sign_data(
                0,
                name.as_ptr(),
                payload.as_ptr(),
                payload.len(),
                keys_arr.as_ptr(),
                1,
                cb::pwd_correct,
                GfrSignMode::Detached,
                true,
                std::ptr::null_mut(),
            ),
            GfrStatus::ErrorInvalidInput
        );
    }

    #[test]
    fn sign_data_round_trips_through_its_free() {
        let name = CString::new("").expect("no NUL");
        let key = &keys::V4_SIGN;
        let keys_arr = [kbuf(&key.secret_armored)];
        let payload = b"ffi signing payload";
        let mut out = MaybeUninit::<GfrSignResultC>::zeroed();

        assert_eq!(
            gfr_crypto_sign_data(
                0,
                name.as_ptr(),
                payload.as_ptr(),
                payload.len(),
                keys_arr.as_ptr(),
                keys_arr.len(),
                cb::pwd_correct,
                GfrSignMode::Detached,
                true,
                out.as_mut_ptr(),
            ),
            GfrStatus::Success
        );
        let mut result = unsafe { out.assume_init() };
        assert!(!result.data.is_null());
        assert!(result.data_len > 0);
        assert_eq!(result.meta.signature_count, 1);
        crate::ffi::mem::gfr_crypto_free_sign_result(&mut result);
    }

    #[test]
    fn sign_data_reports_the_issuer_and_a_strong_hash() {
        let name = CString::new("").expect("no NUL");
        let key = &keys::V4_SIGN;
        let keys_arr = [kbuf(&key.secret_armored)];
        let payload = b"x";
        let mut out = MaybeUninit::<GfrSignResultC>::zeroed();

        assert_eq!(
            gfr_crypto_sign_data(
                0,
                name.as_ptr(),
                payload.as_ptr(),
                payload.len(),
                keys_arr.as_ptr(),
                1,
                cb::pwd_correct,
                GfrSignMode::Detached,
                false,
                out.as_mut_ptr(),
            ),
            GfrStatus::Success
        );
        let mut result = unsafe { out.assume_init() };
        let sig = unsafe { &*result.meta.signatures };
        let hash = unsafe { CStr::from_ptr(sig.hash_algo) }
            .to_string_lossy()
            .into_owned();
        assert!(!crate::crypto::sig_hash_algo_is_weak(&hash), "{hash}");
        assert!(!sig.issuer_fpr.is_null());
        crate::ffi::mem::gfr_crypto_free_sign_result(&mut result);
    }

    #[test]
    fn sign_data_with_a_public_key_block_fails() {
        let name = CString::new("").expect("no NUL");
        let keys_arr = [kbuf(&keys::V4_SIGN.public_armored)];
        let payload = b"x";
        let mut out = MaybeUninit::<GfrSignResultC>::zeroed();
        assert_ne!(
            gfr_crypto_sign_data(
                0,
                name.as_ptr(),
                payload.as_ptr(),
                payload.len(),
                keys_arr.as_ptr(),
                1,
                cb::pwd_correct,
                GfrSignMode::Detached,
                true,
                out.as_mut_ptr(),
            ),
            GfrStatus::Success
        );
    }

    // -- verify_data -----------------------------------------------------------

    #[test]
    fn verify_data_rejects_null_arguments() {
        let mut out = MaybeUninit::<GfrVerifyResultC>::zeroed();
        assert_eq!(
            gfr_crypto_verify_data(
                std::ptr::null(),
                0,
                corpus::SIG_GOOD_DETACHED.as_ptr(),
                corpus::SIG_GOOD_DETACHED.len(),
                cb::pubkey_none,
                std::ptr::null_mut(),
                GfrSignMode::Detached,
                out.as_mut_ptr(),
            ),
            GfrStatus::ErrorInvalidInput
        );
        assert_eq!(
            gfr_crypto_verify_data(
                corpus::PAYLOAD.as_ptr(),
                corpus::PAYLOAD.len(),
                corpus::SIG_GOOD_DETACHED.as_ptr(),
                corpus::SIG_GOOD_DETACHED.len(),
                cb::pubkey_none,
                std::ptr::null_mut(),
                GfrSignMode::Detached,
                std::ptr::null_mut(),
            ),
            GfrStatus::ErrorInvalidInput
        );
    }

    #[test]
    fn verify_data_reports_a_valid_signature() {
        cb::set_pubkey_answer(corpus::AUX_GOOD);
        let mut out = MaybeUninit::<GfrVerifyResultC>::zeroed();
        assert_eq!(
            gfr_crypto_verify_data(
                corpus::PAYLOAD.as_ptr(),
                corpus::PAYLOAD.len(),
                corpus::SIG_GOOD_DETACHED.as_ptr(),
                corpus::SIG_GOOD_DETACHED.len(),
                cb::pubkey_fetch,
                std::ptr::null_mut(),
                GfrSignMode::Detached,
                out.as_mut_ptr(),
            ),
            GfrStatus::Success
        );
        let mut result = unsafe { out.assume_init() };
        assert_eq!(result.meta.signature_count, 1);
        let sig = unsafe { &*result.meta.signatures };
        assert_eq!(sig.status, crate::types::GfrSignatureStatus::Valid);
        crate::ffi::mem::gfr_crypto_free_verify_result(&mut result);
    }

    #[test]
    fn verify_data_reports_an_unknown_issuer_as_no_key() {
        // Never `BadSignature`: the user has no key, not a forgery.
        cb::clear_pubkey_answer();
        let mut out = MaybeUninit::<GfrVerifyResultC>::zeroed();
        assert_eq!(
            gfr_crypto_verify_data(
                corpus::PAYLOAD.as_ptr(),
                corpus::PAYLOAD.len(),
                corpus::SIG_GOOD_DETACHED.as_ptr(),
                corpus::SIG_GOOD_DETACHED.len(),
                cb::pubkey_none,
                std::ptr::null_mut(),
                GfrSignMode::Detached,
                out.as_mut_ptr(),
            ),
            GfrStatus::Success
        );
        let mut result = unsafe { out.assume_init() };
        let sig = unsafe { &*result.meta.signatures };
        assert_eq!(sig.status, crate::types::GfrSignatureStatus::NoKey);
        crate::ffi::mem::gfr_crypto_free_verify_result(&mut result);
    }

    #[test]
    fn verify_data_never_panics_on_adversarial_input() {
        for vector in [corpus::GARBAGE, corpus::EMPTY, corpus::ENC_SED_TAG9] {
            let outcome = std::panic::catch_unwind(|| {
                let mut out = MaybeUninit::<GfrVerifyResultC>::zeroed();
                let status = gfr_crypto_verify_data(
                    corpus::PAYLOAD.as_ptr(),
                    corpus::PAYLOAD.len(),
                    vector.as_ptr(),
                    vector.len(),
                    cb::pubkey_none,
                    std::ptr::null_mut(),
                    GfrSignMode::Detached,
                    out.as_mut_ptr(),
                );
                if status == GfrStatus::Success {
                    let mut r = unsafe { out.assume_init() };
                    crate::ffi::mem::gfr_crypto_free_verify_result(&mut r);
                }
            });
            assert!(outcome.is_ok());
        }
    }

    #[test]
    fn a_signature_we_produce_verifies_back_through_the_ffi() {
        // The full boundary round trip: sign through the FFI, verify through
        // the FFI, free both results.
        let name = CString::new("").expect("no NUL");
        let key = &keys::V4_SIGN;
        let keys_arr = [kbuf(&key.secret_armored)];
        let payload = b"boundary round trip";
        let mut sign_out = MaybeUninit::<GfrSignResultC>::zeroed();

        assert_eq!(
            gfr_crypto_sign_data(
                0,
                name.as_ptr(),
                payload.as_ptr(),
                payload.len(),
                keys_arr.as_ptr(),
                1,
                cb::pwd_correct,
                GfrSignMode::Detached,
                false,
                sign_out.as_mut_ptr(),
            ),
            GfrStatus::Success
        );
        let mut signed = unsafe { sign_out.assume_init() };

        cb::set_pubkey_answer(&key.public_armored);
        let mut verify_out = MaybeUninit::<GfrVerifyResultC>::zeroed();
        assert_eq!(
            gfr_crypto_verify_data(
                payload.as_ptr(),
                payload.len(),
                signed.data,
                signed.data_len,
                cb::pubkey_fetch,
                std::ptr::null_mut(),
                GfrSignMode::Detached,
                verify_out.as_mut_ptr(),
            ),
            GfrStatus::Success
        );
        let mut verified = unsafe { verify_out.assume_init() };
        let sig = unsafe { &*verified.meta.signatures };
        assert_eq!(sig.status, crate::types::GfrSignatureStatus::Valid);

        crate::ffi::mem::gfr_crypto_free_verify_result(&mut verified);
        crate::ffi::mem::gfr_crypto_free_sign_result(&mut signed);
    }

    // -- get_signature_issuers ----------------------------------------------------

    #[test]
    fn get_signature_issuers_rejects_null_arguments() {
        let mut out = std::ptr::null_mut();
        assert_eq!(
            gfr_crypto_get_signature_issuers(std::ptr::null(), 0, &mut out),
            GfrStatus::ErrorInvalidInput
        );
        assert_eq!(
            gfr_crypto_get_signature_issuers(
                corpus::SIG_GOOD_DETACHED.as_ptr(),
                corpus::SIG_GOOD_DETACHED.len(),
                std::ptr::null_mut()
            ),
            GfrStatus::ErrorInvalidInput
        );
    }

    #[test]
    fn get_signature_issuers_returns_a_csv_and_frees_cleanly() {
        let mut out = std::ptr::null_mut();
        assert_eq!(
            gfr_crypto_get_signature_issuers(
                corpus::SIG_GOOD_DETACHED.as_ptr(),
                corpus::SIG_GOOD_DETACHED.len(),
                &mut out
            ),
            GfrStatus::Success
        );
        assert!(!out.is_null());
        let issuers = unsafe { CStr::from_ptr(out) }.to_string_lossy().into_owned();
        assert!(!issuers.is_empty());
        crate::ffi::mem::gfr_crypto_free_string(out);
    }

    #[test]
    fn get_signature_issuers_of_garbage_does_not_crash() {
        let mut out = std::ptr::null_mut();
        let status = gfr_crypto_get_signature_issuers(
            corpus::GARBAGE.as_ptr(),
            corpus::GARBAGE.len(),
            &mut out,
        );
        if status == GfrStatus::Success && !out.is_null() {
            crate::ffi::mem::gfr_crypto_free_string(out);
        }
    }

    // -- file variants ----------------------------------------------------------------

    #[test]
    fn sign_file_rejects_null_paths() {
        let keys_arr = [kbuf(&keys::V4_SIGN.secret_armored)];
        let mut out = MaybeUninit::<GfrSignResultC>::zeroed();
        assert_eq!(
            gfr_crypto_sign_file(
                0,
                std::ptr::null(),
                std::ptr::null(),
                keys_arr.as_ptr(),
                1,
                cb::pwd_correct,
                GfrSignMode::Detached,
                true,
                out.as_mut_ptr(),
            ),
            GfrStatus::ErrorInvalidInput
        );
    }

    #[test]
    fn sign_file_writes_a_signature_next_to_the_input() {
        let dir = tempfile::tempdir().expect("tempdir");
        let in_path = dir.path().join("payload.bin");
        let out_path = dir.path().join("payload.sig");
        std::fs::write(&in_path, b"file payload").expect("write");

        let in_c = CString::new(in_path.to_string_lossy().as_ref()).expect("no NUL");
        let out_c = CString::new(out_path.to_string_lossy().as_ref()).expect("no NUL");
        let keys_arr = [kbuf(&keys::V4_SIGN.secret_armored)];
        let mut out = MaybeUninit::<GfrSignResultC>::zeroed();

        assert_eq!(
            gfr_crypto_sign_file(
                0,
                in_c.as_ptr(),
                out_c.as_ptr(),
                keys_arr.as_ptr(),
                1,
                cb::pwd_correct,
                GfrSignMode::Detached,
                true,
                out.as_mut_ptr(),
            ),
            GfrStatus::Success
        );
        assert!(out_path.exists());
        let mut result = unsafe { out.assume_init() };
        crate::ffi::mem::gfr_crypto_free_sign_result(&mut result);
    }

    #[test]
    fn sign_file_with_a_missing_input_fails() {
        let in_c = CString::new("/nonexistent/definitely/not/here").expect("no NUL");
        let out_c = CString::new("/tmp/should-not-be-written.sig").expect("no NUL");
        let keys_arr = [kbuf(&keys::V4_SIGN.secret_armored)];
        let mut out = MaybeUninit::<GfrSignResultC>::zeroed();
        assert_ne!(
            gfr_crypto_sign_file(
                0,
                in_c.as_ptr(),
                out_c.as_ptr(),
                keys_arr.as_ptr(),
                1,
                cb::pwd_correct,
                GfrSignMode::Detached,
                true,
                out.as_mut_ptr(),
            ),
            GfrStatus::Success
        );
    }

    #[test]
    fn verify_file_rejects_null_paths() {
        let mut out = MaybeUninit::<GfrVerifyResultC>::zeroed();
        assert_eq!(
            gfr_crypto_verify_file(
                0,
                std::ptr::null(),
                std::ptr::null(),
                std::ptr::null(),
                cb::pubkey_none,
                std::ptr::null_mut(),
                GfrSignMode::Detached,
                out.as_mut_ptr(),
            ),
            GfrStatus::ErrorInvalidInput
        );
    }

    #[test]
    fn a_file_signature_verifies_back_through_the_ffi() {
        let dir = tempfile::tempdir().expect("tempdir");
        let in_path = dir.path().join("payload.bin");
        let sig_path = dir.path().join("payload.sig");
        std::fs::write(&in_path, b"file round trip").expect("write");

        let in_c = CString::new(in_path.to_string_lossy().as_ref()).expect("no NUL");
        let sig_c = CString::new(sig_path.to_string_lossy().as_ref()).expect("no NUL");
        let keys_arr = [kbuf(&keys::V4_SIGN.secret_armored)];
        let mut sign_out = MaybeUninit::<GfrSignResultC>::zeroed();

        assert_eq!(
            gfr_crypto_sign_file(
                0,
                in_c.as_ptr(),
                sig_c.as_ptr(),
                keys_arr.as_ptr(),
                1,
                cb::pwd_correct,
                GfrSignMode::Detached,
                true,
                sign_out.as_mut_ptr(),
            ),
            GfrStatus::Success
        );
        let mut signed = unsafe { sign_out.assume_init() };
        crate::ffi::mem::gfr_crypto_free_sign_result(&mut signed);

        cb::set_pubkey_answer(&keys::V4_SIGN.public_armored);
        let mut verify_out = MaybeUninit::<GfrVerifyResultC>::zeroed();
        assert_eq!(
            gfr_crypto_verify_file(
                0,
                in_c.as_ptr(),
                sig_c.as_ptr(),
                std::ptr::null(),
                cb::pubkey_fetch,
                std::ptr::null_mut(),
                GfrSignMode::Detached,
                verify_out.as_mut_ptr(),
            ),
            GfrStatus::Success
        );
        let mut verified = unsafe { verify_out.assume_init() };
        let sig = unsafe { &*verified.meta.signatures };
        assert_eq!(sig.status, crate::types::GfrSignatureStatus::Valid);
        crate::ffi::mem::gfr_crypto_free_verify_result(&mut verified);
    }
}
