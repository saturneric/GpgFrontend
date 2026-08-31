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

// FFI boundary functions intentionally dereference raw pointer arguments after
// null-checking them. The functions are `extern "C"` (callable from C++ without
// the `unsafe` keyword) and all unsafe operations are guarded by explicit
// `unsafe {}` blocks. Suppressing per-function is impractical at this scale.
#![allow(clippy::not_unsafe_ptr_arg_deref)]
// FFI boundary functions that map 1-to-1 to C API entry points necessarily
// have as many arguments as the C signature requires; grouping into structs
// would break the ABI contract with the C++ caller.
#![allow(clippy::too_many_arguments)]
// Some nested if-let patterns are clearer when left as two separate conditions
// rather than merged with `&&`, particularly when the bodies are large.
#![allow(clippy::collapsible_if)]

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
//! - `ffi::crypto` — message and file encrypt/decrypt/sign/verify operations
//! - `ffi::key` — key block manipulation (metadata, password, subkey, revocation)
//! - `ffi::keygen` — key and subkey generation
//! - `ffi::mem` — memory deallocation helpers for FFI-owned pointers
//! - `ffi::user_id` — user ID add/delete/update/revoke/set-primary operations
//! - `types` — `#[repr(C)]` types shared across the FFI boundary

pub mod host;

pub mod ffi;
pub mod types;

mod cancel;
mod crypto;
mod err;
mod key;
mod keygen;
mod user_id;
mod utils;

mod cache;
mod tar;

/// Shared unit-test scaffolding: the committed RFC 9580 corpus, the Appendix A
/// known-answer vectors, lazily generated key fixtures, packet builders and the
/// host-symbol stubs. Compiled only for `cargo test`; see `testutil.rs`.
#[cfg(test)]
mod testutil;

#[cfg(test)]
mod crate_smoke_tests {
    //! Crate-level wiring: the modules the FFI contract promises are present,
    //! and the exported symbols the C++ core links against exist with the
    //! signatures `GFCoreRust.h` declares.

    #[test]
    fn the_public_modules_are_reachable() {
        // `host` and `types` are `pub` because cbindgen walks them to generate
        // the header; `ffi` carries the entry points themselves.
        let _ = crate::types::GfrStatus::Success;
        let _ = crate::types::GfrBuffer::empty();
        let _: extern "C" fn() -> *mut std::os::raw::c_char = crate::ffi::gfr_rust_engine_version;
    }

    #[test]
    fn the_runtime_entry_points_have_the_declared_signatures() {
        // Taking each as a typed function pointer is a compile-time assertion
        // that the ABI has not drifted from the generated header.
        let _: extern "C" fn() = crate::ffi::gfr_rust_hello;
        let _: extern "C" fn() -> *mut std::os::raw::c_char =
            crate::ffi::gfr_rust_engine_build_info;
        let _: extern "C" fn(i32, bool) = crate::ffi::gfr_set_operation_cancelled;
        let _: extern "C" fn(u64, u64) = crate::ffi::gfr_set_password_cache_ttl;
        let _: extern "C" fn(u8, u8, u8) = crate::ffi::gfr_set_argon2_s2k_params;
        let _: extern "C" fn() = crate::ffi::gfr_clear_password_cache;
        let _: extern "C" fn() = crate::ffi::gfr_init_logger;
        let _: extern "C" fn() -> *mut std::os::raw::c_char = crate::err::gfr_get_last_error_msg;
    }

    #[test]
    fn every_heap_transferring_result_has_a_matching_free() {
        // The memory contract in this file's module docs: anything handed to
        // C++ is reclaimed by a `gfr_crypto_free_*` counterpart.
        use crate::ffi::mem::*;
        let _: extern "C" fn(*mut std::os::raw::c_char) = gfr_crypto_free_string;
        let _: extern "C" fn(*mut u8, usize) = gfr_crypto_free_buffer;
        let _: extern "C" fn(*mut crate::types::GfrKeyGenerateResult) =
            gfr_crypto_free_key_generate_result;
        let _: extern "C" fn(*mut crate::types::GfrEncryptResultC) = gfr_crypto_free_encrypt_result;
        let _: extern "C" fn(*mut crate::types::GfrDecryptResultC) = gfr_crypto_free_decrypt_result;
        let _: extern "C" fn(*mut crate::types::GfrSignResultC) = gfr_crypto_free_sign_result;
        let _: extern "C" fn(*mut crate::types::GfrVerifyResultC) = gfr_crypto_free_verify_result;
    }

    #[test]
    fn the_engine_version_matches_the_package_version() {
        // `gfr_rust_engine_version` is what the About dialog shows; it must
        // track Cargo.toml rather than drifting into a hand-maintained string.
        use std::ffi::CStr;
        let ptr = crate::ffi::gfr_rust_engine_version();
        assert!(!ptr.is_null());
        let version = unsafe { CStr::from_ptr(ptr) }
            .to_string_lossy()
            .into_owned();
        crate::ffi::mem::gfr_crypto_free_string(ptr);
        assert_eq!(version, env!("CARGO_PKG_VERSION"));
    }
}
