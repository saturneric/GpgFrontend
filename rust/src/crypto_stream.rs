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

use crate::{
    cache::{PASSWORD_CACHE, PasswordCachePolicy},
    crypto::{
        InvalidRecipientInternal, RecipientResultInternal, SelectedKey, SignatureResultInternal,
        algo_to_string_simple, cert_contains_issuer, parse_signer_block, sniff_signatures,
        with_signing_key,
    },
    err::{IntoGfrResult, set_last_error},
    tar::build_tar_tempfile_from_directory,
    types::{
        GfrFreeCb, GfrPasswordFetchCb, GfrPublicKeyFetchCb, GfrRecipientStatus,
        GfrSecretKeyFetchCb, GfrSignMode, GfrSignatureStatus, GfrStatus,
    },
    utils::fetch_password_with_cache,
};
use core::fmt;
use log::debug;
use pgp::{
    composed::{
        ArmorOptions, CleartextSignedMessage, Deserializable, DetachedSignature, Esk, Message,
        MessageBuilder, SignedPublicKey, SignedSecretKey,
    },
    crypto::{hash::HashAlgorithm, sym::SymmetricKeyAlgorithm},
    ser::Serialize,
    types::{KeyDetails, Password, SecretParams, StringToKey},
};
use rand::thread_rng;
use std::{
    ffi::{CStr, CString, c_void},
    fs::File,
    io::{BufReader, Cursor, Read, Seek, SeekFrom, Write},
    path::Path,
};

pub struct EncryptStreamResultInternal {
    pub invalid_recipients: Vec<InvalidRecipientInternal>,
}

pub fn encrypt_stream_internal<R, W>(
    filename_hint: &str,
    input_stream: R,
    output_stream: W,
    public_key_blocks: &[&str],
    ascii_armor: bool,
) -> Result<EncryptStreamResultInternal, GfrStatus>
where
    R: Read + Send + Sync,
    W: Write + Send + Sync,
{
    // Delegate to the shared engine, passing empty arrays for signing
    let result = encrypt_and_sign_stream_internal(
        0, // Dummy channel
        filename_hint,
        input_stream,
        output_stream,
        public_key_blocks,
        &[], // Empty secret keys skips the signing phase!
        None,
        None,
        ascii_armor,
    )?;

    Ok(EncryptStreamResultInternal {
        invalid_recipients: result.invalid_recipients,
    })
}

fn create_output_file(out_file_path: &str) -> Result<File, GfrStatus> {
    File::create(out_file_path).map_err(|e| {
        log::error!("Failed to create output file: {}", e);
        set_last_error(&e.to_string());
        GfrStatus::ErrorIo
    })
}

type ParsedSigner = (SignedSecretKey, Option<String>);

fn parse_secret_signers(secret_key_blocks: &[&str]) -> Result<Vec<ParsedSigner>, GfrStatus> {
    let mut parsed_keys = Vec::with_capacity(secret_key_blocks.len());

    for block in secret_key_blocks {
        let (target, armor_block) = parse_signer_block(block);
        let (skey, _) =
            SignedSecretKey::from_string(armor_block).map_err(|_| GfrStatus::ErrorInvalidInput)?;
        parsed_keys.push((skey, target));
    }

    Ok(parsed_keys)
}

/// Package a directory as a Tar and encrypt it as a stream
pub fn encrypt_directory_internal(
    in_dir_path: &str,
    out_file_path: &str,
    public_key_blocks: &[&str],
    ascii_armor: bool,
) -> Result<crate::crypto_stream::EncryptStreamResultInternal, crate::types::GfrStatus> {
    let (temp_archive, filename_hint) = build_tar_tempfile_from_directory(in_dir_path)?;
    let out_file = create_output_file(out_file_path)?;

    log::info!("Encrypting tar archive...");
    crate::crypto_stream::encrypt_stream_internal(
        &filename_hint,
        temp_archive,
        out_file,
        public_key_blocks,
        ascii_armor,
    )
}

pub struct SignStreamResultInternal {
    pub signatures: Vec<SignatureResultInternal>,
}

