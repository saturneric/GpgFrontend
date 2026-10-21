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

use crate::utils::{armor_opts_for, is_v4_era, password_from_zeroizing_bytes};

use pgp::{
    packet::{SignatureConfig, SignatureType, Subpacket, SubpacketData},
    types::{KeyDetails, KeyVersion, SigningKey, Timestamp},
};

use super::*;

/// Sign a stream or buffer with one or more secret keys.
///
/// The three modes have different buffering requirements:
/// - **Inline** — true streaming; the builder consumes the input reader directly
///   and can sign multiple keys in a single pass.
/// - **ClearText** — the entire input is buffered to memory because rPGP
///   requires `&str` for CRLF normalization. All keys are signed via `new_many`.
/// - **Detached** — the entire input is buffered to compute the hash. All keys
///   are signed and their signature packets are concatenated.
pub fn sign_stream_internal<R, W>(
    channel: i32,
    name: &str,
    input_stream: R,
    mut output_stream: W,
    secret_key_blocks: &[&str],
    fetch_cb: Option<GfrPasswordFetchCb>,
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

    // Wrap the source so a user cancel request aborts the streaming read,
    // whether the mode hands the reader to rPGP (Inline) or drains it to a
    // buffer (ClearText / Detached).
    let mut input_stream = crate::cancel::CancellableReader::new(channel, input_stream);

    log::info!(
        "Parsed {} secret key blocks for signing operation '{}'",
        parsed_keys.len(),
        name
    );

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
            expires_at: 0,
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
            )?;
            Ok(password_from_zeroizing_bytes(pwd_bytes))
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
                    selected_key.check_passphrase(&pwd)?;

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

            // Execute the pipeline: read from input, sign chunks, write to output.
            // RFC 9580 §6.1: a CRC24 footer only when every signer is v4.
            let v4_only = parsed_keys
                .iter()
                .all(|(skey, _)| is_v4_era(skey.primary_key.version()));
            let result = if ascii_armor {
                builder.to_armored_writer(&mut rng, armor_opts_for(v4_only), &mut output_stream)
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

            result.record_err_with(|| {
                crate::cancel::status_or_canceled(channel, GfrStatus::ErrorInternal)
            })?;
            output_stream.flush().record_err_with(|| {
                crate::cancel::status_or_canceled(channel, GfrStatus::ErrorInternal)
            })?;

            Ok(SignStreamResultInternal {
                signatures: created_signatures,
            })
        }

        // ---------------------------------------------------------
        // MODE 1: CLEARTEXT SIGNATURE (Buffered)
        // ---------------------------------------------------------
        GfrSignMode::ClearText => {
            let mut data = Vec::new();
            input_stream.read_to_end(&mut data).record_err_with(|| {
                crate::cancel::status_or_canceled(channel, GfrStatus::ErrorInvalidInput)
            })?;
            let text_str = std::str::from_utf8(&data).record_err(GfrStatus::ErrorInvalidInput)?;

            if parsed_keys.is_empty() {
                return Err(GfrStatus::ErrorInvalidInput);
            }

            let keys_ref = &parsed_keys;
            let record_sig_fn = &mut record_sig;
            let fetch_pwd = &fetch_pwd_for_key;

            let text_str = &trim_trailing_whitespace(text_str);
            // rPGP's callback can only return a message; keep the first real
            // status (a cancel, a wrong passphrase, an unusable key) for the
            // caller instead of flattening it to ErrorInternal.
            let mut failure: Option<GfrStatus> = None;
            let failure_ref = &mut failure;
            let msg = CleartextSignedMessage::new_many(
                text_str,
                |normalized_text| -> pgp::errors::Result<Vec<pgp::packet::Signature>> {
                    let mut sigs = Vec::new();

                    for (skey, target) in keys_ref.iter() {
                        let sig_result =
                            with_signing_key(skey, target.as_deref(), |selected_key| {
                                let fpr = selected_key.fpr();
                                let is_enc = selected_key.is_encrypted();
                                let algo_str = algo_to_string_simple(selected_key.algorithm());
                                let pwd = fetch_pwd(is_enc, &fpr)?;
                                selected_key.check_passphrase(&pwd)?;

                                let normalized_bytes = normalized_text.as_bytes();
                                let cursor = Cursor::new(normalized_bytes);

                                let signature = match selected_key {
                                    SelectedKey::Primary(k) => {
                                        let hashed_subpackets = vec![
                                            Subpacket::regular(
                                                SubpacketData::SignatureCreationTime(
                                                    Timestamp::now(),
                                                ),
                                            )
                                            .map_err(|_| GfrStatus::ErrorInternal)?,
                                            Subpacket::regular(SubpacketData::IssuerFingerprint(
                                                k.fingerprint(),
                                            ))
                                            .map_err(|_| GfrStatus::ErrorInternal)?,
                                        ];
                                        // Pin the cleartext signing hash to SHA-512 so the reported metadata is
                                        // truthful and never a weak digest (RFC 9580 §9.5).
                                        let mut config = sha512_signature_config(
                                            &mut rng,
                                            k,
                                            SignatureType::Text,
                                        )
                                        .map_err(|_| GfrStatus::ErrorInternal)?;
                                        config.hashed_subpackets = hashed_subpackets;
                                        if k.version() <= KeyVersion::V4 {
                                            config.unhashed_subpackets = vec![
                                                Subpacket::regular(SubpacketData::IssuerKeyId(
                                                    k.legacy_key_id(),
                                                ))
                                                .map_err(|_| GfrStatus::ErrorInternal)?,
                                            ];
                                        }
                                        config.sign(k, &pwd, cursor).map_err(|e| {
                                            // The passphrase was checked already
                                            // (`check_passphrase`): this is not a
                                            // passphrase problem, so it is not
                                            // reported as one and nothing is evicted.
                                            set_last_error(&format!(
                                                "signing failed for key {}: {}",
                                                fpr, e
                                            ));
                                            GfrStatus::ErrorInternal
                                        })?
                                    }
                                    SelectedKey::Sub(k) => {
                                        let hashed_subpackets = vec![
                                            Subpacket::regular(
                                                SubpacketData::SignatureCreationTime(
                                                    Timestamp::now(),
                                                ),
                                            )
                                            .map_err(|_| GfrStatus::ErrorInternal)?,
                                            Subpacket::regular(SubpacketData::IssuerFingerprint(
                                                k.fingerprint(),
                                            ))
                                            .map_err(|_| GfrStatus::ErrorInternal)?,
                                        ];
                                        // Pin the cleartext signing hash to SHA-512 so the reported metadata is
                                        // truthful and never a weak digest (RFC 9580 §9.5).
                                        let mut config = sha512_signature_config(
                                            &mut rng,
                                            k,
                                            SignatureType::Text,
                                        )
                                        .map_err(|_| GfrStatus::ErrorInternal)?;
                                        config.hashed_subpackets = hashed_subpackets;
                                        if k.version() <= KeyVersion::V4 {
                                            config.unhashed_subpackets = vec![
                                                Subpacket::regular(SubpacketData::IssuerKeyId(
                                                    k.legacy_key_id(),
                                                ))
                                                .map_err(|_| GfrStatus::ErrorInternal)?,
                                            ];
                                        }
                                        config.sign(k, &pwd, cursor).map_err(|e| {
                                            // The passphrase was checked already
                                            // (`check_passphrase`): this is not a
                                            // passphrase problem, so it is not
                                            // reported as one and nothing is evicted.
                                            set_last_error(&format!(
                                                "signing failed for key {}: {}",
                                                fpr, e
                                            ));
                                            GfrStatus::ErrorInternal
                                        })?
                                    }
                                };

                                record_sig_fn(fpr, algo_str);
                                Ok(signature)
                            });

                        sigs.push(sig_result.map_err(|e| {
                            failure_ref.get_or_insert(e);
                            e.to_string()
                        })?);
                    }

                    Ok(sigs)
                },
            )
            .map_err(|_| failure.unwrap_or(GfrStatus::ErrorInternal))?;

            // RFC 9580 §6.1: a CRC24 footer only when every signature is v4.
            let v4_only = msg
                .signatures()
                .iter()
                .all(|sig| sig.version() == pgp::packet::SignatureVersion::V4);
            let armored = msg
                .to_armored_string(armor_opts_for(v4_only))
                .map_err(|_| GfrStatus::ErrorArmorFailed)?;
            let all_v6 = msg
                .signatures()
                .iter()
                .all(|sig| sig.version() == pgp::packet::SignatureVersion::V6);
            let armored = dash_escape_from_lines(&armored);
            let out = if all_v6 {
                strip_cleartext_hash_headers(&armored)
            } else {
                armored
            }
            .into_bytes();

            output_stream.write_all(&out).map_err(|_| {
                crate::cancel::status_or_canceled(channel, GfrStatus::ErrorInternal)
            })?;
            output_stream.flush().map_err(|_| {
                crate::cancel::status_or_canceled(channel, GfrStatus::ErrorInternal)
            })?;

            Ok(SignStreamResultInternal {
                signatures: created_signatures,
            })
        }

        // ---------------------------------------------------------
        // MODE 2: DETACHED SIGNATURE (Buffered)
        // ---------------------------------------------------------
        GfrSignMode::Detached => {
            let mut data = Vec::new();
            input_stream.read_to_end(&mut data).record_err_with(|| {
                crate::cancel::status_or_canceled(channel, GfrStatus::ErrorInvalidInput)
            })?;

            if parsed_keys.is_empty() {
                return Err(GfrStatus::ErrorInvalidInput);
            }

            let mut all_sigs = Vec::new();
            let mut at_least_one_signer = false;

            for (skey, target) in &parsed_keys {
                let res = with_signing_key(skey, target.as_deref(), |selected_key| {
                    let fpr = selected_key.fpr();
                    let is_enc = selected_key.is_encrypted();
                    let algo_str = algo_to_string_simple(selected_key.algorithm());
                    let pwd = fetch_pwd_for_key(is_enc, &fpr)?;
                    selected_key.check_passphrase(&pwd)?;

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

                    match sig_res {
                        Ok(sig) => {
                            record_sig(fpr, algo_str);
                            return Ok(sig);
                        }
                        Err(e) => {
                            // The passphrase was checked already
                            // (`check_passphrase`): not a passphrase problem.
                            set_last_error(&format!("signing failed for key {}: {}", fpr, e));
                            Err(GfrStatus::ErrorInternal)
                        }
                    }
                })?;

                all_sigs.push(res);
                at_least_one_signer = true;
            }

            if !at_least_one_signer {
                return Err(GfrStatus::ErrorInvalidInput);
            }

            // One detached signature is "one or more Signature packets" (RFC
            // 9580 §10.4): armored, all of them go into a single block.
            let all_out = if ascii_armor {
                // RFC 9580 §6.1: a CRC24 footer only when every signature
                // is v4, never for v6 signatures.
                let opts = armor_opts_for(
                    all_sigs
                        .iter()
                        .all(|sig| sig.signature.version() == pgp::packet::SignatureVersion::V4),
                );
                let mut armored = Vec::new();
                pgp::armor::write(
                    &all_sigs,
                    pgp::armor::BlockType::Signature,
                    &mut armored,
                    opts.headers,
                    opts.include_checksum,
                )
                .record_err(GfrStatus::ErrorArmorFailed)?;
                armored
            } else {
                all_sigs.to_bytes().record_err(GfrStatus::ErrorInternal)?
            };

            output_stream.write_all(&all_out).map_err(|_| {
                crate::cancel::status_or_canceled(channel, GfrStatus::ErrorInternal)
            })?;
            output_stream.flush().map_err(|_| {
                crate::cancel::status_or_canceled(channel, GfrStatus::ErrorInternal)
            })?;

            Ok(SignStreamResultInternal {
                signatures: created_signatures,
            })
        }
    }
}

