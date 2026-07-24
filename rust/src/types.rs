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

//! C-compatible (`#[repr(C)]`) types shared across the FFI boundary.
//!
//! All structs and enums in this module are layout-compatible with the
//! corresponding C/C++ declarations in `GFCoreRust.h`. Callers must free
//! any heap-allocated pointer fields using the corresponding
//! `gfr_crypto_free_*` functions in `ffi_mem`.

use core::fmt;
use std::{error::Error, ffi::c_void, os::raw::c_char};

use zeroize::{Zeroize, ZeroizeOnDrop};

/// Return status code for all exported FFI functions.
///
/// A value of `Success` (0) indicates the operation completed without error.
/// All negative values indicate failure; the specific variant identifies the
/// failure category.
#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord)]
pub enum GfrStatus {
    Success = 0,

    // --- General Errors ---
    /// Null pointer or otherwise invalid argument was passed.
    ErrorInvalidInput = -1,
    /// An unexpected internal error occurred (e.g. CString conversion failure).
    ErrorInternal = -5,
    /// A Rust panic was caught via `catch_unwind`; state is undefined.
    ErrorPanic = -99,

    // --- Key & Auth Errors ---
    /// Key pair generation failed.
    ErrorKeygenFailed = -2,
    /// Passphrase operation failed.
    ErrorPasswordFailed = -3,
    /// The required key was not found.
    ErrorNoKey = -6,
    /// The passphrase fetch callback failed or returned an error.
    ErrorFetchPasswordFailed = -9,
    /// The supplied passphrase was incorrect.
    ErrorWrongPassword = -10,
    /// The passphrase confirmation did not match the first entry.
    ErrorPasswordMismatch = -14,

    // --- Data & Parsing Errors ---
    /// Armoring or de-armoring the OpenPGP data failed.
    ErrorArmorFailed = -4,
    /// General parsing error (malformed packet or data).
    ErrorInvalidData = -7,
    /// Packet checksum failed; data may have been tampered with.
    ErrorDataCorrupted = -11,
    /// The algorithm used by the key is not supported (e.g. IDEA).
    ErrorUnsupportedAlgorithm = -12,

    // --- IO & Execution Errors ---
    /// File read or write failure.
    ErrorIo = -13,
    /// Decryption failed (session key could not be recovered).
    ErrorDecryptionFailed = -8,

    ErrorCanceled = -15,
    /// No public key available to verify or encrypt.
    ErrorNoPublicKey = -16,
    /// No secret key available to decrypt or sign.
    ErrorNoSecretKey = -17,
    /// The signature is mathematically invalid.
    ErrorBadSignature = -18,
    /// The key has expired.
    ErrorExpiredKey = -19,
    /// The key has been revoked.
    ErrorRevokedKey = -20,
    /// The key cannot be used (disabled, not capable, etc.).
    ErrorUnusableKey = -21,
    /// The requested feature is not supported by this engine version.
    ErrorUnsupportedFeature = -22,
    /// No data was found where data was expected.
    ErrorNoData = -23,
    /// The ASCII armor header or trailer is malformed.
    ErrorBadArmor = -24,
    /// The passphrase supplied for decryption is wrong.
    ErrorBadPassphrase = -25,
}

impl fmt::Display for GfrStatus {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{:?}", self)
    }
}

impl Error for GfrStatus {}

/// Safe buffer type for passing binary data across FFI boundary.
///
/// Contains a pointer and explicit length, avoiding null-termination issues.
/// Used for key blocks, signatures, and other binary data.
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct GfrBuffer {
    /// Pointer to the data. May be null if len is 0.
    pub data: *const u8,
    /// Length of the data in bytes.
    pub len: usize,
}

impl GfrBuffer {
    /// Create an empty GfrBuffer.
    pub const fn empty() -> Self {
        Self {
            data: std::ptr::null(),
            len: 0,
        }
    }

    /// Check if the buffer is empty (null pointer or zero length).
    pub fn is_empty(&self) -> bool {
        self.data.is_null() || self.len == 0
    }

    /// Convert to a byte slice. Returns None if the pointer is null.
    ///
    /// # Safety
    /// The caller must ensure the pointer is valid for `len` bytes.
    pub unsafe fn as_slice(&self) -> Option<&[u8]> {
        if self.data.is_null() {
            None
        } else {
            Some(unsafe { std::slice::from_raw_parts(self.data, self.len) })
        }
    }

    /// Convert to a UTF-8 string slice. Returns error if the data is not valid UTF-8.
    ///
    /// # Safety
    /// The caller must ensure the pointer is valid for `len` bytes.
    pub unsafe fn as_str(&self) -> Result<&str, GfrStatus> {
        if self.is_empty() {
            return Err(GfrStatus::ErrorInvalidInput);
        }
        let bytes = unsafe { std::slice::from_raw_parts(self.data, self.len) };
        std::str::from_utf8(bytes).map_err(|_| GfrStatus::ErrorInvalidInput)
    }
}

/// OpenPGP key version identifier.
#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Eq, Zeroize)]
pub enum GfrOpenPGPKeyVersion {
    Unknown = 0,
    /// OpenPGP v4 key (RFC 4880).
    V4 = 4,
    /// OpenPGP v6 key (draft-ietf-openpgp-crypto-refresh).
    V6 = 6,
}

/// Public-key algorithm identifier for key generation and export.
///
/// Post-quantum variants (`KYBER*`, `MLDSA*`, `SLHDSA*`) are experimental and
/// non-standard. `ED25519LEGACY` is kept only for backward compatibility with
/// keys generated by old GpgFrontend versions.
#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Eq, Zeroize)]
pub enum GfrKeyAlgo {
    Unknown = 0,
    ED25519,
    CV25519,
    NISTP256,
    NISTP384,
    NISTP521,
    BRAINPOOLP256,
    BRAINPOOLP384,
    BRAINPOOLP512,
    ED448,
    X448,
    RSA1024,
    RSA2048,
    RSA3072,
    RSA4096,
    SECP256K1,
    DSA1024,
    DSA2048,
    DSA3072,
    /// Non-standard Ed25519 variant from early GpgFrontend versions. Read-only;
    /// new keys must use `ED25519`.
    ED25519LEGACY,
    // Post-quantum (experimental, non-standard)
    KYBER768X25519,
    KYBER1024X448,
    MLDSA65ED25519,
    MLDSA87ED448,
    SLHDSASHAKE128S,
    SLHDSASHAKE128F,
    SLHDSASHAKE256S,
}

