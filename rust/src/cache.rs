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
        let mut guard = self.inner.lock().unwrap_or_else(|e| e.into_inner());

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
        let mut guard = self.inner.lock().unwrap_or_else(|e| e.into_inner());
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
        let mut guard = self.inner.lock().unwrap_or_else(|e| e.into_inner());
        guard.remove(&key);
    }

    pub fn remove_by_fpr(&self, fpr: &str) {
        let fpr = fpr.to_uppercase(); // Ensure FPR is case-insensitive
        let mut guard = self.inner.lock().unwrap_or_else(|e| e.into_inner());
        guard.retain(|k, _| k.fpr != fpr);
    }

    /// Drop every cached entry, for every channel and fingerprint.
    pub fn clear(&self) {
        let mut guard = self.inner.lock().unwrap_or_else(|e| e.into_inner());
        guard.clear();
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

    #[test]
    fn clear_evicts_every_entry() {
        let cache = PasswordCache::new(Duration::from_secs(600), Duration::from_secs(7200));
        let mut other_key = key();
        other_key.fpr = "CAFEBABE".to_string();
        cache.put(key(), b"a".to_vec());
        cache.put(other_key.clone(), b"b".to_vec());
        cache.clear();
        assert_eq!(cache.get(&key()), None);
        assert_eq!(cache.get(&other_key), None);
    }
}

#[cfg(test)]
mod cache_more_tests {
    //! The gpg-agent-style passphrase cache: sliding idle window, absolute
    //! cap, case-insensitive fingerprints, and zeroing on eviction.
    //!
    //! Timing is expressed with zero-length windows rather than sleeps, so the
    //! suite stays fast and deterministic.

    use super::*;

    fn key_for(channel: i32, fpr: &str, info: &str) -> PasswordCacheKey {
        PasswordCacheKey {
            channel,
            fpr: fpr.to_string(),
            info: info.to_string(),
        }
    }

    fn cache(ttl_secs: u64, max_secs: u64) -> PasswordCache {
        PasswordCache::new(Duration::from_secs(ttl_secs), Duration::from_secs(max_secs))
    }

    // -- keying ------------------------------------------------------------

    #[test]
    fn a_miss_returns_none_rather_than_panicking() {
        let c = cache(60, 600);
        assert!(c.get(&key_for(0, "NOSUCHKEY", "Decryption")).is_none());
    }

    #[test]
    fn the_channel_is_part_of_the_key() {
        // Two OpenPGP contexts may hold the same key with different
        // passphrase policies; they must not share a cache entry.
        let c = cache(60, 600);
        c.put(key_for(1, "AABB", "Decryption"), b"one".to_vec());
        assert_eq!(
            c.get(&key_for(1, "AABB", "Decryption")).as_deref(),
            Some(&b"one"[..])
        );
        assert!(c.get(&key_for(2, "AABB", "Decryption")).is_none());
    }

    #[test]
    fn the_info_string_is_part_of_the_key() {
        let c = cache(60, 600);
        c.put(key_for(0, "AABB", "Decryption"), b"dec".to_vec());
        assert!(c.get(&key_for(0, "AABB", "Signing")).is_none());
    }

    #[test]
    fn the_fingerprint_is_matched_case_insensitively_on_insert() {
        // Fingerprints reach the cache from several code paths, some
        // uppercase, some not; normalising on both sides keeps them one entry.
        let c = cache(60, 600);
        c.put(key_for(0, "abcdef", "Decryption"), b"v".to_vec());
        assert_eq!(
            c.get(&key_for(0, "ABCDEF", "Decryption")).as_deref(),
            Some(&b"v"[..])
        );
    }

    #[test]
    fn the_fingerprint_is_matched_case_insensitively_on_lookup() {
        let c = cache(60, 600);
        c.put(key_for(0, "ABCDEF", "Decryption"), b"v".to_vec());
        assert_eq!(
            c.get(&key_for(0, "abcdef", "Decryption")).as_deref(),
            Some(&b"v"[..])
        );
    }

