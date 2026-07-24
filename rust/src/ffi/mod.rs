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

/// Return build details of the Rust engine as a heap-allocated C string.
///
/// The payload is a newline-delimited list of `key\tvalue` records describing
/// the engine version, the compiler/target/profile it was built with, and the
/// resolved versions of key dependencies (each prefixed with `dep:`). Values
/// captured at build time (see `build.rs`) may be empty if unavailable.
///
/// The caller is responsible for freeing the returned pointer with
/// `gfr_crypto_free_string`.
#[unsafe(no_mangle)]
pub extern "C" fn gfr_rust_engine_build_info() -> *mut c_char {
    let mut records: Vec<(String, String)> = vec![
        ("engine".to_string(), env!("CARGO_PKG_VERSION").to_string()),
        (
            "rustc".to_string(),
            option_env!("GFR_RUSTC_VERSION")
                .unwrap_or_default()
                .to_string(),
        ),
        (
            "target".to_string(),
            option_env!("GFR_TARGET").unwrap_or_default().to_string(),
        ),
        (
            "profile".to_string(),
            option_env!("GFR_PROFILE").unwrap_or_default().to_string(),
        ),
    ];

    // Keep this list and order aligned with `TRACKED_DEPS` in `build.rs`.
    let deps: &[(&str, Option<&str>)] = &[
        ("pgp", option_env!("GFR_DEP_PGP")),
        ("rsa", option_env!("GFR_DEP_RSA")),
        ("rand", option_env!("GFR_DEP_RAND")),
        ("zeroize", option_env!("GFR_DEP_ZEROIZE")),
        ("anyhow", option_env!("GFR_DEP_ANYHOW")),
        ("tar", option_env!("GFR_DEP_TAR")),
        ("env_logger", option_env!("GFR_DEP_ENV_LOGGER")),
        ("log", option_env!("GFR_DEP_LOG")),
        ("once_cell", option_env!("GFR_DEP_ONCE_CELL")),
        ("tempfile", option_env!("GFR_DEP_TEMPFILE")),
    ];
    for (name, version) in deps {
        let version = version.unwrap_or_default();
        if !version.is_empty() {
            records.push((format!("dep:{name}"), version.to_string()));
        }
    }

    let body = records
        .iter()
        .map(|(k, v)| format!("{k}\t{v}"))
        .collect::<Vec<_>>()
        .join("\n");

    CString::new(body)
        .unwrap_or_else(|_| CString::new("").unwrap())
        .into_raw()
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

/// Drop every cached passphrase, for every channel and key.
///
/// Used when the keyring underneath the cache is replaced wholesale — otherwise
/// an entry cached for a fingerprint keeps serving a passphrase that no longer
/// unlocks the key now carrying that fingerprint.
#[unsafe(no_mangle)]
pub extern "C" fn gfr_clear_password_cache() {
    crate::cache::PASSWORD_CACHE.clear();
}

/// Initialise the `env_logger` backend writing to stdout.
///
/// Defaults to INFO, but honours the `RUST_LOG` environment variable so the
/// C++ side can align the Rust log level with the app's `--log-level` flag
/// (see `ParseLogLevel`, which exports `RUST_LOG`). An explicit `RUST_LOG`
/// set by the user always wins.
///
/// Safe to call multiple times; subsequent calls are no-ops if the logger
/// was already initialized.
#[unsafe(no_mangle)]
pub extern "C" fn gfr_init_logger() {
    let _ = env_logger::builder()
        .target(env_logger::Target::Stdout)
        .filter_level(LevelFilter::Info)
        .parse_default_env()
        .try_init();
}

#[cfg(test)]
mod ffi_mod_tests {
    //! The runtime entry points: version reporting, build info, cancellation
    //! and password-cache configuration.

    use super::*;
    use crate::types::GfrStatus;
    use std::ffi::CStr;

    fn take_string(ptr: *mut c_char) -> String {
        assert!(!ptr.is_null(), "the entry point must return a string");
        let s = unsafe { CStr::from_ptr(ptr) }.to_string_lossy().into_owned();
        crate::ffi::mem::gfr_crypto_free_string(ptr);
        s
    }

    #[test]
    fn hello_does_not_panic() {
        // It only logs, but it is the first Rust symbol the app calls at
        // startup, so a panic here would be a hard failure to launch.
        gfr_rust_hello();
    }

    #[test]
    fn the_engine_version_matches_the_package_version() {
        assert_eq!(take_string(gfr_rust_engine_version()), env!("CARGO_PKG_VERSION"));
    }

    #[test]
    fn the_engine_version_is_a_fresh_allocation_each_call() {
        // Each caller frees what it got, so returning a shared pointer would
        // be a double free waiting to happen.
        let a = gfr_rust_engine_version();
        let b = gfr_rust_engine_version();
        assert_ne!(a, b);
        crate::ffi::mem::gfr_crypto_free_string(a);
        crate::ffi::mem::gfr_crypto_free_string(b);
    }

    #[test]
    fn the_build_info_is_tab_delimited_records() {
        let info = take_string(gfr_rust_engine_build_info());
        assert!(!info.is_empty());
        for line in info.lines().filter(|l| !l.is_empty()) {
            assert!(line.contains('\t'), "not a key\\tvalue record: {line:?}");
        }
    }

    #[test]
    fn the_build_info_reports_the_engine_version() {
        let info = take_string(gfr_rust_engine_build_info());
        assert!(
            info.lines()
                .any(|l| l.starts_with("engine\t") && l.contains(env!("CARGO_PKG_VERSION")))
        );
    }

    #[test]
    fn the_build_info_reports_the_pgp_dependency() {
        // The rPGP version is the single most useful thing in a bug report
        // about this engine.
        let info = take_string(gfr_rust_engine_build_info());
        assert!(
            info.lines().any(|l| l.starts_with("dep:pgp\t")),
            "build info should record the pgp crate version:\n{info}"
        );
    }

    #[test]
    fn setting_the_cancel_flag_is_visible_to_the_engine() {
        const CH: i32 = 0x0F_01;
        gfr_set_operation_cancelled(CH, true);
        assert!(crate::cancel::is_cancelled(CH));
        gfr_set_operation_cancelled(CH, false);
        assert!(!crate::cancel::is_cancelled(CH));
    }

    #[test]
    fn the_cancel_flag_is_per_channel() {
        const A: i32 = 0x0F_02;
        const B: i32 = 0x0F_03;
        gfr_set_operation_cancelled(A, true);
        assert!(!crate::cancel::is_cancelled(B));
        gfr_set_operation_cancelled(A, false);
    }

    #[test]
    fn setting_the_cancel_flag_twice_is_idempotent() {
        const CH: i32 = 0x0F_04;
        gfr_set_operation_cancelled(CH, true);
        gfr_set_operation_cancelled(CH, true);
        assert!(crate::cancel::is_cancelled(CH));
        gfr_set_operation_cancelled(CH, false);
    }

    #[test]
    fn initialising_the_logger_more_than_once_is_safe() {
        // The C++ side may call this from more than one place; a second
        // attempt must not panic on "logger already set".
        gfr_init_logger();
        gfr_init_logger();
    }

    #[test]
    fn the_password_cache_ttl_can_be_reconfigured() {
        // Restore gpg-agent-like defaults afterwards so this does not disturb
        // other tests sharing the global cache.
        gfr_set_password_cache_ttl(120, 1200);
        gfr_set_password_cache_ttl(600, 7200);
    }

    #[test]
    fn a_cache_ttl_longer_than_its_cap_is_clamped_not_rejected() {
        gfr_set_password_cache_ttl(900, 30);
        gfr_set_password_cache_ttl(600, 7200);
    }

    #[test]
    fn clearing_the_password_cache_does_not_panic() {
        gfr_clear_password_cache();
    }

    #[test]
    fn the_last_error_slot_is_reachable_through_the_ffi() {
        crate::err::set_last_error("an ffi-visible failure");
        let msg = take_string(crate::err::gfr_get_last_error_msg());
        assert_eq!(msg, "an ffi-visible failure");
    }

    #[test]
    fn the_last_error_slot_is_null_when_empty() {
        crate::err::clear_last_error();
        assert!(crate::err::gfr_get_last_error_msg().is_null());
    }

    #[test]
    fn every_runtime_entry_point_is_callable_without_arguments_it_can_reject() {
        // A smoke sweep: none of the no-argument entry points may panic, in
        // any order, however many times they are called.
        for _ in 0..3 {
            gfr_rust_hello();
            gfr_init_logger();
            gfr_clear_password_cache();
            let _ = take_string(gfr_rust_engine_version());
            let _ = take_string(gfr_rust_engine_build_info());
        }
        assert_eq!(GfrStatus::Success as i32, 0);
    }
}