pub fn sign_stream_internal<R, W>(
    channel: i32,
    name: &str,
    mut input_stream: R,
    mut output_stream: W,
    secret_key_blocks: &[&str],
    fetch_cb: Option<GfrPasswordFetchCb>,
    free_cb: Option<GfrFreeCb>,
    mode: GfrSignMode,
    ascii_armor: bool,
) -> Result<SignStreamResultInternal, GfrStatus>
where
    R: Read + Send + Sync,
    W: Write + Send + Sync,
{
    if secret_key_blocks.is_empty() {
        return Err(GfrStatus::ErrorInvalidInput);
    }

    // 1. Parse Keys and Extract Exact Targets
    let parsed_keys = parse_secret_signers(secret_key_blocks)?;

    let mut rng = thread_rng();
    let mut created_signatures = Vec::new();
    let current_time = std::time::SystemTime::now()
        .duration_since(std::time::UNIX_EPOCH)
        .unwrap()
        .as_secs() as u32;

    // Helper closure to record successful signatures
    let mut record_sig = |fpr: String, pub_algo: String| {
        created_signatures.push(SignatureResultInternal {
            fpr,
            status: GfrSignatureStatus::Valid,
            created_at: current_time,
            pub_algo,
            hash_algo: "SHA512".to_string(), // rpgp uses SHA512 by default in our builder
            sig_type: mode,
        });
    };

    // Helper closure to dynamically fetch passwords if the target key is locked
    let fetch_pwd_for_key = |is_encrypted: bool, fpr: &str| -> Result<Password, GfrStatus> {
        if is_encrypted {
            let pwd_bytes = fetch_password_with_cache(
                Some(&PASSWORD_CACHE),
                PasswordCachePolicy::Default,
                channel,
                fpr,
                "Signing",
                fetch_cb,
                free_cb,
            )?;
            Ok(Password::from(pwd_bytes.as_slice()))
        } else {
            Ok(Password::empty())
        }
    };

    // 2. Route the operation based on the selected mode
    match mode {
        // ---------------------------------------------------------
        // MODE 0: INLINE SIGNATURE (True Streaming)
        // ---------------------------------------------------------
        GfrSignMode::Inline => {
            // Hand over the stream directly to the builder for chunked processing
            let filename_bytes = name.as_bytes().to_vec();
            let mut builder = MessageBuilder::from_reader(filename_bytes, input_stream);
            let mut at_least_one_signer = false;

            for (skey, target) in &parsed_keys {
                with_signing_key(skey, target.as_deref(), |selected_key| {
                    let fpr = selected_key.fpr();
                    let is_enc = selected_key.is_encrypted();
                    let algo_str = algo_to_string_simple(selected_key.algorithm());
                    let pwd = fetch_pwd_for_key(is_enc, &fpr)?;

                    // Apply the signature to the streaming pipeline
                    match selected_key {
                        SelectedKey::Primary(k) => builder.sign(k, pwd, HashAlgorithm::Sha512),
                        SelectedKey::Sub(k) => builder.sign(k, pwd, HashAlgorithm::Sha512),
                    };

                    record_sig(fpr, algo_str);
                    at_least_one_signer = true;
                    Ok(())
                })?;
            }

            if !at_least_one_signer {
                return Err(GfrStatus::ErrorInvalidInput);
            }

            // Execute the pipeline: read from input, sign chunks, write to output
            let result = if ascii_armor {
                builder.to_armored_writer(&mut rng, ArmorOptions::default(), &mut output_stream)
            } else {
                builder.to_writer(&mut rng, &mut output_stream)
            };

            result.map_err(|_| GfrStatus::ErrorInternal)?;
            output_stream
                .flush()
                .map_err(|_| GfrStatus::ErrorInternal)?;

            Ok(SignStreamResultInternal {
                signatures: created_signatures,
            })
        }

        // ---------------------------------------------------------
        // MODE 1: CLEARTEXT SIGNATURE (Buffered)
        // ---------------------------------------------------------
        GfrSignMode::ClearText => {
            // rpgp strictly requires a &str for cleartext signing due to CRLF normalization.
            // We must buffer the stream into memory.
            let mut data = Vec::new();
            input_stream
                .read_to_end(&mut data)
                .map_err(|_| GfrStatus::ErrorInvalidInput)?;
            let text_str = std::str::from_utf8(&data).map_err(|_| GfrStatus::ErrorInvalidInput)?;

            for (skey, target) in &parsed_keys {
                let res = with_signing_key(skey, target.as_deref(), |selected_key| {
                    let fpr = selected_key.fpr();
                    let is_enc = selected_key.is_encrypted();
                    let algo_str = algo_to_string_simple(selected_key.algorithm());
                    let pwd = fetch_pwd_for_key(is_enc, &fpr)?;

                    let msg_res = match selected_key {
                        SelectedKey::Primary(k) => {
                            CleartextSignedMessage::sign(&mut rng, text_str, k, &pwd)
                        }
                        SelectedKey::Sub(k) => {
                            CleartextSignedMessage::sign(&mut rng, text_str, k, &pwd)
                        }
                    };

                    if let Ok(msg) = msg_res {
                        record_sig(fpr, algo_str);
                        let out = msg
                            .to_armored_string(ArmorOptions::default())
                            .map_err(|_| GfrStatus::ErrorArmorFailed)?
                            .into_bytes();
                        return Ok(out);
                    }
                    Err(GfrStatus::ErrorInternal)
                });

                if let Ok(out_data) = res {
                    output_stream
                        .write_all(&out_data)
                        .map_err(|_| GfrStatus::ErrorInternal)?;
                    output_stream
                        .flush()
                        .map_err(|_| GfrStatus::ErrorInternal)?;

                    return Ok(SignStreamResultInternal {
                        signatures: created_signatures,
                    });
                }
            }

            Err(GfrStatus::ErrorInvalidInput)
        }

        // ---------------------------------------------------------
        // MODE 2: DETACHED SIGNATURE (Buffered)
        // ---------------------------------------------------------
        GfrSignMode::Detached => {
            // rpgp's standard detached signature API requires &[u8].
            // Buffering the payload to memory to compute the hash.
            let mut data = Vec::new();
            input_stream
                .read_to_end(&mut data)
                .map_err(|_| GfrStatus::ErrorInvalidInput)?;

            for (skey, target) in &parsed_keys {
                let res = with_signing_key(skey, target.as_deref(), |selected_key| {
                    let fpr = selected_key.fpr();
                    let is_enc = selected_key.is_encrypted();
                    let algo_str = algo_to_string_simple(selected_key.algorithm());
                    let pwd = fetch_pwd_for_key(is_enc, &fpr)?;

                    let sig_res = match selected_key {
                        SelectedKey::Primary(k) => DetachedSignature::sign_binary_data(
                            &mut rng,
                            k,
                            &pwd,
                            HashAlgorithm::Sha512,
                            &*data,
                        ),
                        SelectedKey::Sub(k) => DetachedSignature::sign_binary_data(
                            &mut rng,
                            k,
                            &pwd,
                            HashAlgorithm::Sha512,
                            &*data,
                        ),
                    };

                    if let Ok(sig) = sig_res {
                        record_sig(fpr, algo_str);
                        let out = if ascii_armor {
                            sig.to_armored_bytes(None.into())
                                .map_err(|_| GfrStatus::ErrorArmorFailed)?
                        } else {
                            sig.to_bytes().map_err(|_| GfrStatus::ErrorInternal)?
                        };
                        return Ok(out);
                    }
                    Err(GfrStatus::ErrorInternal)
                });

                if let Ok(out_data) = res {
                    output_stream
                        .write_all(&out_data)
                        .map_err(|_| GfrStatus::ErrorInternal)?;
                    output_stream
                        .flush()
                        .map_err(|_| GfrStatus::ErrorInternal)?;

                    return Ok(SignStreamResultInternal {
                        signatures: created_signatures,
                    });
                }
            }

            Err(GfrStatus::ErrorInvalidInput)
        }
    }
}

