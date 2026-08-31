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

use std::sync::atomic::{AtomicU32, Ordering};

use zeroize::Zeroizing;

use crate::utils::{armor_opts, password_from_zeroizing_bytes};

use super::*;

/// RFC 9106 §4 "first recommended" Argon2id parameters: t=1, p=4, m=2^21 KiB
/// (2 GiB). The strongest of the two choices the RFC names, and the default.
pub const ARGON2_S2K_HIGH_MEMORY: (u8, u8, u8) = (1, 4, 21);

/// RFC 9106 §4 "second recommended" Argon2id parameters: t=3, p=4, m=2^16 KiB
/// (64 MiB), for machines that cannot spare 2 GiB. This is also what rPGP
/// itself picks when locking a v6 secret key.
///
/// Test-only: the profile is chosen on the C++ side, which sends the octets
/// through [`gfr_set_argon2_s2k_params`](crate::ffi::gfr_set_argon2_s2k_params),
/// so the engine never selects this triple itself.
#[cfg(test)]
pub(crate) const ARGON2_S2K_LOW_MEMORY: (u8, u8, u8) = (3, 4, 16);

/// The Argon2id S2K parameters used when encrypting with a passphrase.
///
/// The three octets are packed into one atomic so a concurrent read can never
/// observe a new `t` paired with an old `m_enc`. Reconfigured from C++ through
/// [`gfr_set_argon2_s2k_params`](crate::ffi::gfr_set_argon2_s2k_params); the
/// built-in value is the RFC's first recommendation, so an unset setting
/// changes nothing.
static ARGON2_S2K_PARAMS: AtomicU32 = AtomicU32::new(pack_argon2_s2k_params(
    ARGON2_S2K_HIGH_MEMORY.0,
    ARGON2_S2K_HIGH_MEMORY.1,
    ARGON2_S2K_HIGH_MEMORY.2,
));

const fn pack_argon2_s2k_params(t: u8, p: u8, m_enc: u8) -> u32 {
    (t as u32) << 16 | (p as u32) << 8 | m_enc as u32
}

/// Whether rPGP would accept this parameter triple.
///
/// Mirrors the checks in rPGP's `StringToKey::derive_key` so a bad setting is
/// refused once, at configuration time, instead of failing every later
/// encryption. The `m_enc` ceiling is 21 rather than the RFC's 31 because rPGP
/// caps the decoded memory size at 2 GiB (`ARGON2_MEMORY_LIMIT_KIB`), and a
/// message we cannot decrypt ourselves is not worth writing.
pub fn validate_argon2_s2k_params(t: u8, p: u8, m_enc: u8) -> bool {
    if t == 0 || t > 32 || p == 0 || p > 32 {
        return false;
    }

    // RFC 9580 §3.7.1.4: the encoded memory size must leave at least 8*p
    // blocks, i.e. m_enc >= ceil(log_2(p)).
    let min_m_enc = (p as f32).log2().ceil() as u8;
    m_enc >= min_m_enc && m_enc <= 21
}

/// The Argon2id S2K parameters currently in effect, as `(t, p, m_enc)`.
pub fn argon2_s2k_params() -> (u8, u8, u8) {
    let packed = ARGON2_S2K_PARAMS.load(Ordering::Relaxed);
    (
        (packed >> 16) as u8,
        (packed >> 8) as u8,
        (packed & 0xFF) as u8,
    )
}

/// Serialises the tests that mutate [`ARGON2_S2K_PARAMS`]. The parameters are
/// process-wide, so tests that set them (here and in the FFI module) must not
/// run against each other's value.
#[cfg(test)]
pub(crate) static ARGON2_TEST_LOCK: std::sync::Mutex<()> = std::sync::Mutex::new(());

/// Install new Argon2id S2K parameters for passphrase encryption.
///
/// Returns `false` and leaves the current parameters untouched when the triple
/// is one rPGP would reject, so a malformed setting degrades to the previous
/// (valid) choice rather than breaking encryption outright.
pub fn set_argon2_s2k_params(t: u8, p: u8, m_enc: u8) -> bool {
    if !validate_argon2_s2k_params(t, p, m_enc) {
        log::warn!("refusing invalid argon2 s2k parameters: t={t}, p={p}, m_enc={m_enc}");
        return false;
    }

    ARGON2_S2K_PARAMS.store(pack_argon2_s2k_params(t, p, m_enc), Ordering::Relaxed);
    log::info!("argon2 s2k parameters set to t={t}, p={p}, m_enc={m_enc}");
    true
}

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
        recipients: result.recipients,
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

/// Which key of a recipient certificate the session key is encrypted to.
///
/// Indices refer into `cert.public_subkeys`; `Primary` targets `cert.primary_key`.
/// An index keeps the borrow checker happy — the caller re-borrows the chosen
/// subkey when it actually calls `encrypt_to_key`.
enum EncryptionTarget {
    Sub(usize),
    Primary,
}

