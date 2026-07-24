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

//! Memory deallocation helpers for all heap-allocated FFI output values.
//!
//! Each function here is the counterpart to one or more FFI functions that
//! transfer heap ownership to the C++ caller. The caller **must** use the
//! matching `gfr_crypto_free_*` function — passing pointers to `free()` or
//! any other allocator is undefined behaviour.

use zeroize::Zeroize;

use crate::types::{
    GfrDecryptAndVerifyResultC, GfrDecryptMetadataC, GfrDecryptResultC, GfrEncryptAndSignResultC,
    GfrEncryptMetadataC, GfrEncryptResultC, GfrKeyGenerateResult, GfrKeyMetadataC,
    GfrRecipientResultC, GfrSignMetadataC, GfrSignResultC, GfrVerifyMetadataC, GfrVerifyResultC,
};
use std::ffi::{CString, c_char};

unsafe fn secure_free_c_string(ptr: *mut c_char) {
    if ptr.is_null() {
        return;
    }

    let c_string = unsafe { CString::from_raw(ptr) };
    let mut bytes = c_string.into_bytes_with_nul();

    // Clear the contents before Vec frees the allocation.
    bytes.zeroize();
}

/// Free a heap-allocated C string returned by any `gfr_*` function.
///
/// # Safety
/// `ptr` must have been allocated by the Rust engine. Passing null is a no-op.
#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_free_string(ptr: *mut c_char) {
    if !ptr.is_null() {
        unsafe { secure_free_c_string(ptr) }
    }
}

/// Free a raw byte buffer returned in a `data`/`data_len` pair by any `gfr_*`
/// function.
///
/// # Safety
/// `ptr` and `len` must exactly match a previously returned `data`/`data_len`
/// pair. Passing null or len 0 is a no-op.
#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_free_buffer(ptr: *mut u8, len: usize) {
    if !ptr.is_null() && len > 0 {
        unsafe {
            let mut bytes = Vec::from_raw_parts(ptr, len, len);
            bytes.zeroize();
        }
    }
}

/// Free the `GfrRecipientResultC` array returned by
/// `gfr_crypto_get_recipients`.
///
/// Reclaims the boxed array and each element's `key_id` / `pub_algo` C strings.
/// This is the dedicated free routine for the bare recipient array shape (the
/// array is not owned by any result/metadata struct, so the `free_*_result`
/// helpers do not cover it).
///
/// # Safety
/// `ptr`/`count` must exactly match a `*out_recipients`/`*out_count` pair
/// returned by `gfr_crypto_get_recipients`. Passing null or count 0 is a no-op.
#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_free_recipients(ptr: *mut GfrRecipientResultC, count: usize) {
    if ptr.is_null() || count == 0 {
        return;
    }
    unsafe {
        let recs_slice = std::slice::from_raw_parts_mut(ptr, count);
        for rec in recs_slice.iter_mut() {
            if !rec.key_id.is_null() {
                secure_free_c_string(rec.key_id);
                rec.key_id = std::ptr::null_mut();
            }
            if !rec.pub_algo.is_null() {
                secure_free_c_string(rec.pub_algo);
                rec.pub_algo = std::ptr::null_mut();
            }
        }
        // The array was handed out via `into_boxed_slice` (cap == len), so
        // reclaim it as a boxed slice of exactly `count` elements.
        let array_ptr = std::ptr::slice_from_raw_parts_mut(ptr, count);
        drop(Box::from_raw(array_ptr));
    }
}

/// Free all heap-allocated string fields within a `GfrKeyGenerateResult`.
///
/// # Safety
/// `result` must point to a `GfrKeyGenerateResult` populated by
/// `gfr_crypto_generate_key` or `gfr_crypto_add_subkey`. Passing null is a
/// no-op.
#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_free_key_generate_result(result: *mut GfrKeyGenerateResult) {
    if result.is_null() {
        return;
    }
    unsafe {
        if !(*result).secret_key.is_null() {
            secure_free_c_string((*result).secret_key);
        }
        if !(*result).public_key.is_null() {
            secure_free_c_string((*result).public_key);
        }
        if !(*result).fingerprint.is_null() {
            secure_free_c_string((*result).fingerprint);
        }
        (*result).secret_key = std::ptr::null_mut();
        (*result).public_key = std::ptr::null_mut();
        (*result).fingerprint = std::ptr::null_mut();
    }
}

