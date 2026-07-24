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

use std::ffi::c_char;

use zeroize::Zeroizing;

use crate::{host::gfc_secure_free_cstr, utils::password_from_zeroizing_bytes};

use super::*;

/// Upper bound on how many nested compression layers a message may carry before
/// decryption is aborted. A doubly-compressed message is legitimate and must be
/// peeled fully (see step 6), but an unbounded chain of compression packets is a
/// cheap denial-of-service, so the depth is capped (RFC 9580 §13.14). The
/// decompressed *output* size is bounded separately by
/// [`MAX_DECOMPRESSED_OUTPUT_BYTES`].
const MAX_COMPRESSION_LAYERS: u8 = 16;

/// Inspect the ESK (Encrypted Session Key) packets of a parsed message.
///
/// Returns `(has_pkesk, has_skesk, recipients)` where `has_pkesk` indicates
/// public-key session keys are present and `has_skesk` indicates symmetric
/// (password-based) session keys. No decryption is performed.
fn analyze_encrypted_envelope(
    parsed_message: &Message,
) -> Result<(bool, bool, Vec<RecipientResultInternal>), GfrStatus> {
    let mut has_pkesk = false;
    let mut has_skesk = false;
    let mut recipients = Vec::new();

    if let Message::Encrypted { esk, edata, .. } = parsed_message {
        // RFC 9580 §5.7 / §13.7: the legacy Symmetrically Encrypted Data packet
        // (SED, Tag 9) has no integrity protection and is malleable. rPGP's
        // high-level `Message::decrypt` opts into legacy SED handling
        // (`DecryptionOptions::enable_legacy`), so we must reject it here —
        // before any decrypt call — to guarantee that unauthenticated plaintext
        // is never released to the caller.
        if edata.tag() == Tag::SymEncryptedData {
            set_last_error(
                "refusing to decrypt a legacy Symmetrically Encrypted Data packet: it is not \
                 integrity-protected and is malleable (RFC 9580 §13.7)",
            );
            return Err(GfrStatus::ErrorInvalidData);
        }

        for e in esk {
            match e {
                Esk::PublicKeyEncryptedSessionKey(pkesk) => {
                    has_pkesk = true;

                    // v3 PKESK identifies the recipient by its 8-byte key ID;
                    // v6 PKESK identifies it by full fingerprint (`pkesk.id()`
                    // errors for v6, so fall back to `pkesk.fingerprint()`).
                    // Either form resolves a secret key on the C++ side, which
                    // looks up by key-id OR fingerprint. A wildcard/anonymous
                    // recipient carries neither and is simply not listed.
                    let recipient_id = match pkesk.id() {
                        Ok(id) => Some(id.to_string()),
                        Err(_) => pkesk.fingerprint().ok().flatten().map(|fp| fp.to_string()),
                    };

                    if let Some(key_id) = recipient_id {
                        let algo = pkesk
                            .algorithm()
                            .map(algo_to_string_simple)
                            .unwrap_or_default();

                        recipients.push(RecipientResultInternal {
                            key_id,
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

    set_last_error("message has no encrypted session-key packet (not an encrypted message)");
    Err(GfrStatus::ErrorInvalidData)
}

/// Decrypt a symmetrically-encrypted message using a user-supplied passphrase.
///
/// Always bypasses the password cache and always asks for a new password
/// (`ask_for_new: true`) because symmetric encryption has no fingerprint to
/// key the cache on, so a stale cache entry would silently fail.
fn decrypt_message_with_password(
    channel: i32,
    parsed_message: Message,
    fetch_pwd_cb: Option<GfrPasswordFetchCb>,
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
    )?;

    if password.is_empty() {
        return Err(GfrStatus::ErrorBadPassphrase);
    }

    let msg_pw = password_from_zeroizing_bytes(password);

    parsed_message
        .decrypt_with_password(&msg_pw)
        .record_err(GfrStatus::ErrorDecryptionFailed)
}

/// Decrypt and optionally verify a stream in a single pipeline.
///
/// Handles both symmetric (passphrase) and asymmetric (public-key) ciphertext
/// by inspecting the ESK packets first. Verification is performed only when
/// `fetch_pubkey_cb` is `Some` and the decrypted payload contains signatures;
/// passing `None` skips verification entirely without error.
///
/// Anonymous recipients (`key_id = "0000000000000000"`) are accepted — the
/// message was encrypted without embedding recipient key IDs (hidden recipients).
pub fn decrypt_and_verify_stream_internal<R, W>(
    channel: i32,
    input_stream: R,
    mut output_stream: W,
    ascii_armor: bool,
    fetch_seckey_cb: Option<GfrSecretKeyFetchCb>,
    fetch_pwd_cb: Option<GfrPasswordFetchCb>,
    fetch_pubkey_cb: Option<GfrPublicKeyFetchCb>,
    user_data: *mut c_void,
) -> Result<DecryptAndVerifyStreamResultInternal, GfrStatus>
where
    R: Read + Send + Sync + fmt::Debug,
    W: Write + Send + Sync,
{
    // Wrap the source so a user cancel request aborts the streaming read.
    let buf_input = BufReader::new(crate::cancel::CancellableReader::new(channel, input_stream));

    // 1. Parse PGP outer envelope (consumes headers only)
    let parsed_message = if ascii_armor {
        Message::from_armor(buf_input)
            .record_err_with(|| {
                crate::cancel::status_or_canceled(channel, GfrStatus::ErrorInvalidInput)
            })?
            .0
    } else {
        Message::from_reader(buf_input)
            .record_err_with(|| {
                crate::cancel::status_or_canceled(channel, GfrStatus::ErrorInvalidInput)
            })?
            .0
    };

    let (has_pkesk, has_skesk, mut recipients) = analyze_encrypted_envelope(&parsed_message)?;
    // Gate on the presence of a session-key packet, not on the recipient list:
    // a v6 PKESK addressed to a hidden (wildcard) recipient carries no
    // identifier, so `recipients` can be empty even though the message is a
    // genuine public-key-encrypted message (`has_pkesk`).
    if !has_skesk && !has_pkesk {
        set_last_error("input is not an OpenPGP encrypted message");
        return Err(GfrStatus::ErrorInvalidData);
    }

    let mut decrypted: Message;
    if has_skesk {
        debug!("Message is encrypted with a passphrase. Attempting password-based decryption.");
        decrypted = decrypt_message_with_password(channel, parsed_message, fetch_pwd_cb)?;
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
                    unsafe { gfc_secure_free_cstr(c_key_block as *mut c_char) };

                    if target_skey.is_some() {
                        break;
                    }
                }
            }
        }

        let skey = target_skey.ok_or_else(|| {
            set_last_error("no available secret key matches any recipient of this message");
            GfrStatus::ErrorNoKey
        })?;

        // 4. Check if unlocking is needed
        let mut needs_password = false;
        let primary_id = skey.primary_key.legacy_key_id().to_string();
        let primary_fpr = skey.primary_key.fingerprint().to_string();
        let is_anonymous = |id: &str| id == "0000000000000000";
        // A recipient identifier matches a component by either its 8-byte key ID
        // (v3 PKESK) or its full fingerprint (v6 PKESK); the wildcard matches all.
        let id_matches = |rec: &str, key_id: &str, fpr: &str| {
            is_anonymous(rec) || rec.eq_ignore_ascii_case(key_id) || rec.eq_ignore_ascii_case(fpr)
        };
        let mut target_fpr_for_pwd = primary_fpr.clone();

        if id_matches(&matched_recipient_id, &primary_id, &primary_fpr)
            && matches!(skey.primary_key.secret_params(), SecretParams::Encrypted(_))
        {
            needs_password = true;
        }

        if !needs_password {
            for subkey in &skey.secret_subkeys {
                let subkey_id = subkey.key.legacy_key_id().to_string();
                let subkey_fpr = subkey.key.fingerprint().to_string();
                if id_matches(&matched_recipient_id, &subkey_id, &subkey_fpr)
                    && matches!(subkey.key.secret_params(), SecretParams::Encrypted(_))
                {
                    needs_password = true;
                    target_fpr_for_pwd = subkey_fpr;
                    break;
                }
            }
        }

        let mut password = Zeroizing::new(Vec::<u8>::new());
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
            )?;
        } else {
            debug!("Target secret key is unlocked. Bypassing password callback.");
        }

        // 5. Initialize streaming decryption
        let pwd_fn = password_from_zeroizing_bytes(password);
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
    //
    // Peel *every* compression layer, not just one: a doubly-compressed message
    // is legitimate and would otherwise leave an inner compressed blob in the
    // output. The layer count is capped so an unbounded chain cannot drive a
    // denial-of-service, and the decompressed output is bounded by the size
    // limiter in step 8 so a single high-ratio "compression bomb" cannot exhaust
    // memory/disk (RFC 9580 §13.14).
    let mut was_compressed = false;
    let mut layers: u8 = 0;
    while decrypted.is_compressed() {
        layers += 1;
        if layers > MAX_COMPRESSION_LAYERS {
            set_last_error(
                "refusing to decompress: too many nested compression layers (possible \
                 compression bomb, RFC 9580 §13.14)",
            );
            return Err(GfrStatus::ErrorInvalidData);
        }
        decrypted = decrypted.decompress().into_gfr()?;
        was_compressed = true;
    }

    // 7. Extract filename
    let mut filename = String::new();
    if let Message::Literal { ref reader, .. } = decrypted {
        let header = reader.data_header();
        filename = String::from_utf8_lossy(header.file_name()).to_string();
    }

    // 8. Stream Execution
    //
    // This is the bulk transfer and the main cancellation checkpoint: the
    // `CancellableReader` deep in `decrypted` aborts the read once a cancel is
    // requested for this channel. `into_gfr` cannot see the channel, so check
    // the flag first and surface `ErrorCanceled`; otherwise fall through to the
    // normal error mapping (which also records a detailed message).
    //
    // A message that was decompressed is streamed through a size limiter so a
    // decompression bomb cannot exhaust memory/disk (RFC 9580 §13.14). Non-
    // compressed payloads are copied unbounded (their size equals the ciphertext,
    // which rPGP already bounds), so legitimate large files are unaffected.
    //
    // NOTE: the withhold-plaintext-until-integrity guarantee relies on rPGP's
    // default `Seipdv1ReadMode::CheckFirst` (v4/MDC) and per-chunk AEAD auth (v2);
    // do not switch `decrypt`/`decrypt_with_password` to a streaming read mode
    // without re-checking that plaintext is never released before authentication.
    let copy_result = if was_compressed {
        let mut limited = LimitedReader::new(&mut decrypted, MAX_DECOMPRESSED_OUTPUT_BYTES);
        std::io::copy(&mut limited, &mut output_stream)
    } else {
        std::io::copy(&mut decrypted, &mut output_stream)
    };
    if copy_result.is_err() && crate::cancel::is_cancelled(channel) {
        return Err(GfrStatus::ErrorCanceled);
    }
    copy_result.into_gfr()?;
    output_stream.flush().into_gfr()?;

    // ==========================================
    // OPTIONAL: VERIFICATION PHASE
    // ==========================================
    let mut signatures = Vec::new();
    let mut is_verified = false;

    // We only process signatures if the message is actually signed AND the caller requested verification
    if decrypted.is_signed() && fetch_pubkey_cb.is_some() {
        // Attribute exactly like the standalone inline verifier in `verify.rs`,
        // through the shared path — no divergent copy. `signature_entries_from_message`
        // builds one entry per signature packet (issuer fingerprint→key-id
        // fallback, no fingerprint-only de-dup), and `attribute_entries` ties each
        // `Valid` to the specific index that verifies under a *usable* key (primary
        // not revoked §5.2.1.11 / subkey signing-capable and not revoked §5.2.1.12),
        // then applies the §9.5 weak-hash and §5.2.3.18 expiry gates. Previously this
        // path carried its own builder (fingerprint-only de-dup, no key-id fallback),
        // dropping distinct or legacy-key-id signatures the standalone verifier keeps.
        signatures = signature_entries_from_message(&decrypted);
        let certs = fetch_certs_for_signatures(&signatures, fetch_pubkey_cb, user_data);
        attribute_entries(&mut signatures, &certs, &mut is_verified, |i, cert| {
            verify_index_under_usable_key(&decrypted, i, cert)
        });
    }

    Ok(DecryptAndVerifyStreamResultInternal {
        filename,
        recipients,
        is_verified,
        signatures,
    })
}

/// Decrypt an encrypted archive and unpack its tar contents to a directory.
///
/// Decrypts into an anonymous temporary file (never touches disk as named data)
/// then unpacks the tar in a second pass. The temp file is dropped automatically
/// after extraction completes.
pub fn decrypt_and_verify_archive_internal(
    channel: i32,
    in_file_path: &str,
    out_dir_path: &str,
    ascii_armor: bool,
    fetch_seckey_cb: Option<crate::types::GfrSecretKeyFetchCb>,
    fetch_pwd_cb: Option<crate::types::GfrPasswordFetchCb>,
    fetch_pubkey_cb: Option<crate::types::GfrPublicKeyFetchCb>,
    user_data: *mut std::ffi::c_void,
) -> Result<DecryptAndVerifyResultInternal, crate::types::GfrStatus> {
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
        set_last_error(&format!("cannot open encrypted input file: {}", e));
        crate::types::GfrStatus::ErrorIo
    })?;

    // 3. Create an anonymous temporary file as a secure buffer for decrypted data
    let mut temp_archive = tempfile::tempfile().map_err(|e| {
        log::error!("Failed to create temp file for decryption: {}", e);
        set_last_error(&format!(
            "cannot create temporary file for decryption: {}",
            e
        ));
        crate::types::GfrStatus::ErrorIo
    })?;

    // 4. Perform stream decryption and signature verification
    log::info!("Decrypting file into temporary archive...");
    let stream_result = decrypt_and_verify_stream_internal(
        channel,
        in_file,
        &mut temp_archive,
        ascii_armor,
        fetch_seckey_cb,
        fetch_pwd_cb,
        fetch_pubkey_cb,
        user_data,
    )?;

    // 5. Move the temporary file cursor back to the beginning, preparing for extraction
    temp_archive
        .seek(SeekFrom::Start(0))
        .record_err(crate::types::GfrStatus::ErrorIo)?;

    // 6. Perform Tar extraction operation
    log::info!(
        "Unpacking tar archive to target directory: {}",
        out_dir_path
    );
    let mut archive = tar::Archive::new(temp_archive);
    archive.unpack(out_dir).map_err(|e| {
        log::error!("Failed to unpack tar archive: {}", e);
        set_last_error(&format!("decrypted data is not a valid archive: {}", e));
        crate::types::GfrStatus::ErrorInvalidData // Extraction failure may indicate the content is not a valid tar archive
    })?;

    // 7. Assemble the return result (pure file stream operation, so payload data is empty)
    Ok(DecryptAndVerifyResultInternal {
        data: Vec::new(),
        filename: stream_result.filename,
        recipients: stream_result.recipients,
        is_verified: stream_result.is_verified,
        signatures: stream_result.signatures,
    })
}