    #[test]
    fn the_info_string_is_matched_exactly() {
        // Only the fingerprint is normalised; the info string is supplied by
        // the engine itself and is already canonical.
        let c = cache(60, 600);
        c.put(key_for(0, "AABB", "DECRYPTION"), b"v".to_vec());
        assert!(c.get(&key_for(0, "AABB", "decryption")).is_none());
    }

    #[test]
    fn an_empty_fingerprint_is_a_usable_key() {
        // `fetch_password_with_cache` bypasses the cache for empty
        // fingerprints, but the cache itself must not choke on one.
        let c = cache(60, 600);
        c.put(key_for(0, "", "Symmetric"), b"v".to_vec());
        assert_eq!(
            c.get(&key_for(0, "", "Symmetric")).as_deref(),
            Some(&b"v"[..])
        );
    }

    // -- values ------------------------------------------------------------

    #[test]
    fn a_stored_value_is_returned_byte_for_byte() {
        let c = cache(60, 600);
        let secret = vec![0x00, 0xFF, 0x41, 0x00, 0x7F];
        c.put(key_for(0, "AABB", "Decryption"), secret.clone());
        assert_eq!(c.get(&key_for(0, "AABB", "Decryption")), Some(secret));
    }

    #[test]
    fn an_empty_value_round_trips() {
        let c = cache(60, 600);
        c.put(key_for(0, "AABB", "Decryption"), Vec::new());
        assert_eq!(c.get(&key_for(0, "AABB", "Decryption")), Some(Vec::new()));
    }

    #[test]
    fn a_second_put_replaces_the_first() {
        let c = cache(60, 600);
        c.put(key_for(0, "AABB", "Decryption"), b"old".to_vec());
        c.put(key_for(0, "AABB", "Decryption"), b"new".to_vec());
        assert_eq!(
            c.get(&key_for(0, "AABB", "Decryption")).as_deref(),
            Some(&b"new"[..])
        );
    }

    #[test]
    fn a_large_value_round_trips() {
        let c = cache(60, 600);
        let big = vec![0x5Au8; 64 * 1024];
        c.put(key_for(0, "AABB", "Decryption"), big.clone());
        assert_eq!(c.get(&key_for(0, "AABB", "Decryption")), Some(big));
    }

    #[test]
    fn a_get_returns_a_copy_not_a_view() {
        // The caller owns what it gets back; mutating it must not corrupt the
        // cached secret.
        let c = cache(60, 600);
        c.put(key_for(0, "AABB", "Decryption"), b"original".to_vec());
        let mut first = c.get(&key_for(0, "AABB", "Decryption")).expect("hit");
        first[0] = b'X';
        assert_eq!(
            c.get(&key_for(0, "AABB", "Decryption")).as_deref(),
            Some(&b"original"[..])
        );
    }

    // -- expiry ------------------------------------------------------------

    #[test]
    fn a_zero_sliding_window_expires_immediately() {
        let c = cache(0, 600);
        c.put(key_for(0, "AABB", "Decryption"), b"v".to_vec());
        assert!(c.get(&key_for(0, "AABB", "Decryption")).is_none());
    }

    #[test]
    fn a_zero_absolute_cap_is_clamped_up_and_does_not_expire_entries() {
        // The cap can never be shorter than the sliding window, so asking for
        // a zero cap alongside a 600s window yields a 600s cap -- the entry
        // stays. (Asking for both to be zero *does* expire immediately; see
        // `a_zero_sliding_window_expires_immediately`.)
        let c = cache(600, 0);
        assert_eq!(c.max_ttl(), Duration::from_secs(600));
        c.put(key_for(0, "AABB", "Decryption"), b"v".to_vec());
        assert!(c.get(&key_for(0, "AABB", "Decryption")).is_some());
    }