pub struct VerifyStreamResultInternal {
    pub is_verified: bool,
    pub signatures: Vec<SignatureResultInternal>,
}

pub fn verify_detached_stream_internal<R>(
    mut data_stream: R,
    sig_data: &[u8],
    fetch_pubkey_cb: Option<GfrPublicKeyFetchCb>,
    free_cb: Option<GfrFreeCb>,
    user_data: *mut std::ffi::c_void,
) -> Result<VerifyStreamResultInternal, GfrStatus>
where
    R: Read + Seek + Send + Sync,
{
    if sig_data.is_empty() {
        return Err(GfrStatus::ErrorInvalidInput);
    }

    // 1. Parse the detached signature file (Attempt armored first, fallback to raw bytes)
    let sig_msg = DetachedSignature::from_armor_single(Cursor::new(sig_data))
        .map(|(s, _)| s)
        .or_else(|_| DetachedSignature::from_bytes(sig_data))
        .map_err(|_| GfrStatus::ErrorInvalidInput)?;

    // 2. Sniff signature metadata to know WHO signed it before fetching keys
    let mut signatures = sniff_signatures(sig_data, GfrSignMode::Detached);
    let mut is_verified = false;

    // 3. Dynamically Fetch Public Keys via C Callback based on sniffed fingerprints
    let mut certs = Vec::new();
    if let Some(cb) = fetch_pubkey_cb {
        for sig in &signatures {
            let c_fpr = std::ffi::CString::new(sig.fpr.clone()).unwrap_or_default();
            let c_key_block = cb(c_fpr.as_ptr(), user_data);

            if !c_key_block.is_null() {
                if let Ok(key_str) = unsafe { std::ffi::CStr::from_ptr(c_key_block) }.to_str() {
                    if let Ok((cert, _)) = SignedPublicKey::from_string(key_str) {
                        certs.push(cert);
                    }
                }
                if let Some(f_cb) = free_cb {
                    f_cb(c_key_block as *mut std::ffi::c_void, user_data);
                }
            }
        }
    }

    log::debug!(
        "Fetched and parsed {} public keys for detached stream verification",
        certs.len()
    );

    // Helper closure to update signature statuses uniformly
    let update_signatures = |cert: &SignedPublicKey,
                             is_cert_valid: bool,
                             signatures: &mut Vec<SignatureResultInternal>,
                             is_verified: &mut bool|
     -> bool {
        let mut found = false;
        for sig in signatures.iter_mut() {
            if cert_contains_issuer(cert, &sig.fpr) {
                found = true;
                if is_cert_valid {
                    sig.status = GfrSignatureStatus::Valid;
                    *is_verified = true;
                } else if sig.status == GfrSignatureStatus::NoKey {
                    // Only mark as bad if we haven't already validated it via another key
                    sig.status = GfrSignatureStatus::BadSignature;
                }
            }
        }
        found
    };

    // 4. Stream Verification
    for cert in &certs {
        // Rewind the stream to the beginning before starting verification
        data_stream
            .seek(SeekFrom::Start(0))
            .map_err(|_| GfrStatus::ErrorInternal)?;

        // Try verifying with the primary key first
        let mut is_cert_valid = sig_msg.signature.verify(cert, &mut data_stream).is_ok();

        // Fallback: If primary key fails, test subkeys (to bypass rpgp's strict identity matching)
        if !is_cert_valid {
            for subkey in &cert.public_subkeys {
                // We MUST rewind the stream before each subsequent verification attempt!
                data_stream
                    .seek(SeekFrom::Start(0))
                    .map_err(|_| GfrStatus::ErrorInternal)?;

                if sig_msg.signature.verify(subkey, &mut data_stream).is_ok() {
                    is_cert_valid = true;
                    break;
                }
            }
        }

        update_signatures(cert, is_cert_valid, &mut signatures, &mut is_verified);
    }

    Ok(VerifyStreamResultInternal {
        is_verified,
        signatures,
    })
}

