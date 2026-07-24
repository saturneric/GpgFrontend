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

//! FFI entry points for user ID (UID) management operations.
//!
//! All functions operate on armored key blocks passed as C strings and write
//! an updated armored key block to an output pointer. The caller must free the
//! returned string with `gfr_crypto_free_string`.

use crate::types::{GfrBuffer, GfrRevocationCode, GfrStatus};
use crate::user_id::{
    add_user_id_internal, delete_user_id_internal, revoke_user_id_internal,
    set_primary_user_id_internal, update_user_id_internal,
};
use crate::{err::clear_last_error, types::GfrPasswordFetchCb};
use std::{
    ffi::{CStr, CString, c_char},
    panic::catch_unwind,
};

/// Delete the user ID matching `target_uid` from the armored `key_block`.
///
/// On success `*out_block` is set to a heap-allocated updated armored key
/// block. Free it with `gfr_crypto_free_string`.
///
/// # Safety
/// `key_block`, `target_uid`, and `out_block` must all be non-null.
#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_delete_user_id(
    key_block: GfrBuffer,
    target_uid: *const c_char,
    out_block: *mut *mut c_char,
) -> GfrStatus {
    clear_last_error();

    let result = catch_unwind(|| -> Result<(), GfrStatus> {
        if target_uid.is_null() || out_block.is_null() {
            return Err(GfrStatus::ErrorInvalidInput);
        }

        let block_str = unsafe { key_block.as_str() }?;
        let uid_str = unsafe { CStr::from_ptr(target_uid) }.to_str().unwrap_or("");

        let new_block = delete_user_id_internal(block_str, uid_str)?;

        unsafe {
            *out_block = CString::new(new_block.as_bytes())
                .map_err(|_| GfrStatus::ErrorInternal)?
                .into_raw();
        }

        Ok(())
    });

    match result {
        Ok(Ok(_)) => GfrStatus::Success,
        Ok(Err(e)) => e,
        Err(_) => GfrStatus::ErrorPanic,
    }
}

/// Add `new_uid` to the armored `secret_key_block`.
///
/// The primary key passphrase is obtained via `fetch_pwd_cb`.
/// On success `*out_block` is set to a heap-allocated updated armored key block.
///
/// # Safety
/// `secret_key_block`, `new_uid`, and `out_block` must all be non-null.
#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_add_user_id(
    channel: i32,
    secret_key_block: GfrBuffer,
    new_uid: *const c_char,
    fetch_pwd_cb: GfrPasswordFetchCb,
    out_block: *mut *mut c_char,
) -> GfrStatus {
    clear_last_error();

    let result = catch_unwind(|| -> Result<(), GfrStatus> {
        if new_uid.is_null() || out_block.is_null() {
            return Err(GfrStatus::ErrorInvalidInput);
        }

        let block_str = unsafe { secret_key_block.as_str() }?;
        let uid_str = unsafe { CStr::from_ptr(new_uid) }.to_str().unwrap_or("");

        let new_block = add_user_id_internal(channel, block_str, uid_str, Some(fetch_pwd_cb))?;

        unsafe {
            *out_block = CString::new(new_block.as_bytes())
                .map_err(|_| GfrStatus::ErrorInternal)?
                .into_raw();
        }

        Ok(())
    });

    match result {
        Ok(Ok(_)) => GfrStatus::Success,
        Ok(Err(e)) => e,
        Err(_) => GfrStatus::ErrorPanic,
    }
}

/// Replace `old_uid` with `new_uid` in the armored `secret_key_block`.
///
/// The primary key passphrase is obtained via `fetch_pwd_cb`.
/// On success `*out_block` is set to a heap-allocated updated armored key block.
///
/// # Safety
/// `secret_key_block`, `old_uid`, `new_uid`, and `out_block` must all be non-null.
#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_update_user_id(
    channel: i32,
    secret_key_block: GfrBuffer,
    old_uid: *const c_char,
    new_uid: *const c_char,
    fetch_pwd_cb: GfrPasswordFetchCb,
    out_block: *mut *mut c_char,
) -> GfrStatus {
    clear_last_error();

    let result = catch_unwind(|| -> Result<(), GfrStatus> {
        if old_uid.is_null() || new_uid.is_null() || out_block.is_null() {
            return Err(GfrStatus::ErrorInvalidInput);
        }

        let block_str = unsafe { secret_key_block.as_str() }?;
        let old_str = unsafe { CStr::from_ptr(old_uid) }.to_str().unwrap_or("");
        let new_str = unsafe { CStr::from_ptr(new_uid) }.to_str().unwrap_or("");

        let new_block =
            update_user_id_internal(channel, block_str, old_str, new_str, Some(fetch_pwd_cb))?;

        unsafe {
            *out_block = CString::new(new_block.as_bytes())
                .map_err(|_| GfrStatus::ErrorInternal)?
                .into_raw();
        }

        Ok(())
    });

    match result {
        Ok(Ok(_)) => GfrStatus::Success,
        Ok(Err(e)) => e,
        Err(_) => GfrStatus::ErrorPanic,
    }
}