/// OpenPGP signature mode.
#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Eq, Zeroize)]
pub enum GfrSignMode {
    /// Inline signature — plaintext and signature are combined in one packet.
    Inline = 0,
    /// Clear-text signature — human-readable body with an appended signature.
    ClearText = 1,
    /// Detached signature — the signature is a separate file/buffer.
    Detached = 2,
}

/// Reason code used when revoking a key or subkey.
#[repr(C)]
#[derive(Debug, Clone, Copy, Zeroize)]
pub enum GfrRevocationCode {
    NoReason = 0,
    Superseded = 1,
    Compromised = 2,
    Retired = 3,
    UserIdInvalid = 32,
}

/// Configuration for generating a primary key or subkey.
#[repr(C)]
#[derive(Zeroize, ZeroizeOnDrop)]
pub struct GfrKeyConfig {
    /// Algorithm to use for this key.
    pub algo: GfrKeyAlgo,
    /// Whether the key should be capable of signing.
    pub can_sign: bool,
    /// Whether the key should be capable of encryption.
    pub can_encrypt: bool,
    /// Whether the key should be capable of authentication.
    pub can_auth: bool,
    /// Whether the generated key should be protected by a passphrase.
    pub has_passphrase: bool,
    /// Requested OpenPGP key format version for the primary key. `Unknown`
    /// means "let the engine decide" (currently v4). Post-quantum algorithms
    /// always force v6 regardless of this field. Ignored on subkey configs,
    /// which inherit the primary key's version.
    pub ver: GfrOpenPGPKeyVersion,
    /// Absolute expiration time for this key component (Unix epoch, seconds).
    /// `0` means "never expires". Applied per-component: the primary config
    /// governs the primary key, each subkey config governs its own subkey.
    pub expiration_epoch_secs: u64,
}

/// Output of a successful key generation operation.
///
/// All pointer fields are heap-allocated and must be freed with
/// `gfr_crypto_free_key_generate_result`.
#[repr(C)]
pub struct GfrKeyGenerateResult {
    /// Armored secret key block.
    pub secret_key: *mut c_char,
    /// Armored public key block.
    pub public_key: *mut c_char,
    /// Primary key fingerprint string.
    pub fingerprint: *mut c_char,
}

/// Metadata for a single subkey within a key block.
///
/// All pointer fields are heap-allocated; free the enclosing `GfrKeyMetadataC`
/// array with `gfr_free_metadata_array`.
#[repr(C)]
#[derive(Zeroize, ZeroizeOnDrop)]
pub struct GfrSubkeyMetadataC {
    /// OpenPGP key version.
    pub ver: GfrOpenPGPKeyVersion,
    /// Subkey fingerprint string.
    #[zeroize(skip)]
    pub fpr: *mut c_char,
    /// Subkey ID string (last 16 hex digits of the fingerprint).
    #[zeroize(skip)]
    pub key_id: *mut c_char,
    /// Algorithm used by this subkey.
    pub algo: GfrKeyAlgo,
    /// Nominal key length in bits.
    pub key_length: u32,
    /// Creation timestamp (Unix epoch, seconds).
    pub created_at: u32,
    /// Expiration timestamp (Unix epoch, seconds); `0` means never expires.
    pub expires_at: u32,
    /// True if the secret component is available.
    pub has_secret: bool,
    /// True if the subkey has been revoked.
    pub is_revoked: bool,
    pub can_sign: bool,
    pub can_encrypt: bool,
    pub can_auth: bool,
    pub can_certify: bool,
}

/// A single OpenPGP user ID with its revocation and primary status.
#[repr(C)]
#[derive(Zeroize, ZeroizeOnDrop)]
pub struct GfrUserIdC {
    /// User ID string in "Name (Comment) <email>" format.
    #[zeroize(skip)]
    pub user_id: *mut c_char,
    /// True if this is the primary user ID.
    pub is_primary: bool,
    /// True if this user ID has been revoked.
    pub is_revoked: bool,
}

/// Full metadata for a primary OpenPGP key, including its user IDs and subkeys.
///
/// All pointer fields are heap-allocated; free the array with
/// `gfr_free_metadata_array`.
#[repr(C)]
#[derive(Zeroize, ZeroizeOnDrop)]
pub struct GfrKeyMetadataC {
    /// OpenPGP key version.
    pub ver: GfrOpenPGPKeyVersion,
    /// Primary key fingerprint string.
    #[zeroize(skip)]
    pub fpr: *mut c_char,
    /// Primary key ID string.
    #[zeroize(skip)]
    pub key_id: *mut c_char,

    /// Array of user IDs associated with this key.
    #[zeroize(skip)]
    pub user_ids: *mut GfrUserIdC,
    /// Number of entries in `user_ids`.
    pub user_id_count: usize,

    /// Algorithm used by the primary key.
    pub algo: GfrKeyAlgo,
    /// Nominal key length in bits.
    pub key_length: u32,
    /// Creation timestamp (Unix epoch, seconds).
    pub created_at: u32,
    /// Expiration timestamp (Unix epoch, seconds); `0` means never expires.
    pub expires_at: u32,
    /// True if the secret component is available.
    pub has_secret: bool,
    /// True if the key has been revoked.
    pub is_revoked: bool,

    /// Armored public key block (always present).
    #[zeroize(skip)]
    pub public_key_block: *mut std::os::raw::c_char,
    /// Armored secret key block, or null if no secret key is available.
    #[zeroize(skip)]
    pub secret_key_block: *mut std::os::raw::c_char,

    pub can_sign: bool,
    pub can_encrypt: bool,
    pub can_auth: bool,
    pub can_certify: bool,