pub struct EncryptAndSignStreamResultInternal {
    pub signatures: Vec<SignatureResultInternal>,
    pub invalid_recipients: Vec<InvalidRecipientInternal>,
}

pub fn encrypt_and_sign_stream_internal<R, W>(
    channel: i32,
    name: &str,
    input_stream: R,
    mut output_stream: W,
    public_key_blocks: &[&str],
    secret_key_blocks: &[&str], // If empty, performs ENCRYPT ONLY
    fetch_cb: Option<GfrPasswordFetchCb>,
    free_cb: Option<GfrFreeCb>,
    ascii_armor: bool,
) -> Result<EncryptAndSignStreamResultInternal, GfrStatus>
where
    R: Read + Send + Sync,
    W: Write + Send + Sync,
{
    let mut rng = thread_rng();
    let filename_bytes = name.as_bytes().to_vec();

    let parsed_skeys = parse_secret_signers(secret_key_blocks)?;

    let mut builder = MessageBuilder::from_reader(filename_bytes, input_stream);
    builder.partial_chunk_size(512 * 1024).into_gfr()?; // Set chunk size to 512KB for better performance on large files

    let mut created_signatures = Vec::new();

    // 3. Process signing if secret keys are provided (if empty, skip signing)
    if !parsed_skeys.is_empty() {
        let current_time = std::time::SystemTime::now()
            .duration_since(std::time::UNIX_EPOCH)
            .unwrap()
            .as_secs() as u32;

        let fetch_pwd_for_key = |is_encrypted: bool, fpr: &str| -> Result<Password, GfrStatus> {
            if is_encrypted {
                let pwd_bytes = fetch_password_with_cache(
                    Some(&PASSWORD_CACHE),
                    PasswordCachePolicy::Default,
                    channel,
                    fpr,
                    "Signing",
                    fetch_cb,
                    free_cb,
                )?;
                Ok(Password::from(pwd_bytes.as_slice()))
            } else {
                debug!("Target secret key is unlocked. Bypassing password callback for signing.");
                Ok(Password::empty())
            }
        };

        let mut at_least_one_signer = false;

        // Iterate over all parsed secret keys and apply signatures to the builder
        for (skey, target) in &parsed_skeys {
            with_signing_key(skey, target.as_deref(), |selected_key| {
                let fpr = selected_key.fpr();
                let is_enc = selected_key.is_encrypted();
                let algo_str = algo_to_string_simple(selected_key.algorithm());
                let pwd = fetch_pwd_for_key(is_enc, &fpr)?;

                // rpgp's builder API will handle the streaming signing
                // internally, we just need to call sign() for each key
                match selected_key {
                    SelectedKey::Primary(k) => builder.sign(k, pwd, HashAlgorithm::Sha512),
                    SelectedKey::Sub(k) => builder.sign(k, pwd, HashAlgorithm::Sha512),
                };

                created_signatures.push(SignatureResultInternal {
                    fpr,
                    status: GfrSignatureStatus::Valid,
                    created_at: current_time,
                    pub_algo: algo_str,
                    hash_algo: "SHA512".to_string(),
                    sig_type: GfrSignMode::Inline,
                });
                at_least_one_signer = true;
                Ok(())
            })?;
        }

        if !at_least_one_signer {
            return Err(GfrStatus::ErrorInvalidInput);
        }
    }

    let mut enc_builder = builder.seipd_v1(&mut rng, SymmetricKeyAlgorithm::AES256);

    let mut has_recipient = false;
    let mut invalid_recipients = Vec::new();

    for block in public_key_blocks {
        match SignedPublicKey::from_string(block) {
            Ok((cert, _)) => {
                let mut added_for_this_cert = false;
                let fpr = cert.primary_key.fingerprint().to_string();

                for subkey in &cert.public_subkeys {
                    if subkey.key.algorithm().can_encrypt() {
                        if enc_builder.encrypt_to_key(&mut rng, subkey).is_ok() {
                            added_for_this_cert = true;
                            has_recipient = true;
                            break;
                        }
                    }
                }

                if !added_for_this_cert && cert.primary_key.algorithm().can_encrypt() {
                    if enc_builder
                        .encrypt_to_key(&mut rng, &cert.primary_key)
                        .is_ok()
                    {
                        added_for_this_cert = true;
                        has_recipient = true;
                    }
                }

                if !added_for_this_cert {
                    invalid_recipients.push(InvalidRecipientInternal {
                        fpr,
                        reason: GfrStatus::ErrorNoKey,
                    });
                }
            }
            Err(_) => {
                invalid_recipients.push(InvalidRecipientInternal {
                    fpr: String::from("Unknown"),
                    reason: GfrStatus::ErrorInvalidData,
                });
            }
        }
    }

    if !has_recipient {
        return Err(GfrStatus::ErrorInvalidInput);
    }

    let result = if ascii_armor {
        enc_builder.to_armored_writer(&mut rng, ArmorOptions::default(), &mut output_stream)
    } else {
        enc_builder.to_writer(&mut rng, &mut output_stream)
    };

    result.map_err(|_| GfrStatus::ErrorInternal)?;
    output_stream
        .flush()
        .map_err(|_| GfrStatus::ErrorInternal)?;

    Ok(EncryptAndSignStreamResultInternal {
        signatures: created_signatures,
        invalid_recipients,
    })
}

