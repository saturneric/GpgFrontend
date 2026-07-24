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

//! Cooperative cancellation for streaming crypto operations.
//!
//! rPGP has no native interrupt, but every operation funnels through a
//! streaming `Read`/`Write` pipeline that processes data in chunks. By wrapping
//! the input stream with [`CancellableReader`], each chunk read consults a
//! per-channel flag and aborts with an [`std::io::Error`] once cancellation is
//! requested. The error surfaces through `IntoGfrResult` as
//! [`crate::types::GfrStatus::ErrorCanceled`].
//!
//! Cancellation is keyed by OpenPGP context channel: each channel has its own
//! flag, so cancelling an operation on one channel never disturbs an operation
//! running on another. The C++ side flips a channel's flag via the
//! `gfr_set_operation_cancelled` FFI entry point: `true` on a user cancel
//! request, `false` before each new operation begins on that channel.

use std::collections::HashMap;
use std::io::{self, Read};
use std::sync::{LazyLock, RwLock};

/// Per-channel cancellation flags. A channel is present-and-`true` only while a
/// cancel is pending for it; entries are removed on reset to keep the map small.
static CANCEL_FLAGS: LazyLock<RwLock<HashMap<i32, bool>>> =
    LazyLock::new(|| RwLock::new(HashMap::new()));

/// Request or clear cancellation of the streaming operation on `channel`.
pub fn set_cancelled(channel: i32, cancelled: bool) {
    // Recover from a poisoned lock rather than propagating a panic across the
    // FFI boundary; the critical section only touches the map.
    let mut map = CANCEL_FLAGS.write().unwrap_or_else(|e| e.into_inner());
    if cancelled {
        map.insert(channel, true);
    } else {
        map.remove(&channel);
    }
}

/// Return whether cancellation has been requested for `channel`.
pub fn is_cancelled(channel: i32) -> bool {
    CANCEL_FLAGS
        .read()
        .map(|m| m.get(&channel).copied().unwrap_or(false))
        .unwrap_or(false)
}

/// Pick the status to report for a failed operation on `channel`:
/// [`ErrorCanceled`] when a cancel is in effect for it, otherwise the supplied
/// fallback. Use at sites that map stream errors directly (bypassing
/// `IntoGfrResult`) so a user cancel is not reported as a generic failure.
///
/// [`ErrorCanceled`]: crate::types::GfrStatus::ErrorCanceled
pub fn status_or_canceled(
    channel: i32,
    fallback: crate::types::GfrStatus,
) -> crate::types::GfrStatus {
    if is_cancelled(channel) {
        crate::types::GfrStatus::ErrorCanceled
    } else {
        fallback
    }
}

/// A `Read` adapter that aborts when cancellation is requested.
///
/// Each `read` first checks [`is_cancelled`]; if set, it returns an
/// `io::Error` of kind `Other` instead of forwarding to the inner
/// reader. Because rPGP pulls plaintext/ciphertext through the reader in
/// bounded chunks, this provides frequent cancellation checkpoints whose
/// granularity scales with the data size — exactly the long-running case the
/// user wants to be able to abort.
///
/// The error kind is deliberately `Other`, not `Interrupted`: the standard
/// `Read::read_to_end`/`read_exact` adapters (used by the buffered signing
/// modes) silently *retry* on `Interrupted`, which would spin forever once
/// cancellation latches. `Other` is propagated by every consumer instead.
pub struct CancellableReader<R> {
    channel: i32,
    inner: R,
}

impl<R: Read> CancellableReader<R> {
    pub fn new(channel: i32, inner: R) -> Self {
        Self { channel, inner }
    }
}

impl<R: Read> Read for CancellableReader<R> {
    fn read(&mut self, buf: &mut [u8]) -> io::Result<usize> {
        if is_cancelled(self.channel) {
            return Err(io::Error::other("operation cancelled by user"));
        }
        self.inner.read(buf)
    }
}

