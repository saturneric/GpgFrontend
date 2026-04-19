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

use core::fmt;
use std::{error::Error, ffi::c_void, os::raw::c_char};

#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord)]
pub enum GfrStatus {
    Success = 0,

    // --- General Errors ---
    ErrorInvalidInput = -1,
    ErrorInternal = -5,
    ErrorPanic = -99,

    // --- Key & Auth Errors ---
    ErrorKeygenFailed = -2,
    ErrorPasswordFailed = -3,
    ErrorNoKey = -6,
    ErrorFetchPasswordFailed = -9,
    ErrorWrongPassword = -10, // Specifically for incorrect passphrase

    // --- Data & Parsing Errors ---
    ErrorArmorFailed = -4,
    ErrorInvalidData = -7,           // General parsing error
    ErrorDataCorrupted = -11,        // Packet checksum failed / Tampered data
    ErrorUnsupportedAlgorithm = -12, // e.g., IDEA or old deprecated algos

    // --- IO & Execution Errors ---
    ErrorIo = -13, // File read/write failures
    ErrorDecryptionFailed = -8,
}

impl fmt::Display for GfrStatus {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        // Just print the enum variant name for simplicity
        write!(f, "{:?}", self)
    }
}

impl Error for GfrStatus {}

#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
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
}

#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum GfrSignMode {
    Inline = 0,
    ClearText = 1,
    Detached = 2,
}

#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub enum GfrRevocationCode {
    NoReason = 0,
    Superseded = 1,
    Compromised = 2,
    Retired = 3,
    UserIdInvalid = 32,
}

#[repr(C)]
pub struct GfrKeyConfig {
    pub algo: GfrKeyAlgo,
    pub can_sign: bool,
    pub can_encrypt: bool,
    pub can_auth: bool,
    pub has_passphrase: bool,
}

#[repr(C)]
pub struct GfrKeyGenerateResult {
    pub secret_key: *mut c_char,
    pub public_key: *mut c_char,
    pub fingerprint: *mut c_char,
}

#[repr(C)]
pub struct GfrSubkeyMetadataC {
    pub fpr: *mut c_char,
    pub key_id: *mut c_char,
    pub algo: GfrKeyAlgo,
    pub key_length: u32,
    pub created_at: u32,
    pub has_secret: bool,
    pub is_revoked: bool,
    pub can_sign: bool,
    pub can_encrypt: bool,
    pub can_auth: bool,
    pub can_certify: bool,
}

#[repr(C)]
pub struct GfrUserIdC {
    pub user_id: *mut c_char,
    pub is_primary: bool,
    pub is_revoked: bool,
}

#[repr(C)]
pub struct GfrKeyMetadataC {
    pub fpr: *mut c_char,
    pub key_id: *mut c_char,

    // Array of strings to support multiple User IDs
    pub user_ids: *mut GfrUserIdC,
    pub user_id_count: usize,

    pub algo: GfrKeyAlgo,
    pub key_length: u32,
    pub created_at: u32,
    pub has_secret: bool,

    pub public_key_block: *mut std::os::raw::c_char,
    pub secret_key_block: *mut std::os::raw::c_char,

    pub can_sign: bool,
    pub can_encrypt: bool,
    pub can_auth: bool,
    pub can_certify: bool,

    pub subkeys: *mut GfrSubkeyMetadataC,
    pub subkey_count: usize,
}

/// Status of an individual signature
#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum GfrSignatureStatus {
    Valid = 0,        // The signature matches the payload and the public key
    BadSignature = 1, // The signature is mathematically invalid (payload tampered or wrong key)
    NoKey = 2,        // We don't have the public key required to verify this signature
    UnknownError = 3, // Other parsing or internal errors related to this signature
}

/// Verification result for a single signature found in the message
#[repr(C)]
pub struct GfrSignatureResultC {
    pub sig_type: GfrSignMode, // The type of signature (Inline, ClearText, Detached)
    pub issuer_fpr: *mut c_char, // The Fingerprint of the signer (if available, otherwise null)
    pub status: GfrSignatureStatus, // The verification status for this specific signature
    pub created_at: u32,       // Signature creation timestamp (Unix epoch)
    pub pub_algo: *mut c_char, // The algorithm used for this signature (if available)
    pub hash_algo: *mut c_char, // The hash algorithm used for this signature (if available)
}

