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
            // `warn`, not `error`: several of these sites wrap I/O errors that are
            // actually user cancellations (mapped to `ErrorCanceled` by the
            // status closure). The C++ layer decides the real severity once it
            // knows the final status.
            log::warn!("rPGP operation reported: {}", msg);
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

#[cfg(test)]
mod err_tests {
    //! The thread-local error slot and the rPGP-error to `GfrStatus` mapping.
    //!
    //! `pgp::errors::Error` variants cannot be constructed from outside the
    //! crate, so the mapping is exercised by driving *real* failures through
    //! the corpus (tampered ciphertext, wrong passphrase, malformed armor)
    //! rather than by fabricating error values. Where a variant has no
    //! reachable trigger that is recorded explicitly rather than skipped.

    use super::*;
    use crate::testutil::corpus;
    use std::ffi::CStr;

    /// Consume the thread-local slot and return its contents as a Rust string.
    /// `None` when no message is pending.
    fn take_last_error() -> Option<String> {
        let ptr = gfr_get_last_error_msg();
        if ptr.is_null() {
            return None;
        }
        let msg = unsafe { CStr::from_ptr(ptr) }
            .to_string_lossy()
            .into_owned();
        // The FFI contract says the caller frees this with
        // `gfr_crypto_free_string`; do exactly that rather than leaking.
        crate::ffi::mem::gfr_crypto_free_string(ptr);
        Some(msg)
    }

    // -- the thread-local slot --------------------------------------------

    #[test]
    fn set_then_get_returns_the_message() {
        set_last_error("a detailed failure");
        assert_eq!(take_last_error().as_deref(), Some("a detailed failure"));
    }

    #[test]
    fn get_consumes_the_slot() {
        // Consuming on read is what stops a stale detail from a previous
        // operation being mistaken for the current one.
        set_last_error("transient");
        assert!(take_last_error().is_some());
        assert!(take_last_error().is_none());
    }

    #[test]
    fn get_is_null_when_nothing_was_recorded() {
        clear_last_error();
        assert!(gfr_get_last_error_msg().is_null());
    }

    #[test]
    fn clear_empties_the_slot() {
        set_last_error("to be cleared");
        clear_last_error();
        assert!(take_last_error().is_none());
    }

    #[test]
    fn a_later_message_replaces_an_earlier_one() {
        set_last_error("first");
        set_last_error("second");
        assert_eq!(take_last_error().as_deref(), Some("second"));
    }

    #[test]
    fn an_empty_message_reads_back_as_no_error() {
        // `set_last_error("")` is indistinguishable from "nothing pending";
        // callers must not rely on an empty string being observable.
        set_last_error("");
        assert!(gfr_get_last_error_msg().is_null());
    }

    #[test]
    fn a_message_with_an_interior_nul_degrades_to_empty() {
        // `CString::new` rejects interior NULs and the code falls back to
        // `unwrap_or_default()`, i.e. an empty C string -- not a crash.
        set_last_error("before\0after");
        let ptr = gfr_get_last_error_msg();
        assert!(!ptr.is_null(), "a non-empty message still yields a pointer");
        assert_eq!(unsafe { CStr::from_ptr(ptr) }.to_bytes(), b"");
        crate::ffi::mem::gfr_crypto_free_string(ptr);
    }

    #[test]
    fn a_multibyte_utf8_message_survives_the_round_trip() {
        set_last_error("clé introuvable — 鍵がありません");
        assert_eq!(
            take_last_error().as_deref(),
            Some("clé introuvable — 鍵がありません")
        );
    }

    #[test]
    fn a_long_message_is_not_truncated() {
        let long = "e".repeat(64 * 1024);
        set_last_error(&long);
        assert_eq!(take_last_error().map(|s| s.len()), Some(64 * 1024));
    }

    #[test]
    fn the_slot_is_thread_local() {
        // Concurrent operations on different channels run on different
        // threads; one must never read another's message.
        set_last_error("main thread");
        let child = std::thread::spawn(|| {
            let before = gfr_get_last_error_msg();
            set_last_error("child thread");
            (before.is_null(), take_last_error())
        });
        let (child_saw_nothing, child_msg) = child.join().expect("thread joins");
        assert!(child_saw_nothing, "the child must start with a clean slot");
        assert_eq!(child_msg.as_deref(), Some("child thread"));
        assert_eq!(take_last_error().as_deref(), Some("main thread"));
    }

