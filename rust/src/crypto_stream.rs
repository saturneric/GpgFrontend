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
    crypto::{
        InvalidRecipientInternal, RecipientResultInternal, SelectedKey, SignatureResultInternal,
        algo_to_string_simple, cert_contains_issuer, parse_signer_block, sniff_signatures,
        with_signing_key,
    },
    types::{
        GfrFreeCb, GfrPasswordFetchCb, GfrPublicKeyFetchCb, GfrRecipientStatus,
        GfrSecretKeyFetchCb, GfrSignMode, GfrSignatureStatus, GfrStatus,
    },
    utils::fetch_password_internal,
};
use core::fmt;
use log::debug;
use pgp::{
    armor::Dearmor,
    composed::{
        ArmorOptions, CleartextSignedMessage, Deserializable, DetachedSignature, Esk, Message,
        MessageBuilder, SignedPublicKey, SignedSecretKey,
    },
    crypto::{hash::HashAlgorithm, public_key::PublicKeyAlgorithm, sym::SymmetricKeyAlgorithm},
    packet::{Packet, PacketParser, SecretKey, SecretSubkey},
    ser::Serialize,
    types::{KeyDetails, Password, SecretParams},
};
use rand::thread_rng;
use std::{
    ffi::{CStr, CString, c_void},
    io::{BufReader, Cursor, Read, Seek, SeekFrom, Write},
};

pub struct EncryptStreamResultInternal {
    pub invalid_recipients: Vec<InvalidRecipientInternal>,
}

pub fn encrypt_stream_internal<R, W>(
    filename_hint: &str,
    input_stream: R,
    mut output_stream: W,
    public_key_blocks: &[&str],
    ascii_armor: bool,
) -> Result<EncryptStreamResultInternal, GfrStatus>
where
    R: Read,
    W: Write,
{
    let mut rng = thread_rng();

    let mut builder = MessageBuilder::from_reader(filename_hint.as_bytes().to_vec(), input_stream);

    builder
        .partial_chunk_size(512 * 1024)
        .map_err(|_| GfrStatus::ErrorInternal)?;

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

    if result.is_err() {
        return Err(GfrStatus::ErrorInternal);
    }

    if output_stream.flush().is_err() {
        return Err(GfrStatus::ErrorInternal);
    }

    Ok(EncryptStreamResultInternal { invalid_recipients })
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
    let mut parsed_keys = Vec::with_capacity(secret_key_blocks.len());
    for block in secret_key_blocks {
        let (target, armor_block) = parse_signer_block(block);
        let (skey, _) =
            SignedSecretKey::from_string(armor_block).map_err(|_| GfrStatus::ErrorInvalidInput)?;
        parsed_keys.push((skey, target));
    }

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
            let pwd_bytes = fetch_password_internal(channel, fpr, "Signing", fetch_cb, free_cb)?;
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
    channel: i32, // Added for callback context
    name: &str,
    input_stream: R,
    mut output_stream: W,
    public_key_blocks: &[&str],
    secret_key_blocks: &[&str],
    fetch_cb: Option<GfrPasswordFetchCb>,
    free_cb: Option<GfrFreeCb>,
    ascii_armor: bool,
) -> Result<EncryptAndSignStreamResultInternal, GfrStatus>
where
    R: Read + Send + Sync,
    W: Write + Send + Sync,
{
    // Check if we have at least one secret key to sign with
    if secret_key_blocks.is_empty() {
        return Err(GfrStatus::ErrorInvalidInput);
    }

    let mut rng = thread_rng();
    let filename_bytes = name.as_bytes().to_vec();

    // 1. Initialize the builder with the streaming payload
    // Using from_reader hooks up the input stream to the pipeline
    let mut builder = MessageBuilder::from_reader(filename_bytes, input_stream);

    // 2. Process Signers FIRST (Signing must wrap the payload before encryption)
    // Using the robust parse_signer_block for exact subkey target matching (!)
    let mut parsed_skeys = Vec::with_capacity(secret_key_blocks.len());
    for block in secret_key_blocks {
        let (target, armor_block) = parse_signer_block(block);
        let (skey, _) =
            SignedSecretKey::from_string(armor_block).map_err(|_| GfrStatus::ErrorInvalidInput)?;
        parsed_skeys.push((skey, target));
    }

    let mut created_signatures = Vec::new();
    let current_time = std::time::SystemTime::now()
        .duration_since(std::time::UNIX_EPOCH)
        .unwrap()
        .as_secs() as u32;

    let mut record_sig = |fpr: String, algo_str: String| {
        created_signatures.push(SignatureResultInternal {
            fpr,
            status: GfrSignatureStatus::Valid,
            created_at: current_time,
            pub_algo: algo_str,
            hash_algo: "SHA512".to_string(),
            sig_type: GfrSignMode::Inline, // Encrypt+Sign is always Inline nested
        });
    };

    // Helper closure to dynamically fetch password for signing keys
    let fetch_pwd_for_key = |is_encrypted: bool, fpr: &str| -> Result<Password, GfrStatus> {
        if is_encrypted {
            let pwd_bytes = fetch_password_internal(channel, fpr, "Signing", fetch_cb, free_cb)?;
            Ok(Password::from(pwd_bytes.as_slice()))
        } else {
            debug!("Target secret key is unlocked. Bypassing password callback for signing.");
            Ok(Password::empty())
        }
    };

    let mut at_least_one_signer = false;

    // Iterate over each provided signing key
    for (skey, target) in &parsed_skeys {
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
        return Err(GfrStatus::ErrorInvalidInput); // Missing valid signing key
    }

    // 3. Transition the Builder into Encryption Mode
    // This wraps the signed inner payload into an encrypted outer envelope
    let mut enc_builder = builder.seipd_v1(&mut rng, SymmetricKeyAlgorithm::AES256);

    // 4. Process Recipients
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
        return Err(GfrStatus::ErrorInvalidInput); // Missing valid encryption key
    }

    // 5. Finalize the nested payload and stream to output
    let result = if ascii_armor {
        enc_builder.to_armored_writer(&mut rng, ArmorOptions::default(), &mut output_stream)
    } else {
        enc_builder.to_writer(&mut rng, &mut output_stream)
    };

    if result.is_err() {
        return Err(GfrStatus::ErrorInternal);
    }

    if output_stream.flush().is_err() {
        return Err(GfrStatus::ErrorInternal);
    }

    Ok(EncryptAndSignStreamResultInternal {
        signatures: created_signatures,
        invalid_recipients,
    })
}

