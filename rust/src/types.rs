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

/// Callback to prompt the user for a passphrase.
///
/// `channel` identifies the OpenPGP context requesting the passphrase.
/// `state` describes the request (fingerprint, retry flag, etc.).
/// On success the implementation writes a heap-allocated UTF-8 byte buffer to
/// `*out_pwd` and returns its length as a positive byte count; on cancellation
/// or error it returns a value `<= 0` (and `*out_pwd` is ignored). The engine
/// copies the buffer and then frees it via the host secure-free routine.
pub type GfrPasswordFetchCb = extern "C" fn(
    channel: i32,
    state: GfrPassphraseState,
    out_pwd: *mut *mut u8,
    user_data: *mut c_void,
) -> i32;
