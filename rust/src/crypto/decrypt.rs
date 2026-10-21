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

use pgp::{
    composed::{DecryptionOptions, PlainSessionKey, TheRing, decrypt_session_key_with_password},
    crypto::sym::SymmetricKeyAlgorithm,
    types::{DecryptionKey, EskType, PkeskVersion, Seipdv1ReadMode, SkeskVersion},
};
use zeroize::Zeroizing;

use crate::{host::gfc_secure_free_cstr, utils::password_from_zeroizing_bytes};

use super::*;

/// The largest SEIPD v1 payload an in-memory decryption accepts: rPGP's own
/// `CheckFirst` default, which buffers the whole payload to authenticate it
/// before releasing a byte. File and archive decryption never use this limit.
pub(crate) const IN_MEMORY_SEIPDV1_LIMIT: usize = 1024 * 1024 * 1024;

/// For staged (file/archive) output, how many bytes of decompressed plaintext
/// one byte of ciphertext may expand to before the stream is treated as a
/// compression bomb. Real data, even all-zero disk images under ZLIB (~1032:1),
/// stays far below it; a bomb is stopped long before it fills a disk.
const STAGED_DECOMPRESSION_RATIO: u64 = 4096;

/// Where decrypted plaintext goes. Callers choose this, never a raw rPGP read
/// mode, so SEIPD v1 `Streaming` (plaintext released before the MDC check) can
/// only be selected together with a stage that is discarded on error.
enum Sink {
    /// Collected in memory and returned only on success. SEIPD v1 is read in
    /// `CheckFirst` mode, authenticated before any plaintext is released, up
    /// to `seipdv1_limit` bytes.
    Memory { seipdv1_limit: usize },
    /// Written to a stage ([`StagedFile`] or an anonymous temp file) that the
    /// caller commits only on `Ok`. SEIPD v1 is streamed with no size limit;
    /// decompressed output is bounded by [`STAGED_DECOMPRESSION_RATIO`] times
    /// the ciphertext length, but never below [`MAX_DECOMPRESSED_OUTPUT_BYTES`].
    Staged { ciphertext_len: u64 },
}

impl Sink {
    fn seipdv1_mode(&self) -> Seipdv1ReadMode {
        match self {
            Sink::Memory { seipdv1_limit } => Seipdv1ReadMode::CheckFirst {
                max_message_size: *seipdv1_limit,
            },
            Sink::Staged { .. } => Seipdv1ReadMode::Streaming,
        }
    }

    fn decompression_cap(&self) -> u64 {
        match self {
            Sink::Memory { .. } => MAX_DECOMPRESSED_OUTPUT_BYTES,
            Sink::Staged { ciphertext_len } => MAX_DECOMPRESSED_OUTPUT_BYTES
                .max(ciphertext_len.saturating_mul(STAGED_DECOMPRESSION_RATIO)),
        }
    }
}

/// The in-memory sink every buffer API uses.
const MEMORY_SINK: Sink = Sink::Memory {
    seipdv1_limit: IN_MEMORY_SEIPDV1_LIMIT,
};

/// Records whether a write failed, so a full disk is reported as an output
/// error rather than as a corrupt message.
struct TrackedWriter<W> {
    inner: W,
    failed: bool,
}

impl<W: Write> Write for TrackedWriter<W> {
    fn write(&mut self, buf: &[u8]) -> std::io::Result<usize> {
        self.inner.write(buf).inspect_err(|_| self.failed = true)
    }

    fn flush(&mut self) -> std::io::Result<()> {
        self.inner.flush().inspect_err(|_| self.failed = true)
    }
}

use crate::err::{IntegrityFailure, io_payload_failure, payload_failure};

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

/// Recover the session key from the password-protected session-key packets
/// (SKESK) with a passphrase from the user.
///
/// Always bypasses the password cache and always asks for a new password
/// (`ask_for_new: true`) because symmetric encryption has no fingerprint to
/// key the cache on, so a stale cache entry would silently fail.
fn skesk_session_key(
    channel: i32,
    esks: &[Esk],
    fetch_pwd_cb: Option<GfrPasswordFetchCb>,
) -> Result<PlainSessionKey, GfrStatus> {
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

    let mut last_err = None;
    for esk in esks {
        let Esk::SymKeyEncryptedSessionKey(skesk) = esk else {
            continue;
        };
        // v5 is GnuPG's pre-standard AEAD draft, which rPGP only reads on
        // request; a plaintext "session key" is forbidden outright.
        if skesk.version() == SkeskVersion::V5
            || !matches!(skesk.sym_algorithm(), Some(alg) if alg != SymmetricKeyAlgorithm::Plaintext)
        {
            continue;
        }
        match decrypt_session_key_with_password(skesk, &msg_pw) {
            Ok(key) => return Ok(key),
            Err(e) => last_err = Some(e),
        }
    }
    match last_err {
        Some(e) => Err(e).record_err(GfrStatus::ErrorDecryptionFailed),
        None => {
            set_last_error("the message has no usable password-protected session-key packet");
            Err(GfrStatus::ErrorDecryptionFailed)
        }
    }
}