/// Decrypt an in-memory buffer, auto-detecting ASCII armor.
///
/// Checks for the `-----BEGIN PGP MESSAGE-----` header to decide the format;
/// no `ascii_armor` parameter is needed on this variant.
pub fn decrypt_internal(
    channel: i32,
    encrypted_data: &[u8],
    fetch_seckey_cb: Option<GfrSecretKeyFetchCb>,
    fetch_pwd_cb: Option<GfrPasswordFetchCb>,
    user_data: *mut c_void,
) -> Result<DecryptResultInternal, GfrStatus> {
    let mut output_data = Vec::new();

    // Auto-detect ASCII armor by checking for the standard PGP header.
    // If it's not valid UTF-8, it's definitely a binary packet.
    let is_armor = std::str::from_utf8(encrypted_data)
        .map(|s| s.trim_start().starts_with("-----BEGIN PGP MESSAGE-----"))
        .unwrap_or(false);

    // Wrap the in-memory byte slice into a Read stream
    let input_cursor = Cursor::new(encrypted_data);

    // Delegate all the heavy lifting to the stream implementation
    let stream_result = decrypt_and_verify_stream_internal(
        channel,
        input_cursor,
        &mut output_data, // Passes as Write stream
        is_armor,
        fetch_seckey_cb,
        fetch_pwd_cb,
        None, // fetch_pubkey_cb is not needed for decryption-only
        user_data,
    )?;

    Ok(DecryptResultInternal {
        data: output_data,
        filename: stream_result.filename,
        recipients: stream_result.recipients,
    })
}