/// Mark `target_uid` as the primary user ID in the armored `secret_key_block`.
///
/// The primary key passphrase is obtained via `fetch_pwd_cb`.
/// On success `*out_block` is set to a heap-allocated updated armored key block.
///
/// # Safety
/// `secret_key_block`, `target_uid`, and `out_block` must all be non-null.
#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_set_primary_user_id(
    channel: i32,
    secret_key_block: GfrBuffer,
    target_uid: *const std::os::raw::c_char,
    fetch_pwd_cb: GfrPasswordFetchCb,
    out_block: *mut *mut std::os::raw::c_char,
) -> GfrStatus {
    let result = std::panic::catch_unwind(|| -> Result<(), GfrStatus> {
        if target_uid.is_null() || out_block.is_null() {
            return Err(GfrStatus::ErrorInvalidInput);
        }

        let block_str = unsafe { secret_key_block.as_str() }?;
        let uid_str = unsafe { std::ffi::CStr::from_ptr(target_uid) }
            .to_str()
            .unwrap_or("");

        let new_block =
            set_primary_user_id_internal(channel, block_str, uid_str, Some(fetch_pwd_cb))?;

        unsafe {
            *out_block = std::ffi::CString::new(new_block.as_bytes())
                .map_err(|_| GfrStatus::ErrorInternal)?
                .into_raw();
        }

        Ok(())
    });

    match result {
        Ok(Ok(_)) => GfrStatus::Success,
        Ok(Err(e)) => e,
        Err(_) => GfrStatus::ErrorPanic,
    }
}

