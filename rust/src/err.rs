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

//! Thread-local last-error storage and `IntoGfrResult` conversion trait.
//!
//! Every FFI call that may fail stores a human-readable message in a
//! thread-local `LAST_ERROR` slot. C++ retrieves it via `gfr_get_last_error_msg`
//! after checking the returned `GfrStatus` code. The slot is independent per
//! OS thread, so concurrent operations on different channels do not clobber
//! each other's error messages.

use crate::types::GfrStatus;
use std::cell::RefCell;
use std::ffi::CString;
use std::os::raw::c_char;

thread_local! {
    static LAST_ERROR: RefCell<String> = const { RefCell::new(String::new()) };
}

/// Record a detailed error message for the current thread.
///
/// Called by internal code immediately before returning a `GfrStatus` error so
/// C++ can retrieve the full message with `gfr_get_last_error_msg`.
pub fn set_last_error(msg: &str) {
    LAST_ERROR.with(|e| {
        *e.borrow_mut() = msg.to_string();
    });
}

/// Clear the thread-local error slot before starting a new operation.
pub fn clear_last_error() {
    LAST_ERROR.with(|e| {
        e.borrow_mut().clear();
    });
}

/// Return the last error message for the current thread as a heap-allocated C string.
///
/// Returns null when no error is pending. Reading **consumes** the message: the
/// thread-local slot is cleared afterwards, so a stale detail from a previous
/// operation can never be mistaken for the current one even when a later failure
/// path returns a bare status without recording a message. The caller must free
/// the returned pointer with `gfr_crypto_free_string`.
#[unsafe(no_mangle)]
pub extern "C" fn gfr_get_last_error_msg() -> *mut c_char {
    LAST_ERROR.with(|e| {
        let msg = std::mem::take(&mut *e.borrow_mut());
        if msg.is_empty() {
            std::ptr::null_mut()
        } else {
            CString::new(msg).unwrap_or_default().into_raw()
        }
    })
}

/// Convert a foreign `Result` into a `Result<T, GfrStatus>`, logging and storing
/// the error message as a side effect.
pub trait IntoGfrResult<T> {
    fn into_gfr(self) -> Result<T, GfrStatus>;
}

/// Record an arbitrary error's message before collapsing it into a `GfrStatus`.
///
/// Use this instead of the lossy `.map_err(|_| GfrStatus::X)` pattern so the
/// specific cause (e.g. "checksum mismatch", "unsupported algorithm") survives
/// for C++ to retrieve via `gfr_get_last_error_msg`, the way GnuPG surfaces
/// `gpg_strerror` text. When the source error is a `pgp::errors::Error`, prefer
/// [`IntoGfrResult::into_gfr`], which also picks the most precise status itself.
pub trait RecordErr<T> {
    /// Record the error message, then map to the fixed `status`.
    fn record_err(self, status: GfrStatus) -> Result<T, GfrStatus>;

    /// Record the error message, then map to the status produced by `f`
    /// (e.g. `cancel::status_or_canceled(channel, ...)`).
    fn record_err_with(self, f: impl FnOnce() -> GfrStatus) -> Result<T, GfrStatus>;
}

impl<T, E: std::fmt::Display> RecordErr<T> for Result<T, E> {
    fn record_err(self, status: GfrStatus) -> Result<T, GfrStatus> {
        self.record_err_with(|| status)
    }

    fn record_err_with(self, f: impl FnOnce() -> GfrStatus) -> Result<T, GfrStatus> {
        self.map_err(|e| {
            let msg = e.to_string();
            log::error!("rPGP operation failed: {}", msg);
            set_last_error(&msg);
            f()
        })
    }
}

impl<T> IntoGfrResult<T> for Result<T, pgp::errors::Error> {
    fn into_gfr(self) -> Result<T, GfrStatus> {
        match self {
            Ok(val) => Ok(val),
            Err(err) => {
                // Note: cancellation is detected at the streaming call sites
                // (which know their channel) before calling `into_gfr`, since
                // this trait has no channel context to consult a per-channel
                // flag with.

                // 1. record the detailed error message for C++ retrieval
                let err_str = err.to_string();
                log::error!("rPGP Error: {}", err_str);
                set_last_error(&err_str);

                // 2. map specific rPGP errors to our GfrStatus codes
                use pgp::errors::Error::*;
                let status = match err {
                    UnpadError | BlockMode | Aead { .. } | AesKw { .. } | AesKek { .. } => {
                        GfrStatus::ErrorWrongPassword
                    }

                    // data integrity errors
                    MdcError
                    | InvalidChecksum
                    | ChecksumMissmatch { .. }
                    | Sha1HashCollision { .. } => GfrStatus::ErrorDataCorrupted,

                    MissingKey => GfrStatus::ErrorNoKey,

                    InvalidArmorWrappers | Base64Decode { .. } => GfrStatus::ErrorArmorFailed,
                    PacketError { .. }
                    | PacketParsing { .. }
                    | PacketIncomplete { .. }
                    | InvalidPacketContent { .. }
                    | NoMatchingPacket { .. }
                    | TooManyPackets => GfrStatus::ErrorInvalidData,

                    // unsupported xxx errors
                    Unsupported { .. } | Unimplemented { .. } => {
                        GfrStatus::ErrorUnsupportedAlgorithm
                    }

                    IO { .. } => GfrStatus::ErrorIo,
                    InvalidInput { .. } => GfrStatus::ErrorInvalidInput,

                    _ => GfrStatus::ErrorInternal,
                };

                Err(status)
            }
        }
    }
}

impl<T> IntoGfrResult<T> for Result<T, std::io::Error> {
    fn into_gfr(self) -> Result<T, GfrStatus> {
        match self {
            Ok(val) => Ok(val),
            Err(err) => {
                let err_str = err.to_string();
                log::error!("IO Error: {}", err_str);
                set_last_error(&err_str);
                Err(GfrStatus::ErrorIo)
            }
        }
    }
}