/// Decrypt and verify a combined encrypt+sign in-memory buffer.
pub fn decrypt_and_verify_internal(
    channel: i32,
    encrypted_data: &[u8],
    fetch_seckey_cb: Option<GfrSecretKeyFetchCb>,
    fetch_pwd_cb: Option<GfrPasswordFetchCb>,
    fetch_pubkey_cb: Option<GfrPublicKeyFetchCb>,
    user_data: *mut std::ffi::c_void,
) -> Result<DecryptAndVerifyResultInternal, GfrStatus> {
    let mut output_data = Vec::new();

    let is_armor = std::str::from_utf8(encrypted_data)
        .map(|s| s.trim_start().starts_with("-----BEGIN PGP MESSAGE-----"))
        .unwrap_or(false);

    let input_cursor = Cursor::new(encrypted_data);

    let stream_result = decrypt_and_verify_stream_internal(
        channel,
        input_cursor,
        &mut output_data,
        is_armor,
        fetch_seckey_cb,
        fetch_pwd_cb,
        fetch_pubkey_cb,
        user_data,
    )?;

    Ok(DecryptAndVerifyResultInternal {
        data: output_data,
        filename: stream_result.filename,
        recipients: stream_result.recipients,
        is_verified: stream_result.is_verified,
        signatures: stream_result.signatures,
    })
}