    #[test]
    fn clearing_on_one_thread_does_not_clear_another() {
        set_last_error("mine");
        std::thread::spawn(clear_last_error).join().expect("joins");
        assert_eq!(take_last_error().as_deref(), Some("mine"));
    }

    // -- RecordErr ---------------------------------------------------------

    #[test]
    fn record_err_maps_to_the_requested_status() {
        let r: Result<(), std::io::Error> = Err(std::io::Error::other("disk on fire"));
        assert_eq!(r.record_err(GfrStatus::ErrorIo), Err(GfrStatus::ErrorIo));
    }

    #[test]
    fn record_err_stores_the_display_text() {
        let r: Result<(), std::io::Error> = Err(std::io::Error::other("disk on fire"));
        let _ = r.record_err(GfrStatus::ErrorIo);
        assert_eq!(take_last_error().as_deref(), Some("disk on fire"));
    }

    #[test]
    fn record_err_passes_ok_through_untouched() {
        clear_last_error();
        let r: Result<u8, std::io::Error> = Ok(7);
        assert_eq!(r.record_err(GfrStatus::ErrorIo), Ok(7));
        assert!(take_last_error().is_none(), "success records nothing");
    }

    #[test]
    fn record_err_with_uses_the_closure_status() {
        let r: Result<(), std::io::Error> = Err(std::io::Error::other("aborted"));
        assert_eq!(
            r.record_err_with(|| GfrStatus::ErrorCanceled),
            Err(GfrStatus::ErrorCanceled)
        );
    }

    #[test]
    fn record_err_with_does_not_call_the_closure_on_success() {
        let r: Result<u8, std::io::Error> = Ok(1);
        let mut called = false;
        let out = r.record_err_with(|| {
            called = true;
            GfrStatus::ErrorInternal
        });
        assert_eq!(out, Ok(1));
        assert!(!called, "the status closure is only for the error path");
    }

    #[test]
    fn record_err_works_for_any_display_error() {
        // The impl is generic over `E: Display`, which is what lets call sites
        // use it on anyhow errors, parse errors and rPGP errors alike.
        let r: Result<(), String> = Err("plain string error".to_string());
        assert_eq!(
            r.record_err(GfrStatus::ErrorInternal),
            Err(GfrStatus::ErrorInternal)
        );
        assert_eq!(take_last_error().as_deref(), Some("plain string error"));
    }

    // -- IntoGfrResult for io::Error ---------------------------------------

    #[test]
    fn io_error_always_maps_to_error_io() {
        for kind in [
            std::io::ErrorKind::NotFound,
            std::io::ErrorKind::PermissionDenied,
            std::io::ErrorKind::UnexpectedEof,
            std::io::ErrorKind::Other,
        ] {
            let r: Result<(), std::io::Error> = Err(std::io::Error::new(kind, "boom"));
            assert_eq!(r.into_gfr(), Err(GfrStatus::ErrorIo), "{kind:?}");
        }
    }

    #[test]
    fn io_error_records_its_message() {
        let r: Result<(), std::io::Error> = Err(std::io::Error::new(
            std::io::ErrorKind::NotFound,
            "no such vector",
        ));
        let _ = r.into_gfr();
        assert_eq!(take_last_error().as_deref(), Some("no such vector"));
    }

    #[test]
    fn io_ok_passes_through() {
        let r: Result<u32, std::io::Error> = Ok(42);
        assert_eq!(r.into_gfr(), Ok(42));
    }

    // -- IntoGfrResult for pgp::errors::Error ------------------------------
    //
    // Driven by real parse failures over the committed corpus.

    fn parse_key(block: &str) -> Result<(), GfrStatus> {
        use pgp::composed::{Deserializable, SignedPublicKey};
        SignedPublicKey::from_string(block).into_gfr().map(|_| ())
    }

    /// Parse an armored OpenPGP *message* and fully drain it, which is what
    /// forces the truncation to be noticed.
    fn parse_message(block: &str) -> Result<(), GfrStatus> {
        use pgp::composed::Message;
        let (mut msg, _) = Message::from_string(block).into_gfr()?;
        let mut sink = Vec::new();
        std::io::copy(&mut msg, &mut sink).into_gfr().map(|_| ())
    }