/// A SHA-512 signature config for `key`. The hash is chosen before the config
/// is built because a v6 salt is sized for it (RFC 9580 §5.2.3, Table 23):
/// overriding `hash_alg` afterwards would keep the salt of the key's default
/// hash and produce a malformed signature.
fn sha512_signature_config<R: rand::CryptoRng + rand::Rng>(
    rng: R,
    key: &impl SigningKey,
    typ: SignatureType,
) -> pgp::errors::Result<SignatureConfig> {
    match key.version() {
        KeyVersion::V6 => SignatureConfig::v6(rng, typ, key.algorithm(), HashAlgorithm::Sha512),
        _ => Ok(SignatureConfig::v4(
            typ,
            key.algorithm(),
            HashAlgorithm::Sha512,
        )),
    }
}

/// Drop the `Hash:` armor headers of a cleartext signed message. rPGP always
/// writes them, but RFC 9580 §6.2.2.3 says they "SHOULD NOT be emitted
/// unless ... the cleartext signed message contains a version 4 signature
/// made using a SHA2-based digest" (for legacy verifiers).
fn strip_cleartext_hash_headers(armored: &str) -> String {
    let mut lines = armored.split_inclusive('\n');
    let mut out = String::with_capacity(armored.len());
    if let Some(begin) = lines.next() {
        out.push_str(begin);
    }
    let mut in_headers = true;
    for line in lines {
        if in_headers {
            if line.trim_end_matches(['\r', '\n']).is_empty() {
                in_headers = false;
            } else if line.starts_with("Hash: ") {
                continue;
            }
        }
        out.push_str(line);
    }
    out
}