// rPGP's streaming decrypt requires the source reader to be `Debug`; forward a
// minimal representation without requiring the inner reader to be `Debug`.
impl<R> std::fmt::Debug for CancellableReader<R> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.write_str("CancellableReader")
    }
}

#[cfg(test)]
mod cancel_tests {
    //! Per-channel cooperative cancellation.
    //!
    //! `CANCEL_FLAGS` is process-global and cargo runs tests in parallel, so
    //! every test here uses its own channel id. The ids are allocated from a
    //! private block that no other test module touches.

    use super::*;
    use crate::types::GfrStatus;
    use std::io::Cursor;

    /// Channel ids reserved for this module, one per test.
    const CH_BASE: i32 = 0x0C_00;

    // -- flag lifecycle ----------------------------------------------------

    #[test]
    fn setting_the_flag_makes_the_channel_cancelled() {
        let ch = CH_BASE + 1;
        assert!(!is_cancelled(ch));
        set_cancelled(ch, true);
        assert!(is_cancelled(ch));
        set_cancelled(ch, false);
    }

    #[test]
    fn resetting_the_flag_clears_it() {
        let ch = CH_BASE + 2;
        set_cancelled(ch, true);
        set_cancelled(ch, false);
        assert!(!is_cancelled(ch));
    }

    #[test]
    fn an_untouched_channel_is_not_cancelled() {
        // The map only holds pending cancels, so a channel that has never been
        // seen must read as "not cancelled" rather than panicking on a miss.
        assert!(!is_cancelled(CH_BASE + 3));
    }

    #[test]
    fn channels_are_independent() {
        // The whole reason the flag is keyed by channel: cancelling one
        // context must not abort an operation running on another.
        let (a, b) = (CH_BASE + 4, CH_BASE + 5);
        set_cancelled(a, true);
        assert!(is_cancelled(a));
        assert!(!is_cancelled(b));
        set_cancelled(a, false);
    }

    #[test]
    fn setting_twice_is_idempotent() {
        let ch = CH_BASE + 6;
        set_cancelled(ch, true);
        set_cancelled(ch, true);
        assert!(is_cancelled(ch));
        set_cancelled(ch, false);
        assert!(!is_cancelled(ch));
    }

    #[test]
    fn resetting_a_channel_that_was_never_set_is_harmless() {
        let ch = CH_BASE + 7;
        set_cancelled(ch, false);
        assert!(!is_cancelled(ch));
    }

    #[test]
    fn negative_channel_ids_work() {
        // Channel ids come from the C++ side as a plain i32; nothing forbids a
        // negative value, and the map must not treat it specially.
        let ch = -(CH_BASE + 8);
        set_cancelled(ch, true);
        assert!(is_cancelled(ch));
        set_cancelled(ch, false);
    }

    #[test]
    fn extreme_channel_ids_work() {
        for ch in [i32::MIN, i32::MAX] {
            set_cancelled(ch, true);
            assert!(is_cancelled(ch), "{ch}");
            set_cancelled(ch, false);
            assert!(!is_cancelled(ch), "{ch}");
        }
    }

    #[test]
    fn resetting_removes_the_map_entry_rather_than_storing_false() {
        // An implementation detail worth pinning: the map is meant to stay
        // small over a long session, so a reset erases the entry instead of
        // accumulating one `false` per channel ever used.
        let ch = CH_BASE + 9;
        set_cancelled(ch, true);
        set_cancelled(ch, false);
        let map = CANCEL_FLAGS.read().expect("lock");
        assert!(!map.contains_key(&ch));
    }

    // -- status_or_canceled -------------------------------------------------

    #[test]
    fn status_or_canceled_returns_the_fallback_when_not_cancelled() {
        let ch = CH_BASE + 10;
        assert_eq!(
            status_or_canceled(ch, GfrStatus::ErrorIo),
            GfrStatus::ErrorIo
        );
    }

