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

//! Rust crypto engine for GpgFrontend (rPGP backend).
//!
//! Exposes a C-compatible FFI surface consumed by the C++ core. All exported
//! symbols use `#[unsafe(no_mangle)]` and follow a consistent contract:
//! - Return `GfrStatus::ErrorInvalidInput` for null pointer or out-of-range arguments.
//! - Return `GfrStatus::ErrorPanic` if a Rust panic is caught via `catch_unwind`.
//! - Return `GfrStatus::Success` (0) on success.
//!
//! Heap memory transferred to the caller must be freed with the corresponding
//! `gfr_crypto_free_*` function exported from `ffi_mem`.
//!
//! # Module layout
//! - `ffi` — basic runtime entry points (version, logger init)
//! - `ffi_crypto` — message and file encrypt/decrypt/sign/verify operations
//! - `ffi_key` — key block manipulation (metadata, password, subkey, revocation)
//! - `ffi_keygen` — key and subkey generation
//! - `ffi_mem` — memory deallocation helpers for FFI-owned pointers
//! - `ffi_user_id` — user ID add/delete/update/revoke/set-primary operations
//! - `types` — `#[repr(C)]` types shared across the FFI boundary

pub mod ffi;
pub mod ffi_crypto;
pub mod ffi_key;
pub mod ffi_keygen;
pub mod ffi_mem;
pub mod ffi_user_id;
pub mod types;

mod crypto;
mod crypto_stream;
mod err;
mod key;
mod keygen;
mod user_id;
mod utils;

mod cache;
mod tar;