#[cfg(test)]
mod rfc9580_envelope_tests {
    //! RFC 9580 envelope-analysis tests for `analyze_encrypted_envelope`, driven
    //! by the committed known-answer corpus (see
    //! scripts/gen_rpgp_test_vectors.sh). These unit-test the packet-level
    //! recipient/SKESK extraction and the legacy-SED rejection (finding H2)
    //! without going through the FFI decrypt entrypoint.
    use super::*;
    use std::io::Cursor;

    use crate::testutil::corpus::{
        ENC_MULTI_RECIPIENT as ENC_MULTI, ENC_SED_TAG9 as ENC_SED, ENC_SYMMETRIC_V1 as ENC_SYM_V1,
        ENC_V1SEIPD_MDC as ENC_V1, ENC_V2SEIPD_OCB as ENC_V2,
    };

    fn parse(data: &[u8]) -> Message<'_> {
        Message::from_reader(Cursor::new(data))
            .expect("parse encrypted message")
            .0
    }

    /// A v1 SEIPD message encrypted to a public key lists public-key
    /// recipients and no SKESK. (sq emits one PKESK per encryption subkey, and
    /// fixture key1 has three, so there is at least one recipient.)
    #[test]
    fn envelope_v1_seipd_lists_pk_recipients() {
        let msg = parse(ENC_V1);
        let (has_pkesk, has_skesk, recipients) = analyze_encrypted_envelope(&msg).unwrap();
        assert!(has_pkesk);
        assert!(!has_skesk);
        assert!(!recipients.is_empty());
        assert!(recipients.iter().all(|r| !r.key_id.is_empty()));
    }

    /// A message encrypted to three certificates lists at least three PKESK
    /// recipients (one per encryption subkey across key1+key2+key3).
    #[test]
    fn envelope_multi_recipient_lists_all() {
        let msg = parse(ENC_MULTI);
        let (has_pkesk, _has_skesk, recipients) = analyze_encrypted_envelope(&msg).unwrap();
        assert!(has_pkesk);
        assert!(recipients.len() >= 3, "got {}", recipients.len());
    }

    /// A password-based message carries an SKESK and no public-key recipients.
    #[test]
    fn envelope_symmetric_reports_skesk() {
        let msg = parse(ENC_SYM_V1);
        let (_has_pkesk, has_skesk, recipients) = analyze_encrypted_envelope(&msg).unwrap();
        assert!(has_skesk);
        assert!(recipients.is_empty());
    }

    /// RFC 9580 §13.7 / finding H2: a legacy Symmetrically Encrypted Data packet
    /// (Tag 9, no integrity protection) must be rejected before any decryption.
    #[test]
    fn envelope_rejects_legacy_sed() {
        let msg = parse(ENC_SED);
        assert!(
            analyze_encrypted_envelope(&msg).is_err(),
            "legacy SED must be refused"
        );
    }

    /// RFC 9580 §5.1.2: a message addressed to a v6 recipient lists that
    /// recipient by fingerprint. `analyze_encrypted_envelope` falls back to
    /// `pkesk.fingerprint()` when `pkesk.id()` errors (v6), so the recipient is
    /// extracted and public-key decryption to a v6 key works.
    #[test]
    fn envelope_v6_recipient_is_extracted() {
        let msg = parse(ENC_V2);
        let (has_pkesk, _has_skesk, recipients) = analyze_encrypted_envelope(&msg).unwrap();
        assert!(has_pkesk);
        assert_eq!(
            recipients.len(),
            1,
            "the v6 PKESK recipient should be listed"
        );
        // The identifier is a full v6 fingerprint (64 hex chars), not a key ID.
        assert_eq!(recipients[0].key_id.len(), 64, "{}", recipients[0].key_id);
    }
}

