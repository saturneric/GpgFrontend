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

use zeroize::Zeroizing;

use crate::utils::password_from_zeroizing_bytes;

use super::*;

/// Encrypt a stream with one or more public keys (no signature).
///
/// Thin wrapper around [`encrypt_and_sign_stream_internal`] with an empty
/// signer list, so the signing phase is skipped entirely.
pub fn encrypt_stream_internal<R, W>(
    channel: i32,
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
        channel,
        filename_hint,
        input_stream,
        output_stream,
        public_key_blocks,
        &[], // Empty secret keys skips the signing phase!
        None,
        ascii_armor,
    )?;

    Ok(EncryptStreamResultInternal {
        invalid_recipients: result.invalid_recipients,
    })
}

/// Pack a directory into a temporary tar archive, then encrypt it with public keys.
///
/// The tar file is created in the OS temp directory as an anonymous file and is
/// removed automatically when the handle is dropped.
pub fn encrypt_directory_internal(
    channel: i32,
    in_dir_path: &str,
    out_file_path: &str,
    public_key_blocks: &[&str],
    ascii_armor: bool,
) -> Result<EncryptStreamResultInternal, crate::types::GfrStatus> {
    let (temp_archive, filename_hint) = build_tar_tempfile_from_directory(in_dir_path)?;
    let out_file = create_output_file(out_file_path)?;

    log::info!("Encrypting tar archive...");
    encrypt_stream_internal(
        channel,
        &filename_hint,
        temp_archive,
        out_file,
        public_key_blocks,
        ascii_armor,
    )
}

/// Encrypt a stream with public keys and optionally sign it.
///
/// Pass an empty `secret_key_blocks` slice to encrypt without signing; the
/// signing phase is skipped entirely in that case. Uses SEIPD v1 (AES-256)
/// and 512 KiB partial packets for streaming efficiency on large payloads.
pub fn encrypt_and_sign_stream_internal<R, W>(
    channel: i32,
    name: &str,
    input_stream: R,
    mut output_stream: W,
    public_key_blocks: &[&str],
    secret_key_blocks: &[&str],
    fetch_cb: Option<GfrPasswordFetchCb>,
    ascii_armor: bool,
) -> Result<EncryptAndSignStreamResultInternal, GfrStatus>
where
    R: Read + Send + Sync,
    W: Write + Send + Sync,
{
    let mut rng = thread_rng();
    let filename_bytes = name.as_bytes().to_vec();

    let parsed_skeys = parse_secret_signers(secret_key_blocks)?;

    // Wrap the source so a user cancel request aborts the streaming read.
    let input_stream = crate::cancel::CancellableReader::new(channel, input_stream);
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
                    PassphraseStateInternal {
                        fpr: fpr.to_string(),
                        info: "Unlock key for signing".to_string(),
                        retry: false,
                        ask_for_new: false,
                        should_confirm: false,
                    },
                    fetch_cb,
                )?;
                Ok(password_from_zeroizing_bytes(pwd_bytes))
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

    result.record_err_with(|| {
        crate::cancel::status_or_canceled(channel, GfrStatus::ErrorInternal)
    })?;
    output_stream.flush().record_err_with(|| {
        crate::cancel::status_or_canceled(channel, GfrStatus::ErrorInternal)
    })?;

    Ok(EncryptAndSignStreamResultInternal {
        signatures: created_signatures,
        invalid_recipients,
    })
}

/// Pack a directory into a tar archive, then encrypt and sign it in one pass.
pub fn encrypt_and_sign_directory_internal(
    channel: i32,
    in_dir_path: &str,
    out_file_path: &str,
    public_key_blocks: &[&str],
    secret_key_blocks: &[&str],
    fetch_pwd_cb: Option<crate::types::GfrPasswordFetchCb>,
    ascii_armor: bool,
) -> Result<EncryptAndSignResultInternal, crate::types::GfrStatus> {
    let (temp_archive, filename_hint) = build_tar_tempfile_from_directory(in_dir_path)?;
    let out_file = create_output_file(out_file_path)?;

    log::info!("Encrypting and signing tar archive...");
    let stream_result = encrypt_and_sign_stream_internal(
        channel,
        &filename_hint,
        temp_archive,
        out_file,
        public_key_blocks,
        secret_key_blocks,
        fetch_pwd_cb,
        ascii_armor,
    )?;

    Ok(EncryptAndSignResultInternal {
        data: Vec::new(),
        signatures: stream_result.signatures,
        invalid_recipients: stream_result.invalid_recipients,
    })
}