/// Recover the session key from the public-key session-key packets (PKESK)
/// with the secret key the host holds for one of `recipients`, prompting for
/// that key's passphrase when it is locked. Returns the session key and the
/// recipient identifier it matched.
fn pkesk_session_key(
    channel: i32,
    esks: &[Esk],
    recipients: &[RecipientResultInternal],
    fetch_seckey_cb: Option<GfrSecretKeyFetchCb>,
    fetch_pwd_cb: Option<GfrPasswordFetchCb>,
    user_data: *mut c_void,
) -> Result<(PlainSessionKey, String), GfrStatus> {
    // Request the secret key from C++.
    let mut target_skey: Option<SignedSecretKey> = None;
    let mut matched_recipient_id = String::new();

    if let Some(cb) = fetch_seckey_cb {
        for rec in recipients {
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

    // Check if unlocking is needed.
    let mut needs_password = false;
    let primary_id = skey.primary_key.legacy_key_id().to_string();
    let primary_fpr = skey.primary_key.fingerprint().to_string();
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
    let pwd = password_from_zeroizing_bytes(password);

    // Try every PKESK addressed to a component of the key, as rPGP's own ring
    // would. The outer `Result` of `DecryptionKey::decrypt` is the unlock, the
    // inner one the session-key decryption.
    let mut last_err = None;
    for esk in esks {
        let Esk::PublicKeyEncryptedSessionKey(pkesk) = esk else {
            continue;
        };
        let typ = match pkesk.version() {
            PkeskVersion::V3 => EskType::V3_4,
            PkeskVersion::V6 => EskType::V6,
            _ => continue,
        };
        let Ok(values) = pkesk.values() else {
            continue;
        };
        let mut candidates: Vec<&dyn DecryptionKey> = Vec::new();
        if pkesk.match_identity(skey.primary_key.public_key()) {
            candidates.push(&skey.primary_key);
        }
        for subkey in &skey.secret_subkeys {
            if pkesk.match_identity(subkey.public_key()) {
                candidates.push(&subkey.key);
            }
        }
        for key in candidates {
            match key.decrypt(&pwd, values, typ) {
                Ok(Ok(session_key)) => return Ok((session_key, matched_recipient_id)),
                Ok(Err(e)) => last_err = Some(e),
                Err(e) => {
                    if needs_password {
                        log::warn!(
                            "Unlocking failed. Evicting bad password for FPR: {}",
                            target_fpr_for_pwd
                        );
                        PASSWORD_CACHE.remove_by_fpr(&target_fpr_for_pwd);
                    }
                    set_last_error(&format!(
                        "the passphrase does not unlock the secret key: {e}"
                    ));
                    return Err(GfrStatus::ErrorBadPassphrase);
                }
            }
        }
    }

    match last_err {
        Some(e) => Err(e).record_err(GfrStatus::ErrorDecryptionFailed),
        None => {
            set_last_error("the secret key does not match any session-key packet of this message");
            Err(GfrStatus::ErrorNoKey)
        }
    }
}

/// The cipher and mode protecting `parsed`'s payload, in gpgme's
/// `symkey_algo` form: "AES256.OCB" for a SEIPD v2, "AES256.CFB" for a
/// SEIPD v1 (whose cipher only the session key names). Empty if unknown.
fn container_description(parsed: &Message, session_key: &PlainSessionKey) -> String {
    use pgp::composed::Edata;
    use pgp::packet::{ProtectedDataConfig, SymEncryptedProtectedDataConfig};
    let Message::Encrypted {
        edata: Edata::SymEncryptedProtectedData { reader },
        ..
    } = parsed
    else {
        return String::new();
    };
    let name = |alg: &SymmetricKeyAlgorithm| format!("{alg:?}").to_uppercase();
    match (reader.config(), session_key) {
        (
            ProtectedDataConfig::Seipd(SymEncryptedProtectedDataConfig::V2 {
                sym_alg, aead, ..
            }),
            _,
        ) => format!("{}.{}", name(&sym_alg), format!("{aead:?}").to_uppercase()),
        (
            ProtectedDataConfig::Seipd(SymEncryptedProtectedDataConfig::V1),
            PlainSessionKey::V3_4 { sym_alg, .. },
        ) => format!("{}.CFB", name(&sym_alg)),
        _ => String::new(),
    }
}

fn is_anonymous(id: &str) -> bool {
    id == "0000000000000000"
}

/// Decrypt and optionally verify a stream in a single pipeline.
///
/// Handles both symmetric (passphrase) and asymmetric (public-key) ciphertext
/// by inspecting the ESK packets first. Verification is performed only when
/// `fetch_pubkey_cb` is `Some` and the decrypted payload contains signatures;
/// passing `None` skips verification entirely without error.
///
/// Anonymous recipients are not supported (RFC 9580 §5.1.8 leaves trying
/// every private key a MAY): a v3 wildcard key ID ("0000000000000000") is
/// handed to the host's key lookup, which matches no key, and a v6 wildcard
/// PKESK names no recipient at all, so either ends in `ErrorNoKey` unless the
/// message also has a password packet.
///
/// `sink` decides how a SEIPD v1 payload is read and how far decompressed
/// output may grow (see [`Sink`]). Private on purpose: the public entry points
/// fix the sink, so streaming can only be paired with a stage.
#[allow(clippy::too_many_arguments)]
fn decrypt_message_stream<R, W>(
    channel: i32,
    input_stream: R,
    output_stream: W,
    ascii_armor: bool,
    fetch_seckey_cb: Option<GfrSecretKeyFetchCb>,
    fetch_pwd_cb: Option<GfrPasswordFetchCb>,
    fetch_pubkey_cb: Option<GfrPublicKeyFetchCb>,
    user_data: *mut c_void,
    sink: Sink,
) -> Result<DecryptAndVerifyStreamResultInternal, GfrStatus>
where
    R: Read + Send + Sync + fmt::Debug,
    W: Write + Send + Sync,
{
    let seipdv1_mode = sink.seipdv1_mode();

    // Wrap the source so a user cancel request aborts the streaming read.
    let buf_input = BufReader::new(crate::cancel::CancellableReader::new(channel, input_stream));

    // 1. Parse PGP outer envelope (consumes headers only). Input that ends
    //    inside a packet is a truncated message, whose integrity is suspect
    //    (RFC 9580 §13.7), not merely invalid input.
    let parsed = if ascii_armor {
        Message::from_armor(buf_input).map(|(msg, _)| msg)
    } else {
        Message::from_reader(buf_input).map(|(msg, _)| msg)
    };
    let parsed_message = match parsed {
        Ok(msg) => msg,
        Err(e) => {
            if let Some(kind) =
                payload_failure(&e).filter(|_| !crate::cancel::is_cancelled(channel))
            {
                return Err(kind.report());
            }
            return Err(e).record_err_with(|| {
                crate::cancel::status_or_canceled(channel, GfrStatus::ErrorInvalidInput)
            });
        }
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

    // 2. Recover the session key from the ESK packets before touching the
    //    payload. A mixed message "can be decrypted with either a passphrase
    //    or a public key pair" (RFC 9580 §3.7.2.2): the private key is tried
    //    first and the message password only if it does not yield the
    //    session key. A cancelled prompt aborts; when both fail, one uniform
    //    error is reported so the failure does not reveal which mechanism
    //    came closer.
    let esks: &[Esk] = match &parsed_message {
        Message::Encrypted { esk, .. } => esk,
        _ => &[],
    };

    let mut session_key = None;
    let mut key_failure = None;
    if has_pkesk {
        match pkesk_session_key(
            channel,
            esks,
            &recipients,
            fetch_seckey_cb,
            fetch_pwd_cb,
            user_data,
        ) {
            Ok((key, matched_recipient_id)) => {
                session_key = Some(key);
                for rec in &mut recipients {
                    if rec.key_id == matched_recipient_id || is_anonymous(&rec.key_id) {
                        rec.status = GfrRecipientStatus::Success;
                    }
                }
            }
            Err(GfrStatus::ErrorCanceled) => return Err(GfrStatus::ErrorCanceled),
            Err(status) => key_failure = Some(status),
        }
    }
    if session_key.is_none() && has_skesk {
        debug!("Attempting password-based decryption.");
        match skesk_session_key(channel, esks, fetch_pwd_cb) {
            Ok(key) => session_key = Some(key),
            Err(GfrStatus::ErrorCanceled) => return Err(GfrStatus::ErrorCanceled),
            Err(_) if has_pkesk => {
                set_last_error("no usable private key or password opens this message");
                return Err(GfrStatus::ErrorDecryptionFailed);
            }
            Err(status) => return Err(status),
        }
    }
    let Some(session_key) = session_key else {
        return Err(key_failure.unwrap_or(GfrStatus::ErrorNoKey));
    };

    let sym_algo = container_description(&parsed_message, &session_key);

    // 5. Initialize streaming decryption with the recovered session key.
    let ring = TheRing {
        session_keys: vec![session_key],
        decrypt_options: DecryptionOptions::new().set_seipdv1_read_mode(seipdv1_mode),
        ..Default::default()
    };
    let decrypted = parsed_message
        .decrypt_the_ring(ring, true)
        .map(|(msg, _)| msg)
        .or_else(|e| {
            if crate::cancel::is_cancelled(channel) {
                return Err(GfrStatus::ErrorCanceled);
            }
            match payload_failure(&e) {
                Some(kind) => Err(kind.report()),
                // The session key is known, so anything else rPGP refuses
                // here is the decrypted content itself, e.g. an unknown
                // critical packet (RFC 9580 §4.3): an invalid message, not an
                // internal error.
                None => match Err::<(), _>(e).into_gfr().unwrap_err() {
                    GfrStatus::ErrorInternal => Err(GfrStatus::ErrorInvalidData),
                    status => Err(status),
                },
            }
        })?;

    // 6. Mount decompression pipeline
    //
    // Peel *every* compression layer, not just one: a doubly-compressed message
    // is legitimate and would otherwise leave an inner compressed blob in the
    // output. The layer count is capped so an unbounded chain cannot drive a
    // denial-of-service, and the decompressed output is bounded by the size
    // limiter in step 8 so a single high-ratio "compression bomb" cannot exhaust
    // memory/disk (RFC 9580 §13.14).
    //
    // §13.7 lists a decompression failure inside an encrypted container among
    // the malleability indicators: the integrity is suspect.
    let (mut decrypted, was_compressed) = peel_compression(decrypted, |e| {
        if crate::cancel::is_cancelled(channel) {
            return GfrStatus::ErrorCanceled;
        }
        payload_failure(&e)
            .unwrap_or_else(|| IntegrityFailure::Malformed(format!("decompression failed: {e}")))
            .report()
    })?;

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
    // NOTE: with a `Sink::Memory` a v1/MDC payload is authenticated before any
    // plaintext is released. With a `Sink::Staged` (v1 `Streaming`), and with
    // SEIPD v2 (per-chunk AEAD) in either sink, plaintext reaches
    // `output_stream` before the final check, so the caller discards it unless
    // this function returns `Ok`.
    //
    // A failure here is classified by where it came from: a cancel request, the
    // output (a full disk), a failed MDC/AEAD check, or anything else.
    let mut output = TrackedWriter {
        inner: output_stream,
        failed: false,
    };
    let copy_result = if was_compressed {
        let mut limited = LimitedReader::new(&mut decrypted, sink.decompression_cap());
        std::io::copy(&mut limited, &mut output)
    } else {
        std::io::copy(&mut decrypted, &mut output)
    }
    .and_then(|_| output.flush());
    if let Err(err) = copy_result {
        if crate::cancel::is_cancelled(channel) {
            return Err(GfrStatus::ErrorCanceled);
        }
        if output.failed {
            set_last_error(&format!("cannot write the decrypted output: {err}"));
            return Err(GfrStatus::ErrorIo);
        }
        if let Some(kind) = io_payload_failure(&err) {
            return Err(kind.report());
        }
        return Err(err).into_gfr();
    }

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
        attribute_entries(&mut signatures, &certs, &mut is_verified, |i, cert, t| {
            message_index_verdict(&decrypted, i, cert, t)
        });
    }

    Ok(DecryptAndVerifyStreamResultInternal {
        filename,
        sym_algo,
        recipients,
        is_verified,
        signatures,
    })
}

/// Wraps the stage a file or archive decryption writes into. Production passes
/// [`plain_stage_writer`]; tests inject faults (a full disk, a cancel request)
/// here to exercise the cleanup paths.
type StageWriter = for<'a> fn(&'a mut File) -> Box<dyn Write + Send + Sync + 'a>;

fn plain_stage_writer(file: &mut File) -> Box<dyn Write + Send + Sync + '_> {
    Box::new(file)
}

/// Open the ciphertext and report its length, for [`Sink::Staged`].
fn open_ciphertext(in_file_path: &str) -> Result<(File, u64), GfrStatus> {
    let in_file = File::open(in_file_path).map_err(|e| {
        log::error!("Failed to open encrypted input file: {}", e);
        set_last_error(&format!("cannot open encrypted input file: {}", e));
        GfrStatus::ErrorInvalidInput
    })?;
    let len = in_file.metadata().map(|m| m.len()).unwrap_or(0);
    Ok((in_file, len))
}

fn stage_error(what: &str, e: std::io::Error) -> GfrStatus {
    log::error!("{}: {}", what, e);
    set_last_error(&format!("{what}: {e}"));
    GfrStatus::ErrorIo
}

/// Decrypt an encrypted archive and unpack its tar contents to a directory.
///
/// Transactional: the payload is decrypted (SEIPD v1 streamed, any size) into
/// an anonymous temp file in `out_dir_path`'s parent directory, so a multi-GB
/// archive never lands in a RAM-backed `/tmp`. Only after decryption and
/// integrity checking succeed is the tar extracted, into a [`StagedDir`] that
/// replaces or merges into `out_dir_path` once extraction has completed. Any
/// failure leaves `out_dir_path` exactly as it was (it is not even created).
#[allow(clippy::too_many_arguments)]
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
    decrypt_archive_staged(
        channel,
        in_file_path,
        out_dir_path,
        ascii_armor,
        fetch_seckey_cb,
        fetch_pwd_cb,
        fetch_pubkey_cb,
        user_data,
        plain_stage_writer,
    )
}

#[allow(clippy::too_many_arguments)]
fn decrypt_archive_staged(
    channel: i32,
    in_file_path: &str,
    out_dir_path: &str,
    ascii_armor: bool,
    fetch_seckey_cb: Option<crate::types::GfrSecretKeyFetchCb>,
    fetch_pwd_cb: Option<crate::types::GfrPasswordFetchCb>,
    fetch_pubkey_cb: Option<crate::types::GfrPublicKeyFetchCb>,
    user_data: *mut std::ffi::c_void,
    stage_writer: StageWriter,
) -> Result<DecryptAndVerifyResultInternal, crate::types::GfrStatus> {
    let out_dir = Path::new(out_dir_path);
    let (in_file, ciphertext_len) = open_ciphertext(in_file_path)?;

    // 1. Stage the extraction next to the destination (same filesystem).
    let stage = super::staging::StagedDir::new(out_dir)
        .map_err(|e| stage_error("cannot create the extraction staging directory", e))?;

    // 2. Decrypt into an anonymous temp file beside the stage. It has no name,
    // so it disappears on every exit path, including a crash.
    let mut temp_archive = tempfile::tempfile_in(stage.work())
        .map_err(|e| stage_error("cannot create a temporary file for decryption", e))?;

    log::info!("Decrypting file into temporary archive...");
    let stream_result = decrypt_message_stream(
        channel,
        in_file,
        stage_writer(&mut temp_archive),
        ascii_armor,
        fetch_seckey_cb,
        fetch_pwd_cb,
        fetch_pubkey_cb,
        user_data,
        // Safe: nothing is extracted unless this returns `Ok`.
        Sink::Staged { ciphertext_len },
    )?;

    // 3. Extract the authenticated tar into the stage.
    temp_archive
        .seek(SeekFrom::Start(0))
        .record_err(crate::types::GfrStatus::ErrorIo)?;
    let extract_dir = stage.tree();
    log::info!(
        "Unpacking tar archive to staging directory for: {}",
        out_dir_path
    );
    let mut archive = tar::Archive::new(temp_archive);
    archive.unpack(&extract_dir).map_err(|e| {
        log::error!("Failed to unpack tar archive: {}", e);
        set_last_error(&format!("decrypted data is not a valid archive: {}", e));
        crate::types::GfrStatus::ErrorInvalidData // Extraction failure may indicate the content is not a valid tar archive
    })?;
    drop(archive);

    // 4. Commit the extracted tree.
    stage
        .commit()
        .map_err(|e| stage_error("cannot move the extracted files into place", e))?;

    // 5. Assemble the return result (pure file stream operation, so payload data is empty)
    Ok(DecryptAndVerifyResultInternal {
        data: Vec::new(),
        filename: stream_result.filename,
        sym_algo: stream_result.sym_algo,
        recipients: stream_result.recipients,
        is_verified: stream_result.is_verified,
        signatures: stream_result.signatures,
    })
}