/// Helper to free sign metadata
#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_free_sign_metadata(meta: *mut GfrSignMetadataC) {
    if meta.is_null() {
        return;
    }
    unsafe {
        if !(*meta).signatures.is_null() && (*meta).signature_count > 0 {
            let sigs_slice =
                std::slice::from_raw_parts_mut((*meta).signatures, (*meta).signature_count);
            for sig in sigs_slice.iter_mut() {
                if !sig.issuer_fpr.is_null() {
                    secure_free_c_string(sig.issuer_fpr);
                }
                if !sig.pub_algo.is_null() {
                    secure_free_c_string(sig.pub_algo);
                }
                if !sig.hash_algo.is_null() {
                    secure_free_c_string(sig.hash_algo);
                }
            }
            let array_ptr =
                std::ptr::slice_from_raw_parts_mut((*meta).signatures, (*meta).signature_count);
            drop(Box::from_raw(array_ptr));
        }
        (*meta).signatures = std::ptr::null_mut();
        (*meta).signature_count = 0;
    }
}

/// Helper to free encrypt metadata
#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_free_encrypt_metadata(meta: *mut GfrEncryptMetadataC) {
    if meta.is_null() {
        return;
    }
    unsafe {
        if !(*meta).invalid_recipients.is_null() && (*meta).invalid_recipient_count > 0 {
            let recs_slice = std::slice::from_raw_parts_mut(
                (*meta).invalid_recipients,
                (*meta).invalid_recipient_count,
            );
            for rec in recs_slice.iter_mut() {
                if !rec.fpr.is_null() {
                    secure_free_c_string(rec.fpr);
                }
            }
            let array_ptr = std::ptr::slice_from_raw_parts_mut(
                (*meta).invalid_recipients,
                (*meta).invalid_recipient_count,
            );
            drop(Box::from_raw(array_ptr));
        }
        (*meta).invalid_recipients = std::ptr::null_mut();
        (*meta).invalid_recipient_count = 0;

        if !(*meta).recipients.is_null() && (*meta).recipient_count > 0 {
            let recs_slice =
                std::slice::from_raw_parts_mut((*meta).recipients, (*meta).recipient_count);
            for rec in recs_slice.iter_mut() {
                if !rec.key_id.is_null() {
                    secure_free_c_string(rec.key_id);
                }
                if !rec.pub_algo.is_null() {
                    secure_free_c_string(rec.pub_algo);
                }
            }
            let array_ptr =
                std::ptr::slice_from_raw_parts_mut((*meta).recipients, (*meta).recipient_count);
            drop(Box::from_raw(array_ptr));
        }
        (*meta).recipients = std::ptr::null_mut();
        (*meta).recipient_count = 0;
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_free_decrypt_metadata(meta: *mut GfrDecryptMetadataC) {
    if meta.is_null() {
        return;
    }

    unsafe {
        if !(*meta).filename.is_null() {
            secure_free_c_string((*meta).filename);
        }

        if !(*meta).recipients.is_null() && (*meta).recipient_count > 0 {
            let recs_slice =
                std::slice::from_raw_parts_mut((*meta).recipients, (*meta).recipient_count);
            for rec in recs_slice.iter_mut() {
                if !rec.key_id.is_null() {
                    secure_free_c_string(rec.key_id);
                }
                if !rec.pub_algo.is_null() {
                    secure_free_c_string(rec.pub_algo);
                }
            }
            let array_ptr =
                std::ptr::slice_from_raw_parts_mut((*meta).recipients, (*meta).recipient_count);
            drop(Box::from_raw(array_ptr));
        }
        (*meta).filename = std::ptr::null_mut();
        (*meta).recipients = std::ptr::null_mut();
        (*meta).recipient_count = 0;
    }
}

/// Free the verification result memory
#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_free_verify_metadata(meta: *mut GfrVerifyMetadataC) {
    if meta.is_null() {
        return;
    }

    unsafe {
        // 2. Free the signatures array and its internal strings
        if !(*meta).signatures.is_null() && (*meta).signature_count > 0 {
            let sigs_slice =
                std::slice::from_raw_parts_mut((*meta).signatures, (*meta).signature_count);

            for sig in sigs_slice.iter_mut() {
                if !sig.issuer_fpr.is_null() {
                    secure_free_c_string(sig.issuer_fpr);
                }
                if !sig.pub_algo.is_null() {
                    secure_free_c_string(sig.pub_algo);
                }
                if !sig.hash_algo.is_null() {
                    secure_free_c_string(sig.hash_algo);
                }
            }

            // Free the array itself
            let array_ptr =
                std::ptr::slice_from_raw_parts_mut((*meta).signatures, (*meta).signature_count);
            drop(Box::from_raw(array_ptr));
        }

        (*meta).signatures = std::ptr::null_mut();
        (*meta).signature_count = 0;
        (*meta).is_verified = false;
    }
}