    /// Array of subkey metadata entries.
    #[zeroize(skip)]
    pub subkeys: *mut GfrSubkeyMetadataC,
    /// Number of entries in `subkeys`.
    pub subkey_count: usize,
}

/// Verification status for a single signature found in a message.
#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Eq, Zeroize)]
pub enum GfrSignatureStatus {
    /// The signature is mathematically valid and the public key was found.
    Valid = 0,
    /// The signature is mathematically invalid (payload tampered or wrong key).
    BadSignature = 1,
    /// The public key required to verify this signature is not available.
    NoKey = 2,
    /// Another parsing or internal error occurred for this signature.
    UnknownError = 3,
}

/// Verification result for a single signature found in the message.
///
/// All pointer fields are heap-allocated; free the enclosing result struct
/// with `gfr_crypto_free_verify_result` or `gfr_crypto_free_verify_metadata`.
#[repr(C)]
#[derive(Zeroize, ZeroizeOnDrop)]
pub struct GfrSignatureResultC {
    /// The type of signature (Inline, ClearText, Detached).
    pub sig_type: GfrSignMode,
    /// Fingerprint of the signer, or null if not available.
    #[zeroize(skip)]
    pub issuer_fpr: *mut c_char,
    /// Verification status for this specific signature.
    pub status: GfrSignatureStatus,
    /// Signature creation timestamp (Unix epoch, seconds).
    pub created_at: u32,
    /// Public-key algorithm string, or null if not available.
    #[zeroize(skip)]
    pub pub_algo: *mut c_char,
    /// Hash algorithm string, or null if not available.
    #[zeroize(skip)]
    pub hash_algo: *mut c_char,
}

/// Status of an individual decryption recipient.
#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum GfrRecipientStatus {
    /// Successfully decrypted using this recipient's secret key.
    Success = 0,
    /// Key ID was found in the message but the secret key is not available.
    NoKey = 1,
    /// The secret key is present but decryption failed (e.g. wrong passphrase).
    Error = 2,
}

/// Information about a single decryption recipient key.
#[repr(C)]
pub struct GfrRecipientResultC {
    /// Key ID string of the recipient subkey used to encrypt the session key.
    pub key_id: *mut c_char,
    /// Public-key algorithm string.
    pub pub_algo: *mut c_char,
    /// Decryption status for this recipient.
    pub status: GfrRecipientStatus,
}

/// An invalid (rejected) encryption recipient.
#[repr(C)]
pub struct GfrInvalidRecipientC {
    /// Fingerprint of the rejected recipient.
    pub fpr: *mut c_char,
    /// Reason the recipient was rejected.
    pub reason: GfrStatus,
}

/// Metadata produced by a signing operation.
#[repr(C)]
#[derive(Zeroize, ZeroizeOnDrop)]
pub struct GfrSignMetadataC {
    /// Array of per-signature results.
    #[zeroize(skip)]
    pub signatures: *mut GfrSignatureResultC,
    /// Number of entries in `signatures`.
    pub signature_count: usize,
}

/// Metadata produced by an encryption operation.
#[repr(C)]
#[derive(Zeroize, ZeroizeOnDrop)]
pub struct GfrEncryptMetadataC {
    /// Array of recipients that could not be encrypted to.
    #[zeroize(skip)]
    pub invalid_recipients: *mut GfrInvalidRecipientC,
    /// Number of entries in `invalid_recipients`.
    pub invalid_recipient_count: usize,
    /// Array of subkeys the session key was actually encrypted to.
    #[zeroize(skip)]
    pub recipients: *mut GfrRecipientResultC,
    /// Number of entries in `recipients`.
    pub recipient_count: usize,
}

/// Metadata produced by a decryption operation.
#[repr(C)]
#[derive(Zeroize, ZeroizeOnDrop)]
pub struct GfrDecryptMetadataC {
    /// Embedded filename from the OpenPGP literal data packet, or null.
    #[zeroize(skip)]
    pub filename: *mut c_char,
    /// Array of recipient key information.
    #[zeroize(skip)]
    pub recipients: *mut GfrRecipientResultC,
    /// Number of entries in `recipients`.
    pub recipient_count: usize,
}

/// Metadata produced by a verification operation.
#[repr(C)]
#[derive(Zeroize, ZeroizeOnDrop)]
pub struct GfrVerifyMetadataC {
    /// Array of per-signature results.
    #[zeroize(skip)]
    pub signatures: *mut GfrSignatureResultC,
    /// Number of entries in `signatures`.
    pub signature_count: usize,
    /// True if at least one signature is fully valid.
    pub is_verified: bool,
}

/// Result of an encryption operation.
///
/// For in-memory operations `data`/`data_len` hold the ciphertext; for
/// file-based operations they are null/0 and the output was written to disk.
/// Free with `gfr_crypto_free_encrypt_result`.
#[repr(C)]
#[derive(Zeroize, ZeroizeOnDrop)]
pub struct GfrEncryptResultC {
    /// Ciphertext bytes, or null for file-based operations.
    #[zeroize(skip)]
    pub data: *mut u8,
    /// Length of `data` in bytes.
    pub data_len: usize,
    /// Encryption metadata (invalid recipients).
    pub meta: GfrEncryptMetadataC,
}

/// Result of a decryption operation. Free with `gfr_crypto_free_decrypt_result`.
#[repr(C)]
#[derive(Zeroize, ZeroizeOnDrop)]
pub struct GfrDecryptResultC {
    /// Decrypted plaintext bytes, or null for file-based operations.
    #[zeroize(skip)]
    pub data: *mut u8,
    /// Length of `data` in bytes.
    pub data_len: usize,
    /// Decryption metadata (filename, recipients).
    pub meta: GfrDecryptMetadataC,
}

/// Result of a signing operation. Free with `gfr_crypto_free_sign_result`.
#[repr(C)]
#[derive(Zeroize, ZeroizeOnDrop)]
pub struct GfrSignResultC {
    /// Signed data bytes (payload + signature for inline; signature only for detached).
    #[zeroize(skip)]
    pub data: *mut u8,
    /// Length of `data` in bytes.
    pub data_len: usize,
    /// Signing metadata (per-signature results).
    pub meta: GfrSignMetadataC,
}

