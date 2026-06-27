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

//! Basic runtime entry points: version query and logger initialisation.

use std::ffi::{CString, c_char};

use log::LevelFilter;

pub mod crypto;
pub mod key;
pub mod keygen;
pub mod mem;
pub mod user_id;

/// Log a greeting that includes the Rust engine version. Used to confirm the
/// Rust library was loaded successfully at application startup.
#[unsafe(no_mangle)]
pub extern "C" fn gfr_rust_hello() {
    log::info!(
        "Hello from Rust! (Rust Engine version {})",
        env!("CARGO_PKG_VERSION")
    );
}

/// Return the Rust engine version string as a heap-allocated C string.
///
/// The caller is responsible for freeing the returned pointer with
/// `gfr_crypto_free_string`.
#[unsafe(no_mangle)]
pub extern "C" fn gfr_rust_engine_version() -> *mut c_char {
    let ver_str =
        CString::new(env!("CARGO_PKG_VERSION")).unwrap_or_else(|_| CString::new("0.0.0").unwrap());
    ver_str.into_raw()
}

/// Request or clear cancellation of the crypto operation on `channel`.
///
/// Pass `true` to abort the in-flight streaming operation on that channel as
/// soon as it reads its next chunk; pass `false` to reset the channel's flag
/// before starting a new operation. Each channel has its own flag, so
/// cancelling one channel never affects another. The aborted operation
/// surfaces as `GfrStatus::ErrorCanceled`.
#[unsafe(no_mangle)]
pub extern "C" fn gfr_set_operation_cancelled(channel: i32, cancelled: bool) {
    crate::cancel::set_cancelled(channel, cancelled);
}

/// Configure the passphrase cache timeouts, in seconds.
///
/// `ttl_secs` is the sliding idle window, renewed on every cache hit;
/// `max_ttl_secs` is the absolute cap measured from first entry, which the
/// sliding window can never extend past. `max_ttl_secs` is clamped up to at
/// least `ttl_secs`.
///
/// A `ttl_secs` of 0 leaves the built-in defaults untouched, to avoid
/// accidentally disabling the cache via an unset setting; pass `max_ttl_secs`
/// 0 to mean "no cap beyond the sliding window".
#[unsafe(no_mangle)]
pub extern "C" fn gfr_set_password_cache_ttl(ttl_secs: u64, max_ttl_secs: u64) {
    if ttl_secs == 0 {
        return;
    }
    let max_secs = if max_ttl_secs == 0 {
        ttl_secs
    } else {
        max_ttl_secs
    };
    crate::cache::PASSWORD_CACHE.set_ttl(
        std::time::Duration::from_secs(ttl_secs),
        std::time::Duration::from_secs(max_secs),
    );
}

/// Initialise the `env_logger` backend at INFO level, writing to stdout.
///
/// Safe to call multiple times; subsequent calls are no-ops if the logger
/// was already initialized.
#[unsafe(no_mangle)]
pub extern "C" fn gfr_init_logger() {
    let _ = env_logger::builder()
        .target(env_logger::Target::Stdout)
        .filter_level(LevelFilter::Info)
        .try_init();
}