pub fn encrypt_and_sign_directory_internal(
    channel: i32,
    in_dir_path: &str,
    out_file_path: &str,
    public_key_blocks: &[&str],
    secret_key_blocks: &[&str],
    fetch_pwd_cb: Option<crate::types::GfrPasswordFetchCb>,
    free_cb: Option<crate::types::GfrFreeCb>,
    ascii_armor: bool,
) -> Result<crate::crypto::EncryptAndSignResultInternal, crate::types::GfrStatus> {
    let (temp_archive, filename_hint) = build_tar_tempfile_from_directory(in_dir_path)?;
    let out_file = create_output_file(out_file_path)?;

    log::info!("Encrypting and signing tar archive...");
    let stream_result = crate::crypto_stream::encrypt_and_sign_stream_internal(
        channel,
        &filename_hint,
        temp_archive,
        out_file,
        public_key_blocks,
        secret_key_blocks,
        fetch_pwd_cb,
        free_cb,
        ascii_armor,
    )?;

    Ok(crate::crypto::EncryptAndSignResultInternal {
        data: Vec::new(),
        signatures: stream_result.signatures,
        invalid_recipients: stream_result.invalid_recipients,
    })
}

pub struct DecryptAndVerifyStreamResultInternal {
    pub filename: String,
    pub recipients: Vec<RecipientResultInternal>,
    pub is_verified: bool,
    pub signatures: Vec<SignatureResultInternal>,
}

fn analyze_encrypted_envelope(
    parsed_message: &Message,
) -> Result<(bool, bool, Vec<RecipientResultInternal>), GfrStatus> {
    let mut has_pkesk = false;
    let mut has_skesk = false;
    let mut recipients = Vec::new();

    if let Message::Encrypted { esk, .. } = parsed_message {
        for e in esk {
            match e {
                Esk::PublicKeyEncryptedSessionKey(pkesk) => {
                    has_pkesk = true;

                    if let Ok(id) = pkesk.id() {
                        let algo = pkesk
                            .algorithm()
                            .map(algo_to_string_simple)
                            .unwrap_or_default();

                        recipients.push(RecipientResultInternal {
                            key_id: id.to_string(),
                            pub_algo: algo,
                            status: GfrRecipientStatus::NoKey,
                        });
                    }
                }
                Esk::SymKeyEncryptedSessionKey(_) => {
                    has_skesk = true;
                }
            }
        }

        return Ok((has_pkesk, has_skesk, recipients));
    }

    Err(GfrStatus::ErrorInvalidData)
}