/// Dash-escape the cleartext lines that start with "From " (RFC 9580 §7.2:
/// "the line SHOULD be dash-escaped"), which rPGP leaves alone: a mail
/// transport may rewrite them to ">From " and break the signature. The
/// escape is removed before hashing, so the signed text is unchanged.
fn dash_escape_from_lines(armored: &str) -> String {
    let mut out = String::with_capacity(armored.len());
    // 0: BEGIN line and headers, 1: cleartext body, 2: the signature block.
    let mut part = 0;
    for line in armored.split_inclusive('\n') {
        match part {
            0 if line.trim_end_matches(['\r', '\n']).is_empty() => part = 1,
            1 if line.starts_with("-----BEGIN PGP SIGNATURE-----") => part = 2,
            1 if line.starts_with("From ") => out.push_str("- "),
            _ => {}
        }
        out.push_str(line);
    }
    out
}

/// Strip spaces and tabs from the end of every line (RFC 9580 §7.1). A
/// verifier hashes the trimmed text, but rPGP's signer hashes it verbatim, so
/// untrimmed input would yield a signature nobody can verify.
///
/// A carriage return ending the unterminated last line goes too: the armor
/// writer follows the text with a line break, so a reader sees that CR as
/// part of a CRLF line ending, not as text, while the signer would hash it.
fn trim_trailing_whitespace(text: &str) -> String {
    text.split_inclusive('\n')
        .map(|line| {
            if !line.ends_with('\n') {
                return line.trim_end_matches([' ', '\t', '\r']).to_string();
            }
            let body = line.trim_end_matches(['\r', '\n']);
            let end = &line[body.len()..];
            format!("{}{}", body.trim_end_matches([' ', '\t']), end)
        })
        .collect()
}

/// Sign an in-memory buffer with one or more secret keys.
pub fn sign_internal(
    channel: i32,
    name: &str,
    data: &[u8],
    secret_key_blocks: &[&str],
    fetch_cb: Option<GfrPasswordFetchCb>,
    mode: GfrSignMode,
    ascii_armor: bool,
) -> Result<SignResultInternal, GfrStatus> {
    let mut output_data = Vec::new();
    let input_cursor = Cursor::new(data);

    let stream_result = sign_stream_internal(
        channel,
        name,
        input_cursor,
        &mut output_data,
        secret_key_blocks,
        fetch_cb,
        mode,
        ascii_armor,
    )?;

    Ok(SignResultInternal {
        data: output_data,
        signatures: stream_result.signatures,
    })
}

#[cfg(test)]
mod sign_tests {
    //! Signature production in all three modes (§5.4 inline, §7 cleartext,
    //! §10.4 detached), signer selection, and the produce-side policy the
    //! engine is responsible for: strong hashes (§9.5), matching signature
    //! versions (§10.3.2.2), and CRC24-free armor (§6.1).

    use super::*;
    use crate::testutil::{corpus, keys};