/// Free the encryption result memory
#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_free_encrypt_result(result: *mut GfrEncryptResultC) {
    if result.is_null() {
        return;
    }
    unsafe {
        gfr_crypto_free_buffer((*result).data, (*result).data_len);
        (*result).data = std::ptr::null_mut();
        (*result).data_len = 0;
        gfr_crypto_free_encrypt_metadata(&mut (*result).meta);
    }
}

/// Free the signature result memory
#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_free_sign_result(result: *mut GfrSignResultC) {
    if result.is_null() {
        return;
    }
    unsafe {
        gfr_crypto_free_buffer((*result).data, (*result).data_len);
        (*result).data = std::ptr::null_mut();
        (*result).data_len = 0;
        gfr_crypto_free_sign_metadata(&mut (*result).meta);
    }
}

/// Free the decryption result memory
#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_free_decrypt_result(result: *mut GfrDecryptResultC) {
    if result.is_null() {
        return;
    }
    unsafe {
        gfr_crypto_free_buffer((*result).data, (*result).data_len);
        (*result).data = std::ptr::null_mut();
        (*result).data_len = 0;
        gfr_crypto_free_decrypt_metadata(&mut (*result).meta);
    }
}

/// Free the verification result memory
#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_free_verify_result(result: *mut GfrVerifyResultC) {
    if result.is_null() {
        return;
    }
    unsafe {
        gfr_crypto_free_buffer((*result).data, (*result).data_len);
        (*result).data = std::ptr::null_mut();
        (*result).data_len = 0;
        gfr_crypto_free_verify_metadata(&mut (*result).meta);
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_free_encrypt_and_sign_result(result: *mut GfrEncryptAndSignResultC) {
    if result.is_null() {
        return;
    }
    unsafe {
        gfr_crypto_free_buffer((*result).data, (*result).data_len);
        (*result).data = std::ptr::null_mut();
        (*result).data_len = 0;
        gfr_crypto_free_sign_metadata(&mut (*result).sign_meta);
        gfr_crypto_free_encrypt_metadata(&mut (*result).encrypt_meta);
    }
}

/// Free all heap-allocated fields within a `GfrDecryptAndVerifyResultC`.
///
/// # Safety
/// `result` must point to a struct populated by
/// `gfr_crypto_decrypt_and_verify_*`. Passing null is a no-op.
#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_free_decrypt_and_verify_result(
    result: *mut GfrDecryptAndVerifyResultC,
) {
    if result.is_null() {
        return;
    }
    unsafe {
        gfr_crypto_free_buffer((*result).data, (*result).data_len);
        (*result).data = std::ptr::null_mut();
        (*result).data_len = 0;
        gfr_crypto_free_decrypt_metadata(&mut (*result).decrypt_meta);
        gfr_crypto_free_verify_metadata(&mut (*result).verify_meta);
    }
}