/// Revoke `target_uid` in the armored `secret_key_block`.
///
/// `reason_code` identifies why the UID is being revoked; `reason_text` is an
/// optional human-readable description (may be null). The passphrase is
/// obtained via `fetch_pwd_cb`. On success `*out_block` is set to a
/// heap-allocated updated armored key block.
///
/// # Safety
/// `secret_key_block`, `target_uid`, and `out_block` must all be non-null.
/// `reason_text` may be null.
#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_revoke_user_id(
    channel: i32,
    secret_key_block: GfrBuffer,
    target_uid: *const c_char,
    reason_code: GfrRevocationCode,
    reason_text: *const c_char,
    fetch_pwd_cb: GfrPasswordFetchCb,
    out_block: *mut *mut c_char,
) -> GfrStatus {
    clear_last_error();

    if !out_block.is_null() {
        unsafe {
            *out_block = std::ptr::null_mut();
        }
    }

    let result = catch_unwind(|| -> Result<(), GfrStatus> {
        if target_uid.is_null() || out_block.is_null() {
            return Err(GfrStatus::ErrorInvalidInput);
        }

        let block_str = unsafe { secret_key_block.as_str() }?;

        let uid_str = unsafe { CStr::from_ptr(target_uid) }
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

        let new_block = revoke_user_id_internal(
            channel,
            block_str,
            uid_str,
            reason_code,
            reason_text,
            Some(fetch_pwd_cb),
        )?;

        let c_new_block =
            CString::new(new_block.as_bytes()).map_err(|_| GfrStatus::ErrorInternal)?;

        unsafe {
            *out_block = c_new_block.into_raw();
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
mod ffi_user_id_tests {
    //! The five user ID FFI entry points.

    use super::*;
    use crate::testutil::{cb, keys};
    use std::ffi::{CStr, CString};

    fn buf(s: &str) -> GfrBuffer {
        GfrBuffer {
            data: s.as_ptr(),
            len: s.len(),
        }
    }

    fn take(ptr: *mut c_char) -> String {
        assert!(!ptr.is_null());
        let s = unsafe { CStr::from_ptr(ptr) }.to_string_lossy().into_owned();
        crate::ffi::mem::gfr_crypto_free_string(ptr);
        s
    }

    fn first_uid(block: &str) -> String {
        crate::key::extract_metadata_many_internal(block).expect("meta")[0].user_ids[0]
            .user_id
            .clone()
    }

    #[test]
    fn delete_user_id_rejects_null_arguments() {
        let uid = CString::new("x").expect("no NUL");
        assert_eq!(
            gfr_crypto_delete_user_id(
                buf(&keys::V4_SIGN.secret_armored),
                std::ptr::null(),
                &mut std::ptr::null_mut()
            ),
            GfrStatus::ErrorInvalidInput
        );
        assert_eq!(
            gfr_crypto_delete_user_id(
                buf(&keys::V4_SIGN.secret_armored),
                uid.as_ptr(),
                std::ptr::null_mut()
            ),
            GfrStatus::ErrorInvalidInput
        );
    }

    #[test]
    fn delete_user_id_rejects_an_empty_key_block() {
        let uid = CString::new("x").expect("no NUL");
        let mut out = std::ptr::null_mut();
        assert_eq!(
            gfr_crypto_delete_user_id(GfrBuffer::empty(), uid.as_ptr(), &mut out),
            GfrStatus::ErrorInvalidInput
        );
    }

    #[test]
    fn delete_user_id_removes_the_named_identity() {
        let key = &keys::V4_SIGN;
        let existing = first_uid(&key.secret_armored);
        let uid = CString::new(existing.as_str()).expect("no NUL");
        let mut out = std::ptr::null_mut();
        assert_eq!(
            gfr_crypto_delete_user_id(buf(&key.secret_armored), uid.as_ptr(), &mut out),
            GfrStatus::Success
        );
        let block = take(out);
        let meta = crate::key::extract_metadata_many_internal(&block).expect("meta");
        assert!(meta[0].user_ids.is_empty());
    }

    #[test]
    fn delete_user_id_with_an_unknown_identity_fails() {
        let uid = CString::new("nobody <nobody@example.test>").expect("no NUL");
        let mut out = std::ptr::null_mut();
        assert_ne!(
            gfr_crypto_delete_user_id(
                buf(&keys::V4_SIGN.secret_armored),
                uid.as_ptr(),
                &mut out
            ),
            GfrStatus::Success
        );
    }

    #[test]
    fn add_user_id_rejects_null_arguments() {
        let uid = CString::new("New <new@example.test>").expect("no NUL");
        assert_eq!(
            gfr_crypto_add_user_id(
                0,
                buf(&keys::V4_SIGN.secret_armored),
                std::ptr::null(),
                cb::pwd_correct,
                &mut std::ptr::null_mut()
            ),
            GfrStatus::ErrorInvalidInput
        );
        assert_eq!(
            gfr_crypto_add_user_id(
                0,
                buf(&keys::V4_SIGN.secret_armored),
                uid.as_ptr(),
                cb::pwd_correct,
                std::ptr::null_mut()
            ),
            GfrStatus::ErrorInvalidInput
        );
    }

    #[test]
    fn add_user_id_appends_the_identity() {
        let uid = CString::new("Added <added@example.test>").expect("no NUL");
        let mut out = std::ptr::null_mut();
        assert_eq!(
            gfr_crypto_add_user_id(
                0,
                buf(&keys::V4_SIGN.secret_armored),
                uid.as_ptr(),
                cb::pwd_correct,
                &mut out
            ),
            GfrStatus::Success
        );
        let block = take(out);
        let meta = crate::key::extract_metadata_many_internal(&block).expect("meta");
        assert_eq!(meta[0].user_ids.len(), 2);
    }

    #[test]
    fn add_user_id_accepts_a_multibyte_identity() {
        // RFC 9580 §5.11: a user ID is UTF-8 text with no content restriction.
        let uid = CString::new("Renée 鍵 <r@example.test>").expect("no NUL");
        let mut out = std::ptr::null_mut();
        assert_eq!(
            gfr_crypto_add_user_id(
                0,
                buf(&keys::V4_SIGN.secret_armored),
                uid.as_ptr(),
                cb::pwd_correct,
                &mut out
            ),
            GfrStatus::Success
        );
        let block = take(out);
        assert!(
            crate::key::extract_metadata_many_internal(&block).expect("meta")[0]
                .user_ids
                .iter()
                .any(|u| u.user_id.contains('鍵'))
        );
    }

    #[test]
    fn add_user_id_to_a_public_block_fails() {
        let uid = CString::new("X <x@example.test>").expect("no NUL");
        let mut out = std::ptr::null_mut();
        assert_ne!(
            gfr_crypto_add_user_id(
                0,
                buf(&keys::V4_SIGN.public_armored),
                uid.as_ptr(),
                cb::pwd_correct,
                &mut out
            ),
            GfrStatus::Success
        );
    }

    #[test]
    fn update_user_id_rejects_null_arguments() {
        let old = CString::new("a").expect("no NUL");
        assert_eq!(
            gfr_crypto_update_user_id(
                0,
                buf(&keys::V4_SIGN.secret_armored),
                old.as_ptr(),
                std::ptr::null(),
                cb::pwd_correct,
                &mut std::ptr::null_mut()
            ),
            GfrStatus::ErrorInvalidInput
        );
    }

    #[test]
    fn set_primary_user_id_rejects_null_arguments() {
        assert_eq!(
            gfr_crypto_set_primary_user_id(
                0,
                buf(&keys::V4_SIGN.secret_armored),
                std::ptr::null(),
                cb::pwd_correct,
                &mut std::ptr::null_mut()
            ),
            GfrStatus::ErrorInvalidInput
        );
    }

    #[test]
    fn set_primary_user_id_marks_the_named_identity() {
        let key = &keys::V4_SIGN;
        let existing = first_uid(&key.secret_armored);
        let uid = CString::new(existing.as_str()).expect("no NUL");
        let mut out = std::ptr::null_mut();
        assert_eq!(
            gfr_crypto_set_primary_user_id(
                0,
                buf(&key.secret_armored),
                uid.as_ptr(),
                cb::pwd_correct,
                &mut out
            ),
            GfrStatus::Success
        );
        let block = take(out);
        let meta = crate::key::extract_metadata_many_internal(&block).expect("meta");
        assert!(meta[0].user_ids.iter().any(|u| u.is_primary));
    }

    #[test]
    fn revoke_user_id_rejects_null_arguments() {
        assert_eq!(
            gfr_crypto_revoke_user_id(
                0,
                buf(&keys::V4_SIGN.secret_armored),
                std::ptr::null(),
                GfrRevocationCode::UserIdInvalid,
                std::ptr::null(),
                cb::pwd_correct,
                &mut std::ptr::null_mut()
            ),
            GfrStatus::ErrorInvalidInput
        );
    }

    #[test]
    fn revoke_user_id_flags_the_identity_without_deleting_it() {
        // §5.2.1.13: revocation is a statement; third parties need the user ID
        // present in order to see that it was revoked.
        let key = &keys::V4_SIGN;
        let existing = first_uid(&key.secret_armored);
        let uid = CString::new(existing.as_str()).expect("no NUL");
        let mut out = std::ptr::null_mut();
        assert_eq!(
            gfr_crypto_revoke_user_id(
                0,
                buf(&key.secret_armored),
                uid.as_ptr(),
                GfrRevocationCode::UserIdInvalid,
                std::ptr::null(),
                cb::pwd_correct,
                &mut out
            ),
            GfrStatus::Success
        );
        let block = take(out);
        let meta = crate::key::extract_metadata_many_internal(&block).expect("meta");
        assert_eq!(meta[0].user_ids.len(), 1);
        assert!(meta[0].user_ids[0].is_revoked);
    }

    #[test]
    fn every_user_id_entry_point_rejects_an_empty_key_block() {
        let uid = CString::new("x").expect("no NUL");
        let mut out = std::ptr::null_mut();
        let statuses = [
            gfr_crypto_delete_user_id(GfrBuffer::empty(), uid.as_ptr(), &mut out),
            gfr_crypto_add_user_id(0, GfrBuffer::empty(), uid.as_ptr(), cb::pwd_correct, &mut out),
            gfr_crypto_update_user_id(
                0,
                GfrBuffer::empty(),
                uid.as_ptr(),
                uid.as_ptr(),
                cb::pwd_correct,
                &mut out,
            ),
            gfr_crypto_set_primary_user_id(
                0,
                GfrBuffer::empty(),
                uid.as_ptr(),
                cb::pwd_correct,
                &mut out,
            ),
            gfr_crypto_revoke_user_id(
                0,
                GfrBuffer::empty(),
                uid.as_ptr(),
                GfrRevocationCode::NoReason,
                std::ptr::null(),
                cb::pwd_correct,
                &mut out,
            ),
        ];
        for status in statuses {
            assert_ne!(status, GfrStatus::Success);
        }
    }

    #[test]
    fn user_id_entry_points_never_panic_on_a_garbage_block() {
        let uid = CString::new("x").expect("no NUL");
        let outcome = std::panic::catch_unwind(|| {
            let mut out = std::ptr::null_mut();
            let _ = gfr_crypto_delete_user_id(buf("junk"), uid.as_ptr(), &mut out);
            let _ = gfr_crypto_add_user_id(0, buf("junk"), uid.as_ptr(), cb::pwd_correct, &mut out);
            let _ = gfr_crypto_set_primary_user_id(
                0,
                buf("junk"),
                uid.as_ptr(),
                cb::pwd_correct,
                &mut out,
            );
        });
        assert!(outcome.is_ok());
    }
}