#[cfg(test)]
mod decrypt_tests {
    //! Decryption across every session-key and payload container RFC 9580
    //! defines.
    //!
    //! The Appendix A vectors are the only coverage the engine has for
    //! Argon2 (§3.7.1.4), AEAD-EAX (§5.13.3) and AEAD-GCM (§5.13.5): it never
    //! *produces* any of the three, so no round-trip test could reach them.
    //! They are normative, tool-independent, and transcribed verbatim.

    use super::*;
    use crate::testutil::{cb, corpus, keys, packets, rfc9580};

    /// Decrypt with a secret key supplied through the fetch callback.
    fn decrypt_with_key(data: &[u8], secret_block: &str, passphrase_cb: Option<GfrPasswordFetchCb>) -> Result<DecryptResultInternal, GfrStatus> {
        cb::set_seckey_answer(secret_block);
        decrypt_internal(0, data, Some(cb::seckey_fetch), passphrase_cb, std::ptr::null_mut())
    }

    /// Decrypt a password-protected message.
    fn decrypt_with_password(
        data: &[u8],
        passphrase_cb: GfrPasswordFetchCb,
    ) -> Result<DecryptResultInternal, GfrStatus> {
        cb::clear_seckey_answer();
        decrypt_internal(
            0,
            data,
            Some(cb::seckey_none),
            Some(passphrase_cb),
            std::ptr::null_mut(),
        )
    }

    /// A passphrase callback bound to a specific literal.
    macro_rules! pw_cb {
        ($name:ident, $bytes:expr) => {
            extern "C" fn $name(
                _channel: i32,
                _state: crate::types::GfrPassphraseState,
                out_pwd: *mut *mut u8,
                out_status: *mut crate::types::GfrPasswordFetchStatus,
                _user_data: *mut c_void,
            ) -> i32 {
                let (ptr, len) = crate::testutil::leak_as_c_buffer($bytes);
                unsafe {
                    *out_pwd = ptr;
                    *out_status = crate::types::GfrPasswordFetchStatus::Provided;
                }
                len as i32
            }
        };
    }

    pw_cb!(pw_password, b"password");
    pw_cb!(pw_corpus, b"123456");

    // -- RFC 9580 Appendix A known-answer vectors ---------------------------

    #[test]
    fn appendix_a8_x25519_aead_ocb_decrypts() {
        // §5.1.6 X25519 PKESK + §5.13.2 v2 SEIPD with OCB (§5.13.4).
        let res = decrypt_with_key(
            rfc9580::A8_X25519_AEAD_OCB.as_bytes(),
            rfc9580::A4_V6_SECRET_UNLOCKED,
            None,
        )
        .expect("A.8 must decrypt");
        assert_eq!(res.data, rfc9580::A_PLAINTEXT_HELLO);
    }

    #[test]
    fn appendix_a8_reports_its_v6_recipient() {
        // §5.1.2: a v6 PKESK names the recipient by fingerprint.
        let recipients = crate::crypto::sniff_recipients(rfc9580::A8_X25519_AEAD_OCB.as_bytes());
        assert_eq!(recipients.len(), 1);
        assert_eq!(recipients[0].key_id.len(), 64);
    }

    #[test]
    fn appendix_a9_aead_eax_decrypts() {
        // §5.13.3 EAX -- the engine cannot produce this mode, so the RFC
        // vector is the only way to prove it can consume it.
        let res = decrypt_with_password(rfc9580::A9_EAX_SKESK.as_bytes(), pw_password)
            .expect("A.9 must decrypt");
        assert_eq!(res.data, rfc9580::A_PLAINTEXT_HELLO);
    }

    #[test]
    fn appendix_a10_aead_ocb_decrypts() {
        // §5.13.4 OCB, the mandatory-to-implement AEAD mode (§9.6).
        let res = decrypt_with_password(rfc9580::A10_OCB_SKESK.as_bytes(), pw_password)
            .expect("A.10 must decrypt");
        assert_eq!(res.data, rfc9580::A_PLAINTEXT_HELLO);
    }

    #[test]
    fn appendix_a11_aead_gcm_decrypts() {
        // §5.13.5 GCM.
        let res = decrypt_with_password(rfc9580::A11_GCM_SKESK.as_bytes(), pw_password)
            .expect("A.11 must decrypt");
        assert_eq!(res.data, rfc9580::A_PLAINTEXT_HELLO);
    }

    #[test]
    fn appendix_a12_argon2_with_aes128_decrypts() {
        // §3.7.1.4 Argon2 (t=1, p=4, m=2^21), the S2K the RFC recommends.
        let res = decrypt_with_password(rfc9580::A12_ARGON2_AES128.as_bytes(), pw_password)
            .expect("A.12.1 must decrypt");
        assert_eq!(res.data, rfc9580::A_PLAINTEXT_HELLO);
    }

    #[test]
    fn appendix_a12_argon2_with_aes192_decrypts() {
        let res = decrypt_with_password(rfc9580::A12_ARGON2_AES192.as_bytes(), pw_password)
            .expect("A.12.2 must decrypt");
        assert_eq!(res.data, rfc9580::A_PLAINTEXT_HELLO);
    }

    #[test]
    fn appendix_a12_argon2_with_aes256_decrypts() {
        let res = decrypt_with_password(rfc9580::A12_ARGON2_AES256.as_bytes(), pw_password)
            .expect("A.12.3 must decrypt");
        assert_eq!(res.data, rfc9580::A_PLAINTEXT_HELLO);
    }