fn decrypt_message_with_password(
    channel: i32,
    parsed_message: Message,
    fetch_pwd_cb: Option<GfrPasswordFetchCb>,
    free_cb: Option<GfrFreeCb>,
) -> Result<Message, GfrStatus> {
    let password = fetch_password_with_cache(
        Some(&PASSWORD_CACHE),
        PasswordCachePolicy::Default,
        channel,
        "",
        "Symmetric Decryption",
        fetch_pwd_cb,
        free_cb,
    )?;

    if password.is_empty() {
        return Err(GfrStatus::ErrorInvalidInput);
    }

    let msg_pw = Password::from(password.as_slice());

    parsed_message
        .decrypt_with_password(&msg_pw)
        .map_err(|_| GfrStatus::ErrorDecryptionFailed)
}

pub fn decrypt_and_verify_stream_internal<R, W>(
    channel: i32,
    input_stream: R,
    mut output_stream: W,
    ascii_armor: bool,
    fetch_seckey_cb: Option<GfrSecretKeyFetchCb>,
    fetch_pwd_cb: Option<GfrPasswordFetchCb>,
    fetch_pubkey_cb: Option<GfrPublicKeyFetchCb>,
    free_cb: Option<GfrFreeCb>,
    user_data: *mut c_void,
) -> Result<DecryptAndVerifyStreamResultInternal, GfrStatus>
where
    R: Read + Send + Sync + fmt::Debug,
    W: Write + Send + Sync,
{
    let buf_input = BufReader::new(input_stream);

    // 1. Parse PGP outer envelope (consumes headers only)
    let parsed_message = if ascii_armor {
        Message::from_armor(buf_input)
            .map_err(|_| GfrStatus::ErrorInvalidInput)?
            .0
    } else {
        Message::from_reader(buf_input)
            .map_err(|_| GfrStatus::ErrorInvalidInput)?
            .0
    };

    let (_, has_skesk, mut recipients) = analyze_encrypted_envelope(&parsed_message)?;
    if !has_skesk && recipients.is_empty() {
        return Err(GfrStatus::ErrorInvalidData);
    }

    let mut decrypted: Message;
    if has_skesk {
        debug!("Message is encrypted with a passphrase. Attempting password-based decryption.");
        decrypted = decrypt_message_with_password(channel, parsed_message, fetch_pwd_cb, free_cb)?;
    } else {
        // 3. Request Secret Key from C++
        let mut target_skey: Option<SignedSecretKey> = None;
        let mut matched_recipient_id = String::new();

        if let Some(cb) = fetch_seckey_cb {
            for rec in &recipients {
                let c_key_id = CString::new(rec.key_id.clone()).unwrap_or_default();
                let c_key_block = cb(c_key_id.as_ptr(), user_data);

                if !c_key_block.is_null() {
                    if let Ok(key_str) = unsafe { CStr::from_ptr(c_key_block) }.to_str() {
                        if let Ok((cert, _)) = SignedSecretKey::from_string(key_str) {
                            target_skey = Some(cert);
                            matched_recipient_id = rec.key_id.clone();
                        }
                    }
                    if let Some(f_cb) = free_cb {
                        f_cb(c_key_block as *mut c_void, user_data);
                    }
                    if target_skey.is_some() {
                        break;
                    }
                }
            }
        }

        let skey = target_skey.ok_or(GfrStatus::ErrorNoKey)?;

        // 4. Check if unlocking is needed
        let mut needs_password = false;
        let primary_id = skey.primary_key.legacy_key_id().to_string();
        let is_anonymous = |id: &str| id == "0000000000000000";
        let mut target_fpr_for_pwd = skey.primary_key.fingerprint().to_string();

        if (matched_recipient_id == primary_id || is_anonymous(&matched_recipient_id))
            && matches!(skey.primary_key.secret_params(), SecretParams::Encrypted(_))
        {
            needs_password = true;
        }

        if !needs_password {
            for subkey in &skey.secret_subkeys {
                let subkey_id = subkey.key.legacy_key_id().to_string();
                if (matched_recipient_id == subkey_id || is_anonymous(&matched_recipient_id))
                    && matches!(subkey.key.secret_params(), SecretParams::Encrypted(_))
                {
                    needs_password = true;
                    target_fpr_for_pwd = subkey.key.fingerprint().to_string();
                    break;
                }
            }
        }

        let mut password = Vec::<u8>::new();
        if needs_password {
            password = fetch_password_with_cache(
                Some(&PASSWORD_CACHE),
                PasswordCachePolicy::Default,
                channel,
                &target_fpr_for_pwd,
                "Decryption",
                fetch_pwd_cb,
                free_cb,
            )?;
        } else {
            debug!("Target secret key is unlocked. Bypassing password callback.");
        }

        // 5. Initialize streaming decryption
        let pwd_fn = Password::from(password.as_slice());
        decrypted = parsed_message.decrypt(&pwd_fn, &skey).into_gfr()?;

        for rec in &mut recipients {
            if rec.key_id == matched_recipient_id || is_anonymous(&rec.key_id) {
                rec.status = GfrRecipientStatus::Success;
            }
        }
    }

    // 6. Mount decompression pipeline
    if decrypted.is_compressed() {
        decrypted = decrypted.decompress().into_gfr()?;
    }

    // 7. Extract filename
    let mut filename = String::new();
    if let Message::Literal { ref reader, .. } = decrypted {
        let header = reader.data_header();
        filename = String::from_utf8_lossy(header.file_name()).to_string();
    }

    // 8. Stream Execution
    std::io::copy(&mut decrypted, &mut output_stream).into_gfr()?;
    output_stream.flush().into_gfr()?;

    // ==========================================
    // OPTIONAL: VERIFICATION PHASE
    // ==========================================
    let mut signatures = Vec::new();
    let mut is_verified = false;

    // We only process signatures if the message is actually signed AND the caller requested verification
    if decrypted.is_signed() && fetch_pubkey_cb.is_some() {
        if let pgp::composed::Message::Signed { ref reader, .. } = decrypted {
            for i in 0..reader.num_signatures() {
                if let Some(sig) = reader.signature(i) {
                    for issuer in sig.issuer_fingerprint() {
                        let fpr = issuer.to_string();
                        let (hash_algo, pub_algo) = sig
                            .config()
                            .map(|c| (c.hash_alg.to_string(), algo_to_string_simple(c.pub_alg)))
                            .unwrap_or_default();

                        if !signatures
                            .iter()
                            .any(|r: &SignatureResultInternal| r.fpr == fpr)
                        {
                            signatures.push(SignatureResultInternal {
                                fpr,
                                status: GfrSignatureStatus::NoKey,
                                created_at: sig.created().map(|d| d.as_secs() as u32).unwrap_or(0),
                                pub_algo,
                                hash_algo,
                                sig_type: GfrSignMode::Inline,
                            });
                        }
                    }
                }
            }
        }

        // Fetch Public Keys
        let mut certs = Vec::new();
        if let Some(cb) = fetch_pubkey_cb {
            for sig in &signatures {
                let c_fpr = CString::new(sig.fpr.clone()).unwrap_or_default();
                let c_key_block = cb(c_fpr.as_ptr(), user_data);

                if !c_key_block.is_null() {
                    if let Ok(key_str) = unsafe { CStr::from_ptr(c_key_block) }.to_str() {
                        if let Ok((cert, _)) = SignedPublicKey::from_string(key_str) {
                            certs.push(cert);
                        }
                    }
                    if let Some(f_cb) = free_cb {
                        f_cb(c_key_block as *mut c_void, user_data);
                    }
                }
            }
        }

        // Verify signatures
        let update_signatures = |cert: &SignedPublicKey,
                                 is_cert_valid: bool,
                                 signatures: &mut Vec<SignatureResultInternal>,
                                 is_verified: &mut bool|
         -> bool {
            let mut found = false;
            for sig in signatures.iter_mut() {
                if cert_contains_issuer(cert, &sig.fpr) {
                    found = true;
                    if is_cert_valid {
                        sig.status = GfrSignatureStatus::Valid;
                        *is_verified = true;
                    } else if sig.status == GfrSignatureStatus::NoKey {
                        sig.status = GfrSignatureStatus::BadSignature;
                    }
                }
            }
            found
        };

        for cert in &certs {
            let is_cert_valid = decrypted.verify(cert).is_ok()
                || cert
                    .public_subkeys
                    .iter()
                    .any(|sk| decrypted.verify(sk).is_ok());

            update_signatures(cert, is_cert_valid, &mut signatures, &mut is_verified);
        }
    }

    Ok(DecryptAndVerifyStreamResultInternal {
        filename,
        recipients,
        is_verified,
        signatures,
    })
}

