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

use super::*;

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
        PasswordCachePolicy::Bypass,
        channel,
        PassphraseStateInternal {
            fpr: String::new(),
            info: "Symmetric Decryption".to_string(),
            retry: false,
            ask_for_new: true, // For symmetric decryption, we always want to ask for a new password and bypass cache to avoid false hits
            should_confirm: false,
        },
        fetch_pwd_cb,
        free_cb,
    )?;

    if password.is_empty() {
        return Err(GfrStatus::ErrorBadPassphrase);
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
                PassphraseStateInternal {
                    fpr: target_fpr_for_pwd.clone(),
                    info: "Decryption".to_string(),
                    retry: false,
                    ask_for_new: false,
                    should_confirm: false,
                },
                fetch_pwd_cb,
                free_cb,
            )?;
        } else {
            debug!("Target secret key is unlocked. Bypassing password callback.");
        }

        // 5. Initialize streaming decryption
        let pwd_fn = Password::from(password.as_slice());
        decrypted = parsed_message
            .decrypt(&pwd_fn, &skey)
            .inspect_err(|_| {
                if needs_password {
                    log::warn!(
                        "Asymmetric decryption failed. Evicting bad password for FPR: {}",
                        target_fpr_for_pwd
                    );
                    PASSWORD_CACHE.remove_by_fpr(&target_fpr_for_pwd);
                }
            })
            .into_gfr()?;

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
                                created_at: sig.created().map(|d| d.as_secs()).unwrap_or(0),
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