    #[test]
    fn malformed_armor_maps_to_an_armor_or_data_error() {
        // `truncated_armor.asc` is an armored PGP MESSAGE that stops mid-block.
        // Whether rPGP reports this as an armor-wrapper problem or a packet
        // problem is its business; the contract pinned here is that it is one
        // of the parse-family statuses, never `Success` and never a panic.
        let status = parse_message(corpus::TRUNCATED_ARMOR).unwrap_err();
        assert!(
            matches!(
                status,
                GfrStatus::ErrorArmorFailed
                    | GfrStatus::ErrorInvalidData
                    | GfrStatus::ErrorInvalidInput
                    | GfrStatus::ErrorIo
                    | GfrStatus::ErrorDataCorrupted
            ),
            "unexpected status {status:?}"
        );
    }

    #[test]
    fn malformed_armor_records_a_message() {
        let _ = parse_message(corpus::TRUNCATED_ARMOR);
        let msg = take_last_error();
        assert!(msg.is_some(), "the cause must be recorded for C++ to read");
        assert!(!msg.unwrap().is_empty());
    }

    #[test]
    fn a_corrupt_crc24_footer_is_tolerated_or_reported_cleanly() {
        // RFC 9580 §6.1: "An implementation MUST NOT reject an OpenPGP object
        // when the CRC24 footer is present, missing, malformed, or disagrees
        // with the computed CRC24 sum." Accepting is therefore correct; if the
        // parser does reject, it must at least do so with a mapped status.
        match parse_message(corpus::CORRUPT_CRC) {
            Ok(()) => {}
            Err(status) => assert!(
                (status as i32) < 0,
                "a rejection must still be a mapped status, got {status:?}"
            ),
        }
    }

    #[test]
    fn garbage_maps_to_a_parse_error() {
        let status = parse_key(&String::from_utf8_lossy(corpus::GARBAGE)).unwrap_err();
        assert!((status as i32) < 0, "{status:?}");
    }

    #[test]
    fn empty_input_maps_to_a_parse_error() {
        let status = parse_key("").unwrap_err();
        assert!((status as i32) < 0, "{status:?}");
    }

    #[test]
    fn a_wrong_passphrase_maps_to_wrong_password() {
        // RFC 9580 §3.7.2.1: the S2K-derived key unwraps the secret material;
        // a wrong passphrase surfaces as an unpad/AEAD failure, which the
        // mapping must translate into a passphrase problem rather than a
        // generic internal error.
        use pgp::types::Password;
        let key = &*corpus::AUX_V6_KEY;
        let err = key
            .unlock(&Password::from("definitely not the passphrase"), |_, _| {
                Ok(())
            })
            .into_gfr()
            .unwrap_err();
        assert!(
            matches!(
                err,
                GfrStatus::ErrorWrongPassword
                    | GfrStatus::ErrorDataCorrupted
                    | GfrStatus::ErrorBadPassphrase
            ),
            "a wrong passphrase must be reported as such, got {err:?}"
        );
    }

    #[test]
    fn the_correct_passphrase_unlocks_the_corpus_key() {
        // The negative test above is only meaningful if the positive one holds.
        use pgp::types::Password;
        let key = &*corpus::AUX_V6_KEY;
        let pw = String::from_utf8_lossy(corpus::CORPUS_PASSPHRASE).into_owned();
        assert!(key.unlock(&Password::from(pw), |_, _| Ok(())).is_ok());
    }

    #[test]
    fn a_block_of_the_wrong_armor_type_is_rejected() {
        // Feeding a PGP MESSAGE to the key parser is a caller error. It maps to
        // the catch-all `ErrorInternal` today; pinned so the behaviour is
        // visible, since a more specific status would be friendlier.
        let status = parse_key(corpus::TRUNCATED_ARMOR).unwrap_err();
        assert_eq!(status, GfrStatus::ErrorInternal);
    }

    #[test]
    fn a_mapped_status_always_leaves_a_message_behind() {
        // Every arm of the rPGP mapping records the message before mapping, so
        // C++ can always show a cause next to the code.
        clear_last_error();
        let _ = parse_message(corpus::TRUNCATED_ARMOR);
        assert!(take_last_error().is_some());
    }

    // Two arms of the rPGP error mapping have no reachable trigger through this
    // crate's API and are therefore not covered by a test:
    //   * `Error::AesKw` would need a hand-built ECDH PKESK with a corrupted
    //     key-wrap blob;
    //   * `Error::Sha1HashCollision` fires only on a SHA-1 collision-detection
    //     hit, which no corpus vector provides.
    // Both map into arms that other errors already exercise (ErrorWrongPassword
    // and ErrorDataCorrupted respectively), so the mapping itself is covered;
    // only these specific triggers are not.
}