/// Stream decryption and extraction of a Tar directory
pub fn decrypt_and_verify_archive_internal(
    channel: i32,
    in_file_path: &str,
    out_dir_path: &str,
    ascii_armor: bool,
    fetch_seckey_cb: Option<crate::types::GfrSecretKeyFetchCb>,
    fetch_pwd_cb: Option<crate::types::GfrPasswordFetchCb>,
    fetch_pubkey_cb: Option<crate::types::GfrPublicKeyFetchCb>,
    free_cb: Option<crate::types::GfrFreeCb>,
    user_data: *mut std::ffi::c_void,
) -> Result<crate::crypto::DecryptAndVerifyResultInternal, crate::types::GfrStatus> {
    let out_dir = Path::new(out_dir_path);

    // 1. Ensure the target extraction directory exists
    if !out_dir.exists() {
        std::fs::create_dir_all(out_dir).map_err(|e| {
            log::error!("Failed to create output directory: {}", e);
            set_last_error(&e.to_string());
            crate::types::GfrStatus::ErrorIo
        })?;
    }

    // 2. Open the encrypted input file
    let in_file = File::open(in_file_path).map_err(|e| {
        log::error!("Failed to open encrypted input file: {}", e);
        crate::types::GfrStatus::ErrorIo
    })?;

    // 3. Create an anonymous temporary file as a secure buffer for decrypted data
    let mut temp_archive = tempfile::tempfile().map_err(|e| {
        log::error!("Failed to create temp file for decryption: {}", e);
        crate::types::GfrStatus::ErrorIo
    })?;

    // 4. Perform stream decryption and signature verification
    log::info!("Decrypting file into temporary archive...");
    let stream_result = crate::crypto_stream::decrypt_and_verify_stream_internal(
        channel,
        in_file,
        &mut temp_archive,
        ascii_armor,
        fetch_seckey_cb,
        fetch_pwd_cb,
        fetch_pubkey_cb,
        free_cb,
        user_data,
    )?;

    // 5. Move the temporary file cursor back to the beginning, preparing for extraction
    temp_archive
        .seek(SeekFrom::Start(0))
        .map_err(|_| crate::types::GfrStatus::ErrorIo)?;

    // 6. Perform Tar extraction operation
    log::info!(
        "Unpacking tar archive to target directory: {}",
        out_dir_path
    );
    let mut archive = tar::Archive::new(temp_archive);
    archive.unpack(out_dir).map_err(|e| {
        log::error!("Failed to unpack tar archive: {}", e);
        crate::types::GfrStatus::ErrorInvalidData // Extraction failure may indicate the content is not a valid tar archive
    })?;

    // 7. Assemble the return result (pure file stream operation, so payload data is empty)
    Ok(crate::crypto::DecryptAndVerifyResultInternal {
        data: Vec::new(),
        filename: stream_result.filename,
        recipients: stream_result.recipients,
        is_verified: stream_result.is_verified,
        signatures: stream_result.signatures,
    })
}