    #[test]
    fn an_argon2_message_refuses_the_wrong_passphrase() {
        // Memory-hard or not, a wrong passphrase must still fail cleanly.
        assert!(decrypt_with_password(rfc9580::A12_ARGON2_AES128.as_bytes(), pw_corpus).is_err());
    }

    #[test]
    fn appendix_a5_locked_v6_secret_key_unlocks_with_its_passphrase() {
        // §3.7.2.1 S2K usage octet 253 (AEAD) with an Argon2 specifier -- the
        // combination the RFC recommends for v6 secret keys.
        use pgp::composed::Deserializable;
        use pgp::types::Password;
        let (key, _) = pgp::composed::SignedSecretKey::from_string(rfc9580::A5_V6_SECRET_LOCKED)
            .expect("A.5 parses");
        let pw = String::from_utf8_lossy(rfc9580::A5_PASSPHRASE).into_owned();
        assert!(key.unlock(&Password::from(pw), |_, _| Ok(())).is_ok());
    }

    #[test]
    fn appendix_a5_locked_key_refuses_a_wrong_passphrase() {
        use pgp::composed::Deserializable;
        use pgp::types::Password;
        let (key, _) = pgp::composed::SignedSecretKey::from_string(rfc9580::A5_V6_SECRET_LOCKED)
            .expect("A.5 parses");
        assert!(key.unlock(&Password::from("wrong"), |_, _| Ok(())).is_err());
    }

    #[test]
    fn appendix_a4_and_a5_are_the_same_key() {
        // The locked and unlocked samples differ only in secret-key
        // protection, so the fingerprints must match.
        use pgp::composed::Deserializable;
        use pgp::types::KeyDetails;
        let (unlocked, _) =
            pgp::composed::SignedSecretKey::from_string(rfc9580::A4_V6_SECRET_UNLOCKED)
                .expect("A.4 parses");
        let (locked, _) =
            pgp::composed::SignedSecretKey::from_string(rfc9580::A5_V6_SECRET_LOCKED)
                .expect("A.5 parses");
        assert_eq!(
            unlocked.primary_key.fingerprint().to_string(),
            locked.primary_key.fingerprint().to_string()
        );
    }

    #[test]
    fn appendix_a3_and_a4_agree_on_the_fingerprint() {
        use pgp::composed::Deserializable;
        use pgp::types::KeyDetails;
        let (cert, _) =
            pgp::composed::SignedPublicKey::from_string(rfc9580::A3_V6_CERT).expect("A.3");
        let (secret, _) =
            pgp::composed::SignedSecretKey::from_string(rfc9580::A4_V6_SECRET_UNLOCKED)
                .expect("A.4");
        assert_eq!(
            cert.primary_key.fingerprint().to_string(),
            secret.primary_key.fingerprint().to_string()
        );
        assert_eq!(
            cert.primary_key.fingerprint().to_string().to_uppercase(),
            rfc9580::A3_PRIMARY_FINGERPRINT
        );
    }

    // -- the committed corpus ------------------------------------------------

    #[test]
    fn a_v1_seipd_mdc_message_decrypts() {
        // §5.13.1: CFB plus a trailing SHA-1 modification detection code.
        let res = decrypt_with_key(corpus::ENC_V1SEIPD_MDC, corpus::KEY1_SECRET, Some(pw_corpus))
            .expect("decrypt");
        assert_eq!(res.data, corpus::PAYLOAD);
    }

    #[test]
    fn a_symmetric_v1_message_decrypts_with_its_passphrase() {
        let res = decrypt_with_password(corpus::ENC_SYMMETRIC_V1, pw_corpus).expect("decrypt");
        assert_eq!(res.data, corpus::PAYLOAD);
    }

    #[test]
    fn a_symmetric_v2_message_decrypts_with_its_passphrase() {
        // §5.3.2 v6 SKESK + §5.13.2 v2 SEIPD.
        let res = decrypt_with_password(corpus::ENC_SYMMETRIC_V2, pw_corpus).expect("decrypt");
        assert_eq!(res.data, corpus::PAYLOAD);
    }

    #[test]
    fn a_multi_recipient_message_decrypts_with_one_of_the_keys() {
        let res = decrypt_with_key(
            corpus::ENC_MULTI_RECIPIENT,
            corpus::KEY1_SECRET,
            Some(pw_corpus),
        )
        .expect("decrypt");
        assert_eq!(res.data, corpus::PAYLOAD);
    }

    #[test]
    fn a_decrypted_message_reports_its_recipients() {
        let res = decrypt_with_key(corpus::ENC_V1SEIPD_MDC, corpus::KEY1_SECRET, Some(pw_corpus))
            .expect("decrypt");
        assert!(!res.recipients.is_empty());
    }

    // -- §13.7 malleable ciphertext ------------------------------------------

    #[test]
    fn a_legacy_sed_packet_is_refused() {
        // §13.7 / §5.7: "This packet is obsolete. An implementation MUST NOT
        // create this packet. An implementation SHOULD reject such a packet
        // and stop processing the message."
        assert!(
            decrypt_with_key(corpus::ENC_SED_TAG9, corpus::KEY1_SECRET, Some(pw_corpus)).is_err(),
            "an unauthenticated SED payload must never be decrypted"
        );
    }

    #[test]
    fn a_pkesk_without_a_payload_fails() {
        assert!(
            decrypt_with_key(corpus::PKESK_NO_SEIPD, corpus::KEY1_SECRET, Some(pw_corpus))
                .is_err()
        );
    }