    #[test]
    fn status_or_canceled_returns_canceled_when_cancelled() {
        // Without this, a user pressing Cancel would see "General error".
        let ch = CH_BASE + 11;
        set_cancelled(ch, true);
        assert_eq!(
            status_or_canceled(ch, GfrStatus::ErrorIo),
            GfrStatus::ErrorCanceled
        );
        set_cancelled(ch, false);
    }

    #[test]
    fn status_or_canceled_preserves_any_fallback_status() {
        let ch = CH_BASE + 12;
        for fallback in [
            GfrStatus::ErrorIo,
            GfrStatus::ErrorInvalidData,
            GfrStatus::ErrorNoKey,
            GfrStatus::Success,
        ] {
            assert_eq!(status_or_canceled(ch, fallback), fallback);
        }
    }

    // -- CancellableReader --------------------------------------------------

    #[test]
    fn reader_passes_data_through_when_not_cancelled() {
        let ch = CH_BASE + 20;
        let data = b"the quick brown fox";
        let mut r = CancellableReader::new(ch, Cursor::new(data));
        let mut out = Vec::new();
        std::io::copy(&mut r, &mut out).expect("copy");
        assert_eq!(out, data);
    }

    #[test]
    fn reader_aborts_once_the_flag_is_set() {
        let ch = CH_BASE + 21;
        set_cancelled(ch, true);
        let mut r = CancellableReader::new(ch, Cursor::new(vec![0u8; 1024]));
        let mut buf = [0u8; 16];
        let err = r.read(&mut buf).expect_err("must abort");
        set_cancelled(ch, false);
        assert_eq!(err.kind(), io::ErrorKind::Other);
    }

    #[test]
    fn reader_error_kind_is_other_not_interrupted() {
        // Load-bearing: `Read::read_to_end` and `read_exact` silently *retry*
        // on `Interrupted`, so an Interrupted error here would make a cancelled
        // buffered operation spin forever instead of aborting.
        let ch = CH_BASE + 22;
        set_cancelled(ch, true);
        let mut r = CancellableReader::new(ch, Cursor::new(vec![0u8; 8]));
        let mut buf = [0u8; 4];
        let err = r.read(&mut buf).expect_err("must abort");
        set_cancelled(ch, false);
        assert_ne!(
            err.kind(),
            io::ErrorKind::Interrupted,
            "Interrupted would be retried by the std adapters, hanging the operation"
        );
    }

    #[test]
    fn read_to_end_terminates_on_cancel() {
        // The concrete manifestation of the previous test: this call must
        // return an error rather than loop.
        let ch = CH_BASE + 23;
        set_cancelled(ch, true);
        let mut r = CancellableReader::new(ch, Cursor::new(vec![7u8; 4096]));
        let mut out = Vec::new();
        let res = r.read_to_end(&mut out);
        set_cancelled(ch, false);
        assert!(res.is_err(), "read_to_end must propagate the cancellation");
    }

    #[test]
    fn read_exact_propagates_cancel() {
        let ch = CH_BASE + 24;
        set_cancelled(ch, true);
        let mut r = CancellableReader::new(ch, Cursor::new(vec![1u8; 64]));
        let mut buf = [0u8; 32];
        let res = r.read_exact(&mut buf);
        set_cancelled(ch, false);
        assert!(res.is_err());
    }

    #[test]
    fn a_zero_length_read_still_checks_the_flag() {
        // A consumer that probes with an empty buffer must not be able to slip
        // past a pending cancellation.
        let ch = CH_BASE + 25;
        set_cancelled(ch, true);
        let mut r = CancellableReader::new(ch, Cursor::new(vec![0u8; 8]));
        let res = r.read(&mut []);
        set_cancelled(ch, false);
        assert!(res.is_err());
    }

    #[test]
    fn cancelling_mid_stream_truncates_the_output() {
        // Cancellation is cooperative and checked per chunk, so data already
        // read is kept and nothing further is produced.
        let ch = CH_BASE + 26;
        let mut r = CancellableReader::new(ch, Cursor::new(vec![9u8; 4096]));
        let mut first = [0u8; 16];
        assert_eq!(r.read(&mut first).expect("first chunk"), 16);

        set_cancelled(ch, true);
        let mut rest = Vec::new();
        let res = r.read_to_end(&mut rest);
        set_cancelled(ch, false);

        assert!(res.is_err());
        assert!(rest.is_empty(), "nothing may be produced after the cancel");
    }