/// Free a `GfrKeyMetadataC` array returned by `gfr_crypto_extract_metadata`.
///
/// Recursively frees all heap-allocated string fields in each entry
/// (fingerprints, key IDs, armored blocks, user IDs, and subkeys) before
/// freeing the outer array.
///
/// # Safety
/// `metadata_ptr` must have been set by `gfr_crypto_extract_metadata` and
/// `count` must match the returned `*out_metadata_count`. Passing null or 0
/// count is a no-op.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn gfr_free_metadata_array(metadata_ptr: *mut GfrKeyMetadataC, count: usize) {
    if metadata_ptr.is_null() || count == 0 {
        return;
    }

    // Rebuild the outer slice
    let metadata_slice = unsafe { std::slice::from_raw_parts_mut(metadata_ptr, count) };

    for meta in metadata_slice.iter_mut() {
        // Free strings in main key
        if !meta.fpr.is_null() {
            gfr_crypto_free_string(meta.fpr);
        }
        if !meta.key_id.is_null() {
            gfr_crypto_free_string(meta.key_id);
        }

        if !meta.public_key_block.is_null() {
            gfr_crypto_free_string(meta.public_key_block);
        }
        if !meta.secret_key_block.is_null() {
            gfr_crypto_free_string(meta.secret_key_block);
        }

        if !meta.user_ids.is_null() && meta.user_id_count > 0 {
            let uids_slice =
                unsafe { std::slice::from_raw_parts_mut(meta.user_ids, meta.user_id_count) };
            for uid in uids_slice.iter_mut() {
                if !uid.user_id.is_null() {
                    gfr_crypto_free_string(uid.user_id);
                }
            }
            let _ = unsafe {
                Vec::from_raw_parts(meta.user_ids, meta.user_id_count, meta.user_id_count)
            };
        }

        // Free subkeys and their strings
        if !meta.subkeys.is_null() && meta.subkey_count > 0 {
            let subkeys_slice =
                unsafe { std::slice::from_raw_parts_mut(meta.subkeys, meta.subkey_count) };
            for sub in subkeys_slice.iter_mut() {
                if !sub.fpr.is_null() {
                    gfr_crypto_free_string(sub.fpr);
                }
                if !sub.key_id.is_null() {
                    gfr_crypto_free_string(sub.key_id);
                }
            }
            // Free the subkeys array itself
            let _ =
                unsafe { Vec::from_raw_parts(meta.subkeys, meta.subkey_count, meta.subkey_count) };
        }
    }

    // Free the outer array itself
    let _ = unsafe { Vec::from_raw_parts(metadata_ptr, count, count) };
}

#[cfg(test)]
mod mem_tests {
    //! The deallocation half of the FFI memory contract.
    //!
    //! Every heap pointer the engine hands to C++ has exactly one matching
    //! `gfr_crypto_free_*`. What is testable here is that each one tolerates
    //! null, reclaims a genuine allocation, and — for the structs that null
    //! their fields — is safe to call twice.
    //!
    //! Deliberately absent: double-free tests. Freeing the same pointer twice
    //! is undefined behaviour and cannot be tested, only avoided.

    use super::*;
    use crate::types::{GfrSignatureResultC, GfrStatus};
    use std::ffi::{CStr, CString};

    fn c_str(s: &str) -> *mut c_char {
        CString::new(s).expect("no interior NUL").into_raw()
    }

    // -- free_string ---------------------------------------------------------

    #[test]
    fn freeing_a_null_string_is_a_no_op() {
        gfr_crypto_free_string(std::ptr::null_mut());
    }

    #[test]
    fn freeing_a_string_reclaims_it() {
        gfr_crypto_free_string(c_str("a heap allocated value"));
    }

    #[test]
    fn freeing_an_empty_string_is_fine() {
        gfr_crypto_free_string(c_str(""));
    }