pub struct SymmetricEncryptStreamResultInternal {}

pub fn encrypt_stream_with_password_internal<R, W>(
    channel: i32,
    name: &str,
    input_stream: R,
    mut output_stream: W,
    fetch_pwd_cb: Option<GfrPasswordFetchCb>,
    free_cb: Option<GfrFreeCb>,
    ascii_armor: bool,
) -> Result<(), GfrStatus>
where
    R: Read + Send + Sync,
    W: Write + Send + Sync,
{
    let mut rng = thread_rng();
    let filename_bytes = name.as_bytes().to_vec();

    let mut builder = MessageBuilder::from_reader(filename_bytes, input_stream);
    builder.partial_chunk_size(512 * 1024).into_gfr()?;

    let mut enc_builder = builder.seipd_v1(&mut rng, SymmetricKeyAlgorithm::AES256);

    let password: Vec<u8> = fetch_password_with_cache(
        Some(&PASSWORD_CACHE),
        PasswordCachePolicy::Default,
        channel,
        "",
        "Symmetric Encryption",
        fetch_pwd_cb,
        free_cb,
    )?;
    if password.is_empty() {
        return Err(GfrStatus::ErrorInvalidInput);
    }

    let msg_pw = Password::from(password.as_slice());

    let s2k = StringToKey::new_argon2(&mut rng, 1, 4, 21);

    enc_builder.encrypt_with_password(s2k, &msg_pw).into_gfr()?;

    let result = if ascii_armor {
        enc_builder.to_armored_writer(&mut rng, ArmorOptions::default(), &mut output_stream)
    } else {
        enc_builder.to_writer(&mut rng, &mut output_stream)
    };

    result.map_err(|_| GfrStatus::ErrorInternal)?;
    output_stream
        .flush()
        .map_err(|_| GfrStatus::ErrorInternal)?;
    Ok(())
}

/// Package a directory as a Tar and encrypt it as a stream
pub fn encrypt_directory_with_password_internal(
    channel: i32,
    in_dir_path: &str,
    out_file_path: &str,
    fetch_pwd_cb: Option<GfrPasswordFetchCb>,
    free_cb: Option<GfrFreeCb>,
    ascii_armor: bool,
) -> Result<SymmetricEncryptStreamResultInternal, crate::types::GfrStatus> {
    let (temp_archive, filename_hint) = build_tar_tempfile_from_directory(in_dir_path)?;
    let out_file = create_output_file(out_file_path)?;

    log::info!("Encrypting tar archive...");
    encrypt_stream_with_password_internal(
        channel,
        &filename_hint,
        temp_archive,
        out_file,
        fetch_pwd_cb,
        free_cb,
        ascii_armor,
    )?;

    Ok(SymmetricEncryptStreamResultInternal {})
}