    #[test]
    fn an_expired_entry_is_evicted_not_merely_hidden() {
        // Leaving the entry in place would keep the secret in memory past its
        // TTL, which is the whole thing the cap exists to prevent.
        let c = cache(0, 600);
        c.put(key_for(0, "AABB", "Decryption"), b"v".to_vec());
        assert!(c.get(&key_for(0, "AABB", "Decryption")).is_none());
        let guard = c.inner.lock().expect("lock");
        assert!(guard.is_empty(), "the expired entry must be removed");
    }

    #[test]
    fn a_hit_slides_the_idle_window_forward() {
        let c = cache(600, 3600);
        let k = key_for(0, "AABB", "Decryption");
        c.put(k.clone(), b"v".to_vec());

        let first_deadline = {
            let guard = c.inner.lock().expect("lock");
            guard.get(&k).expect("entry").expires_at
        };
        assert!(c.get(&k).is_some());
        let second_deadline = {
            let guard = c.inner.lock().expect("lock");
            guard.get(&k).expect("entry").expires_at
        };
        assert!(
            second_deadline >= first_deadline,
            "using an entry should renew its idle window"
        );
    }

    #[test]
    fn the_sliding_window_never_passes_the_absolute_cap() {
        // The cap is what bounds how long a passphrase can live in memory no
        // matter how often it is used.
        let c = cache(600, 600);
        let k = key_for(0, "AABB", "Decryption");
        c.put(k.clone(), b"v".to_vec());
        assert!(c.get(&k).is_some());
        let guard = c.inner.lock().expect("lock");
        let entry = guard.get(&k).expect("entry");
        assert!(entry.expires_at <= entry.hard_expires_at);
    }

    #[test]
    fn new_clamps_the_cap_up_to_the_sliding_window() {
        // A cap shorter than the idle window is nonsense; it is raised rather
        // than silently making every entry expire at once.
        let c = cache(600, 60);
        assert_eq!(c.max_ttl(), Duration::from_secs(600));
    }

    #[test]
    fn set_ttl_reconfigures_both_windows() {
        let c = cache(60, 600);
        c.set_ttl(Duration::from_secs(120), Duration::from_secs(1200));
        assert_eq!(c.ttl(), Duration::from_secs(120));
        assert_eq!(c.max_ttl(), Duration::from_secs(1200));
    }

    #[test]
    fn set_ttl_clamps_the_cap_up_to_the_window() {
        let c = cache(60, 600);
        c.set_ttl(Duration::from_secs(900), Duration::from_secs(30));
        assert_eq!(c.max_ttl(), Duration::from_secs(900));
    }

    #[test]
    fn set_ttl_does_not_disturb_existing_entries() {
        // Documented behaviour: only entries inserted or refreshed after the
        // call use the new windows.
        let c = cache(600, 3600);
        let k = key_for(0, "AABB", "Decryption");
        c.put(k.clone(), b"v".to_vec());
        c.set_ttl(Duration::from_secs(0), Duration::from_secs(0));
        assert!(
            c.get(&k).is_some(),
            "an entry already in the cache keeps its own deadlines"
        );
    }

    // -- eviction ----------------------------------------------------------

    #[test]
    fn remove_drops_exactly_one_entry() {
        let c = cache(60, 600);
        c.put(key_for(0, "AABB", "Decryption"), b"a".to_vec());
        c.put(key_for(0, "CCDD", "Decryption"), b"b".to_vec());
        c.remove(&key_for(0, "AABB", "Decryption"));
        assert!(c.get(&key_for(0, "AABB", "Decryption")).is_none());
        assert!(c.get(&key_for(0, "CCDD", "Decryption")).is_some());
    }

    #[test]
    fn remove_is_case_insensitive_in_the_fingerprint() {
        let c = cache(60, 600);
        c.put(key_for(0, "AABB", "Decryption"), b"a".to_vec());
        c.remove(&key_for(0, "aabb", "Decryption"));
        assert!(c.get(&key_for(0, "AABB", "Decryption")).is_none());
    }