/// Result of a verification operation. Free with `gfr_crypto_free_verify_result`.
#[repr(C)]
#[derive(Zeroize, ZeroizeOnDrop)]
pub struct GfrVerifyResultC {
    /// Extracted plaintext bytes (for inline/clear-text modes), or null.
    #[zeroize(skip)]
    pub data: *mut u8,
    /// Length of `data` in bytes.
    pub data_len: usize,
    /// Verification metadata (signatures, is_verified flag).
    pub meta: GfrVerifyMetadataC,
}

/// Result of a combined encrypt-and-sign operation.
/// Free with `gfr_crypto_free_encrypt_and_sign_result`.
#[repr(C)]
#[derive(Zeroize, ZeroizeOnDrop)]
pub struct GfrEncryptAndSignResultC {
    /// Ciphertext bytes, or null for file-based operations.
    #[zeroize(skip)]
    pub data: *mut u8,
    /// Length of `data` in bytes.
    pub data_len: usize,
    /// Signing metadata (per-signature results).
    pub sign_meta: GfrSignMetadataC,
    /// Encryption metadata (invalid recipients).
    pub encrypt_meta: GfrEncryptMetadataC,
}

/// Result of a combined decrypt-and-verify operation.
/// Free with `gfr_crypto_free_decrypt_and_verify_result`.
#[repr(C)]
#[derive(Zeroize, ZeroizeOnDrop)]
pub struct GfrDecryptAndVerifyResultC {
    /// Decrypted plaintext bytes, or null for file-based operations.
    #[zeroize(skip)]
    pub data: *mut u8,
    /// Length of `data` in bytes.
    pub data_len: usize,
    /// Decryption metadata (filename, recipients).
    pub decrypt_meta: GfrDecryptMetadataC,
    /// Verification metadata (signatures, is_verified flag).
    #[zeroize(skip)]
    pub verify_meta: GfrVerifyMetadataC,
}

/// Callback to fetch an armored public key block by fingerprint.
///
/// The implementation must return a heap-allocated C string containing the
/// armored public key, or null if the key is not found. The returned pointer
/// is freed by the engine via the accompanying `GfrFreeCb`.
pub type GfrPublicKeyFetchCb =
    extern "C" fn(issuer_fpr: *const c_char, user_data: *mut c_void) -> *mut c_char;

/// Callback to fetch an armored secret key block by key ID.
///
/// The implementation must return a heap-allocated C string, or null if the
/// key is not found. Freed by the engine via the accompanying `GfrFreeCb`.
pub type GfrSecretKeyFetchCb = extern "C" fn(
    key_id: *const std::os::raw::c_char,
    user_data: *mut std::ffi::c_void,
) -> *mut std::os::raw::c_char;

/// State passed to a passphrase fetch callback describing the request context.
#[repr(C)]
#[derive(Zeroize, ZeroizeOnDrop)]
pub struct GfrPassphraseState {
    /// Fingerprint of the key the passphrase is needed for; may be null.
    #[zeroize(skip)]
    pub fpr: *mut c_char,
    /// Additional informational text to display in the dialog.
    #[zeroize(skip)]
    pub info: *mut c_char,
    /// True if this is a retry after an incorrect passphrase.
    pub retry: bool,
    /// True if the user should enter a new passphrase (not unlock an existing key).
    pub ask_for_new: bool,
    /// True if the user should confirm the passphrase (e.g. when setting a new one).
    pub should_confirm: bool,
}

/// Outcome of a passphrase fetch callback, reported via its `out_status`
/// out-parameter so the engine can tell a deliberate user cancellation apart
/// from a fetch failure (timeout, missing provider, internal error). The byte
/// length is still carried by the callback's return value.
#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum GfrPasswordFetchStatus {
    /// A passphrase was provided; `*out_pwd` holds it and the return value is
    /// its length.
    Provided = 0,
    /// The user explicitly declined (Cancel button or closing the dialog).
    /// Mapped to `GfrStatus::ErrorCanceled`.
    Cancelled = 1,
    /// No passphrase could be obtained for any other reason (input timeout,
    /// no provider registered, internal error). Mapped to
    /// `GfrStatus::ErrorFetchPasswordFailed`.
    Failed = 2,
}

/// Callback to prompt the user for a passphrase.
///
/// `channel` identifies the OpenPGP context requesting the passphrase.
/// `state` describes the request (fingerprint, retry flag, etc.).
/// On success the implementation writes a heap-allocated UTF-8 byte buffer to
/// `*out_pwd`, sets `*out_status` to `Provided`, and returns the buffer length
/// as a positive byte count. Otherwise it sets `*out_status` to `Cancelled` or
/// `Failed` (leaving `*out_pwd` untouched) and returns a value `<= 0`. The
/// engine copies the buffer and then frees it via the host secure-free routine.
pub type GfrPasswordFetchCb = extern "C" fn(
    channel: i32,
    state: GfrPassphraseState,
    out_pwd: *mut *mut u8,
    out_status: *mut GfrPasswordFetchStatus,
    user_data: *mut c_void,
) -> i32;

#[cfg(test)]
mod abi_tests {
    //! The types in this module are the ABI contract with the C++ core: every
    //! one has a mirror declaration in `src/core/GFCoreRust.h`, generated by
    //! cbindgen. Reordering a variant or changing a discriminant silently
    //! reinterprets values on the other side of the boundary, so the
    //! discriminants are pinned here as explicit integers.
    //!
    //! Several also carry RFC 9580 meaning (revocation reason codes §5.2.3.31,
    //! key versions §5.5.2, signature modes §5.2.1), and those values are fixed
    //! by the standard rather than by us.

    use super::*;
    use std::mem::{align_of, size_of};

    // -- GfrStatus ---------------------------------------------------------

    #[test]
    fn status_success_is_zero() {
        // The whole FFI contract is "0 means success"; every caller in the C++
        // core tests `!= 0` rather than matching variants.
        assert_eq!(GfrStatus::Success as i32, 0);
    }