    #[test]
    fn the_error_message_names_the_user() {
        let ch = CH_BASE + 27;
        set_cancelled(ch, true);
        let mut r = CancellableReader::new(ch, Cursor::new(vec![0u8; 4]));
        let err = r.read(&mut [0u8; 4]).expect_err("aborts");
        set_cancelled(ch, false);
        assert!(err.to_string().contains("cancel"), "{err}");
    }

    #[test]
    fn reader_debug_does_not_require_a_debug_inner() {
        // rPGP's streaming decrypt demands `Debug` on the source reader, but
        // the wrapped readers (files, cursors, FFI adapters) are not all
        // `Debug` -- hence the hand-written impl.
        struct NotDebug;
        impl Read for NotDebug {
            fn read(&mut self, _buf: &mut [u8]) -> io::Result<usize> {
                Ok(0)
            }
        }
        let r = CancellableReader::new(CH_BASE + 28, NotDebug);
        assert_eq!(format!("{r:?}"), "CancellableReader");
    }

    #[test]
    fn reader_forwards_short_reads_faithfully() {
        // A reader that returns fewer bytes than asked for must not be treated
        // as EOF by the wrapper.
        struct OneAtATime(Vec<u8>);
        impl Read for OneAtATime {
            fn read(&mut self, buf: &mut [u8]) -> io::Result<usize> {
                if self.0.is_empty() || buf.is_empty() {
                    return Ok(0);
                }
                buf[0] = self.0.remove(0);
                Ok(1)
            }
        }
        let mut r = CancellableReader::new(CH_BASE + 29, OneAtATime(b"abc".to_vec()));
        let mut out = Vec::new();
        r.read_to_end(&mut out).expect("copy");
        assert_eq!(out, b"abc");
    }

    #[test]
    fn reader_propagates_inner_errors_unchanged() {
        struct Failing;
        impl Read for Failing {
            fn read(&mut self, _buf: &mut [u8]) -> io::Result<usize> {
                Err(io::Error::new(io::ErrorKind::PermissionDenied, "nope"))
            }
        }
        let mut r = CancellableReader::new(CH_BASE + 30, Failing);
        let err = r.read(&mut [0u8; 4]).expect_err("inner error");
        assert_eq!(err.kind(), io::ErrorKind::PermissionDenied);
    }

    #[test]
    fn two_threads_on_two_channels_do_not_interfere() {
        let (a, b) = (CH_BASE + 40, CH_BASE + 41);
        set_cancelled(a, true);

        let handle = std::thread::spawn(move || {
            // The flag map is shared across threads, so the child sees `a`'s
            // pending cancel but must find `b` untouched.
            (is_cancelled(a), is_cancelled(b))
        });
        let (saw_a, saw_b) = handle.join().expect("joins");
        set_cancelled(a, false);

        assert!(saw_a, "a pending cancel is visible from any thread");
        assert!(!saw_b, "an unrelated channel stays clear");
    }

    #[test]
    fn concurrent_set_and_read_do_not_deadlock() {
        // `set_cancelled` takes a write lock and `is_cancelled` a read lock;
        // hammering both from several threads catches a lock-ordering mistake.
        let base = CH_BASE + 50;
        let handles: Vec<_> = (0..4)
            .map(|i| {
                std::thread::spawn(move || {
                    let ch = base + i;
                    for _ in 0..200 {
                        set_cancelled(ch, true);
                        assert!(is_cancelled(ch));
                        set_cancelled(ch, false);
                        assert!(!is_cancelled(ch));
                    }
                })
            })
            .collect();
        for h in handles {
            h.join().expect("no thread panicked or deadlocked");
        }
    }
}
