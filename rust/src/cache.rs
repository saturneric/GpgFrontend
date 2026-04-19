use once_cell::sync::Lazy;
use std::{
    collections::HashMap,
    sync::{Arc, Mutex},
    time::{Duration, Instant},
};
use zeroize::Zeroize;

#[derive(Debug)]
pub struct CachedSecret {
    data: Vec<u8>,
}

impl CachedSecret {
    pub fn new(mut data: Vec<u8>) -> Self {
        data.shrink_to_fit();
        Self { data }
    }

    pub fn as_slice(&self) -> &[u8] {
        &self.data
    }
}

impl Drop for CachedSecret {
    fn drop(&mut self) {
        self.data.zeroize();
    }
}

#[derive(Debug)]
struct PasswordCacheEntry {
    secret: CachedSecret,
    expires_at: Instant,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum PasswordCachePolicy {
    /// search cache first, if not found or expired, then fetch and update cache
    Default,

    /// completely bypass the cache, neither read nor write
    Bypass,

    /// force re-fetching the password:
    /// 1. first delete the old cache
    /// 2. then re-prompt for the password
    /// 3. finally update the cache with the new password
    Refresh,
}

#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct PasswordCacheKey {
    pub channel: i32,
    pub fpr: String,
    pub info: String,
}

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