    #[test]
    fn every_error_status_is_negative() {
        for status in ALL_STATUSES {
            if status == GfrStatus::Success {
                continue;
            }
            assert!(
                (status as i32) < 0,
                "{status:?} must be negative so `< 0` reliably means failure"
            );
        }
    }

    /// Every status value, pinned. Adding a variant means adding it here; that
    /// is deliberate friction, because the C++ side has a matching switch.
    const ALL_STATUSES: [GfrStatus; 27] = [
        GfrStatus::Success,
        GfrStatus::ErrorInvalidInput,
        GfrStatus::ErrorInternal,
        GfrStatus::ErrorPanic,
        GfrStatus::ErrorKeygenFailed,
        GfrStatus::ErrorPasswordFailed,
        GfrStatus::ErrorNoKey,
        GfrStatus::ErrorFetchPasswordFailed,
        GfrStatus::ErrorWrongPassword,
        GfrStatus::ErrorPasswordMismatch,
        GfrStatus::ErrorArmorFailed,
        GfrStatus::ErrorInvalidData,
        GfrStatus::ErrorDataCorrupted,
        GfrStatus::ErrorUnsupportedAlgorithm,
        GfrStatus::ErrorIo,
        GfrStatus::ErrorDecryptionFailed,
        GfrStatus::ErrorCanceled,
        GfrStatus::ErrorNoPublicKey,
        GfrStatus::ErrorNoSecretKey,
        GfrStatus::ErrorBadSignature,
        GfrStatus::ErrorExpiredKey,
        GfrStatus::ErrorRevokedKey,
        GfrStatus::ErrorUnusableKey,
        GfrStatus::ErrorUnsupportedFeature,
        GfrStatus::ErrorNoData,
        GfrStatus::ErrorBadArmor,
        GfrStatus::ErrorBadPassphrase,
    ];

    #[test]
    fn status_discriminants_are_stable() {
        // Pinned against the generated header. A mismatch here means the C++
        // core would misinterpret an error code.
        assert_eq!(GfrStatus::ErrorInvalidInput as i32, -1);
        assert_eq!(GfrStatus::ErrorKeygenFailed as i32, -2);
        assert_eq!(GfrStatus::ErrorPasswordFailed as i32, -3);
        assert_eq!(GfrStatus::ErrorArmorFailed as i32, -4);
        assert_eq!(GfrStatus::ErrorInternal as i32, -5);
        assert_eq!(GfrStatus::ErrorNoKey as i32, -6);
        assert_eq!(GfrStatus::ErrorInvalidData as i32, -7);
        assert_eq!(GfrStatus::ErrorDecryptionFailed as i32, -8);
        assert_eq!(GfrStatus::ErrorFetchPasswordFailed as i32, -9);
        assert_eq!(GfrStatus::ErrorWrongPassword as i32, -10);
    }

    #[test]
    fn status_discriminants_are_stable_continued() {
        assert_eq!(GfrStatus::ErrorDataCorrupted as i32, -11);
        assert_eq!(GfrStatus::ErrorUnsupportedAlgorithm as i32, -12);
        assert_eq!(GfrStatus::ErrorIo as i32, -13);
        assert_eq!(GfrStatus::ErrorPasswordMismatch as i32, -14);
        assert_eq!(GfrStatus::ErrorCanceled as i32, -15);
        assert_eq!(GfrStatus::ErrorNoPublicKey as i32, -16);
        assert_eq!(GfrStatus::ErrorNoSecretKey as i32, -17);
        assert_eq!(GfrStatus::ErrorBadSignature as i32, -18);
        assert_eq!(GfrStatus::ErrorExpiredKey as i32, -19);
        assert_eq!(GfrStatus::ErrorRevokedKey as i32, -20);
        assert_eq!(GfrStatus::ErrorUnusableKey as i32, -21);
        assert_eq!(GfrStatus::ErrorUnsupportedFeature as i32, -22);
        assert_eq!(GfrStatus::ErrorNoData as i32, -23);
        assert_eq!(GfrStatus::ErrorBadArmor as i32, -24);
        assert_eq!(GfrStatus::ErrorBadPassphrase as i32, -25);
        assert_eq!(GfrStatus::ErrorPanic as i32, -99);
    }

    #[test]
    fn status_codes_are_unique() {
        // Two variants sharing a discriminant would make the C++ switch
        // ambiguous and is accepted silently by rustc.
        let mut seen: Vec<i32> = ALL_STATUSES.iter().map(|s| *s as i32).collect();
        let total = seen.len();
        seen.sort_unstable();
        seen.dedup();
        assert_eq!(seen.len(), total, "duplicate GfrStatus discriminant");
    }

    #[test]
    fn status_display_renders_the_variant_name() {
        assert_eq!(GfrStatus::Success.to_string(), "Success");
        assert_eq!(GfrStatus::ErrorNoKey.to_string(), "ErrorNoKey");
        assert_eq!(GfrStatus::ErrorPanic.to_string(), "ErrorPanic");
    }

    #[test]
    fn status_implements_std_error() {
        fn as_error(e: GfrStatus) -> Box<dyn Error> {
            Box::new(e)
        }
        // `?` on a GfrStatus in an anyhow context relies on this impl.
        assert_eq!(as_error(GfrStatus::ErrorIo).to_string(), "ErrorIo");
    }

    #[test]
    fn status_is_copy_and_comparable() {
        let a = GfrStatus::ErrorNoKey;
        let b = a; // Copy, not a move.
        assert_eq!(a, b);
        assert_ne!(a, GfrStatus::Success);
        // Ord is derived; used for sorting in a couple of diagnostics paths.
        assert!(GfrStatus::Success > GfrStatus::ErrorInvalidInput);
    }

    #[test]
    fn status_round_trips_through_i32() {
        for status in ALL_STATUSES {
            let raw = status as i32;
            let back = ALL_STATUSES
                .iter()
                .copied()
                .find(|s| *s as i32 == raw)
                .expect("every discriminant maps back to exactly one variant");
            assert_eq!(back, status);
        }
    }