/// Symmetrically encrypt a stream with a user-supplied passphrase.
///
/// The password is always fetched fresh via the callback (cache bypassed) and
/// the user is asked to confirm it. Key derivation uses Argon2id via the
/// OpenPGP `StringToKey` mechanism, so decryption requires an rPGP-aware tool.
pub fn encrypt_stream_with_password_internal<R, W>(
    channel: i32,
    name: &str,
    input_stream: R,
    mut output_stream: W,
    fetch_pwd_cb: Option<GfrPasswordFetchCb>,
    ascii_armor: bool,
) -> Result<(), GfrStatus>
where
    R: Read + Send + Sync,
    W: Write + Send + Sync,
{
    let mut rng = thread_rng();
    let filename_bytes = name.as_bytes().to_vec();

    // Wrap the source so a user cancel request aborts the streaming read.
    let input_stream = crate::cancel::CancellableReader::new(channel, input_stream);
    let mut builder = MessageBuilder::from_reader(filename_bytes, input_stream);
    builder.partial_chunk_size(512 * 1024).into_gfr()?;

    let mut enc_builder = builder.seipd_v1(&mut rng, SymmetricKeyAlgorithm::AES256);

    let password: Zeroizing<Vec<u8>> = fetch_password_with_cache(
        Some(&PASSWORD_CACHE),
        PasswordCachePolicy::Bypass,
        channel,
        PassphraseStateInternal {
            fpr: String::new(),
            info: "Symmetric Encryption".to_string(),
            retry: false,
            ask_for_new: true,
            should_confirm: true, // Should confirm for symmetric encryption to avoid accidental encryptions with wrong passwords
        },
        fetch_pwd_cb,
    )?;
    if password.is_empty() {
        return Err(GfrStatus::ErrorBadPassphrase);
    }

    let msg_pw = password_from_zeroizing_bytes(password);
    let s2k = StringToKey::new_argon2(&mut rng, 1, 4, 21);
    enc_builder.encrypt_with_password(s2k, &msg_pw).into_gfr()?;

    let result = if ascii_armor {
        enc_builder.to_armored_writer(&mut rng, ArmorOptions::default(), &mut output_stream)
    } else {
        enc_builder.to_writer(&mut rng, &mut output_stream)
    };

    result.record_err_with(|| {
        crate::cancel::status_or_canceled(channel, GfrStatus::ErrorInternal)
    })?;
    output_stream.flush().record_err_with(|| {
        crate::cancel::status_or_canceled(channel, GfrStatus::ErrorInternal)
    })?;
    Ok(())
}

/// Pack a directory into a tar archive, then symmetrically encrypt it with a passphrase.
pub fn encrypt_directory_with_password_internal(
    channel: i32,
    in_dir_path: &str,
    out_file_path: &str,
    fetch_pwd_cb: Option<GfrPasswordFetchCb>,
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
        ascii_armor,
    )?;

    Ok(SymmetricEncryptStreamResultInternal {})
}

/// Encrypt an in-memory buffer with the given public keys.
pub fn encrypt_internal(
    channel: i32,
    name: &str,
    data: &[u8],
    public_key_blocks: &[&str],
    ascii_armor: bool,
) -> Result<EncryptResultInternal, GfrStatus> {
    let mut output = Vec::new();

    let stream_result =
        encrypt_stream_internal(channel, name, data, &mut output, public_key_blocks, ascii_armor)?;

    Ok(EncryptResultInternal {
        data: output,
        invalid_recipients: stream_result.invalid_recipients,
    })
}

/// Encrypt and sign an in-memory buffer in a single operation.
pub fn encrypt_and_sign_internal(
    channel: i32,
    name: &str,
    data: &[u8],
    public_key_blocks: &[&str],
    secret_key_blocks: &[&str],
    fetch_cb: Option<GfrPasswordFetchCb>,
    ascii_armor: bool,
) -> Result<EncryptAndSignResultInternal, GfrStatus> {
    let mut output_data = Vec::new();
    let input_cursor = Cursor::new(data);

    // Delegate processing to the streaming pipeline
    let stream_result = encrypt_and_sign_stream_internal(
        channel,
        name,
        input_cursor,
        &mut output_data, // Safely handles the output as a Write stream
        public_key_blocks,
        secret_key_blocks,
        fetch_cb,
        ascii_armor,
    )?;

    Ok(EncryptAndSignResultInternal {
        data: output_data,
        signatures: stream_result.signatures,
        invalid_recipients: stream_result.invalid_recipients,
    })
}