/// Decrypt (and optionally verify) a file into `out_file_path`.
///
/// SEIPD v1 payloads are streamed, so files of any size decrypt in bounded
/// memory (rPGP's default `CheckFirst` mode rejects messages over 1 GiB).
/// Because streaming releases plaintext before the integrity check, the output
/// is written to a [`StagedFile`] next to the destination and committed (fsync,
/// then an atomic rename) only after decryption, integrity checking and every
/// write have succeeded. On any error, including cancellation, the stage is
/// deleted and an existing `out_file_path` is left untouched.
#[allow(clippy::too_many_arguments)]
pub fn decrypt_and_verify_file_internal(
    channel: i32,
    in_file_path: &str,
    out_file_path: &str,
    ascii_armor: bool,
    fetch_seckey_cb: Option<GfrSecretKeyFetchCb>,
    fetch_pwd_cb: Option<GfrPasswordFetchCb>,
    fetch_pubkey_cb: Option<GfrPublicKeyFetchCb>,
    user_data: *mut c_void,
) -> Result<DecryptAndVerifyStreamResultInternal, GfrStatus> {
    decrypt_file_staged(
        channel,
        in_file_path,
        out_file_path,
        ascii_armor,
        fetch_seckey_cb,
        fetch_pwd_cb,
        fetch_pubkey_cb,
        user_data,
        plain_stage_writer,
    )
}

#[allow(clippy::too_many_arguments)]
fn decrypt_file_staged(
    channel: i32,
    in_file_path: &str,
    out_file_path: &str,
    ascii_armor: bool,
    fetch_seckey_cb: Option<GfrSecretKeyFetchCb>,
    fetch_pwd_cb: Option<GfrPasswordFetchCb>,
    fetch_pubkey_cb: Option<GfrPublicKeyFetchCb>,
    user_data: *mut c_void,
    stage_writer: StageWriter,
) -> Result<DecryptAndVerifyStreamResultInternal, GfrStatus> {
    let (in_file, ciphertext_len) = open_ciphertext(in_file_path)?;
    let mut stage = super::staging::StagedFile::new(Path::new(out_file_path))
        .map_err(|e| stage_error("cannot create the output file", e))?;

    let stream_result = decrypt_message_stream(
        channel,
        in_file,
        stage_writer(stage.file_mut()),
        ascii_armor,
        fetch_seckey_cb,
        fetch_pwd_cb,
        fetch_pubkey_cb,
        user_data,
        // Safe: `stage` is deleted unless this returns `Ok`.
        Sink::Staged { ciphertext_len },
    )?;

    stage
        .commit()
        .map_err(|e| stage_error("cannot move the decrypted output into place", e))?;

    Ok(stream_result)
}

