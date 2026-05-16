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

/// Sign a stream or buffer with one or more secret keys.
///
/// The three modes have different buffering requirements:
/// - **Inline** — true streaming; the builder consumes the input reader directly
///   and can sign multiple keys in a single pass.
/// - **ClearText** — the entire input is buffered to memory because rPGP
///   requires `&str` for CRLF normalization. Only the first key is used.
/// - **Detached** — the entire input is buffered to compute the hash. Only
///   the first key is used.
///
/// ClearText and Detached use only `parsed_keys.first()` by design: the
/// OpenPGP clear-text and detached formats do not compose well with multiple
/// independent signers in a single-pass stream.
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
                PassphraseStateInternal {
                    fpr: fpr.to_string(),
                    info: "Signing".to_string(),
                    retry: false,
                    ask_for_new: false,
                    should_confirm: false,
                },
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

            if result.is_err() {
                log::warn!("Stream signing pipeline failed. Evicting all target password caches.");
                for (skey, _) in &parsed_keys {
                    PASSWORD_CACHE.remove_by_fpr(&skey.primary_key.fingerprint().to_string());
                    for sub in &skey.secret_subkeys {
                        PASSWORD_CACHE.remove_by_fpr(&sub.key.fingerprint().to_string());
                    }
                }
            }

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
            let mut data = Vec::new();
            input_stream
                .read_to_end(&mut data)
                .map_err(|_| GfrStatus::ErrorInvalidInput)?;
            let text_str = std::str::from_utf8(&data).map_err(|_| GfrStatus::ErrorInvalidInput)?;

            match parsed_keys.first() {
                None => Err(GfrStatus::ErrorInvalidInput),
                Some((skey, target)) => {
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

                        log::warn!("Signing failed for {}. Evicting bad password.", fpr);
                        PASSWORD_CACHE.remove_by_fpr(&fpr);
                        Err(GfrStatus::ErrorBadPassphrase)
                    });

                    if let Ok(out_data) = res {
                        output_stream
                            .write_all(&out_data)
                            .map_err(|_| GfrStatus::ErrorInternal)?;
                        output_stream
                            .flush()
                            .map_err(|_| GfrStatus::ErrorInternal)?;
                        Ok(SignStreamResultInternal {
                            signatures: created_signatures,
                        })
                    } else {
                        log::error!("Failed to create cleartext signature. Error: {:?}", res);
                        Err(res.err().unwrap_or(GfrStatus::ErrorInternal))
                    }
                }
            }
        }

        // ---------------------------------------------------------
        // MODE 2: DETACHED SIGNATURE (Buffered)
        // ---------------------------------------------------------
        GfrSignMode::Detached => {
            let mut data = Vec::new();
            input_stream
                .read_to_end(&mut data)
                .map_err(|_| GfrStatus::ErrorInvalidInput)?;

            match parsed_keys.first() {
                None => Err(GfrStatus::ErrorInvalidInput),
                Some((skey, target)) => {
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

                        log::warn!("Signing failed for {}. Evicting bad password.", fpr);
                        PASSWORD_CACHE.remove_by_fpr(&fpr);
                        Err(GfrStatus::ErrorBadPassphrase)
                    });

                    if let Ok(out_data) = res {
                        output_stream
                            .write_all(&out_data)
                            .map_err(|_| GfrStatus::ErrorInternal)?;
                        output_stream
                            .flush()
                            .map_err(|_| GfrStatus::ErrorInternal)?;
                        Ok(SignStreamResultInternal {
                            signatures: created_signatures,
                        })
                    } else {
                        log::error!("Failed to create detached signature. Error: {:?}", res);
                        Err(res.err().unwrap_or(GfrStatus::ErrorInternal))
                    }
                }
            }
        }
    }
}