    #[test]
    fn freeing_a_multibyte_string_is_fine() {
        gfr_crypto_free_string(c_str("clé 鍵 🔑"));
    }

    #[test]
    fn a_version_string_round_trips_through_its_free() {
        // The realistic shape: the engine allocates, C++ reads, C++ frees.
        let ptr = crate::ffi::gfr_rust_engine_version();
        assert!(!ptr.is_null());
        let text = unsafe { CStr::from_ptr(ptr) }.to_string_lossy().into_owned();
        assert!(!text.is_empty());
        gfr_crypto_free_string(ptr);
    }

    // -- free_buffer ---------------------------------------------------------

    #[test]
    fn freeing_a_null_buffer_is_a_no_op() {
        gfr_crypto_free_buffer(std::ptr::null_mut(), 0);
        gfr_crypto_free_buffer(std::ptr::null_mut(), 128);
    }

    #[test]
    fn freeing_a_buffer_reclaims_it() {
        let boxed = vec![7u8; 64].into_boxed_slice();
        let len = boxed.len();
        let ptr = Box::into_raw(boxed).cast::<u8>();
        gfr_crypto_free_buffer(ptr, len);
    }

    #[test]
    fn freeing_a_zero_length_buffer_is_a_no_op() {
        let boxed: Box<[u8]> = Vec::new().into_boxed_slice();
        let ptr = Box::into_raw(boxed).cast::<u8>();
        gfr_crypto_free_buffer(ptr, 0);
    }

    #[test]
    fn freeing_a_buffer_containing_nul_bytes_is_fine() {
        // The length-delimited free must not depend on a terminator.
        let boxed = vec![0u8, 1, 0, 2, 0].into_boxed_slice();
        let len = boxed.len();
        let ptr = Box::into_raw(boxed).cast::<u8>();
        gfr_crypto_free_buffer(ptr, len);
    }

    // -- free_recipients -----------------------------------------------------

    #[test]
    fn freeing_null_recipients_is_a_no_op() {
        gfr_crypto_free_recipients(std::ptr::null_mut(), 0);
        gfr_crypto_free_recipients(std::ptr::null_mut(), 4);
    }

    #[test]
    fn freeing_a_zero_count_recipient_array_is_a_no_op() {
        let boxed: Box<[GfrRecipientResultC]> = Vec::new().into_boxed_slice();
        let ptr = Box::into_raw(boxed).cast::<GfrRecipientResultC>();
        gfr_crypto_free_recipients(ptr, 0);
    }

    #[test]
    fn a_recipient_array_round_trips_through_its_free() {
        let items = vec![
            GfrRecipientResultC {
                key_id: c_str("AABBCCDDEEFF0011"),
                pub_algo: c_str("ED25519"),
                status: crate::types::GfrRecipientStatus::Success,
            },
            GfrRecipientResultC {
                key_id: c_str("1122334455667788"),
                pub_algo: c_str("X25519"),
                status: crate::types::GfrRecipientStatus::NoKey,
            },
        ];
        let boxed = items.into_boxed_slice();
        let count = boxed.len();
        let ptr = Box::into_raw(boxed).cast::<GfrRecipientResultC>();
        gfr_crypto_free_recipients(ptr, count);
    }

    // -- free_key_generate_result --------------------------------------------

    #[test]
    fn freeing_a_null_key_generate_result_is_a_no_op() {
        gfr_crypto_free_key_generate_result(std::ptr::null_mut());
    }

    #[test]
    fn freeing_a_key_generate_result_nulls_its_fields() {
        // Nulling is what makes a second call safe, which C++ relies on in its
        // RAII wrappers.
        let mut result = GfrKeyGenerateResult {
            secret_key: c_str("-----BEGIN PGP PRIVATE KEY BLOCK-----"),
            public_key: c_str("-----BEGIN PGP PUBLIC KEY BLOCK-----"),
            fingerprint: c_str("AABBCCDD"),
        };
        gfr_crypto_free_key_generate_result(&mut result);
        assert!(result.secret_key.is_null());
        assert!(result.public_key.is_null());
        assert!(result.fingerprint.is_null());
    }