pub struct DecryptAndVerifyStreamResultInternal {
    pub filename: String,
    pub recipients: Vec<RecipientResultInternal>,
    pub is_verified: bool,
    pub signatures: Vec<SignatureResultInternal>,
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

    // 2. Extract recipient Key IDs
    let mut recipients = Vec::new();
    if let Message::Encrypted { esk, .. } = &parsed_message {
        for e in esk {
            if let Esk::PublicKeyEncryptedSessionKey(pkesk) = e {
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
        }
    }

    if recipients.is_empty() {
        return Err(GfrStatus::ErrorInvalidData);
    }

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
        password = fetch_password_internal(
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
    let mut decrypted = parsed_message
        .decrypt(&pwd_fn, &skey)
        .map_err(|_| GfrStatus::ErrorDecryptionFailed)?;

    for rec in &mut recipients {
        if rec.key_id == matched_recipient_id || is_anonymous(&rec.key_id) {
            rec.status = GfrRecipientStatus::Success;
        }
    }

    // 6. Mount decompression pipeline
    if decrypted.is_compressed() {
        decrypted = decrypted
            .decompress()
            .map_err(|_| GfrStatus::ErrorInternal)?;
    }

    // 7. Extract filename
    let mut filename = String::new();
    if let Message::Literal { ref reader, .. } = decrypted {
        let header = reader.data_header();
        filename = String::from_utf8_lossy(header.file_name()).to_string();
    }

    // 8. Stream Execution
    std::io::copy(&mut decrypted, &mut output_stream).map_err(|_| GfrStatus::ErrorInternal)?;
    output_stream
        .flush()
        .map_err(|_| GfrStatus::ErrorInternal)?;

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
        let mut update_signatures = |cert: &SignedPublicKey,
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