    #[test]
    fn status_is_c_int_sized() {
        // cbindgen renders these as a C enum; the C++ side stores them in an
        // `int`-sized field.
        assert_eq!(size_of::<GfrStatus>(), size_of::<i32>());
        assert_eq!(align_of::<GfrStatus>(), align_of::<i32>());
    }

    // -- GfrBuffer ---------------------------------------------------------

    #[test]
    fn empty_buffer_has_null_pointer_and_zero_length() {
        let b = GfrBuffer::empty();
        assert!(b.data.is_null());
        assert_eq!(b.len, 0);
    }

    #[test]
    fn empty_buffer_is_empty() {
        assert!(GfrBuffer::empty().is_empty());
    }

    #[test]
    fn null_pointer_is_empty_regardless_of_length() {
        // A caller that sets a length but forgets the pointer must not make the
        // engine dereference null.
        let b = GfrBuffer {
            data: std::ptr::null(),
            len: 32,
        };
        assert!(b.is_empty());
    }

    #[test]
    fn zero_length_is_empty_regardless_of_pointer() {
        let data = [1u8, 2, 3];
        let b = GfrBuffer {
            data: data.as_ptr(),
            len: 0,
        };
        assert!(b.is_empty());
    }

    #[test]
    fn populated_buffer_is_not_empty() {
        let data = [1u8, 2, 3];
        let b = GfrBuffer {
            data: data.as_ptr(),
            len: data.len(),
        };
        assert!(!b.is_empty());
    }

    #[test]
    fn as_slice_is_none_for_a_null_pointer() {
        let b = GfrBuffer::empty();
        assert!(unsafe { b.as_slice() }.is_none());
    }

    #[test]
    fn as_slice_returns_exactly_len_bytes() {
        let data = [10u8, 20, 30, 40];
        let b = GfrBuffer {
            data: data.as_ptr(),
            len: 3,
        };
        // Deliberately shorter than the backing array: the length field, not
        // the allocation, defines the slice.
        assert_eq!(unsafe { b.as_slice() }.unwrap(), &[10, 20, 30]);
    }

    #[test]
    fn as_slice_with_zero_length_yields_an_empty_slice() {
        let data = [1u8];
        let b = GfrBuffer {
            data: data.as_ptr(),
            len: 0,
        };
        assert_eq!(unsafe { b.as_slice() }.unwrap(), &[] as &[u8]);
    }

    #[test]
    fn as_str_rejects_an_empty_buffer() {
        let empty = GfrBuffer::empty();
        assert_eq!(unsafe { empty.as_str() }, Err(GfrStatus::ErrorInvalidInput));
    }

    #[test]
    fn as_str_rejects_a_zero_length_buffer() {
        let data = [b'x'];
        let b = GfrBuffer {
            data: data.as_ptr(),
            len: 0,
        };
        assert_eq!(unsafe { b.as_str() }, Err(GfrStatus::ErrorInvalidInput));
    }

    #[test]
    fn as_str_accepts_ascii() {
        let data = b"hello";
        let b = GfrBuffer {
            data: data.as_ptr(),
            len: data.len(),
        };
        assert_eq!(unsafe { b.as_str() }.unwrap(), "hello");
    }

    #[test]
    fn as_str_accepts_multibyte_utf8() {
        // RFC 9580 §3.4: OpenPGP text is UTF-8, so user IDs routinely carry
        // non-ASCII.
        let data = "Käse 🧀 日本語".as_bytes();
        let b = GfrBuffer {
            data: data.as_ptr(),
            len: data.len(),
        };
        assert_eq!(unsafe { b.as_str() }.unwrap(), "Käse 🧀 日本語");
    }

    #[test]
    fn as_str_rejects_invalid_utf8() {
        // 0xC3 starts a two-byte sequence; 0x28 is not a valid continuation.
        let data = [0xC3u8, 0x28];
        let b = GfrBuffer {
            data: data.as_ptr(),
            len: data.len(),
        };
        assert_eq!(unsafe { b.as_str() }, Err(GfrStatus::ErrorInvalidInput));
    }

    #[test]
    fn as_str_rejects_a_lone_surrogate_encoding() {
        // CESU-8 style encoding of U+D800, which is not valid UTF-8.
        let data = [0xEDu8, 0xA0, 0x80];
        let b = GfrBuffer {
            data: data.as_ptr(),
            len: data.len(),
        };
        assert_eq!(unsafe { b.as_str() }, Err(GfrStatus::ErrorInvalidInput));
    }

    #[test]
    fn as_str_rejects_a_truncated_multibyte_sequence() {
        let full = "é".as_bytes();
        let b = GfrBuffer {
            data: full.as_ptr(),
            len: 1,
        };
        assert_eq!(unsafe { b.as_str() }, Err(GfrStatus::ErrorInvalidInput));
    }

    #[test]
    fn as_str_accepts_an_embedded_nul() {
        // Unlike a C string, GfrBuffer is length-delimited, so an interior NUL
        // is ordinary data rather than a terminator.
        let data = b"a\0b";
        let b = GfrBuffer {
            data: data.as_ptr(),
            len: data.len(),
        };
        assert_eq!(unsafe { b.as_str() }.unwrap(), "a\0b");
    }

    #[test]
    fn buffer_is_copy() {
        let data = [1u8, 2];
        let a = GfrBuffer {
            data: data.as_ptr(),
            len: 2,
        };
        let b = a;
        assert_eq!(a.len, b.len);
        assert_eq!(a.data, b.data);
    }

    #[test]
    fn buffer_round_trips_a_vec() {
        let v = vec![9u8, 8, 7, 6];
        let b = GfrBuffer {
            data: v.as_ptr(),
            len: v.len(),
        };
        assert_eq!(unsafe { b.as_slice() }.unwrap(), v.as_slice());
    }

    // -- GfrOpenPGPKeyVersion ---------------------------------------------