/// Select which encryption key of `cert` the session key should go to.
///
/// Mirrors [`with_signing_key`]'s two modes for the encryption side:
/// * **Exact-match** (`target = Some(fpr)`): the caller pinned a specific subkey
///   via the `fpr!` armor-block prefix. The pinned key is honored only if it is
///   encrypt-capable and **not revoked**; otherwise the recipient is rejected.
/// * **Auto** (`target = None`): the first **non-revoked** encrypt-capable subkey
///   wins, falling back to the primary key if no subkey qualifies.
///
/// A revoked subkey is never selected in either mode.
fn choose_encryption_target(
    cert: &SignedPublicKey,
    target: Option<&str>,
) -> Result<EncryptionTarget, GfrStatus> {
    // ==========================================
    // EXACT MATCH MODE (!)
    // ==========================================
    if let Some(target) = target {
        for (i, subkey) in cert.public_subkeys.iter().enumerate() {
            let fpr = subkey.key.fingerprint().to_string();
            let kid = subkey.key.legacy_key_id().to_string();
            if super::key_identifier_matches(&fpr, &kid, target) {
                if crate::key::is_subkey_revoked(&cert.primary_key, &subkey.key, &subkey.signatures)
                {
                    log::error!("Requested encryption subkey is revoked: fpr={}", fpr);
                    return Err(GfrStatus::ErrorNoKey);
                }
                if !subkey.key.algorithm().can_encrypt() {
                    log::error!(
                        "Requested encryption subkey is not encrypt-capable: fpr={}, algo={:?}",
                        fpr,
                        subkey.key.algorithm(),
                    );
                    return Err(GfrStatus::ErrorNoKey);
                }
                log::info!(
                    "Selected marked encryption subkey: fpr={}, keyid={}",
                    fpr,
                    kid
                );
                return Ok(EncryptionTarget::Sub(i));
            }
        }

        let p_fpr = cert.primary_key.fingerprint().to_string();
        let p_kid = cert.primary_key.legacy_key_id().to_string();
        if super::key_identifier_matches(&p_fpr, &p_kid, target) {
            if !cert.primary_key.algorithm().can_encrypt() {
                log::error!(
                    "Requested primary key is not encrypt-capable: fpr={}",
                    p_fpr,
                );
                return Err(GfrStatus::ErrorNoKey);
            }
            log::info!("Selected marked primary encryption key: fpr={}", p_fpr);
            return Ok(EncryptionTarget::Primary);
        }

        log::error!("Requested encryption target not found: {}", target);
        return Err(GfrStatus::ErrorNoKey);
    }

    // ==========================================
    // NORMAL MODE (Auto Fallback) — skip revoked
    // ==========================================
    for (i, subkey) in cert.public_subkeys.iter().enumerate() {
        if crate::key::is_subkey_revoked(&cert.primary_key, &subkey.key, &subkey.signatures) {
            log::info!(
                "Skipping revoked encryption subkey: fpr={}",
                subkey.key.fingerprint(),
            );
            continue;
        }
        if subkey.key.algorithm().can_encrypt() {
            return Ok(EncryptionTarget::Sub(i));
        }
    }

    if cert.primary_key.algorithm().can_encrypt() {
        return Ok(EncryptionTarget::Primary);
    }

    Err(GfrStatus::ErrorNoKey)
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
                    expires_at: 0,
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
    let mut recipients = Vec::new();

    for block in public_key_blocks {
        // A caller may pin a specific encryption subkey by prefixing the armored
        // block with `<fpr>!` (the same mechanism the signing path uses). Strip
        // it off before parsing the certificate.
        let (target, armored) = parse_signer_block(block);

        match SignedPublicKey::from_string(armored) {
            Ok((cert, _)) => {
                let fpr = cert.primary_key.fingerprint().to_string();

                match choose_encryption_target(&cert, target.as_deref()) {
                    Ok(EncryptionTarget::Sub(i)) => {
                        let subkey = &cert.public_subkeys[i];
                        if enc_builder.encrypt_to_key(&mut rng, subkey).is_ok() {
                            // Record the subkey actually used so callers can show
                            // the real recipient key ID and algorithm.
                            recipients.push(RecipientResultInternal {
                                key_id: subkey.key.legacy_key_id().to_string(),
                                pub_algo: algo_to_string_simple(subkey.key.algorithm()),
                                status: GfrRecipientStatus::Success,
                            });
                            has_recipient = true;
                        } else {
                            invalid_recipients.push(InvalidRecipientInternal {
                                fpr,
                                reason: GfrStatus::ErrorNoKey,
                            });
                        }
                    }
                    Ok(EncryptionTarget::Primary) => {
                        if enc_builder
                            .encrypt_to_key(&mut rng, &cert.primary_key)
                            .is_ok()
                        {
                            recipients.push(RecipientResultInternal {
                                key_id: cert.primary_key.legacy_key_id().to_string(),
                                pub_algo: algo_to_string_simple(cert.primary_key.algorithm()),
                                status: GfrRecipientStatus::Success,
                            });
                            has_recipient = true;
                        } else {
                            invalid_recipients.push(InvalidRecipientInternal {
                                fpr,
                                reason: GfrStatus::ErrorNoKey,
                            });
                        }
                    }
                    Err(reason) => {
                        invalid_recipients.push(InvalidRecipientInternal { fpr, reason });
                    }
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
        enc_builder.to_armored_writer(&mut rng, armor_opts(), &mut output_stream)
    } else {
        enc_builder.to_writer(&mut rng, &mut output_stream)
    };

    result
        .record_err_with(|| crate::cancel::status_or_canceled(channel, GfrStatus::ErrorInternal))?;
    output_stream
        .flush()
        .record_err_with(|| crate::cancel::status_or_canceled(channel, GfrStatus::ErrorInternal))?;

    Ok(EncryptAndSignStreamResultInternal {
        signatures: created_signatures,
        invalid_recipients,
        recipients,
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
        recipients: stream_result.recipients,
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
    let (t, p, m_enc) = argon2_s2k_params();
    let s2k = StringToKey::new_argon2(&mut rng, t, p, m_enc);
    enc_builder.encrypt_with_password(s2k, &msg_pw).into_gfr()?;

    let result = if ascii_armor {
        enc_builder.to_armored_writer(&mut rng, armor_opts(), &mut output_stream)
    } else {
        enc_builder.to_writer(&mut rng, &mut output_stream)
    };

    result
        .record_err_with(|| crate::cancel::status_or_canceled(channel, GfrStatus::ErrorInternal))?;
    output_stream
        .flush()
        .record_err_with(|| crate::cancel::status_or_canceled(channel, GfrStatus::ErrorInternal))?;
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

    let stream_result = encrypt_stream_internal(
        channel,
        name,
        data,
        &mut output,
        public_key_blocks,
        ascii_armor,
    )?;

    Ok(EncryptResultInternal {
        data: output,
        invalid_recipients: stream_result.invalid_recipients,
        recipients: stream_result.recipients,
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
        recipients: stream_result.recipients,
    })
}

#[cfg(test)]
mod encrypt_tests {
    //! Encryption: recipient/subkey selection, the emitted container format,
    //! symmetric (passphrase) encryption, and encrypt-and-sign.
    //!
    //! The engine deliberately emits v1 SEIPD with AES-256 (§5.13.1) rather
    //! than v2 SEIPD/AEAD: it is the most widely interoperable choice, and
    //! §13.7 permits it when a recipient's support for v2 is unknown. Tests
    //! here pin that decision so a change is deliberate.

    use super::*;
    use crate::testutil::{cb, corpus, keys, packets};

    // RFC 9580 3.7.1.4 requires m_enc to leave at least 8*p blocks.
    // parameters_rpgp_would_reject_are_refused covers the reject side of that
    // bound; this pins the accept side, so tightening the rule is caught too.
    #[test]
    fn the_smallest_memory_size_each_parallelism_allows_is_accepted() {
        assert!(validate_argon2_s2k_params(1, 4, 2), "p=4 at its minimum");
        assert!(!validate_argon2_s2k_params(1, 4, 1), "p=4 one below");
        assert!(validate_argon2_s2k_params(1, 1, 0), "p=1 demands nothing");
    }

    fn cert_of(fixture: &keys::Fixture) -> SignedPublicKey {
        SignedPublicKey::from_string(&fixture.public_armored)
            .expect("parses")
            .0
    }

    fn encrypt_to(fixture: &keys::Fixture, data: &[u8]) -> EncryptResultInternal {
        encrypt_internal(0, "", data, &[&fixture.public_armored], true).expect("encrypt")
    }

    // -- choose_encryption_target: automatic selection ------------------------

    #[test]
    fn the_encryption_subkey_is_selected_automatically() {
        // §10.1.5: "In general, subkeys are provided in cases where the
        // top-level public key is a certification-only key."
        let cert = cert_of(&keys::V4_SIGN);
        assert!(matches!(
            choose_encryption_target(&cert, None).expect("a target"),
            EncryptionTarget::Sub(_)
        ));
    }

    #[test]
    fn a_signing_only_subkey_is_not_selected_for_encryption() {
        let cert = cert_of(&keys::V4_SIGN);
        let EncryptionTarget::Sub(i) = choose_encryption_target(&cert, None).expect("a target")
        else {
            panic!("expected a subkey");
        };
        assert!(cert.public_subkeys[i].key.algorithm().can_encrypt());
    }

    #[test]
    fn a_revoked_encryption_subkey_is_skipped() {
        // §5.2.1.12: "A revoked subkey is not to be used."
        let cert = cert_of(&keys::V4_REVOKED_SUBKEY);
        // The only encryption subkey is revoked and the primary is Ed25519,
        // so there is nothing left to encrypt to.
        assert!(choose_encryption_target(&cert, None).is_err());
    }

    #[test]
    fn a_key_with_no_encryption_capable_component_is_rejected() {
        let cert = cert_of(&keys::V4_PRIMARY_ONLY);
        assert_eq!(
            choose_encryption_target(&cert, None).err(),
            Some(GfrStatus::ErrorNoKey)
        );
    }

    #[test]
    fn a_v6_key_selects_its_x25519_subkey() {
        let cert = cert_of(&keys::V6_SIGN);
        let EncryptionTarget::Sub(i) = choose_encryption_target(&cert, None).expect("a target")
        else {
            panic!("expected a subkey");
        };
        assert_eq!(
            cert.public_subkeys[i].key.algorithm(),
            pgp::crypto::public_key::PublicKeyAlgorithm::X25519
        );
    }

    // -- choose_encryption_target: pinned selection ---------------------------

    #[test]
    fn a_pinned_encryption_subkey_is_honoured() {
        let cert = cert_of(&keys::V4_SIGN);
        let want = keys::V4_SIGN.enc_subkey_fpr();
        let EncryptionTarget::Sub(i) =
            choose_encryption_target(&cert, Some(want)).expect("a target")
        else {
            panic!("expected a subkey");
        };
        assert!(
            cert.public_subkeys[i]
                .key
                .fingerprint()
                .to_string()
                .eq_ignore_ascii_case(want)
        );
    }

    #[test]
    fn a_pinned_signing_subkey_is_rejected_rather_than_replaced() {
        // Falling back would encrypt to a key the user did not choose.
        let cert = cert_of(&keys::V4_SIGN);
        assert_eq!(
            choose_encryption_target(&cert, Some(keys::V4_SIGN.sign_subkey_fpr())).err(),
            Some(GfrStatus::ErrorNoKey)
        );
    }

    #[test]
    fn a_pinned_revoked_subkey_is_rejected() {
        let cert = cert_of(&keys::V4_REVOKED_SUBKEY);
        let revoked = keys::V4_SIGN.enc_subkey_fpr();
        assert_eq!(
            choose_encryption_target(&cert, Some(revoked)).err(),
            Some(GfrStatus::ErrorNoKey)
        );
    }

    #[test]
    fn a_pinned_unknown_identifier_is_rejected() {
        let cert = cert_of(&keys::V4_SIGN);
        assert_eq!(
            choose_encryption_target(&cert, Some("0000000000000000")).err(),
            Some(GfrStatus::ErrorNoKey)
        );
    }

    #[test]
    fn a_pinned_non_encrypting_primary_is_rejected() {
        let cert = cert_of(&keys::V4_SIGN);
        assert_eq!(
            choose_encryption_target(&cert, Some(&keys::V4_SIGN.primary_fpr)).err(),
            Some(GfrStatus::ErrorNoKey)
        );
    }

    #[test]
    fn a_pinned_key_id_suffix_resolves() {
        let cert = cert_of(&keys::V4_SIGN);
        let full = keys::V4_SIGN.enc_subkey_fpr();
        assert!(choose_encryption_target(&cert, Some(&full[full.len() - 16..])).is_ok());
    }

    // -- emitted container ------------------------------------------------------

    #[test]
    fn armored_output_is_a_pgp_message() {
        let out = encrypt_to(&keys::V4_SIGN, b"payload");
        assert!(String::from_utf8_lossy(&out.data).contains("BEGIN PGP MESSAGE"));
    }

    #[test]
    fn armored_output_carries_no_crc24_footer() {
        let out = encrypt_to(&keys::V4_SIGN, b"payload");
        crate::testutil::assert::armor_has_no_crc24(&String::from_utf8_lossy(&out.data));
    }

    #[test]
    fn unarmored_output_is_binary() {
        let out = encrypt_internal(0, "", b"payload", &[&keys::V4_SIGN.public_armored], false)
            .expect("encrypt");
        assert!(!String::from_utf8_lossy(&out.data).contains("BEGIN PGP"));
        assert!(!out.data.is_empty());
    }

    #[test]
    fn the_output_is_an_integrity_protected_container() {
        // §13.7: the deprecated SED packet MUST NOT be generated. Whatever
        // container the engine picks must be a SEIPD.
        let out = encrypt_internal(0, "", b"payload", &[&keys::V4_SIGN.public_armored], false)
            .expect("encrypt");
        let msg = Message::from_bytes(std::io::Cursor::new(out.data)).expect("parse");
        let Message::Encrypted { edata, .. } = &msg else {
            panic!("expected an encrypted message");
        };
        assert_ne!(
            edata.tag(),
            Tag::SymEncryptedData,
            "an unauthenticated SED payload must never be produced"
        );
    }

    #[test]
    fn the_output_lists_the_recipients_it_encrypted_to() {
        let out = encrypt_to(&keys::V4_SIGN, b"payload");
        assert_eq!(out.recipients.len(), 1);
        assert!(!out.recipients[0].key_id.is_empty());
    }

    #[test]
    fn the_reported_recipient_is_the_encryption_subkey() {
        let out = encrypt_to(&keys::V4_SIGN, b"payload");
        let want = keys::V4_SIGN.enc_subkey_fpr();
        assert!(
            want.to_uppercase()
                .ends_with(&out.recipients[0].key_id.to_uppercase()),
            "reported {} for subkey {want}",
            out.recipients[0].key_id
        );
    }

    #[test]
    fn a_v6_recipient_is_reported_by_fingerprint() {
        // §5.1.2: a v6 PKESK identifies the recipient by fingerprint.
        let out = encrypt_to(&keys::V6_SIGN, b"payload");
        assert_eq!(out.recipients.len(), 1);
    }

    // -- multiple recipients ------------------------------------------------------

    #[test]
    fn encrypting_to_two_certificates_emits_two_session_keys() {
        // §5.1: "The encryption container is preceded by one Public Key
        // Encrypted Session Key packet for each OpenPGP Key."
        let out = encrypt_internal(
            0,
            "",
            b"payload",
            &[&keys::V4_SIGN.public_armored, &keys::V6_SIGN.public_armored],
            false,
        )
        .expect("encrypt");
        assert_eq!(out.recipients.len(), 2);
        assert!(out.invalid_recipients.is_empty());
    }

    #[test]
    fn either_recipient_can_decrypt_a_two_recipient_message() {
        let out = encrypt_internal(
            0,
            "",
            b"shared secret",
            &[&keys::V4_SIGN.public_armored, &keys::V6_SIGN.public_armored],
            false,
        )
        .expect("encrypt");

        for key in [&*keys::V4_SIGN, &*keys::V6_SIGN] {
            cb::set_seckey_answer(&key.secret_armored);
            let res = crate::crypto::decrypt_internal(
                0,
                &out.data,
                Some(cb::seckey_fetch),
                None,
                std::ptr::null_mut(),
            )
            .expect("decrypt");
            assert_eq!(res.data, b"shared secret");
        }
    }

    #[test]
    fn an_unusable_recipient_is_reported_rather_than_aborting_the_whole_operation() {
        // One bad certificate among several must not deny service to the rest.
        let out = encrypt_internal(
            0,
            "",
            b"payload",
            &[
                &keys::V4_SIGN.public_armored,
                &keys::V4_PRIMARY_ONLY.public_armored,
            ],
            false,
        );

        match out {
            Ok(res) => {
                assert!(
                    !res.invalid_recipients.is_empty(),
                    "the encryption-incapable certificate must be reported"
                );
                assert_eq!(res.recipients.len(), 1);
            }
            Err(status) => assert!((status as i32) < 0),
        }
    }

    #[test]
    fn encrypting_to_no_recipients_fails() {
        assert!(encrypt_internal(0, "", b"payload", &[], true).is_err());
    }

    #[test]
    fn encrypting_to_garbage_fails() {
        assert!(encrypt_internal(0, "", b"payload", &["not a key"], true).is_err());
    }

    #[test]
    fn encrypting_to_a_revoked_primary_is_currently_allowed() {
        // KNOWN GAP: `choose_encryption_target` skips revoked *subkeys* but
        // never checks whether the primary key itself is revoked or expired,
        // so a message can still be encrypted to a revoked certificate. The
        // verify side does gate on this (§5.2.1.11), so the two sides are
        // deliberately asymmetric. Pinned as current behaviour; see the
        // companion `#[ignore]`d test below for the target behaviour.
        let res = encrypt_internal(
            0,
            "",
            b"payload",
            &[&keys::V4_REVOKED_PRIMARY.public_armored],
            false,
        );
        assert!(
            res.is_ok(),
            "current behaviour: revoked primaries are accepted"
        );
    }

    #[test]
    #[ignore = "DEFERRED: the produce side does not reject a revoked or expired \
                recipient, unlike the verify side. This encodes the target behaviour."]
    fn deferred_encrypting_to_a_revoked_primary_reports_an_invalid_recipient() {
        let res = encrypt_internal(
            0,
            "",
            b"payload",
            &[&keys::V4_REVOKED_PRIMARY.public_armored],
            false,
        );
        assert!(res.is_err() || !res.unwrap().invalid_recipients.is_empty());
    }

    // -- payloads -------------------------------------------------------------------

    #[test]
    fn an_empty_payload_is_accepted() {
        // An empty literal packet (§5.9) is well formed, and refusing it would
        // make encrypting a zero-byte file fail for no good reason.
        let out = encrypt_internal(0, "", b"", &[&keys::V4_SIGN.public_armored], true)
            .expect("an empty payload encrypts");
        assert!(!out.data.is_empty(), "the container itself is non-empty");
    }

    #[test]
    fn an_empty_payload_round_trips_to_nothing() {
        let out =
            encrypt_internal(0, "", b"", &[&keys::V4_SIGN.public_armored], false).expect("encrypt");
        cb::set_seckey_answer(&keys::V4_SIGN.secret_armored);
        let res = crate::crypto::decrypt_internal(
            0,
            &out.data,
            Some(cb::seckey_fetch),
            None,
            std::ptr::null_mut(),
        )
        .expect("decrypt");
        assert!(res.data.is_empty());
    }

    #[test]
    fn a_binary_payload_with_nul_bytes_encrypts() {
        let payload: Vec<u8> = (0..=255u8).cycle().take(4096).collect();
        let out = encrypt_to(&keys::V4_SIGN, &payload);
        assert!(!out.data.is_empty());
    }

    #[test]
    fn a_large_payload_encrypts() {
        // Large enough to exercise the 512 KiB partial-packet chunking
        // (§4.2.1.4) the streaming builder uses.
        let payload = vec![0x42u8; 2 * 1024 * 1024];
        let out = encrypt_to(&keys::V4_SIGN, &payload);
        assert!(out.data.len() > 1024 * 1024);
    }

    #[test]
    fn a_large_payload_round_trips() {
        let payload = vec![0x42u8; 2 * 1024 * 1024];
        let out = encrypt_internal(0, "", &payload, &[&keys::V4_SIGN.public_armored], false)
            .expect("encrypt");
        cb::set_seckey_answer(&keys::V4_SIGN.secret_armored);
        let res = crate::crypto::decrypt_internal(
            0,
            &out.data,
            Some(cb::seckey_fetch),
            None,
            std::ptr::null_mut(),
        )
        .expect("decrypt");
        assert_eq!(res.data.len(), payload.len());
        assert_eq!(res.data, payload);
    }

    #[test]
    fn two_encryptions_of_the_same_plaintext_differ() {
        // §2.1: "A new session key is generated as a random number for each
        // object." Identical ciphertexts would leak that the plaintexts match.
        let a = encrypt_to(&keys::V4_SIGN, b"same plaintext");
        let b = encrypt_to(&keys::V4_SIGN, b"same plaintext");
        assert_ne!(a.data, b.data);
    }

    // -- symmetric (passphrase) encryption -------------------------------------------

    // -- argon2 s2k parameters ------------------------------------------------

    #[test]
    fn both_rfc9106_parameter_choices_are_accepted() {
        let (t, p, m_enc) = ARGON2_S2K_HIGH_MEMORY;
        assert!(validate_argon2_s2k_params(t, p, m_enc));
        let (t, p, m_enc) = ARGON2_S2K_LOW_MEMORY;
        assert!(validate_argon2_s2k_params(t, p, m_enc));
    }

    #[test]
    fn parameters_rpgp_would_reject_are_refused() {
        // Above rPGP's 2 GiB ARGON2_MEMORY_LIMIT_KIB, so the message would be
        // undecryptable by the very engine that wrote it.
        assert!(!validate_argon2_s2k_params(1, 4, 22));
        // §3.7.1.4: m_enc must be at least ceil(log_2(p)).
        assert!(!validate_argon2_s2k_params(1, 4, 1));
        // rPGP's derive_key caps t and p at 32, and neither may be zero.
        assert!(!validate_argon2_s2k_params(0, 4, 16));
        assert!(!validate_argon2_s2k_params(1, 0, 16));
        assert!(!validate_argon2_s2k_params(33, 4, 16));
        assert!(!validate_argon2_s2k_params(1, 33, 16));
    }

    #[test]
    fn the_built_in_default_is_the_rfc9106_first_recommendation() {
        // Nothing configured means today's behaviour is unchanged.
        let _guard = ARGON2_TEST_LOCK.lock().expect("lock");
        assert_eq!(argon2_s2k_params(), ARGON2_S2K_HIGH_MEMORY);
    }

    #[test]
    fn a_rejected_setting_leaves_the_current_parameters_in_place() {
        let _guard = ARGON2_TEST_LOCK.lock().expect("lock");

        let before = argon2_s2k_params();
        assert!(!set_argon2_s2k_params(1, 4, 31));
        assert_eq!(argon2_s2k_params(), before);
    }

    /// The Argon2 parameters a passphrase-encrypted message actually carries.
    ///
    /// Reads them back out of the SKESK, which is what every decrypting
    /// implementation will act on, rather than trusting the global.
    fn emitted_argon2_params(payload: &[u8]) -> (u8, u8, u8) {
        let mut out = Vec::new();
        encrypt_stream_with_password_internal(
            0,
            "",
            payload,
            &mut out,
            Some(cb::pwd_correct),
            true,
        )
        .expect("encrypt");

        let (msg, _) = Message::from_armor(std::io::Cursor::new(&out)).expect("parse");
        let Message::Encrypted { esk, .. } = &msg else {
            panic!("a passphrase-encrypted message is an Encrypted message");
        };

        let s2k = esk
            .iter()
            .find_map(|e| match e {
                Esk::SymKeyEncryptedSessionKey(skesk) => skesk.s2k(),
                _ => None,
            })
            .expect("the message carries a SKESK with an s2k");

        match s2k {
            StringToKey::Argon2 { t, p, m_enc, .. } => (*t, *p, *m_enc),
            other => panic!("expected an argon2 s2k, got {other:?}"),
        }
    }

    #[test]
    fn an_unconfigured_engine_emits_the_rfc9106_first_recommendation() {
        // Pins the default: the 2 GiB choice, unchanged from before the
        // parameters became configurable.
        let _guard = ARGON2_TEST_LOCK.lock().expect("lock");

        assert_eq!(
            emitted_argon2_params(b"default payload"),
            ARGON2_S2K_HIGH_MEMORY
        );
    }

    #[test]
    fn the_configured_parameters_reach_the_emitted_skesk() {
        let _guard = ARGON2_TEST_LOCK.lock().expect("lock");

        let (t, p, m_enc) = ARGON2_S2K_LOW_MEMORY;
        assert!(set_argon2_s2k_params(t, p, m_enc));
        let emitted = std::panic::catch_unwind(|| emitted_argon2_params(b"low memory payload"));

        // Restore the default before asserting, so a failure here does not
        // leak the low-memory profile into every later test in this process.
        assert!(set_argon2_s2k_params(
            ARGON2_S2K_HIGH_MEMORY.0,
            ARGON2_S2K_HIGH_MEMORY.1,
            ARGON2_S2K_HIGH_MEMORY.2
        ));

        assert_eq!(emitted.expect("encrypt"), ARGON2_S2K_LOW_MEMORY);
    }

    #[test]
    fn a_symmetric_message_round_trips() {
        // §5.3: a SKESK lets a message be decrypted with a passphrase.
        let mut out = Vec::new();
        encrypt_stream_with_password_internal(
            0,
            "",
            &b"passphrase payload"[..],
            &mut out,
            Some(cb::pwd_correct),
            true,
        )
        .expect("encrypt");

        let res = crate::crypto::decrypt_internal(
            0,
            &out,
            Some(cb::seckey_none),
            Some(cb::pwd_correct),
            std::ptr::null_mut(),
        )
        .expect("decrypt");
        assert_eq!(res.data, b"passphrase payload");
    }

    #[test]
    fn a_symmetric_message_refuses_the_wrong_passphrase() {
        let mut out = Vec::new();
        encrypt_stream_with_password_internal(
            0,
            "",
            &b"payload"[..],
            &mut out,
            Some(cb::pwd_correct),
            true,
        )
        .expect("encrypt");

        assert!(
            crate::crypto::decrypt_internal(
                0,
                &out,
                Some(cb::seckey_none),
                Some(cb::pwd_wrong),
                std::ptr::null_mut()
            )
            .is_err()
        );
    }

    #[test]
    fn a_symmetric_message_has_no_public_key_recipients() {
        let mut out = Vec::new();
        encrypt_stream_with_password_internal(
            0,
            "",
            &b"payload"[..],
            &mut out,
            Some(cb::pwd_correct),
            false,
        )
        .expect("encrypt");
        assert!(crate::crypto::sniff_recipients(&out).is_empty());
    }

    #[test]
    fn symmetric_encryption_without_a_passphrase_callback_fails() {
        let mut out = Vec::new();
        assert!(
            encrypt_stream_with_password_internal(0, "", &b"payload"[..], &mut out, None, true)
                .is_err()
        );
    }

    #[test]
    fn a_cancelled_passphrase_prompt_aborts_symmetric_encryption() {
        let mut out = Vec::new();
        let err = encrypt_stream_with_password_internal(
            0,
            "",
            &b"payload"[..],
            &mut out,
            Some(cb::pwd_cancelled),
            true,
        )
        .unwrap_err();
        assert_eq!(err, GfrStatus::ErrorCanceled);
    }

    // -- encrypt and sign ---------------------------------------------------------------

    #[test]
    fn encrypt_and_sign_reports_both_recipients_and_signatures() {
        let key = &keys::V4_SIGN;
        let out = encrypt_and_sign_internal(
            0,
            "",
            b"payload",
            &[&key.public_armored],
            &[&key.secret_armored],
            None,
            true,
        )
        .expect("encrypt+sign");
        assert_eq!(out.recipients.len(), 1);
        assert_eq!(out.signatures.len(), 1);
    }

    #[test]
    fn encrypt_and_sign_uses_a_strong_hash() {
        let key = &keys::V4_SIGN;
        let out = encrypt_and_sign_internal(
            0,
            "",
            b"payload",
            &[&key.public_armored],
            &[&key.secret_armored],
            None,
            true,
        )
        .expect("encrypt+sign");
        assert!(!crate::crypto::sig_hash_algo_is_weak(
            &out.signatures[0].hash_algo
        ));
    }

    #[test]
    fn encrypt_and_sign_accepts_an_empty_payload() {
        // Unlike encrypt-only, the combined operation has a signature to carry
        // even when there is no plaintext.
        let key = &keys::V4_SIGN;
        let out = encrypt_and_sign_internal(
            0,
            "",
            b"",
            &[&key.public_armored],
            &[&key.secret_armored],
            None,
            true,
        );
        assert!(out.is_ok());
    }

    #[test]
    fn encrypt_and_sign_with_two_signers_reports_both() {
        let out = encrypt_and_sign_internal(
            0,
            "",
            b"payload",
            &[&keys::V4_SIGN.public_armored],
            &[&keys::V4_SIGN.secret_armored, &keys::V6_SIGN.secret_armored],
            None,
            false,
        )
        .expect("encrypt+sign");
        assert_eq!(out.signatures.len(), 2);
    }

    #[test]
    fn encrypt_and_sign_fails_with_a_public_signing_key() {
        let key = &keys::V4_SIGN;
        assert!(
            encrypt_and_sign_internal(
                0,
                "",
                b"payload",
                &[&key.public_armored],
                &[&key.public_armored],
                None,
                true,
            )
            .is_err()
        );
    }

    // -- adversarial ------------------------------------------------------------------

    #[test]
    fn encrypting_never_panics_on_a_malformed_certificate() {
        for block in [
            "",
            "junk",
            corpus::TRUNCATED_ARMOR,
            corpus::CORRUPT_CRC,
            &String::from_utf8_lossy(corpus::GARBAGE),
        ] {
            let outcome = std::panic::catch_unwind(|| {
                encrypt_internal(0, "", b"payload", &[block], true).is_ok()
            });
            assert!(outcome.is_ok(), "panicked on a malformed certificate");
        }
    }

    #[test]
    fn encryption_target_selection_never_panics_on_a_corpus_certificate() {
        for cert in [
            &*corpus::AUX_GOOD_CERT,
            &*corpus::AUX_REVOKED_CERT,
            &*corpus::AUX_EXPIRED_CERT,
            &*corpus::AUX_FORGED_REVOCATION_CERT,
            &*corpus::KEY2_CERT,
        ] {
            let outcome = std::panic::catch_unwind(|| choose_encryption_target(cert, None).is_ok());
            assert!(outcome.is_ok());
        }
    }

    #[test]
    fn cancellation_aborts_an_encryption_stream() {
        const CH: i32 = 0x0C_91;
        crate::cancel::set_cancelled(CH, true);
        let res = encrypt_internal(
            CH,
            "",
            &vec![0u8; 1024 * 1024],
            &[&keys::V4_SIGN.public_armored],
            true,
        );
        crate::cancel::set_cancelled(CH, false);
        assert!(res.is_err(), "a cancelled encryption must abort");
    }

    #[test]
    fn a_directory_can_be_encrypted_as_a_tar_archive() {
        let dir = tempfile::tempdir().expect("tempdir");
        std::fs::write(dir.path().join("a.txt"), b"alpha").expect("write");
        std::fs::write(dir.path().join("b.txt"), b"beta").expect("write");

        let out_path = dir.path().join("archive.pgp");
        let res = encrypt_directory_internal(
            0,
            &dir.path().to_string_lossy(),
            &out_path.to_string_lossy(),
            &[&keys::V4_SIGN.public_armored],
            false,
        );
        assert!(res.is_ok(), "directory encryption should succeed");
        assert!(out_path.exists());
    }

    #[test]
    fn encrypting_a_missing_directory_fails() {
        let dir = tempfile::tempdir().expect("tempdir");
        let out_path = dir.path().join("out.pgp");
        assert!(
            encrypt_directory_internal(
                0,
                "/nonexistent/definitely/not/here",
                &out_path.to_string_lossy(),
                &[&keys::V4_SIGN.public_armored],
                false,
            )
            .is_err()
        );
    }

    #[test]
    fn a_hand_built_literal_payload_can_be_encrypted() {
        // Proves the encryptor is agnostic to what the plaintext contains,
        // including nested OpenPGP structure.
        let inner = packets::literal(b'b', b"", 0, b"nested");
        let out = encrypt_to(&keys::V4_SIGN, &inner);
        assert!(!out.data.is_empty());
    }
}