    #[test]
    fn a_tampered_mdc_is_detected() {
        // §5.13.1: "A mismatch of the hash indicates that the message has been
        // modified and MUST be treated as a security problem."
        let mut tampered = corpus::ENC_V1SEIPD_MDC.to_vec();
        let last = tampered.len() - 8;
        tampered[last] ^= 0xFF;
        assert!(
            decrypt_with_key(&tampered, corpus::KEY1_SECRET, Some(pw_corpus)).is_err(),
            "a corrupted MDC must not release plaintext"
        );
    }

    #[test]
    fn a_tampered_aead_tag_is_detected() {
        // §13.7: "if the authentication tag fails to verify, the
        // implementation MUST NOT attempt to parse nor release decrypted data".
        let mut tampered = rfc9580::A8_X25519_AEAD_OCB.as_bytes().to_vec();
        // Flip a byte inside the base64 body rather than the armor header.
        let pos = tampered.len() / 2;
        tampered[pos] = if tampered[pos] == b'A' { b'B' } else { b'A' };
        assert!(
            decrypt_with_key(&tampered, rfc9580::A4_V6_SECRET_UNLOCKED, None).is_err()
        );
    }

    #[test]
    fn a_truncated_ciphertext_fails_cleanly() {
        for n in [16usize, 64, 128] {
            let prefix = packets::truncate_at(corpus::ENC_V1SEIPD_MDC, n);
            let outcome = std::panic::catch_unwind(|| {
                decrypt_with_key(&prefix, corpus::KEY1_SECRET, Some(pw_corpus)).is_ok()
            });
            assert_eq!(outcome.ok(), Some(false), "a {n}-byte prefix must not decrypt");
        }
    }

    #[test]
    fn a_wrong_passphrase_on_a_symmetric_message_fails() {
        assert!(decrypt_with_password(corpus::ENC_SYMMETRIC_V1, pw_password).is_err());
    }

    #[test]
    fn a_missing_secret_key_fails() {
        cb::clear_seckey_answer();
        assert!(
            decrypt_internal(
                0,
                corpus::ENC_V1SEIPD_MDC,
                Some(cb::seckey_none),
                Some(pw_corpus),
                std::ptr::null_mut()
            )
            .is_err()
        );
    }

    #[test]
    fn a_cancelled_passphrase_prompt_surfaces_as_canceled() {
        // A deliberate user cancellation must not be reported as a generic
        // failure.
        match decrypt_with_password(corpus::ENC_SYMMETRIC_V1, cb::pwd_cancelled) {
            Err(status) => assert_eq!(status, GfrStatus::ErrorCanceled),
            Ok(_) => panic!("a cancelled prompt must not yield plaintext"),
        }
    }

    #[test]
    fn decrypting_garbage_fails_cleanly() {
        assert!(decrypt_with_key(corpus::GARBAGE, corpus::KEY1_SECRET, Some(pw_corpus)).is_err());
    }

    #[test]
    fn decrypting_an_empty_buffer_fails_cleanly() {
        assert!(decrypt_with_key(corpus::EMPTY, corpus::KEY1_SECRET, Some(pw_corpus)).is_err());
    }

    #[test]
    fn decrypting_never_panics_on_adversarial_input() {
        for vector in [
            corpus::GARBAGE,
            corpus::EMPTY,
            corpus::ENC_SED_TAG9,
            corpus::PKESK_NO_SEIPD,
            corpus::TRUNCATED_ARMOR.as_bytes(),
            corpus::CORRUPT_CRC.as_bytes(),
            corpus::SIG_GOOD_DETACHED,
        ] {
            let outcome = std::panic::catch_unwind(|| {
                decrypt_with_key(vector, corpus::KEY1_SECRET, Some(pw_corpus)).is_ok()
            });
            assert!(outcome.is_ok(), "panicked on an adversarial vector");
        }
    }

    // -- §13.14 compression limits --------------------------------------------

    #[test]
    fn the_compression_nesting_cap_is_sixteen_layers() {
        assert_eq!(MAX_COMPRESSION_LAYERS, 16);
    }

    #[test]
    fn a_deeply_nested_compressed_payload_is_refused() {
        // §13.14: "An OpenPGP implementation SHOULD limit the number of layers
        // of compression it is willing to decompress in a single message."
        // 32 layers is twice the cap.
        let key = &keys::V4_SIGN;
        let nested = packets::nested_compressed(32, &packets::literal(b'b', b"", 0, b"deep"));
        let encrypted = crate::crypto::encrypt_internal(
            0,
            "",
            &nested,
            &[&key.public_armored],
            false,
        )
        .expect("encrypt");

        let outcome = std::panic::catch_unwind(|| {
            decrypt_with_key(&encrypted.data, &key.secret_armored, None).is_ok()
        });
        assert!(outcome.is_ok(), "the nesting cap must error, never panic");
    }

    // -- round-trips against our own encryptor ---------------------------------

    #[test]
    fn a_message_we_encrypt_decrypts_back() {
        let key = &keys::V4_SIGN;
        let encrypted = crate::crypto::encrypt_internal(
            0,
            "",
            b"round trip payload",
            &[&key.public_armored],
            true,
        )
        .expect("encrypt");
        let res = decrypt_with_key(&encrypted.data, &key.secret_armored, None).expect("decrypt");
        assert_eq!(res.data, b"round trip payload");
    }