/// Decrypt an in-memory buffer, auto-detecting ASCII armor. SEIPD v1 is
/// authenticated before release (`CheckFirst`, up to [`IN_MEMORY_SEIPDV1_LIMIT`]).
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
    let stream_result = decrypt_message_stream(
        channel,
        input_cursor,
        &mut output_data, // Passes as Write stream
        is_armor,
        fetch_seckey_cb,
        fetch_pwd_cb,
        None, // fetch_pubkey_cb is not needed for decryption-only
        user_data,
        MEMORY_SINK,
    )?;

    Ok(DecryptResultInternal {
        data: output_data,
        filename: stream_result.filename,
        sym_algo: stream_result.sym_algo,
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

    let stream_result = decrypt_message_stream(
        channel,
        input_cursor,
        &mut output_data,
        is_armor,
        fetch_seckey_cb,
        fetch_pwd_cb,
        fetch_pubkey_cb,
        user_data,
        MEMORY_SINK,
    )?;

    Ok(DecryptAndVerifyResultInternal {
        data: output_data,
        filename: stream_result.filename,
        sym_algo: stream_result.sym_algo,
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
    fn decrypt_with_key(
        data: &[u8],
        secret_block: &str,
        passphrase_cb: Option<GfrPasswordFetchCb>,
    ) -> Result<DecryptResultInternal, GfrStatus> {
        cb::set_seckey_answer(secret_block);
        decrypt_internal(
            0,
            data,
            Some(cb::seckey_fetch),
            passphrase_cb,
            std::ptr::null_mut(),
        )
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
        // The container is reported in gpgme's `symkey_algo` form.
        assert_eq!(res.sym_algo, "AES128.EAX");
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
        let (locked, _) = pgp::composed::SignedSecretKey::from_string(rfc9580::A5_V6_SECRET_LOCKED)
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
        let res = decrypt_with_key(
            corpus::ENC_V1SEIPD_MDC,
            corpus::KEY1_SECRET,
            Some(pw_corpus),
        )
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
        let res = decrypt_with_key(
            corpus::ENC_V1SEIPD_MDC,
            corpus::KEY1_SECRET,
            Some(pw_corpus),
        )
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
            decrypt_with_key(corpus::PKESK_NO_SEIPD, corpus::KEY1_SECRET, Some(pw_corpus)).is_err()
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
        assert!(decrypt_with_key(&tampered, rfc9580::A4_V6_SECRET_UNLOCKED, None).is_err());
    }

    #[test]
    fn a_truncated_ciphertext_fails_cleanly() {
        for n in [16usize, 64, 128] {
            let prefix = packets::truncate_at(corpus::ENC_V1SEIPD_MDC, n);
            let outcome = std::panic::catch_unwind(|| {
                decrypt_with_key(&prefix, corpus::KEY1_SECRET, Some(pw_corpus)).is_ok()
            });
            assert_eq!(
                outcome.ok(),
                Some(false),
                "a {n}-byte prefix must not decrypt"
            );
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

    // -- SEIPD v1 read mode and the file paths -----------------------------------

    /// A binary v1 SEIPD message (v4 recipient) carrying `len` bytes.
    fn v1_message(len: usize) -> (Vec<u8>, Vec<u8>) {
        let payload: Vec<u8> = (0..=255u8).cycle().take(len).collect();
        let out = crate::crypto::encrypt_internal(
            0,
            "",
            &payload,
            &[&keys::V4_SIGN.public_armored],
            false,
        )
        .expect("encrypt");
        (payload, out.data)
    }

    fn stream_decrypt(data: &[u8], sink: Sink) -> Result<Vec<u8>, GfrStatus> {
        cb::set_seckey_answer(&keys::V4_SIGN.secret_armored);
        let mut out = Vec::new();
        decrypt_message_stream(
            0,
            Cursor::new(data),
            &mut out,
            false,
            Some(cb::seckey_fetch),
            None,
            None,
            std::ptr::null_mut(),
            sink,
        )
        .map(|_| out)
    }

    const STAGED: Sink = Sink::Staged { ciphertext_len: 0 };

    /// Flip a byte inside the trailing MDC packet of a binary v1 message.
    fn tamper_mdc(data: &mut [u8]) {
        let last = data.len() - 8;
        data[last] ^= 0xFF;
    }

    #[test]
    fn check_first_mode_refuses_a_v1_message_over_its_cap() {
        // Pins the upstream trigger of the "Input stream too long for
        // ProtectedCheckFirst mode" report: rPGP buffers a v1 payload in full
        // and rejects it past `max_message_size` (1 GiB by default).
        let (_, data) = v1_message(64 * 1024);
        let sink = Sink::Memory {
            seipdv1_limit: 1024,
        };
        assert!(stream_decrypt(&data, sink).is_err());
    }

    #[test]
    fn streaming_mode_decrypts_a_v1_message_over_the_check_first_cap() {
        let (payload, data) = v1_message(64 * 1024);
        assert_eq!(stream_decrypt(&data, STAGED).expect("decrypt"), payload);
    }

    #[test]
    fn streaming_mode_still_detects_a_tampered_mdc() {
        let (_, mut data) = v1_message(64 * 1024);
        tamper_mdc(&mut data);
        assert!(stream_decrypt(&data, STAGED).is_err());
    }

    fn decrypt_file(in_path: &Path, out_path: &Path) -> Result<(), GfrStatus> {
        cb::set_seckey_answer(&keys::V4_SIGN.secret_armored);
        decrypt_and_verify_file_internal(
            0,
            in_path.to_str().unwrap(),
            out_path.to_str().unwrap(),
            false,
            Some(cb::seckey_fetch),
            None,
            None,
            std::ptr::null_mut(),
        )
        .map(|_| ())
    }

    fn dir_entries(dir: &Path) -> Vec<std::path::PathBuf> {
        let mut entries: Vec<_> = std::fs::read_dir(dir)
            .expect("read_dir")
            .map(|e| e.expect("entry").path())
            .collect();
        entries.sort();
        entries
    }

    #[test]
    fn a_v1_file_decrypts_to_the_output_path() {
        let dir = tempfile::tempdir().expect("tempdir");
        let (payload, data) = v1_message(64 * 1024);
        let in_path = dir.path().join("in.gpg");
        let out_path = dir.path().join("out.bin");
        std::fs::write(&in_path, &data).expect("write");

        decrypt_file(&in_path, &out_path).expect("decrypt");
        assert_eq!(std::fs::read(&out_path).expect("read"), payload);
        assert_eq!(dir_entries(dir.path()), vec![in_path, out_path]);
    }

    #[test]
    fn a_tampered_v1_file_leaves_no_output_behind() {
        // Streaming releases plaintext before the MDC is checked, so the file
        // path must stage it and never leave it under the output name.
        let dir = tempfile::tempdir().expect("tempdir");
        let (_, mut data) = v1_message(64 * 1024);
        tamper_mdc(&mut data);
        let in_path = dir.path().join("in.gpg");
        let out_path = dir.path().join("out.bin");
        std::fs::write(&in_path, &data).expect("write");

        assert!(decrypt_file(&in_path, &out_path).is_err());
        assert!(!out_path.exists(), "no plaintext may reach the output path");
        assert_eq!(
            dir_entries(dir.path()),
            vec![in_path],
            "the staging file must be removed"
        );
    }

    #[test]
    fn a_failed_file_decrypt_keeps_an_existing_output_file() {
        let dir = tempfile::tempdir().expect("tempdir");
        let (_, mut data) = v1_message(1024);
        tamper_mdc(&mut data);
        let in_path = dir.path().join("in.gpg");
        let out_path = dir.path().join("out.bin");
        std::fs::write(&in_path, &data).expect("write");
        std::fs::write(&out_path, b"previous").expect("write");

        assert!(decrypt_file(&in_path, &out_path).is_err());
        assert_eq!(std::fs::read(&out_path).expect("read"), b"previous");
    }

    #[test]
    fn a_tampered_v1_archive_extracts_nothing() {
        let dir = tempfile::tempdir().expect("tempdir");
        let (_, mut data) = v1_message(64 * 1024);
        tamper_mdc(&mut data);
        let in_path = dir.path().join("in.gpg");
        let out_dir = dir.path().join("out");
        std::fs::write(&in_path, &data).expect("write");

        cb::set_seckey_answer(&keys::V4_SIGN.secret_armored);
        let res = decrypt_and_verify_archive_internal(
            0,
            in_path.to_str().unwrap(),
            out_dir.to_str().unwrap(),
            false,
            Some(cb::seckey_fetch),
            None,
            None,
            std::ptr::null_mut(),
        );
        assert!(res.is_err());
        assert!(
            !out_dir.exists(),
            "a failed archive decrypt creates nothing"
        );
        assert_eq!(
            dir_entries(dir.path()),
            vec![in_path],
            "the stage is removed"
        );
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
            msg.literal_data_header().map(|h| h.file_name().to_vec()),
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

#[cfg(test)]
mod seipd_file_tests {
    //! SEIPD v1 / v2 file and archive decryption: the bounded-memory streaming
    //! path, its integrity guarantee (no unauthenticated plaintext is ever
    //! committed), and its interop with other implementations.
    //!
    //! Everything here is fast. Real files over the former 1 GiB `CheckFirst`
    //! limit live in `large_file_tests` (the dedicated large-file tier).

    use super::*;
    use crate::testutil::{cb, corpus, interop, keys, seipd};

    const SEED: u64 = 0x5E1D_0001;
    const PAYLOAD: usize = 256 * 1024;

    // -- helpers ---------------------------------------------------------------

    fn names(dir: &Path) -> Vec<String> {
        let mut names: Vec<String> = std::fs::read_dir(dir)
            .expect("read_dir")
            .map(|e| e.expect("entry").file_name().to_string_lossy().into_owned())
            .collect();
        names.sort();
        names
    }

    /// Decrypt `data` through the file path into `dir/out.bin`, as `key`,
    /// verifying against `signer_cert` when given.
    fn decrypt_file_as(
        dir: &Path,
        data: &[u8],
        key: &str,
        signer_cert: Option<&str>,
        stage_writer: StageWriter,
    ) -> Result<DecryptAndVerifyStreamResultInternal, GfrStatus> {
        let in_path = dir.join("in.pgp");
        std::fs::write(&in_path, data).unwrap();
        cb::set_seckey_answer(key);
        if let Some(cert) = signer_cert {
            cb::set_pubkey_answer(cert);
        }
        decrypt_file_staged(
            0,
            in_path.to_str().unwrap(),
            dir.join("out.bin").to_str().unwrap(),
            false,
            Some(cb::seckey_fetch),
            Some(cb::pwd_corpus),
            Some(if signer_cert.is_some() {
                cb::pubkey_fetch
            } else {
                cb::pubkey_none
            }),
            std::ptr::null_mut(),
            stage_writer,
        )
    }

    /// Assert a failed file decryption committed nothing: only the input is
    /// left (no output, no stage), or `out.bin` still holds `previous`.
    fn assert_nothing_committed(dir: &Path, previous: Option<&[u8]>) {
        match previous {
            None => assert_eq!(
                names(dir),
                vec!["in.pgp"],
                "no output and no stage may remain"
            ),
            Some(bytes) => {
                assert_eq!(names(dir), vec!["in.pgp", "out.bin"], "no stage may remain");
                assert_eq!(std::fs::read(dir.join("out.bin")).unwrap(), bytes);
            }
        }
    }

    fn memory_decrypt(data: &[u8], key: &str, limit: usize) -> Result<Vec<u8>, GfrStatus> {
        cb::set_seckey_answer(key);
        let mut out = Vec::new();
        decrypt_message_stream(
            0,
            Cursor::new(data),
            &mut out,
            false,
            Some(cb::seckey_fetch),
            Some(cb::pwd_corpus),
            None,
            std::ptr::null_mut(),
            Sink::Memory {
                seipdv1_limit: limit,
            },
        )
        .map(|_| out)
    }

    fn v1(len: usize) -> (Vec<u8>, Vec<u8>) {
        let payload = seipd::bytes(len, SEED);
        let data = seipd::message(&keys::V4_SIGN.public_armored, &payload, None);
        assert_eq!(
            crate::inspect::inspect(&data).blocks[0]
                .packets
                .last()
                .unwrap()
                .version,
            Some(1)
        );
        (payload, data)
    }

    /// The smallest `CheckFirst` limit that accepts `data`.
    fn check_first_threshold(data: &[u8]) -> usize {
        let key = &keys::V4_SIGN.secret_armored;
        let (mut lo, mut hi) = (1usize, data.len() * 2);
        assert!(memory_decrypt(data, key, hi).is_ok());
        while lo < hi {
            let mid = (lo + hi) / 2;
            if memory_decrypt(data, key, mid).is_ok() {
                hi = mid;
            } else {
                lo = mid + 1;
            }
        }
        lo
    }

    // -- the read mode is fixed by the sink --------------------------------------

    #[test]
    fn only_the_staged_sink_streams_seipd_v1() {
        assert_eq!(
            MEMORY_SINK.seipdv1_mode(),
            Seipdv1ReadMode::CheckFirst {
                max_message_size: IN_MEMORY_SEIPDV1_LIMIT
            }
        );
        assert_eq!(IN_MEMORY_SEIPDV1_LIMIT, 1 << 30);
        assert_eq!(
            Sink::Staged { ciphertext_len: 0 }.seipdv1_mode(),
            Seipdv1ReadMode::Streaming
        );
    }

    #[test]
    fn the_staged_decompression_cap_scales_with_the_ciphertext() {
        assert_eq!(
            MEMORY_SINK.decompression_cap(),
            MAX_DECOMPRESSED_OUTPUT_BYTES
        );
        // A 4.25 GiB GnuPG file must not hit the in-memory 4 GiB cap.
        let big = 4 * (1u64 << 30) + (1 << 28);
        assert!(
            Sink::Staged {
                ciphertext_len: big
            }
            .decompression_cap()
                >= big
        );
        // A tiny bomb is still stopped.
        assert_eq!(
            Sink::Staged {
                ciphertext_len: 1024
            }
            .decompression_cap(),
            MAX_DECOMPRESSED_OUTPUT_BYTES
        );
    }

    #[test]
    fn check_first_accepts_exactly_up_to_its_limit_and_the_file_path_ignores_it() {
        // Below, at and above a reduced CheckFirst limit, with the exact
        // threshold measured rather than guessed. The file path decrypts all
        // three, because it never uses CheckFirst.
        let (_, probe) = v1(16 * 1024);
        let overhead = check_first_threshold(&probe) - 16 * 1024;
        let limit = 64 * 1024;
        let at = limit - overhead;
        let key = &keys::V4_SIGN.secret_armored;

        for (len, memory_ok) in [(at / 2, true), (at, true), (at + 1, false), (at * 4, false)] {
            let (payload, data) = v1(len);
            if len == at {
                assert_eq!(
                    check_first_threshold(&data),
                    limit,
                    "payload {len} sits exactly at the limit"
                );
            }
            assert_eq!(
                memory_decrypt(&data, key, limit).is_ok(),
                memory_ok,
                "CheckFirst({limit}) on a {len}-byte payload"
            );

            let dir = tempfile::tempdir().unwrap();
            decrypt_file_as(dir.path(), &data, key, None, plain_stage_writer)
                .unwrap_or_else(|e| panic!("file path on a {len}-byte payload: {e:?}"));
            assert_eq!(std::fs::read(dir.path().join("out.bin")).unwrap(), payload);
        }
    }

    #[test]
    fn an_armored_file_decrypts_through_the_binary_file_path() {
        // The C++ layer always passes ascii=false; rPGP sniffs the armor.
        let payload = seipd::bytes(PAYLOAD, SEED);
        let armored = crate::crypto::encrypt_internal(
            0,
            "",
            &payload,
            &[&keys::V4_SIGN.public_armored],
            true,
        )
        .expect("encrypt")
        .data;
        let dir = tempfile::tempdir().unwrap();
        decrypt_file_as(
            dir.path(),
            &armored,
            &keys::V4_SIGN.secret_armored,
            None,
            plain_stage_writer,
        )
        .expect("decrypt");
        assert_eq!(std::fs::read(dir.path().join("out.bin")).unwrap(), payload);
    }

    // -- integrity: corruption never yields committed output -----------------------

    #[test]
    fn every_byte_of_the_final_mdc_is_checked() {
        // The 22 trailing ciphertext bytes encrypt the MDC packet (tag, length,
        // SHA-1). Flipping any of them must fail as corrupted data, with
        // nothing committed, through both the file and the memory path.
        let (_, data) = v1(PAYLOAD);
        let key = &keys::V4_SIGN.secret_armored;
        for back in 1..=22isize {
            let tampered = seipd::flipped(&data, -back);
            let dir = tempfile::tempdir().unwrap();
            assert_eq!(
                decrypt_file_as(dir.path(), &tampered, key, None, plain_stage_writer).err(),
                Some(GfrStatus::ErrorDataCorrupted),
                "byte -{back}"
            );
            assert_nothing_committed(dir.path(), None);
            assert_eq!(
                memory_decrypt(&tampered, key, IN_MEMORY_SEIPDV1_LIMIT).err(),
                Some(GfrStatus::ErrorDataCorrupted),
                "memory path, byte -{back}"
            );
        }
        assert!(crate::err::last_error().contains("integrity check failed"));
    }

    #[test]
    fn tampering_anywhere_in_the_payload_commits_nothing() {
        let (_, data) = v1(PAYLOAD);
        let key = &keys::V4_SIGN.secret_armored;
        let len = data.len() as isize;
        for at in [-(len / 2), -(len / 3), -1024, -23] {
            let dir = tempfile::tempdir().unwrap();
            assert!(
                decrypt_file_as(
                    dir.path(),
                    &seipd::flipped(&data, at),
                    key,
                    None,
                    plain_stage_writer
                )
                .is_err(),
                "tamper at {at}"
            );
            assert_nothing_committed(dir.path(), None);
        }
    }

    #[test]
    fn truncated_ciphertext_commits_nothing() {
        let (_, data) = v1(PAYLOAD);
        let key = &keys::V4_SIGN.secret_armored;
        for cut in [1usize, 21, 22, 23, 4096, data.len() / 2, data.len() - 200] {
            let dir = tempfile::tempdir().unwrap();
            let short = &data[..data.len() - cut];
            assert!(
                decrypt_file_as(dir.path(), short, key, None, plain_stage_writer).is_err(),
                "truncated by {cut}"
            );
            assert_nothing_committed(dir.path(), None);
        }
    }

    #[test]
    fn malformed_ciphertext_commits_nothing() {
        let key = &keys::V4_SIGN.secret_armored;
        for data in [
            &b""[..],
            b"garbage",
            corpus::GARBAGE,
            corpus::PKESK_NO_SEIPD,
        ] {
            let dir = tempfile::tempdir().unwrap();
            assert!(decrypt_file_as(dir.path(), data, key, None, plain_stage_writer).is_err());
            assert_nothing_committed(dir.path(), None);
        }
    }

    #[test]
    fn a_failed_decryption_preserves_an_existing_output_file() {
        let (_, data) = v1(PAYLOAD);
        let dir = tempfile::tempdir().unwrap();
        std::fs::write(dir.path().join("out.bin"), b"previous").unwrap();
        assert!(
            decrypt_file_as(
                dir.path(),
                &seipd::flipped(&data, -1),
                &keys::V4_SIGN.secret_armored,
                None,
                plain_stage_writer
            )
            .is_err()
        );
        assert_nothing_committed(dir.path(), Some(b"previous"));
    }

    #[test]
    fn a_successful_decryption_replaces_an_existing_output_file() {
        let (payload, data) = v1(PAYLOAD);
        let dir = tempfile::tempdir().unwrap();
        std::fs::write(dir.path().join("out.bin"), b"previous").unwrap();
        decrypt_file_as(
            dir.path(),
            &data,
            &keys::V4_SIGN.secret_armored,
            None,
            plain_stage_writer,
        )
        .expect("decrypt");
        assert_eq!(std::fs::read(dir.path().join("out.bin")).unwrap(), payload);
        assert_eq!(names(dir.path()), vec!["in.pgp", "out.bin"]);
    }

    #[cfg(unix)]
    #[test]
    fn output_permissions_are_private_for_new_files_and_kept_for_replaced_ones() {
        use std::os::unix::fs::PermissionsExt;
        let (_, data) = v1(1024);
        let key = &keys::V4_SIGN.secret_armored;
        let dir = tempfile::tempdir().unwrap();
        let out = dir.path().join("out.bin");
        decrypt_file_as(dir.path(), &data, key, None, plain_stage_writer).expect("decrypt");
        assert_eq!(
            std::fs::metadata(&out).unwrap().permissions().mode() & 0o777,
            0o600
        );
        std::fs::set_permissions(&out, std::fs::Permissions::from_mode(0o644)).unwrap();
        decrypt_file_as(dir.path(), &data, key, None, plain_stage_writer).expect("decrypt");
        assert_eq!(
            std::fs::metadata(&out).unwrap().permissions().mode() & 0o777,
            0o644
        );
    }

    // -- fault injection -------------------------------------------------------------

    /// A stage that runs out of space after 64 KiB.
    struct FullDisk<'a> {
        file: &'a mut File,
        left: usize,
    }

    impl Write for FullDisk<'_> {
        fn write(&mut self, buf: &[u8]) -> std::io::Result<usize> {
            if self.left == 0 {
                return Err(std::io::Error::from(std::io::ErrorKind::StorageFull));
            }
            let n = buf.len().min(self.left);
            self.left -= n;
            self.file.write(&buf[..n])
        }
        fn flush(&mut self) -> std::io::Result<()> {
            self.file.flush()
        }
    }

    fn full_disk_writer(file: &mut File) -> Box<dyn Write + Send + Sync + '_> {
        Box::new(FullDisk {
            file,
            left: 64 * 1024,
        })
    }

    const CANCEL_CHANNEL: i32 = 0x5E1D;

    /// A stage that requests cancellation of [`CANCEL_CHANNEL`] after its
    /// first write, i.e. in the middle of the stream.
    struct CancelAfterFirstWrite<'a>(&'a mut File);

    impl Write for CancelAfterFirstWrite<'_> {
        fn write(&mut self, buf: &[u8]) -> std::io::Result<usize> {
            let n = self.0.write(buf)?;
            crate::cancel::set_cancelled(CANCEL_CHANNEL, true);
            Ok(n)
        }
        fn flush(&mut self) -> std::io::Result<()> {
            self.0.flush()
        }
    }

    fn cancelling_writer(file: &mut File) -> Box<dyn Write + Send + Sync + '_> {
        Box::new(CancelAfterFirstWrite(file))
    }

    #[test]
    fn a_full_disk_is_an_output_error_and_commits_nothing() {
        let (_, data) = v1(PAYLOAD);
        let dir = tempfile::tempdir().unwrap();
        std::fs::write(dir.path().join("out.bin"), b"previous").unwrap();
        assert_eq!(
            decrypt_file_as(
                dir.path(),
                &data,
                &keys::V4_SIGN.secret_armored,
                None,
                full_disk_writer
            )
            .err(),
            Some(GfrStatus::ErrorIo)
        );
        assert!(crate::err::last_error().contains("cannot write the decrypted output"));
        assert_nothing_committed(dir.path(), Some(b"previous"));
    }

    #[test]
    fn cancelling_mid_stream_commits_nothing() {
        let (_, data) = v1(PAYLOAD);
        let dir = tempfile::tempdir().unwrap();
        let in_path = dir.path().join("in.pgp");
        std::fs::write(&in_path, &data).unwrap();
        cb::set_seckey_answer(&keys::V4_SIGN.secret_armored);
        crate::cancel::set_cancelled(CANCEL_CHANNEL, false);
        let res = decrypt_file_staged(
            CANCEL_CHANNEL,
            in_path.to_str().unwrap(),
            dir.path().join("out.bin").to_str().unwrap(),
            false,
            Some(cb::seckey_fetch),
            None,
            None,
            std::ptr::null_mut(),
            cancelling_writer,
        );
        crate::cancel::set_cancelled(CANCEL_CHANNEL, false);
        assert_eq!(res.err(), Some(GfrStatus::ErrorCanceled));
        assert_nothing_committed(dir.path(), None);
    }

    #[test]
    fn a_failed_commit_keeps_the_destination_and_removes_the_stage() {
        // The destination is a directory, which a file cannot replace.
        let (_, data) = v1(PAYLOAD);
        let dir = tempfile::tempdir().unwrap();
        std::fs::create_dir(dir.path().join("out.bin")).unwrap();
        assert_eq!(
            decrypt_file_as(
                dir.path(),
                &data,
                &keys::V4_SIGN.secret_armored,
                None,
                plain_stage_writer
            )
            .err(),
            Some(GfrStatus::ErrorIo)
        );
        assert_eq!(names(dir.path()), vec!["in.pgp", "out.bin"]);
        assert!(dir.path().join("out.bin").is_dir());
    }

    #[test]
    fn a_missing_output_directory_or_input_creates_nothing() {
        let (_, data) = v1(1024);
        let dir = tempfile::tempdir().unwrap();
        let in_path = dir.path().join("in.pgp");
        std::fs::write(&in_path, &data).unwrap();
        cb::set_seckey_answer(&keys::V4_SIGN.secret_armored);
        let run = |input: &Path, output: &Path| {
            decrypt_and_verify_file_internal(
                0,
                input.to_str().unwrap(),
                output.to_str().unwrap(),
                false,
                Some(cb::seckey_fetch),
                None,
                None,
                std::ptr::null_mut(),
            )
            .err()
        };
        assert_eq!(
            run(&in_path, &dir.path().join("missing/out.bin")),
            Some(GfrStatus::ErrorIo)
        );
        assert_eq!(
            run(&dir.path().join("absent.pgp"), &dir.path().join("out.bin")),
            Some(GfrStatus::ErrorInvalidInput)
        );
        assert_eq!(names(dir.path()), vec!["in.pgp"]);
    }

    // -- signatures are reported separately from integrity -----------------------------

    #[test]
    fn a_signed_message_reports_mdc_and_signature_results_separately() {
        let payload = seipd::bytes(PAYLOAD, SEED);
        let key = &*keys::V4_SIGN;
        let data = seipd::message(&key.public_armored, &payload, Some(key));

        // Authentic, signer known: plaintext committed and verified.
        let dir = tempfile::tempdir().unwrap();
        let res = decrypt_file_as(
            dir.path(),
            &data,
            &key.secret_armored,
            Some(&key.public_armored),
            plain_stage_writer,
        )
        .expect("decrypt");
        assert!(res.is_verified);
        assert!(
            res.signatures
                .iter()
                .all(|s| s.status == GfrSignatureStatus::Valid)
        );
        assert_eq!(std::fs::read(dir.path().join("out.bin")).unwrap(), payload);

        // Authentic, signer unknown: integrity holds, so the plaintext is
        // committed, but the signature is not reported valid.
        let dir = tempfile::tempdir().unwrap();
        let res = decrypt_file_as(
            dir.path(),
            &data,
            &key.secret_armored,
            None,
            plain_stage_writer,
        )
        .expect("an unverifiable signature does not make the data unauthentic");
        assert!(!res.is_verified);
        assert!(!res.signatures.is_empty());
        assert!(
            res.signatures
                .iter()
                .all(|s| s.status != GfrSignatureStatus::Valid)
        );
        assert_eq!(std::fs::read(dir.path().join("out.bin")).unwrap(), payload);

        // MDC broken: an integrity failure, never a signature result.
        let dir = tempfile::tempdir().unwrap();
        assert_eq!(
            decrypt_file_as(
                dir.path(),
                &seipd::flipped(&data, -1),
                &key.secret_armored,
                Some(&key.public_armored),
                plain_stage_writer
            )
            .err(),
            Some(GfrStatus::ErrorDataCorrupted)
        );
        assert_nothing_committed(dir.path(), None);
    }

    // -- formats: legacy rPGP output, post-quantum keys, SEIPD v2 ------------------------

    #[test]
    fn legacy_rpgp_v1_output_to_v4_v6_and_pq_keys_still_decrypts() {
        // What GpgFrontend wrote before format selection existed, including
        // the original report's post-quantum (ML-KEM) key.
        let payload = seipd::bytes(PAYLOAD, SEED);
        for key in [&*keys::V4_SIGN, &*keys::V6_SIGN, &*keys::V6_PQ] {
            let data = seipd::legacy_v1_message(key, Cursor::new(payload.clone()));
            let dir = tempfile::tempdir().unwrap();
            decrypt_file_as(
                dir.path(),
                &data,
                &key.secret_armored,
                None,
                plain_stage_writer,
            )
            .expect("legacy v1 file");
            assert_eq!(std::fs::read(dir.path().join("out.bin")).unwrap(), payload);

            let dir = tempfile::tempdir().unwrap();
            assert_eq!(
                decrypt_file_as(
                    dir.path(),
                    &seipd::flipped(&data, -1),
                    &key.secret_armored,
                    None,
                    plain_stage_writer
                )
                .err(),
                Some(GfrStatus::ErrorDataCorrupted)
            );
            assert_nothing_committed(dir.path(), None);
        }
    }

    #[test]
    fn seipd_v2_files_get_the_same_transactional_guarantee() {
        let payload = seipd::bytes(PAYLOAD, SEED);
        for key in [&*keys::V6_SIGN, &*keys::V6_PQ] {
            let data = seipd::message(&key.public_armored, &payload, None);
            assert_eq!(
                crate::inspect::inspect(&data).blocks[0]
                    .packets
                    .last()
                    .unwrap()
                    .version,
                Some(2)
            );
            let dir = tempfile::tempdir().unwrap();
            decrypt_file_as(
                dir.path(),
                &data,
                &key.secret_armored,
                None,
                plain_stage_writer,
            )
            .expect("v2");
            assert_eq!(std::fs::read(dir.path().join("out.bin")).unwrap(), payload);

            for back in [1isize, 16, 17, 4096] {
                let dir = tempfile::tempdir().unwrap();
                assert!(
                    decrypt_file_as(
                        dir.path(),
                        &seipd::flipped(&data, -back),
                        &key.secret_armored,
                        None,
                        plain_stage_writer
                    )
                    .is_err(),
                    "v2 tamper at -{back}"
                );
                assert_nothing_committed(dir.path(), None);
            }
        }
    }

    // -- archives ------------------------------------------------------------------------

    fn sample_tree(root: &Path) {
        std::fs::create_dir_all(root.join("sub")).unwrap();
        std::fs::write(root.join("a.bin"), seipd::bytes(PAYLOAD, SEED)).unwrap();
        std::fs::write(root.join("sub/b.txt"), b"nested").unwrap();
    }

    fn encrypt_tree(src: &Path, out: &Path, key: &keys::Fixture) {
        crate::crypto::encrypt_directory_internal(
            0,
            src.to_str().unwrap(),
            out.to_str().unwrap(),
            &[&key.public_armored],
            false,
        )
        .expect("encrypt directory");
    }

    fn decrypt_archive(
        input: &Path,
        out_dir: &Path,
        key: &str,
        writer: StageWriter,
    ) -> Result<(), GfrStatus> {
        cb::set_seckey_answer(key);
        decrypt_archive_staged(
            0,
            input.to_str().unwrap(),
            out_dir.to_str().unwrap(),
            false,
            Some(cb::seckey_fetch),
            None,
            None,
            std::ptr::null_mut(),
            writer,
        )
        .map(|_| ())
    }

    #[test]
    fn an_archive_round_trips_into_a_new_and_an_existing_directory() {
        let dir = tempfile::tempdir().unwrap();
        let src = dir.path().join("src");
        sample_tree(&src);
        let enc = dir.path().join("tree.tar.gpg");
        encrypt_tree(&src, &enc, &keys::V4_SIGN);
        let key = &keys::V4_SIGN.secret_armored;

        let fresh = dir.path().join("fresh");
        decrypt_archive(&enc, &fresh, key, plain_stage_writer).expect("decrypt");
        // The archive holds the directory's contents at its top level.
        assert_eq!(names(&fresh), vec!["a.bin", "sub"]);
        assert_eq!(std::fs::read(fresh.join("sub/b.txt")).unwrap(), b"nested");
        assert_eq!(
            std::fs::read(fresh.join("a.bin")).unwrap(),
            seipd::bytes(PAYLOAD, SEED)
        );

        let existing = dir.path().join("existing");
        std::fs::create_dir(&existing).unwrap();
        std::fs::write(existing.join("keep.txt"), b"keep").unwrap();
        decrypt_archive(&enc, &existing, key, plain_stage_writer).expect("decrypt");
        assert_eq!(std::fs::read(existing.join("keep.txt")).unwrap(), b"keep");
        assert_eq!(
            names(&existing),
            vec!["a.bin", "keep.txt", "sub"],
            "merged into"
        );
        assert!(
            names(dir.path()).iter().all(|n| !n.starts_with(".gf-")),
            "no stage may remain"
        );
    }

    #[test]
    fn a_failed_archive_decryption_leaves_the_destination_untouched() {
        let dir = tempfile::tempdir().unwrap();
        let src = dir.path().join("src");
        sample_tree(&src);
        let enc = dir.path().join("tree.tar.gpg");
        encrypt_tree(&src, &enc, &keys::V4_SIGN);
        let key = &keys::V4_SIGN.secret_armored;
        let good = std::fs::read(&enc).unwrap();

        let existing = dir.path().join("existing");
        std::fs::create_dir(&existing).unwrap();
        std::fs::write(existing.join("keep.txt"), b"keep").unwrap();
        let tampered = dir.path().join("tampered.tar.gpg");
        std::fs::write(&tampered, seipd::flipped(&good, -1)).unwrap();
        let not_tar = dir.path().join("not-a-tar.gpg");
        std::fs::write(
            &not_tar,
            seipd::message(&keys::V4_SIGN.public_armored, &[0xAB; 4096], None),
        )
        .unwrap();

        let cases: [(&Path, StageWriter, GfrStatus); 3] = [
            (&tampered, plain_stage_writer, GfrStatus::ErrorDataCorrupted),
            (&enc, full_disk_writer, GfrStatus::ErrorIo),
            (&not_tar, plain_stage_writer, GfrStatus::ErrorInvalidData),
        ];
        for (input, writer, want) in cases {
            let fresh = dir.path().join("fresh");
            assert_eq!(
                decrypt_archive(input, &fresh, key, writer).err(),
                Some(want),
                "{input:?}"
            );
            assert!(
                !fresh.exists(),
                "a failed archive decryption creates nothing"
            );
            assert_eq!(
                decrypt_archive(input, &existing, key, writer).err(),
                Some(want)
            );
            assert_eq!(names(&existing), vec!["keep.txt"]);
        }
        assert!(
            names(dir.path()).iter().all(|n| !n.starts_with(".gf-")),
            "no stage may remain"
        );
    }

    // -- interop matrix ------------------------------------------------------------------

    /// SEIPD v1 messages from GnuPG, every compression it offers, signed and
    /// not: each decrypts and verifies, and each fails as corrupted with
    /// nothing committed when its MDC is broken. GnuPG compresses by default,
    /// which exercises the MDC check after a decompressor reached its end.
    #[test]
    fn gnupg_seipd_v1_matrix() {
        let Some(gnupg) = interop::Gnupg::new() else {
            return;
        };
        // GnuPG 2.4 implements neither RFC 9580's native Ed25519/X25519
        // (algorithms 27/25) nor v6 keys, so it encrypts to a P-256 v4 key.
        let key = &*keys::V4_NISTP256;
        gnupg.import(&key.public_armored);
        let payload = seipd::bytes(PAYLOAD, SEED);

        for compress in ["zlib", "zip", "bzip2", "none"] {
            for sign in [false, true] {
                let case = format!("gpg compress={compress} sign={sign}");
                let data = gnupg.encrypt(&key.primary_fpr, &payload, compress, sign);
                let nodes = crate::inspect::inspect(&data).blocks[0].packets.clone();
                let seipd = nodes
                    .iter()
                    .find(|n| n.tag == 18)
                    .unwrap_or_else(|| panic!("{case}: no SEIPD"));
                assert_eq!(seipd.version, Some(1), "{case}");

                let dir = tempfile::tempdir().unwrap();
                let cert = sign.then_some(gnupg.cert.as_str());
                let res = decrypt_file_as(
                    dir.path(),
                    &data,
                    &key.secret_armored,
                    cert,
                    plain_stage_writer,
                )
                .unwrap_or_else(|e| panic!("{case}: {e:?}"));
                assert_eq!(
                    std::fs::read(dir.path().join("out.bin")).unwrap(),
                    payload,
                    "{case}"
                );
                assert_eq!(res.is_verified, sign, "{case}");
                assert_eq!(
                    memory_decrypt(&data, &key.secret_armored, IN_MEMORY_SEIPDV1_LIMIT)
                        .expect("memory"),
                    payload,
                    "{case}"
                );

                for back in [1isize, 10, 22] {
                    let dir = tempfile::tempdir().unwrap();
                    assert_eq!(
                        decrypt_file_as(
                            dir.path(),
                            &seipd::flipped(&data, -back),
                            &key.secret_armored,
                            cert,
                            plain_stage_writer
                        )
                        .err(),
                        Some(GfrStatus::ErrorDataCorrupted),
                        "{case}, MDC byte -{back}"
                    );
                    assert_nothing_committed(dir.path(), None);
                }
                let dir = tempfile::tempdir().unwrap();
                assert!(
                    decrypt_file_as(
                        dir.path(),
                        &seipd::flipped(&data, -(data.len() as isize / 2)),
                        &key.secret_armored,
                        cert,
                        plain_stage_writer
                    )
                    .is_err(),
                    "{case}, mid-stream tamper"
                );
                assert_nothing_committed(dir.path(), None);
            }
        }

        // And the reverse direction: GnuPG reads the v1 output we send its key,
        // plain and signed.
        for signer in [None, Some(&*keys::V4_NISTP256)] {
            let ours = seipd::message(&gnupg.cert, &payload, signer);
            assert_eq!(
                gnupg.decrypt(&ours),
                payload,
                "gnupg reads ours, signed={}",
                signer.is_some()
            );
        }
    }

    /// SEIPD v1 and v2 messages from Sequoia, signed and not.
    #[test]
    fn sequoia_seipd_v1_and_v2_matrix() {
        let Some(sq) = interop::Sq::new() else {
            return;
        };
        let payload = seipd::bytes(PAYLOAD, SEED);
        let signer_cert = SignedPublicKey::from(corpus::AUX_V6_KEY.clone())
            .to_armored_string(crate::utils::armor_opts())
            .unwrap();
        for (key, want_version) in [(&*keys::V4_SIGN, 1), (&*keys::V6_SIGN, 2)] {
            for sign in [false, true] {
                let case = format!("sq v{want_version} sign={sign}");
                let signer = sign.then_some((corpus::AUX_V6_SECRET, corpus::CORPUS_PASSPHRASE));
                let data = sq.encrypt(&key.public_armored, &payload, signer);
                let nodes = crate::inspect::inspect(&data).blocks[0].packets.clone();
                assert_eq!(
                    nodes.iter().find(|n| n.tag == 18).and_then(|n| n.version),
                    Some(want_version),
                    "{case}"
                );

                let dir = tempfile::tempdir().unwrap();
                let cert = sign.then_some(signer_cert.as_str());
                let res = decrypt_file_as(
                    dir.path(),
                    &data,
                    &key.secret_armored,
                    cert,
                    plain_stage_writer,
                )
                .unwrap_or_else(|e| panic!("{case}: {e:?}"));
                assert_eq!(
                    std::fs::read(dir.path().join("out.bin")).unwrap(),
                    payload,
                    "{case}"
                );
                assert_eq!(res.is_verified, sign, "{case}");

                let dir = tempfile::tempdir().unwrap();
                assert!(
                    decrypt_file_as(
                        dir.path(),
                        &seipd::flipped(&data, -1),
                        &key.secret_armored,
                        cert,
                        plain_stage_writer
                    )
                    .is_err(),
                    "{case}, tampered"
                );
                assert_nothing_committed(dir.path(), None);
            }
        }
    }
}

#[cfg(test)]
mod large_file_tests {
    //! The large-file tier: real SEIPD v1 and v2 files below, at and above the
    //! former 1 GiB `CheckFirst` limit, multi-GB files, and GnuPG output above
    //! the 4 GiB in-memory decompression cap. Every test checks byte-for-byte
    //! recovery, complete integrity checking and bounded peak memory, and that
    //! a failure commits nothing.
    //!
    //! `#[ignore]`d so the regular suite stays fast; run them, each in its own
    //! process so the peak-RSS measurement is clean, with
    //! `scripts/run_tests.sh --large-file-only`. Inputs are generated at run
    //! time (nothing large is committed). Knobs:
    //!   GF_LARGE_FILE_DIR  where to put the files (default: the temp dir)
    //!   GF_LARGE_FILE_GIB  size of the multi-gigabyte case (default 5)

    use super::*;
    use crate::testutil::{cb, interop, keys, memory::PeakRss, seipd};

    const GIB: u64 = 1 << 30;
    const MIB: u64 = 1 << 20;
    const SEED: u64 = 0x1A26_E000;
    /// Peak RSS growth allowed while handling a multi-GB file. CheckFirst
    /// would need the whole payload; streaming needs a few buffers.
    const MEMORY_LIMIT: u64 = 256 * MIB;

    fn workdir() -> tempfile::TempDir {
        match std::env::var_os("GF_LARGE_FILE_DIR") {
            Some(dir) => tempfile::tempdir_in(dir),
            None => tempfile::tempdir(),
        }
        .expect("large-file work dir")
    }

    fn names(dir: &Path) -> Vec<String> {
        let mut names: Vec<String> = std::fs::read_dir(dir)
            .unwrap()
            .map(|e| e.unwrap().file_name().to_string_lossy().into_owned())
            .collect();
        names.sort();
        names
    }

    /// Decrypt `input` into `output` through the public file API, measuring
    /// peak memory, with `signer_cert` answering signature lookups.
    fn decrypt_measured(
        input: &Path,
        output: &Path,
        key: &keys::Fixture,
        signer_cert: Option<&str>,
    ) -> Result<DecryptAndVerifyStreamResultInternal, GfrStatus> {
        cb::set_seckey_answer(&key.secret_armored);
        if let Some(cert) = signer_cert {
            cb::set_pubkey_answer(cert);
        }
        let size = std::fs::metadata(input).unwrap().len();
        let peak = PeakRss::start();
        let res = decrypt_and_verify_file_internal(
            0,
            input.to_str().unwrap(),
            output.to_str().unwrap(),
            false,
            Some(cb::seckey_fetch),
            None,
            Some(if signer_cert.is_some() {
                cb::pubkey_fetch
            } else {
                cb::pubkey_none
            }),
            std::ptr::null_mut(),
        );
        peak.assert_below(MEMORY_LIMIT, size);
        res
    }

    /// Encrypt `len` bytes to `key` with our engine, decrypt through the file
    /// path, and check the result; then break the final MDC/tag byte and check
    /// that nothing is committed.
    fn round_trip(key: &keys::Fixture, len: u64, want_version: u32, signed: bool) {
        let dir = workdir();
        let input = dir.path().join("in.pgp");
        let output = dir.path().join("out.bin");
        let signer = signed.then_some(key);

        let peak = PeakRss::start();
        seipd::encrypt_payload_to_file(&input, key, len, SEED, signer);
        peak.assert_below(MEMORY_LIMIT, len);
        assert_eq!(seipd::seipd_version_of_file(&input), want_version);

        let cert = signed.then_some(key.public_armored.as_str());
        let res = decrypt_measured(&input, &output, key, cert).expect("decrypt the large file");
        assert_eq!(res.is_verified, signed);
        seipd::assert_file_is_payload(&output, len, SEED);
        std::fs::remove_file(&output).unwrap();

        seipd::flip_in_file(&input, 1);
        let res = decrypt_measured(&input, &output, key, cert);
        assert!(res.is_err(), "a tampered large file must fail");
        if want_version == 1 {
            assert_eq!(res.err(), Some(GfrStatus::ErrorDataCorrupted));
        }
        assert_eq!(
            names(dir.path()),
            vec!["in.pgp"],
            "nothing committed, no stage left"
        );
    }

    #[test]
    #[ignore = "large-file tier: run with scripts/run_tests.sh --large-file-only"]
    fn large_file_seipd_v1_below_the_former_limit() {
        round_trip(&keys::V4_SIGN, GIB - MIB, 1, false);
    }

    #[test]
    #[ignore = "large-file tier: run with scripts/run_tests.sh --large-file-only"]
    fn large_file_seipd_v1_at_the_former_limit() {
        round_trip(&keys::V4_SIGN, GIB, 1, false);
    }

    #[test]
    #[ignore = "large-file tier: run with scripts/run_tests.sh --large-file-only"]
    fn large_file_seipd_v1_above_the_former_limit() {
        round_trip(&keys::V4_SIGN, GIB + MIB, 1, false);
    }

    #[test]
    #[ignore = "large-file tier: run with scripts/run_tests.sh --large-file-only"]
    fn large_file_seipd_v1_signed_above_the_former_limit() {
        round_trip(&keys::V4_SIGN, GIB + MIB, 1, true);
    }

    #[test]
    #[ignore = "large-file tier: run with scripts/run_tests.sh --large-file-only"]
    fn large_file_seipd_v2_above_the_former_limit() {
        round_trip(&keys::V6_SIGN, GIB + MIB, 2, false);
    }

    /// The original report: a multi-GB SEIPD v1 file (legacy rPGP format) for
    /// a post-quantum key.
    #[test]
    #[ignore = "large-file tier: run with scripts/run_tests.sh --large-file-only"]
    fn large_file_legacy_seipd_v1_multi_gigabyte_for_a_pq_key() {
        let gib: f64 = std::env::var("GF_LARGE_FILE_GIB")
            .ok()
            .and_then(|v| v.parse().ok())
            .unwrap_or(5.0);
        let len = (gib * GIB as f64) as u64;
        assert!(
            len > GIB,
            "the multi-gigabyte case must exceed the former limit"
        );
        let key = &*keys::V6_PQ;
        let dir = workdir();
        let input = dir.path().join("in.pgp");
        let output = dir.path().join("out.bin");

        let file = std::io::BufWriter::new(File::create(&input).unwrap());
        seipd::write_legacy_v1(key, seipd::Payload::new(len, SEED), file);
        assert_eq!(seipd::seipd_version_of_file(&input), 1);

        decrypt_measured(&input, &output, key, None).expect("decrypt the multi-GB file");
        seipd::assert_file_is_payload(&output, len, SEED);
        std::fs::remove_file(&output).unwrap();

        seipd::flip_in_file(&input, 10);
        assert_eq!(
            decrypt_measured(&input, &output, key, None).err(),
            Some(GfrStatus::ErrorDataCorrupted)
        );
        assert_eq!(names(dir.path()), vec!["in.pgp"]);
    }

    /// GnuPG compresses by default; a compressed file over 4 GiB used to hit
    /// the in-memory decompression cap even on the file path.
    #[test]
    #[ignore = "large-file tier: run with scripts/run_tests.sh --large-file-only"]
    fn large_file_gnupg_compressed_seipd_v1_above_4_gib() {
        let Some(gnupg) = interop::Gnupg::new() else {
            return;
        };
        let key = &*keys::V4_NISTP256;
        gnupg.import(&key.public_armored);
        let len = 4 * GIB + 256 * MIB;
        let dir = workdir();
        let input = dir.path().join("in.pgp");
        let output = dir.path().join("out.bin");
        gnupg.encrypt_stream(
            &key.primary_fpr,
            seipd::Payload::new(len, SEED),
            &input,
            "zlib",
            true,
        );
        assert_eq!(seipd::seipd_version_of_file(&input), 1);

        let res = decrypt_measured(&input, &output, key, Some(&gnupg.cert)).expect("decrypt");
        assert!(res.is_verified, "GnuPG's signature verifies");
        seipd::assert_file_is_payload(&output, len, SEED);
        std::fs::remove_file(&output).unwrap();

        seipd::flip_in_file(&input, 1);
        assert_eq!(
            decrypt_measured(&input, &output, key, Some(&gnupg.cert)).err(),
            Some(GfrStatus::ErrorDataCorrupted)
        );
        assert_eq!(names(dir.path()), vec!["in.pgp"]);
    }

    #[test]
    #[ignore = "large-file tier: run with scripts/run_tests.sh --large-file-only"]
    fn large_file_archive_above_the_former_limit() {
        let dir = workdir();
        let src = dir.path().join("src");
        std::fs::create_dir(&src).unwrap();
        let len = GIB + MIB;
        std::io::copy(
            &mut seipd::Payload::new(len, SEED),
            &mut std::io::BufWriter::new(File::create(src.join("big.bin")).unwrap()),
        )
        .unwrap();
        let enc = dir.path().join("src.tar.gpg");
        crate::crypto::encrypt_directory_internal(
            0,
            src.to_str().unwrap(),
            enc.to_str().unwrap(),
            &[&keys::V4_SIGN.public_armored],
            false,
        )
        .expect("encrypt directory");
        std::fs::remove_dir_all(&src).unwrap();
        assert_eq!(seipd::seipd_version_of_file(&enc), 1);

        let out = dir.path().join("out");
        cb::set_seckey_answer(&keys::V4_SIGN.secret_armored);
        let peak = PeakRss::start();
        decrypt_and_verify_archive_internal(
            0,
            enc.to_str().unwrap(),
            out.to_str().unwrap(),
            false,
            Some(cb::seckey_fetch),
            None,
            None,
            std::ptr::null_mut(),
        )
        .expect("decrypt archive");
        peak.assert_below(MEMORY_LIMIT, len);
        seipd::assert_file_is_payload(&out.join("big.bin"), len, SEED);
        std::fs::remove_dir_all(&out).unwrap();

        seipd::flip_in_file(&enc, 1);
        assert_eq!(
            decrypt_and_verify_archive_internal(
                0,
                enc.to_str().unwrap(),
                out.to_str().unwrap(),
                false,
                Some(cb::seckey_fetch),
                None,
                None,
                std::ptr::null_mut(),
            )
            .err(),
            Some(GfrStatus::ErrorDataCorrupted)
        );
        assert_eq!(
            names(dir.path()),
            vec!["src.tar.gpg"],
            "nothing extracted, no stage left"
        );
    }
}

#[cfg(test)]
mod esk_and_integrity_tests {
    //! Messages carrying both a password (SKESK) and public-key (PKESK)
    //! session-key packets, and how a failed authentication is reported.
    //!
    //! Mixed messages follow the product decision in key-validity.md: a
    //! matching private key is tried first, the message password second; a
    //! cancelled prompt aborts; when both fail one uniform error is reported.

    use super::*;
    use crate::testutil::{cb, keys, packets, seipd};

    const PAYLOAD: &[u8] = b"mixed session keys\n";

    fn decrypt(
        data: &[u8],
        seckey: Option<&str>,
        pwd: GfrPasswordFetchCb,
    ) -> Result<DecryptResultInternal, GfrStatus> {
        match seckey {
            Some(block) => cb::set_seckey_answer(block),
            None => cb::clear_seckey_answer(),
        }
        decrypt_internal(
            0,
            data,
            Some(cb::seckey_fetch),
            Some(pwd),
            std::ptr::null_mut(),
        )
    }

    fn failure(result: Result<DecryptResultInternal, GfrStatus>, what: &str) -> GfrStatus {
        match result {
            Ok(_) => panic!("{what}: must not decrypt"),
            Err(status) => status,
        }
    }

    // -- mixed SKESK + PKESK ------------------------------------------------

    #[test]
    fn a_mixed_message_opens_with_the_private_key_without_asking_for_the_password() {
        for (fixture, v2) in [(&*keys::V4_SIGN, false), (&*keys::V6_SIGN, true)] {
            let msg = seipd::mixed_message(fixture, "message password", PAYLOAD, v2);
            cb::reset_prompt_count();
            let out = decrypt(&msg, Some(&fixture.secret_armored), cb::pwd_failed)
                .unwrap_or_else(|e| panic!("v2={v2}: {e:?}"));
            assert_eq!(out.data, PAYLOAD);
            assert_eq!(
                cb::prompt_count(),
                0,
                "v2={v2}: no prompt for an unlocked key"
            );
        }
    }

    #[test]
    fn a_mixed_message_falls_back_to_the_password_without_a_matching_key() {
        for (fixture, v2) in [(&*keys::V4_SIGN, false), (&*keys::V6_SIGN, true)] {
            let msg = seipd::mixed_message(fixture, cb::CORRECT_PASSPHRASE, PAYLOAD, v2);
            let out =
                decrypt(&msg, None, cb::pwd_correct).unwrap_or_else(|e| panic!("v2={v2}: {e:?}"));
            assert_eq!(out.data, PAYLOAD);
        }
    }

    #[test]
    fn a_mixed_message_falls_back_to_the_password_when_the_key_does_not_fit() {
        // The fetch callback hands back a key that does not decrypt the
        // session key (it answers for any recipient).
        let msg = seipd::mixed_message(&keys::V4_SIGN, cb::CORRECT_PASSPHRASE, PAYLOAD, false);
        let out = decrypt(
            &msg,
            Some(&keys::V4_NISTP256.secret_armored),
            cb::pwd_correct,
        )
        .expect("the password still opens it");
        assert_eq!(out.data, PAYLOAD);
    }

    #[test]
    fn a_cancelled_password_prompt_on_a_mixed_message_aborts() {
        let msg = seipd::mixed_message(&keys::V4_SIGN, "message password", PAYLOAD, false);
        assert_eq!(
            decrypt(&msg, None, cb::pwd_cancelled).err(),
            Some(GfrStatus::ErrorCanceled)
        );
    }

    #[test]
    fn a_mixed_message_neither_mechanism_opens_reports_one_uniform_error() {
        let msg = seipd::mixed_message(&keys::V4_SIGN, "message password", PAYLOAD, false);
        let err = failure(
            decrypt(&msg, Some(&keys::V4_NISTP256.secret_armored), cb::pwd_wrong),
            "nothing opens it",
        );
        assert_eq!(err, GfrStatus::ErrorDecryptionFailed);
        assert!(
            crate::err::last_error().contains("no usable private key or password"),
            "{}",
            crate::err::last_error()
        );
    }

    #[test]
    fn the_container_cipher_and_mode_are_reported() {
        // What the GUI shows next to "Symmetric algorithm" for GnuPG.
        let v2 = seipd::message(&keys::V6_SIGN.public_armored, PAYLOAD, None);
        let out = decrypt(&v2, Some(&keys::V6_SIGN.secret_armored), cb::pwd_failed)
            .unwrap_or_else(|e| panic!("{e:?}"));
        assert_eq!(out.sym_algo, "AES256.OCB");

        let v1 = seipd::message(&keys::V4_SIGN.public_armored, PAYLOAD, None);
        let out = decrypt(&v1, Some(&keys::V4_SIGN.secret_armored), cb::pwd_failed)
            .unwrap_or_else(|e| panic!("{e:?}"));
        assert_eq!(out.sym_algo, "AES256.CFB");
    }

    // -- integrity failures (RFC 9580 §13.7) ---------------------------------

    /// A SEIPD v2 (AES-256/OCB, 4 KiB chunks) message of several chunks.
    fn v2_message() -> (Vec<u8>, Vec<u8>) {
        let payload = seipd::bytes(5 * 4096 + 17, 7);
        let msg = seipd::message(&keys::V6_SIGN.public_armored, &payload, None);
        (msg, payload)
    }

    fn assert_integrity_failure(data: &[u8], fixture: &keys::Fixture, what: &str) {
        let err = failure(
            decrypt(data, Some(&fixture.secret_armored), cb::pwd_failed),
            what,
        );
        assert_eq!(err, GfrStatus::ErrorDataCorrupted, "{what}");
        assert!(
            crate::err::last_error().contains("integrity check failed"),
            "{what}: {}",
            crate::err::last_error()
        );
    }

    #[test]
    fn an_aead_tag_failure_in_any_chunk_is_an_integrity_failure() {
        // "If the authentication tag fails to verify, the implementation MUST
        // NOT attempt to parse nor release decrypted data ... and it MUST halt
        // with an error." The failure is about the message, never a password.
        let (msg, _) = v2_message();
        let n = msg.len() as isize;
        for (what, at) in [
            ("first chunk", n - (5 * 4096 + 17) - 16 * 7 + 10),
            ("middle chunk", n / 2),
            ("last data chunk", n - 40),
            ("final tag", n - 1),
        ] {
            assert_integrity_failure(&seipd::flipped(&msg, at), &keys::V6_SIGN, what);
        }
    }

    #[test]
    fn a_truncated_aead_message_is_an_integrity_failure() {
        let (msg, _) = v2_message();
        for cut in [msg.len() - 1, msg.len() - 16, msg.len() / 2] {
            assert_integrity_failure(&msg[..cut], &keys::V6_SIGN, &format!("cut at {cut}"));
        }
    }

    #[test]
    fn an_unknown_critical_packet_in_a_decrypted_message_is_invalid_data() {
        // RFC 9580 §4.3: an unknown critical packet (Type ID 0-39) means "it
        // MUST reject the whole packet sequence". Not an integrity failure
        // (§13.7 does not list it): an invalid message.
        let mut inner = packets::unknown_packet(30, b"unknown");
        inner.extend(packets::literal(b'b', b"", 0, PAYLOAD));
        let msg = seipd::v1_with_inner(&keys::V4_SIGN, &inner);
        let err = failure(
            decrypt(&msg, Some(&keys::V4_SIGN.secret_armored), cb::pwd_failed),
            "an unknown critical packet",
        );
        assert_eq!(err, GfrStatus::ErrorInvalidData);
    }

    #[test]
    fn an_unknown_non_critical_packet_in_a_decrypted_message_is_ignored() {
        // RFC 9580 §4.3: "an unknown non-critical packet MUST be ignored".
        let mut inner = packets::unknown_packet(60, b"private use");
        inner.extend(packets::literal(b'b', b"", 0, PAYLOAD));
        let msg = seipd::v1_with_inner(&keys::V4_SIGN, &inner);
        let out = decrypt(&msg, Some(&keys::V4_SIGN.secret_armored), cb::pwd_failed)
            .unwrap_or_else(|e| panic!("{e:?}"));
        assert_eq!(out.data, PAYLOAD);
    }

    #[test]
    fn a_deeply_nested_compressed_message_is_refused_inside_the_container() {
        // §13.14 (SHOULD limit the layers): 32 layers is twice the cap. The
        // layers sit inside the SEIPD, where the cap actually applies.
        let inner = packets::nested_compressed(32, &packets::literal(b'b', b"", 0, PAYLOAD));
        let msg = seipd::v1_with_inner(&keys::V4_SIGN, &inner);
        let err = failure(
            decrypt(&msg, Some(&keys::V4_SIGN.secret_armored), cb::pwd_failed),
            "32 compression layers",
        );
        assert_eq!(err, GfrStatus::ErrorInvalidData);
        assert!(crate::err::last_error().contains("too many nested compression layers"));
    }

    #[test]
    fn a_decompression_failure_inside_an_encrypted_message_is_reported_as_integrity_suspect() {
        // §13.7 lists "any Compressed Data packet where there is a
        // decompression failure" among the malleability indicators that MUST
        // produce a clear integrity-suspect error.
        let inner = packets::packet(8, &[2, 0xde, 0xad, 0xbe, 0xef, 0x00, 0x11]);
        let msg = seipd::v1_with_inner(&keys::V4_SIGN, &inner);
        let err = failure(
            decrypt(&msg, Some(&keys::V4_SIGN.secret_armored), cb::pwd_failed),
            "a malformed compressed packet",
        );
        assert_eq!(err, GfrStatus::ErrorDataCorrupted);
        assert!(
            crate::err::last_error().contains("integrity"),
            "{}",
            crate::err::last_error()
        );
    }
}

#[cfg(test)]
mod mixed_esk_interop_tests {
    //! Messages carrying both a PKESK and an SKESK, produced by GnuPG and
    //! Sequoia, opened by GF either way (RFC 9580 §3.7.2.2).

    use super::*;
    use crate::testutil::{cb, interop, keys};

    const PAYLOAD: &[u8] = b"mixed from another implementation\n";

    fn decrypt(data: &[u8], seckey: Option<&str>, pwd: GfrPasswordFetchCb) -> Vec<u8> {
        match seckey {
            Some(block) => cb::set_seckey_answer(block),
            None => cb::clear_seckey_answer(),
        }
        match decrypt_internal(
            0,
            data,
            Some(cb::seckey_fetch),
            Some(pwd),
            std::ptr::null_mut(),
        ) {
            Ok(out) => out.data,
            Err(e) => panic!("decrypt: {e:?} ({})", crate::err::last_error()),
        }
    }

    #[test]
    fn we_open_a_sequoia_mixed_message_with_either_mechanism() {
        let Some(sq) = interop::Sq::new() else {
            return;
        };
        for fixture in [&*keys::V4_SIGN, &*keys::V6_SIGN] {
            let msg = sq.encrypt_mixed(&fixture.public_armored, cb::CORRECT_PASSPHRASE, PAYLOAD);
            assert_eq!(
                decrypt(&msg, Some(&fixture.secret_armored), cb::pwd_failed),
                PAYLOAD
            );
            assert_eq!(decrypt(&msg, None, cb::pwd_correct), PAYLOAD);
        }
    }

    #[test]
    fn we_open_a_gnupg_mixed_message_with_either_mechanism() {
        let Some(gpg) = interop::Gnupg::new() else {
            return;
        };
        let fixture = &keys::V4_NISTP256;
        gpg.import(&fixture.public_armored);
        let msg = gpg.encrypt_mixed(&fixture.primary_fpr, cb::CORRECT_PASSPHRASE, PAYLOAD);
        assert_eq!(
            decrypt(&msg, Some(&fixture.secret_armored), cb::pwd_failed),
            PAYLOAD
        );
        assert_eq!(decrypt(&msg, None, cb::pwd_correct), PAYLOAD);
    }
}
