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
 */

//! FFI entry points for OpenPGP message and file encrypt/decrypt/sign/verify operations.

pub mod decrypt;
pub mod encrypt;
pub mod sign;

use crate::types::GfrStatus;

/// Result structs that are valid when every byte is zero: only pointers,
/// lengths and flags, so all-zero means "empty, nothing to free".
pub(crate) trait ZeroableResult {}

impl ZeroableResult for crate::types::GfrEncryptResultC {}
impl ZeroableResult for crate::types::GfrDecryptResultC {}
impl ZeroableResult for crate::types::GfrSignResultC {}
impl ZeroableResult for crate::types::GfrVerifyResultC {}
impl ZeroableResult for crate::types::GfrEncryptAndSignResultC {}
impl ZeroableResult for crate::types::GfrDecryptAndVerifyResultC {}

/// The key blocks a C caller passed as an array of `GfrBuffer`, as string
/// slices. A block that is empty or not UTF-8 refuses the whole call.
///
/// # Safety
/// `blocks` must point to `count` valid `GfrBuffer`s that outlive the result.
pub(crate) unsafe fn key_blocks_arg<'a>(
    blocks: *const crate::types::GfrBuffer,
    count: usize,
) -> Result<Vec<&'a str>, GfrStatus> {
    unsafe { std::slice::from_raw_parts(blocks, count) }
        .iter()
        .map(|block| unsafe { block.as_str() })
        .collect()
}

/// On `ErrorUnusableKey`, put the recipients the encryption rejected into
/// `meta`'s invalid-recipient list, so the caller learns each one's reason
/// even though no message was produced. Freed with the result as usual.
pub(crate) fn report_rejected_recipients(
    status: GfrStatus,
    meta: *mut crate::types::GfrEncryptMetadataC,
) -> GfrStatus {
    let rejected = crate::crypto::take_rejected_recipients();
    if status != GfrStatus::ErrorUnusableKey || meta.is_null() || rejected.is_empty() {
        return status;
    }
    let recs: Box<[crate::types::GfrInvalidRecipientC]> = rejected
        .into_iter()
        .map(|rec| crate::types::GfrInvalidRecipientC {
            fpr: std::ffi::CString::new(rec.fpr)
                .unwrap_or_default()
                .into_raw(),
            reason: rec.reason,
        })
        .collect();
    let count = recs.len();
    let ptr = Box::into_raw(recs) as *mut crate::types::GfrInvalidRecipientC;
    // SAFETY: non-null, caller-owned, zeroed by reset_out_result on entry.
    unsafe {
        (*meta).invalid_recipients = ptr;
        (*meta).invalid_recipient_count = count;
    }
    status
}

/// Empty `out_result` before any fallible work. C++ calls the matching
/// `gfr_crypto_free_*` after every call whatever the status, so an entry
/// point that fails early must not leave the caller's struct uninitialised.
pub(crate) fn reset_out_result<T: ZeroableResult>(out_result: *mut T) {
    if !out_result.is_null() {
        // SAFETY: non-null, caller-owned, and all-zero is a valid `T`.
        unsafe { std::ptr::write_bytes(out_result, 0, 1) };
    }
}