    fn sign(data: &[u8], key_block: &str, mode: GfrSignMode, armor: bool) -> SignResultInternal {
        sign_internal(0, "", data, &[key_block], None, mode, armor).expect("sign")
    }

    fn try_sign(
        data: &[u8],
        key_block: &str,
        mode: GfrSignMode,
    ) -> Result<SignResultInternal, GfrStatus> {
        sign_internal(0, "", data, &[key_block], None, mode, true)
    }

    // -- output shape per mode ------------------------------------------------

    #[test]
    fn a_detached_signature_does_not_contain_the_payload() {
        // §10.4: "detached signatures are simply one or more Signature packets
        // stored separately from the data."
        let out = sign(
            b"the payload",
            &keys::V4_SIGN.secret_armored,
            GfrSignMode::Detached,
            true,
        );
        let text = String::from_utf8_lossy(&out.data);
        assert!(text.contains("BEGIN PGP SIGNATURE"));
        assert!(!text.contains("the payload"));
    }

    #[test]
    fn a_cleartext_signature_contains_the_readable_payload() {
        // §7: the whole point of the framework is that the text stays legible.
        let out = sign(
            b"human readable line\n",
            &keys::V4_SIGN.secret_armored,
            GfrSignMode::ClearText,
            true,
        );
        let text = String::from_utf8_lossy(&out.data);
        assert!(text.contains("BEGIN PGP SIGNED MESSAGE"));
        assert!(text.contains("human readable line"));
        assert!(text.contains("BEGIN PGP SIGNATURE"));
    }

    #[test]
    fn an_inline_signature_is_a_pgp_message() {
        let out = sign(
            b"payload",
            &keys::V4_SIGN.secret_armored,
            GfrSignMode::Inline,
            true,
        );
        let text = String::from_utf8_lossy(&out.data);
        assert!(text.contains("BEGIN PGP MESSAGE"));
    }

    #[test]
    fn unarmored_output_is_binary() {
        let out = sign(
            b"payload",
            &keys::V4_SIGN.secret_armored,
            GfrSignMode::Detached,
            false,
        );
        let text = String::from_utf8_lossy(&out.data);
        assert!(!text.contains("BEGIN PGP"), "armor must be off");
        assert!(!out.data.is_empty());
    }

    #[test]
    fn armored_v4_output_carries_a_crc24_footer() {
        // RFC 9580 §6.1 interoperability exception: v4-only armor carries the
        // CRC24 footer (GnuPG 2.4); v6 armor never does (utils::crc24_policy_tests).
        for mode in [
            GfrSignMode::Detached,
            GfrSignMode::Inline,
            GfrSignMode::ClearText,
        ] {
            let out = sign(b"payload", &keys::V4_SIGN.secret_armored, mode, true);
            crate::testutil::assert::armor_has_crc24(&String::from_utf8_lossy(&out.data));
        }
    }

    #[test]
    fn a_v6_signature_carries_no_crc24_footer() {
        // The case §6.1 states as a MUST NOT.
        let out = sign(
            b"payload",
            &keys::V6_SIGN.secret_armored,
            GfrSignMode::Detached,
            true,
        );
        crate::testutil::assert::armor_has_no_crc24(&String::from_utf8_lossy(&out.data));
    }

    // -- reported signature metadata ------------------------------------------

    #[test]
    fn signing_reports_one_signature_per_key() {
        let out = sign(
            b"payload",
            &keys::V4_SIGN.secret_armored,
            GfrSignMode::Detached,
            true,
        );
        assert_eq!(out.signatures.len(), 1);
    }

    #[test]
    fn the_reported_issuer_is_the_signing_subkey() {
        // §10.1.5: good practice is a dedicated signing subkey, and that is
        // what the engine selects, so that is what must be reported.
        let out = sign(
            b"payload",
            &keys::V4_SIGN.secret_armored,
            GfrSignMode::Detached,
            true,
        );
        assert!(
            out.signatures[0]
                .fpr
                .eq_ignore_ascii_case(keys::V4_SIGN.sign_subkey_fpr())
        );
    }

    #[test]
    fn the_reported_mode_matches_the_request() {
        for mode in [
            GfrSignMode::Detached,
            GfrSignMode::Inline,
            GfrSignMode::ClearText,
        ] {
            let out = sign(b"payload", &keys::V4_SIGN.secret_armored, mode, true);
            assert_eq!(out.signatures[0].sig_type, mode);
        }
    }

    #[test]
    fn a_produced_signature_uses_a_strong_hash() {
        // §9.5: MD5, SHA-1 and RIPEMD-160 MUST NOT be used for new signatures.
        for mode in [
            GfrSignMode::Detached,
            GfrSignMode::Inline,
            GfrSignMode::ClearText,
        ] {
            let out = sign(b"payload", &keys::V4_SIGN.secret_armored, mode, true);
            assert!(
                !sig_hash_algo_is_weak(&out.signatures[0].hash_algo),
                "{mode:?} produced a {} signature",
                out.signatures[0].hash_algo
            );
        }
    }

    #[test]
    fn a_produced_signature_records_a_creation_time() {
        // §5.2.3.11: "This subpacket MUST be present in the hashed area."
        let out = sign(
            b"payload",
            &keys::V4_SIGN.secret_armored,
            GfrSignMode::Detached,
            true,
        );
        assert!(out.signatures[0].created_at > 1_600_000_000);
    }