    #[test]
    fn key_version_discriminants_match_the_wire_versions() {
        // RFC 9580 §5.5.2: the version octet in a key packet is literally 4 or
        // 6, and this enum stores that octet.
        assert_eq!(GfrOpenPGPKeyVersion::Unknown as i32, 0);
        assert_eq!(GfrOpenPGPKeyVersion::V4 as i32, 4);
        assert_eq!(GfrOpenPGPKeyVersion::V6 as i32, 6);
    }

    #[test]
    fn key_version_has_no_v3_or_v5_variant() {
        // DEFERRED: RFC 9580 §5.5.2 also describes v3 keys (deprecated) and
        // notes v5 is unspecified. The engine maps both to `Unknown` rather
        // than rejecting them outright; see `key.rs::build_public_metadata`.
        // This test documents that choice so a future change is deliberate.
        assert_eq!(GfrOpenPGPKeyVersion::Unknown as i32, 0);
        assert_ne!(GfrOpenPGPKeyVersion::V4 as i32, 3);
        assert_ne!(GfrOpenPGPKeyVersion::V4 as i32, 5);
    }

    #[test]
    fn key_version_is_copy_and_comparable() {
        let a = GfrOpenPGPKeyVersion::V6;
        let b = a;
        assert_eq!(a, b);
        assert_ne!(a, GfrOpenPGPKeyVersion::V4);
    }

    #[test]
    fn key_version_zeroize_is_a_no_op_for_a_fieldless_enum() {
        // KNOWN BEHAVIOUR: `#[derive(Zeroize)]` on an enum zeroizes the *fields*
        // of each variant. Every variant here is a unit variant, so there is
        // nothing to zero and the value survives untouched. That is harmless --
        // a key version is not secret -- but it means the derive on these enums
        // is documentation rather than protection. Pinned so nobody later
        // assumes a `GfrKeyConfig::zeroize()` erased its algorithm choice.
        let mut v = GfrOpenPGPKeyVersion::V6;
        v.zeroize();
        assert_eq!(v, GfrOpenPGPKeyVersion::V6);
    }

    #[test]
    fn key_version_is_c_int_sized() {
        assert_eq!(size_of::<GfrOpenPGPKeyVersion>(), size_of::<i32>());
    }

    // -- GfrKeyAlgo --------------------------------------------------------

    const ALL_ALGOS: [GfrKeyAlgo; 26] = [
        GfrKeyAlgo::Unknown,
        GfrKeyAlgo::ED25519,
        GfrKeyAlgo::CV25519,
        GfrKeyAlgo::NISTP256,
        GfrKeyAlgo::NISTP384,
        GfrKeyAlgo::NISTP521,
        GfrKeyAlgo::BRAINPOOLP256,
        GfrKeyAlgo::BRAINPOOLP384,
        GfrKeyAlgo::BRAINPOOLP512,
        GfrKeyAlgo::ED448,
        GfrKeyAlgo::X448,
        GfrKeyAlgo::RSA1024,
        GfrKeyAlgo::RSA2048,
        GfrKeyAlgo::RSA3072,
        GfrKeyAlgo::RSA4096,
        GfrKeyAlgo::SECP256K1,
        GfrKeyAlgo::DSA1024,
        GfrKeyAlgo::DSA2048,
        GfrKeyAlgo::DSA3072,
        GfrKeyAlgo::ED25519LEGACY,
        GfrKeyAlgo::KYBER768X25519,
        GfrKeyAlgo::KYBER1024X448,
        GfrKeyAlgo::MLDSA65ED25519,
        GfrKeyAlgo::MLDSA87ED448,
        GfrKeyAlgo::SLHDSASHAKE128S,
        GfrKeyAlgo::SLHDSASHAKE128F,
    ];

    #[test]
    fn algo_unknown_is_zero() {
        // Zero-initialised C memory must decode as "unspecified", not as a
        // real algorithm.
        assert_eq!(GfrKeyAlgo::Unknown as i32, 0);
    }

    #[test]
    fn algo_discriminants_are_dense_and_ordered() {
        // The variants have no explicit discriminants, so they are 0..n in
        // declaration order. Inserting one in the middle would shift every
        // later value and silently remap algorithms on the C++ side.
        for (i, algo) in ALL_ALGOS.iter().enumerate() {
            assert_eq!(
                *algo as i32, i as i32,
                "{algo:?} moved to discriminant {}",
                *algo as i32
            );
        }
    }

    #[test]
    fn algo_pins_the_boundaries_of_each_family() {
        assert_eq!(GfrKeyAlgo::ED25519 as i32, 1);
        assert_eq!(GfrKeyAlgo::RSA1024 as i32, 11);
        assert_eq!(GfrKeyAlgo::SECP256K1 as i32, 15);
        assert_eq!(GfrKeyAlgo::DSA1024 as i32, 16);
        assert_eq!(GfrKeyAlgo::ED25519LEGACY as i32, 19);
        assert_eq!(GfrKeyAlgo::KYBER768X25519 as i32, 20);
    }

    #[test]
    fn algo_zeroize_is_a_no_op_for_a_fieldless_enum() {
        // Same as `key_version_zeroize_is_a_no_op_for_a_fieldless_enum`: unit
        // variants carry no fields for the derive to clear. Not a secrecy
        // problem (the algorithm is public), but pinned so the limitation is
        // visible rather than assumed away.
        let mut a = GfrKeyAlgo::RSA4096;
        a.zeroize();
        assert_eq!(a, GfrKeyAlgo::RSA4096);
    }

    #[test]
    fn algo_all_variants_are_distinct() {
        let mut seen: Vec<i32> = ALL_ALGOS.iter().map(|a| *a as i32).collect();
        let total = seen.len();
        seen.sort_unstable();
        seen.dedup();
        assert_eq!(seen.len(), total, "duplicate GfrKeyAlgo discriminant");
    }

    // -- GfrSignMode -------------------------------------------------------

    #[test]
    fn sign_mode_discriminants_are_stable() {
        assert_eq!(GfrSignMode::Inline as i32, 0);
        assert_eq!(GfrSignMode::ClearText as i32, 1);
        assert_eq!(GfrSignMode::Detached as i32, 2);
    }