    #[test]
    fn removing_a_missing_entry_is_harmless() {
        let c = cache(60, 600);
        c.remove(&key_for(0, "NOTTHERE", "Decryption"));
    }

    #[test]
    fn remove_by_fpr_drops_every_channel_and_context_for_that_key() {
        // Used when a key is deleted or its passphrase changed: every cached
        // secret for it must go, whichever context cached it.
        let c = cache(60, 600);
        c.put(key_for(0, "AABB", "Decryption"), b"a".to_vec());
        c.put(key_for(1, "AABB", "Signing"), b"b".to_vec());
        c.put(key_for(0, "CCDD", "Decryption"), b"c".to_vec());

        c.remove_by_fpr("AABB");

        assert!(c.get(&key_for(0, "AABB", "Decryption")).is_none());
        assert!(c.get(&key_for(1, "AABB", "Signing")).is_none());
        assert!(c.get(&key_for(0, "CCDD", "Decryption")).is_some());
    }

    #[test]
    fn remove_by_fpr_is_case_insensitive() {
        let c = cache(60, 600);
        c.put(key_for(0, "AABB", "Decryption"), b"a".to_vec());
        c.remove_by_fpr("aabb");
        assert!(c.get(&key_for(0, "AABB", "Decryption")).is_none());
    }

    #[test]
    fn clear_empties_everything() {
        let c = cache(60, 600);
        for i in 0..8 {
            c.put(key_for(i, &format!("KEY{i}"), "Decryption"), vec![i as u8]);
        }
        c.clear();
        for i in 0..8 {
            assert!(
                c.get(&key_for(i, &format!("KEY{i}"), "Decryption"))
                    .is_none()
            );
        }
    }

    #[test]
    fn clear_on_an_empty_cache_is_harmless() {
        cache(60, 600).clear();
    }

    // -- CachedSecret ------------------------------------------------------

    #[test]
    fn a_cached_secret_exposes_its_bytes() {
        let s = CachedSecret::new(b"passphrase".to_vec());
        assert_eq!(s.as_slice(), b"passphrase");
    }

    #[test]
    fn an_empty_cached_secret_is_constructible() {
        // `lock_memory` on a zero-length buffer must not be treated as a
        // failure that panics.
        let s = CachedSecret::new(Vec::new());
        assert!(s.as_slice().is_empty());
    }

    #[test]
    fn dropping_a_cached_secret_does_not_panic_whether_or_not_mlock_succeeded() {
        // mlock is best-effort (it needs RLIMIT_MEMLOCK headroom); the Drop
        // impl only unlocks when the lock actually succeeded.
        for size in [0usize, 1, 4096, 1 << 16] {
            drop(CachedSecret::new(vec![0xAA; size]));
        }
    }

    // -- concurrency -------------------------------------------------------

    #[test]
    fn the_cache_is_shared_across_clones() {
        // `PasswordCache` is `Clone` over an `Arc`, so a clone must see the
        // same entries rather than starting empty.
        let c = cache(60, 600);
        let c2 = c.clone();
        c.put(key_for(0, "AABB", "Decryption"), b"v".to_vec());
        assert_eq!(
            c2.get(&key_for(0, "AABB", "Decryption")).as_deref(),
            Some(&b"v"[..])
        );
    }

    #[test]
    fn concurrent_access_does_not_deadlock_or_corrupt() {
        let c = cache(600, 3600);
        let handles: Vec<_> = (0..4)
            .map(|t| {
                let c = c.clone();
                std::thread::spawn(move || {
                    for i in 0..100 {
                        let k = key_for(t, &format!("K{i}"), "Decryption");
                        c.put(k.clone(), vec![t as u8, i as u8]);
                        assert_eq!(c.get(&k), Some(vec![t as u8, i as u8]));
                        c.remove(&k);
                    }
                })
            })
            .collect();
        for h in handles {
            h.join().expect("no thread panicked or deadlocked");
        }
    }
}
