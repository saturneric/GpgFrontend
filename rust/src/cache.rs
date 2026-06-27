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
//! Entries are zeroized on eviction or drop. Each entry has two deadlines,
//! modelled on gpg-agent's `default-cache-ttl` / `max-cache-ttl`:
//!
//! * a **sliding** idle window (`ttl`) that is renewed on every cache hit, so a
//!   passphrase that keeps being used does not expire mid-session, and
//! * an **absolute** cap (`max_ttl`) measured from first entry, which the
//!   sliding window can never extend past.
//!
//! The global `PASSWORD_CACHE` singleton defaults to a 10-minute sliding window
//! and a 2-hour hard cap; both are reconfigurable at runtime via
//! [`PasswordCache::set_ttl`]. The cache is bypassed when no fingerprint is known.

use once_cell::sync::Lazy;
use std::io;
use std::{
    collections::HashMap,
    sync::{
        Arc, Mutex,
        atomic::{AtomicU64, Ordering},
    },
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
    /// Sliding deadline, renewed on each cache hit; never extended past `hard_expires_at`.
    expires_at: Instant,
    /// Absolute deadline from first insertion; the sliding window cannot exceed it.
    hard_expires_at: Instant,
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
///
/// The `ttl` (sliding) and `max_ttl` (absolute) deadlines are stored as atomics
/// in seconds so they can be reconfigured at runtime via [`set_ttl`](Self::set_ttl)
/// without rebuilding the cache or losing existing entries.
#[derive(Clone)]
pub struct PasswordCache {
    inner: Arc<Mutex<HashMap<PasswordCacheKey, PasswordCacheEntry>>>,
    ttl_secs: Arc<AtomicU64>,
    max_ttl_secs: Arc<AtomicU64>,
}

impl PasswordCache {
    pub fn new(ttl: Duration, max_ttl: Duration) -> Self {
        Self {
            inner: Arc::new(Mutex::new(HashMap::new())),
            ttl_secs: Arc::new(AtomicU64::new(ttl.as_secs())),
            // The hard cap can never be shorter than the sliding window.
            max_ttl_secs: Arc::new(AtomicU64::new(max_ttl.as_secs().max(ttl.as_secs()))),
        }
    }

    /// Current sliding idle window.
    fn ttl(&self) -> Duration {
        Duration::from_secs(self.ttl_secs.load(Ordering::Relaxed))
    }

    /// Current absolute cap from first insertion.
    fn max_ttl(&self) -> Duration {
        Duration::from_secs(self.max_ttl_secs.load(Ordering::Relaxed))
    }

    /// Reconfigure the cache timeouts. `max_ttl` is clamped up to at least `ttl`
    /// so the absolute cap can never be shorter than the sliding window. Only
    /// affects entries inserted or refreshed after this call.
    pub fn set_ttl(&self, ttl: Duration, max_ttl: Duration) {
        let ttl = ttl.as_secs();
        self.ttl_secs.store(ttl, Ordering::Relaxed);
        self.max_ttl_secs
            .store(max_ttl.as_secs().max(ttl), Ordering::Relaxed);
    }

    pub fn get(&self, key: &PasswordCacheKey) -> Option<Vec<u8>> {
        let mut key = key.clone();
        key.fpr = key.fpr.to_uppercase(); // Ensure FPR is case-insensitive
        let now = Instant::now();
        let ttl = self.ttl();
        let mut guard = self.inner.lock().expect("password cache poisoned");

        let entry = guard.get_mut(&key)?;

        // Evict once either the sliding window or the absolute cap has elapsed.
        if entry.expires_at <= now || entry.hard_expires_at <= now {
            guard.remove(&key);
            return None;
        }

        // Slide the idle window forward on use, but never past the hard cap.
        entry.expires_at = (now + ttl).min(entry.hard_expires_at);
        Some(entry.secret.as_slice().to_vec())
    }

    pub fn put(&self, key: PasswordCacheKey, value: Vec<u8>) {
        let mut key = key;
        key.fpr = key.fpr.to_uppercase(); // Ensure FPR is case-insitive
        let now = Instant::now();
        let max_ttl = self.max_ttl();
        // The initial sliding deadline must also respect the hard cap.
        let ttl = self.ttl().min(max_ttl);
        let mut guard = self.inner.lock().expect("password cache poisoned");
        guard.insert(
            key,
            PasswordCacheEntry {
                secret: CachedSecret::new(value),
                expires_at: now + ttl,
                hard_expires_at: now + max_ttl,
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

/// Global passphrase cache: 10-minute sliding window, 2-hour absolute cap,
/// mirroring gpg-agent's `default-cache-ttl` / `max-cache-ttl` defaults.
pub static PASSWORD_CACHE: Lazy<PasswordCache> =
    Lazy::new(|| PasswordCache::new(Duration::from_secs(600), Duration::from_secs(7200)));

#[cfg(test)]
mod tests {
    use super::*;

    fn key() -> PasswordCacheKey {
        PasswordCacheKey {
            channel: 0,
            fpr: "DEADBEEF".to_string(),
            info: "decrypt".to_string(),
        }
    }

    #[test]
    fn get_returns_stored_value() {
        let cache = PasswordCache::new(Duration::from_secs(600), Duration::from_secs(7200));
        cache.put(key(), b"secret".to_vec());
        assert_eq!(cache.get(&key()).as_deref(), Some(b"secret".as_ref()));
    }

    #[test]
    fn fpr_lookup_is_case_insensitive() {
        let cache = PasswordCache::new(Duration::from_secs(600), Duration::from_secs(7200));
        let mut lower = key();
        lower.fpr = "deadbeef".to_string();
        cache.put(lower, b"secret".to_vec());
        assert_eq!(cache.get(&key()).as_deref(), Some(b"secret".as_ref()));
    }

    #[test]
    fn entry_expires_after_sliding_window() {
        let cache = PasswordCache::new(Duration::from_millis(0), Duration::from_secs(7200));
        cache.put(key(), b"secret".to_vec());
        // A zero-length idle window means the entry is already stale on next access.
        assert_eq!(cache.get(&key()), None);
    }

    #[test]
    fn new_clamps_hard_cap_up_to_sliding_window() {
        // A hard cap shorter than the sliding window is nonsensical; the
        // constructor must raise it so the cap is never the shorter of the two.
        let cache = PasswordCache::new(Duration::from_secs(600), Duration::from_secs(60));
        assert_eq!(cache.ttl(), Duration::from_secs(600));
        assert_eq!(cache.max_ttl(), Duration::from_secs(600));
    }

    #[test]
    fn fresh_entry_within_both_windows_is_returned() {
        // ttl < max_ttl: a just-inserted entry is inside both deadlines and a
        // read both returns it and slides the idle window forward.
        let cache = PasswordCache::new(Duration::from_secs(600), Duration::from_secs(7200));
        cache.put(key(), b"secret".to_vec());
        assert_eq!(cache.get(&key()).as_deref(), Some(b"secret".as_ref()));
        assert_eq!(cache.get(&key()).as_deref(), Some(b"secret".as_ref()));
    }

    #[test]
    fn set_ttl_clamps_max_below_ttl() {
        let cache = PasswordCache::new(Duration::from_secs(600), Duration::from_secs(7200));
        cache.set_ttl(Duration::from_secs(900), Duration::from_secs(300));
        // max_ttl is clamped up to ttl, so it can never be the shorter of the two.
        assert_eq!(cache.max_ttl(), Duration::from_secs(900));
        assert_eq!(cache.ttl(), Duration::from_secs(900));
    }

    #[test]
    fn remove_by_fpr_evicts_all_matching_entries() {
        let cache = PasswordCache::new(Duration::from_secs(600), Duration::from_secs(7200));
        let mut other_info = key();
        other_info.info = "sign".to_string();
        cache.put(key(), b"a".to_vec());
        cache.put(other_info, b"b".to_vec());
        cache.remove_by_fpr("deadbeef");
        assert_eq!(cache.get(&key()), None);
    }
}