    #[test]
    fn freeing_a_key_generate_result_twice_is_safe() {
        let mut result = GfrKeyGenerateResult {
            secret_key: c_str("secret"),
            public_key: c_str("public"),
            fingerprint: c_str("fpr"),
        };
        gfr_crypto_free_key_generate_result(&mut result);
        gfr_crypto_free_key_generate_result(&mut result);
        assert!(result.secret_key.is_null());
    }

    #[test]
    fn freeing_a_partially_populated_key_generate_result_is_safe() {
        let mut result = GfrKeyGenerateResult {
            secret_key: std::ptr::null_mut(),
            public_key: c_str("public"),
            fingerprint: std::ptr::null_mut(),
        };
        gfr_crypto_free_key_generate_result(&mut result);
        assert!(result.public_key.is_null());
    }

    #[test]
    fn a_generated_key_result_round_trips_through_its_free() {
        // The end-to-end shape, through the real generation entry point.
        let uid = CString::new("FFI <ffi@example.test>").expect("no NUL");
        let cfg = crate::testutil::keys::cfg(
            crate::types::GfrKeyAlgo::ED25519,
            true,
            false,
            crate::types::GfrOpenPGPKeyVersion::V4,
        );
        let mut out = GfrKeyGenerateResult {
            secret_key: std::ptr::null_mut(),
            public_key: std::ptr::null_mut(),
            fingerprint: std::ptr::null_mut(),
        };
        let status = crate::ffi::keygen::gfr_crypto_generate_key(
            uid.as_ptr(),
            cfg,
            std::ptr::null(),
            0,
            crate::testutil::cb::pwd_correct,
            &mut out,
        );
        assert_eq!(status, GfrStatus::Success);
        assert!(!out.secret_key.is_null());
        assert!(!out.public_key.is_null());
        assert!(!out.fingerprint.is_null());
        gfr_crypto_free_key_generate_result(&mut out);
        assert!(out.secret_key.is_null());
    }

    // -- metadata blocks -----------------------------------------------------

    #[test]
    fn freeing_null_metadata_blocks_is_a_no_op() {
        gfr_crypto_free_sign_metadata(std::ptr::null_mut());
        gfr_crypto_free_encrypt_metadata(std::ptr::null_mut());
        gfr_crypto_free_decrypt_metadata(std::ptr::null_mut());
        gfr_crypto_free_verify_metadata(std::ptr::null_mut());
    }

    #[test]
    fn freeing_an_empty_sign_metadata_block_is_safe() {
        let mut meta = GfrSignMetadataC {
            signatures: std::ptr::null_mut(),
            signature_count: 0,
        };
        gfr_crypto_free_sign_metadata(&mut meta);
        assert!(meta.signatures.is_null());
        assert_eq!(meta.signature_count, 0);
    }

    #[test]
    fn freeing_a_sign_metadata_block_reclaims_every_string() {
        let sigs = vec![GfrSignatureResultC {
            sig_type: crate::types::GfrSignMode::Detached,
            issuer_fpr: c_str("AABBCCDD"),
            status: crate::types::GfrSignatureStatus::Valid,
            created_at: 1_700_000_000,
            pub_algo: c_str("ED25519"),
            hash_algo: c_str("SHA512"),
        }];
        let boxed = sigs.into_boxed_slice();
        let count = boxed.len();
        let mut meta = GfrSignMetadataC {
            signatures: Box::into_raw(boxed).cast::<GfrSignatureResultC>(),
            signature_count: count,
        };
        gfr_crypto_free_sign_metadata(&mut meta);
        assert!(meta.signatures.is_null());
        assert_eq!(meta.signature_count, 0);
    }