    #[test]
    fn a_produced_signature_does_not_expire_by_default() {
        // §5.2.3.18: absent subpacket means it never expires.
        let out = sign(
            b"payload",
            &keys::V4_SIGN.secret_armored,
            GfrSignMode::Detached,
            true,
        );
        assert_eq!(out.signatures[0].expires_at, 0);
    }

    #[test]
    fn a_produced_signature_names_its_public_key_algorithm() {
        let out = sign(
            b"payload",
            &keys::V4_SIGN.secret_armored,
            GfrSignMode::Detached,
            true,
        );
        assert!(!out.signatures[0].pub_algo.is_empty());
    }

    // -- §10.3.2.2 version correspondence --------------------------------------

    #[test]
    fn a_v4_key_produces_a_v4_signature_packet() {
        let out = sign(
            b"payload",
            &keys::V4_SIGN.secret_armored,
            GfrSignMode::Detached,
            false,
        );
        let sigs = crate::crypto::parse_all_signature_packets(&out.data);
        assert_eq!(sigs.len(), 1);
        assert_eq!(sigs[0].version(), pgp::packet::SignatureVersion::V4);
    }

    #[test]
    fn a_v6_key_produces_a_v6_signature_packet() {
        // "When a version 6 key produces a Signature packet, it MUST produce a
        // version 6 Signature packet."
        let out = sign(
            b"payload",
            &keys::V6_SIGN.secret_armored,
            GfrSignMode::Detached,
            false,
        );
        let sigs = crate::crypto::parse_all_signature_packets(&out.data);
        assert_eq!(sigs.len(), 1);
        assert_eq!(sigs[0].version(), pgp::packet::SignatureVersion::V6);
    }

    #[test]
    fn a_v6_signature_carries_an_issuer_fingerprint_not_a_key_id() {
        // §5.2.3.12: "If the version of that key is greater than 4, this
        // subpacket MUST NOT be included" -- use Issuer Fingerprint instead.
        let out = sign(
            b"payload",
            &keys::V6_SIGN.secret_armored,
            GfrSignMode::Detached,
            false,
        );
        let sigs = crate::crypto::parse_all_signature_packets(&out.data);
        assert!(
            !sigs[0].issuer_fingerprint().is_empty(),
            "a v6 signature must carry an Issuer Fingerprint subpacket"
        );
    }

    #[test]
    fn a_signature_carries_an_issuer_fingerprint_on_v4_too() {
        // §5.2.3.35: "This subpacket SHOULD be included in all signatures."
        let out = sign(
            b"payload",
            &keys::V4_SIGN.secret_armored,
            GfrSignMode::Detached,
            false,
        );
        let sigs = crate::crypto::parse_all_signature_packets(&out.data);
        assert!(!sigs[0].issuer_fingerprint().is_empty());
    }

    // -- signer selection -------------------------------------------------------

    #[test]
    fn a_pinned_signer_is_used() {
        // The `fpr!` prefix protocol on the key block pins which component
        // signs.
        let key = &keys::V4_SIGN;
        let pinned = format!("{}!\n{}", key.primary_fpr, key.secret_armored);
        let out = sign_internal(
            0,
            "",
            b"payload",
            &[&pinned],
            None,
            GfrSignMode::Detached,
            true,
        )
        .expect("sign");
        assert!(out.signatures[0].fpr.eq_ignore_ascii_case(&key.primary_fpr));
    }

    #[test]
    fn a_pinned_unknown_signer_fails_rather_than_falling_back() {
        // Silently signing with a different key than the user pinned would
        // defeat the purpose of pinning.
        let pinned = format!("0000000000000000!\n{}", keys::V4_SIGN.secret_armored);
        assert_eq!(
            sign_internal(
                0,
                "",
                b"payload",
                &[&pinned],
                None,
                GfrSignMode::Detached,
                true
            )
            .err(),
            Some(GfrStatus::ErrorInvalidInput)
        );
    }

    #[test]
    fn a_primary_only_key_signs_with_its_primary() {
        let key = &keys::V4_PRIMARY_ONLY;
        let out = sign(b"payload", &key.secret_armored, GfrSignMode::Detached, true);
        assert!(out.signatures[0].fpr.eq_ignore_ascii_case(&key.primary_fpr));
    }

    #[test]
    fn two_signers_produce_two_signatures() {
        // §5.4: several one-pass signatures may bracket one message.
        let out = sign_internal(
            0,
            "",
            b"payload",
            &[&keys::V4_SIGN.secret_armored, &keys::V6_SIGN.secret_armored],
            None,
            GfrSignMode::Detached,
            false,
        )
        .expect("sign");
        assert_eq!(out.signatures.len(), 2);
        let packets = crate::crypto::parse_all_signature_packets(&out.data);
        assert_eq!(packets.len(), 2);
    }

    #[test]
    fn an_armored_two_signer_detached_signature_is_one_block_with_both_packets() {
        // RFC 9580 §10.4: a detached signature is "one or more Signature
        // packets"; armored, that is one block carrying all of them.
        let out = sign_internal(
            0,
            "",
            b"payload",
            &[&keys::V4_SIGN.secret_armored, &keys::V6_SIGN.secret_armored],
            None,
            GfrSignMode::Detached,
            true,
        )
        .expect("sign");
        let text = String::from_utf8(out.data.clone()).expect("armor is text");
        assert_eq!(text.matches("-----BEGIN PGP SIGNATURE-----").count(), 1);
        assert_eq!(
            crate::crypto::parse_all_signature_packets(&out.data).len(),
            2
        );
    }

