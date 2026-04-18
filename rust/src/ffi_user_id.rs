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
use crate::types::{GfrFreeCb, GfrRevocationCode, GfrStatus};
use crate::user_id::{
    add_user_id_internal, delete_user_id_internal, revoke_user_id_internal,
    set_primary_user_id_internal, update_user_id_internal,
};
use crate::{err::clear_last_error, types::GfrPasswordFetchCb};
use std::{
    ffi::{CStr, CString, c_char},
    panic::catch_unwind,
};

#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_delete_user_id(
    key_block: *const c_char,
    target_uid: *const c_char,
    out_block: *mut *mut c_char,
) -> GfrStatus {
    clear_last_error();

    let result = catch_unwind(|| -> Result<(), GfrStatus> {
        if key_block.is_null() || target_uid.is_null() || out_block.is_null() {
            return Err(GfrStatus::ErrorInvalidInput);
        }

        let block_str = unsafe { CStr::from_ptr(key_block) }.to_str().unwrap_or("");
        let uid_str = unsafe { CStr::from_ptr(target_uid) }.to_str().unwrap_or("");

        let new_block = delete_user_id_internal(block_str, uid_str)?;

        unsafe {
            *out_block = CString::new(new_block).unwrap_or_default().into_raw();
        }

        Ok(())
    });

    match result {
        Ok(Ok(_)) => GfrStatus::Success,
        Ok(Err(e)) => e,
        Err(_) => GfrStatus::ErrorPanic,
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_add_user_id(
    channel: i32,
    secret_key_block: *const c_char,
    new_uid: *const c_char,
    fetch_pwd_cb: GfrPasswordFetchCb,
    free_cb: GfrFreeCb,
    out_block: *mut *mut c_char,
) -> GfrStatus {
    clear_last_error();

    let result = catch_unwind(|| -> Result<(), GfrStatus> {
        if secret_key_block.is_null() || new_uid.is_null() || out_block.is_null() {
            return Err(GfrStatus::ErrorInvalidInput);
        }

        let block_str = unsafe { CStr::from_ptr(secret_key_block) }
            .to_str()
            .unwrap_or("");
        let uid_str = unsafe { CStr::from_ptr(new_uid) }.to_str().unwrap_or("");

        let new_block = add_user_id_internal(
            channel,
            block_str,
            uid_str,
            Some(fetch_pwd_cb),
            Some(free_cb),
        )?;

        unsafe {
            *out_block = CString::new(new_block).unwrap_or_default().into_raw();
        }

        Ok(())
    });

    match result {
        Ok(Ok(_)) => GfrStatus::Success,
        Ok(Err(e)) => e,
        Err(_) => GfrStatus::ErrorPanic,
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_update_user_id(
    channel: i32,
    secret_key_block: *const c_char,
    old_uid: *const c_char,
    new_uid: *const c_char,
    fetch_pwd_cb: GfrPasswordFetchCb,
    free_cb: GfrFreeCb,
    out_block: *mut *mut c_char,
) -> GfrStatus {
    clear_last_error();

    let result = catch_unwind(|| -> Result<(), GfrStatus> {
        if secret_key_block.is_null()
            || old_uid.is_null()
            || new_uid.is_null()
            || out_block.is_null()
        {
            return Err(GfrStatus::ErrorInvalidInput);
        }

        let block_str = unsafe { CStr::from_ptr(secret_key_block) }
            .to_str()
            .unwrap_or("");
        let old_str = unsafe { CStr::from_ptr(old_uid) }.to_str().unwrap_or("");
        let new_str = unsafe { CStr::from_ptr(new_uid) }.to_str().unwrap_or("");

        let new_block = update_user_id_internal(
            channel,
            block_str,
            old_str,
            new_str,
            Some(fetch_pwd_cb),
            Some(free_cb),
        )?;

        unsafe {
            *out_block = CString::new(new_block).unwrap_or_default().into_raw();
        }

        Ok(())
    });

    match result {
        Ok(Ok(_)) => GfrStatus::Success,
        Ok(Err(e)) => e,
        Err(_) => GfrStatus::ErrorPanic,
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_set_primary_user_id(
    channel: i32,
    secret_key_block: *const std::os::raw::c_char,
    target_uid: *const std::os::raw::c_char,
    fetch_pwd_cb: crate::types::GfrPasswordFetchCb,
    free_cb: crate::types::GfrFreeCb,
    out_block: *mut *mut std::os::raw::c_char,
) -> GfrStatus {
    // crate::error_handler::clear_last_error();

    let result = std::panic::catch_unwind(|| -> Result<(), GfrStatus> {
        if secret_key_block.is_null() || target_uid.is_null() || out_block.is_null() {
            return Err(GfrStatus::ErrorInvalidInput);
        }

        let block_str = unsafe { std::ffi::CStr::from_ptr(secret_key_block) }
            .to_str()
            .unwrap_or("");
        let uid_str = unsafe { std::ffi::CStr::from_ptr(target_uid) }
            .to_str()
            .unwrap_or("");

        let new_block = set_primary_user_id_internal(
            channel,
            block_str,
            uid_str,
            Some(fetch_pwd_cb),
            Some(free_cb),
        )?;

        unsafe {
            *out_block = std::ffi::CString::new(new_block)
                .unwrap_or_default()
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

#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_revoke_user_id(
    channel: i32,
    secret_key_block: *const c_char,
    target_uid: *const c_char,
    reason_code: GfrRevocationCode,
    reason_text: *const c_char,
    fetch_pwd_cb: GfrPasswordFetchCb,
    free_cb: GfrFreeCb,
    out_block: *mut *mut c_char,
) -> GfrStatus {
    clear_last_error();

    if !out_block.is_null() {
        unsafe {
            *out_block = std::ptr::null_mut();
        }
    }

    let result = catch_unwind(|| -> Result<(), GfrStatus> {
        if secret_key_block.is_null() || target_uid.is_null() || out_block.is_null() {
            return Err(GfrStatus::ErrorInvalidInput);
        }

        let block_str = unsafe { CStr::from_ptr(secret_key_block) }
            .to_str()
            .map_err(|_| GfrStatus::ErrorInvalidInput)?;

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
            Some(free_cb),
        )?;

        let c_new_block = CString::new(new_block).map_err(|_| GfrStatus::ErrorInternal)?;

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
