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

/// Result structs that are valid when every byte is zero: only pointers,
/// lengths and flags, so all-zero means "empty, nothing to free".
pub(crate) trait ZeroableResult {}

impl ZeroableResult for crate::types::GfrEncryptResultC {}
impl ZeroableResult for crate::types::GfrDecryptResultC {}
impl ZeroableResult for crate::types::GfrSignResultC {}
impl ZeroableResult for crate::types::GfrVerifyResultC {}
impl ZeroableResult for crate::types::GfrEncryptAndSignResultC {}
impl ZeroableResult for crate::types::GfrDecryptAndVerifyResultC {}

/// Empty `out_result` before any fallible work. C++ calls the matching
/// `gfr_crypto_free_*` after every call whatever the status, so an entry
/// point that fails early must not leave the caller's struct uninitialised.
pub(crate) fn reset_out_result<T: ZeroableResult>(out_result: *mut T) {
    if !out_result.is_null() {
        // SAFETY: non-null, caller-owned, and all-zero is a valid `T`.
        unsafe { std::ptr::write_bytes(out_result, 0, 1) };
    }
}
