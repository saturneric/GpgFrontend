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
    return ver_str.into_raw();
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