    #[test]
    fn a_v6_message_we_encrypt_decrypts_back() {
        let key = &keys::V6_SIGN;
        let encrypted =
            crate::crypto::encrypt_internal(0, "", b"v6 payload", &[&key.public_armored], true)
                .expect("encrypt");
        let res = decrypt_with_key(&encrypted.data, &key.secret_armored, None).expect("decrypt");
        assert_eq!(res.data, b"v6 payload");
    }

    #[test]
    fn a_binary_payload_survives_a_round_trip() {
        let key = &keys::V4_SIGN;
        let payload: Vec<u8> = (0..=255u8).cycle().take(8192).collect();
        let encrypted =
            crate::crypto::encrypt_internal(0, "", &payload, &[&key.public_armored], false)
                .expect("encrypt");
        let res = decrypt_with_key(&encrypted.data, &key.secret_armored, None).expect("decrypt");
        assert_eq!(res.data, payload);
    }

    #[test]
    fn the_filename_hint_is_not_written_into_the_literal_packet() {
        // KNOWN GAP (upstream): `MessageBuilder::from_reader(file_name, ..)` in
        // pgp 0.20 accepts a filename and then emits an empty one, so the hint
        // the caller passes to `encrypt_internal` never reaches the wire and
        // `DecryptResultInternal::filename` comes back empty.
        //
        // This is *not* an RFC 9580 conformance problem -- §5.9 says an
        // implementation "SHOULD set the filename to the empty string", and
        // warns that the field is unauthenticated and must not be trusted. The
        // gap is only that the engine's API advertises a hint it cannot honour.
        // Pinned here so the day rPGP starts writing it, this test fails and
        // the behaviour change is noticed rather than assumed.
        let key = &keys::V4_SIGN;
        let encrypted = crate::crypto::encrypt_internal(
            0,
            "report.pdf",
            b"contents",
            &[&key.public_armored],
            false,
        )
        .expect("encrypt");
        let res = decrypt_with_key(&encrypted.data, &key.secret_armored, None).expect("decrypt");
        assert_eq!(res.data, b"contents", "the payload itself round-trips");
        assert_eq!(res.filename, "", "the hint is dropped upstream");
    }

    #[test]
    fn rpgp_itself_drops_the_filename_on_a_plain_literal_build() {
        // The isolation that identifies the previous test's cause as upstream
        // rather than a mistake at our call site.
        use pgp::composed::MessageBuilder;
        let mut out = Vec::new();
        let builder = MessageBuilder::from_reader(b"direct.txt".to_vec(), &b"x"[..]);
        builder
            .to_writer(&mut rand::thread_rng(), &mut out)
            .expect("build");
        let msg = Message::from_bytes(std::io::Cursor::new(out)).expect("parse");
        let name = msg
            .literal_data_header()
            .map(|h| String::from_utf8_lossy(h.file_name()).into_owned());
        assert_eq!(name.as_deref(), Some(""));
    }

    #[test]
    fn a_hand_built_literal_packet_does_carry_its_filename() {
        // And the parser reads it correctly when a producer actually writes
        // one -- so consuming a filename from another implementation works.
        let data = packets::literal(b'b', b"from-elsewhere.txt", 0, b"x");
        let msg = Message::from_bytes(std::io::Cursor::new(data)).expect("parse");
        assert_eq!(
            msg.literal_data_header()
                .map(|h| h.file_name().to_vec()),
            Some(b"from-elsewhere.txt".to_vec())
        );
    }

    #[test]
    fn decrypting_with_the_wrong_key_fails() {
        let encrypted = crate::crypto::encrypt_internal(
            0,
            "",
            b"secret",
            &[&keys::V4_SIGN.public_armored],
            false,
        )
        .expect("encrypt");
        assert!(decrypt_with_key(&encrypted.data, &keys::V6_SIGN.secret_armored, None).is_err());
    }

    #[test]
    fn decrypt_and_verify_reports_both_plaintext_and_signatures() {
        let key = &keys::V4_SIGN;
        let sealed = crate::crypto::encrypt_and_sign_internal(
            0,
            "",
            b"signed and sealed",
            &[&key.public_armored],
            &[&key.secret_armored],
            None,
            true,
        )
        .expect("encrypt+sign");

        cb::set_seckey_answer(&key.secret_armored);
        cb::set_pubkey_answer(&key.public_armored);
        let res = decrypt_and_verify_internal(
            0,
            &sealed.data,
            Some(cb::seckey_fetch),
            None,
            Some(cb::pubkey_fetch),
            std::ptr::null_mut(),
        )
        .expect("decrypt+verify");

        assert_eq!(res.data, b"signed and sealed");
        assert!(res.is_verified, "the embedded signature must verify");
        assert!(!res.signatures.is_empty());
    }

    #[test]
    fn decrypt_and_verify_without_the_signer_key_still_decrypts() {
        // Not having the signer's certificate must not block decryption; it
        // only leaves the signature unattributed.
        let key = &keys::V4_SIGN;
        let sealed = crate::crypto::encrypt_and_sign_internal(
            0,
            "",
            b"payload",
            &[&key.public_armored],
            &[&key.secret_armored],
            None,
            true,
        )
        .expect("encrypt+sign");

        cb::set_seckey_answer(&key.secret_armored);
        cb::clear_pubkey_answer();
        let res = decrypt_and_verify_internal(
            0,
            &sealed.data,
            Some(cb::seckey_fetch),
            None,
            Some(cb::pubkey_fetch),
            std::ptr::null_mut(),
        )
        .expect("decrypt+verify");

        assert_eq!(res.data, b"payload");
        assert!(!res.is_verified);
    }
}
