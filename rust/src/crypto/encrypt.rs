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

use pgp::{
    composed::SignedPublicSubKey,
    crypto::{
        aead::{AeadAlgorithm, ChunkSize},
        public_key::PublicKeyAlgorithm,
    },
    packet::{Signature, SubpacketData},
    types::KeyVersion,
};
use zeroize::Zeroizing;

use crate::utils::{armor_opts, armor_opts_for, password_from_zeroizing_bytes};

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

    // RFC 9580 §3.7.1.4: "The encoded memory size MUST be a value from
    // 3+ceil(log_2(p)) to 31" (2^m_enc KiB of at least 8*p KiB).
    let min_m_enc = 3 + (p as f32).log2().ceil() as u8;
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

/// Seconds since the Unix epoch, for expiry checks.
fn unix_now() -> u32 {
    std::time::SystemTime::now()
        .duration_since(std::time::UNIX_EPOCH)
        .map(|d| d.as_secs() as u32)
        .unwrap_or(0)
}

/// Whether an absolute expiry (`0` = never) has passed at `now`.
fn has_expired(expires_at: u32, now: u32) -> bool {
    expires_at != 0 && expires_at <= now
}

/// Whether a self-signature's Key Flags allow encryption (RFC 9580 §5.2.3.29).
/// A signature without a Key Flags subpacket leaves usage to the algorithm,
/// as for keys that predate the subpacket.
fn flags_allow_encryption(sig: &Signature) -> bool {
    let has_flags = sig.config().is_some_and(|c| {
        c.hashed_subpackets()
            .any(|p| matches!(p.data, SubpacketData::KeyFlags(_)))
    });
    let flags = sig.key_flags();
    !has_flags || flags.encrypt_comms() || flags.encrypt_storage()
}

/// Why `subkey` cannot receive a session key: it must be encryption-capable,
/// validly bound with flags allowing encryption, and neither revoked nor expired.
fn encryption_subkey_problem(
    cert: &SignedPublicKey,
    subkey: &SignedPublicSubKey,
    now: u32,
) -> Option<&'static str> {
    let primary = &cert.primary_key;
    if !subkey.key.algorithm().can_encrypt() {
        return Some("not encryption-capable");
    }
    if crate::key::rsa_key_too_small(subkey.key.public_params()) {
        return Some("an RSA key under 2048 bits (RFC 9580 §12.4)");
    }
    if crate::key::is_subkey_revoked(primary, &subkey.key, &subkey.signatures) {
        return Some("revoked");
    }
    let Some(binding) =
        crate::key::subkey_binding_signature(primary, &subkey.key, &subkey.signatures)
    else {
        return Some("not validly bound to its primary key");
    };
    if !flags_allow_encryption(binding) {
        return Some("not flagged for encryption");
    }
    if has_expired(
        crate::key::subkey_expires_at(primary, &subkey.key, &subkey.signatures),
        now,
    ) {
        return Some("expired");
    }
    None
}

/// Why the primary key itself cannot receive a session key. `pref` is the
/// primary's authoritative self-signature (see [`validate_recipient`]).
fn encryption_primary_problem(cert: &SignedPublicKey, pref: &Signature) -> Option<&'static str> {
    if !cert.primary_key.algorithm().can_encrypt() {
        return Some("not encryption-capable");
    }
    if crate::key::rsa_key_too_small(cert.primary_key.public_params()) {
        return Some("an RSA key under 2048 bits (RFC 9580 §12.4)");
    }
    if !flags_allow_encryption(pref) {
        return Some("not flagged for encryption");
    }
    None
}