    fn cleartext_headers(signed: &[u8]) -> Vec<String> {
        let text = String::from_utf8(signed.to_vec()).expect("cleartext is text");
        text.lines()
            .skip(1)
            .take_while(|line| !line.is_empty())
            .map(str::to_string)
            .collect()
    }

    #[test]
    fn a_v6_cleartext_signature_carries_no_hash_header() {
        // RFC 9580 §6.2.2.3: Hash headers "SHOULD NOT be emitted unless ...
        // the cleartext signed message contains a version 4 signature made
        // using a SHA2-based digest".
        let out = sign_internal(
            0,
            "",
            b"text\n",
            &[&keys::V6_SIGN.secret_armored],
            None,
            GfrSignMode::ClearText,
            true,
        )
        .expect("sign");
        assert!(
            cleartext_headers(&out.data).is_empty(),
            "{:?}",
            cleartext_headers(&out.data)
        );
    }

    #[test]
    fn a_v4_cleartext_signature_keeps_its_hash_header() {
        // The legacy-verifier exception of §6.2.2.3.
        let out = sign_internal(
            0,
            "",
            b"text\n",
            &[&keys::V4_SIGN.secret_armored],
            None,
            GfrSignMode::ClearText,
            true,
        )
        .expect("sign");
        assert_eq!(
            cleartext_headers(&out.data),
            vec!["Hash: SHA512".to_string()]
        );
    }

    #[test]
    fn two_signers_are_reported_with_distinct_issuers() {
        let out = sign_internal(
            0,
            "",
            b"payload",
            &[&keys::V4_SIGN.secret_armored, &keys::V6_SIGN.secret_armored],
            None,
            GfrSignMode::Detached,
            false,
        )
        .expect("sign");
        assert_ne!(out.signatures[0].fpr, out.signatures[1].fpr);
    }

    // -- payload handling ---------------------------------------------------------

    #[test]
    fn a_binary_payload_with_nul_bytes_signs() {
        let payload: Vec<u8> = (0..=255u8).cycle().take(4096).collect();
        let out = sign(
            &payload,
            &keys::V4_SIGN.secret_armored,
            GfrSignMode::Detached,
            true,
        );
        assert_eq!(out.signatures.len(), 1);
    }

    #[test]
    fn a_large_payload_signs() {
        let payload = vec![0x33u8; 512 * 1024];
        let out = sign(
            &payload,
            &keys::V4_SIGN.secret_armored,
            GfrSignMode::Detached,
            true,
        );
        assert_eq!(out.signatures.len(), 1);
    }

    #[test]
    fn a_utf8_payload_signs_in_cleartext_mode() {
        let out = sign(
            "naïve café 日本語\n".as_bytes(),
            &keys::V4_SIGN.secret_armored,
            GfrSignMode::ClearText,
            true,
        );
        let text = String::from_utf8_lossy(&out.data);
        assert!(text.contains("naïve café"));
    }

    #[test]
    fn cleartext_dash_escaping_is_applied() {
        // §7.2: "MUST dash-escape any line commencing in a dash", otherwise
        // the parser would mistake it for an armor header.
        let out = sign(
            b"-----BEGIN SOMETHING-----\nbody\n",
            &keys::V4_SIGN.secret_armored,
            GfrSignMode::ClearText,
            true,
        );
        let text = String::from_utf8_lossy(&out.data);
        assert!(
            text.contains("- -----BEGIN SOMETHING-----"),
            "a leading dash must be escaped:\n{text}"
        );
    }

    // -- failure modes --------------------------------------------------------------

    #[test]
    fn signing_with_a_public_key_block_fails() {
        assert!(
            try_sign(
                b"payload",
                &keys::V4_SIGN.public_armored,
                GfrSignMode::Detached
            )
            .is_err()
        );
    }

    #[test]
    fn signing_with_garbage_fails() {
        assert!(try_sign(b"payload", "not a key", GfrSignMode::Detached).is_err());
    }

    #[test]
    fn signing_with_no_keys_fails() {
        assert!(sign_internal(0, "", b"payload", &[], None, GfrSignMode::Detached, true).is_err());
    }

    #[test]
    fn signing_with_a_locked_key_and_no_callback_fails() {
        // The key needs unlocking and there is no way to ask for a passphrase.
        let mut primary = crate::testutil::keys::cfg(
            crate::types::GfrKeyAlgo::ED25519,
            true,
            false,
            crate::types::GfrOpenPGPKeyVersion::V4,
        );
        primary.has_passphrase = true;
        let locked = crate::keygen::create_key_internal(
            "Locked <locked@example.test>",
            primary,
            &[],
            Some(crate::testutil::cb::pwd_correct),
        )
        .expect("create");
        assert!(try_sign(b"payload", &locked.secret, GfrSignMode::Detached).is_err());
    }

    #[test]
    fn signing_with_a_locked_key_succeeds_given_the_passphrase() {
        let mut primary = crate::testutil::keys::cfg(
            crate::types::GfrKeyAlgo::ED25519,
            true,
            false,
            crate::types::GfrOpenPGPKeyVersion::V4,
        );
        primary.has_passphrase = true;
        let locked = crate::keygen::create_key_internal(
            "Locked <locked@example.test>",
            primary,
            &[],
            Some(crate::testutil::cb::pwd_correct),
        )
        .expect("create");

        let out = sign_internal(
            0,
            "",
            b"payload",
            &[&locked.secret],
            Some(crate::testutil::cb::pwd_correct),
            GfrSignMode::Detached,
            true,
        )
        .expect("sign with a passphrase");
        assert_eq!(out.signatures.len(), 1);
    }