#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum GfrRecipientStatus {
    Success = 0, // Successfully decrypted using this key
    NoKey = 1,   // Key ID found, but we don't have the secret key to unlock it
    Error = 2,   // We have the key, but decryption failed (e.g., wrong password)
}

#[repr(C)]
pub struct GfrRecipientResultC {
    pub key_id: *mut c_char,
    pub pub_algo: *mut c_char,
    pub status: GfrRecipientStatus,
}

#[repr(C)]
pub struct GfrInvalidRecipientC {
    pub fpr: *mut c_char,
    pub reason: GfrStatus,
}

// Metadata for signing operations (reusable)
#[repr(C)]
pub struct GfrSignMetadataC {
    pub signatures: *mut GfrSignatureResultC,
    pub signature_count: usize,
}

// Metadata for encryption operations (reusable)
#[repr(C)]
pub struct GfrEncryptMetadataC {
    pub invalid_recipients: *mut GfrInvalidRecipientC,
    pub invalid_recipient_count: usize,
}

#[repr(C)]
pub struct GfrDecryptMetadataC {
    pub filename: *mut c_char,
    pub recipients: *mut GfrRecipientResultC,
    pub recipient_count: usize,
}

#[repr(C)]
pub struct GfrVerifyMetadataC {
    // The list of signatures found and evaluated
    pub signatures: *mut GfrSignatureResultC,
    pub signature_count: usize,

    // Helper flag: true if AT LEAST ONE signature is perfectly Valid
    pub is_verified: bool,
}

// Result structure for the encryption operation
#[repr(C)]
pub struct GfrEncryptResultC {
    pub data: *mut u8,
    pub data_len: usize,
    pub meta: GfrEncryptMetadataC,
}

#[repr(C)]
pub struct GfrDecryptResultC {
    pub data: *mut u8,
    pub data_len: usize,
    pub meta: GfrDecryptMetadataC,
}

/// The comprehensive result of a signing operation
#[repr(C)]
pub struct GfrSignResultC {
    pub data: *mut u8,
    pub data_len: usize,
    pub meta: GfrSignMetadataC,
}

/// The comprehensive result of a verification operation
#[repr(C)]
pub struct GfrVerifyResultC {
    pub data: *mut u8,
    pub data_len: usize,
    pub meta: GfrVerifyMetadataC,
}

// Result for encrypting AND signing (The perfect composition)
#[repr(C)]
pub struct GfrEncryptAndSignResultC {
    pub data: *mut u8,
    pub data_len: usize,
    pub sign_meta: GfrSignMetadataC,
    pub encrypt_meta: GfrEncryptMetadataC,
}

#[repr(C)]
pub struct GfrDecryptAndVerifyResultC {
    pub data: *mut u8,
    pub data_len: usize,
    pub decrypt_meta: GfrDecryptMetadataC,
    pub verify_meta: GfrVerifyMetadataC,
}

// Callback to fetch a public key block by its Fingerprint.
// Returns a dynamically allocated C-string containing the armored key, or null if not found.
pub type GfrPublicKeyFetchCb =
    extern "C" fn(issuer_fpr: *const c_char, user_data: *mut c_void) -> *mut c_char;

// Callback to fetch a secret key block by its Key ID.
// Returns a dynamically allocated C-string containing the armored key, or null if not found.
pub type GfrSecretKeyFetchCb = extern "C" fn(
    key_id: *const std::os::raw::c_char,
    user_data: *mut std::ffi::c_void,
) -> *mut std::os::raw::c_char;

// Callback to free the memory allocated by the fetch callback.
pub type GfrFreeCb = extern "C" fn(ptr: *mut c_void, user_data: *mut c_void);

// Callback to fetch a password for a given key hint (e.g., fingerprint).
pub type GfrPasswordFetchCb = extern "C" fn(
    channel: i32,
    fpr: *const c_char,
    info: *const c_char,
    out_pwd: *mut *mut u8,
    user_data: *mut c_void,
) -> i32;
