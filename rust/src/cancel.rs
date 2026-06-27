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
    let mut map = CANCEL_FLAGS
        .write()
        .unwrap_or_else(|e| e.into_inner());
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
pub fn status_or_canceled(channel: i32, fallback: crate::types::GfrStatus) -> crate::types::GfrStatus {
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