    #[test]
    fn a_cancelled_passphrase_prompt_aborts_signing() {
        let mut primary = crate::testutil::keys::cfg(
            crate::types::GfrKeyAlgo::ED25519,
            true,
            false,
            crate::types::GfrOpenPGPKeyVersion::V4,
        );
        primary.has_passphrase = true;
        let locked = crate::keygen::create_key_internal(
            "Locked <locked@example.test>",
            primary,
            &[],
            Some(crate::testutil::cb::pwd_correct),
        )
        .expect("create");

        match sign_internal(
            0,
            "",
            b"payload",
            &[&locked.secret],
            Some(crate::testutil::cb::pwd_cancelled),
            GfrSignMode::Detached,
            true,
        ) {
            Err(status) => assert_eq!(status, GfrStatus::ErrorCanceled),
            Ok(_) => panic!("a cancelled prompt must not produce a signature"),
        }
    }

    #[test]
    fn signing_never_panics_on_a_malformed_key_block() {
        for block in [
            "",
            "junk",
            corpus::TRUNCATED_ARMOR,
            corpus::CORRUPT_CRC,
            &String::from_utf8_lossy(corpus::GARBAGE),
        ] {
            for mode in [
                GfrSignMode::Detached,
                GfrSignMode::Inline,
                GfrSignMode::ClearText,
            ] {
                let outcome =
                    std::panic::catch_unwind(|| try_sign(b"payload", block, mode).is_ok());
                assert!(outcome.is_ok(), "panicked on {mode:?}");
            }
        }
    }

    #[test]
    fn cancellation_aborts_a_signing_stream() {
        // Cooperative cancellation, checked per chunk by CancellableReader.
        const CH: i32 = 0x0C_90;
        crate::cancel::set_cancelled(CH, true);
        let res = sign_internal(
            CH,
            "",
            &vec![0u8; 1024 * 1024],
            &[&keys::V4_SIGN.secret_armored],
            None,
            GfrSignMode::Detached,
            true,
        );
        crate::cancel::set_cancelled(CH, false);
        assert_eq!(
            res.err(),
            Some(GfrStatus::ErrorCanceled),
            "a cancelled signing operation must abort"
        );
    }
}

#[cfg(test)]
mod passphrase_parity_tests {
    //! Path parity (round four): every signing path reports a passphrase
    //! problem the same way, so the host can re-prompt, and none leaves a
    //! wrong passphrase in the cache.

    use super::*;
    use crate::testutil::{cb, corpus, keys};

    fn forget_key1_passphrase() {
        crate::cache::PASSWORD_CACHE
            .remove_by_fpr(&corpus::KEY1.primary_key.fingerprint().to_string());
        for sub in &corpus::KEY1.secret_subkeys {
            crate::cache::PASSWORD_CACHE.remove_by_fpr(&sub.key.fingerprint().to_string());
        }
    }

    /// Sign `PAYLOAD` with corpus KEY1 (passphrase-protected) on `path`.
    fn attempt(path: &str, pwd: GfrPasswordFetchCb) -> Result<(), GfrStatus> {
        let key = corpus::KEY1_SECRET;
        let payload = b"passphrase parity\n";
        match path {
            "inline" | "cleartext" | "detached" => {
                let mode = match path {
                    "inline" => GfrSignMode::Inline,
                    "cleartext" => GfrSignMode::ClearText,
                    _ => GfrSignMode::Detached,
                };
                sign_internal(0, "", payload, &[key], Some(pwd), mode, true).map(|_| ())
            }
            _ => crate::crypto::encrypt_and_sign_internal(
                0,
                "",
                payload,
                &[&keys::V4_SIGN.public_armored],
                &[key],
                Some(pwd),
                true,
            )
            .map(|_| ()),
        }
    }

    #[test]
    fn encrypt_and_sign_without_a_signer_is_refused() {
        // Path parity: signing refuses an empty signer list; "encrypt and
        // sign" must not quietly produce an unsigned message instead.
        let res = crate::crypto::encrypt_and_sign_internal(
            0,
            "",
            b"unsigned?",
            &[&keys::V4_SIGN.public_armored],
            &[],
            None,
            true,
        );
        assert_eq!(res.err(), Some(GfrStatus::ErrorInvalidInput));
    }

    #[test]
    fn every_signing_path_reports_passphrase_problems_alike() {
        for path in ["inline", "cleartext", "detached", "encrypt+sign"] {
            forget_key1_passphrase();
            assert_eq!(
                attempt(path, cb::pwd_wrong).err(),
                Some(GfrStatus::ErrorBadPassphrase),
                "{path}: wrong passphrase"
            );
            // The wrong passphrase must not have been kept: the right one,
            // asked for again, works.
            assert_eq!(attempt(path, cb::pwd_corpus), Ok(()), "{path}: retry");
            forget_key1_passphrase();
            assert_eq!(
                attempt(path, cb::pwd_cancelled).err(),
                Some(GfrStatus::ErrorCanceled),
                "{path}: cancel"
            );
        }
    }
}

#[cfg(test)]
mod sign_interop_tests {
    //! Signatures GF produces, consumed by GnuPG and Sequoia. Each case
    //! names the RFC 9580 rule the produced object follows; an interop pass
    //! alone does not prove conformance, the packet checks in `sign_tests` do.

    use super::*;
    use crate::testutil::{interop, keys};
    use crate::types::{GfrKeyAlgo, GfrOpenPGPKeyVersion};

