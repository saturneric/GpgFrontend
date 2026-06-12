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

//! In-memory TTL cache for decryption passphrases, keyed by (channel, fingerprint, info).
//!
//! Entries are zeroized on eviction or drop. The global `PASSWORD_CACHE` singleton
//! has a 5-minute TTL; the cache is bypassed when no fingerprint is known.

use once_cell::sync::Lazy;
use std::io;
use std::{
    collections::HashMap,
    sync::{Arc, Mutex},
    time::{Duration, Instant},
};
use zeroize::Zeroize;

#[cfg(unix)]
fn lock_memory(buf: &[u8]) -> io::Result<()> {
    if buf.is_empty() {
        return Ok(());
    }

    let ret = unsafe { libc::mlock(buf.as_ptr() as *const libc::c_void, buf.len()) };

    if ret == 0 {
        Ok(())
    } else {
        Err(io::Error::last_os_error())
    }
}

#[cfg(unix)]
fn unlock_memory(buf: &[u8]) -> io::Result<()> {
    if buf.is_empty() {
        return Ok(());
    }

    let ret = unsafe { libc::munlock(buf.as_ptr() as *const libc::c_void, buf.len()) };

    if ret == 0 {
        Ok(())
    } else {
        Err(io::Error::last_os_error())
    }
}

#[cfg(windows)]
fn lock_memory(buf: &[u8]) -> io::Result<()> {
    use windows_sys::Win32::System::Memory::VirtualLock;

    if buf.is_empty() {
        return Ok(());
    }

    let ok = unsafe { VirtualLock(buf.as_ptr() as *const core::ffi::c_void, buf.len()) };

    if ok != 0 {
        Ok(())
    } else {
        Err(io::Error::last_os_error())
    }
}

#[cfg(windows)]
fn unlock_memory(buf: &[u8]) -> io::Result<()> {
    use windows_sys::Win32::System::Memory::VirtualUnlock;

    if buf.is_empty() {
        return Ok(());
    }

    let ok = unsafe { VirtualUnlock(buf.as_ptr() as *const core::ffi::c_void, buf.len()) };

    if ok != 0 {
        Ok(())
    } else {
        Err(io::Error::last_os_error())
    }
}

#[cfg(not(any(unix, windows)))]
fn lock_memory(_buf: &[u8]) -> io::Result<()> {
    Ok(())
}

#[cfg(not(any(unix, windows)))]
fn unlock_memory(_buf: &[u8]) -> io::Result<()> {
    Ok(())
}

/// A passphrase buffer that zeroes its memory when dropped.
#[derive(Debug)]
pub struct CachedSecret {
    data: Vec<u8>,
    locked: bool, // if locked, the secret is currently in use and should not be evicted from the cache
}

impl CachedSecret {
    pub fn new(data: Vec<u8>) -> Self {
        let locked = lock_memory(&data).is_ok();
        Self { data, locked }
    }

    pub fn as_slice(&self) -> &[u8] {
        &self.data.as_slice()
    }
}

impl Drop for CachedSecret {
    fn drop(&mut self) {
        // Zero the memory before dropping.
        self.data.zeroize();

        // Unlock the memory if it was successfully locked. This is a best-effort attempt;
        if self.locked {
            let _ = unlock_memory(&self.data);
        }
    }
}

#[derive(Debug)]
struct PasswordCacheEntry {
    secret: CachedSecret,
    expires_at: Instant,
}

/// Controls how the passphrase cache is consulted for a given operation.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum PasswordCachePolicy {
    /// Check the cache first; on miss, fetch from the callback and store the result.
    Default,
    /// Skip the cache entirely — neither read from nor write to it.
    ///
    /// Used for symmetric operations where no fingerprint is available to key
    /// the cache on, and for the "set new password" flow.
    Bypass,
    /// Evict the existing entry, re-prompt the user, then store the new value.
    ///
    /// Used after a bad-passphrase error to force the user to re-enter.
    Refresh,
}

/// Cache lookup key: a combination of channel, key fingerprint, and operation context.
///
/// FPR comparisons are case-insensitive (normalised to uppercase on insert/lookup).
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct PasswordCacheKey {
    pub channel: i32,
    pub fpr: String,
    pub info: String,
}

/// Thread-safe TTL cache for decryption passphrases. Entries are zeroed on eviction or drop.
#[derive(Clone)]
pub struct PasswordCache {
    inner: Arc<Mutex<HashMap<PasswordCacheKey, PasswordCacheEntry>>>,
    ttl: Duration,
}

impl PasswordCache {
    pub fn new(ttl: Duration) -> Self {
        Self {
            inner: Arc::new(Mutex::new(HashMap::new())),
            ttl,
        }
    }

    pub fn get(&self, key: &PasswordCacheKey) -> Option<Vec<u8>> {
        let mut key = key.clone();
        key.fpr = key.fpr.to_uppercase(); // Ensure FPR is case-insensitive
        let now = Instant::now();
        let mut guard = self.inner.lock().expect("password cache poisoned");

        let expired = guard
            .get(&key)
            .map(|e| e.expires_at <= now)
            .unwrap_or(false);

        if expired {
            guard.remove(&key);
            return None;
        }

        guard.get(&key).map(|e| e.secret.as_slice().to_vec())
    }

    pub fn put(&self, key: PasswordCacheKey, value: Vec<u8>) {
        let mut key = key;
        key.fpr = key.fpr.to_uppercase(); // Ensure FPR is case-insitive
        let mut guard = self.inner.lock().expect("password cache poisoned");
        guard.insert(
            key,
            PasswordCacheEntry {
                secret: CachedSecret::new(value),
                expires_at: Instant::now() + self.ttl,
            },
        );
    }

    pub fn remove(&self, key: &PasswordCacheKey) {
        let mut key = key.clone();
        key.fpr = key.fpr.to_uppercase(); // Ensure FPR is case-insensitive
        let mut guard = self.inner.lock().expect("password cache poisoned");
        guard.remove(&key);
    }

    pub fn remove_by_fpr(&self, fpr: &str) {
        let fpr = fpr.to_uppercase(); // Ensure FPR is case-insensitive
        let mut guard = self.inner.lock().expect("password cache poisoned");
        guard.retain(|k, _| k.fpr != fpr);
    }
}

pub static PASSWORD_CACHE: Lazy<PasswordCache> =
    Lazy::new(|| PasswordCache::new(Duration::from_secs(300)));
