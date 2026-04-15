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

use crate::types::{
    GfrDecryptAndVerifyResultC, GfrDecryptMetadataC, GfrDecryptResultC, GfrEncryptAndSignResultC,
    GfrEncryptMetadataC, GfrEncryptResultC, GfrKeyGenerateResult, GfrKeyMetadataC,
    GfrSignMetadataC, GfrSignResultC, GfrVerifyMetadataC, GfrVerifyResultC,
};
use std::ffi::{CString, c_char};

#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_free_string(ptr: *mut c_char) {
    if !ptr.is_null() {
        unsafe { drop(CString::from_raw(ptr)) }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_free_buffer(ptr: *mut u8, len: usize) {
    if !ptr.is_null() && len > 0 {
        unsafe {
            let _ = Vec::from_raw_parts(ptr, len, len);
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_free_key_generate_result(result: *mut GfrKeyGenerateResult) {
    if result.is_null() {
        return;
    }
    unsafe {
        if !(*result).secret_key.is_null() {
            drop(CString::from_raw((*result).secret_key));
        }
        if !(*result).public_key.is_null() {
            drop(CString::from_raw((*result).public_key));
        }
        if !(*result).fingerprint.is_null() {
            drop(CString::from_raw((*result).fingerprint));
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
                    drop(CString::from_raw(sig.issuer_fpr));
                }
                if !sig.pub_algo.is_null() {
                    drop(CString::from_raw(sig.pub_algo));
                }
                if !sig.hash_algo.is_null() {
                    drop(CString::from_raw(sig.hash_algo));
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
                    drop(CString::from_raw(rec.fpr));
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
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_free_decrypt_metadata(meta: *mut GfrDecryptMetadataC) {
    if meta.is_null() {
        return;
    }

    unsafe {
        if !(*meta).filename.is_null() {
            drop(CString::from_raw((*meta).filename));
        }

        if !(*meta).recipients.is_null() && (*meta).recipient_count > 0 {
            let recs_slice =
                std::slice::from_raw_parts_mut((*meta).recipients, (*meta).recipient_count);
            for rec in recs_slice.iter_mut() {
                if !rec.key_id.is_null() {
                    drop(CString::from_raw(rec.key_id));
                }
                if !rec.pub_algo.is_null() {
                    drop(CString::from_raw(rec.pub_algo));
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
                    drop(CString::from_raw(sig.issuer_fpr));
                }
                if !sig.pub_algo.is_null() {
                    drop(CString::from_raw(sig.pub_algo));
                }
                if !sig.hash_algo.is_null() {
                    drop(CString::from_raw(sig.hash_algo));
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
        // 1. Free the payload data
        if !(*result).data.is_null() && (*result).data_len > 0 {
            let _ = Vec::from_raw_parts((*result).data, (*result).data_len, (*result).data_len);
            (*result).data = std::ptr::null_mut();
            (*result).data_len = 0;
        }

        // 2. Delegate freeing metadata to our helper
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
        // 1. Free the payload data
        if !(*result).data.is_null() && (*result).data_len > 0 {
            let _ = Vec::from_raw_parts((*result).data, (*result).data_len, (*result).data_len);
            (*result).data = std::ptr::null_mut();
            (*result).data_len = 0;
        }

        // 2. Delegate freeing metadata to our helper
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
        // 1. Free the payload data
        if !(*result).data.is_null() && (*result).data_len > 0 {
            let _ = Vec::from_raw_parts((*result).data, (*result).data_len, (*result).data_len);
            (*result).data = std::ptr::null_mut();
            (*result).data_len = 0;
        }

        // 2. Delegate freeing metadata to our helper
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
        // 1. Free the payload data
        if !(*result).data.is_null() && (*result).data_len > 0 {
            let _ = Vec::from_raw_parts((*result).data, (*result).data_len, (*result).data_len);
            (*result).data = std::ptr::null_mut();
            (*result).data_len = 0;
        }

        // 2. Delegate freeing metadata to our helper
        gfr_crypto_free_verify_metadata(&mut (*result).meta);
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_free_encrypt_and_sign_result(result: *mut GfrEncryptAndSignResultC) {
    if result.is_null() {
        return;
    }
    unsafe {
        // 1. Free the payload data
        if !(*result).data.is_null() && (*result).data_len > 0 {
            let _ = Vec::from_raw_parts((*result).data, (*result).data_len, (*result).data_len);
            (*result).data = std::ptr::null_mut();
            (*result).data_len = 0;
        }

        // 2. Delegate freeing metadata to our helpers
        gfr_crypto_free_sign_metadata(&mut (*result).sign_meta);
        gfr_crypto_free_encrypt_metadata(&mut (*result).encrypt_meta);
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_free_decrypt_and_verify_result(
    result: *mut GfrDecryptAndVerifyResultC,
) {
    unsafe {
        // 1. Free the payload data
        if !(*result).data.is_null() && (*result).data_len > 0 {
            let _ = Vec::from_raw_parts((*result).data, (*result).data_len, (*result).data_len);
            (*result).data = std::ptr::null_mut();
            (*result).data_len = 0;
        }

        // 2. Delegate freeing metadata to our helper
        gfr_crypto_free_decrypt_metadata(&mut (*result).decrypt_meta);
        gfr_crypto_free_verify_metadata(&mut (*result).verify_meta);
    }
}

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
            let _ = unsafe { CString::from_raw(meta.fpr as *mut _) };
        }
        if !meta.key_id.is_null() {
            let _ = unsafe { CString::from_raw(meta.key_id as *mut _) };
        }

        if !meta.public_key_block.is_null() {
            let _ = unsafe { CString::from_raw(meta.public_key_block as *mut _) };
        }
        if !meta.secret_key_block.is_null() {
            let _ = unsafe { CString::from_raw(meta.secret_key_block as *mut _) };
        }

        if meta.user_ids.is_null() == false && meta.user_id_count > 0 {
            let uids_slice =
                unsafe { std::slice::from_raw_parts_mut(meta.user_ids, meta.user_id_count) };
            for uid in uids_slice.iter_mut() {
                if !uid.is_null() {
                    let _ = unsafe { CString::from_raw(*uid as *mut _) };
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
                    let _ = unsafe { CString::from_raw(sub.fpr as *mut _) };
                }
                if !sub.key_id.is_null() {
                    let _ = unsafe { CString::from_raw(sub.key_id as *mut _) };
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