    const TEXT: &[u8] = b"interop text   \nwith a trailing tab\t\n- dash line\n";

    fn sign(signers: &[&str], mode: GfrSignMode, data: &[u8]) -> Vec<u8> {
        sign_internal(0, "", data, signers, None, mode, true)
            .expect("sign")
            .data
    }

    #[test]
    fn gnupg_verifies_our_armored_two_signer_detached_signature() {
        // §10.4: one detached signature object, one armor block, both packets.
        let Some(gpg) = interop::Gnupg::new() else {
            return;
        };
        let second = keys::generate(
            "P256 Second <p256-second@example.test>",
            &keys::cfg(GfrKeyAlgo::NISTP256, true, false, GfrOpenPGPKeyVersion::V4),
            &[],
        );
        let second_secret = second
            .to_armored_string(crate::utils::armor_opts())
            .unwrap();
        let second_public = SignedPublicKey::from(second)
            .to_armored_string(crate::utils::armor_opts())
            .unwrap();
        gpg.import(&keys::V4_NISTP256.public_armored);
        gpg.import(&second_public);

        let sig = sign(
            &[&keys::V4_NISTP256.secret_armored, &second_secret],
            GfrSignMode::Detached,
            TEXT,
        );
        let out = gpg.verify(&sig, Some(TEXT));
        let stderr = String::from_utf8_lossy(&out.stderr);
        assert!(out.status.success(), "{stderr}");
        assert_eq!(stderr.matches("Good signature").count(), 2, "{stderr}");
    }

    #[test]
    fn gnupg_and_sequoia_verify_our_cleartext_with_a_dash_escaped_from_line() {
        // §7.2: a line starting with "From " is dash-escaped (round three);
        // the escape is not part of the signed text for either verifier.
        let (Some(gpg), Some(sq)) = (interop::Gnupg::new(), interop::Sq::new()) else {
            return;
        };
        let text = b"From the top\nnot From here\n";
        let signed = sign(
            &[&keys::V4_NISTP256.secret_armored],
            GfrSignMode::ClearText,
            text,
        );
        assert!(String::from_utf8_lossy(&signed).contains("\n- From the top\n"));
        gpg.import(&keys::V4_NISTP256.public_armored);
        let out = gpg.verify(&signed, None);
        assert!(
            out.status.success(),
            "{}",
            String::from_utf8_lossy(&out.stderr)
        );
        let out = sq.verify(&keys::V4_NISTP256.public_armored, &signed, None);
        assert!(
            out.status.success(),
            "{}",
            String::from_utf8_lossy(&out.stderr)
        );
    }

    #[test]
    fn sequoia_reads_our_v6_revocation_certificate_as_a_revoked_key() {
        // RFC 9580 §10.1.2: a v6 revocation certificate is the primary key
        // followed by its revocation signature (round three, R3-4).
        let Some(sq) = interop::Sq::new() else {
            return;
        };
        let rev = crate::key::generate_key_rev_cert_internal(
            0,
            &keys::V6_SIGN.secret_armored,
            crate::types::GfrRevocationCode::Retired,
            Some("interop"),
            None,
        )
        .expect("revocation certificate");
        let out = sq.inspect(rev.as_bytes());
        let text = format!(
            "{}{}",
            String::from_utf8_lossy(&out.stdout),
            String::from_utf8_lossy(&out.stderr)
        );
        assert!(out.status.success(), "{text}");
        assert!(text.to_lowercase().contains("revoked"), "{text}");
        assert!(
            text.to_uppercase().contains(&keys::V6_SIGN.primary_fpr),
            "{text}"
        );
    }

    #[test]
    fn gnupg_and_sequoia_verify_our_v4_cleartext_signature_over_trailing_whitespace() {
        // §7.1: trailing whitespace is not part of the signed text.
        let (Some(gpg), Some(sq)) = (interop::Gnupg::new(), interop::Sq::new()) else {
            return;
        };
        let signed = sign(
            &[&keys::V4_NISTP256.secret_armored],
            GfrSignMode::ClearText,
            TEXT,
        );
        gpg.import(&keys::V4_NISTP256.public_armored);
        let out = gpg.verify(&signed, None);
        assert!(
            out.status.success(),
            "{}",
            String::from_utf8_lossy(&out.stderr)
        );
        let out = sq.verify(&keys::V4_NISTP256.public_armored, &signed, None);
        assert!(
            out.status.success(),
            "{}",
            String::from_utf8_lossy(&out.stderr)
        );
    }

    #[test]
    fn sequoia_verifies_our_v6_cleartext_and_detached_signatures() {
        // §5.2.3: the v6 salt matches SHA2-512; §6.2.2.3: no Hash header.
        let Some(sq) = interop::Sq::new() else {
            return;
        };
        let cert = &keys::V6_SIGN.public_armored;
        let signed = sign(
            &[&keys::V6_SIGN.secret_armored],
            GfrSignMode::ClearText,
            TEXT,
        );
        let out = sq.verify(cert, &signed, None);
        assert!(
            out.status.success(),
            "{}",
            String::from_utf8_lossy(&out.stderr)
        );

        let sig = sign(
            &[&keys::V6_SIGN.secret_armored],
            GfrSignMode::Detached,
            TEXT,
        );
        let out = sq.verify(cert, &sig, Some(TEXT));
        assert!(
            out.status.success(),
            "{}",
            String::from_utf8_lossy(&out.stderr)
        );
    }
}
