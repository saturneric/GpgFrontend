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

use crate::types::GfrStatus;
use std::cell::RefCell;
use std::ffi::CString;
use std::os::raw::c_char;

thread_local! {
    static LAST_ERROR: RefCell<String> = const { RefCell::new(String::new()) };
}

// Helper to set the last error message from within Rust
pub fn set_last_error(msg: &str) {
    LAST_ERROR.with(|e| {
        *e.borrow_mut() = msg.to_string();
    });
}

// Helper to clear the last error before starting a new operation
pub fn clear_last_error() {
    LAST_ERROR.with(|e| {
        e.borrow_mut().clear();
    });
}

// FFI function for C++ to retrieve the detailed error message
#[unsafe(no_mangle)]
pub extern "C" fn gfr_get_last_error_msg() -> *mut c_char {
    LAST_ERROR.with(|e| {
        let msg = e.borrow();
        if msg.is_empty() {
            std::ptr::null_mut()
        } else {
            CString::new(msg.clone()).unwrap_or_default().into_raw()
        }
    })
}

// Trait to convert external results into our FFI status
pub trait IntoGfrResult<T> {
    fn into_gfr(self) -> Result<T, GfrStatus>;
}

// Implementation for rpgp's Result type
impl<T> IntoGfrResult<T> for Result<T, pgp::errors::Error> {
    fn into_gfr(self) -> Result<T, GfrStatus> {
        match self {
            Ok(val) => Ok(val),
            Err(err) => {
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

// Implementation for standard IO errors
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