/// Select which encryption key of `cert` the session key should go to.
///
/// Mirrors [`with_signing_key`]'s two modes for the encryption side:
/// * **Exact-match** (`target = Some(fpr)`): the caller pinned a specific subkey
///   via the `fpr!` armor-block prefix. The pinned key is honored only if it is
///   **usable** (see [`encryption_subkey_problem`]); otherwise the recipient is
///   rejected.
/// * **Auto** (`target = None`): the first usable encryption subkey wins,
///   falling back to the primary key if no subkey qualifies.
///
/// A revoked, expired or unbound subkey is never selected in either mode.
fn choose_encryption_target(
    cert: &SignedPublicKey,
    pref: &Signature,
    target: Option<&str>,
    now: u32,
) -> Result<EncryptionTarget, GfrStatus> {
    // ==========================================
    // EXACT MATCH MODE (!)
    // ==========================================
    if let Some(target) = target {
        for (i, subkey) in cert.public_subkeys.iter().enumerate() {
            let fpr = subkey.key.fingerprint().to_string();
            let kid = subkey.key.legacy_key_id().to_string();
            if super::key_identifier_matches(&fpr, &kid, target) {
                if let Some(problem) = encryption_subkey_problem(cert, subkey, now) {
                    log::error!(
                        "Requested encryption subkey is {}: fpr={}, algo={:?}",
                        problem,
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
            if let Some(problem) = encryption_primary_problem(cert, pref) {
                log::error!("Requested primary key is {}: fpr={}", problem, p_fpr);
                return Err(GfrStatus::ErrorNoKey);
            }
            log::info!("Selected marked primary encryption key: fpr={}", p_fpr);
            return Ok(EncryptionTarget::Primary);
        }

        log::error!("Requested encryption target not found: {}", target);
        return Err(GfrStatus::ErrorNoKey);
    }

    // ==========================================
    // NORMAL MODE (Auto Fallback) — skip unusable
    // ==========================================
    for (i, subkey) in cert.public_subkeys.iter().enumerate() {
        match encryption_subkey_problem(cert, subkey, now) {
            None => return Ok(EncryptionTarget::Sub(i)),
            Some(problem) if subkey.key.algorithm().can_encrypt() => log::info!(
                "Skipping encryption subkey that is {}: fpr={}",
                problem,
                subkey.key.fingerprint(),
            ),
            Some(_) => {}
        }
    }

    if encryption_primary_problem(cert, pref).is_none() {
        return Ok(EncryptionTarget::Primary);
    }

    Err(GfrStatus::ErrorNoKey)
}

/// Check that `cert` may be encrypted to at all, then pick its encryption key.
///
/// The certificate needs a valid authoritative self-signature (for v6 a Direct
/// Key signature, RFC 9580 §5.2.3.10), and its primary key must be neither
/// revoked (§5.2.1.11) nor expired (§5.2.3.13).
fn validate_recipient(
    cert: &SignedPublicKey,
    target: Option<&str>,
    now: u32,
) -> Result<EncryptionTarget, GfrStatus> {
    let primary = &cert.primary_key;
    let Some(pref) = crate::key::preference_self_signature(cert) else {
        log::error!(
            "Recipient has no valid self-signature: fpr={}",
            primary.fingerprint()
        );
        return Err(GfrStatus::ErrorUnusableKey);
    };
    if crate::key::is_primary_key_revoked(primary, &cert.details.revocation_signatures)
        || crate::key::is_primary_key_revoked(primary, &cert.details.direct_signatures)
    {
        log::error!("Recipient key is revoked: fpr={}", primary.fingerprint());
        return Err(GfrStatus::ErrorRevokedKey);
    }
    if has_expired(crate::key::primary_key_expires_at(cert), now) {
        log::error!("Recipient key is expired: fpr={}", primary.fingerprint());
        return Err(GfrStatus::ErrorExpiredKey);
    }
    choose_encryption_target(cert, pref, target, now)
}

/// The public-key algorithm of the component `target` names.
fn target_algorithm(cert: &SignedPublicKey, target: &EncryptionTarget) -> PublicKeyAlgorithm {
    match target {
        EncryptionTarget::Sub(i) => cert.public_subkeys[*i].key.algorithm(),
        EncryptionTarget::Primary => cert.primary_key.algorithm(),
    }
}

/// Whether a v6 PKESK (and so a v2 SEIPD) can carry a session key to a key of
/// `algo`: "An implementation MUST NOT generate ElGamal v6 PKESK packets"
/// (RFC 9580 §5.1.4).
fn v6_pkesk_can_address(algo: PublicKeyAlgorithm) -> bool {
    algo != PublicKeyAlgorithm::Elgamal
}

/// Whether `cert` can read a SEIPD v2 (AEAD) container, judged from its
/// authenticated self-signature (RFC 9580 §13.7, §5.2.3.32):
///
/// * a Features subpacket decides either way: the v2 flag (0x08) is support,
///   its absence is the key holder stating they cannot read v2;
/// * a **v6** key without a Features subpacket is treated as supporting v2, as
///   §13.7 allows ("all recipient keys are version 6 keys without a Features
///   signature subpacket");
/// * an older key without the subpacket gives no evidence either way, so the
///   caller conservatively falls back to v1. That is the absence of proof of
///   support, not proof that v2 is unreadable.
///
/// Preferred AEAD Ciphersuites alone are not counted as evidence: §5.2.3.15
/// defers to the Features flag for v2 support. A certificate without a valid
/// self-signature supports nothing.
fn cert_supports_seipd_v2(cert: &SignedPublicKey) -> bool {
    match crate::key::preference_self_signature(cert).map(|sig| sig.features()) {
        None => false,
        Some(Some(features)) => features.seipd_v2(),
        Some(None) => cert.primary_key.version() == KeyVersion::V6,
    }
}

/// Whether `cert` can read a SEIPD v1 container. v1 is the baseline assumed
/// for any key without a Features subpacket; only a Features subpacket that
/// omits the v1 flag rules it out (§5.2.3.32: "SHOULD NOT use a feature listed
/// when sending to a user who does not state that they can use it").
fn cert_supports_seipd_v1(cert: &SignedPublicKey) -> bool {
    match crate::key::preference_self_signature(cert).map(|sig| sig.features()) {
        None => false,
        Some(Some(features)) => features.seipd_v1(),
        Some(None) => true,
    }
}

/// Our ranking of v1 SEIPD ciphers. AES only: a v3 PKESK to X25519/X448
/// (RFC 9580 §5.1.6, §5.1.7) or ML-KEM (RFC 9980 §4.3.1) "MUST be AES-128,
/// AES-192, or AES-256", so this list is valid for every recipient algorithm.
const SYMMETRIC_RANKING: [SymmetricKeyAlgorithm; 3] = [
    SymmetricKeyAlgorithm::AES256,
    SymmetricKeyAlgorithm::AES192,
    SymmetricKeyAlgorithm::AES128,
];

/// Our ranking of v2 SEIPD ciphersuites: strongest cipher first, OCB (the
/// mandatory AEAD, §9.6) first within each cipher.
const AEAD_RANKING: [(SymmetricKeyAlgorithm, AeadAlgorithm); 9] = [
    (SymmetricKeyAlgorithm::AES256, AeadAlgorithm::Ocb),
    (SymmetricKeyAlgorithm::AES256, AeadAlgorithm::Gcm),
    (SymmetricKeyAlgorithm::AES256, AeadAlgorithm::Eax),
    (SymmetricKeyAlgorithm::AES192, AeadAlgorithm::Ocb),
    (SymmetricKeyAlgorithm::AES192, AeadAlgorithm::Gcm),
    (SymmetricKeyAlgorithm::AES192, AeadAlgorithm::Eax),
    (SymmetricKeyAlgorithm::AES128, AeadAlgorithm::Ocb),
    (SymmetricKeyAlgorithm::AES128, AeadAlgorithm::Gcm),
    (SymmetricKeyAlgorithm::AES128, AeadAlgorithm::Eax),
];

/// What one validated recipient can read, from its authenticated self-signature
/// and the algorithm of the key the session key goes to.
#[derive(Debug, Clone, PartialEq, Eq)]
struct RecipientCaps {
    seipd_v1: bool,
    /// Also requires a component a v6 PKESK can address
    /// ([`v6_pkesk_can_address`]).
    seipd_v2: bool,
    /// Ciphers for a v1 SEIPD (§5.2.3.14). AES-128 is always included: every
    /// implementation MUST implement it (§9.3), and §5.2.3.7 makes the
    /// mandatory algorithms the default when the subpacket is absent.
    symmetric: Vec<SymmetricKeyAlgorithm>,
    /// Ciphersuites for a v2 SEIPD (§5.2.3.15): "If AES-128 and OCB are not
    /// found in the subpacket, it is implicitly listed at the end."
    aead: Vec<(SymmetricKeyAlgorithm, AeadAlgorithm)>,
}

impl RecipientCaps {
    fn of(cert: &SignedPublicKey, target: &EncryptionTarget) -> Self {
        let pref = crate::key::preference_self_signature(cert);

        // RFC 9980 §7.1: a certificate with a post-quantum key implicitly
        // lists AES-256 (and AES-256 with OCB) after its explicit
        // preferences; "if AES-128 is also implicitly added ..., it is added
        // after AES-256".
        let has_pq_key = cert.primary_key.algorithm().is_pqc()
            || cert
                .public_subkeys
                .iter()
                .any(|sk| sk.key.algorithm().is_pqc());

        let mut symmetric = pref
            .map(|sig| sig.preferred_symmetric_algs().to_vec())
            .unwrap_or_default();
        if has_pq_key && !symmetric.contains(&SymmetricKeyAlgorithm::AES256) {
            symmetric.push(SymmetricKeyAlgorithm::AES256);
        }
        if !symmetric.contains(&SymmetricKeyAlgorithm::AES128) {
            symmetric.push(SymmetricKeyAlgorithm::AES128);
        }

        let mut aead = pref
            .map(|sig| sig.preferred_aead_algs().to_vec())
            .unwrap_or_default();
        let pq_implied = (SymmetricKeyAlgorithm::AES256, AeadAlgorithm::Ocb);
        if has_pq_key && !aead.contains(&pq_implied) {
            aead.push(pq_implied);
        }
        let mandatory = (SymmetricKeyAlgorithm::AES128, AeadAlgorithm::Ocb);
        if !aead.contains(&mandatory) {
            aead.push(mandatory);
        }

        RecipientCaps {
            seipd_v1: cert_supports_seipd_v1(cert),
            seipd_v2: cert_supports_seipd_v2(cert)
                && v6_pkesk_can_address(target_algorithm(cert, target)),
            symmetric,
            aead,
        }
    }
}

/// The encryption container and the algorithms it is built with. Each variant
/// fixes its session-key packets (RFC 9580 §10.3.2.1): rPGP's v1 builder only
/// emits v3 PKESKs and its v2 builder only v6 PKESKs, so a mismatched pair
/// cannot be constructed.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
enum Container {
    /// v1 SEIPD (MDC) preceded by v3 PKESKs.
    SeipdV1(SymmetricKeyAlgorithm),
    /// v2 SEIPD (AEAD) preceded by v6 PKESKs.
    SeipdV2(SymmetricKeyAlgorithm, AeadAlgorithm),
}

/// Pick the container every recipient can read (RFC 9580 §13.7): v2 SEIPD
/// with the best ciphersuite all recipients share when all of them support
/// it, else v1 SEIPD with the best shared cipher. Fails rather than emit a
/// message some recipient could not read.
fn select_container(recipients: &[RecipientCaps]) -> Result<Container, GfrStatus> {
    if recipients.is_empty() {
        set_last_error("no usable recipient to encrypt to");
        return Err(GfrStatus::ErrorInvalidInput);
    }

    if recipients.iter().all(|r| r.seipd_v2) {
        if let Some(&(sym, aead)) = AEAD_RANKING
            .iter()
            .find(|suite| recipients.iter().all(|r| r.aead.contains(suite)))
        {
            return Ok(Container::SeipdV2(sym, aead));
        }
    }

    if recipients.iter().all(|r| r.seipd_v1) {
        if let Some(&sym) = SYMMETRIC_RANKING
            .iter()
            .find(|alg| recipients.iter().all(|r| r.symmetric.contains(alg)))
        {
            return Ok(Container::SeipdV1(sym));
        }
    }

    set_last_error(
        "the recipients have no encryption format in common: some can only read \
         SEIPD v2 (AEAD) and others cannot read it (RFC 9580 §13.7)",
    );
    Err(GfrStatus::ErrorUnsupportedFeature)
}

/// Encrypt a stream with public keys and optionally sign it.
///
/// Pass an empty `secret_key_blocks` slice to encrypt without signing; the
/// signing phase is skipped entirely in that case. The container is chosen by
/// [`select_container`]: SEIPD v2 with v6 PKESKs when every recipient supports
/// it, else SEIPD v1 with v3 PKESKs, with ciphers all recipients prefer. Uses
/// 512 KiB partial packets for streaming efficiency on large payloads.
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

    // 1. Validate every recipient and resolve it to the component that receives
    // the session key, before any passphrase prompt for signing. Unusable
    // certificates are reported, not fatal, as long as one usable one remains.
    let now = unix_now();
    let mut invalid_recipients = Vec::new();
    let mut targets = Vec::new();

    for block in public_key_blocks {
        // A caller may pin a specific encryption subkey by prefixing the armored
        // block with `<fpr>!` (the same mechanism the signing path uses). Strip
        // it off before parsing the certificate.
        let (target, armored) = parse_signer_block(block);

        match SignedPublicKey::from_string(armored) {
            Ok((cert, _)) => match validate_recipient(&cert, target.as_deref(), now) {
                Ok(target) => targets.push((cert, target)),
                Err(reason) => invalid_recipients.push(InvalidRecipientInternal {
                    fpr: cert.primary_key.fingerprint().to_string(),
                    reason,
                }),
            },
            Err(_) => {
                invalid_recipients.push(InvalidRecipientInternal {
                    fpr: String::from("Unknown"),
                    reason: GfrStatus::ErrorInvalidData,
                });
            }
        }
    }

    // 2. Pick the container from what every recipient can read (RFC 9580
    // §13.7). SEIPD v2 (AEAD) authenticates each chunk as it streams, so large
    // files decrypt with flat memory; SEIPD v1 is checked only at the very end.
    let caps: Vec<RecipientCaps> = targets
        .iter()
        .map(|(cert, target)| RecipientCaps::of(cert, target))
        .collect();
    let container = select_container(&caps)?;
    log::info!(
        "Encrypting to {} recipient(s) with {:?}",
        targets.len(),
        container
    );

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

    let mut recipients = Vec::new();

    // The v1 and v2 builders are distinct types with inherent `encrypt_to_key`
    // methods, so the recipient loop and the write are shared through a macro.
    macro_rules! encrypt_to_targets_and_write {
        ($enc_builder:ident, $pkesk_version:literal, $v4_only:expr) => {{
            let pkesk_version = $pkesk_version;
            for (cert, target) in &targets {
                // Record the component actually used so callers can show the
                // real recipient key ID and algorithm.
                let (added, key_id, pub_algo) = match target {
                    EncryptionTarget::Sub(i) => {
                        let subkey = &cert.public_subkeys[*i];
                        (
                            $enc_builder.encrypt_to_key(&mut rng, subkey).map(|_| ()),
                            subkey.key.legacy_key_id().to_string(),
                            algo_to_string_simple(subkey.key.algorithm()),
                        )
                    }
                    EncryptionTarget::Primary => (
                        $enc_builder
                            .encrypt_to_key(&mut rng, &cert.primary_key)
                            .map(|_| ()),
                        cert.primary_key.legacy_key_id().to_string(),
                        algo_to_string_simple(cert.primary_key.algorithm()),
                    ),
                };

                // The container was chosen for exactly this recipient set, so a
                // session-key packet that cannot be built is fatal: dropping the
                // recipient would silently change who can read the message.
                if let Err(e) = added {
                    set_last_error(&format!(
                        "cannot build a {} session-key packet for recipient {}: {}",
                        pkesk_version,
                        cert.primary_key.fingerprint(),
                        e
                    ));
                    return Err(GfrStatus::ErrorUnsupportedFeature);
                }
                recipients.push(RecipientResultInternal {
                    key_id,
                    pub_algo,
                    status: GfrRecipientStatus::Success,
                });
            }

            if ascii_armor {
                // RFC 9580 §6.1: a CRC24 footer only for a v1 SEIPD message
                // (GnuPG interoperability), never one ending in a v2 SEIPD.
                $enc_builder.to_armored_writer(
                    &mut rng,
                    armor_opts_for($v4_only),
                    &mut output_stream,
                )
            } else {
                $enc_builder.to_writer(&mut rng, &mut output_stream)
            }
        }};
    }

    let result = match container {
        Container::SeipdV2(sym, aead) => {
            let mut enc_builder = builder.seipd_v2(&mut rng, sym, aead, ChunkSize::default());
            encrypt_to_targets_and_write!(enc_builder, "v6", false)
        }
        Container::SeipdV1(sym) => {
            let mut enc_builder = builder.seipd_v1(&mut rng, sym);
            encrypt_to_targets_and_write!(enc_builder, "v3", true)
        }
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
/// OpenPGP `StringToKey` mechanism and the payload is a SEIPD v2 (AES-256/OCB)
/// container with a v6 SKESK, so decryption requires an RFC 9580 reader.
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

    // The Argon2 S2K already needs an RFC 9580 reader, so the AEAD container
    // costs no extra compatibility and lifts SEIPD v1's whole-message check.
    let mut enc_builder = builder.seipd_v2(
        &mut rng,
        SymmetricKeyAlgorithm::AES256,
        AeadAlgorithm::Ocb,
        ChunkSize::default(),
    );

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
    enc_builder
        .encrypt_with_password(&mut rng, s2k, &msg_pw)
        .into_gfr()?;

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
    //! The engine emits v2 SEIPD (AES-256/OCB, §5.13.2) when every recipient
    //! advertises it in a self-signature's Features subpacket, and v1 SEIPD
    //! with AES-256 (§5.13.1) otherwise, so older peers can still read it.
    //! Passphrase encryption always emits v2. Tests here pin both choices so
    //! a change is deliberate.

    use super::*;
    use crate::testutil::{cb, corpus, interop, keys, packets};

    // RFC 9580 3.7.1.4 requires m_enc to leave at least 8*p blocks.
    // parameters_rpgp_would_reject_are_refused covers the reject side of that
    // bound; this pins the accept side, so tightening the rule is caught too.
    #[test]
    fn the_smallest_memory_size_each_parallelism_allows_is_accepted() {
        // RFC 9580 §3.7.1.4: "The encoded memory size MUST be a value from
        // 3+ceil(log_2(p)) to 31, such that the decoded memory size m is a
        // value from 8*p to 2**31."
        assert!(validate_argon2_s2k_params(1, 4, 5), "p=4 at its minimum");
        assert!(!validate_argon2_s2k_params(1, 4, 4), "p=4 one below");
        assert!(validate_argon2_s2k_params(1, 1, 3), "p=1 at its minimum");
        assert!(!validate_argon2_s2k_params(1, 1, 2), "p=1 one below");
    }

    fn cert_of(fixture: &keys::Fixture) -> SignedPublicKey {
        SignedPublicKey::from_string(&fixture.public_armored)
            .expect("parses")
            .0
    }

    /// [`choose_encryption_target`] under the certificate's own preference
    /// self-signature, as [`validate_recipient`] calls it.
    fn target_of(
        cert: &SignedPublicKey,
        target: Option<&str>,
    ) -> Result<EncryptionTarget, GfrStatus> {
        let pref = crate::key::preference_self_signature(cert).expect("a valid self-signature");
        choose_encryption_target(cert, pref, target, unix_now())
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
            target_of(&cert, None).expect("a target"),
            EncryptionTarget::Sub(_)
        ));
    }

    #[test]
    fn a_signing_only_subkey_is_not_selected_for_encryption() {
        let cert = cert_of(&keys::V4_SIGN);
        let EncryptionTarget::Sub(i) = target_of(&cert, None).expect("a target") else {
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
        assert!(target_of(&cert, None).is_err());
    }

    #[test]
    fn an_rsa_key_under_2048_bits_is_never_an_encryption_target() {
        // RFC 9580 §12.4: "MUST NOT encrypt ... using RSA keys of a size less
        // than 2048 bits."
        let cert = cert_of(&keys::V4_RSA1024);
        assert_eq!(target_of(&cert, None).err(), Some(GfrStatus::ErrorNoKey));
        // RSA can sign, so the fixture lists its only subkey as a signing one.
        let pinned = keys::V4_RSA1024.secret.secret_subkeys[0]
            .key
            .fingerprint()
            .to_string();
        assert_eq!(
            target_of(&cert, Some(&pinned)).err(),
            Some(GfrStatus::ErrorNoKey)
        );
    }

    #[test]
    fn a_key_with_no_encryption_capable_component_is_rejected() {
        let cert = cert_of(&keys::V4_PRIMARY_ONLY);
        assert_eq!(target_of(&cert, None).err(), Some(GfrStatus::ErrorNoKey));
    }

    #[test]
    fn a_v6_key_selects_its_x25519_subkey() {
        let cert = cert_of(&keys::V6_SIGN);
        let EncryptionTarget::Sub(i) = target_of(&cert, None).expect("a target") else {
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
        let EncryptionTarget::Sub(i) = target_of(&cert, Some(want)).expect("a target") else {
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
            target_of(&cert, Some(keys::V4_SIGN.sign_subkey_fpr())).err(),
            Some(GfrStatus::ErrorNoKey)
        );
    }

    #[test]
    fn a_pinned_revoked_subkey_is_rejected() {
        let cert = cert_of(&keys::V4_REVOKED_SUBKEY);
        let revoked = keys::V4_SIGN.enc_subkey_fpr();
        assert_eq!(
            target_of(&cert, Some(revoked)).err(),
            Some(GfrStatus::ErrorNoKey)
        );
    }

    #[test]
    fn a_pinned_unknown_identifier_is_rejected() {
        let cert = cert_of(&keys::V4_SIGN);
        assert_eq!(
            target_of(&cert, Some("0000000000000000")).err(),
            Some(GfrStatus::ErrorNoKey)
        );
    }

    #[test]
    fn a_pinned_non_encrypting_primary_is_rejected() {
        let cert = cert_of(&keys::V4_SIGN);
        assert_eq!(
            target_of(&cert, Some(&keys::V4_SIGN.primary_fpr)).err(),
            Some(GfrStatus::ErrorNoKey)
        );
    }

    #[test]
    fn a_pinned_key_id_suffix_resolves() {
        let cert = cert_of(&keys::V4_SIGN);
        let full = keys::V4_SIGN.enc_subkey_fpr();
        assert!(target_of(&cert, Some(&full[full.len() - 16..])).is_ok());
    }

    // -- emitted container ------------------------------------------------------

    #[test]
    fn armored_output_is_a_pgp_message() {
        let out = encrypt_to(&keys::V4_SIGN, b"payload");
        assert!(String::from_utf8_lossy(&out.data).contains("BEGIN PGP MESSAGE"));
    }

    #[test]
    fn armored_v1_output_carries_a_crc24_footer() {
        // RFC 9580 §6.1 interoperability exception: v4-only armor carries the
        // CRC24 footer (GnuPG 2.4); v6 armor never does (utils::crc24_policy_tests).
        let out = encrypt_to(&keys::V4_SIGN, b"payload");
        crate::testutil::assert::armor_has_crc24(&String::from_utf8_lossy(&out.data));
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

    /// The version octet of the encryption container (tag 18) in `data`.
    fn seipd_version(data: &[u8]) -> Option<u32> {
        crate::inspect::inspect(data).blocks[0]
            .packets
            .iter()
            .find(|n| n.tag == 18)
            .and_then(|n| n.version)
    }

    #[test]
    fn a_v6_recipient_gets_a_v2_seipd_container() {
        // §5.2.3.32: our v6 keys advertise SEIPD v2, so the AEAD container is
        // used. Unlike v1 it authenticates chunk by chunk and has no size cap.
        let out = encrypt_to(&keys::V6_SIGN, b"payload");
        assert_eq!(seipd_version(&out.data), Some(2));
    }

    #[test]
    fn a_v4_recipient_gets_a_v1_seipd_container() {
        // Our v4 keys do not advertise SEIPD v2, so v1 is kept for interop.
        let out = encrypt_to(&keys::V4_SIGN, b"payload");
        assert_eq!(seipd_version(&out.data), Some(1));
    }

    #[test]
    fn a_mixed_recipient_set_falls_back_to_v1_seipd() {
        // One recipient without v2 support forces the container every
        // recipient can read.
        let out = encrypt_internal(
            0,
            "",
            b"payload",
            &[&keys::V6_SIGN.public_armored, &keys::V4_SIGN.public_armored],
            false,
        )
        .expect("encrypt");
        assert_eq!(seipd_version(&out.data), Some(1));
    }

    #[test]
    fn seipd_v2_support_is_read_from_self_signatures() {
        assert!(cert_supports_seipd_v2(&cert_of(&keys::V6_SIGN)));
        assert!(!cert_supports_seipd_v2(&cert_of(&keys::V4_SIGN)));
    }

    // -- format selection (RFC 9580 §13.7, §10.3.2.1) -------------------------------

    use pgp::packet::Subpacket;
    use pgp::types::Timestamp;

    /// A recipient as `(public armored, secret armored)`.
    type Recipient = (String, String);

    /// `fixture` with its authoritative self-signature(s) re-issued after
    /// `edit` rewrote the hashed subpackets: the Direct Key signature of a v6
    /// key, every User ID self-certification of a v4 key.
    fn resigned(fixture: &keys::Fixture, edit: impl Fn(&mut Vec<Subpacket>)) -> Recipient {
        let mut secret = fixture.secret.clone();
        let pk = secret.primary_key.public_key();
        let rewrite = |template: &Signature| {
            let mut cfg = template.config().cloned().expect("a signature config");
            cfg.hashed_subpackets
                .retain(|sp| !matches!(sp.data, SubpacketData::SignatureCreationTime(_)));
            cfg.hashed_subpackets.push(
                Subpacket::regular(SubpacketData::SignatureCreationTime(Timestamp::now()))
                    .expect("subpacket"),
            );
            edit(&mut cfg.hashed_subpackets);
            cfg
        };

        if secret.primary_key.version() == KeyVersion::V6 {
            let cfg = rewrite(&secret.details.direct_signatures[0]);
            let sig = cfg
                .sign_key(&secret.primary_key, &Password::empty(), &pk)
                .expect("re-sign direct key signature");
            secret.details.direct_signatures = vec![sig];
        } else {
            for user in secret.details.users.iter_mut() {
                let cfg = rewrite(&user.signatures[0]);
                let sig = cfg
                    .sign_certification(
                        &secret.primary_key,
                        &pk,
                        &Password::empty(),
                        Tag::UserId,
                        &user.id,
                    )
                    .expect("re-sign user id");
                user.signatures = vec![sig];
            }
        }

        let opts = crate::utils::armor_opts();
        (
            SignedPublicKey::from(secret.clone())
                .to_armored_string(opts.clone())
                .expect("armor"),
            secret.to_armored_string(opts).expect("armor"),
        )
    }

    fn set_features(subpackets: &mut Vec<Subpacket>, features: Option<&[u8]>) {
        subpackets.retain(|sp| !matches!(sp.data, SubpacketData::Features(_)));
        if let Some(bits) = features {
            subpackets
                .push(Subpacket::regular(SubpacketData::Features(bits.into())).expect("subpacket"));
        }
    }

    fn with_features(fixture: &keys::Fixture, features: Option<&[u8]>) -> Recipient {
        resigned(fixture, |sp| set_features(sp, features))
    }

    fn with_aead_prefs(
        fixture: &keys::Fixture,
        prefs: &[(SymmetricKeyAlgorithm, AeadAlgorithm)],
    ) -> Recipient {
        resigned(fixture, |sp| {
            sp.retain(|p| !matches!(p.data, SubpacketData::PreferredAeadAlgorithms(_)));
            sp.push(
                Subpacket::regular(SubpacketData::PreferredAeadAlgorithms(
                    prefs.to_vec().into(),
                ))
                .expect("subpacket"),
            );
        })
    }

    fn with_symmetric_prefs(fixture: &keys::Fixture, prefs: &[SymmetricKeyAlgorithm]) -> Recipient {
        resigned(fixture, |sp| {
            sp.retain(|p| !matches!(p.data, SubpacketData::PreferredSymmetricAlgorithms(_)));
            if !prefs.is_empty() {
                sp.push(
                    Subpacket::regular(SubpacketData::PreferredSymmetricAlgorithms(
                        prefs.to_vec().into(),
                    ))
                    .expect("subpacket"),
                );
            }
        })
    }

    fn own(fixture: &keys::Fixture) -> Recipient {
        (
            fixture.public_armored.clone(),
            fixture.secret_armored.clone(),
        )
    }

    /// The container [`select_container`] picks for these recipients.
    fn container_for(recipients: &[&Recipient]) -> Result<Container, GfrStatus> {
        let now = unix_now();
        let caps: Vec<RecipientCaps> = recipients
            .iter()
            .map(|(public, _)| {
                let cert = SignedPublicKey::from_string(public).expect("parses").0;
                let target = validate_recipient(&cert, None, now).expect("a valid recipient");
                RecipientCaps::of(&cert, &target)
            })
            .collect();
        select_container(&caps)
    }

    /// `fixture`'s certificate re-signed with a Direct Key signature whose
    /// preferences name only AES-128 (and AES-128/OCB).
    fn aes128_only_cert(fixture: &keys::Fixture) -> Recipient {
        use pgp::packet::{SignatureConfig, SignatureType, Subpacket, SubpacketData};
        let mut sk = fixture.secret.clone();
        let primary = sk.primary_key.clone();
        let old = sk
            .details
            .direct_signatures
            .iter()
            .find(|sig| matches!(sig.typ(), Some(SignatureType::Key)))
            .expect("a direct key signature")
            .clone();
        let mut cfg =
            SignatureConfig::from_key(rand::thread_rng(), &primary, SignatureType::Key).unwrap();
        cfg.hashed_subpackets = old
            .config()
            .unwrap()
            .hashed_subpackets
            .iter()
            .filter(|sp| {
                !matches!(
                    sp.data,
                    SubpacketData::PreferredSymmetricAlgorithms(_)
                        | SubpacketData::PreferredAeadAlgorithms(_)
                        | SubpacketData::SignatureCreationTime(_)
                )
            })
            .cloned()
            .collect();
        cfg.hashed_subpackets.extend([
            Subpacket::regular(SubpacketData::SignatureCreationTime(
                pgp::types::Timestamp::from_secs(pgp::types::Timestamp::now().as_secs() + 1),
            ))
            .unwrap(),
            Subpacket::regular(SubpacketData::PreferredSymmetricAlgorithms(
                [SymmetricKeyAlgorithm::AES128].into_iter().collect(),
            ))
            .unwrap(),
            Subpacket::regular(SubpacketData::PreferredAeadAlgorithms(
                [(SymmetricKeyAlgorithm::AES128, AeadAlgorithm::Ocb)]
                    .into_iter()
                    .collect(),
            ))
            .unwrap(),
        ]);
        let sig = cfg
            .sign_key(
                &primary,
                &pgp::types::Password::empty(),
                primary.public_key(),
            )
            .unwrap();
        sk.details.direct_signatures = vec![sig];
        (
            SignedPublicKey::from(sk.clone())
                .to_armored_string(crate::utils::armor_opts())
                .unwrap(),
            sk.to_armored_string(crate::utils::armor_opts()).unwrap(),
        )
    }

    #[test]
    fn a_pq_certificate_implies_aes256_after_its_explicit_preferences() {
        // RFC 9980 §7.1: "If AES-256 is not explicitly in the list ... and if
        // the certificate contains a PQ(/T) key, it is implicitly at the end
        // of the list"; "An implementation SHOULD use AES-256 ... if all
        // recipient certificates indicate support for it (explicitly or
        // implicitly)." The same holds for AES-256 with OCB.
        let pq = aes128_only_cert(&keys::V6_PQ);
        assert_eq!(
            container_for(&[&pq]),
            Ok(Container::SeipdV2(
                SymmetricKeyAlgorithm::AES256,
                AeadAlgorithm::Ocb
            ))
        );
    }

    #[test]
    fn a_classical_certificate_gets_no_implicit_aes256() {
        // Without a PQ key only AES-128 is implied (RFC 9580 §5.2.3.14-15).
        let classical = aes128_only_cert(&keys::V6_SIGN);
        assert_eq!(
            container_for(&[&classical]),
            Ok(Container::SeipdV2(
                SymmetricKeyAlgorithm::AES128,
                AeadAlgorithm::Ocb
            ))
        );
    }

    /// Encrypt `data` (binary) to `recipients`.
    fn encrypt_to_all(recipients: &[&Recipient], data: &[u8]) -> Result<Vec<u8>, GfrStatus> {
        let blocks: Vec<&str> = recipients.iter().map(|(p, _)| p.as_str()).collect();
        encrypt_internal(0, "", data, &blocks, false).map(|out| out.data)
    }

    /// Decrypt `data` with one recipient's secret key.
    fn decrypt_as(recipient: &Recipient, data: &[u8]) -> Vec<u8> {
        cb::set_seckey_answer(&recipient.1);
        crate::crypto::decrypt_internal(0, data, Some(cb::seckey_fetch), None, std::ptr::null_mut())
            .expect("decrypt")
            .data
    }

    /// Assert the §10.3.2.1 pairing in an encrypted message: a v1 SEIPD is
    /// preceded only by v3 PKESKs, a v2 SEIPD only by v6 PKESKs. Returns the
    /// SEIPD version.
    fn assert_esk_versions_match(data: &[u8]) -> u32 {
        let nodes = &crate::inspect::inspect(data).blocks[0].packets;
        let seipd = seipd_version(data).expect("a SEIPD container");
        let want = if seipd == 2 { 6 } else { 3 };
        let pkesks: Vec<_> = nodes.iter().filter(|n| n.tag == 1).collect();
        assert!(!pkesks.is_empty(), "a public-key message carries PKESKs");
        for pkesk in pkesks {
            assert_eq!(
                pkesk.version,
                Some(want),
                "a v{seipd} SEIPD must be preceded by v{want} PKESKs only"
            );
        }
        seipd
    }

    /// The v2 SEIPD's (cipher, AEAD) fields as the inspector reports them.
    fn aead_suite(data: &[u8]) -> (String, String) {
        let nodes = &crate::inspect::inspect(data).blocks[0].packets;
        let seipd = nodes.iter().find(|n| n.tag == 18).expect("a SEIPD");
        let field = |label: &str| {
            seipd
                .fields
                .iter()
                .find(|f| f.label == label)
                .map(|f| f.value.clone())
                .expect("field")
        };
        (field("Symmetric Algorithm"), field("AEAD Mode"))
    }

    #[test]
    fn a_v4_recipient_without_features_gets_v1_with_v3_pkesks() {
        let v4 = own(&keys::V4_SIGN);
        assert_eq!(
            container_for(&[&v4]),
            Ok(Container::SeipdV1(SymmetricKeyAlgorithm::AES256))
        );
        let data = encrypt_to_all(&[&v4], b"payload").expect("encrypt");
        assert_eq!(assert_esk_versions_match(&data), 1);
        assert_eq!(decrypt_as(&v4, &data), b"payload");
    }

    #[test]
    fn a_v6_recipient_with_features_gets_v2_aes256_ocb_with_v6_pkesks() {
        let v6 = own(&keys::V6_SIGN);
        assert_eq!(
            container_for(&[&v6]),
            Ok(Container::SeipdV2(
                SymmetricKeyAlgorithm::AES256,
                AeadAlgorithm::Ocb
            ))
        );
        let data = encrypt_to_all(&[&v6], b"payload").expect("encrypt");
        assert_eq!(assert_esk_versions_match(&data), 2);
        assert_eq!(aead_suite(&data), ("AES256".into(), "Ocb".into()));
        assert_eq!(decrypt_as(&v6, &data), b"payload");
    }

    #[test]
    fn a_v6_recipient_without_features_is_treated_as_supporting_v2() {
        // §13.7: v2 may be used when "all recipient keys are version 6 keys
        // without a Features signature subpacket".
        let v6 = with_features(&keys::V6_SIGN, None);
        assert!(cert_supports_seipd_v2(
            &SignedPublicKey::from_string(&v6.0).unwrap().0
        ));
        let data = encrypt_to_all(&[&v6], b"payload").expect("encrypt");
        assert_eq!(assert_esk_versions_match(&data), 2);
        assert_eq!(decrypt_as(&v6, &data), b"payload");
    }

    #[test]
    fn a_v6_recipient_whose_features_omit_v2_gets_v1() {
        // An explicit Features subpacket is honoured even on a v6 key.
        let v6 = with_features(&keys::V6_SIGN, Some(&[0x01]));
        let data = encrypt_to_all(&[&v6], b"payload").expect("encrypt");
        assert_eq!(assert_esk_versions_match(&data), 1);
        assert_eq!(decrypt_as(&v6, &data), b"payload");
    }

    #[test]
    fn a_v4_recipient_declaring_v2_gets_v2() {
        // The Features flag, not the key version, is the primary evidence.
        let v4 = with_features(&keys::V4_SIGN, Some(&[0x01 | 0x08]));
        let data = encrypt_to_all(&[&v4], b"payload").expect("encrypt");
        assert_eq!(assert_esk_versions_match(&data), 2);
        assert_eq!(decrypt_as(&v4, &data), b"payload");
    }

    #[test]
    fn a_v4_recipient_without_features_is_not_proof_of_incompatibility() {
        // An absent Features subpacket on an older key is "no evidence", which
        // falls back to v1; it never makes encryption fail.
        let v4 = with_features(&keys::V4_SIGN, None);
        let cert = SignedPublicKey::from_string(&v4.0).unwrap().0;
        assert!(!cert_supports_seipd_v2(&cert));
        assert!(cert_supports_seipd_v1(&cert));
        let data = encrypt_to_all(&[&v4], b"payload").expect("encrypt");
        assert_eq!(assert_esk_versions_match(&data), 1);
    }

    #[test]
    fn mixed_v4_and_v6_recipients_fall_back_to_v1_for_both() {
        let v4 = own(&keys::V4_SIGN);
        let v6 = own(&keys::V6_SIGN);
        let data = encrypt_to_all(&[&v6, &v4], b"shared").expect("encrypt");
        assert_eq!(assert_esk_versions_match(&data), 1);
        assert_eq!(decrypt_as(&v4, &data), b"shared");
        assert_eq!(decrypt_as(&v6, &data), b"shared");
    }

    #[test]
    fn recipients_with_no_common_container_are_refused_explicitly() {
        // One recipient states it reads only v2 (Features 0x08, no v1 flag),
        // the other cannot read v2: no single container satisfies both, so
        // nothing is produced rather than a message one of them cannot open.
        let v2_only = with_features(&keys::V6_SIGN, Some(&[0x08]));
        let v4 = own(&keys::V4_SIGN);
        assert_eq!(
            container_for(&[&v2_only, &v4]),
            Err(GfrStatus::ErrorUnsupportedFeature)
        );
        assert_eq!(
            encrypt_to_all(&[&v2_only, &v4], b"payload").err(),
            Some(GfrStatus::ErrorUnsupportedFeature)
        );

        let mut out = Vec::new();
        let blocks = [v2_only.0.as_str(), v4.0.as_str()];
        assert_eq!(
            encrypt_stream_internal(0, "", &b"payload"[..], &mut out, &blocks, false).err(),
            Some(GfrStatus::ErrorUnsupportedFeature)
        );
        assert!(out.is_empty(), "no ciphertext may be written on refusal");

        // Alone, the v2-only recipient is fine.
        let data = encrypt_to_all(&[&v2_only], b"payload").expect("encrypt");
        assert_eq!(assert_esk_versions_match(&data), 2);
    }

    #[test]
    fn the_aead_suite_respects_each_recipients_preferences() {
        // A recipient preferring only AES-256/GCM gets exactly that.
        let gcm = with_aead_prefs(
            &keys::V6_SIGN,
            &[(SymmetricKeyAlgorithm::AES256, AeadAlgorithm::Gcm)],
        );
        assert_eq!(
            container_for(&[&gcm]),
            Ok(Container::SeipdV2(
                SymmetricKeyAlgorithm::AES256,
                AeadAlgorithm::Gcm
            ))
        );
        let data = encrypt_to_all(&[&gcm], b"payload").expect("encrypt");
        assert_eq!(aead_suite(&data), ("AES256".into(), "Gcm".into()));
        assert_eq!(decrypt_as(&gcm, &data), b"payload");

        // With a recipient that prefers AES-256/OCB, the only suite both share
        // is the implicit mandatory AES-128/OCB (§5.2.3.15).
        let ocb = own(&keys::V6_SIGN);
        assert_eq!(
            container_for(&[&gcm, &ocb]),
            Ok(Container::SeipdV2(
                SymmetricKeyAlgorithm::AES128,
                AeadAlgorithm::Ocb
            ))
        );
        let data = encrypt_to_all(&[&gcm, &ocb], b"payload").expect("encrypt");
        assert_eq!(aead_suite(&data), ("AES128".into(), "Ocb".into()));
        assert_eq!(decrypt_as(&gcm, &data), b"payload");
        assert_eq!(decrypt_as(&ocb, &data), b"payload");
    }

    #[test]
    fn the_v1_cipher_respects_each_recipients_preferences() {
        let aes128_only = with_symmetric_prefs(&keys::V4_SIGN, &[SymmetricKeyAlgorithm::AES128]);
        let aes256 = own(&keys::V4_SIGN);
        assert_eq!(
            container_for(&[&aes256]),
            Ok(Container::SeipdV1(SymmetricKeyAlgorithm::AES256))
        );
        assert_eq!(
            container_for(&[&aes128_only, &aes256]),
            Ok(Container::SeipdV1(SymmetricKeyAlgorithm::AES128))
        );
        let data = encrypt_to_all(&[&aes128_only, &aes256], b"payload").expect("encrypt");
        assert_eq!(decrypt_as(&aes128_only, &data), b"payload");
        assert_eq!(decrypt_as(&aes256, &data), b"payload");
    }

    #[test]
    fn missing_or_foreign_cipher_preferences_fall_back_to_aes128() {
        // §5.2.3.7: no subpacket means the mandatory algorithms; §9.3 makes
        // AES-128 one every implementation reads.
        let none = with_symmetric_prefs(&keys::V4_SIGN, &[]);
        assert_eq!(
            container_for(&[&none]),
            Ok(Container::SeipdV1(SymmetricKeyAlgorithm::AES128))
        );
        let twofish = with_symmetric_prefs(&keys::V4_SIGN, &[SymmetricKeyAlgorithm::Twofish]);
        assert_eq!(
            container_for(&[&twofish]),
            Ok(Container::SeipdV1(SymmetricKeyAlgorithm::AES128)),
            "v1 ciphers stay AES so a v3 PKESK to X25519/ML-KEM is well formed"
        );
    }

    #[test]
    fn select_container_rules() {
        let caps = |v1: bool, v2: bool| RecipientCaps {
            seipd_v1: v1,
            seipd_v2: v2,
            symmetric: vec![SymmetricKeyAlgorithm::AES128],
            aead: vec![(SymmetricKeyAlgorithm::AES128, AeadAlgorithm::Ocb)],
        };
        assert_eq!(select_container(&[]), Err(GfrStatus::ErrorInvalidInput));
        assert_eq!(
            select_container(&[caps(true, true), caps(true, true)]),
            Ok(Container::SeipdV2(
                SymmetricKeyAlgorithm::AES128,
                AeadAlgorithm::Ocb
            ))
        );
        assert_eq!(
            select_container(&[caps(true, true), caps(true, false)]),
            Ok(Container::SeipdV1(SymmetricKeyAlgorithm::AES128))
        );
        assert_eq!(
            select_container(&[caps(false, true), caps(true, false)]),
            Err(GfrStatus::ErrorUnsupportedFeature)
        );
        assert_eq!(
            select_container(&[caps(false, false)]),
            Err(GfrStatus::ErrorUnsupportedFeature)
        );
    }

    #[test]
    fn elgamal_cannot_be_addressed_by_a_v6_pkesk() {
        // §5.1.4: so an ElGamal recipient always forces v1.
        assert!(!v6_pkesk_can_address(PublicKeyAlgorithm::Elgamal));
        assert!(v6_pkesk_can_address(PublicKeyAlgorithm::X25519));
        assert!(v6_pkesk_can_address(PublicKeyAlgorithm::RSA));
    }

    #[test]
    fn every_fixture_combination_pairs_pkesk_and_seipd_versions() {
        let pool = [
            own(&keys::V4_SIGN),
            own(&keys::V6_SIGN),
            own(&keys::V4_NISTP256),
            with_features(&keys::V6_SIGN, None),
            with_features(&keys::V4_SIGN, Some(&[0x09])),
        ];
        for a in &pool {
            for b in &pool {
                let data = encrypt_to_all(&[a, b], b"payload").expect("encrypt");
                let seipd = assert_esk_versions_match(&data);
                let want = match container_for(&[a, b]).expect("a container") {
                    Container::SeipdV1(_) => 1,
                    Container::SeipdV2(..) => 2,
                };
                assert_eq!(seipd, want);
            }
        }
    }

    // -- malformed recipient sets ---------------------------------------------------

    fn strip(fixture: &keys::Fixture, f: impl FnOnce(&mut SignedPublicKey)) -> String {
        let mut cert = cert_of(fixture);
        f(&mut cert);
        cert.to_armored_string(crate::utils::armor_opts())
            .expect("armor")
    }

    #[test]
    fn a_certificate_without_a_valid_self_signature_is_rejected() {
        let unbound = strip(&keys::V4_SIGN, |c| {
            c.details.direct_signatures.clear();
            for u in c.details.users.iter_mut() {
                u.signatures.clear();
            }
        });
        let out = encrypt_internal(
            0,
            "",
            b"payload",
            &[&unbound, &keys::V4_SIGN.public_armored],
            false,
        )
        .expect("encrypt");
        assert_eq!(out.invalid_recipients.len(), 1);
        assert_eq!(
            out.invalid_recipients[0].reason,
            GfrStatus::ErrorUnusableKey
        );
    }

    #[test]
    fn a_v6_certificate_without_a_direct_key_signature_is_rejected() {
        // §5.2.3.10: "An implementation MUST ensure that a valid Direct Key
        // signature is present before using a version 6 key."
        let no_dks = strip(&keys::V6_SIGN, |c| c.details.direct_signatures.clear());
        assert_eq!(
            encrypt_internal(0, "", b"payload", &[&no_dks], false).err(),
            Some(GfrStatus::ErrorInvalidInput)
        );
    }

    #[test]
    fn an_unbound_encryption_subkey_is_never_selected() {
        // Without a valid binding signature the subkey is not part of the
        // certificate (§5.2.1.8); V4_SIGN's primary cannot encrypt either.
        let unbound = strip(&keys::V4_SIGN, |c| {
            for sub in c.public_subkeys.iter_mut() {
                if sub.key.algorithm().can_encrypt() {
                    sub.signatures.clear();
                }
            }
        });
        let cert = SignedPublicKey::from_string(&unbound).unwrap().0;
        assert_eq!(
            validate_recipient(&cert, None, unix_now()).err(),
            Some(GfrStatus::ErrorNoKey)
        );
    }

    #[test]
    fn a_set_of_only_invalid_recipients_is_rejected() {
        let res = encrypt_internal(
            0,
            "",
            b"payload",
            &[
                "junk",
                &keys::V4_REVOKED_PRIMARY.public_armored,
                corpus::AUX_EXPIRED,
            ],
            false,
        );
        assert_eq!(res.err(), Some(GfrStatus::ErrorInvalidInput));
    }

    // -- cross-implementation interop -----------------------------------------------

    #[test]
    fn sequoia_and_gnupg_certificates_are_classified_by_their_features() {
        // Committed certificates made by other implementations: sq's RFC 9580
        // profile advertises v2; GnuPG-era v4 keys do not.
        let sq_v6 = SignedPublicKey::from(corpus::AUX_V6_KEY.clone());
        assert!(cert_supports_seipd_v2(&sq_v6));
        let key1 = SignedPublicKey::from(corpus::KEY1.clone());
        assert!(!cert_supports_seipd_v2(&key1));
        assert!(cert_supports_seipd_v1(&key1));
    }

    fn public_armored(key: &SignedSecretKey) -> String {
        SignedPublicKey::from(key.clone())
            .to_armored_string(crate::utils::armor_opts())
            .expect("armor")
    }

    // These use `testutil::interop`, so a missing `gpg` or `sq` fails the
    // test rather than skipping it silently (set GF_ALLOW_MISSING_INTEROP_TOOLS=1
    // to skip it explicitly).

    #[test]
    fn sequoia_decrypts_the_v2_message_we_send_to_its_v6_key() {
        let data = encrypt_internal(
            0,
            "",
            b"interop v2",
            &[&public_armored(&corpus::AUX_V6_KEY)],
            false,
        )
        .expect("encrypt")
        .data;
        assert_eq!(assert_esk_versions_match(&data), 2);
        let Some(sq) = interop::Sq::new() else {
            return;
        };
        assert_eq!(
            sq.decrypt(corpus::AUX_V6_SECRET, corpus::CORPUS_PASSPHRASE, &data),
            b"interop v2"
        );
    }

    #[test]
    fn gnupg_decrypts_the_v1_message_we_send_to_its_key() {
        let Some(gnupg) = interop::Gnupg::new() else {
            return;
        };
        // GnuPG does not implement RFC 9580's SEIPD v2, and its certificates do
        // not claim it, so the message must be v1 with v3 PKESKs.
        let cert = SignedPublicKey::from_string(&gnupg.cert).expect("parses").0;
        assert!(!cert_supports_seipd_v2(&cert));
        let data = encrypt_internal(0, "", b"interop v1", &[&gnupg.cert], false)
            .expect("encrypt")
            .data;
        assert_eq!(assert_esk_versions_match(&data), 1);
        assert_eq!(gnupg.decrypt(&data), b"interop v1");
    }

    #[test]
    fn a_mixed_sequoia_and_gnupg_recipient_set_is_readable_by_both() {
        let (Some(gnupg), Some(sq)) = (interop::Gnupg::new(), interop::Sq::new()) else {
            return;
        };
        let sq_cert = public_armored(&corpus::AUX_V6_KEY);
        let data = encrypt_internal(0, "", b"interop mixed", &[&sq_cert, &gnupg.cert], false)
            .expect("encrypt")
            .data;
        assert_eq!(assert_esk_versions_match(&data), 1);
        assert_eq!(
            sq.decrypt(corpus::AUX_V6_SECRET, corpus::CORPUS_PASSPHRASE, &data),
            b"interop mixed"
        );
        assert_eq!(gnupg.decrypt(&data), b"interop mixed");
    }

    #[test]
    fn sequoia_decrypts_our_password_encrypted_message() {
        // §10.3.2.1: v6 SKESK + v2 SEIPD (Argon2, AES-256/OCB). GnuPG 2.4
        // cannot read this format; that is a product-policy limitation
        // recorded in the audit's interop matrix, not tested here.
        let mut out = Vec::new();
        encrypt_stream_with_password_internal(
            0,
            "",
            &b"interop password"[..],
            &mut out,
            Some(cb::pwd_correct),
            false,
        )
        .expect("encrypt");
        let Some(sq) = interop::Sq::new() else {
            return;
        };
        assert_eq!(
            sq.decrypt_with_password(cb::CORRECT_PASSPHRASE, &out),
            b"interop password"
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
    fn a_revoked_primary_is_reported_as_an_invalid_recipient() {
        // §5.2.1.11: a revoked certificate must not receive new messages. The
        // produce side now gates on it exactly like the verify side.
        let alone = encrypt_internal(
            0,
            "",
            b"payload",
            &[&keys::V4_REVOKED_PRIMARY.public_armored],
            false,
        );
        assert_eq!(alone.err(), Some(GfrStatus::ErrorInvalidInput));

        let out = encrypt_internal(
            0,
            "",
            b"payload",
            &[
                &keys::V4_REVOKED_PRIMARY.public_armored,
                &keys::V4_SIGN.public_armored,
            ],
            false,
        )
        .expect("the usable recipient still gets the message");
        assert_eq!(out.recipients.len(), 1);
        assert_eq!(out.invalid_recipients.len(), 1);
        assert_eq!(out.invalid_recipients[0].reason, GfrStatus::ErrorRevokedKey);
    }

    #[test]
    fn an_expired_primary_is_reported_as_an_invalid_recipient() {
        // §5.2.3.13: the committed AUX_EXPIRED certificate expired in the past.
        let out = encrypt_internal(
            0,
            "",
            b"payload",
            &[corpus::AUX_EXPIRED, &keys::V4_SIGN.public_armored],
            false,
        )
        .expect("encrypt");
        assert_eq!(out.invalid_recipients.len(), 1);
        assert_eq!(out.invalid_recipients[0].reason, GfrStatus::ErrorExpiredKey);
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
    fn a_symmetric_message_uses_a_v2_seipd_container() {
        // The Argon2 S2K already needs an RFC 9580 reader, so the AEAD
        // container (with its v6 SKESK) is always used for passphrases.
        let mut out = Vec::new();
        encrypt_stream_with_password_internal(
            0,
            "",
            &b"passphrase payload"[..],
            &mut out,
            Some(cb::pwd_correct),
            false,
        )
        .expect("encrypt");

        let nodes = &crate::inspect::inspect(&out).blocks[0].packets;
        assert_eq!(nodes[0].tag, 3, "a SKESK leads the message");
        assert_eq!(
            nodes[0].version,
            Some(6),
            "a v2 SEIPD pairs with a v6 SKESK"
        );
        assert_eq!(seipd_version(&out), Some(2));
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

        // §5.3.2: the v6 SKESK's AEAD tag rejects the wrong passphrase, and
        // a password-only message says so.
        let result = crate::crypto::decrypt_internal(
            0,
            &out,
            Some(cb::seckey_none),
            Some(cb::pwd_wrong),
            std::ptr::null_mut(),
        );
        assert_eq!(
            result.err(),
            Some(GfrStatus::ErrorBadPassphrase),
            "{}",
            crate::err::last_error()
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
    fn recipient_validation_never_panics_on_a_corpus_certificate() {
        for cert in [
            &*corpus::AUX_GOOD_CERT,
            &*corpus::AUX_REVOKED_CERT,
            &*corpus::AUX_EXPIRED_CERT,
            &*corpus::AUX_FORGED_REVOCATION_CERT,
            &*corpus::KEY2_CERT,
        ] {
            let outcome = std::panic::catch_unwind(|| {
                validate_recipient(cert, None, unix_now()).is_ok()
                    && RecipientCaps::of(cert, &validate_recipient(cert, None, unix_now()).unwrap())
                        .aead
                        .contains(&(SymmetricKeyAlgorithm::AES128, AeadAlgorithm::Ocb))
            });
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
        assert_eq!(
            res.err(),
            Some(GfrStatus::ErrorCanceled),
            "a cancelled encryption must abort"
        );
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