    #[test]
    fn freeing_a_sign_metadata_block_twice_is_safe() {
        let mut meta = GfrSignMetadataC {
            signatures: std::ptr::null_mut(),
            signature_count: 0,
        };
        gfr_crypto_free_sign_metadata(&mut meta);
        gfr_crypto_free_sign_metadata(&mut meta);
    }

    // -- result structs ------------------------------------------------------

    #[test]
    fn freeing_null_results_is_a_no_op() {
        gfr_crypto_free_encrypt_result(std::ptr::null_mut());
        gfr_crypto_free_decrypt_result(std::ptr::null_mut());
        gfr_crypto_free_sign_result(std::ptr::null_mut());
        gfr_crypto_free_verify_result(std::ptr::null_mut());
        gfr_crypto_free_encrypt_and_sign_result(std::ptr::null_mut());
        gfr_crypto_free_decrypt_and_verify_result(std::ptr::null_mut());
    }

    #[test]
    fn an_encrypt_result_round_trips_through_its_free() {
        // The realistic path: encrypt through the FFI, then free with the
        // matching deallocator.
        let key = &*crate::testutil::keys::V4_SIGN;
        let name = CString::new("").expect("no NUL");
        let block = key.public_armored.as_bytes();
        let bufs = [crate::types::GfrBuffer {
            data: block.as_ptr(),
            len: block.len(),
        }];
        let payload = b"ffi payload";
        let mut out = std::mem::MaybeUninit::<GfrEncryptResultC>::zeroed();

        let status = crate::ffi::crypto::encrypt::gfr_crypto_encrypt_data(
            0,
            name.as_ptr(),
            payload.as_ptr(),
            payload.len(),
            bufs.as_ptr(),
            bufs.len(),
            true,
            out.as_mut_ptr(),
        );
        assert_eq!(status, GfrStatus::Success);

        let mut result = unsafe { out.assume_init() };
        assert!(!result.data.is_null());
        assert!(result.data_len > 0);
        gfr_crypto_free_encrypt_result(&mut result);
    }

    #[test]
    fn a_file_shaped_encrypt_result_with_a_null_data_pointer_frees_cleanly() {
        // File-mode operations write to disk and leave `data` null; the free
        // must cope rather than dereferencing it.
        let mut result = GfrEncryptResultC {
            data: std::ptr::null_mut(),
            data_len: 0,
            meta: GfrEncryptMetadataC {
                invalid_recipients: std::ptr::null_mut(),
                invalid_recipient_count: 0,
                recipients: std::ptr::null_mut(),
                recipient_count: 0,
            },
        };
        gfr_crypto_free_encrypt_result(&mut result);
    }

    // -- free_metadata_array --------------------------------------------------

    #[test]
    fn freeing_a_null_metadata_array_is_a_no_op() {
        unsafe { gfr_free_metadata_array(std::ptr::null_mut(), 0) };
        unsafe { gfr_free_metadata_array(std::ptr::null_mut(), 3) };
    }

    #[test]
    fn a_metadata_array_round_trips_through_extract_and_free() {
        // The pairing C++ actually uses when importing a key.
        let key = &*crate::testutil::keys::V4_SIGN;
        let block = key.public_armored.as_bytes();
        let buf = crate::types::GfrBuffer {
            data: block.as_ptr(),
            len: block.len(),
        };
        let mut list: *mut crate::types::GfrKeyMetadataC = std::ptr::null_mut();
        let mut count: usize = 0;

        let status = crate::ffi::key::gfr_crypto_extract_metadata(buf, &mut list, &mut count);
        assert_eq!(status, GfrStatus::Success);
        assert_eq!(count, 1);
        assert!(!list.is_null());

        unsafe { gfr_free_metadata_array(list, count) };
    }

    #[test]
    fn a_metadata_array_with_a_zero_count_frees_cleanly() {
        let boxed: Box<[crate::types::GfrKeyMetadataC]> = Vec::new().into_boxed_slice();
        let ptr = Box::into_raw(boxed).cast::<crate::types::GfrKeyMetadataC>();
        unsafe { gfr_free_metadata_array(ptr, 0) };
    }
}