    #[test]
    fn sign_mode_zeroize_is_a_no_op_for_a_fieldless_enum() {
        let mut m = GfrSignMode::Detached;
        m.zeroize();
        assert_eq!(m, GfrSignMode::Detached);
    }

    #[test]
    fn sign_mode_covers_every_rfc9580_signing_shape() {
        // RFC 9580 offers exactly three ways to attach a signature to data:
        // inline/one-pass (§5.4), the Cleartext Signature Framework (§7), and
        // detached (§10.4).
        let modes = [
            GfrSignMode::Inline,
            GfrSignMode::ClearText,
            GfrSignMode::Detached,
        ];
        let mut raw: Vec<i32> = modes.iter().map(|m| *m as i32).collect();
        raw.sort_unstable();
        assert_eq!(raw, vec![0, 1, 2]);
    }

    // -- GfrRevocationCode -------------------------------------------------

    #[test]
    fn revocation_code_no_reason_is_zero() {
        // RFC 9580 §5.2.3.31 Table 10 fixes these octets; they go on the wire
        // verbatim in the Reason for Revocation subpacket.
        assert_eq!(GfrRevocationCode::NoReason as i32, 0);
    }

    #[test]
    fn revocation_code_superseded_is_one() {
        assert_eq!(GfrRevocationCode::Superseded as i32, 1);
    }

    #[test]
    fn revocation_code_compromised_is_two() {
        assert_eq!(GfrRevocationCode::Compromised as i32, 2);
    }

    #[test]
    fn revocation_code_retired_is_three() {
        assert_eq!(GfrRevocationCode::Retired as i32, 3);
    }

    #[test]
    fn revocation_code_user_id_invalid_is_thirty_two() {
        // The one non-contiguous value in Table 10; a dense enum would get it
        // wrong.
        assert_eq!(GfrRevocationCode::UserIdInvalid as i32, 32);
    }

    // -- signature / recipient / passphrase status enums -------------------

    #[test]
    fn signature_status_discriminants_are_stable() {
        // `Valid` is 0 so the C++ side can treat it as the success case.
        assert_eq!(GfrSignatureStatus::Valid as i32, 0);
        assert_eq!(GfrSignatureStatus::BadSignature as i32, 1);
        assert_eq!(GfrSignatureStatus::NoKey as i32, 2);
        assert_eq!(GfrSignatureStatus::UnknownError as i32, 3);
    }

    #[test]
    fn signature_status_distinguishes_no_key_from_bad_signature() {
        // A distinction the verify path depends on: an unknown signer must
        // never be reported as a forgery.
        assert_ne!(GfrSignatureStatus::NoKey, GfrSignatureStatus::BadSignature);
    }

    #[test]
    fn recipient_status_discriminants_are_stable() {
        assert_eq!(GfrRecipientStatus::Success as i32, 0);
        assert_eq!(GfrRecipientStatus::NoKey as i32, 1);
        assert_eq!(GfrRecipientStatus::Error as i32, 2);
    }

    #[test]
    fn password_fetch_status_discriminants_are_stable() {
        // `Cancelled` must stay distinct from `Failed`: the engine maps the
        // former to ErrorCanceled (a deliberate user action) and the latter to
        // ErrorFetchPasswordFailed.
        assert_eq!(GfrPasswordFetchStatus::Provided as i32, 0);
        assert_eq!(GfrPasswordFetchStatus::Cancelled as i32, 1);
        assert_eq!(GfrPasswordFetchStatus::Failed as i32, 2);
    }

    // -- struct layout -----------------------------------------------------

    #[test]
    fn pointer_fields_are_pointer_sized() {
        assert_eq!(size_of::<*mut c_char>(), size_of::<usize>());
        assert_eq!(size_of::<GfrBuffer>(), 2 * size_of::<usize>());
    }

    #[test]
    fn result_structs_are_pointer_aligned() {
        // Every *C struct is handed to C++ by pointer; misalignment would be a
        // portability bug on stricter targets.
        assert_eq!(align_of::<GfrKeyGenerateResult>(), align_of::<*mut c_char>());
        assert_eq!(align_of::<GfrRecipientResultC>(), align_of::<*mut c_char>());
        assert_eq!(align_of::<GfrSignatureResultC>(), align_of::<*mut c_char>());
        assert_eq!(align_of::<GfrInvalidRecipientC>(), align_of::<*mut c_char>());
    }

    #[test]
    fn key_generate_result_is_three_pointers() {
        assert_eq!(size_of::<GfrKeyGenerateResult>(), 3 * size_of::<*mut c_char>());
    }

    #[test]
    fn passphrase_state_carries_two_strings_and_three_flags() {
        let s = GfrPassphraseState {
            fpr: std::ptr::null_mut(),
            info: std::ptr::null_mut(),
            retry: true,
            ask_for_new: false,
            should_confirm: true,
        };
        assert!(s.retry && !s.ask_for_new && s.should_confirm);
        assert!(size_of::<GfrPassphraseState>() >= 2 * size_of::<*mut c_char>() + 3);
    }

    #[test]
    fn callback_typedefs_are_plain_function_pointers() {
        // Not `Option<fn>`: the C++ side must always supply these, so there is
        // no null case to handle. Being pointer-sized is what makes them
        // ABI-compatible with a C function pointer.
        assert_eq!(size_of::<GfrPublicKeyFetchCb>(), size_of::<*const c_void>());
        assert_eq!(size_of::<GfrSecretKeyFetchCb>(), size_of::<*const c_void>());
        assert_eq!(size_of::<GfrPasswordFetchCb>(), size_of::<*const c_void>());
    }

    #[test]
    fn optional_callback_is_null_pointer_optimised() {
        // `Option<GfrPasswordFetchCb>` is how the engine models "no callback";
        // the niche optimisation keeps it ABI-compatible with a nullable C
        // function pointer.
        assert_eq!(
            size_of::<Option<GfrPasswordFetchCb>>(),
            size_of::<GfrPasswordFetchCb>()
        );
    }
}
