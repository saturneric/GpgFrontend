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

use crate::utils::{armor_opts, password_from_zeroizing_bytes};

use pgp::{
    packet::{SignatureConfig, SignatureType, Subpacket, SubpacketData},
    types::{KeyVersion, Timestamp},
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
                builder.to_armored_writer(&mut rng, armor_opts(), &mut output_stream)
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

            let msg = CleartextSignedMessage::new_many(text_str, |normalized_text| -> pgp::errors::Result<Vec<pgp::packet::Signature>> {
                let mut sigs = Vec::new();

                for (skey, target) in keys_ref.iter() {
                    let sig_result = with_signing_key(skey, target.as_deref(), |selected_key| {
                        let fpr = selected_key.fpr();
                        let is_enc = selected_key.is_encrypted();
                        let algo_str = algo_to_string_simple(selected_key.algorithm());
                        let pwd = fetch_pwd(is_enc, &fpr)?;

                        let normalized_bytes = normalized_text.as_bytes();
                        let cursor = Cursor::new(normalized_bytes);

                        let signature = match selected_key {
                            SelectedKey::Primary(k) => {
                                let hashed_subpackets = vec![
                                    Subpacket::regular(SubpacketData::SignatureCreationTime(
                                        Timestamp::now(),
                                    ))
                                    .map_err(|_| GfrStatus::ErrorInternal)?,
                                    Subpacket::regular(SubpacketData::IssuerFingerprint(
                                        k.fingerprint(),
                                    ))
                                    .map_err(|_| GfrStatus::ErrorInternal)?,
                                ];
                                let mut config = SignatureConfig::from_key(
                                    &mut rng, k, SignatureType::Text,
                                )
                                .map_err(|_| GfrStatus::ErrorInternal)?;
                                config.hashed_subpackets = hashed_subpackets;
                                // Pin the cleartext signing hash to SHA-512 so the reported metadata is
                                // truthful and never a weak digest (RFC 9580 §9.5).
                                config.hash_alg = HashAlgorithm::Sha512;
                                if k.version() <= KeyVersion::V4 {
                                    config.unhashed_subpackets = vec![Subpacket::regular(
                                        SubpacketData::IssuerKeyId(k.legacy_key_id()),
                                    )
                                    .map_err(|_| GfrStatus::ErrorInternal)?];
                                }
                                config.sign(k, &pwd, cursor).map_err(|e| {
                                    log::warn!("Cleartext signing failed for {}. Evicting bad password.", fpr);
                                    PASSWORD_CACHE.remove_by_fpr(&fpr);
                                    set_last_error(&format!("signing failed for key {}: {}", fpr, e));
                                    GfrStatus::ErrorBadPassphrase
                                })?
                            }
                            SelectedKey::Sub(k) => {
                                let hashed_subpackets = vec![
                                    Subpacket::regular(SubpacketData::SignatureCreationTime(
                                        Timestamp::now(),
                                    ))
                                    .map_err(|_| GfrStatus::ErrorInternal)?,
                                    Subpacket::regular(SubpacketData::IssuerFingerprint(
                                        k.fingerprint(),
                                    ))
                                    .map_err(|_| GfrStatus::ErrorInternal)?,
                                ];
                                let mut config = SignatureConfig::from_key(
                                    &mut rng, k, SignatureType::Text,
                                )
                                .map_err(|_| GfrStatus::ErrorInternal)?;
                                config.hashed_subpackets = hashed_subpackets;
                                // Pin the cleartext signing hash to SHA-512 so the reported metadata is
                                // truthful and never a weak digest (RFC 9580 §9.5).
                                config.hash_alg = HashAlgorithm::Sha512;
                                if k.version() <= KeyVersion::V4 {
                                    config.unhashed_subpackets = vec![Subpacket::regular(
                                        SubpacketData::IssuerKeyId(k.legacy_key_id()),
                                    )
                                    .map_err(|_| GfrStatus::ErrorInternal)?];
                                }
                                config.sign(k, &pwd, cursor).map_err(|e| {
                                    log::warn!("Cleartext signing failed for {}. Evicting bad password.", fpr);
                                    PASSWORD_CACHE.remove_by_fpr(&fpr);
                                    set_last_error(&format!("signing failed for key {}: {}", fpr, e));
                                    GfrStatus::ErrorBadPassphrase
                                })?
                            }
                        };

                        record_sig_fn(fpr, algo_str);
                        Ok(signature)
                    });

                    sigs.push(sig_result.map_err(|e| e.to_string())?);
                }

                Ok(sigs)
            })
            .map_err(|_| GfrStatus::ErrorInternal)?;

            let out = msg
                .to_armored_string(armor_opts())
                .map_err(|_| GfrStatus::ErrorArmorFailed)?
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

            let mut all_out = Vec::new();
            let mut at_least_one_signer = false;

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

                    match sig_res {
                        Ok(sig) => {
                            record_sig(fpr, algo_str);
                            let out = if ascii_armor {
                                sig.to_armored_bytes(armor_opts())
                                    .record_err(GfrStatus::ErrorArmorFailed)?
                            } else {
                                sig.to_bytes().record_err(GfrStatus::ErrorInternal)?
                            };
                            return Ok(out);
                        }
                        Err(e) => {
                            log::warn!("Signing failed for {}. Evicting bad password.", fpr);
                            PASSWORD_CACHE.remove_by_fpr(&fpr);
                            set_last_error(&format!("signing failed for key {}: {}", fpr, e));
                            Err(GfrStatus::ErrorBadPassphrase)
                        }
                    }
                })?;

                all_out.extend_from_slice(&res);
                at_least_one_signer = true;
            }

            if !at_least_one_signer {
                return Err(GfrStatus::ErrorInvalidInput);
            }

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
        let out = sign(b"the payload", &keys::V4_SIGN.secret_armored, GfrSignMode::Detached, true);
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
        let out = sign(b"payload", &keys::V4_SIGN.secret_armored, GfrSignMode::Inline, true);
        let text = String::from_utf8_lossy(&out.data);
        assert!(text.contains("BEGIN PGP MESSAGE"));
    }

    #[test]
    fn unarmored_output_is_binary() {
        let out = sign(b"payload", &keys::V4_SIGN.secret_armored, GfrSignMode::Detached, false);
        let text = String::from_utf8_lossy(&out.data);
        assert!(!text.contains("BEGIN PGP"), "armor must be off");
        assert!(!out.data.is_empty());
    }

    #[test]
    fn armored_output_carries_no_crc24_footer() {
        // §6.1: generating the footer is discouraged and forbidden for v6.
        for mode in [GfrSignMode::Detached, GfrSignMode::Inline, GfrSignMode::ClearText] {
            let out = sign(b"payload", &keys::V4_SIGN.secret_armored, mode, true);
            crate::testutil::assert::armor_has_no_crc24(&String::from_utf8_lossy(&out.data));
        }
    }

    #[test]
    fn a_v6_signature_carries_no_crc24_footer() {
        // The case §6.1 states as a MUST NOT.
        let out = sign(b"payload", &keys::V6_SIGN.secret_armored, GfrSignMode::Detached, true);
        crate::testutil::assert::armor_has_no_crc24(&String::from_utf8_lossy(&out.data));
    }

    // -- reported signature metadata ------------------------------------------

    #[test]
    fn signing_reports_one_signature_per_key() {
        let out = sign(b"payload", &keys::V4_SIGN.secret_armored, GfrSignMode::Detached, true);
        assert_eq!(out.signatures.len(), 1);
    }

    #[test]
    fn the_reported_issuer_is_the_signing_subkey() {
        // §10.1.5: good practice is a dedicated signing subkey, and that is
        // what the engine selects, so that is what must be reported.
        let out = sign(b"payload", &keys::V4_SIGN.secret_armored, GfrSignMode::Detached, true);
        assert!(
            out.signatures[0]
                .fpr
                .eq_ignore_ascii_case(keys::V4_SIGN.sign_subkey_fpr())
        );
    }

    #[test]
    fn the_reported_mode_matches_the_request() {
        for mode in [GfrSignMode::Detached, GfrSignMode::Inline, GfrSignMode::ClearText] {
            let out = sign(b"payload", &keys::V4_SIGN.secret_armored, mode, true);
            assert_eq!(out.signatures[0].sig_type, mode);
        }
    }

    #[test]
    fn a_produced_signature_uses_a_strong_hash() {
        // §9.5: MD5, SHA-1 and RIPEMD-160 MUST NOT be used for new signatures.
        for mode in [GfrSignMode::Detached, GfrSignMode::Inline, GfrSignMode::ClearText] {
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
        let out = sign(b"payload", &keys::V4_SIGN.secret_armored, GfrSignMode::Detached, true);
        assert!(out.signatures[0].created_at > 1_600_000_000);
    }

    #[test]
    fn a_produced_signature_does_not_expire_by_default() {
        // §5.2.3.18: absent subpacket means it never expires.
        let out = sign(b"payload", &keys::V4_SIGN.secret_armored, GfrSignMode::Detached, true);
        assert_eq!(out.signatures[0].expires_at, 0);
    }

    #[test]
    fn a_produced_signature_names_its_public_key_algorithm() {
        let out = sign(b"payload", &keys::V4_SIGN.secret_armored, GfrSignMode::Detached, true);
        assert!(!out.signatures[0].pub_algo.is_empty());
    }

    // -- §10.3.2.2 version correspondence --------------------------------------

    #[test]
    fn a_v4_key_produces_a_v4_signature_packet() {
        let out = sign(b"payload", &keys::V4_SIGN.secret_armored, GfrSignMode::Detached, false);
        let sigs = crate::crypto::parse_all_signature_packets(&out.data);
        assert_eq!(sigs.len(), 1);
        assert_eq!(sigs[0].version(), pgp::packet::SignatureVersion::V4);
    }

    #[test]
    fn a_v6_key_produces_a_v6_signature_packet() {
        // "When a version 6 key produces a Signature packet, it MUST produce a
        // version 6 Signature packet."
        let out = sign(b"payload", &keys::V6_SIGN.secret_armored, GfrSignMode::Detached, false);
        let sigs = crate::crypto::parse_all_signature_packets(&out.data);
        assert_eq!(sigs.len(), 1);
        assert_eq!(sigs[0].version(), pgp::packet::SignatureVersion::V6);
    }

    #[test]
    fn a_v6_signature_carries_an_issuer_fingerprint_not_a_key_id() {
        // §5.2.3.12: "If the version of that key is greater than 4, this
        // subpacket MUST NOT be included" -- use Issuer Fingerprint instead.
        let out = sign(b"payload", &keys::V6_SIGN.secret_armored, GfrSignMode::Detached, false);
        let sigs = crate::crypto::parse_all_signature_packets(&out.data);
        assert!(
            !sigs[0].issuer_fingerprint().is_empty(),
            "a v6 signature must carry an Issuer Fingerprint subpacket"
        );
    }

    #[test]
    fn a_signature_carries_an_issuer_fingerprint_on_v4_too() {
        // §5.2.3.35: "This subpacket SHOULD be included in all signatures."
        let out = sign(b"payload", &keys::V4_SIGN.secret_armored, GfrSignMode::Detached, false);
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
        assert!(
            sign_internal(0, "", b"payload", &[&pinned], None, GfrSignMode::Detached, true)
                .is_err()
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
        let out = sign(&payload, &keys::V4_SIGN.secret_armored, GfrSignMode::Detached, true);
        assert_eq!(out.signatures.len(), 1);
    }

    #[test]
    fn a_large_payload_signs() {
        let payload = vec![0x33u8; 512 * 1024];
        let out = sign(&payload, &keys::V4_SIGN.secret_armored, GfrSignMode::Detached, true);
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
        assert!(try_sign(b"payload", &keys::V4_SIGN.public_armored, GfrSignMode::Detached).is_err());
    }

    #[test]
    fn signing_with_garbage_fails() {
        assert!(try_sign(b"payload", "not a key", GfrSignMode::Detached).is_err());
    }

    #[test]
    fn signing_with_no_keys_fails() {
        assert!(
            sign_internal(0, "", b"payload", &[], None, GfrSignMode::Detached, true).is_err()
        );
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
            for mode in [GfrSignMode::Detached, GfrSignMode::Inline, GfrSignMode::ClearText] {
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
        assert!(res.is_err(), "a cancelled signing operation must abort");
    }
}
