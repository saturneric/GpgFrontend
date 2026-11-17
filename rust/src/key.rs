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

//! Key block manipulation: metadata extraction, merging, password change,
//! subkey deletion/revocation, and revocation certificate generation/import.
//!
//! All functions operate on armored OpenPGP key blocks (strings) and return
//! updated armored blocks. No key material is stored; the caller is responsible
//! for persisting the returned blocks.

use crate::cache::{PASSWORD_CACHE, PasswordCachePolicy};
use crate::err::IntoGfrResult;
use crate::keygen::GeneratedKeys;
use crate::types::{
    GfrKeyAlgo, GfrOpenPGPKeyVersion, GfrPasswordFetchCb, GfrRevocationCode, GfrStatus,
};
#[cfg(test)]
use crate::utils::armor_opts;
use crate::utils::{
    PassphraseStateInternal, build_revocation_reason_subpacket, choose_template_self_sig,
    determine_algo, extract_key_length, fetch_password_with_cache, is_property_self_sig, is_v4_era,
    key_armor_opts, password_from_zeroizing_bytes, push_issuer, refresh_reissued_config,
};
use pgp::armor::{self, BlockType};
use pgp::composed::{SignedKeyDetails, SignedPublicSubKey, SignedSecretSubKey};
use pgp::packet::{
    Packet, PacketHeader, PublicKey, PublicSubkey, RevocationCode, SignatureConfig, SignatureType,
    Subpacket, SubpacketData,
};
use pgp::types::{
    Duration, EcdhPublicParams, KeyDetails, KeyVersion, Password, PublicParams, SecretParams,
    SignedUser, Tag,
};
use pgp::{
    composed::{Deserializable, SignedPublicKey, SignedSecretKey},
    packet::Signature,
    ser::Serialize,
};
use std::collections::{BTreeMap, HashMap, HashSet};
use std::io::{self, BufReader, Cursor, Read};
use zeroize::Zeroizing;

/// A single user ID extracted from a key block.
pub struct ExtractUserId {
    pub user_id: String,
    pub is_primary: bool,
    pub is_revoked: bool,
}

/// Metadata for a single subkey within a key block.
pub struct ExtractedSubkey {
    pub ver: GfrOpenPGPKeyVersion,
    pub fpr: String,
    pub key_id: String,
    pub algo: GfrKeyAlgo,
    pub key_length: u32,
    pub created_at: u32,
    /// Absolute expiration time (Unix epoch, seconds); `0` means never expires.
    pub expires_at: u32,
    pub has_secret: bool,
    pub is_revoked: bool,
    pub can_sign: bool,
    pub can_encrypt: bool,
    pub can_certify: bool,
    pub can_auth: bool,
}

/// Full metadata for a primary key, including its subkeys and user IDs.
///
/// `secret_key_block` is `None` for public-only keys. Both armored blocks
/// are always re-exported from the parsed key objects (not taken verbatim from
/// the input) to ensure they are canonical.
pub struct ExtractedMetadata {
    pub ver: GfrOpenPGPKeyVersion,
    pub fpr: String,
    pub key_id: String,
    pub algo: GfrKeyAlgo,
    pub key_length: u32,
    pub created_at: u32,
    /// Absolute expiration time (Unix epoch, seconds); `0` means never expires.
    pub expires_at: u32,
    pub has_secret: bool,
    pub is_revoked: bool,
    pub can_sign: bool,
    pub can_encrypt: bool,
    pub can_auth: bool,
    pub can_certify: bool,
    pub user_ids: Vec<ExtractUserId>,
    pub subkeys: Vec<ExtractedSubkey>,
    pub public_key_block: String,
    pub secret_key_block: Option<Zeroizing<String>>,
}

/// True if `user` carries a *cryptographically valid* certification-revocation
/// (0x30) issued by `primary` (RFC 9580 §5.2.3.32).
///
/// The revocation is verified, not merely matched by its Issuer subpacket, so a
/// fabricated revocation smuggled into a foreign certificate is never honored,
/// and a genuine self-revocation always is. See [[project_rpgp_rfc9580_verify_gaps]].
pub(crate) fn is_user_id_revoked(primary: &PublicKey, user: &SignedUser) -> bool {
    user_id_revoked_at(primary, user, None)
}

/// True if `user` carries a valid certification revocation (0x30) made at or
/// before `at` (`None`: at any time). A User ID revoked later was still
/// certified when an earlier signature was made.
fn user_id_revoked_at(primary: &PublicKey, user: &SignedUser, at: Option<u32>) -> bool {
    user.signatures.iter().any(|sig| {
        matches!(sig.typ(), Some(SignatureType::CertRevocation))
            && at.is_none_or(|t| created_secs(sig) <= t)
            && sig
                .verify_certification(primary, Tag::UserId, &user.id)
                .is_ok()
    })
}

/// True if `signatures` contains a cryptographically valid key-revocation (0x20)
/// issued by `primary` (RFC 9580 §5.2.3.31). Verified rather than issuer-matched,
/// so fabricated revocation subpackets in a foreign certificate cannot be trusted.
pub(crate) fn is_primary_key_revoked(primary: &PublicKey, signatures: &[Signature]) -> bool {
    signatures.iter().any(|sig| {
        matches!(sig.typ(), Some(SignatureType::KeyRevocation)) && sig.verify_key(primary).is_ok()
    })
}

/// True if `sig` names `primary` as its issuer, by Issuer Fingerprint or (as
/// legacy v4 signatures do) by Issuer Key ID alone. Says nothing about
/// validity: used to retire every self-signature a re-issued one supersedes.
pub(crate) fn names_primary_as_issuer(sig: &Signature, primary: &PublicKey) -> bool {
    let fpr = primary.fingerprint();
    let key_id = primary.legacy_key_id();
    sig.issuer_fingerprint().iter().any(|f| **f == fpr)
        || sig.issuer_key_id().iter().any(|id| **id == key_id)
}

/// The template for re-issuing `user`'s self-certification: its newest
/// *verified* property self-certification. A re-issued signature is signed
/// anew by the keyholder, so only a signature the keyholder verifiably made
/// may lend it subpackets; a forged or merged-in one never can.
pub(crate) fn uid_reissue_template<'a>(
    primary: &PublicKey,
    user: &'a SignedUser,
) -> Option<&'a Signature> {
    choose_template_self_sig(&verified_uid_self_sigs(primary, user))
}

/// [`uid_reissue_template`] for a Direct Key signature (0x1F).
fn direct_reissue_template<'a>(
    primary: &PublicKey,
    signatures: &'a [Signature],
) -> Option<&'a Signature> {
    let sigs: Vec<&Signature> = verified_primary_self_sigs(primary, signatures)
        .into_iter()
        .filter(|sig| matches!(sig.typ(), Some(SignatureType::Key)))
        .collect();
    choose_template_self_sig(&sigs)
}

/// [`uid_reissue_template`] for a subkey binding (0x18).
fn binding_reissue_template<'a>(
    primary: &PublicKey,
    subkey: &PublicSubkey,
    signatures: &'a [Signature],
) -> Option<&'a Signature> {
    let sigs: Vec<&Signature> = verified_subkey_self_sigs(primary, subkey, signatures)
        .into_iter()
        .filter(|sig| matches!(sig.typ(), Some(SignatureType::SubkeyBinding)))
        .collect();
    choose_template_self_sig(&sigs)
}

/// Collect the cryptographically valid direct-key self-signatures (0x1F) and
/// key-revocations (0x20) from `signatures`, verified under `primary`.
///
/// Only these may be trusted for capability, expiry, and revocation decisions on
/// the primary key: an unverified self-signature in an imported certificate may
/// carry fabricated key-flags / expiry / revocation subpackets (RFC 9580 §5.2.3).
fn verified_primary_self_sigs<'a>(
    primary: &PublicKey,
    signatures: &'a [Signature],
) -> Vec<&'a Signature> {
    signatures
        .iter()
        .filter(|sig| {
            sig.verify_key(primary).is_ok()
                && certification_hash_acceptable(sig)
                && evaluable_self_sig(sig)
        })
        .collect()
}

/// RFC 9580 §5.2.3.7: a self-signature carrying a critical subpacket GF does
/// not understand (see `crypto::has_critical_unsupported_subpacket`) is in
/// error and speaks for nothing. Revocations are exempt: honouring one can
/// only withdraw trust.
fn evaluable_self_sig(sig: &Signature) -> bool {
    matches!(
        sig.typ(),
        Some(
            SignatureType::KeyRevocation
                | SignatureType::SubkeyRevocation
                | SignatureType::CertRevocation
        )
    ) || !crate::crypto::has_critical_unsupported_subpacket(sig)
}

/// Collect the cryptographically valid self-certifications (0x10–0x13) and
/// certification-revocations (0x30) for `user`, verified under `primary`.
fn verified_uid_self_sigs<'a>(primary: &PublicKey, user: &'a SignedUser) -> Vec<&'a Signature> {
    user.signatures
        .iter()
        .filter(|sig| {
            sig.verify_certification(primary, Tag::UserId, &user.id)
                .is_ok()
                && certification_hash_acceptable(sig)
                && evaluable_self_sig(sig)
        })
        .collect()
}

/// Collect the cryptographically valid binding (0x18) and revocation (0x28)
/// signatures of `subkey`, verified under `primary`.
fn verified_subkey_self_sigs<'a>(
    primary: &PublicKey,
    subkey: &PublicSubkey,
    signatures: &'a [Signature],
) -> Vec<&'a Signature> {
    signatures
        .iter()
        .filter(|sig| {
            sig.verify_subkey_binding(primary, subkey).is_ok()
                && certification_hash_acceptable(sig)
                && evaluable_self_sig(sig)
                && !pq_binding_hash_forbidden(subkey, sig)
        })
        .collect()
}

/// RFC 9980 §7.2: a subkey binding (0x18) over a post-quantum subkey "MUST
/// NOT be made with MD5, SHA-1, or RIPEMD-160", and a receiver "MUST treat
/// such a signature as invalid": no date cutoff applies, unlike
/// [`certification_hash_acceptable`].
fn pq_binding_hash_forbidden(subkey: &PublicSubkey, sig: &Signature) -> bool {
    use pgp::crypto::hash::HashAlgorithm;
    subkey.algorithm().is_pqc()
        && sig.typ() == Some(SignatureType::SubkeyBinding)
        && matches!(
            sig.hash_alg(),
            Some(HashAlgorithm::Md5 | HashAlgorithm::Sha1 | HashAlgorithm::Ripemd160)
        )
}

/// Certifications (self-signatures, bindings, back-signatures) made with SHA-1
/// or RIPEMD-160 count only if created before this instant, 2023-02-01T00:00Z
/// (Sequoia's standard-policy cutoff). RFC 9580 §9.5: "MUST NOT validate any
/// recent signature that depends on MD5, SHA-1, or RIPEMD-160"; "recent" is
/// left to the implementation. MD5 certifications never count.
pub(crate) const WEAK_CERTIFICATION_HASH_CUTOFF: u32 = 1_675_209_600;

/// True if `sig`'s hash is acceptable for a certification (RFC 9580 §9.5, see
/// [`WEAK_CERTIFICATION_HASH_CUTOFF`]). Revocations are deliberately *not*
/// filtered by this: honoring a weak revocation can only withdraw trust.
pub(crate) fn certification_hash_acceptable(sig: &Signature) -> bool {
    use pgp::crypto::hash::HashAlgorithm;
    match sig.hash_alg() {
        None | Some(HashAlgorithm::None | HashAlgorithm::Md5) => false,
        Some(HashAlgorithm::Sha1 | HashAlgorithm::Ripemd160) => {
            created_secs(sig) < WEAK_CERTIFICATION_HASH_CUTOFF
        }
        Some(_) => true,
    }
}

/// RFC 9580 §12.4: "MUST NOT encrypt, sign, or verify using RSA keys of a
/// size less than 2048 bits."
pub(crate) fn rsa_key_too_small(params: &PublicParams) -> bool {
    matches!(params, PublicParams::RSA(_)) && extract_key_length(params).unwrap_or(0) < 2048
}

/// A key RFC 9580 forbids using at all: RSA under 2048 bits (§12.4) or DSA
/// (§12.5: "MUST NOT sign or verify using DSA keys"). A primary key of
/// either kind poisons its certificate, whose self-signatures could only be
/// checked by verifying with it.
pub(crate) fn key_forbidden(params: &PublicParams) -> bool {
    rsa_key_too_small(params) || matches!(params, PublicParams::DSA(_))
}

/// Refuse an edit that would make `primary` sign a new self-signature when
/// it is a forbidden key ([`key_forbidden`]). Revocations are not routed
/// through this, so an old key can still be retired.
pub(crate) fn refuse_forbidden_signer(params: &PublicParams) -> Result<(), GfrStatus> {
    if key_forbidden(params) {
        crate::err::set_last_error(
            "a DSA or sub-2048-bit RSA primary key may not sign (RFC 9580 §12.4, §12.5); it can only be revoked",
        );
        return Err(GfrStatus::ErrorUnusableKey);
    }
    Ok(())
}

fn created_secs(sig: &Signature) -> u32 {
    sig.created().map(|t| t.as_secs()).unwrap_or(0)
}

/// The newest of `sigs` by creation time.
fn newest_sig<'a, I>(sigs: I) -> Option<&'a Signature>
where
    I: IntoIterator<Item = &'a Signature>,
{
    sigs.into_iter().max_by_key(|sig| created_secs(sig))
}

/// The self-signature in force at `at` among the valid self-signatures `sigs`
/// of one object: the newest created at or before `at` (RFC 9580 §5.2.3.10:
/// "MUST select the most recent valid self-signature and ignore all other
/// self-signatures"). `None` means now.
///
/// For a past instant with no earlier self-signature the oldest one is used:
/// tools commonly drop superseded self-signatures when they rewrite one (GF's
/// own expiry update does), and that must not orphan every older signature.
///
/// A self-signature past its own Signature Expiration Time at that instant is
/// no longer valid (§5.2.3.18), so it yields `None` rather than letting an
/// older one speak again.
fn sig_in_force<'a>(sigs: Vec<&'a Signature>, at: Option<u32>) -> Option<&'a Signature> {
    let chosen = match at {
        None => newest_sig(sigs),
        Some(t) => newest_sig(sigs.iter().copied().filter(|sig| created_secs(sig) <= t))
            .or_else(|| sigs.into_iter().min_by_key(|sig| created_secs(sig))),
    };
    let t = at.unwrap_or_else(|| pgp::types::Timestamp::now().as_secs());
    chosen.filter(|sig| sig_alive_at(sig, t))
}

/// False once `sig`'s Signature Expiration Time (§5.2.3.18) has passed at
/// `t`. Zero or absent means it never expires.
fn sig_alive_at(sig: &Signature, t: u32) -> bool {
    match sig.signature_expiration_time().map(|d| d.as_secs()) {
        Some(secs) if secs != 0 => created_secs(sig).saturating_add(secs) > t,
        _ => true,
    }
}

/// The self-signature that speaks for the primary key at `at` (`None`: now),
/// or `None` if the key carries no valid one.
///
/// * v6: the Direct Key signature (0x1F). RFC 9580 §5.2.3.10: "An
///   implementation MUST ensure that a valid Direct Key signature is present
///   before using a version 6 key."
/// * v4: the self-certification of the primary User ID, else of any
///   non-revoked User ID, else a Direct Key signature. The Primary User ID
///   flag is read only from valid self-certifications, newest first
///   (§5.2.3.27).
fn primary_sig_in_force<'a>(
    primary: &PublicKey,
    details: &'a SignedKeyDetails,
    at: Option<u32>,
) -> Option<&'a Signature> {
    let direct = sig_in_force(
        verified_primary_self_sigs(primary, &details.direct_signatures)
            .into_iter()
            .filter(|sig| matches!(sig.typ(), Some(SignatureType::Key)))
            .collect(),
        at,
    );
    if primary.version() == KeyVersion::V6 {
        return direct;
    }
    uid_certification_in_force(primary, details, at).or(direct)
}

/// The User ID self-certification that speaks for the key at `at` (`None`:
/// now): that of the primary User ID, else of the newest certified
/// non-revoked one. The Primary User ID flag is read only from valid
/// self-certifications, newest first (§5.2.3.27).
pub(crate) fn uid_certification_in_force<'a>(
    primary: &PublicKey,
    details: &'a SignedKeyDetails,
    at: Option<u32>,
) -> Option<&'a Signature> {
    let uid_sigs: Vec<&Signature> = details
        .users
        .iter()
        .filter(|user| !user_id_revoked_at(primary, user, at))
        .filter_map(|user| {
            sig_in_force(
                verified_uid_self_sigs(primary, user)
                    .into_iter()
                    .filter(|sig| !matches!(sig.typ(), Some(SignatureType::CertRevocation)))
                    .collect(),
                at,
            )
        })
        .collect();

    newest_sig(uid_sigs.iter().copied().filter(|sig| sig.is_primary()))
        .or_else(|| newest_sig(uid_sigs.iter().copied()))
}

/// The cryptographically valid self-signature whose subpackets (Features,
/// preferred algorithms, Key Flags) speak for `cert`'s primary key now, or
/// `None` if the certificate carries no valid one and so must not be used.
pub(crate) fn preference_self_signature(cert: &SignedPublicKey) -> Option<&Signature> {
    primary_sig_in_force(&cert.primary_key, &cert.details, None)
}

/// The binding signature (0x18) of `subkey` in force at `at` (`None`: now),
/// or `None` if the subkey is not validly bound to `primary`.
fn subkey_binding_in_force<'a>(
    primary: &PublicKey,
    subkey: &PublicSubkey,
    signatures: &'a [Signature],
    at: Option<u32>,
) -> Option<&'a Signature> {
    sig_in_force(
        verified_subkey_self_sigs(primary, subkey, signatures)
            .into_iter()
            .filter(|sig| matches!(sig.typ(), Some(SignatureType::SubkeyBinding)))
            .collect(),
        at,
    )
}

/// The current cryptographically valid binding signature (0x18) of `subkey`,
/// or `None` if the subkey is not validly bound to `primary`.
pub(crate) fn subkey_binding_signature<'a>(
    primary: &PublicKey,
    subkey: &PublicSubkey,
    signatures: &'a [Signature],
) -> Option<&'a Signature> {
    subkey_binding_in_force(primary, subkey, signatures, None)
}

/// `caps` (sign, encrypt, auth, certify) from the Key Flags, narrowed to
/// what the engine will actually do with a key of `params` in a certificate
/// whose primary has `primary_params`: a forbidden key ([`key_forbidden`])
/// neither signs nor authenticates, a forbidden primary poisons the whole
/// certificate, and an algorithm the engine cannot encrypt to is not offered
/// for encryption. The GUI offers keys by these flags.
fn usable_capabilities(
    caps: (bool, bool, bool, bool),
    params: &PublicParams,
    primary_params: &PublicParams,
) -> (bool, bool, bool, bool) {
    if key_forbidden(primary_params) {
        return (false, false, false, false);
    }
    let (sign, encrypt, auth, certify) = caps;
    let may_sign = !key_forbidden(params);
    (
        sign && may_sign,
        encrypt && crate::crypto::engine_can_encrypt_to(params),
        auth && may_sign,
        certify && may_sign,
    )
}

fn extract_capabilities<'a, I>(signatures: I) -> (bool, bool, bool, bool)
where
    I: IntoIterator<Item = &'a Signature>,
{
    let mut can_sign = false;
    let mut can_encrypt = false;
    let mut can_auth = false;
    let mut can_certify = false;

    for sig in signatures {
        let flags = sig.key_flags();

        if flags.sign() {
            can_sign = true;
        }

        if flags.encrypt_comms() || flags.encrypt_storage() {
            can_encrypt = true;
        }

        if flags.authentication() {
            can_auth = true;
        }

        if flags.certify() {
            can_certify = true;
        }
    }

    (can_sign, can_encrypt, can_auth, can_certify)
}

/// Absolute expiration time (Unix epoch, seconds) set by the governing
/// self-signature `sig` of a component created at `created_at`; `0` means
/// never. The Key Expiration Time is a duration from creation, and its absence
/// or a zero value means the key never expires (RFC 9580 §5.2.3.13) — even if
/// an older, superseded self-signature set one.
fn expiry_of(sig: Option<&Signature>, created_at: u32) -> u32 {
    match sig.and_then(|sig| sig.key_expiration_time()) {
        Some(dur) if dur.as_secs() != 0 => created_at.saturating_add(dur.as_secs()),
        _ => 0,
    }
}

/// True if `signatures` contains a cryptographically valid subkey-revocation
/// (0x28) of `subkey`, issued by `primary` (RFC 9580 §5.2.1.12). Verified rather
/// than issuer-matched, so a fabricated subkey revocation is never honored.
pub(crate) fn is_subkey_revoked(
    primary: &PublicKey,
    subkey: &PublicSubkey,
    signatures: &[Signature],
) -> bool {
    signatures.iter().any(|sig| {
        matches!(sig.typ(), Some(SignatureType::SubkeyRevocation))
            && sig.verify_subkey_binding(primary, subkey).is_ok()
    })
}

/// Absolute expiration time (Unix epoch, seconds) of a certificate's primary
/// key, or `0` if it never expires, from the self-signature in force now.
pub(crate) fn primary_key_expires_at(cert: &SignedPublicKey) -> u32 {
    expiry_of(
        primary_sig_in_force(&cert.primary_key, &cert.details, None),
        cert.primary_key.created_at().as_secs(),
    )
}

/// Absolute expiration time (Unix epoch, seconds) of a public subkey, from its
/// binding signature in force now, or `0` if it never expires.
pub(crate) fn subkey_expires_at(
    primary: &PublicKey,
    subkey: &PublicSubkey,
    signatures: &[Signature],
) -> u32 {
    expiry_of(
        subkey_binding_signature(primary, subkey, signatures),
        subkey.created_at().as_secs(),
    )
}

/// How a key stands with respect to its verified revocations (RFC 9580
/// §5.2.3.31).
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub(crate) enum RevocationState {
    NotRevoked,
    /// Superseded or retired: "old signatures are still valid", so the key is
    /// only invalid from `since` (the earliest such revocation) on.
    Soft {
        since: u32,
    },
    /// Compromised, no reason, or any other code: "all signatures created by
    /// that key are suspect".
    Hard,
}

impl RevocationState {
    fn of<'a, I>(revocations: I) -> Self
    where
        I: IntoIterator<Item = &'a Signature>,
    {
        let mut state = RevocationState::NotRevoked;
        for sig in revocations {
            match sig.revocation_reason_code() {
                Some(RevocationCode::KeySuperseded | RevocationCode::KeyRetired) => {
                    let since = created_secs(sig);
                    state = match state {
                        RevocationState::Soft { since: s } => RevocationState::Soft {
                            since: s.min(since),
                        },
                        _ => RevocationState::Soft { since },
                    };
                }
                _ => return RevocationState::Hard,
            }
        }
        state
    }

    /// True if the key may not vouch for anything made at `t`.
    pub(crate) fn revoked_at(self, t: u32) -> bool {
        match self {
            RevocationState::NotRevoked => false,
            RevocationState::Soft { since } => since <= t,
            RevocationState::Hard => true,
        }
    }
}

/// Revocation state of the primary key, from its verified key revocations
/// (0x20) wherever they are stored.
pub(crate) fn primary_revocation_state(
    primary: &PublicKey,
    details: &SignedKeyDetails,
) -> RevocationState {
    RevocationState::of(
        details
            .revocation_signatures
            .iter()
            .chain(details.direct_signatures.iter())
            .filter(|sig| {
                matches!(sig.typ(), Some(SignatureType::KeyRevocation))
                    && sig.verify_key(primary).is_ok()
            }),
    )
}

/// Revocation state of `subkey`, from its verified subkey revocations (0x28).
pub(crate) fn subkey_revocation_state(
    primary: &PublicKey,
    subkey: &PublicSubkey,
    signatures: &[Signature],
) -> RevocationState {
    RevocationState::of(signatures.iter().filter(|sig| {
        matches!(sig.typ(), Some(SignatureType::SubkeyRevocation))
            && sig.verify_subkey_binding(primary, subkey).is_ok()
    }))
}

/// Whether the self-signature `sig` lets a key of `algo` make signatures:
/// its Key Flags must include 0x02 (RFC 9580 §5.2.3.29). A self-signature
/// without Key Flags — common on legacy keys — defers to the algorithm.
fn flags_allow_signing(
    sig: Option<&Signature>,
    algo: pgp::crypto::public_key::PublicKeyAlgorithm,
) -> bool {
    let has_flags = sig.and_then(|sig| sig.config()).is_some_and(|config| {
        config
            .hashed_subpackets()
            .any(|p| matches!(p.data, SubpacketData::KeyFlags(_)))
    });
    match sig {
        Some(sig) if has_flags => sig.key_flags().sign(),
        _ => algo.can_sign(),
    }
}

/// Why a key that cryptographically verified a signature does — or does not
/// — vouch for it. Ordered so the best verdict across a certificate's keys is
/// the maximum.
#[derive(Clone, Copy, Debug, PartialEq, Eq, PartialOrd, Ord)]
pub(crate) enum KeyVerdict {
    /// No key of the certificate verified the signature.
    NoMatch,
    /// Verified, but the certificate does not authorize that key to sign
    /// (unbound, no back-signature, no sign flag, weak certification, RSA
    /// under 2048 bits, v6 without a Direct Key signature).
    NotAuthorized,
    /// Verified, but the key had expired when the signature was made.
    Expired,
    /// Verified, but the key was revoked as of the signature.
    Revoked,
    Authorized,
}

/// May `cert`'s primary key vouch for a signature made at `t`?
pub(crate) fn primary_signing_verdict(
    primary: &PublicKey,
    details: &SignedKeyDetails,
    t: u32,
) -> KeyVerdict {
    let verdict = primary_verdict(primary, details, t);
    if verdict != KeyVerdict::Authorized {
        return verdict;
    }
    let sig = primary_sig_in_force(primary, details, Some(t));
    if !flags_allow_signing(sig, primary.algorithm()) {
        return KeyVerdict::NotAuthorized;
    }
    KeyVerdict::Authorized
}

/// The primary-key conditions every key of the certificate inherits at `t`.
fn primary_verdict(primary: &PublicKey, details: &SignedKeyDetails, t: u32) -> KeyVerdict {
    if key_forbidden(primary.public_params()) || t < primary.created_at().as_secs() {
        return KeyVerdict::NotAuthorized;
    }
    if primary_revocation_state(primary, details).revoked_at(t) {
        return KeyVerdict::Revoked;
    }
    // v6: §5.2.3.10 requires a valid Direct Key signature. v4: without a
    // valid self-signature in force nothing certifies the key's usage, and
    // dropping one must not also drop its expiry and flags.
    let Some(sig) = primary_sig_in_force(primary, details, Some(t)) else {
        return KeyVerdict::NotAuthorized;
    };
    let sig = Some(sig);
    let expires = expiry_of(sig, primary.created_at().as_secs());
    if expires != 0 && expires <= t {
        return KeyVerdict::Expired;
    }
    KeyVerdict::Authorized
}

/// May `subkey` vouch for a signature made at `t`? Besides the primary's own
/// conditions, the subkey needs a binding (0x18) in force at `t` with the
/// signing flag and an embedded, valid Primary Key Binding (0x19)
/// back-signature (RFC 9580 §5.2.1.8, §10.1.5), and must be neither revoked
/// nor expired as of `t`.
pub(crate) fn subkey_signing_verdict(
    primary: &PublicKey,
    details: &SignedKeyDetails,
    subkey: &PublicSubkey,
    signatures: &[Signature],
    t: u32,
) -> KeyVerdict {
    let verdict = primary_verdict(primary, details, t);
    if verdict != KeyVerdict::Authorized {
        return verdict;
    }
    if key_forbidden(subkey.public_params()) || t < subkey.created_at().as_secs() {
        return KeyVerdict::NotAuthorized;
    }
    if subkey_revocation_state(primary, subkey, signatures).revoked_at(t) {
        return KeyVerdict::Revoked;
    }
    let Some(binding) = subkey_binding_in_force(primary, subkey, signatures, Some(t)) else {
        return KeyVerdict::NotAuthorized;
    };
    if !flags_allow_signing(Some(binding), subkey.algorithm()) {
        return KeyVerdict::NotAuthorized;
    }
    let backsig_ok = binding.embedded_signature().is_some_and(|back| {
        matches!(back.typ(), Some(SignatureType::KeyBinding))
            && certification_hash_acceptable(back)
            && back.verify_primary_key_binding(subkey, primary).is_ok()
    });
    if !backsig_ok {
        return KeyVerdict::NotAuthorized;
    }
    let expires = expiry_of(Some(binding), subkey.created_at().as_secs());
    if expires != 0 && expires <= t {
        return KeyVerdict::Expired;
    }
    KeyVerdict::Authorized
}

impl From<KeyVersion> for GfrOpenPGPKeyVersion {
    fn from(version: KeyVersion) -> Self {
        match version {
            KeyVersion::V4 => GfrOpenPGPKeyVersion::V4,
            KeyVersion::V5 => GfrOpenPGPKeyVersion::V5,
            KeyVersion::V6 => GfrOpenPGPKeyVersion::V6,
            _ => GfrOpenPGPKeyVersion::Unknown,
        }
    }
}

/// Index of the User ID whose self-certification is in force now (see
/// [`uid_certification_in_force`]), and whether that certification marks it
/// primary. Expiry, preferences and capabilities follow the same signature,
/// so the listing and the engine always agree on which User ID speaks for
/// the key.
/// True if `user` carries at least one verified self-certification
/// (0x10-0x13). Anyone can append a User ID packet to someone else's public
/// key; only one the key holder certified belongs to the key.
fn has_verified_uid_certification(primary: &PublicKey, user: &SignedUser) -> bool {
    verified_uid_self_sigs(primary, user).iter().any(|sig| {
        matches!(
            sig.typ(),
            Some(
                SignatureType::CertGeneric
                    | SignatureType::CertPersona
                    | SignatureType::CertCasual
                    | SignatureType::CertPositive
            )
        )
    })
}

/// The User IDs a listing reports, primary first. A User ID without a verified
/// self-certification is left out: it was listed as a valid, non-revoked UID
/// of the key, so a third party could attach their own address to it and have
/// a search by that address find this key.
fn listed_user_ids(pk: &SignedPublicKey) -> Vec<ExtractUserId> {
    let (primary_idx, primary_marked) = primary_user_index(&pk.primary_key, &pk.details);

    let mut user_ids: Vec<ExtractUserId> = Vec::new();
    let mut primary_pos = None;
    for (i, u) in pk.details.users.iter().enumerate() {
        if !has_verified_uid_certification(&pk.primary_key, u) {
            continue;
        }
        let is_revoked = is_user_id_revoked(&pk.primary_key, u);
        if i == primary_idx {
            primary_pos = Some(user_ids.len());
        }
        user_ids.push(ExtractUserId {
            user_id: String::from_utf8_lossy(u.id.id()).into_owned(),
            is_primary: !is_revoked && primary_marked && i == primary_idx,
            is_revoked,
        });
    }

    // The primary UID always comes first, which simplifies capability
    // extraction later.
    if let Some(pos) = primary_pos.filter(|&pos| pos != 0) {
        let primary_uid = user_ids.remove(pos);
        user_ids.insert(0, primary_uid);
    }
    user_ids
}

fn primary_user_index(primary: &PublicKey, details: &SignedKeyDetails) -> (usize, bool) {
    let Some(in_force) = uid_certification_in_force(primary, details, None) else {
        return (0, false);
    };
    let idx = details
        .users
        .iter()
        .position(|u| u.signatures.iter().any(|sig| std::ptr::eq(sig, in_force)))
        .unwrap_or(0);
    (idx, in_force.is_primary())
}

// Helper: Extract metadata from a secret key
fn build_secret_metadata(sk: &SignedSecretKey) -> ExtractedMetadata {
    let pk = SignedPublicKey::from(sk.clone());
    // Subkeys whose secret part lives elsewhere (a card, another machine),
    // listed in packet order: they precede the secret subkeys.
    let mut subs: Vec<ExtractedSubkey> = sk
        .public_subkeys
        .iter()
        .filter(|sub| subkey_is_bound(&pk.primary_key, &sub.key, &sub.signatures))
        .map(|sub| public_subkey_metadata(&pk, sub))
        .collect();

    for sub in &sk.secret_subkeys {
        let sub_pub = sub.key.public_key();
        if !subkey_is_bound(&pk.primary_key, sub_pub, &sub.signatures) {
            continue;
        }
        let (can_sign, can_encrypt, can_auth, can_certify) = usable_capabilities(
            extract_capabilities(subkey_binding_signature(
                &pk.primary_key,
                sub_pub,
                &sub.signatures,
            )),
            sub_pub.public_params(),
            pk.primary_key.public_params(),
        );
        let key_length = extract_key_length(sub.key.public_params());
        let is_revoked = is_subkey_revoked(&pk.primary_key, sub_pub, &sub.signatures);
        let created_at = sub.key.created_at().as_secs();
        let expires_at = subkey_expires_at(&pk.primary_key, sub_pub, &sub.signatures);
        subs.push(ExtractedSubkey {
            ver: sub.version().into(),
            fpr: sub.key.fingerprint().to_string(),
            key_id: sub.key.legacy_key_id().to_string(),
            algo: determine_algo(sub.key.public_params()),
            created_at,
            expires_at,
            has_secret: true,
            can_sign,
            can_encrypt,
            can_auth,
            can_certify,
            key_length: key_length.unwrap_or(0),
            is_revoked,
        });
    }

    let user_ids = listed_user_ids(&pk);

    let (can_sign, can_encrypt, can_auth, can_certify) = usable_capabilities(
        extract_capabilities(primary_sig_in_force(&pk.primary_key, &pk.details, None)),
        pk.primary_key.public_params(),
        pk.primary_key.public_params(),
    );
    let key_length = extract_key_length(pk.primary_key.public_params());
    let is_revoked = is_primary_key_revoked(&pk.primary_key, &pk.details.revocation_signatures)
        || is_primary_key_revoked(&pk.primary_key, &pk.details.direct_signatures);
    let created_at = pk.primary_key.created_at().as_secs();
    let expires_at = primary_key_expires_at(&pk);

    ExtractedMetadata {
        ver: pk.version().into(),
        fpr: pk.primary_key.fingerprint().to_string(),
        key_id: pk.primary_key.legacy_key_id().to_string(),
        algo: determine_algo(pk.primary_key.public_params()),
        created_at,
        expires_at,
        has_secret: true,
        is_revoked,
        can_sign,
        can_encrypt,
        can_auth,
        can_certify,
        user_ids, // Assign the collected vector here
        subkeys: subs,
        public_key_block: pk
            .to_armored_string(key_armor_opts(pk.primary_key.version()))
            .unwrap_or_default(),
        secret_key_block: sk
            .to_armored_string(key_armor_opts(sk.primary_key.version()))
            .ok()
            .map(Zeroizing::new),
        key_length: key_length.unwrap_or(0),
    }
}

// Helper: metadata of a subkey whose secret part is not held
/// True if `subkey` carries a verified binding signature from `primary`, or
/// was revoked by it. Anyone can put someone else's subkey into their own
/// certificate; listed anyway, the key database recorded the subkey as the
/// squatter's, and the real key could then not be imported, or a lookup by the
/// subkey's ID found the wrong certificate.
fn subkey_is_bound(primary: &PublicKey, subkey: &PublicSubkey, signatures: &[Signature]) -> bool {
    subkey_binding_signature(primary, subkey, signatures).is_some()
        || is_subkey_revoked(primary, subkey, signatures)
}

fn public_subkey_metadata(pk: &SignedPublicKey, sub: &SignedPublicSubKey) -> ExtractedSubkey {
    let (can_sign, can_encrypt, can_auth, can_certify) = usable_capabilities(
        extract_capabilities(subkey_binding_signature(
            &pk.primary_key,
            &sub.key,
            &sub.signatures,
        )),
        sub.key.public_params(),
        pk.primary_key.public_params(),
    );
    let key_length = extract_key_length(sub.key.public_params()).unwrap_or(0);
    let is_revoked = is_subkey_revoked(&pk.primary_key, &sub.key, &sub.signatures);
    let created_at = sub.key.created_at().as_secs();
    let expires_at = subkey_expires_at(&pk.primary_key, &sub.key, &sub.signatures);
    ExtractedSubkey {
        ver: sub.version().into(),
        fpr: sub.key.fingerprint().to_string(),
        key_id: sub.key.legacy_key_id().to_string(),
        algo: determine_algo(sub.key.public_params()),
        created_at,
        expires_at,
        has_secret: false,
        can_sign,
        can_encrypt,
        can_auth,
        can_certify,
        key_length,
        is_revoked,
    }
}

// Helper: Extract metadata from a public key
fn build_public_metadata(pk: &SignedPublicKey) -> ExtractedMetadata {
    let subs: Vec<ExtractedSubkey> = pk
        .public_subkeys
        .iter()
        .filter(|sub| subkey_is_bound(&pk.primary_key, &sub.key, &sub.signatures))
        .map(|sub| public_subkey_metadata(pk, sub))
        .collect();

    let user_ids = listed_user_ids(pk);

    // use only cryptographically valid self-signatures of the primary key and its
    // primary User ID to determine capabilities / expiry / revocation.
    let (can_sign, can_encrypt, can_auth, can_certify) = usable_capabilities(
        extract_capabilities(primary_sig_in_force(&pk.primary_key, &pk.details, None)),
        pk.primary_key.public_params(),
        pk.primary_key.public_params(),
    );
    let key_length = extract_key_length(pk.primary_key.public_params());
    let is_revoked = is_primary_key_revoked(&pk.primary_key, &pk.details.revocation_signatures)
        || is_primary_key_revoked(&pk.primary_key, &pk.details.direct_signatures);
    let created_at = pk.primary_key.created_at().as_secs();
    let expires_at = primary_key_expires_at(&pk);
    ExtractedMetadata {
        ver: pk.version().into(),
        fpr: pk.primary_key.fingerprint().to_string(),
        key_id: pk.primary_key.legacy_key_id().to_string(),
        user_ids,
        algo: determine_algo(pk.primary_key.public_params()),
        key_length: key_length.unwrap_or(0),
        created_at,
        expires_at,
        has_secret: false,
        is_revoked,
        subkeys: subs,
        can_sign,
        can_encrypt,
        can_auth,
        can_certify,
        public_key_block: pk
            .to_armored_string(key_armor_opts(pk.primary_key.version()))
            .unwrap_or_default(),
        secret_key_block: None,
    }
}

/// `block` without Trust packets (RFC 9580 §5.10: "SHOULD be ignored on any
/// input other than local keyring files"). rPGP's key parser stops collecting
/// User IDs and subkeys at the first packet it does not expect, so a Trust
/// packet from a GnuPG keyring export would silently cut the certificate
/// short. A block without Trust packets is returned unchanged.
fn without_trust_packets(block: &str) -> std::borrow::Cow<'_, str> {
    use pgp::packet::PacketParser;
    use std::borrow::Cow;
    // May hold unprotected secret key packets: wiped on drop.
    let bytes = Zeroizing::new(crate::crypto::dearmor_all(block.as_bytes()).into_owned());
    let packets: Vec<Packet> = PacketParser::new(Cursor::new(&bytes[..]))
        .flatten()
        .collect();
    if !packets.iter().any(|p| matches!(p, Packet::Trust(_))) {
        return Cow::Borrowed(block);
    }
    let mut kept = Vec::new();
    for packet in packets.iter().filter(|p| !matches!(p, Packet::Trust(_))) {
        if packet.to_writer(&mut kept).is_err() {
            return Cow::Borrowed(block);
        }
    }
    let typ = if block.contains("PRIVATE KEY BLOCK") {
        BlockType::PrivateKey
    } else {
        BlockType::PublicKey
    };
    let mut out = Vec::new();
    match armor::write(
        &MergedRawKeys { packet_bytes: kept },
        typ,
        &mut out,
        None,
        false,
    ) {
        Ok(()) => String::from_utf8(out).map_or(Cow::Borrowed(block), Cow::Owned),
        Err(_) => Cow::Borrowed(block),
    }
}

/// `data` as armored key text. Armor is only a transport encoding (RFC 9580
/// §6), and GnuPG's default `--export` writes binary packets: binary input is
/// re-armored (a secret key block if its first key packet is secret), armored
/// input is returned as is.
pub(crate) fn armored_key_input(data: &[u8]) -> Result<std::borrow::Cow<'_, str>, GfrStatus> {
    use pgp::packet::PacketParser;
    use std::borrow::Cow;
    if let Ok(text) = std::str::from_utf8(data) {
        if text.contains("-----BEGIN PGP") {
            return Ok(Cow::Borrowed(text));
        }
    }
    let packets: Vec<Packet> = PacketParser::new(Cursor::new(data)).flatten().collect();
    let secret = match packets.first() {
        Some(Packet::SecretKey(_)) => true,
        Some(Packet::PublicKey(_)) | Some(Packet::Signature(_)) => false,
        _ => {
            crate::err::set_last_error(
                "input is neither an armored nor a binary OpenPGP key block",
            );
            return Err(GfrStatus::ErrorInvalidInput);
        }
    };
    let mut bytes = Vec::new();
    for packet in &packets {
        packet.to_writer(&mut bytes).into_gfr()?;
    }
    let typ = if secret {
        BlockType::PrivateKey
    } else {
        BlockType::PublicKey
    };
    let mut out = Vec::new();
    armor::write(
        &MergedRawKeys {
            packet_bytes: bytes,
        },
        typ,
        &mut out,
        None,
        false,
    )
    .into_gfr()?;
    String::from_utf8(out)
        .map(Cow::Owned)
        .map_err(|_| GfrStatus::ErrorArmorFailed)
}

fn split_pgp_blocks(input: &str) -> Vec<String> {
    let mut blocks = Vec::new();
    let mut current_block = String::new();
    let mut in_block = false;

    for line in input.lines() {
        if line.contains("-----BEGIN PGP") {
            in_block = true;
            current_block.clear();
        }

        if in_block {
            current_block.push_str(line);
            current_block.push('\n');
        }

        if line.contains("-----END PGP") {
            in_block = false;
            blocks.push(without_trust_packets(&current_block).into_owned());
        }
    }

    blocks
}

/// Parse one or more armored PGP key blocks and return metadata for each unique key.
///
/// The input may contain concatenated armor blocks. Copies of one certificate
/// (say a secret block and a public block carrying newer signatures, or two
/// keyring entries) are merged in input order, as `sq` imports each copy in
/// turn; a secret copy is the base, so its secret parts win. Nothing is
/// filtered here: importing canonicalizes before storing. Keys are returned
/// in the order they first appear.
pub fn extract_metadata_many_internal(
    key_blocks: &str,
) -> Result<Vec<ExtractedMetadata>, GfrStatus> {
    let key_blocks = crate::utils::strip_armor_whitespace(key_blocks);
    let individual_blocks = split_pgp_blocks(&key_blocks);

    if individual_blocks.is_empty() {
        return Err(GfrStatus::ErrorInvalidInput);
    }

    let mut order: Vec<String> = Vec::new();
    let mut certs: HashMap<String, WorkCert> = HashMap::new();
    let budget = CheckBudget::new();
    let mut add = |cert: WorkCert| -> Result<(), GfrStatus> {
        let fpr = cert.fingerprint().to_string();
        let Some(existing) = certs.get_mut(&fpr) else {
            order.push(fpr.clone());
            certs.insert(fpr, cert);
            return Ok(());
        };
        if existing.secret.is_none() && cert.secret.is_some() {
            let public_copy = std::mem::replace(existing, cert);
            existing.absorb(public_copy, false, &budget)
        } else {
            existing.absorb(cert, false, &budget)
        }
    };

    for block in individual_blocks {
        if let Ok((sk_iter, _)) = SignedSecretKey::from_string_many(&block) {
            for sk_res in sk_iter {
                match sk_res {
                    Ok(sk) => add(WorkCert::from_secret(sk)?)?,
                    Err(e) => log::error!("Failed to parse a secret key from block: {e}"),
                }
            }
        }

        if let Ok((pk_iter, _)) = SignedPublicKey::from_string_many(&block) {
            for pk_res in pk_iter {
                match pk_res {
                    Ok(pk) => add(WorkCert::from_public(pk)?)?,
                    Err(e) => log::error!("Failed to parse a public key from block: {e}"),
                }
            }
        }
    }

    log::info!(
        "Completed processing all blocks. Total unique keys found: {}",
        order.len()
    );

    let mut results = Vec::with_capacity(order.len());
    for fpr in order {
        let cert = certs.remove(&fpr).ok_or(GfrStatus::ErrorInternal)?;
        results.push(match cert.into_signed()? {
            SignedSecretKeyOrPublic::Secret(sk) => build_secret_metadata(&sk),
            SignedSecretKeyOrPublic::Public(pk) => build_public_metadata(&pk),
        });
    }

    if results.is_empty() {
        return Err(GfrStatus::ErrorInvalidInput);
    }

    Ok(results)
}

/// Report the packet version (v4 / v6) of the primary key in an armored block.
///
/// Only the first armor block is inspected: the caller asks about one key. This
/// is deliberately cheaper than [`extract_metadata_many_internal`], which
/// re-armors both the public and secret blocks and builds the whole subkey and
/// user ID tree just to reach the same single field.
///
/// Public keys are tried first because the GnuPG detection path feeds a minimal
/// public export; secret blocks are still accepted so the function is usable on
/// whatever the caller has at hand. A version rPGP does not model (v2/v3) comes
/// back as [`GfrOpenPGPKeyVersion::Unknown`].
///
/// v5 (LibrePGP) never survives either parse -- see [`peek_primary_key_version`],
/// which answers for it.
pub fn detect_key_version_internal(key_block: &str) -> Result<GfrOpenPGPKeyVersion, GfrStatus> {
    let block = split_pgp_blocks(key_block)
        .into_iter()
        .next()
        .ok_or(GfrStatus::ErrorInvalidInput)?;

    if let Ok((pk_iter, _)) = SignedPublicKey::from_string_many(&block) {
        if let Some(pk) = pk_iter.flatten().next() {
            return Ok(pk.primary_key.version().into());
        }
    }

    if let Ok((sk_iter, _)) = SignedSecretKey::from_string_many(&block) {
        if let Some(sk) = sk_iter.flatten().next() {
            return Ok(sk.primary_key.version().into());
        }
    }

    peek_primary_key_version(&block).ok_or(GfrStatus::ErrorInvalidInput)
}

/// Read the version octet of the first key packet in an armored block.
///
/// rPGP refuses to parse a v5 (LibrePGP) key packet: its packet parser raises
/// `Unsupported`, and unsupported packets are then dropped *silently*, so a v5
/// block reaches [`detect_key_version_internal`] as an empty key iterator rather
/// than as an error. The version is only the first octet of the packet body
/// though, so it can be read without any of the parsing rPGP declines to do.
///
/// Only the first packet is inspected, and only when it is a key packet -- that
/// is what keeps armored *messages* (whose first packet is a PKESK) out. A
/// version we cannot name comes back as `None`, so the caller keeps reporting an
/// error instead of a bogus answer.
fn peek_primary_key_version(block: &str) -> Option<GfrOpenPGPKeyVersion> {
    let mut reader = BufReader::new(armor::Dearmor::new(Cursor::new(block.as_bytes())));

    let header = PacketHeader::try_from_reader(&mut reader).ok()?;
    if !matches!(header.tag(), Tag::PublicKey | Tag::SecretKey) {
        return None;
    }

    let mut version = [0_u8; 1];
    reader.read_exact(&mut version).ok()?;

    match GfrOpenPGPKeyVersion::from(KeyVersion::from(version[0])) {
        GfrOpenPGPKeyVersion::Unknown => None,
        version => Some(version),
    }
}

/// Derive the public key from an armored secret key block.
///
/// Strips all secret key material and re-exports only the public components.
pub fn extract_public_key_internal(secret_block: &str) -> Result<String, GfrStatus> {
    // 1. Parse the armored secret key block
    let (secret_key, _) =
        SignedSecretKey::from_string(secret_block).map_err(|_| GfrStatus::ErrorInvalidInput)?;

    // 2. Convert to public key (this strips the secret mathematical materials)
    let public_key = SignedPublicKey::from(secret_key);

    // 3. Export back to ASCII Armor format
    let armored_p_key = public_key
        .to_armored_string(key_armor_opts(public_key.primary_key.version()))
        .map_err(|_| GfrStatus::ErrorArmorFailed)?;

    Ok(armored_p_key)
}

struct MergedRawKeys {
    packet_bytes: Vec<u8>,
}

// 2. Implement the Serialize trait so rpgp's armor::write can consume it.
// The bytes may be unprotected secret key packets: wipe them when done.
impl Drop for MergedRawKeys {
    fn drop(&mut self) {
        zeroize::Zeroize::zeroize(&mut self.packet_bytes);
    }
}

impl Serialize for MergedRawKeys {
    fn to_writer<W: io::Write>(&self, writer: &mut W) -> pgp::errors::Result<()> {
        writer.write_all(&self.packet_bytes)?;
        Ok(())
    }

    fn write_len(&self) -> usize {
        self.packet_bytes.len()
    }
}

/// Merge multiple armored key blocks into a single public key armor block.
///
/// If an input block is a secret key it is accepted and its public component
/// is extracted. The merge is done at the raw packet level (no re-signing).
pub fn export_merged_public_keys(key_blocks: &[&str]) -> Result<String, GfrStatus> {
    let mut armored = export_merged_keys(key_blocks, false, true, false)?;
    String::from_utf8(std::mem::take(&mut *armored)).map_err(|_| GfrStatus::ErrorArmorFailed)
}

/// Keep only the signatures the key's owner made on its own User IDs and
/// attributes, as gpg's `export-minimal` does: third-party certifications
/// tell whoever receives the key who else vouched for its owner.
fn strip_third_party_signatures(details: &mut SignedKeyDetails, primary: &PublicKey) {
    for user in &mut details.users {
        user.signatures
            .retain(|sig| names_primary_as_issuer(sig, primary));
    }
    for attr in &mut details.user_attributes {
        attr.signatures
            .retain(|sig| names_primary_as_issuer(sig, primary));
    }
}

/// Merge key blocks into one keyring, as the caller asked for it: secret or
/// public, armored or as plain packets, whole or minimal.
///
/// `secret` takes only secret key blocks (a public one is `ErrorInvalidInput`);
/// otherwise secret blocks are reduced to their public part. `minimal` drops
/// third-party certifications ([`strip_third_party_signatures`]). The merge is
/// done at the raw packet level (no re-signing).
///
/// The bytes may be unprotected secret key packets, so every buffer that holds
/// them is wiped when it goes, and each is sized up front: a Vec that grows
/// frees its old allocation without wiping it.
pub fn export_merged_keys(
    key_blocks: &[&str],
    secret: bool,
    armored: bool,
    minimal: bool,
) -> Result<Zeroizing<Vec<u8>>, GfrStatus> {
    // The packets are never larger than the armored blocks they came from.
    let mut combined_bytes = Zeroizing::new(Vec::with_capacity(
        key_blocks.iter().map(|b| b.len()).sum::<usize>(),
    ));
    let mut v4_only = true;

    for block in key_blocks {
        if secret {
            let Ok((mut sec_key, _)) = SignedSecretKey::from_string(block) else {
                return Err(GfrStatus::ErrorInvalidInput);
            };
            v4_only &= is_v4_era(sec_key.primary_key.version());
            if minimal {
                let primary = sec_key.primary_key.public_key().clone();
                strip_third_party_signatures(&mut sec_key.details, &primary);
            }
            sec_key
                .to_writer(&mut *combined_bytes)
                .map_err(|_| GfrStatus::ErrorInvalidInput)?;
        } else {
            let mut pub_key = if let Ok((sk, _)) = SignedSecretKey::from_string(block) {
                SignedPublicKey::from(sk)
            } else if let Ok((pk, _)) = SignedPublicKey::from_string(block) {
                pk
            } else {
                return Err(GfrStatus::ErrorInvalidInput);
            };
            v4_only &= is_v4_era(pub_key.primary_key.version());
            if minimal {
                let primary = pub_key.primary_key.clone();
                strip_third_party_signatures(&mut pub_key.details, &primary);
            }
            pub_key
                .to_writer(&mut *combined_bytes)
                .map_err(|_| GfrStatus::ErrorInvalidInput)?;
        }
    }

    if combined_bytes.is_empty() {
        return Err(GfrStatus::ErrorInvalidInput);
    }
    if !armored {
        return Ok(combined_bytes);
    }

    // Armor is 4/3 of its input, plus line breaks, headers and the checksum.
    let mut armored_output = Zeroizing::new(Vec::with_capacity(
        combined_bytes.len() / 3 * 4 + combined_bytes.len() / 32 + 1024,
    ));
    let merged_source = MergedRawKeys {
        packet_bytes: std::mem::take(&mut *combined_bytes),
    };
    armor::write(
        &merged_source,
        if secret {
            BlockType::PrivateKey
        } else {
            BlockType::PublicKey
        },
        &mut *armored_output,
        None,
        // RFC 9580 §6.1: a CRC24 footer for v4-only keyrings (GnuPG
        // interoperability), never with a v6 key (see `armor_opts_for`).
        v4_only,
    )
    .map_err(|_| GfrStatus::ErrorArmorFailed)?;
    Ok(armored_output)
}

/// What [`filter_public_keys_internal`] kept and how many it left out.
pub struct FilteredPublicKeys {
    /// The kept certificates, armored; empty when none matched.
    pub armored: String,
    /// Certificates in the input that matched no requested identifier.
    pub dropped: usize,
}

/// Keep only the certificates in `key_blocks` whose primary key is one of
/// `wanted`: a full fingerprint, or a 16-hex-digit key ID. Shorter IDs are
/// never matched, since anyone can make a key with a chosen short ID.
///
/// A key server answers a request for one key with whatever it likes;
/// importing that verbatim let a server, or anyone between it and an
/// `http://` URL, slip in keys nobody asked for. Secret blocks are reduced
/// to their public part, which is all a key server has any business sending.
pub fn filter_public_keys_internal(
    key_blocks: &str,
    wanted: &[&str],
) -> Result<FilteredPublicKeys, GfrStatus> {
    let normalise = |s: &str| -> String {
        let s: String = s.chars().filter(|c| !c.is_whitespace()).collect();
        s.trim_start_matches("0x")
            .trim_start_matches("0X")
            .to_ascii_uppercase()
    };
    let wanted: Vec<String> = wanted
        .iter()
        .map(|w| normalise(w))
        .filter(|w| w.len() >= 16)
        .collect();

    let key_blocks = crate::utils::strip_armor_whitespace(key_blocks);
    let mut certs: Vec<SignedPublicKey> = Vec::new();
    for block in split_pgp_blocks(&key_blocks) {
        if let Ok((sk_iter, _)) = SignedSecretKey::from_string_many(&block) {
            certs.extend(sk_iter.flatten().map(SignedPublicKey::from));
        }
        if let Ok((pk_iter, _)) = SignedPublicKey::from_string_many(&block) {
            certs.extend(pk_iter.flatten());
        }
    }

    let mut kept: Vec<String> = Vec::new();
    let mut dropped = 0;
    for cert in certs {
        let fpr = normalise(&cert.fingerprint().to_string());
        let key_id = normalise(&cert.legacy_key_id().to_string());
        let matches = wanted
            .iter()
            .any(|w| *w == fpr || (w.len() == 16 && *w == key_id));
        if matches {
            kept.push(
                cert.to_armored_string(key_armor_opts(cert.primary_key.version()))
                    .map_err(|_| GfrStatus::ErrorArmorFailed)?,
            );
        } else {
            log::warn!("dropping a certificate nobody asked for: {}", fpr);
            dropped += 1;
        }
    }

    let armored = if kept.is_empty() {
        String::new()
    } else {
        let refs: Vec<&str> = kept.iter().map(String::as_str).collect();
        export_merged_public_keys(&refs)?
    };
    Ok(FilteredPublicKeys { armored, dropped })
}

/// Merge multiple armored secret key blocks into a single secret key armor block.
///
/// All input blocks must be secret keys; a public-key block returns `ErrorInvalidInput`.
pub fn export_merged_secret_keys(key_blocks: &[&str]) -> Result<String, GfrStatus> {
    let mut armored = export_merged_keys(key_blocks, true, true, false)?;
    String::from_utf8(std::mem::take(&mut *armored)).map_err(|_| GfrStatus::ErrorArmorFailed)
}

/// Prompt for the key's current passphrase, to unlock it before re-protection.
fn fetch_old_password(
    channel: i32,
    target_fpr: &str,
    purpose: &str,
    fetch_pwd_cb: Option<GfrPasswordFetchCb>,
) -> Result<Password, GfrStatus> {
    let bytes = fetch_password_with_cache(
        Some(&PASSWORD_CACHE),
        PasswordCachePolicy::Default,
        channel,
        PassphraseStateInternal {
            fpr: target_fpr.to_string(),
            info: purpose.to_string(),
            retry: false,
            ask_for_new: false,
            should_confirm: false,
        },
        fetch_pwd_cb,
    )?;

    if bytes.is_empty() {
        return Err(GfrStatus::ErrorFetchPasswordFailed);
    }

    Ok(password_from_zeroizing_bytes(bytes))
}

/// Prompt for the new passphrase to re-protect the key with.
///
/// Only ever called once the key has actually been unlocked, so the user is not
/// asked to choose a new passphrase for a key they could not open.
fn fetch_new_password(
    channel: i32,
    target_fpr: &str,
    purpose: &str,
    fetch_pwd_cb: Option<GfrPasswordFetchCb>,
) -> Result<Password, GfrStatus> {
    let bytes = fetch_password_with_cache(
        Some(&PASSWORD_CACHE),
        PasswordCachePolicy::Bypass,
        channel,
        PassphraseStateInternal {
            fpr: target_fpr.to_string(),
            info: purpose.to_string(),
            retry: false,
            ask_for_new: true,
            should_confirm: true,
        },
        fetch_pwd_cb,
    )?;

    if bytes.is_empty() {
        return Err(GfrStatus::ErrorFetchPasswordFailed);
    }

    Ok(password_from_zeroizing_bytes(bytes))
}

/// Prompt for a key's current passphrase again, targeted at `fpr`.
///
/// Used when a passphrase already collected for the key block is rejected by an
/// individual subkey, which may legitimately carry a different one.
fn fetch_retry_password(
    channel: i32,
    fpr: &str,
    purpose: &str,
    fetch_pwd_cb: Option<GfrPasswordFetchCb>,
) -> Result<Password, GfrStatus> {
    let bytes = fetch_password_with_cache(
        Some(&PASSWORD_CACHE),
        PasswordCachePolicy::Bypass,
        channel,
        PassphraseStateInternal {
            fpr: fpr.to_string(),
            info: purpose.to_string(),
            retry: true,
            ask_for_new: false,
            should_confirm: false,
        },
        fetch_pwd_cb,
    )?;

    if bytes.is_empty() {
        return Err(GfrStatus::ErrorFetchPasswordFailed);
    }

    Ok(password_from_zeroizing_bytes(bytes))
}

/// How many times a rejected unlock passphrase may be re-entered before the
/// operation gives up, mirroring gpg-agent's default pinentry allowance.
const PASSPHRASE_RETRY_LIMIT: usize = 3;

/// Strip the passphrase from one key, re-prompting when the one supplied is
/// rejected.
///
/// `initial` is tried first when present — the passphrase already collected for
/// this key block, which is usually right and costs no extra prompt. Each
/// rejection spends one retry prompt targeted at `fpr`, so a typo (or a subkey
/// carrying a different passphrase) is recoverable instead of aborting the whole
/// operation. Exhausting the retries reports `ErrorBadPassphrase`, which the C++
/// side turns into `GPG_ERR_BAD_PASSPHRASE` — a bare `ErrorInvalidInput` from
/// the underlying unlock would surface to the user as "General error".
fn remove_password_with_retry<F>(
    channel: i32,
    fpr: &str,
    purpose: &str,
    initial: Option<&Password>,
    fetch_pwd_cb: Option<GfrPasswordFetchCb>,
    mut remove: F,
) -> Result<(), GfrStatus>
where
    F: FnMut(&Password) -> bool,
{
    if let Some(pw) = initial {
        if remove(pw) {
            return Ok(());
        }
        // Whatever is cached for this key was just proven wrong; leaving it in
        // place would feed the same rejected passphrase to the next operation.
        PASSWORD_CACHE.remove_by_fpr(fpr);
        log::warn!("passphrase rejected for key {}, re-prompting", fpr);
    }

    for _ in 0..PASSPHRASE_RETRY_LIMIT {
        let pw = fetch_retry_password(channel, fpr, purpose, fetch_pwd_cb)?;
        if remove(&pw) {
            return Ok(());
        }
        log::warn!("passphrase rejected for key {}, re-prompting", fpr);
    }

    log::error!("passphrase for key {} rejected after retries", fpr);
    Err(GfrStatus::ErrorBadPassphrase)
}

/// Check that `pwd` unlocks `key` before anything is signed with it.
///
/// A wrong passphrase is then reported as such — `ErrorBadPassphrase`, which
/// the C++ side reads as a bad passphrase — rather than as whatever the
/// signing step fails with (an invalid-data error the user would see as
/// "General error"), and it is dropped from the cache so the next try asks
/// again. A key whose secret is not encrypted needs no passphrase.
pub(crate) fn require_unlocking_passphrase(
    key: &pgp::packet::SecretKey,
    pwd: &Password,
    fpr: &str,
) -> Result<(), GfrStatus> {
    if !matches!(key.secret_params(), SecretParams::Encrypted(_)) {
        return Ok(());
    }
    if matches!(key.unlock(pwd, |_, _| Ok(())), Ok(Ok(()))) {
        return Ok(());
    }
    PASSWORD_CACHE.remove_by_fpr(fpr);
    crate::err::set_last_error("the passphrase does not unlock the secret key");
    Err(GfrStatus::ErrorBadPassphrase)
}

/// Re-protect every secret key in the block — primary and all subkeys — under a
/// single new passphrase.
///
/// This mirrors `gpg --passwd` (what `gpgme_op_passwd` runs), which re-protects
/// the primary and every subkey together. One unlock prompt and one new-password
/// prompt are issued against the primary fingerprint; a subkey that rejects the
/// shared unlock passphrase gets its own targeted retry prompt.
fn change_whole_key_password(
    channel: i32,
    secret_key: &mut SignedSecretKey,
    fetch_pwd_cb: Option<GfrPasswordFetchCb>,
) -> Result<(), GfrStatus> {
    let primary_fpr = secret_key
        .primary_key
        .fingerprint()
        .to_string()
        .to_uppercase();

    let any_encrypted = secret_key.primary_key.secret_params().is_encrypted()
        || secret_key
            .secret_subkeys
            .iter()
            .any(|s| s.key.secret_params().is_encrypted());

    // Unlock everything first, so the new passphrase is only asked for once the
    // key is proven to open — never prompt for a new password before the old one
    // is verified. The old passphrase is fetched once and reused across keys; a
    // key that rejects it is re-prompted on its own.
    let old_pw = if any_encrypted {
        Some(fetch_old_password(
            channel,
            &primary_fpr,
            "Unlock Key to change password",
            fetch_pwd_cb,
        )?)
    } else {
        None
    };

    if secret_key.primary_key.secret_params().is_encrypted() {
        let primary = &mut secret_key.primary_key;
        remove_password_with_retry(
            channel,
            &primary_fpr,
            "Unlock Primary Key to change password",
            old_pw.as_ref(),
            fetch_pwd_cb,
            |pw| primary.remove_password(pw).is_ok(),
        )?;
    }

    for subkey in secret_key.secret_subkeys.iter_mut() {
        if subkey.key.secret_params().is_encrypted() {
            let sub_fpr = subkey.key.fingerprint().to_string().to_uppercase();
            let key = &mut subkey.key;
            remove_password_with_retry(
                channel,
                &sub_fpr,
                "Unlock Subkey to change password",
                old_pw.as_ref(),
                fetch_pwd_cb,
                |pw| key.remove_password(pw).is_ok(),
            )?;
        }
    }

    // Everything is open; now collect the single new passphrase and apply it.
    let new_pw = fetch_new_password(
        channel,
        &primary_fpr,
        "Set new password for Key",
        fetch_pwd_cb,
    )?;

    let mut rng = rand::thread_rng();

    secret_key
        .primary_key
        .set_password(&mut rng, &new_pw)
        .into_gfr()?;

    // Invalidate cache entries for this key
    PASSWORD_CACHE.remove_by_fpr(&primary_fpr);

    for subkey in secret_key.secret_subkeys.iter_mut() {
        let sub_fpr = subkey.key.fingerprint().to_string().to_uppercase();
        subkey.key.set_password(&mut rng, &new_pw).into_gfr()?;

        // Decrypt and sign cache under the subkey fingerprint, so a stale entry
        // here would keep serving the passphrase that was just replaced.
        PASSWORD_CACHE.remove_by_fpr(&sub_fpr);
    }

    Ok(())
}

/// Re-protect exactly one key — the primary or a single subkey — identified by
/// `target_fpr`, leaving every other key in the block untouched.
fn change_single_key_password(
    channel: i32,
    secret_key: &mut SignedSecretKey,
    target_fpr: &str,
    fetch_pwd_cb: Option<GfrPasswordFetchCb>,
) -> Result<(), GfrStatus> {
    let primary_fpr = secret_key
        .primary_key
        .fingerprint()
        .to_string()
        .to_uppercase();

    let mut rng = rand::thread_rng();

    if primary_fpr == target_fpr {
        // Unlock the key first; only prompt for the new passphrase once it opens.
        if secret_key.primary_key.secret_params().is_encrypted() {
            let old_pw = fetch_old_password(
                channel,
                target_fpr,
                "Unlock Primary Key to change password",
                fetch_pwd_cb,
            )?;
            let primary = &mut secret_key.primary_key;
            remove_password_with_retry(
                channel,
                &primary_fpr,
                "Unlock Primary Key to change password",
                Some(&old_pw),
                fetch_pwd_cb,
                |pw| primary.remove_password(pw).is_ok(),
            )?;
        }

        let new_pw = fetch_new_password(
            channel,
            target_fpr,
            "Set new password for Primary Key",
            fetch_pwd_cb,
        )?;

        secret_key
            .primary_key
            .set_password(&mut rng, &new_pw)
            .into_gfr()?;

        // Invalidate cache entries for this key
        PASSWORD_CACHE.remove_by_fpr(&primary_fpr);

        return Ok(());
    }

    for subkey in secret_key.secret_subkeys.iter_mut() {
        let sub_fpr = subkey.key.fingerprint().to_string().to_uppercase();

        if sub_fpr != target_fpr {
            continue;
        }

        // Unlock the subkey first; only prompt for the new passphrase once it
        // opens, so a mistyped current passphrase never costs a new one.
        if subkey.key.secret_params().is_encrypted() {
            let old_pw = fetch_old_password(
                channel,
                target_fpr,
                "Unlock Subkey to change password",
                fetch_pwd_cb,
            )?;
            let key = &mut subkey.key;
            remove_password_with_retry(
                channel,
                &sub_fpr,
                "Unlock Subkey to change password",
                Some(&old_pw),
                fetch_pwd_cb,
                |pw| key.remove_password(pw).is_ok(),
            )?;
        }

        let new_pw = fetch_new_password(
            channel,
            target_fpr,
            "Set new password for Subkey",
            fetch_pwd_cb,
        )?;

        subkey.key.set_password(&mut rng, &new_pw).into_gfr()?;

        // Decrypt and sign cache under the subkey fingerprint, so a stale entry
        // here would keep serving the passphrase that was just replaced.
        PASSWORD_CACHE.remove_by_fpr(&sub_fpr);

        return Ok(());
    }

    Err(GfrStatus::ErrorInvalidInput)
}

/// Change the passphrase protecting a secret key block.
///
/// `target_fpr` selects the scope: `None` re-protects the whole key — primary and
/// every subkey — under one new passphrase, matching `gpg --passwd`. `Some(fpr)`
/// re-protects only that key, which may be the primary or any subkey. The entire
/// key block is re-exported either way, so the returned block contains all keys.
pub fn modify_key_password_internal(
    channel: i32,
    secret_key_block: &str,
    target_fpr: Option<&str>,
    fetch_pwd_cb: Option<GfrPasswordFetchCb>,
) -> Result<GeneratedKeys, GfrStatus> {
    let (mut secret_key, _) = SignedSecretKey::from_string(secret_key_block).map_err(|e| {
        log::error!("Failed to parse secret key block: {}", e);
        GfrStatus::ErrorInvalidData
    })?;

    match target_fpr {
        Some(fpr) => {
            change_single_key_password(channel, &mut secret_key, &fpr.to_uppercase(), fetch_pwd_cb)?
        }
        None => change_whole_key_password(channel, &mut secret_key, fetch_pwd_cb)?,
    }

    export_secret_key(secret_key)
}

/// Convert an absolute expiration time into the OpenPGP duration-from-creation
/// form used by the `KeyExpirationTime` subpacket.
///
/// `expiration_epoch_secs == 0` means "never expires" and yields `None`. An
/// expiration at or before the component's creation time is rejected as
/// `ErrorInvalidInput`, as is one so far in the future it overflows the 4-octet
/// duration field.
fn expiration_duration(
    created_at: u32,
    expiration_epoch_secs: u64,
) -> Result<Option<Duration>, GfrStatus> {
    if expiration_epoch_secs == 0 {
        return Ok(None);
    }

    let created = u64::from(created_at);
    if expiration_epoch_secs <= created {
        return Err(GfrStatus::ErrorInvalidInput);
    }

    let secs =
        u32::try_from(expiration_epoch_secs - created).map_err(|_| GfrStatus::ErrorInvalidInput)?;

    Ok(Some(Duration::from_secs(secs)))
}

/// Clone a template self-signature's config and rewrite it to carry `duration`
/// as the key expiration, preserving every other subpacket.
///
/// Key flags, algorithm preferences, features, the primary-UID flag, and any
/// embedded primary-key-binding back-signature (on signing subkeys) all survive
/// because the whole config — including its signature type and version — is
/// cloned from the existing signature. Only the issuer fingerprint, signature
/// creation time, and key expiration are refreshed. A `None` duration drops the
/// expiration entirely ("never expires").
///
/// The template's own Signature Expiration Time (§5.2.3.18) is dropped too:
/// the user is setting the key's validity window, and a copied one would end
/// the new self-signature, and with it the key, at the old date.
fn build_expiration_sig_config(
    template: &Signature,
    primary: &PublicKey,
    default_hash: pgp::crypto::hash::HashAlgorithm,
    duration: Option<Duration>,
) -> Result<SignatureConfig, GfrStatus> {
    let mut cfg = template.config().cloned().ok_or(GfrStatus::ErrorInternal)?;
    refresh_reissued_config(&mut cfg, default_hash)?;

    let strip = |subpackets: &mut Vec<Subpacket>| {
        subpackets.retain(|sp| {
            !matches!(
                sp.data,
                SubpacketData::SignatureCreationTime(_)
                    | SubpacketData::IssuerFingerprint(_)
                    | SubpacketData::IssuerKeyId(_)
                    | SubpacketData::KeyExpirationTime(_)
                    | SubpacketData::SignatureExpirationTime(_)
            )
        });
    };
    strip(&mut cfg.hashed_subpackets);
    strip(&mut cfg.unhashed_subpackets);

    push_issuer(&mut cfg, primary)?;
    cfg.hashed_subpackets.push(
        Subpacket::regular(SubpacketData::SignatureCreationTime(
            pgp::types::Timestamp::now(),
        ))
        .map_err(|_| GfrStatus::ErrorInternal)?,
    );
    if let Some(dur) = duration {
        cfg.hashed_subpackets.push(
            Subpacket::regular(SubpacketData::KeyExpirationTime(dur))
                .map_err(|_| GfrStatus::ErrorInternal)?,
        );
    }

    Ok(cfg)
}

/// Re-issue the primary key's self-signature(s) with a new expiration.
///
/// V6 keys carry key expiration in the direct-key signature (0x1F); V4 keys
/// carry it in each User ID's self-certification. The corresponding signatures
/// are rebuilt from the current ones and replaced in place, so a key with no
/// usable self-signature is rejected as `ErrorInvalidData`.
fn update_primary_expiration(
    secret_key: &mut SignedSecretKey,
    duration: Option<Duration>,
    pwd: &Password,
) -> Result<(), GfrStatus> {
    let pk = secret_key.primary_key.public_key();
    let default_hash = pgp::types::SigningKey::hash_alg(&secret_key.primary_key);

    if secret_key.primary_key.version() == KeyVersion::V6 {
        let template = direct_reissue_template(&pk, &secret_key.details.direct_signatures)
            .cloned()
            .ok_or(GfrStatus::ErrorInvalidData)?;

        let cfg = build_expiration_sig_config(&template, &pk, default_hash, duration)?;
        let new_sig = cfg.sign_key(&secret_key.primary_key, pwd, &pk).into_gfr()?;

        secret_key
            .details
            .direct_signatures
            .retain(|sig| !(names_primary_as_issuer(sig, &pk) && is_property_self_sig(sig)));
        secret_key.details.direct_signatures.push(new_sig);

        return Ok(());
    }

    // V4: re-sign the self-certification on every certified User ID. A
    // revoked one is left alone: a certification newer than its revocation
    // would un-revoke it for GnuPG and Sequoia.
    let mut updated_any = false;
    for user in secret_key.details.users.iter_mut() {
        if is_user_id_revoked(&pk, user) {
            continue;
        }
        let Some(template) = uid_reissue_template(&pk, user).cloned() else {
            continue;
        };

        let cfg = build_expiration_sig_config(&template, &pk, default_hash, duration)?;
        let new_sig = cfg
            .sign_certification(&secret_key.primary_key, &pk, pwd, Tag::UserId, &user.id)
            .into_gfr()?;

        // Replace the certifications only: a 0x30 revocation must survive.
        user.signatures
            .retain(|sig| !(names_primary_as_issuer(sig, &pk) && is_property_self_sig(sig)));
        user.signatures.push(new_sig);
        updated_any = true;
    }

    if updated_any {
        Ok(())
    } else {
        Err(GfrStatus::ErrorInvalidData)
    }
}

/// The subkey `fpr` (uppercase) of a secret key, whether its secret is held
/// here or not: its public key and its signatures. Editing a binding or a
/// revocation needs only the primary's secret, so a public-only subkey (one
/// on a card) is as editable as any other.
fn subkey_entry_mut<'a>(
    secret_subkeys: &'a mut [SignedSecretSubKey],
    public_subkeys: &'a mut [SignedPublicSubKey],
    fpr: &str,
) -> Option<(pgp::packet::PublicSubkey, &'a mut Vec<Signature>)> {
    if let Some(sub) = secret_subkeys
        .iter_mut()
        .find(|sub| sub.key.fingerprint().to_string().to_uppercase() == fpr)
    {
        return Some((sub.key.public_key().clone(), &mut sub.signatures));
    }
    public_subkeys
        .iter_mut()
        .find(|sub| sub.key.fingerprint().to_string().to_uppercase() == fpr)
        .map(|sub| (sub.key.clone(), &mut sub.signatures))
}

/// Re-issue a single subkey's binding signature with a new expiration.
///
/// The subkey binding signature (0x18) is rebuilt from the current verified
/// one and replaced in place; any revocation signatures on the subkey are
/// preserved.
fn update_subkey_expiration(
    secret_key: &mut SignedSecretKey,
    target_fpr: &str,
    duration: Option<Duration>,
    pwd: &Password,
) -> Result<(), GfrStatus> {
    let pk = secret_key.primary_key.public_key();
    let default_hash = pgp::types::SigningKey::hash_alg(&secret_key.primary_key);

    let (sub_key, signatures) = subkey_entry_mut(
        &mut secret_key.secret_subkeys,
        &mut secret_key.public_subkeys,
        target_fpr,
    )
    .ok_or(GfrStatus::ErrorInvalidInput)?;

    let template = binding_reissue_template(pk, &sub_key, signatures)
        .cloned()
        .ok_or(GfrStatus::ErrorInvalidData)?;

    let cfg = build_expiration_sig_config(&template, pk, default_hash, duration)?;
    let new_sig = cfg
        .sign_subkey_binding(&secret_key.primary_key, pk, pwd, &sub_key)
        .into_gfr()?;

    signatures.retain(|sig| {
        !(names_primary_as_issuer(sig, pk)
            && matches!(sig.typ(), Some(SignatureType::SubkeyBinding)))
    });
    signatures.push(new_sig);

    Ok(())
}

/// Change the expiration of a primary key or a single subkey.
///
/// `target_fpr` selects the scope: `None`, an empty string, or the primary key
/// fingerprint targets the primary key; any other fingerprint targets that
/// subkey. `expiration_epoch_secs` is an absolute Unix time, or `0` for "never
/// expires". The primary key is unlocked once (its passphrase drives the
/// re-signing in every case) and the whole key block is re-exported.
pub fn update_key_expiration_internal(
    channel: i32,
    secret_key_block: &str,
    target_fpr: Option<&str>,
    expiration_epoch_secs: u64,
    fetch_cb: Option<GfrPasswordFetchCb>,
) -> Result<GeneratedKeys, GfrStatus> {
    let (mut secret_key, _) = SignedSecretKey::from_string(secret_key_block).map_err(|e| {
        log::error!("Failed to parse secret key block: {}", e);
        GfrStatus::ErrorInvalidData
    })?;

    refuse_forbidden_signer(secret_key.primary_key.public_params())?;

    let primary_fpr = secret_key
        .primary_key
        .fingerprint()
        .to_string()
        .to_uppercase();

    let normalized_target = target_fpr.map(|s| s.to_uppercase());
    let is_primary_target = match normalized_target.as_deref() {
        None | Some("") => true,
        Some(fpr) => fpr == primary_fpr,
    };

    let is_enc = matches!(
        secret_key.primary_key.secret_params(),
        SecretParams::Encrypted(_)
    );
    let pwd_bytes = if is_enc {
        fetch_password_with_cache(
            Some(&PASSWORD_CACHE),
            PasswordCachePolicy::Default,
            channel,
            PassphraseStateInternal {
                fpr: primary_fpr.clone(),
                info: "Unlock Primary Key to change expiration".to_string(),
                retry: false,
                ask_for_new: false,
                should_confirm: false,
            },
            fetch_cb,
        )?
    } else {
        Zeroizing::new(Vec::new())
    };
    let pwd = password_from_zeroizing_bytes(pwd_bytes);

    // Check the passphrase before re-signing, so a wrong one is reported as
    // such rather than as whatever the signing step fails with.
    if is_enc {
        require_unlocking_passphrase(&secret_key.primary_key, &pwd, &primary_fpr)?;
    }

    if is_primary_target {
        let created_at = secret_key.primary_key.created_at().as_secs();
        let duration = expiration_duration(created_at, expiration_epoch_secs)?;
        update_primary_expiration(&mut secret_key, duration, &pwd)?;
    } else {
        let target = normalized_target.as_deref().unwrap_or_default();
        let created_at = subkey_entry_mut(
            &mut secret_key.secret_subkeys,
            &mut secret_key.public_subkeys,
            target,
        )
        .map(|(sub, _)| sub.created_at().as_secs())
        .ok_or(GfrStatus::ErrorInvalidInput)?;
        let duration = expiration_duration(created_at, expiration_epoch_secs)?;
        update_subkey_expiration(&mut secret_key, target, duration, &pwd)?;
    }

    export_secret_key(secret_key)
}

/// Stamp an absolute expiration onto the primary key of an in-memory secret key.
///
/// A thin wrapper over [`update_primary_expiration`] that resolves the
/// duration-from-creation form from the primary key's own creation time, so the
/// generation path can express expirations as absolute Unix times just like the
/// FFI. `expiration_epoch_secs == 0` clears the expiration. The primary key must
/// already be unlocked by `pwd` (an empty password for a freshly generated,
/// not-yet-protected key).
pub(crate) fn apply_primary_key_expiration(
    secret_key: &mut SignedSecretKey,
    expiration_epoch_secs: u64,
    pwd: &Password,
) -> Result<(), GfrStatus> {
    let created_at = secret_key.primary_key.created_at().as_secs();
    let duration = expiration_duration(created_at, expiration_epoch_secs)?;
    update_primary_expiration(secret_key, duration, pwd)
}

/// Stamp an absolute expiration onto a single subkey of an in-memory secret key.
///
/// The companion of [`apply_primary_key_expiration`] for subkeys. Re-issuing the
/// binding signature only needs the primary key unlocked by `pwd`; the subkey's
/// own protection is irrelevant. `expiration_epoch_secs == 0` clears the
/// expiration.
pub(crate) fn apply_subkey_expiration(
    secret_key: &mut SignedSecretKey,
    target_fpr: &str,
    expiration_epoch_secs: u64,
    pwd: &Password,
) -> Result<(), GfrStatus> {
    let normalized = target_fpr.to_uppercase();
    let created_at = subkey_entry_mut(
        &mut secret_key.secret_subkeys,
        &mut secret_key.public_subkeys,
        &normalized,
    )
    .map(|(sub, _)| sub.created_at().as_secs())
    .ok_or(GfrStatus::ErrorInvalidInput)?;
    let duration = expiration_duration(created_at, expiration_epoch_secs)?;
    update_subkey_expiration(secret_key, &normalized, duration, pwd)
}

/// Remove a subkey from a secret key block.
///
/// Attempting to delete the primary key fingerprint returns `ErrorInvalidInput`.
pub fn delete_subkey_internal(
    secret_key_block: &str,
    target_subkey_fpr: &str,
) -> Result<GeneratedKeys, GfrStatus> {
    let (mut secret_key, _) = SignedSecretKey::from_string(secret_key_block).map_err(|e| {
        log::error!("Failed to parse secret key block: {}", e);
        GfrStatus::ErrorInvalidData
    })?;

    let fingerprint_str = secret_key.fingerprint().to_string().to_uppercase();
    let subkey_count = |sk: &SignedSecretKey| sk.secret_subkeys.len() + sk.public_subkeys.len();
    let initial_len = subkey_count(&secret_key);

    if secret_key
        .primary_key
        .fingerprint()
        .to_string()
        .to_uppercase()
        == target_subkey_fpr
    {
        return Err(GfrStatus::ErrorInvalidInput);
    }

    secret_key
        .secret_subkeys
        .retain(|sub| sub.key.fingerprint().to_string().to_uppercase() != target_subkey_fpr);
    secret_key
        .public_subkeys
        .retain(|sub| sub.key.fingerprint().to_string().to_uppercase() != target_subkey_fpr);

    if subkey_count(&secret_key) == initial_len {
        return Err(GfrStatus::ErrorInvalidInput);
    }

    let armored_s_key = secret_key
        .to_armored_string(key_armor_opts(secret_key.primary_key.version()))
        .into_gfr()?;

    let public_key = SignedPublicKey::from(secret_key);
    let armored_p_key = public_key
        .to_armored_string(key_armor_opts(public_key.primary_key.version()))
        .into_gfr()?;

    Ok(GeneratedKeys {
        secret: Zeroizing::new(armored_s_key),
        public: armored_p_key,
        fingerprint: fingerprint_str,
    })
}

/// Extract the OpenPGP ECDH KDF parameters of the (sub)key `target_fpr` from a
/// public key block, returned as the hexified 4-byte octet-string
/// `03 01 <kdf-hash> <kek-cipher>` (upper-case, no separators).
///
/// This is exactly what GnuPG's `ecdh_param_str_from_pk` produces and what the
/// gpg-agent `KEYTOCARD` Assuan command expects as its optional `<ecdh>`
/// argument when moving an ECDH encryption subkey onto a smart card. gpgme does
/// not expose these bytes, so we parse them out of the public key here.
///
/// Returns `ErrorInvalidInput` when no (sub)key matches `target_fpr`, and
/// `ErrorInvalidData` when the matched key is not an ECDH key.
pub fn get_ecdh_kdf_params_internal(
    public_key_block: &str,
    target_fpr: &str,
) -> Result<String, GfrStatus> {
    let (public_key, _) = SignedPublicKey::from_string(public_key_block).map_err(|e| {
        log::error!("Failed to parse public key block: {}", e);
        GfrStatus::ErrorInvalidData
    })?;

    let want = target_fpr.to_uppercase();

    // The primary key is virtually never ECDH, but match it too so callers can
    // pass any (sub)key fingerprint uniformly.
    let params = if public_key
        .primary_key
        .fingerprint()
        .to_string()
        .to_uppercase()
        == want
    {
        Some(public_key.primary_key.public_params())
    } else {
        public_key
            .public_subkeys
            .iter()
            .find(|sub| sub.key.fingerprint().to_string().to_uppercase() == want)
            .map(|sub| sub.key.public_params())
    }
    .ok_or(GfrStatus::ErrorInvalidInput)?;

    let PublicParams::ECDH(ecdh) = params else {
        return Err(GfrStatus::ErrorInvalidData);
    };

    // Every ECDH variant carries the KDF hash and the key-wrap (KEK) symmetric
    // algorithm; that pair is the card-relevant part of the KDF parameters.
    let (hash, alg_sym) = match ecdh {
        EcdhPublicParams::Curve25519Legacy { hash, alg_sym, .. }
        | EcdhPublicParams::P256 { hash, alg_sym, .. }
        | EcdhPublicParams::P384 { hash, alg_sym, .. }
        | EcdhPublicParams::P521 { hash, alg_sym, .. }
        | EcdhPublicParams::Brainpool256 { hash, alg_sym, .. }
        | EcdhPublicParams::Brainpool384 { hash, alg_sym, .. }
        | EcdhPublicParams::Brainpool512 { hash, alg_sym, .. }
        | EcdhPublicParams::Unsupported { hash, alg_sym, .. } => (*hash, *alg_sym),
    };

    // `03` = length of the parameter block, `01` = native fingerprint KDF type.
    Ok(format!(
        "0301{:02X}{:02X}",
        u8::from(hash),
        u8::from(alg_sym)
    ))
}

/// Add a revocation self-signature to the subkey at `target_subkey_fpr`.
///
/// Requires unlocking the primary key to sign the revocation; targeting the
/// primary key fingerprint is rejected with `ErrorInvalidInput`.
pub fn revoke_subkey_internal(
    channel: i32,
    secret_key_block: &str,
    target_subkey_fpr: &str,
    reason_code: GfrRevocationCode,
    reason_text: Option<&str>,
    fetch_cb: Option<GfrPasswordFetchCb>,
) -> Result<GeneratedKeys, GfrStatus> {
    if !crate::utils::revocation_reason_fits(reason_code, false) {
        crate::err::set_last_error(
            "a subkey revocation cannot give \"User ID no longer valid\" as its reason",
        );
        return Err(GfrStatus::ErrorInvalidInput);
    }
    let (mut secret_key, _) = SignedSecretKey::from_string(secret_key_block).into_gfr()?;

    let target_fpr = target_subkey_fpr.to_uppercase();
    if subkey_entry_mut(
        &mut secret_key.secret_subkeys,
        &mut secret_key.public_subkeys,
        &target_fpr,
    )
    .is_none()
    {
        return Err(GfrStatus::ErrorInvalidInput);
    }

    if secret_key
        .primary_key
        .fingerprint()
        .to_string()
        .to_uppercase()
        == target_subkey_fpr.to_uppercase()
    {
        return Err(GfrStatus::ErrorInvalidInput);
    }

    let fpr = secret_key.primary_key.fingerprint().to_string();
    let is_enc = matches!(
        secret_key.primary_key.secret_params(),
        SecretParams::Encrypted(_)
    );

    let pwd_bytes = if is_enc {
        fetch_password_with_cache(
            Some(&PASSWORD_CACHE),
            PasswordCachePolicy::Default,
            channel,
            PassphraseStateInternal {
                fpr: fpr.clone(),
                info: "Unlock Primary Key to revoke subkey".to_string(),
                retry: false,
                ask_for_new: false,
                should_confirm: false,
            },
            fetch_cb,
        )?
    } else {
        Zeroizing::new(Vec::new())
    };
    let pwd = password_from_zeroizing_bytes(pwd_bytes);
    require_unlocking_passphrase(&secret_key.primary_key, &pwd, &fpr)?;

    let pk = secret_key.primary_key.public_key();

    let (sub_key, signatures) = subkey_entry_mut(
        &mut secret_key.secret_subkeys,
        &mut secret_key.public_subkeys,
        &target_fpr,
    )
    .ok_or(GfrStatus::ErrorInternal)?;

    // Detect an existing revocation cryptographically, not merely by matching the
    // issuer subpacket: only a subkey-revocation whose signature actually verifies
    // under the primary counts as already-revoked (RFC 9580 §5.2.1.12).
    let already_revoked = is_subkey_revoked(pk, &sub_key, signatures);

    if already_revoked {
        return Err(GfrStatus::ErrorInvalidInput);
    }

    // The signature version MUST match the primary key version: a v6 key MUST
    // produce a v6 signature (RFC 9580 §10.3.2.2), and a non-v6 signature made by
    // a v6 key MUST be ignored by conforming verifiers (§5.2.5). `from_key`
    // selects v4 for a v4 key and v6 (with the required salt) for a v6 key.
    let mut rng = rand::thread_rng();
    let mut cfg = SignatureConfig::from_key(
        &mut rng,
        &secret_key.primary_key,
        SignatureType::SubkeyRevocation,
    )
    .map_err(|_| GfrStatus::ErrorInternal)?;

    push_issuer(&mut cfg, &secret_key.primary_key)?;

    cfg.hashed_subpackets.push(
        Subpacket::regular(SubpacketData::SignatureCreationTime(
            pgp::types::Timestamp::now(),
        ))
        .map_err(|_| GfrStatus::ErrorInternal)?,
    );

    cfg.hashed_subpackets
        .push(build_revocation_reason_subpacket(reason_code, reason_text)?);

    let revoke_sig = cfg
        .sign_subkey_binding(&secret_key.primary_key, pk, &pwd, &sub_key)
        .into_gfr()?;

    signatures.push(revoke_sig);

    let armored_s_key = secret_key
        .to_armored_string(key_armor_opts(secret_key.primary_key.version()))
        .map_err(|_| GfrStatus::ErrorArmorFailed)?;

    let public_key = SignedPublicKey::from(secret_key);
    let armored_p_key = public_key
        .to_armored_string(key_armor_opts(public_key.primary_key.version()))
        .map_err(|_| GfrStatus::ErrorArmorFailed)?;

    Ok(GeneratedKeys {
        secret: Zeroizing::new(armored_s_key),
        public: armored_p_key,
        fingerprint: fpr,
    })
}

/// Generate a standalone revocation certificate for the primary key.
///
/// The certificate is a single `KeyRevocation` signature packet wrapped in a
/// "PUBLIC KEY BLOCK" armor. It can be imported into any key block with the
/// matching fingerprint via `import_rev_cert_internal`. The signature version
/// matches the key version (v4 for v4 keys, v6 for v6 keys); a v6 key MUST NOT
/// emit a v4 signature, as conforming verifiers ignore it (RFC 9580 §5.2.5).
pub fn generate_key_rev_cert_internal(
    channel: i32,
    secret_key_block: &str,
    reason_code: GfrRevocationCode,
    reason_text: Option<&str>,
    fetch_cb: Option<GfrPasswordFetchCb>,
) -> Result<String, GfrStatus> {
    if !crate::utils::revocation_reason_fits(reason_code, false) {
        crate::err::set_last_error(
            "a key revocation cannot give \"User ID no longer valid\" as its reason",
        );
        return Err(GfrStatus::ErrorInvalidInput);
    }
    use pgp::packet::Packet;

    let (skey, _) = SignedSecretKey::from_string(secret_key_block).into_gfr()?;

    let fpr = skey.primary_key.fingerprint().to_string();
    let is_enc = matches!(skey.primary_key.secret_params(), SecretParams::Encrypted(_));

    let pwd_bytes = if is_enc {
        fetch_password_with_cache(
            Some(&PASSWORD_CACHE),
            PasswordCachePolicy::Default,
            channel,
            PassphraseStateInternal {
                fpr: fpr.clone(),
                info: "Generate Key Revocation Certificate".to_string(),
                retry: false,
                ask_for_new: false,
                should_confirm: false,
            },
            fetch_cb,
        )?
    } else {
        Zeroizing::new(Vec::new())
    };
    let pwd = password_from_zeroizing_bytes(pwd_bytes);

    if is_enc {
        let inner = skey
            .primary_key
            .unlock(&pwd, |_, _| Ok(()))
            .map_err(|_| GfrStatus::ErrorPasswordFailed)?;
        inner.map_err(|_| GfrStatus::ErrorPasswordFailed)?;
    }

    let pk = skey.primary_key.public_key();

    // Version-aware: v4 key -> v4 signature, v6 key -> v6 signature (RFC 9580
    // §10.3.2.2 / §5.2.5). See `from_key` at pgp::packet::SignatureConfig.
    let mut rng = rand::thread_rng();
    let mut cfg =
        SignatureConfig::from_key(&mut rng, &skey.primary_key, SignatureType::KeyRevocation)
            .map_err(|_| GfrStatus::ErrorInternal)?;

    push_issuer(&mut cfg, &skey.primary_key)?;

    cfg.hashed_subpackets.push(
        Subpacket::regular(SubpacketData::SignatureCreationTime(
            pgp::types::Timestamp::now(),
        ))
        .map_err(|_| GfrStatus::ErrorInternal)?,
    );

    cfg.hashed_subpackets
        .push(build_revocation_reason_subpacket(reason_code, reason_text)?);

    let revoke_sig = cfg.sign_key(&skey.primary_key, &pwd, &pk).into_gfr()?;

    let mut out = Vec::new();

    // RFC 9580 §10.1.3: "a version 6 revocation certificate MUST include the
    // primary key packet" (§10.1.2: Primary Key, Revocation Signature). A v4
    // one stays the bare signature, as GnuPG writes it.
    if skey.primary_key.version() == KeyVersion::V6 {
        Packet::PublicKey(pk.clone())
            .to_writer(&mut out)
            .into_gfr()?;
    }
    Packet::Signature(revoke_sig)
        .to_writer(&mut out)
        .into_gfr()?;

    let source = MergedRawKeys { packet_bytes: out };

    let mut armored_output = Vec::new();
    let mut headers = BTreeMap::new();
    headers.insert(
        String::from("Comment"),
        vec![String::from("This is a revocation certificate.")],
    );

    armor::write(
        &source,
        BlockType::PublicKey,
        &mut armored_output,
        Some(&headers),
        // RFC 9580 §6.1: a CRC24 footer for a v4 key's revocation (GnuPG
        // interoperability), never for a v6 signature (see `armor_opts_for`).
        is_v4_era(skey.primary_key.version()),
    )
    .into_gfr()?;

    String::from_utf8(armored_output).map_err(|_| GfrStatus::ErrorArmorFailed)
}

fn serialize_bytes<T: Serialize>(obj: &T) -> Result<Vec<u8>, GfrStatus> {
    let mut buf = Vec::new();
    obj.to_writer(&mut buf)
        .map_err(|_| GfrStatus::ErrorInternal)?;
    Ok(buf)
}

fn export_secret_key(sk: SignedSecretKey) -> Result<GeneratedKeys, GfrStatus> {
    let fingerprint = sk.fingerprint().to_string();
    let opts = key_armor_opts(sk.primary_key.version());
    let secret = Zeroizing::new(sk.to_armored_string(opts.clone()).into_gfr()?);
    let public = SignedPublicKey::from(sk)
        .to_armored_string(opts)
        .into_gfr()?;

    Ok(GeneratedKeys {
        secret,
        public,
        fingerprint,
    })
}

fn export_public_key(pk: SignedPublicKey) -> Result<GeneratedKeys, GfrStatus> {
    let fingerprint = pk.fingerprint().to_string();
    let public = pk
        .to_armored_string(key_armor_opts(pk.primary_key.version()))
        .into_gfr()?;

    Ok(GeneratedKeys {
        secret: Zeroizing::new(String::new()),
        public,
        fingerprint,
    })
}

fn extract_key_revocation_signatures(block: &str) -> Result<Vec<Signature>, GfrStatus> {
    let rev_sigs: Vec<Signature> = parse_signatures_from_armor(block)?
        .into_iter()
        .filter(|sig| matches!(sig.typ(), Some(SignatureType::KeyRevocation)))
        .collect();

    if rev_sigs.is_empty() {
        log::error!("No valid KeyRevocation signatures found.");
        Err(GfrStatus::ErrorInvalidInput)
    } else {
        Ok(rev_sigs)
    }
}

/// Most third-party certifications (and other signatures nothing here could
/// verify) a merge leaves on one component. Far above anything a real key
/// carries; what it stops is the key-server flooding pattern, where a copy
/// carries thousands of signatures that are then stored and re-verified on
/// every listing. Verified self-signatures never count against it.
const MAX_MERGED_SIGNATURES: usize = 256;

/// The same limit for revocations (0x20, 0x28, 0x30) nothing here could
/// verify: a budget of their own, so junk certifications can never push one
/// out.
const MAX_MERGED_REVOCATIONS: usize = 256;

/// Most self-signatures rPGP cannot check (unsupported algorithm, hash or
/// version) one component keeps, the stored ones counted first. They admit
/// nothing: a component needs a self-signature that verifies.
const MAX_UNVERIFIABLE_SELF_SIGNATURES: usize = 16;

/// Most public-key verifications one merge or canonicalization performs.
/// Revocations are checked first. A self-claimed signature the budget does
/// not reach is not stored, and the import reports that the limit was
/// reached. A real key needs a few dozen checks; a copy carrying more than
/// this many self-claimed signatures is a flood.
const MAX_SIGNATURE_CHECKS: usize = 4096;

// Importing a key follows sequoia's model (`Cert::merge_public_and_secret`
// and `Cert::canonicalize`, which `sq` builds on). Where the two differ this
// is stricter, except for one deliberate deviation (secret key material).
//
// Equal to sequoia:
// * Stored data is never filtered, only folded.
// * A self-signature is one naming the primary as issuer, or naming no
//   issuer at all. It is checked on its component and re-filed onto any
//   component it verifies on instead.
// * Signatures are the same when they differ only in their unhashed area
//   (sequoia's `normalized_eq`; the MPIs are part of the comparison).
//
// Stricter than sequoia:
// * A self-signature checked and found bad everywhere is not stored (sequoia
//   stores it as "bad"). An issuer-less one is stored as a third-party
//   certification instead, as sequoia stores it, except on a subkey.
// * Incoming User IDs, User Attributes and subkeys are stored only if a
//   self-signature on them verifies, or if they are stored already.
//   Sequoia stores them unbound.
// * A self-signature rPGP cannot check (unsupported algorithm, hash or
//   version) admits no component, and is stored only on a component that
//   is stored or admitted, at most MAX_UNVERIFIABLE_SELF_SIGNATURES per
//   component, the stored ones counted first. Sequoia stores every one.
// * Third-party signatures are capped per component (MAX_MERGED_SIGNATURES,
//   MAX_MERGED_REVOCATIONS); sequoia has no cap. On a subkey only a subkey
//   revocation by a designated revoker (named in a verified
//   self-signature) is stored; sequoia stores every one.
// * At most MAX_SIGNATURE_CHECKS verifications are made, revocations first.
//   A self-claimed signature (one naming this key, or naming no issuer) they
//   do not reach is not stored, and the import reports that the limit was
//   reached. Third-party signatures are not checked and stay under their
//   caps. Sequoia has no such limit.
// * An incoming self-claimed key-level signature of a type no component
//   takes (e.g. 0x02, 0x19, 0x40) is not stored; a stored one stays.
//   Sequoia stores it as "bad".
// * Of a duplicate's unhashed area only an issuer agreeing with one already
//   named, and an embedded primary key binding that verifies, are taken
//   over; sequoia merges the whole area.
//
// Deviation: stored secret key material always wins; an incoming secret
// only fills a key that has none. Sequoia prefers the later copy's secret
// in `merge_public_and_secret` and calls the choice undefined in general;
// its cert store (`MergeCerts::merge_public`) ignores incoming secrets
// altogether. Keeping the stored secret means re-importing an old backup or
// an unprotected export never replaces the stored key or its passphrase
// protection.

/// True if `sig` counts as a self-signature of `primary`, as sequoia counts
/// one: it names `primary` as its issuer, or names no issuer at all.
fn is_self_sig_candidate(sig: &Signature, primary: &PublicKey) -> bool {
    names_primary_as_issuer(sig, primary) || is_issuerless(sig)
}

fn is_issuerless(sig: &Signature) -> bool {
    sig.issuer_fingerprint().is_empty() && sig.issuer_key_id().is_empty()
}

fn is_revocation(sig: &Signature) -> bool {
    matches!(
        sig.typ(),
        Some(
            SignatureType::KeyRevocation
                | SignatureType::SubkeyRevocation
                | SignatureType::CertRevocation
        )
    )
}

/// The bytes of `sig` without its unhashed area: two signatures with the same
/// bytes here are one signature (sequoia's `normalized_eq`). The signature
/// MPIs are part of them. A signature that cannot be rebuilt is compared by
/// all its bytes.
fn normalized_sig_bytes(sig: &Signature) -> Result<Vec<u8>, GfrStatus> {
    let (Some(config), Some(bytes), Some(prefix)) =
        (sig.config(), sig.signature(), sig.signed_hash_value())
    else {
        return serialize_bytes(sig);
    };
    let mut config = config.clone();
    config.unhashed_subpackets.clear();
    match Signature::from_config(config, prefix, bytes.clone()) {
        Ok(bare) => serialize_bytes(&bare),
        Err(_) => serialize_bytes(sig),
    }
}

/// The Key ID a fingerprint stands for (RFC 9580 §5.5.4): the low 64 bits of
/// a v4 fingerprint, the high 64 bits of a v6 one.
fn key_id_of(fpr: &pgp::types::Fingerprint) -> Option<[u8; 8]> {
    key_id_of_fingerprint_bytes(fpr.as_bytes())
}

fn key_id_of_fingerprint_bytes(bytes: &[u8]) -> Option<[u8; 8]> {
    let id = match bytes.len() {
        20 => &bytes[12..],
        32 => &bytes[..8],
        _ => return None,
    };
    id.try_into().ok()
}

/// What checking a signature here can say about it.
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
enum Verdict {
    Good,
    /// Checked, and it does not verify.
    Bad,
    /// Not checkable: an algorithm, hash or version rPGP cannot verify.
    Unverifiable,
    /// Not checked: no check budget left.
    Unchecked,
}

/// True if rPGP can verify a signature made by a key with `params`.
fn engine_can_verify_with(params: &PublicParams) -> bool {
    use pgp::types::{EcdsaPublicParams, EddsaLegacyPublicParams};
    match params {
        PublicParams::RSA(_)
        | PublicParams::DSA(_)
        | PublicParams::Ed25519(_)
        | PublicParams::Ed448(_)
        | PublicParams::MlDsa65Ed25519(_)
        | PublicParams::MlDsa87Ed448(_)
        | PublicParams::SlhDsaShake128s(_)
        | PublicParams::SlhDsaShake128f(_)
        | PublicParams::SlhDsaShake256s(_) => true,
        PublicParams::ECDSA(ecdsa) => !matches!(ecdsa, EcdsaPublicParams::Unsupported { .. }),
        PublicParams::EdDSALegacy(eddsa) => {
            !matches!(eddsa, EddsaLegacyPublicParams::Unsupported { .. })
        }
        _ => false,
    }
}

/// True if rPGP could check `sig`, made by a key with `signer`, at all: its
/// hash and the signer's algorithm are implemented. Says nothing about
/// whether it verifies.
fn checkable(signer: &PublicParams, sig: &Signature) -> bool {
    sig.hash_alg().is_some_and(|h| h.new_hasher().is_ok()) && engine_can_verify_with(signer)
}

/// A shared count of the public-key verifications a merge may still make
/// (see [`MAX_SIGNATURE_CHECKS`]).
struct CheckBudget(std::cell::Cell<usize>, std::cell::Cell<bool>);

impl CheckBudget {
    fn new() -> Self {
        CheckBudget(
            std::cell::Cell::new(MAX_SIGNATURE_CHECKS),
            std::cell::Cell::new(false),
        )
    }

    /// True once a check was refused because the budget was spent.
    fn reached(&self) -> bool {
        self.1.get()
    }

    /// Check `sig`, made by a key with `signer`, with `verify`: what the
    /// result says; [`Verdict::Unverifiable`] without spending anything when
    /// the check cannot be made at all; [`Verdict::Unchecked`] once the
    /// budget is spent.
    fn check(
        &self,
        signer: &PublicParams,
        sig: &Signature,
        verify: impl FnOnce() -> pgp::errors::Result<()>,
    ) -> Verdict {
        if !checkable(signer, sig) {
            return Verdict::Unverifiable;
        }
        let left = self.0.get();
        if left == 0 {
            self.1.set(true);
            return Verdict::Unchecked;
        }
        self.0.set(left - 1);
        match verify() {
            Ok(()) => Verdict::Good,
            Err(
                pgp::errors::Error::Unsupported { .. } | pgp::errors::Error::Unimplemented { .. },
            ) => Verdict::Unverifiable,
            Err(_) => Verdict::Bad,
        }
    }
}

/// How a merge caps the incoming signatures of a component (see
/// [`MAX_MERGED_SIGNATURES`]): `verified` holds the normalized bytes of the
/// incoming self-signatures canonicalization verified, which never count;
/// `unverifiable` those it could not check, capped at
/// [`MAX_UNVERIFIABLE_SELF_SIGNATURES`].
struct CapPolicy<'a> {
    primary: &'a PublicKey,
    verified: &'a HashSet<Vec<u8>>,
    unverifiable: &'a HashSet<Vec<u8>>,
}

/// The rest of what [`union_signatures`] needs for one component.
struct UnionCtx<'a> {
    cap: Option<&'a CapPolicy<'a>>,
    /// For a subkey: its primary and the subkey, to check an embedded primary
    /// key binding before taking it over.
    subkey: Option<(&'a PublicKey, &'a PublicSubkey)>,
    budget: &'a CheckBudget,
}

/// Give `kept` the issuer subpackets of `other`'s unhashed area it lacks, but
/// only one that agrees with an issuer `kept` already names: an Issuer Key ID
/// matching its Issuer Fingerprint, or an Issuer Fingerprint matching its
/// Issuer Key ID when it has no fingerprint.
fn adopt_issuers(kept: &mut Signature, other: &Signature) {
    let Some(config) = other.config() else {
        return;
    };
    let mut adopted = Vec::new();
    for subpacket in &config.unhashed_subpackets {
        match &subpacket.data {
            SubpacketData::IssuerKeyId(id) => {
                let named = kept.issuer_key_id().iter().any(|k| *k == id);
                let agrees = kept
                    .issuer_fingerprint()
                    .iter()
                    .any(|f| key_id_of(f).is_some_and(|k| k[..] == *id.as_ref()));
                if !named && agrees {
                    adopted.push(subpacket.clone());
                }
            }
            SubpacketData::IssuerFingerprint(fpr) => {
                let agrees = key_id_of(fpr)
                    .is_some_and(|k| kept.issuer_key_id().iter().any(|id| *id.as_ref() == k[..]));
                if kept.issuer_fingerprint().is_empty() && agrees {
                    adopted.push(subpacket.clone());
                }
            }
            _ => {}
        }
    }
    for subpacket in adopted {
        if kept.unhashed_subpacket_push(subpacket).is_err() {
            break;
        }
    }
}

/// Give `kept`, a subkey binding (0x18) of `subkey` without an embedded
/// primary key binding (0x19), the one in `other`'s unhashed area, if it
/// verifies. One that does not, or cannot be checked, is not taken.
fn adopt_backsig(
    kept: &mut Signature,
    other: &Signature,
    primary: &PublicKey,
    subkey: &PublicSubkey,
    budget: &CheckBudget,
) {
    if kept.typ() != Some(SignatureType::SubkeyBinding) || kept.embedded_signature().is_some() {
        return;
    }
    let Some(config) = other.config() else {
        return;
    };
    for subpacket in &config.unhashed_subpackets {
        let SubpacketData::EmbeddedSignature(back) = &subpacket.data else {
            continue;
        };
        if back.typ() != Some(SignatureType::KeyBinding) {
            continue;
        }
        let verdict = budget.check(subkey.public_params(), back, || {
            back.verify_primary_key_binding(subkey, primary)
        });
        if verdict == Verdict::Good {
            let _ = kept.unhashed_subpacket_push(subpacket.clone());
            return;
        }
    }
}

/// Append the signatures of `src` that `dst` lacks, comparing them by
/// [`normalized_sig_bytes`]; a duplicate only lends what [`adopt_issuers`]
/// and [`adopt_backsig`] take. With a cap, an incoming signature that is not
/// a verified self-signature is left out once the component holds
/// [`MAX_MERGED_REVOCATIONS`] such revocations or [`MAX_MERGED_SIGNATURES`]
/// other such signatures, and an uncheckable self-signature once it holds
/// [`MAX_UNVERIFIABLE_SELF_SIGNATURES`]. Of the stored signatures only the
/// uncheckable self-signatures and the third-party ones count; which
/// signatures fit is decided in order, the stored ones first.
fn union_signatures(
    dst: &mut Vec<Signature>,
    src: Vec<Signature>,
    ctx: &UnionCtx<'_>,
) -> Result<(), GfrStatus> {
    let mut index: HashMap<Vec<u8>, usize> = HashMap::new();
    let (mut revocations, mut others, mut uncheckable) = (0usize, 0usize, 0usize);
    for (i, sig) in dst.iter().enumerate() {
        index.entry(normalized_sig_bytes(sig)?).or_insert(i);
        if let Some(cap) = ctx.cap {
            if is_self_sig_candidate(sig, cap.primary)
                && !checkable(cap.primary.public_params(), sig)
            {
                uncheckable += 1;
                continue;
            }
            if names_primary_as_issuer(sig, cap.primary) {
                continue;
            }
            if is_revocation(sig) {
                revocations += 1;
            } else {
                others += 1;
            }
        }
    }

    let mut backsig_tried: HashSet<usize> = HashSet::new();
    let mut capped = 0usize;
    for sig in src {
        let key = normalized_sig_bytes(&sig)?;
        if let Some(&i) = index.get(&key) {
            adopt_issuers(&mut dst[i], &sig);
            if let Some((primary, subkey)) = ctx.subkey {
                if backsig_tried.insert(i) {
                    adopt_backsig(&mut dst[i], &sig, primary, subkey, ctx.budget);
                }
            }
            continue;
        }
        if let Some(cap) = ctx.cap {
            if !cap.verified.contains(&key) {
                let (count, limit) = if cap.unverifiable.contains(&key) {
                    (&mut uncheckable, MAX_UNVERIFIABLE_SELF_SIGNATURES)
                } else if is_revocation(&sig) {
                    (&mut revocations, MAX_MERGED_REVOCATIONS)
                } else {
                    (&mut others, MAX_MERGED_SIGNATURES)
                };
                if *count >= limit {
                    capped += 1;
                    continue;
                }
                *count += 1;
            }
        }
        index.insert(key, dst.len());
        dst.push(sig);
    }
    if capped > 0 {
        log::warn!("left out {capped} signatures past the per-component limit");
    }
    Ok(())
}

/// Fold signatures of `sigs` that differ only in their unhashed area into the
/// first of them. Nothing else is dropped.
fn fold_signatures(sigs: &mut Vec<Signature>) -> Result<(), GfrStatus> {
    let src = std::mem::take(sigs);
    let budget = CheckBudget::new();
    union_signatures(
        sigs,
        src,
        &UnionCtx {
            cap: None,
            subkey: None,
            budget: &budget,
        },
    )
}

fn is_uid_certification(sig: &Signature) -> bool {
    matches!(
        sig.typ(),
        Some(
            SignatureType::CertGeneric
                | SignatureType::CertPersona
                | SignatureType::CertCasual
                | SignatureType::CertPositive
        )
    )
}

/// The fingerprints named by the Revocation Key subpackets of `sig`'s hashed
/// area (RFC 9580 §5.2.3.23).
fn named_revokers(sig: &Signature) -> Vec<Vec<u8>> {
    sig.config()
        .map(|config| {
            config
                .hashed_subpackets()
                .filter_map(|p| match &p.data {
                    SubpacketData::RevocationKey(key) => Some(key.fingerprint.to_vec()),
                    _ => None,
                })
                .collect()
        })
        .unwrap_or_default()
}

/// True if `sig` names one of `revokers` as its issuer.
fn issued_by_revoker(sig: &Signature, revokers: &[Vec<u8>]) -> bool {
    revokers.iter().any(|fpr| {
        sig.issuer_fingerprint()
            .iter()
            .any(|f| f.as_bytes() == &fpr[..])
            || key_id_of_fingerprint_bytes(fpr)
                .is_some_and(|id| sig.issuer_key_id().iter().any(|k| *k.as_ref() == id[..]))
    })
}

/// A subkey of a certificate being merged, with or without its secret part.
enum WorkSubkey {
    Public(SignedPublicSubKey),
    Secret(SignedSecretSubKey),
}

impl WorkSubkey {
    fn public(&self) -> &PublicSubkey {
        match self {
            WorkSubkey::Public(sub) => &sub.key,
            WorkSubkey::Secret(sub) => sub.key.public_key(),
        }
    }

    fn signatures_mut(&mut self) -> &mut Vec<Signature> {
        match self {
            WorkSubkey::Public(sub) => &mut sub.signatures,
            WorkSubkey::Secret(sub) => &mut sub.signatures,
        }
    }
}

/// A certificate being merged: its components, and the secret parts held for
/// the primary and each subkey. Built from either key type, it merges all
/// four combinations of public and secret copies the same way.
struct WorkCert {
    primary: PublicKey,
    secret: Option<pgp::packet::SecretKey>,
    details: SignedKeyDetails,
    subkeys: Vec<WorkSubkey>,
    /// Normalized bytes of the self-signatures [`WorkCert::canonicalize`]
    /// verified.
    verified: HashSet<Vec<u8>>,
    /// Normalized bytes of the self-signatures it could not check.
    unverifiable: HashSet<Vec<u8>>,
}

/// The components a stored copy already holds. An incoming copy of one of
/// them is merged into it without needing a self-signature of its own that
/// verifies (see [`WorkCert::canonicalize`]).
#[derive(Default)]
struct StoredComponents {
    users: HashSet<Vec<u8>>,
    attributes: HashSet<Vec<u8>>,
    subkeys: HashSet<Vec<u8>>,
}

/// One component of a [`WorkCert`] a signature can belong to.
#[derive(Clone, Copy, PartialEq, Eq)]
enum Component {
    Key,
    User(usize),
    Attribute(usize),
    Subkey(usize),
}

/// What canonicalization decided for one incoming signature.
#[derive(Clone, Copy, PartialEq, Eq)]
enum Fate {
    /// Verified on the component it arrived on.
    Here,
    /// Verified elsewhere, or nowhere: placed by the re-filing step.
    Refiled,
    /// Stored on the component it arrived on, though not verified there:
    /// someone else's signature, or a self-signature rPGP cannot check.
    Unverified,
    Dropped,
}

impl WorkCert {
    fn new(
        primary: PublicKey,
        secret: Option<pgp::packet::SecretKey>,
        details: SignedKeyDetails,
        subkeys: Vec<WorkSubkey>,
    ) -> Result<Self, GfrStatus> {
        let mut cert = WorkCert {
            primary,
            secret,
            details,
            subkeys,
            verified: HashSet::new(),
            unverifiable: HashSet::new(),
        };
        cert.collapse_repeated_components()?;
        Ok(cert)
    }

    fn from_public(pk: SignedPublicKey) -> Result<Self, GfrStatus> {
        let subkeys = pk
            .public_subkeys
            .into_iter()
            .map(WorkSubkey::Public)
            .collect();
        Self::new(pk.primary_key, None, pk.details, subkeys)
    }

    fn from_secret(sk: SignedSecretKey) -> Result<Self, GfrStatus> {
        let mut subkeys: Vec<WorkSubkey> = sk
            .public_subkeys
            .into_iter()
            .map(WorkSubkey::Public)
            .collect();
        subkeys.extend(sk.secret_subkeys.into_iter().map(WorkSubkey::Secret));
        Self::new(
            sk.primary_key.public_key().clone(),
            Some(sk.primary_key),
            sk.details,
            subkeys,
        )
    }

    /// The first certificate in an armored `block`, secret if it is one.
    fn parse(block: &str) -> Result<Self, GfrStatus> {
        let block = crate::utils::strip_armor_whitespace(block);
        let block = without_trust_packets(&block);
        if let Ok((sk, _)) = SignedSecretKey::from_string(&block) {
            return Self::from_secret(sk);
        }
        let (pk, _) =
            SignedPublicKey::from_string(&block).map_err(|_| GfrStatus::ErrorInvalidInput)?;
        Self::from_public(pk)
    }

    fn fingerprint(&self) -> pgp::types::Fingerprint {
        self.primary.fingerprint()
    }

    /// Merge User IDs, User Attributes and subkeys that appear more than once
    /// into their first occurrence, as sequoia's `sort_and_dedup` does. A
    /// subkey present both with and without its secret part keeps the secret.
    fn collapse_repeated_components(&mut self) -> Result<(), GfrStatus> {
        let users = std::mem::take(&mut self.details.users);
        for user in users {
            match self
                .details
                .users
                .iter_mut()
                .find(|u| u.id.id() == user.id.id())
            {
                Some(first) => first.signatures.extend(user.signatures),
                None => self.details.users.push(user),
            }
        }

        let attrs = std::mem::take(&mut self.details.user_attributes);
        for attr in attrs {
            match self
                .details
                .user_attributes
                .iter_mut()
                .find(|a| a.attr == attr.attr)
            {
                Some(first) => first.signatures.extend(attr.signatures),
                None => self.details.user_attributes.push(attr),
            }
        }

        let subkeys = std::mem::take(&mut self.subkeys);
        let budget = CheckBudget::new();
        for sub in subkeys {
            self.absorb_subkey(sub, None, &budget)?;
        }
        Ok(())
    }

    /// Add `sub`, or merge it into the subkey with its fingerprint: their
    /// signatures are united, and a secret part fills one that is missing.
    fn absorb_subkey(
        &mut self,
        sub: WorkSubkey,
        cap: Option<&CapPolicy<'_>>,
        budget: &CheckBudget,
    ) -> Result<(), GfrStatus> {
        let fpr = sub.public().fingerprint();
        let Some(pos) = self
            .subkeys
            .iter()
            .position(|s| s.public().fingerprint() == fpr)
        else {
            self.subkeys.push(sub);
            return Ok(());
        };
        let (incoming_sigs, incoming_secret) = match sub {
            WorkSubkey::Public(sub) => (sub.signatures, None),
            WorkSubkey::Secret(sub) => (sub.signatures, Some(sub.key)),
        };
        let primary = &self.primary;
        let existing = &mut self.subkeys[pos];
        if let (WorkSubkey::Public(public), Some(key)) = (&mut *existing, incoming_secret) {
            let signatures = std::mem::take(&mut public.signatures);
            *existing = WorkSubkey::Secret(SignedSecretSubKey { key, signatures });
        }
        let subkey = existing.public().clone();
        union_signatures(
            existing.signatures_mut(),
            incoming_sigs,
            &UnionCtx {
                cap,
                subkey: Some((primary, &subkey)),
                budget,
            },
        )
    }

    /// True if a signature of `sig`'s type can belong to `component`.
    fn fits(sig: &Signature, component: Component) -> bool {
        match component {
            Component::Key => matches!(
                sig.typ(),
                Some(SignatureType::Key | SignatureType::KeyRevocation)
            ),
            Component::User(_) | Component::Attribute(_) => {
                is_uid_certification(sig) || sig.typ() == Some(SignatureType::CertRevocation)
            }
            Component::Subkey(_) => matches!(
                sig.typ(),
                Some(SignatureType::SubkeyBinding | SignatureType::SubkeyRevocation)
            ),
        }
    }

    /// Check `sig`, a self-signature whose type fits `component`, there.
    fn check_on(&self, sig: &Signature, component: Component, budget: &CheckBudget) -> Verdict {
        let primary = &self.primary;
        budget.check(primary.public_params(), sig, || match component {
            Component::Key => sig.verify_key(primary),
            Component::User(i) => {
                sig.verify_certification(primary, Tag::UserId, &self.details.users[i].id)
            }
            Component::Attribute(i) => sig.verify_certification(
                primary,
                Tag::UserAttribute,
                &self.details.user_attributes[i].attr,
            ),
            Component::Subkey(i) => sig.verify_subkey_binding(primary, self.subkeys[i].public()),
        })
    }

    /// True if `sig`, a verified or unverifiable self-signature on
    /// `component`, binds it: a certification of a User ID or User
    /// Attribute, a binding or revocation of a subkey.
    fn binds(sig: &Signature, component: Component) -> bool {
        match component {
            Component::Key => true,
            Component::User(_) | Component::Attribute(_) => is_uid_certification(sig),
            Component::Subkey(_) => Self::fits(sig, component),
        }
    }

    fn components(&self) -> Vec<Component> {
        let mut all = vec![Component::Key];
        all.extend((0..self.details.users.len()).map(Component::User));
        all.extend((0..self.details.user_attributes.len()).map(Component::Attribute));
        all.extend((0..self.subkeys.len()).map(Component::Subkey));
        all
    }

    fn take_signatures(&mut self, component: Component) -> Vec<Signature> {
        match component {
            Component::Key => {
                let mut sigs = std::mem::take(&mut self.details.direct_signatures);
                sigs.append(&mut self.details.revocation_signatures);
                sigs
            }
            Component::User(i) => std::mem::take(&mut self.details.users[i].signatures),
            Component::Attribute(i) => {
                std::mem::take(&mut self.details.user_attributes[i].signatures)
            }
            Component::Subkey(i) => std::mem::take(self.subkeys[i].signatures_mut()),
        }
    }

    fn push_signature(&mut self, component: Component, sig: Signature) {
        match component {
            Component::Key if sig.typ() == Some(SignatureType::KeyRevocation) => {
                self.details.revocation_signatures.push(sig)
            }
            Component::Key => self.details.direct_signatures.push(sig),
            Component::User(i) => self.details.users[i].signatures.push(sig),
            Component::Attribute(i) => self.details.user_attributes[i].signatures.push(sig),
            Component::Subkey(i) => self.subkeys[i].signatures_mut().push(sig),
        }
    }

    /// The designated revokers this certificate names in self-signatures on
    /// the key or a User ID that verify. Only signatures carrying a
    /// Revocation Key subpacket are checked.
    fn designated_revokers(&self, budget: &CheckBudget) -> Vec<Vec<u8>> {
        let mut revokers = Vec::new();
        let key_sigs = self
            .details
            .direct_signatures
            .iter()
            .chain(&self.details.revocation_signatures)
            .map(|sig| (Component::Key, sig));
        let uid_sigs = self.details.users.iter().enumerate().flat_map(|(i, u)| {
            u.signatures
                .iter()
                .map(move |sig| (Component::User(i), sig))
        });
        for (component, sig) in key_sigs.chain(uid_sigs) {
            let named = named_revokers(sig);
            if named.is_empty()
                || !names_primary_as_issuer(sig, &self.primary)
                || !Self::fits(sig, component)
            {
                continue;
            }
            if self.check_on(sig, component, budget) == Verdict::Good {
                revokers.extend(named);
            }
        }
        revokers
    }

    /// Sequoia's `canonicalize`, made stricter, for a certificate that is
    /// not stored yet (see the notes above [`is_self_sig_candidate`]).
    /// `known_revokers` are the designated revokers the stored copy names.
    fn canonicalize(
        &mut self,
        known_revokers: &[Vec<u8>],
        stored: &StoredComponents,
        budget: &CheckBudget,
    ) -> Result<(), GfrStatus> {
        let primary = self.primary.clone();
        let components = self.components();
        let pools: Vec<Vec<Signature>> = components
            .iter()
            .map(|&c| self.take_signatures(c))
            .collect();

        // The self-signature candidates, revocations first, so the check
        // budget reaches them before anything else.
        let mut candidates: Vec<(usize, usize)> = Vec::new();
        for (c, pool) in pools.iter().enumerate() {
            for (i, sig) in pool.iter().enumerate() {
                if is_self_sig_candidate(sig, &primary) {
                    candidates.push((c, i));
                }
            }
        }
        candidates.sort_by_key(|&(c, i)| !is_revocation(&pools[c][i]));

        let mut fates: Vec<Vec<Fate>> = pools
            .iter()
            .map(|pool| vec![Fate::Unverified; pool.len()])
            .collect();
        // Signatures rPGP cannot check at all (see [`Verdict::Unverifiable`]).
        let mut uncheckable: Vec<Vec<bool>> =
            pools.iter().map(|pool| vec![false; pool.len()]).collect();
        let mut refiled: Vec<(usize, Signature)> = Vec::new();
        let mut bound = vec![false; components.len()];
        let mut misplaced: Vec<(usize, usize)> = Vec::new();

        for &(c, i) in &candidates {
            let sig = &pools[c][i];
            if !Self::fits(sig, components[c]) {
                misplaced.push((c, i));
                continue;
            }
            match self.check_on(sig, components[c], budget) {
                Verdict::Good => {
                    fates[c][i] = Fate::Here;
                    bound[c] |= Self::binds(sig, components[c]);
                }
                Verdict::Bad => misplaced.push((c, i)),
                Verdict::Unverifiable => {
                    // Stored on its component, if that component is stored
                    // or admitted by a signature that verifies, and capped at
                    // MAX_UNVERIFIABLE_SELF_SIGNATURES. It admits nothing.
                    fates[c][i] = Fate::Unverified;
                    uncheckable[c][i] = true;
                }
                Verdict::Unchecked => {
                    // Past the check budget: not stored.
                    fates[c][i] = Fate::Dropped;
                }
            }
        }

        for (c, i) in misplaced {
            let sig = &pools[c][i];
            let mut found = Vec::new();
            let (mut unknown, mut unchecked) = (false, false);
            for (d, &component) in components.iter().enumerate() {
                if d == c && Self::fits(sig, components[c]) {
                    continue; // already checked there
                }
                if !Self::fits(sig, component) {
                    continue;
                }
                match self.check_on(sig, component, budget) {
                    Verdict::Good => found.push(d),
                    Verdict::Bad => {}
                    Verdict::Unverifiable => unknown = true,
                    Verdict::Unchecked => unchecked = true,
                }
            }
            let fits_somewhere = components.iter().any(|&d| Self::fits(sig, d));
            if !found.is_empty() {
                fates[c][i] = Fate::Refiled;
                for d in found {
                    bound[d] |= Self::binds(sig, components[d]);
                    refiled.push((d, sig.clone()));
                }
            } else if unchecked || !fits_somewhere {
                // Not checked everywhere it could belong for want of budget,
                // or a type no component takes: not stored.
                fates[c][i] = Fate::Dropped;
            } else if unknown {
                fates[c][i] = Fate::Unverified;
                uncheckable[c][i] = true;
            } else if is_issuerless(sig) && !matches!(components[c], Component::Subkey(_)) {
                // Checked and failed as a self-signature, but it names no
                // issuer: someone else's certification, as sequoia keeps it.
                fates[c][i] = Fate::Unverified;
            } else {
                log::warn!("dropping a self-signature that does not verify");
                fates[c][i] = Fate::Dropped;
            }
        }

        // Designated revokers, from the stored copy and from this one's
        // verified self-signatures.
        let mut revokers = known_revokers.to_vec();
        for (c, pool) in pools.iter().enumerate() {
            if !matches!(components[c], Component::Key | Component::User(_)) {
                continue;
            }
            for (i, sig) in pool.iter().enumerate() {
                if fates[c][i] == Fate::Here {
                    revokers.extend(named_revokers(sig));
                }
            }
        }
        for (d, sig) in &refiled {
            if matches!(components[*d], Component::Key | Component::User(_)) {
                revokers.extend(named_revokers(sig));
            }
        }

        for (c, pool) in pools.into_iter().enumerate() {
            let component = components[c];
            for (i, sig) in pool.into_iter().enumerate() {
                if !is_self_sig_candidate(&sig, &primary) {
                    // Someone else's signature: kept unchecked, as there is
                    // no key here to check it with. On a subkey only a
                    // designated revoker's revocation means anything.
                    let keep = !matches!(component, Component::Subkey(_))
                        || (sig.typ() == Some(SignatureType::SubkeyRevocation)
                            && issued_by_revoker(&sig, &revokers));
                    if keep {
                        self.push_signature(component, sig);
                    }
                    continue;
                }
                match fates[c][i] {
                    Fate::Here => {
                        self.verified.insert(normalized_sig_bytes(&sig)?);
                        self.push_signature(component, sig);
                    }
                    Fate::Unverified => {
                        if uncheckable[c][i] {
                            self.unverifiable.insert(normalized_sig_bytes(&sig)?);
                        }
                        self.push_signature(component, sig);
                    }
                    Fate::Refiled | Fate::Dropped => {}
                }
            }
        }
        for (d, sig) in refiled {
            self.verified.insert(normalized_sig_bytes(&sig)?);
            self.push_signature(components[d], sig);
        }

        // A component no self-signature that verifies binds is not the key
        // holder's, unless it is stored already.
        for (c, &component) in components.iter().enumerate() {
            bound[c] |= match component {
                Component::Key => true,
                Component::User(i) => stored.users.contains(self.details.users[i].id.id()),
                Component::Attribute(i) => stored
                    .attributes
                    .contains(&serialize_bytes(&self.details.user_attributes[i].attr)?),
                Component::Subkey(i) => stored
                    .subkeys
                    .contains(self.subkeys[i].public().fingerprint().as_bytes()),
            };
        }
        let dropped_before = (
            self.details.users.len(),
            self.details.user_attributes.len(),
            self.subkeys.len(),
        );
        let mut keep = components
            .iter()
            .zip(&bound)
            .filter(|(c, _)| !matches!(c, Component::Key))
            .map(|(_, &b)| b);
        self.details.users.retain(|_| keep.next().unwrap_or(false));
        self.details
            .user_attributes
            .retain(|_| keep.next().unwrap_or(false));
        self.subkeys.retain(|_| keep.next().unwrap_or(false));
        if dropped_before
            != (
                self.details.users.len(),
                self.details.user_attributes.len(),
                self.subkeys.len(),
            )
        {
            log::warn!("dropping User IDs, User Attributes or subkeys the key holder never bound");
        }

        let verified = std::mem::take(&mut self.verified);
        let unverifiable = std::mem::take(&mut self.unverifiable);
        let cap = CapPolicy {
            primary: &primary,
            verified: &verified,
            unverifiable: &unverifiable,
        };
        self.fold(Some(&cap), budget)?;
        self.verified = verified;
        self.unverifiable = unverifiable;
        Ok(())
    }

    /// The identities of this copy's components, for canonicalizing another
    /// copy against it.
    fn stored_components(&self) -> Result<StoredComponents, GfrStatus> {
        let mut stored = StoredComponents::default();
        for user in &self.details.users {
            stored.users.insert(user.id.id().to_vec());
        }
        for attr in &self.details.user_attributes {
            stored.attributes.insert(serialize_bytes(&attr.attr)?);
        }
        for sub in &self.subkeys {
            stored
                .subkeys
                .insert(sub.public().fingerprint().as_bytes().to_vec());
        }
        Ok(stored)
    }

    /// Fold every component's duplicate signatures; with `cap`, also cap
    /// the ones that are not verified self-signatures (see
    /// [`union_signatures`]).
    fn fold(&mut self, cap: Option<&CapPolicy<'_>>, budget: &CheckBudget) -> Result<(), GfrStatus> {
        for component in self.components() {
            let sigs = self.take_signatures(component);
            let mut kept = Vec::with_capacity(sigs.len());
            let subkey = match component {
                Component::Subkey(i) => Some(self.subkeys[i].public().clone()),
                _ => None,
            };
            union_signatures(
                &mut kept,
                sigs,
                &UnionCtx {
                    cap,
                    subkey: subkey.as_ref().map(|s| (&self.primary, s)),
                    budget,
                },
            )?;
            for sig in kept {
                self.push_signature(component, sig);
            }
        }
        Ok(())
    }

    /// Merge `other`, a copy of the same certificate, into this one. Nothing
    /// of this one is dropped; with `cap`, `other`'s signatures that are not
    /// verified self-signatures are capped, which is right only once `other`
    /// is canonicalized.
    fn absorb(
        &mut self,
        other: WorkCert,
        cap: bool,
        budget: &CheckBudget,
    ) -> Result<(), GfrStatus> {
        if self.fingerprint() != other.fingerprint() {
            return Err(GfrStatus::ErrorInvalidInput);
        }
        let primary = self.primary.clone();
        let policy = CapPolicy {
            primary: &primary,
            verified: &other.verified,
            unverifiable: &other.unverifiable,
        };
        let cap = cap.then_some(&policy);
        let plain = UnionCtx {
            cap,
            subkey: None,
            budget,
        };

        if self.secret.is_none() {
            self.secret = other.secret;
        }

        union_signatures(
            &mut self.details.direct_signatures,
            other.details.direct_signatures,
            &plain,
        )?;
        union_signatures(
            &mut self.details.revocation_signatures,
            other.details.revocation_signatures,
            &plain,
        )?;

        for user in other.details.users {
            match self
                .details
                .users
                .iter_mut()
                .find(|u| u.id.id() == user.id.id())
            {
                Some(dst) => union_signatures(&mut dst.signatures, user.signatures, &plain)?,
                None => self.details.users.push(user),
            }
        }

        for attr in other.details.user_attributes {
            match self
                .details
                .user_attributes
                .iter_mut()
                .find(|a| a.attr == attr.attr)
            {
                Some(dst) => union_signatures(&mut dst.signatures, attr.signatures, &plain)?,
                None => self.details.user_attributes.push(attr),
            }
        }

        for sub in other.subkeys {
            self.absorb_subkey(sub, cap, budget)?;
        }
        Ok(())
    }

    /// The certificate as rPGP's key types: secret if the primary's secret
    /// part is held.
    fn into_signed(self) -> Result<SignedSecretKeyOrPublic, GfrStatus> {
        let mut public_subkeys = Vec::new();
        let mut secret_subkeys = Vec::new();
        for sub in self.subkeys {
            match sub {
                WorkSubkey::Public(sub) => public_subkeys.push(sub),
                WorkSubkey::Secret(sub) => secret_subkeys.push(sub),
            }
        }
        Ok(match self.secret {
            Some(secret) => SignedSecretKeyOrPublic::Secret(SignedSecretKey::new(
                secret,
                self.details,
                public_subkeys,
                secret_subkeys,
            )),
            None => {
                // A secret subkey without the primary's secret part cannot
                // arise from a merge; kept as public if it ever does.
                public_subkeys.extend(secret_subkeys.into_iter().map(SignedPublicSubKey::from));
                SignedSecretKeyOrPublic::Public(SignedPublicKey::new(
                    self.primary,
                    self.details,
                    public_subkeys,
                ))
            }
        })
    }

    fn export(self) -> Result<GeneratedKeys, GfrStatus> {
        match self.into_signed()? {
            SignedSecretKeyOrPublic::Secret(sk) => export_secret_key(sk),
            SignedSecretKeyOrPublic::Public(pk) => export_public_key(pk),
        }
    }
}

enum SignedSecretKeyOrPublic {
    Secret(SignedSecretKey),
    Public(SignedPublicKey),
}

/// What importing a certificate produced.
pub struct ImportOutcome {
    pub keys: GeneratedKeys,
    /// True if [`MAX_SIGNATURE_CHECKS`] was reached: self-claimed signatures
    /// past it were not checked and not stored.
    pub check_limit_reached: bool,
}

/// Merge `incoming_block` into `base_block`, two armored copies of the same
/// certificate (any combination of secret and public).
///
/// `base_block` is the stored copy and is never filtered, only folded.
/// `incoming_block` is canonicalized first (see the notes above
/// [`is_self_sig_candidate`]). At most [`MAX_SIGNATURE_CHECKS`] signatures
/// are verified in all. Returns `ErrorInvalidInput` if the fingerprints
/// don't match.
pub fn merge_key_block_report(
    base_block: &str,
    incoming_block: &str,
) -> Result<ImportOutcome, GfrStatus> {
    let mut base = WorkCert::parse(base_block)?;
    let mut incoming = WorkCert::parse(incoming_block)?;
    if base.fingerprint() != incoming.fingerprint() {
        return Err(GfrStatus::ErrorInvalidInput);
    }
    let budget = CheckBudget::new();
    let stored_revokers = base.designated_revokers(&budget);
    let stored = base.stored_components()?;
    incoming.canonicalize(&stored_revokers, &stored, &budget)?;
    base.fold(None, &budget)?;
    base.absorb(incoming, true, &budget)?;
    Ok(ImportOutcome {
        keys: base.export()?,
        check_limit_reached: budget.reached(),
    })
}

/// [`merge_key_block_report`] without the report.
#[cfg(test)]
pub fn merge_key_block_internal(
    base_block: &str,
    incoming_block: &str,
) -> Result<GeneratedKeys, GfrStatus> {
    merge_key_block_report(base_block, incoming_block).map(|outcome| outcome.keys)
}

/// Canonicalize the first certificate of an armored `block` that is about to
/// be stored for the first time, as sequoia does on parsing one (see the
/// notes above [`is_self_sig_candidate`]).
pub fn canonicalize_key_block_report(block: &str) -> Result<ImportOutcome, GfrStatus> {
    let mut cert = WorkCert::parse(block)?;
    let budget = CheckBudget::new();
    cert.canonicalize(&[], &StoredComponents::default(), &budget)?;
    Ok(ImportOutcome {
        keys: cert.export()?,
        check_limit_reached: budget.reached(),
    })
}

/// [`canonicalize_key_block_report`] without the report.
#[cfg(test)]
pub fn canonicalize_key_block_internal(block: &str) -> Result<GeneratedKeys, GfrStatus> {
    canonicalize_key_block_report(block).map(|outcome| outcome.keys)
}

fn parse_signatures_from_armor(block: &str) -> Result<Vec<Signature>, GfrStatus> {
    // Every Signature packet of every armor block; other packets (a v6
    // revocation certificate's primary key, RFC 9580 §10.1.2) are skipped.
    let sigs = crate::crypto::parse_all_signature_packets(block.as_bytes());
    if sigs.is_empty() {
        Err(GfrStatus::ErrorInvalidInput)
    } else {
        Ok(sigs)
    }
}

/// Apply a revocation certificate to a key block.
///
/// The certificate's issuer fingerprint must match the base key's primary key;
/// mismatched fingerprints return `ErrorInvalidInput`. Duplicate revocation
/// signatures are deduplicated before the key is re-exported.
pub fn import_rev_cert_internal(
    base_key_block: &str,
    rev_cert_block: &str,
) -> Result<GeneratedKeys, GfrStatus> {
    let rev_sigs = extract_key_revocation_signatures(rev_cert_block)?;

    if let Ok((mut base_sk, _)) = SignedSecretKey::from_string(base_key_block) {
        // Store only revocations that cryptographically verify under the base
        // primary key (RFC 9580 §5.2.3.31), not ones merely naming it in an Issuer
        // subpacket: a fabricated revocation cert must never be imported.
        let primary_pub = base_sk.primary_key.public_key();
        let verified: Vec<Signature> = rev_sigs
            .iter()
            .filter(|sig| {
                matches!(sig.typ(), Some(SignatureType::KeyRevocation))
                    && sig.verify_key(primary_pub).is_ok()
            })
            .cloned()
            .collect();
        if verified.is_empty() {
            return Err(GfrStatus::ErrorInvalidInput);
        }

        base_sk.details.revocation_signatures.extend(verified);
        fold_signatures(&mut base_sk.details.revocation_signatures)?;
        fold_signatures(&mut base_sk.details.direct_signatures)?;

        return export_secret_key(base_sk);
    }

    if let Ok((mut base_pk, _)) = SignedPublicKey::from_string(base_key_block) {
        // Same cryptographic gate as the secret-key branch above.
        let verified: Vec<Signature> = rev_sigs
            .iter()
            .filter(|sig| {
                matches!(sig.typ(), Some(SignatureType::KeyRevocation))
                    && sig.verify_key(&base_pk.primary_key).is_ok()
            })
            .cloned()
            .collect();
        if verified.is_empty() {
            return Err(GfrStatus::ErrorInvalidInput);
        }

        base_pk.details.revocation_signatures.extend(verified);
        fold_signatures(&mut base_pk.details.revocation_signatures)?;
        fold_signatures(&mut base_pk.details.direct_signatures)?;

        return export_public_key(base_pk);
    }

    Err(GfrStatus::ErrorInvalidInput)
}

/// Extract the fingerprint of the key targeted by a revocation certificate.
pub fn extract_rev_cert_target_fpr_internal(rev_cert_block: &str) -> Result<String, GfrStatus> {
    // The first revocation that names its issuer, by fingerprint or (legacy
    // v4) Key ID; the host resolves a key by either.
    let rev_sigs = extract_key_revocation_signatures(rev_cert_block)?;
    rev_sigs
        .iter()
        .find_map(|sig| {
            sig.issuer_fingerprint()
                .first()
                .map(|fp| fp.to_string())
                .or_else(|| sig.issuer_key_id().first().map(|id| id.to_string()))
        })
        .ok_or(GfrStatus::ErrorInvalidInput)
}

#[cfg(test)]
mod rfc9580_tests {
    //! RFC 9580 conformance tests for revocation signatures and Appendix A
    //! known-answer vectors.
    use super::*;
    use crate::keygen::keygen_dynamic;
    use crate::testutil::keys::cfg;
    use crate::testutil::rfc9580;
    use crate::types::{GfrKeyAlgo, GfrOpenPGPKeyVersion};

    /// RFC 9580 §10.3.2.2 / §5.2.5: a v6 key MUST produce v6 signatures. A subkey
    /// revocation on a v6 key must be a v6 signature (a v4 one would be ignored by
    /// conforming verifiers, silently failing the revocation).
    #[test]
    fn v6_subkey_revocation_is_v6_signature() {
        let primary = cfg(GfrKeyAlgo::ED25519, true, false, GfrOpenPGPKeyVersion::V6);
        let sub = cfg(GfrKeyAlgo::ED25519, false, true, GfrOpenPGPKeyVersion::V6);
        let key = keygen_dynamic("rev <rev@example.com>", &primary, &[sub]).expect("keygen");
        assert_eq!(key.primary_key.version(), KeyVersion::V6);

        let sub_fpr = key.secret_subkeys[0].key.fingerprint().to_string();
        let armored = key.to_armored_string(armor_opts()).expect("armor");

        let result = revoke_subkey_internal(
            0,
            &armored,
            &sub_fpr,
            GfrRevocationCode::Compromised,
            None,
            None,
        )
        .expect("revoke");

        let (revoked, _) = SignedSecretKey::from_string(&result.secret).expect("parse");
        let rev_sig = revoked.secret_subkeys[0]
            .signatures
            .iter()
            .find(|s| matches!(s.typ(), Some(SignatureType::SubkeyRevocation)))
            .expect("subkey revocation signature present");
        assert_eq!(
            rev_sig.version(),
            pgp::packet::SignatureVersion::V6,
            "revocation signature on a v6 key must be a v6 signature"
        );
    }

    /// Sanity check: a v4 key still produces v4 revocation signatures (no regression).
    #[test]
    fn v4_subkey_revocation_is_v4_signature() {
        let primary = cfg(GfrKeyAlgo::ED25519, true, false, GfrOpenPGPKeyVersion::V4);
        let sub = cfg(GfrKeyAlgo::ED25519, false, true, GfrOpenPGPKeyVersion::V4);
        let key = keygen_dynamic("rev <rev@example.com>", &primary, &[sub]).expect("keygen");
        let sub_fpr = key.secret_subkeys[0].key.fingerprint().to_string();
        let armored = key.to_armored_string(armor_opts()).expect("armor");

        let result = revoke_subkey_internal(
            0,
            &armored,
            &sub_fpr,
            GfrRevocationCode::Retired,
            None,
            None,
        )
        .expect("revoke");
        let (revoked, _) = SignedSecretKey::from_string(&result.secret).expect("parse");
        let rev_sig = revoked.secret_subkeys[0]
            .signatures
            .iter()
            .find(|s| matches!(s.typ(), Some(SignatureType::SubkeyRevocation)))
            .expect("subkey revocation signature present");
        assert_eq!(rev_sig.version(), pgp::packet::SignatureVersion::V4);
    }

    /// C: `import_rev_cert_internal` stores a revocation only if it
    /// cryptographically verifies under the base key's primary (RFC 9580
    /// §5.2.3.31), not merely because it names the primary in an Issuer subpacket.
    /// A genuine self-revocation is honored; a revocation whose signature does not
    /// verify under the base primary (here, one produced by a different key) is
    /// rejected rather than silently stored.
    #[test]
    fn import_rev_cert_requires_a_verifying_signature() {
        let primary = cfg(GfrKeyAlgo::ED25519, true, false, GfrOpenPGPKeyVersion::V4);

        let key_a = keygen_dynamic("a <a@example.com>", &primary, &[]).expect("keygen a");
        let a_block = key_a.to_armored_string(armor_opts()).expect("armor a");

        let key_b = keygen_dynamic("b <b@example.com>", &primary, &[]).expect("keygen b");
        let b_block = key_b.to_armored_string(armor_opts()).expect("armor b");

        // A's own self-revocation, imported onto A: accepted and honored.
        let a_rev =
            generate_key_rev_cert_internal(0, &a_block, GfrRevocationCode::Compromised, None, None)
                .expect("gen a rev");
        let imported = import_rev_cert_internal(&a_block, &a_rev).expect("import genuine");
        let (revoked, _) = SignedSecretKey::from_string(&imported.secret).expect("parse imported");
        assert!(
            is_primary_key_revoked(
                revoked.primary_key.public_key(),
                &revoked.details.revocation_signatures
            ),
            "a genuine self-revocation must be stored and honored"
        );

        // B's genuine revocation does NOT verify under A's primary: importing it
        // onto A must be rejected (its signature is not from A).
        let b_rev =
            generate_key_rev_cert_internal(0, &b_block, GfrRevocationCode::Compromised, None, None)
                .expect("gen b rev");
        assert!(
            import_rev_cert_internal(&a_block, &b_rev).is_err(),
            "a revocation that does not verify under the base primary must be rejected"
        );
    }

    // ---- RFC 9580 Appendix A known-answer vectors ----
    //
    // The blobs themselves live in `crate::testutil::rfc9580`, transcribed
    // verbatim from the RFC so every module can reuse them.

    /// The v6 certificate parses and yields the fingerprint stated in the RFC.
    #[test]
    fn appendix_a3_v6_cert_fingerprint() {
        let (cert, _) = SignedPublicKey::from_string(rfc9580::A3_V6_CERT).expect("parse A.3 cert");
        assert_eq!(cert.primary_key.version(), KeyVersion::V6);
        assert_eq!(
            cert.primary_key.fingerprint().to_string().to_uppercase(),
            rfc9580::A3_PRIMARY_FINGERPRINT
        );
    }

    /// The Appendix A.6 cleartext message verifies against the A.3 certificate,
    /// and its signature is a v6 signature — exercising the whole v6 verify path.
    #[test]
    fn appendix_a6_cleartext_verifies() {
        use pgp::composed::{CleartextSignedMessage, Deserializable};
        let (cert, _) = SignedPublicKey::from_string(rfc9580::A3_V6_CERT).expect("parse A.3 cert");
        let (msg, _) =
            CleartextSignedMessage::from_string(rfc9580::A6_CLEARTEXT).expect("parse A.6");
        msg.verify(&cert)
            .expect("A.6 cleartext signature must verify against A.3 cert");
        assert!(
            msg.signatures()
                .iter()
                .all(|s| s.version() == pgp::packet::SignatureVersion::V6),
            "Appendix A.6 signature must be a v6 signature"
        );
    }

    /// The ECDH KDF extractor returns the `03 01 <hash> <cipher>` octet-string of
    /// a Curve25519 encryption subkey, hex-encoded — the value gpg-agent's
    /// KEYTOCARD needs. It must agree, byte for byte, with the trailing KDF bytes
    /// the crate itself serializes for that subkey (the authoritative encoding).
    #[test]
    fn ecdh_kdf_params_match_serialized_bytes() {
        let primary = cfg(GfrKeyAlgo::ED25519, true, false, GfrOpenPGPKeyVersion::V4);
        let sub = cfg(GfrKeyAlgo::CV25519, false, true, GfrOpenPGPKeyVersion::V4);
        let key = keygen_dynamic("ecdh <ecdh@example.com>", &primary, &[sub]).expect("keygen");

        let sub_fpr = key.secret_subkeys[0].key.fingerprint().to_string();
        let public = SignedPublicKey::from(key);
        let pub_block = public.to_armored_string(armor_opts()).expect("armor pub");

        let hex = get_ecdh_kdf_params_internal(&pub_block, &sub_fpr).expect("ecdh params");

        // Independently recompute the expected tail from the crate's own serializer.
        let params = public.public_subkeys[0].key.public_params();
        let PublicParams::ECDH(ecdh) = params else {
            panic!("expected an ECDH subkey");
        };
        let mut buf = Vec::new();
        ecdh.to_writer(&mut buf).expect("serialize ecdh params");
        let expected = buf[buf.len() - 4..]
            .iter()
            .map(|b| format!("{:02X}", b))
            .collect::<String>();

        assert_eq!(hex, expected);
        assert!(hex.starts_with("0301"), "native KDF prefix, got {hex}");
        assert_eq!(hex.len(), 8, "four hex-encoded octets");
    }

    /// Asking for a non-ECDH (sub)key is a data error, and an unknown fingerprint
    /// is an input error — callers must not silently get a bogus blob.
    #[test]
    fn ecdh_kdf_params_reject_non_ecdh_and_unknown() {
        let primary = cfg(GfrKeyAlgo::ED25519, true, false, GfrOpenPGPKeyVersion::V4);
        let sub = cfg(GfrKeyAlgo::ED25519, true, false, GfrOpenPGPKeyVersion::V4);
        let key = keygen_dynamic("sig <sig@example.com>", &primary, &[sub]).expect("keygen");

        let sub_fpr = key.secret_subkeys[0].key.fingerprint().to_string();
        let pub_block = SignedPublicKey::from(key)
            .to_armored_string(armor_opts())
            .expect("armor pub");

        assert_eq!(
            get_ecdh_kdf_params_internal(&pub_block, &sub_fpr),
            Err(GfrStatus::ErrorInvalidData),
            "a signing subkey has no ECDH KDF parameters"
        );
        assert_eq!(
            get_ecdh_kdf_params_internal(&pub_block, "DEADBEEF"),
            Err(GfrStatus::ErrorInvalidInput),
            "an unknown fingerprint must be rejected"
        );
    }
}

#[cfg(test)]
mod key_tests {
    //! Key-block parsing, metadata extraction, expiry arithmetic, verified
    //! self-signature gating, merging and export.
    //!
    //! The verified-self-signature tests are the B7 regression lock: before
    //! the fix, key flags, expiry and revocation were read straight out of
    //! subpackets without checking that the signature carrying them actually
    //! verified, so anyone could rewrite a certificate's capabilities.

    use super::*;
    use crate::testutil::{corpus, keys};

    fn parse_secret(block: &str) -> SignedSecretKey {
        SignedSecretKey::from_string(block).expect("parses").0
    }

    fn meta_of(block: &str) -> ExtractedMetadata {
        extract_metadata_many_internal(block)
            .expect("metadata")
            .into_iter()
            .next()
            .expect("one key")
    }

    // -- split_pgp_blocks ---------------------------------------------------

    #[test]
    fn a_single_block_is_split_out() {
        let blocks = split_pgp_blocks(&keys::V4_SIGN.public_armored);
        assert_eq!(blocks.len(), 1);
        assert!(blocks[0].contains("BEGIN PGP PUBLIC KEY BLOCK"));
    }

    #[test]
    fn two_concatenated_blocks_are_split() {
        // RFC 9580 §10.1.5: "Transferable Public Key packet sequences may be
        // concatenated to allow transferring multiple public keys".
        let joined = format!(
            "{}{}",
            keys::V4_SIGN.public_armored,
            keys::V6_SIGN.public_armored
        );
        assert_eq!(split_pgp_blocks(&joined).len(), 2);
    }

    #[test]
    fn text_between_blocks_is_discarded() {
        let joined = format!(
            "some notes\n{}\nmore notes\n{}\ntrailer",
            keys::V4_SIGN.public_armored,
            keys::V6_SIGN.public_armored
        );
        let blocks = split_pgp_blocks(&joined);
        assert_eq!(blocks.len(), 2);
        assert!(!blocks[0].contains("some notes"));
        assert!(!blocks[1].contains("trailer"));
    }

    #[test]
    fn an_unterminated_block_is_dropped() {
        // Without an END line there is no complete block to hand on.
        let truncated = keys::V4_SIGN
            .public_armored
            .lines()
            .take(3)
            .collect::<Vec<_>>()
            .join("\n");
        assert!(split_pgp_blocks(&truncated).is_empty());
    }

    #[test]
    fn an_empty_input_yields_no_blocks() {
        assert!(split_pgp_blocks("").is_empty());
    }

    #[test]
    fn input_without_any_armor_yields_no_blocks() {
        assert!(split_pgp_blocks("just some prose\nwith two lines").is_empty());
    }

    #[test]
    fn a_second_begin_line_restarts_the_current_block() {
        // Defensive: a malformed stream with a stray BEGIN must not glue two
        // half-blocks into one nonsense block.
        let input = format!(
            "-----BEGIN PGP PUBLIC KEY BLOCK-----\ngarbage\n{}",
            keys::V4_SIGN.public_armored
        );
        let blocks = split_pgp_blocks(&input);
        assert_eq!(blocks.len(), 1);
        assert!(!blocks[0].contains("garbage"));
    }

    #[test]
    fn signature_blocks_are_split_too() {
        // The same splitter serves revocation-certificate parsing.
        assert_eq!(split_pgp_blocks(corpus::SIG_GOOD_CLEARTEXT).len(), 1);
    }

    // -- extract_metadata_many_internal -------------------------------------

    #[test]
    fn metadata_reports_the_primary_fingerprint() {
        let meta = meta_of(&keys::V4_SIGN.secret_armored);
        assert_eq!(meta.fpr.to_uppercase(), keys::V4_SIGN.primary_fpr);
    }

    #[test]
    fn metadata_reports_a_v4_key_as_v4() {
        assert_eq!(
            meta_of(&keys::V4_SIGN.secret_armored).ver,
            GfrOpenPGPKeyVersion::V4
        );
    }

    #[test]
    fn metadata_reports_a_v6_key_as_v6() {
        assert_eq!(
            meta_of(&keys::V6_SIGN.secret_armored).ver,
            GfrOpenPGPKeyVersion::V6
        );
    }

    #[test]
    fn metadata_flags_a_secret_block_as_having_a_secret() {
        let meta = meta_of(&keys::V4_SIGN.secret_armored);
        assert!(meta.has_secret);
        assert!(meta.secret_key_block.is_some());
    }

    #[test]
    fn metadata_flags_a_public_block_as_public_only() {
        let meta = meta_of(&keys::V4_SIGN.public_armored);
        assert!(!meta.has_secret);
        assert!(meta.secret_key_block.is_none());
    }

    #[test]
    fn a_secret_block_wins_over_a_public_one_for_the_same_key() {
        // Importing a keyring that holds both must not downgrade the entry to
        // public-only depending on ordering.
        let joined = format!(
            "{}{}",
            keys::V4_SIGN.public_armored,
            keys::V4_SIGN.secret_armored
        );
        let metas = extract_metadata_many_internal(&joined).expect("metadata");
        assert_eq!(metas.len(), 1, "one fingerprint, one entry");
        assert!(metas[0].has_secret);
    }

    #[test]
    fn metadata_lists_every_user_id() {
        let with_second = crate::user_id::add_user_id_internal(
            0,
            &keys::V4_SIGN.secret_armored,
            "Two <two@example.test>",
            None,
        )
        .expect("add");
        let meta = meta_of(&with_second);
        assert_eq!(meta.user_ids.len(), 2);
    }

    #[test]
    fn metadata_lists_every_subkey() {
        let meta = meta_of(&keys::V4_SIGN.secret_armored);
        assert_eq!(meta.subkeys.len(), 2);
    }

    #[test]
    fn subkey_capabilities_come_from_the_binding_signatures() {
        let meta = meta_of(&keys::V4_SIGN.secret_armored);
        assert!(
            meta.subkeys.iter().any(|s| s.can_sign),
            "the signing subkey must be reported as signing-capable"
        );
        assert!(
            meta.subkeys.iter().any(|s| s.can_encrypt),
            "the encryption subkey must be reported as encryption-capable"
        );
    }

    #[test]
    fn the_primary_key_is_reported_as_certification_capable() {
        // §10.1.5: "The primary key MUST be an algorithm capable of making
        // signatures", because it has to certify its own components.
        assert!(meta_of(&keys::V4_SIGN.secret_armored).can_certify);
    }

    #[test]
    fn metadata_reports_the_algorithm_and_key_length() {
        let meta = meta_of(&keys::V4_SIGN.secret_armored);
        assert_eq!(meta.algo, GfrKeyAlgo::ED25519);
        assert_eq!(meta.key_length, 255);
    }

    #[test]
    fn metadata_reports_a_creation_time() {
        let meta = meta_of(&keys::V4_SIGN.secret_armored);
        assert!(meta.created_at > 1_600_000_000, "a plausible timestamp");
    }

    #[test]
    fn a_key_without_an_expiry_reports_zero() {
        // §5.2.3.13: "If this is not present or has a value of zero, the key
        // never expires."
        assert_eq!(meta_of(&keys::V4_SIGN.secret_armored).expires_at, 0);
    }

    #[test]
    fn an_expiring_key_reports_creation_plus_duration() {
        // §5.2.3.13 stores a duration from the key creation time, so the
        // absolute value is created_at + duration.
        let meta = meta_of(&keys::V4_SHORT_EXPIRY.secret_armored);
        assert_eq!(
            meta.expires_at,
            meta.created_at + 1,
            "the fixture expires one second after creation"
        );
    }

    #[test]
    fn a_live_key_is_not_reported_revoked() {
        assert!(!meta_of(&keys::V4_SIGN.secret_armored).is_revoked);
    }

    #[test]
    fn a_revoked_primary_is_reported_revoked() {
        assert!(meta_of(&keys::V4_REVOKED_PRIMARY.secret_armored).is_revoked);
    }

    #[test]
    fn a_revoked_subkey_is_flagged() {
        // §5.2.1.12: a Subkey Revocation signature marks that subkey unusable.
        let meta = meta_of(&keys::V4_REVOKED_SUBKEY.secret_armored);
        assert!(
            meta.subkeys.iter().any(|s| s.is_revoked),
            "the revoked encryption subkey must be flagged"
        );
    }

    #[test]
    fn a_revoked_user_id_is_flagged() {
        let revoked = crate::user_id::revoke_user_id_internal(
            0,
            &keys::V4_SIGN.secret_armored,
            &meta_of(&keys::V4_SIGN.secret_armored).user_ids[0].user_id,
            GfrRevocationCode::UserIdInvalid,
            None,
            None,
        )
        .expect("revoke uid");
        let meta = meta_of(&revoked);
        assert!(meta.user_ids.iter().any(|u| u.is_revoked));
    }

    #[test]
    fn metadata_of_garbage_is_an_error() {
        assert!(extract_metadata_many_internal("not a key block").is_err());
    }

    #[test]
    fn metadata_of_an_empty_string_is_an_error() {
        assert!(extract_metadata_many_internal("").is_err());
    }

    #[test]
    fn metadata_of_the_corpus_key_parses() {
        let meta = meta_of(corpus::KEY1_SECRET);
        assert_eq!(meta.fpr.to_uppercase(), corpus::key1_primary_fpr());
        assert!(meta.has_secret);
    }

    #[test]
    fn metadata_of_the_v6_corpus_key_reports_v6() {
        assert_eq!(meta_of(corpus::AUX_V6_SECRET).ver, GfrOpenPGPKeyVersion::V6);
    }

    #[test]
    fn the_re_exported_public_block_reparses() {
        // The blocks in the metadata are re-serialised from the parsed key, so
        // they must be canonical and round-trippable.
        let meta = meta_of(&keys::V4_SIGN.secret_armored);
        assert!(SignedPublicKey::from_string(&meta.public_key_block).is_ok());
    }

    #[test]
    fn metadata_never_panics_on_adversarial_input() {
        for vector in [
            corpus::TRUNCATED_ARMOR,
            corpus::CORRUPT_CRC,
            &String::from_utf8_lossy(corpus::GARBAGE),
        ] {
            let outcome =
                std::panic::catch_unwind(|| extract_metadata_many_internal(vector).is_ok());
            assert!(outcome.is_ok());
        }
    }

    // -- KeyVersion -> GfrOpenPGPKeyVersion ---------------------------------

    #[test]
    fn every_modelled_key_version_maps_to_its_own_variant() {
        // There is no v5 key fixture -- rPGP cannot generate one -- so the
        // mapping is exercised directly. Without this, a v5 key would be
        // reported as v4 or Unknown.
        for (from, to) in [
            (KeyVersion::V4, GfrOpenPGPKeyVersion::V4),
            (KeyVersion::V5, GfrOpenPGPKeyVersion::V5),
            (KeyVersion::V6, GfrOpenPGPKeyVersion::V6),
        ] {
            assert_eq!(GfrOpenPGPKeyVersion::from(from), to);
        }
    }

    #[test]
    fn a_deprecated_or_unheard_of_key_version_maps_to_unknown() {
        for from in [KeyVersion::V2, KeyVersion::V3, KeyVersion::Other(7)] {
            assert_eq!(
                GfrOpenPGPKeyVersion::from(from),
                GfrOpenPGPKeyVersion::Unknown
            );
        }
    }

    // -- detect_key_version_internal ----------------------------------------

    #[test]
    fn a_v4_public_block_reports_v4() {
        assert_eq!(
            detect_key_version_internal(&keys::V4_SIGN.public_armored),
            Ok(GfrOpenPGPKeyVersion::V4)
        );
    }

    #[test]
    fn a_v6_public_block_reports_v6() {
        assert_eq!(
            detect_key_version_internal(&keys::V6_SIGN.public_armored),
            Ok(GfrOpenPGPKeyVersion::V6)
        );
    }

    #[test]
    fn a_secret_block_reports_the_same_version_as_its_public_half() {
        // The GnuPG path feeds a public export, but the function must not
        // depend on that.
        for fixture in [&*keys::V4_SIGN, &*keys::V6_SIGN] {
            assert_eq!(
                detect_key_version_internal(&fixture.secret_armored),
                detect_key_version_internal(&fixture.public_armored)
            );
        }
    }

    #[test]
    fn only_the_first_block_of_a_concatenation_is_inspected() {
        let joined = format!(
            "{}{}",
            keys::V6_SIGN.public_armored,
            keys::V4_SIGN.public_armored
        );
        assert_eq!(
            detect_key_version_internal(&joined),
            Ok(GfrOpenPGPKeyVersion::V6)
        );
    }

    #[test]
    fn detecting_a_version_agrees_with_full_metadata_extraction() {
        for fixture in [&*keys::V4_SIGN, &*keys::V6_SIGN] {
            assert_eq!(
                detect_key_version_internal(&fixture.public_armored).expect("version"),
                meta_of(&fixture.public_armored).ver
            );
        }
    }

    /// Wrap a hand-written key packet -- `C6 01 <version>`: new-format header,
    /// tag 6 (public key), body length 1 -- in public key armor. Nothing past
    /// the version octet is ever read, and neither our keygen nor rPGP itself
    /// can mint a v5 key, so writing the packet out by hand is the only way to
    /// reach the v5 path. The armor carries no CRC-24 footer, which RFC 9580
    /// §6.2.3 makes optional.
    fn armored_key_packet(base64_packet: &str) -> String {
        format!(
            "-----BEGIN PGP PUBLIC KEY BLOCK-----\n\n{base64_packet}\n\
             -----END PGP PUBLIC KEY BLOCK-----\n"
        )
    }

    /// `C6 01 05` -- a v5 (LibrePGP) public key packet.
    const V5_KEY_PACKET_B64: &str = "xgEF";
    /// `C6 01 03` -- a v3 public key packet, a version we do not name.
    const V3_KEY_PACKET_B64: &str = "xgED";

    #[test]
    fn a_v5_public_block_reports_v5() {
        // rPGP cannot parse a v5 key packet, so the version octet is read
        // directly; without that this returns ErrorInvalidInput and every
        // LibrePGP key in a GnuPG keyring shows no version at all.
        assert_eq!(
            detect_key_version_internal(&armored_key_packet(V5_KEY_PACKET_B64)),
            Ok(GfrOpenPGPKeyVersion::V5)
        );
    }

    #[test]
    fn a_v5_block_is_invisible_to_the_rpgp_key_parser() {
        // Pins *why* the octet peek exists: the parse below does not fail, it
        // yields no key at all, because unsupported packets are dropped
        // silently. Should a future rPGP gain v5 support, this test is the one
        // that says the peek can go.
        let block = armored_key_packet(V5_KEY_PACKET_B64);
        let (pk_iter, _) = SignedPublicKey::from_string_many(&block).expect("armor parses");
        assert_eq!(pk_iter.flatten().count(), 0);
    }

    #[test]
    fn a_deprecated_key_version_still_reports_no_version() {
        // The peek must not hand back a version it cannot name -- an unknown
        // answer stays an error, exactly as before.
        assert_eq!(
            detect_key_version_internal(&armored_key_packet(V3_KEY_PACKET_B64)),
            Err(GfrStatus::ErrorInvalidInput)
        );
    }

    #[test]
    fn detecting_a_version_rejects_adversarial_input() {
        for vector in [
            "",
            "just some prose",
            corpus::TRUNCATED_ARMOR,
            corpus::CORRUPT_CRC,
            &String::from_utf8_lossy(corpus::GARBAGE),
        ] {
            let outcome = std::panic::catch_unwind(|| detect_key_version_internal(vector));
            assert_eq!(
                outcome.expect("no panic"),
                Err(GfrStatus::ErrorInvalidInput),
                "unparsable input must not yield a version"
            );
        }
    }

    // -- verified self-signature gates (B7) ---------------------------------

    #[test]
    fn a_genuine_primary_revocation_is_honoured() {
        let key = parse_secret(&keys::V4_REVOKED_PRIMARY.secret_armored);
        assert!(is_primary_key_revoked(
            key.primary_key.public_key(),
            &key.details.revocation_signatures
        ));
    }

    #[test]
    fn a_live_key_is_not_revoked() {
        let key = parse_secret(&keys::V4_SIGN.secret_armored);
        assert!(!is_primary_key_revoked(
            key.primary_key.public_key(),
            &key.details.revocation_signatures
        ));
    }

    #[test]
    fn a_forged_revocation_signature_is_ignored() {
        // B7: `aux_forged_revocation.asc` carries a fabricated primary-key
        // revocation spliced in at the packet level. Trusting the subpacket
        // without verifying it would let anyone revoke anyone's key.
        assert!(!is_primary_key_revoked(
            &corpus::AUX_FORGED_REVOCATION_CERT.primary_key,
            &corpus::AUX_FORGED_REVOCATION_CERT
                .details
                .revocation_signatures
        ));
    }

    #[test]
    fn a_forged_revocation_does_not_flip_the_metadata_flag() {
        // The same guarantee at the level the C++ side actually consumes.
        assert!(!meta_of(corpus::AUX_FORGED_REVOCATION).is_revoked);
    }

    #[test]
    fn the_genuine_revoked_corpus_cert_is_reported_revoked() {
        // The control for the forged case: a real self-revocation IS honoured.
        assert!(meta_of(corpus::AUX_REVOKED).is_revoked);
    }

    #[test]
    fn subkey_revocation_requires_a_verifying_binding() {
        let key = parse_secret(&keys::V4_REVOKED_SUBKEY.secret_armored);
        let primary = key.primary_key.public_key();
        let revoked_count = key
            .secret_subkeys
            .iter()
            .filter(|s| is_subkey_revoked(primary, s.key.public_key(), &s.signatures))
            .count();
        assert_eq!(revoked_count, 1, "exactly the revoked subkey");
    }

    #[test]
    fn a_subkey_of_a_live_key_is_not_revoked() {
        let key = parse_secret(&keys::V4_SIGN.secret_armored);
        let primary = key.primary_key.public_key();
        for sub in &key.secret_subkeys {
            assert!(!is_subkey_revoked(
                primary,
                sub.key.public_key(),
                &sub.signatures
            ));
        }
    }

    // -- expiry arithmetic ---------------------------------------------------

    #[test]
    fn primary_key_expires_at_is_zero_for_a_never_expiring_key() {
        let cert = SignedPublicKey::from_string(&keys::V4_SIGN.public_armored)
            .expect("parses")
            .0;
        assert_eq!(primary_key_expires_at(&cert), 0);
    }

    #[test]
    fn primary_key_expires_at_reports_the_absolute_time() {
        let cert = SignedPublicKey::from_string(&keys::V4_SHORT_EXPIRY.public_armored)
            .expect("parses")
            .0;
        let created = cert.primary_key.created_at().as_secs();
        assert_eq!(primary_key_expires_at(&cert), created + 1);
    }

    #[test]
    fn the_expired_corpus_certificate_reports_a_past_expiry() {
        // `aux_expired.asc` was minted with a 2020 validity window.
        let expires = primary_key_expires_at(&corpus::AUX_EXPIRED_CERT);
        assert!(expires > 0, "the fixture carries an expiry");
        let now = std::time::SystemTime::now()
            .duration_since(std::time::UNIX_EPOCH)
            .expect("clock")
            .as_secs() as u32;
        assert!(expires < now, "and it is in the past");
    }

    /// `fixture`'s certificate with one more self-certification on its first
    /// User ID, created at `created`, carrying `ket` as Key Expiration Time.
    fn with_extra_uid_self_sig(
        fixture: &keys::Fixture,
        created: u32,
        ket: Option<u32>,
    ) -> SignedPublicKey {
        use pgp::types::Timestamp;
        let mut cert = SignedPublicKey::from(fixture.secret.clone());
        let primary = &fixture.secret.primary_key;
        let mut cfg =
            SignatureConfig::from_key(rand::thread_rng(), primary, SignatureType::CertPositive)
                .unwrap();
        cfg.hashed_subpackets = vec![
            Subpacket::regular(SubpacketData::SignatureCreationTime(Timestamp::from_secs(
                created,
            )))
            .unwrap(),
            Subpacket::regular(SubpacketData::IssuerFingerprint(primary.fingerprint())).unwrap(),
        ];
        if let Some(secs) = ket {
            cfg.hashed_subpackets.push(
                Subpacket::regular(SubpacketData::KeyExpirationTime(Duration::from_secs(secs)))
                    .unwrap(),
            );
        }
        let user = &mut cert.details.users[0];
        let sig = cfg
            .sign_certification(
                primary,
                primary.public_key(),
                &Password::empty(),
                Tag::UserId,
                &user.id,
            )
            .unwrap();
        user.signatures.push(sig);
        cert
    }

    /// A certification of `target`'s first User ID made by `signer`, naming
    /// `issuer` as its issuer and created at `created`.
    fn certification_by(
        signer: &keys::Fixture,
        target: &SignedPublicKey,
        issuer: pgp::types::Fingerprint,
        created: u32,
    ) -> Signature {
        let signing = &signer.secret.primary_key;
        let mut cfg =
            SignatureConfig::from_key(rand::thread_rng(), signing, SignatureType::CertGeneric)
                .unwrap();
        cfg.hashed_subpackets = vec![
            Subpacket::regular(SubpacketData::SignatureCreationTime(
                pgp::types::Timestamp::from_secs(created),
            ))
            .unwrap(),
            Subpacket::regular(SubpacketData::IssuerFingerprint(issuer)).unwrap(),
        ];
        cfg.sign_certification_third_party(
            signing,
            &Password::empty(),
            &target.primary_key,
            Tag::UserId,
            &target.details.users[0].id,
        )
        .unwrap()
    }

    /// A minimal JPEG photo ID packet, parsed by rPGP itself.
    fn photo_id(payload: &[u8]) -> pgp::packet::UserAttribute {
        use pgp::packet::{Packet, PacketParser};
        let mut sub = vec![0x10, 0x00, 0x01, 0x01];
        sub.extend([0u8; 12]);
        sub.extend(payload);
        let mut body = vec![(sub.len() + 1) as u8, 0x01];
        body.extend(sub);
        let mut packet = vec![0xC0 | 17, body.len() as u8];
        packet.extend(body);
        match PacketParser::new(Cursor::new(packet)).next() {
            Some(Ok(Packet::UserAttribute(attr))) => attr,
            other => panic!("not a user attribute: {other:?}"),
        }
    }

    #[test]
    fn a_merge_brings_in_a_certified_photo_id_and_nothing_else() {
        let fixture = &keys::V4_SIGN;
        let base = SignedPublicKey::from(fixture.secret.clone());
        let primary = &fixture.secret.primary_key;
        let created = base.primary_key.created_at().as_secs();

        let certified = photo_id(b"\xFF\xD8 certified");
        let mut cfg =
            SignatureConfig::from_key(rand::thread_rng(), primary, SignatureType::CertPositive)
                .unwrap();
        cfg.hashed_subpackets = vec![
            Subpacket::regular(SubpacketData::SignatureCreationTime(
                pgp::types::Timestamp::from_secs(created + 1),
            ))
            .unwrap(),
            Subpacket::regular(SubpacketData::IssuerFingerprint(primary.fingerprint())).unwrap(),
        ];
        let sig = cfg
            .sign_certification(
                primary,
                primary.public_key(),
                &Password::empty(),
                Tag::UserAttribute,
                &certified,
            )
            .unwrap();

        let mut incoming = base.clone();
        incoming
            .details
            .user_attributes
            .push(pgp::types::SignedUserAttribute::new(
                certified.clone(),
                vec![sig],
            ));
        // Anyone can attach a photo ID; one the key holder never certified is
        // not theirs.
        incoming
            .details
            .user_attributes
            .push(pgp::types::SignedUserAttribute::new(
                photo_id(b"\xFF\xD8 forged"),
                vec![],
            ));

        let merged = merge_key_block_internal(
            &export_public_key(base).unwrap().public,
            &export_public_key(incoming).unwrap().public,
        )
        .expect("merge");
        let (merged, _) = SignedPublicKey::from_string(&merged.public).unwrap();
        let attrs = &merged.details.user_attributes;
        assert_eq!(attrs.len(), 1, "{attrs:?}");
        assert_eq!(attrs[0].attr, certified);
    }

    #[test]
    fn a_minimal_export_keeps_only_the_owners_own_signatures() {
        // gpg's export-minimal: the rPGP engine used to ignore the request
        // and hand out every certification, naming who vouched for the key.
        let target = &keys::V4_SIGN;
        let signer = &keys::V4_PRIMARY_ONLY;
        let mut cert = SignedPublicKey::from(target.secret.clone());
        let created = cert.primary_key.created_at().as_secs();
        let own = cert.details.users[0].signatures.len();
        let certification = certification_by(
            signer,
            &cert,
            signer.secret.primary_key.fingerprint(),
            created + 1,
        );
        cert.details.users[0].signatures.push(certification);
        let block = export_public_key(cert).unwrap().public;

        let whole = export_merged_keys(&[&block], false, true, false).unwrap();
        let (whole, _) =
            SignedPublicKey::from_string(std::str::from_utf8(&whole).unwrap()).unwrap();
        assert_eq!(whole.details.users[0].signatures.len(), own + 1);

        let minimal = export_merged_keys(&[&block], false, true, true).unwrap();
        let (minimal, _) =
            SignedPublicKey::from_string(std::str::from_utf8(&minimal).unwrap()).unwrap();
        let sigs = &minimal.details.users[0].signatures;
        assert_eq!(sigs.len(), own);
        assert!(
            sigs.iter()
                .all(|s| names_primary_as_issuer(s, &minimal.primary_key))
        );

        // Not armored: the same packets, as bytes.
        let binary = export_merged_keys(&[&block], false, false, true).unwrap();
        assert!(!binary.starts_with(b"-----BEGIN"));
        let parsed = SignedPublicKey::from_bytes(binary.as_slice()).unwrap();
        assert_eq!(parsed.fingerprint(), minimal.fingerprint());
        assert_eq!(parsed.details.users[0].signatures.len(), own);
    }

    #[test]
    fn a_merge_keeps_neither_a_flood_nor_a_forged_self_signature() {
        let target = &keys::V4_SIGN;
        let signer = &keys::V4_PRIMARY_ONLY;
        let base = SignedPublicKey::from(target.secret.clone());
        let created = base.primary_key.created_at().as_secs();
        let before = base.details.users[0].signatures.len();

        let mut flooded = base.clone();
        let signer_fpr = signer.secret.primary_key.fingerprint();
        for i in 0..(MAX_MERGED_SIGNATURES as u32 + 40) {
            let sig = certification_by(signer, &base, signer_fpr.clone(), created + 1 + i);
            flooded.details.users[0].signatures.push(sig);
        }
        // Made by another key, but claiming to be the key holder's own.
        let forged = certification_by(
            signer,
            &base,
            base.primary_key.fingerprint(),
            created + 10_000,
        );
        flooded.details.users[0].signatures.push(forged.clone());

        let merged = merge_key_block_internal(
            &export_public_key(base).unwrap().public,
            &export_public_key(flooded).unwrap().public,
        )
        .expect("merge");
        let (merged, _) = SignedPublicKey::from_string(&merged.public).unwrap();
        let sigs = &merged.details.users[0].signatures;

        let third_party = sigs
            .iter()
            .filter(|s| !names_primary_as_issuer(s, &merged.primary_key))
            .count();
        assert_eq!(third_party, MAX_MERGED_SIGNATURES, "the flood is capped");
        assert!(sigs.len() >= before);
        let forged_bytes = serialize_bytes(&forged).unwrap();
        assert!(
            !sigs
                .iter()
                .any(|s| serialize_bytes(s).unwrap() == forged_bytes),
            "a self-signature that does not verify was merged in"
        );
        // The key holder's own certification survives the flood.
        assert!(has_verified_uid_certification(
            &merged.primary_key,
            &merged.details.users[0]
        ));
    }

    #[test]
    fn a_newer_self_signature_without_an_expiry_overrides_an_older_one() {
        // RFC 9580 §5.2.3.10: "MUST select the most recent valid
        // self-signature and ignore all other self-signatures"; §5.2.3.13: an
        // absent Key Expiration Time means the key never expires.
        let fixture = &keys::V4_SHORT_EXPIRY;
        let created = fixture.secret.primary_key.created_at().as_secs();
        let cert = with_extra_uid_self_sig(fixture, created + 100, None);
        assert_eq!(primary_key_expires_at(&cert), 0);
    }

    #[test]
    fn a_newer_self_signature_with_an_expiry_overrides_an_older_one_without() {
        let fixture = &keys::V4_SIGN;
        let created = fixture.secret.primary_key.created_at().as_secs();
        let cert = with_extra_uid_self_sig(fixture, created + 100, Some(7200));
        assert_eq!(primary_key_expires_at(&cert), created + 7200);
    }

    fn sig_with_hash(hash: pgp::crypto::hash::HashAlgorithm, created: u32) -> Signature {
        use pgp::types::{SignatureBytes, Timestamp};
        let mut cfg = SignatureConfig::v4(
            SignatureType::SubkeyBinding,
            pgp::crypto::public_key::PublicKeyAlgorithm::RSA,
            hash,
        );
        cfg.hashed_subpackets = vec![
            Subpacket::regular(SubpacketData::SignatureCreationTime(Timestamp::from_secs(
                created,
            )))
            .unwrap(),
        ];
        Signature::from_config(cfg, [0, 0], SignatureBytes::Mpis(vec![])).unwrap()
    }

    #[test]
    fn sha1_certifications_count_only_before_the_cutoff() {
        // RFC 9580 §9.5: "MUST NOT validate any recent signature that depends
        // on MD5, SHA-1, or RIPEMD-160" (cutoff: key-validity.md).
        use pgp::crypto::hash::HashAlgorithm;
        let before = WEAK_CERTIFICATION_HASH_CUTOFF - 1;
        let after = WEAK_CERTIFICATION_HASH_CUTOFF;
        assert!(certification_hash_acceptable(&sig_with_hash(
            HashAlgorithm::Sha1,
            before
        )));
        assert!(!certification_hash_acceptable(&sig_with_hash(
            HashAlgorithm::Sha1,
            after
        )));
        assert!(certification_hash_acceptable(&sig_with_hash(
            HashAlgorithm::Ripemd160,
            before
        )));
        assert!(!certification_hash_acceptable(&sig_with_hash(
            HashAlgorithm::Ripemd160,
            after
        )));
        assert!(!certification_hash_acceptable(&sig_with_hash(
            HashAlgorithm::Md5,
            1
        )));
        assert!(certification_hash_acceptable(&sig_with_hash(
            HashAlgorithm::Sha256,
            after
        )));
    }

    /// `fixture`'s secret key with its only User ID's self-certifications
    /// replaced by `sigs(...)`, built by the primary key.
    fn with_uid_sigs(
        fixture: &keys::Fixture,
        sigs: impl Fn(&pgp::composed::SignedSecretKey) -> Vec<Signature>,
    ) -> pgp::composed::SignedSecretKey {
        let mut sk = fixture.secret.clone();
        let new = sigs(&sk);
        sk.details.users[0].signatures = new;
        sk
    }

    /// A self-certification (0x13) of `sk`'s first User ID created at
    /// `created`, optionally expiring `sig_expiry` seconds later
    /// (§5.2.3.18), or a certification revocation (0x30) when `revoke`.
    fn uid_self_sig(
        sk: &pgp::composed::SignedSecretKey,
        created: u32,
        sig_expiry: Option<u32>,
        revoke: bool,
    ) -> Signature {
        use pgp::packet::KeyFlags;
        use pgp::types::{Duration, Password, Timestamp};
        let primary = &sk.primary_key;
        let typ = if revoke {
            SignatureType::CertRevocation
        } else {
            SignatureType::CertPositive
        };
        let mut cfg = SignatureConfig::from_key(rand::thread_rng(), primary, typ).unwrap();
        cfg.hashed_subpackets = vec![
            Subpacket::regular(SubpacketData::SignatureCreationTime(Timestamp::from_secs(
                created,
            )))
            .unwrap(),
            Subpacket::regular(SubpacketData::IssuerFingerprint(primary.fingerprint())).unwrap(),
        ];
        if !revoke {
            let mut flags = KeyFlags::default();
            flags.set_sign(true);
            flags.set_certify(true);
            cfg.hashed_subpackets
                .push(Subpacket::regular(SubpacketData::KeyFlags(flags)).unwrap());
        }
        if let Some(secs) = sig_expiry {
            cfg.hashed_subpackets.push(
                Subpacket::regular(SubpacketData::SignatureExpirationTime(Duration::from_secs(
                    secs,
                )))
                .unwrap(),
            );
        }
        cfg.sign_certification(
            primary,
            primary.public_key(),
            &Password::empty(),
            Tag::UserId,
            &sk.details.users[0].id,
        )
        .unwrap()
    }

    fn signing_verdict_at(sk: &pgp::composed::SignedSecretKey, t: u32) -> KeyVerdict {
        let cert = SignedPublicKey::from(sk.clone());
        primary_signing_verdict(&cert.primary_key, &cert.details, t)
    }

    // -- metadata agrees with the verdicts (INV-AUTH) ---------------------------

    /// A self-signature of `typ` by `sk`'s primary over `target` (a User ID
    /// index, or a subkey index for 0x18) created at `created`, with `flags`
    /// and the Primary User ID flag when `primary`.
    fn self_sig_with(
        sk: &pgp::composed::SignedSecretKey,
        target: usize,
        typ: SignatureType,
        created: u32,
        flags: pgp::packet::KeyFlags,
        primary: bool,
    ) -> Signature {
        use pgp::types::{Password, Timestamp};
        let key = &sk.primary_key;
        let mut cfg = SignatureConfig::from_key(rand::thread_rng(), key, typ).unwrap();
        cfg.hashed_subpackets = vec![
            Subpacket::regular(SubpacketData::SignatureCreationTime(Timestamp::from_secs(
                created,
            )))
            .unwrap(),
            Subpacket::regular(SubpacketData::IssuerFingerprint(key.fingerprint())).unwrap(),
            Subpacket::regular(SubpacketData::KeyFlags(flags)).unwrap(),
        ];
        if primary {
            cfg.hashed_subpackets
                .push(Subpacket::regular(SubpacketData::IsPrimary(true)).unwrap());
        }
        let pw = Password::empty();
        match typ {
            SignatureType::SubkeyBinding => cfg
                .sign_subkey_binding(
                    key,
                    key.public_key(),
                    &pw,
                    sk.secret_subkeys[target].key.public_key(),
                )
                .unwrap(),
            _ => cfg
                .sign_certification(
                    key,
                    key.public_key(),
                    &pw,
                    Tag::UserId,
                    &sk.details.users[target].id,
                )
                .unwrap(),
        }
    }

    fn flags(sign: bool, encrypt: bool) -> pgp::packet::KeyFlags {
        let mut f = pgp::packet::KeyFlags::default();
        f.set_sign(sign);
        f.set_certify(sign);
        f.set_encrypt_comms(encrypt);
        f.set_encrypt_storage(encrypt);
        f
    }

    #[test]
    fn metadata_reports_the_capabilities_of_the_binding_in_force() {
        // A newer binding that drops a flag supersedes the older one
        // (§5.2.3.10); the GUI must not keep offering what the engine will
        // refuse.
        let fixture = &keys::V4_SIGN;
        let created = fixture.secret.primary_key.created_at().as_secs();
        let mut sk = fixture.secret.clone();
        let enc = sk
            .secret_subkeys
            .iter()
            .position(|s| {
                s.key.fingerprint().to_string().to_uppercase() == fixture.enc_subkey_fpr()
            })
            .unwrap();
        let old = self_sig_with(
            &sk,
            enc,
            SignatureType::SubkeyBinding,
            created,
            flags(true, true),
            false,
        );
        let new = self_sig_with(
            &sk,
            enc,
            SignatureType::SubkeyBinding,
            created + 10,
            flags(false, true),
            false,
        );
        sk.secret_subkeys[enc].signatures = vec![old, new];
        let meta = build_public_metadata(&SignedPublicKey::from(sk));
        let sub = meta
            .subkeys
            .iter()
            .find(|s| s.fpr.eq_ignore_ascii_case(fixture.enc_subkey_fpr()))
            .unwrap();
        assert!(sub.can_encrypt);
        assert!(!sub.can_sign, "the superseded binding's sign flag is gone");
    }

    #[test]
    fn a_user_id_the_key_holder_never_certified_is_not_listed_or_merged_in() {
        // Anyone can append a User ID packet to someone else's public key.
        // Listed, it read as a valid UID of the key, and a search by its
        // address found the key; merged in from an imported copy, it stayed.
        let fixture = &keys::V4_SIGN;
        let forged_uid = "Mallory <bob@corp.example>";
        let mut pk = SignedPublicKey::from(fixture.secret.clone());
        let forged =
            pgp::packet::UserId::from_str(pgp::types::PacketHeaderVersion::New, forged_uid)
                .unwrap();
        pk.details.users.push(SignedUser::new(forged, vec![]));
        let names = |users: &[ExtractUserId]| -> Vec<String> {
            users.iter().map(|u| u.user_id.clone()).collect()
        };

        let listed = build_public_metadata(&pk);
        assert!(
            !listed.user_ids.is_empty(),
            "the genuine UID is still listed"
        );
        assert!(!names(&listed.user_ids).contains(&forged_uid.to_string()));

        let forged_block = export_public_key(pk).unwrap().public;
        for base in [&fixture.public_armored, &fixture.secret_armored] {
            let merged = merge_key_block_internal(base, &forged_block).unwrap();
            let block = if merged.secret.is_empty() {
                merged.public.clone()
            } else {
                merged.secret.to_string()
            };
            let metas = extract_metadata_many_internal(&block).unwrap();
            assert!(!names(&metas[0].user_ids).contains(&forged_uid.to_string()));
            assert!(!block_users(&block).contains(&forged_uid.to_string()));
        }
    }

    fn block_users(block: &str) -> Vec<String> {
        let users = match SignedSecretKey::from_string(block) {
            Ok((sk, _)) => sk.details.users,
            Err(_) => SignedPublicKey::from_string(block).unwrap().0.details.users,
        };
        users
            .iter()
            .map(|u| String::from_utf8_lossy(u.id.id()).into_owned())
            .collect()
    }

    #[test]
    fn a_subkey_copied_from_another_certificate_is_not_listed() {
        // A squatter certificate carrying someone else's subkey, with that
        // key's binding signature, which does not verify under the squatter.
        let victim = SignedPublicKey::from(keys::V4_SIGN.secret.clone());
        let mut squatter = SignedPublicKey::from(keys::V4_PRIMARY_ONLY.secret.clone());
        squatter
            .public_subkeys
            .push(victim.public_subkeys[0].clone());
        let stolen = victim.public_subkeys[0].key.fingerprint().to_string();

        let listed = build_public_metadata(&squatter);
        assert!(!listed.subkeys.iter().any(|s| s.fpr == stolen));
        assert!(
            build_public_metadata(&victim)
                .subkeys
                .iter()
                .any(|s| s.fpr == stolen)
        );
    }

    #[test]
    fn metadata_names_one_primary_user_id_the_one_in_force() {
        // §5.2.3.27: with several User IDs marked primary, the most recent
        // self-signature decides. The listing, the reported primary and the
        // primary's capabilities must all follow the same signature.
        let fixture = &keys::V4_SIGN;
        let created = fixture.secret.primary_key.created_at().as_secs();
        let mut sk = fixture.secret.clone();
        let second = pgp::packet::UserId::from_str(
            pgp::types::PacketHeaderVersion::New,
            "Second <second@test.example>",
        )
        .unwrap();
        sk.details.users.push(SignedUser::new(second, vec![]));
        let first_sig = self_sig_with(
            &sk,
            0,
            SignatureType::CertPositive,
            created,
            flags(true, true),
            true,
        );
        let second_sig = self_sig_with(
            &sk,
            1,
            SignatureType::CertPositive,
            created + 10,
            flags(true, false),
            true,
        );
        sk.details.users[0].signatures = vec![first_sig];
        sk.details.users[1].signatures = vec![second_sig];
        let meta = build_public_metadata(&SignedPublicKey::from(sk));
        assert_eq!(meta.user_ids[0].user_id, "Second <second@test.example>");
        assert_eq!(
            meta.user_ids.iter().filter(|u| u.is_primary).count(),
            1,
            "exactly one primary User ID"
        );
        assert!(meta.user_ids[0].is_primary);
        assert!(
            !meta.can_encrypt,
            "capabilities come from the primary in force"
        );
    }

    #[test]
    fn a_self_signature_with_a_critical_subpacket_gf_does_not_understand_is_ignored() {
        // RFC 9580 §5.2.3.7 (round four, sibling of R3-11): a self-signature
        // "in error" must not govern the key, here by imposing an expiry.
        use pgp::types::{Duration, Password, Timestamp};
        let fixture = &keys::V4_SIGN;
        let created = fixture.secret.primary_key.created_at().as_secs();
        let sk = with_uid_sigs(fixture, |sk| {
            let primary = &sk.primary_key;
            let mut cfg =
                SignatureConfig::from_key(rand::thread_rng(), primary, SignatureType::CertPositive)
                    .unwrap();
            cfg.hashed_subpackets = vec![
                Subpacket::regular(SubpacketData::SignatureCreationTime(Timestamp::from_secs(
                    created + 10,
                )))
                .unwrap(),
                Subpacket::regular(SubpacketData::IssuerFingerprint(primary.fingerprint()))
                    .unwrap(),
                Subpacket::regular(SubpacketData::KeyFlags(flags(true, false))).unwrap(),
                Subpacket::regular(SubpacketData::KeyExpirationTime(Duration::from_secs(100)))
                    .unwrap(),
                Subpacket::critical(SubpacketData::Notation(pgp::packet::Notation {
                    readable: true,
                    name: "must-understand@example.test".into(),
                    value: "1".into(),
                }))
                .unwrap(),
            ];
            let in_error = cfg
                .sign_certification(
                    primary,
                    primary.public_key(),
                    &Password::empty(),
                    Tag::UserId,
                    &sk.details.users[0].id,
                )
                .unwrap();
            vec![uid_self_sig(sk, created, None, false), in_error]
        });
        let cert = SignedPublicKey::from(sk);
        assert_eq!(
            primary_key_expires_at(&cert),
            0,
            "the self-signature in error governed"
        );
    }

    #[test]
    fn a_forbidden_primary_never_signs_new_self_signatures() {
        // RFC 9580 §12.5 "MUST NOT sign ... using DSA keys" (and §12.4 for
        // RSA under 2048 bits). Editing such a key would generate new DSA
        // signatures. Revoking it stays possible, so an old key can still be
        // retired.
        let dsa = &keys::V4_DSA2048;
        let created = dsa.secret.primary_key.created_at().as_secs();
        let uid = String::from_utf8_lossy(dsa.secret.details.users[0].id.id()).into_owned();
        assert_eq!(
            update_key_expiration_internal(
                0,
                &dsa.secret_armored,
                None,
                (created + 86_400) as u64,
                None
            )
            .err(),
            Some(GfrStatus::ErrorUnusableKey),
            "expiry change"
        );
        assert_eq!(
            crate::user_id::add_user_id_internal(
                0,
                &dsa.secret_armored,
                "New <n@example.test>",
                None
            )
            .err(),
            Some(GfrStatus::ErrorUnusableKey),
            "add User ID"
        );
        assert_eq!(
            crate::user_id::set_primary_user_id_internal(0, &dsa.secret_armored, &uid, None).err(),
            Some(GfrStatus::ErrorUnusableKey),
            "set primary User ID"
        );
        assert_eq!(
            crate::keygen::add_subkey_internal(
                0,
                &dsa.secret_armored,
                &keys::cfg(
                    crate::types::GfrKeyAlgo::ED25519,
                    false,
                    true,
                    crate::types::GfrOpenPGPKeyVersion::V4
                ),
                None
            )
            .err(),
            Some(GfrStatus::ErrorUnusableKey),
            "add subkey"
        );
        assert!(
            generate_key_rev_cert_internal(
                0,
                &dsa.secret_armored,
                GfrRevocationCode::Retired,
                None,
                None
            )
            .is_ok(),
            "revoking remains possible"
        );
    }

    #[test]
    fn metadata_never_offers_what_the_engine_will_refuse() {
        // Round four: the GUI offers keys by these flags. A DSA key may not
        // sign (§12.5) and rPGP cannot encrypt to ElGamal or Brainpool ECDH,
        // so none of them may be offered for that use.
        let dsa = build_public_metadata(&SignedPublicKey::from(keys::V4_DSA2048.secret.clone()));
        assert!(
            !dsa.can_sign && !dsa.can_certify,
            "DSA primary offered for signing"
        );
        for cert in [keys::ELGAMAL_SUBKEY_CERT, keys::BRAINPOOL_SUBKEY_CERT] {
            let pk = SignedPublicKey::from_string(cert).unwrap().0;
            let meta = build_public_metadata(&pk);
            assert!(meta.can_sign, "control: the Ed25519 primary still signs");
            assert!(
                meta.subkeys.iter().all(|s| !s.can_encrypt),
                "an unusable encryption subkey offered"
            );
        }
    }

    #[test]
    fn a_new_user_id_never_inherits_a_self_signature_expiry() {
        // RFC 9580 §5.2.3.18 (round four, sibling of R2-10): a Signature
        // Expiration Time copied from the template would end the new User
        // ID's certification at an offset the user never chose.
        let fixture = &keys::V4_SIGN;
        let created = fixture.secret.primary_key.created_at().as_secs();
        let sk = with_uid_sigs(fixture, |sk| {
            vec![uid_self_sig(sk, created, Some(3600), false)]
        });
        let block = sk.to_armored_string(armor_opts()).unwrap();
        let out = crate::user_id::add_user_id_internal(0, &block, "New <new@example.test>", None)
            .unwrap();
        let after = SignedSecretKey::from_string(&out).unwrap().0;
        let new_uid = after
            .details
            .users
            .iter()
            .find(|u| u.id.id() == b"New <new@example.test>")
            .unwrap();
        assert!(
            new_uid
                .signatures
                .iter()
                .all(|s| s.signature_expiration_time().is_none()),
            "the new certification expires"
        );
    }

    #[test]
    fn a_binary_key_block_imports_like_its_armored_form() {
        // RFC 9580 §6: armor is an optional transport encoding; GnuPG's
        // default `--export` is binary packets (round four, C++ import from a
        // .gpg file). Both a public and a secret block must be read.
        for (block, secret) in [
            (&keys::V4_SIGN.public_armored, false),
            (&keys::V4_SIGN.secret_armored, true),
        ] {
            let binary = crate::crypto::dearmor_all(block.as_bytes()).into_owned();
            assert!(std::str::from_utf8(&binary).is_err() || !binary.starts_with(b"-----"));
            let text = armored_key_input(&binary).expect("binary input is accepted");
            let meta = extract_metadata_many_internal(&text).expect("imports");
            assert_eq!(meta.len(), 1);
            assert!(meta[0].fpr.eq_ignore_ascii_case(&keys::V4_SIGN.primary_fpr));
            assert_eq!(meta[0].secret_key_block.is_some(), secret);
        }
        // Armored input passes through unchanged.
        assert_eq!(
            armored_key_input(keys::V4_SIGN.public_armored.as_bytes()).unwrap(),
            keys::V4_SIGN.public_armored.as_str()
        );
    }

    #[test]
    fn a_v4_primary_with_no_valid_self_signature_is_never_authorized() {
        // RFC 9580 §5.2.3.10, §10.1: a key's usage comes from its valid
        // self-signatures. Filtering out an invalid, weak or revoked one must
        // remove the permission along with its constraints (expiry, flags),
        // not leave an unconstrained key behind.
        let fixture = &keys::V4_SIGN;
        let created = fixture.secret.primary_key.created_at().as_secs();
        let bare = with_uid_sigs(fixture, |_| vec![]);
        assert_eq!(
            signing_verdict_at(&bare, created + 10),
            KeyVerdict::NotAuthorized
        );
        let control = with_uid_sigs(fixture, |sk| vec![uid_self_sig(sk, created, None, false)]);
        assert_eq!(
            signing_verdict_at(&control, created + 10),
            KeyVerdict::Authorized
        );
    }

    #[test]
    fn a_user_id_revocation_takes_effect_from_its_creation_time() {
        // Historical judgement (key-validity.md): a signature made before the
        // only User ID was revoked was made while the key was certified; one
        // made afterwards was not.
        let fixture = &keys::V4_SIGN;
        let created = fixture.secret.primary_key.created_at().as_secs();
        let sk = with_uid_sigs(fixture, |sk| {
            vec![
                uid_self_sig(sk, created, None, false),
                uid_self_sig(sk, created + 100, None, true),
            ]
        });
        assert_eq!(
            signing_verdict_at(&sk, created + 50),
            KeyVerdict::Authorized
        );
        assert_eq!(
            signing_verdict_at(&sk, created + 200),
            KeyVerdict::NotAuthorized
        );
    }

    #[test]
    fn an_expired_self_signature_stops_speaking_for_the_key() {
        // RFC 9580 §5.2.3.18: after its Signature Expiration Time a
        // self-signature is no longer valid, so the key it certified is not
        // usable at a later time (it does not silently become unconstrained).
        let fixture = &keys::V4_SIGN;
        let created = fixture.secret.primary_key.created_at().as_secs();
        let sk = with_uid_sigs(fixture, |sk| {
            vec![uid_self_sig(sk, created, Some(100), false)]
        });
        assert_eq!(
            signing_verdict_at(&sk, created + 50),
            KeyVerdict::Authorized
        );
        assert_eq!(
            signing_verdict_at(&sk, created + 200),
            KeyVerdict::NotAuthorized
        );
    }

    #[test]
    fn extending_a_keys_expiry_also_outlives_an_old_self_signature_expiry() {
        // Changing the expiry re-issues the self-signature. A Signature
        // Expiration Time copied from the template would end the new
        // self-signature, and with it the key, at the old date (§5.2.3.18).
        let fixture = &keys::V4_SIGN;
        let created = fixture.secret.primary_key.created_at().as_secs();
        let sk = with_uid_sigs(fixture, |sk| {
            vec![uid_self_sig(sk, created, Some(100), false)]
        });
        let block = sk.to_armored_string(armor_opts()).unwrap();
        let expiry = created + 365 * 86_400;
        let out =
            update_key_expiration_internal(0, &block, None, expiry as u64, None).expect("extend");
        let extended = SignedSecretKey::from_string(&out.secret).unwrap().0;
        assert_eq!(
            signing_verdict_at(&extended, created + 200),
            KeyVerdict::Authorized
        );
        assert_eq!(
            signing_verdict_at(&extended, expiry + 1),
            KeyVerdict::Expired
        );
    }

    #[test]
    fn a_re_issued_v4_self_signature_names_its_issuer_by_key_id_too() {
        // RFC 9580 §5.2.3.12: a v4 signature SHOULD carry an Issuer Key ID
        // for verifiers that look keys up by ID; rPGP's own builders add it,
        // and a re-issued self-signature must not lose it.
        let fixture = &keys::V4_SIGN;
        let expiry = fixture.secret.primary_key.created_at().as_secs() + 365 * 86_400;
        let out =
            update_key_expiration_internal(0, &fixture.secret_armored, None, expiry as u64, None)
                .unwrap();
        let sk = SignedSecretKey::from_string(&out.secret).unwrap().0;
        let key_id = sk.primary_key.legacy_key_id();
        for sig in sk.details.users[0]
            .signatures
            .iter()
            .chain(sk.secret_subkeys.iter().flat_map(|s| s.signatures.iter()))
        {
            assert!(
                sig.issuer_key_id().iter().any(|id| **id == key_id),
                "{:?} lacks an Issuer Key ID",
                sig.typ()
            );
        }
    }

    #[test]
    fn a_key_id_only_revocation_certificate_still_names_its_target() {
        // GnuPG before 2.1 and PGP wrote v4 revocations naming the issuer by
        // Key ID only (§5.2.3.12). The host resolves a key by Key ID too.
        use pgp::types::Timestamp;
        let fixture = &keys::V4_SIGN;
        let primary = &fixture.secret.primary_key;
        let mut cfg =
            SignatureConfig::from_key(rand::thread_rng(), primary, SignatureType::KeyRevocation)
                .unwrap();
        cfg.hashed_subpackets = vec![
            Subpacket::regular(SubpacketData::SignatureCreationTime(Timestamp::now())).unwrap(),
        ];
        cfg.unhashed_subpackets =
            vec![Subpacket::regular(SubpacketData::IssuerKeyId(primary.legacy_key_id())).unwrap()];
        let rev = cfg
            .sign_key(primary, &Password::empty(), primary.public_key())
            .unwrap();
        let mut block = Vec::new();
        pgp::armor::write(
            &pgp::packet::Packet::from(rev),
            pgp::armor::BlockType::Signature,
            &mut block,
            None,
            true,
        )
        .unwrap();
        let target = extract_rev_cert_target_fpr_internal(&String::from_utf8(block).unwrap())
            .expect("a Key ID names the target");
        assert!(target.eq_ignore_ascii_case(&primary.legacy_key_id().to_string()));
    }

    #[test]
    fn a_wrong_passphrase_on_an_expiry_change_is_a_bad_passphrase() {
        // INV-MAP: a passphrase problem must reach the GUI as one, not as
        // "invalid input" from the signing step that failed afterwards.
        let key1 = &crate::testutil::corpus::KEY1;
        crate::cache::PASSWORD_CACHE.remove_by_fpr(&key1.primary_key.fingerprint().to_string());
        let created = key1.primary_key.created_at().as_secs();
        let result = update_key_expiration_internal(
            0,
            crate::testutil::corpus::KEY1_SECRET,
            None,
            (created + 365 * 86_400) as u64,
            Some(crate::testutil::cb::pwd_wrong),
        );
        assert_eq!(result.err(), Some(GfrStatus::ErrorBadPassphrase));
        assert!(crate::err::last_error().contains("passphrase"));
    }

    #[test]
    fn a_wrong_passphrase_on_a_subkey_revocation_is_a_bad_passphrase() {
        let key1 = &crate::testutil::corpus::KEY1;
        let fpr = key1.primary_key.fingerprint().to_string();
        let subkey = key1.secret_subkeys[0].key.fingerprint().to_string();
        crate::cache::PASSWORD_CACHE.remove_by_fpr(&fpr);
        let result = revoke_subkey_internal(
            0,
            crate::testutil::corpus::KEY1_SECRET,
            &subkey,
            GfrRevocationCode::NoReason,
            None,
            Some(crate::testutil::cb::pwd_wrong),
        );
        assert_eq!(result.err(), Some(GfrStatus::ErrorBadPassphrase));
        crate::cache::PASSWORD_CACHE.remove_by_fpr(&fpr);
    }

    #[test]
    fn a_v6_revocation_certificate_carries_its_primary_key() {
        // RFC 9580 §10.1.3: "a version 6 revocation certificate MUST include
        // the primary key packet, as described in Section 10.1.2". A v4 one
        // may stay the bare signature, as GnuPG writes it.
        use pgp::packet::{Packet, PacketParser};
        let packets_of = |armored: &str| -> Vec<Packet> {
            let bytes = crate::crypto::dearmor_all(armored.as_bytes()).into_owned();
            PacketParser::new(Cursor::new(bytes)).flatten().collect()
        };
        for (fixture, with_key) in [(&*keys::V6_SIGN, true), (&*keys::V4_SIGN, false)] {
            let cert = generate_key_rev_cert_internal(
                0,
                &fixture.secret_armored,
                GfrRevocationCode::NoReason,
                None,
                None,
            )
            .unwrap();
            let packets = packets_of(&cert);
            match (&packets[..], with_key) {
                ([Packet::PublicKey(key), Packet::Signature(sig)], true) => {
                    assert_eq!(
                        key.fingerprint().to_string().to_uppercase(),
                        fixture.primary_fpr
                    );
                    assert_eq!(sig.typ(), Some(SignatureType::KeyRevocation));
                }
                ([Packet::Signature(sig)], false) => {
                    assert_eq!(sig.typ(), Some(SignatureType::KeyRevocation))
                }
                (other, _) => panic!("{}: {other:?}", fixture.primary_fpr),
            }
            let revoked = import_rev_cert_internal(&fixture.secret_armored, &cert)
                .expect("the certificate imports");
            let sk = SignedSecretKey::from_string(&revoked.secret).unwrap().0;
            assert!(is_primary_key_revoked(
                &sk.primary_key.public_key(),
                &sk.details.revocation_signatures
            ));
        }
    }

    #[test]
    fn trust_packets_in_an_imported_key_are_ignored() {
        // RFC 9580 §5.10: Trust packets are implementation-local and
        // "SHOULD be ignored on any input other than local keyring files";
        // GnuPG keyring exports and backups carry them after keys, User IDs
        // and signatures. They must not cut a certificate short.
        use pgp::packet::{Packet, PacketParser};
        use pgp::ser::Serialize;
        let fixture = &keys::V4_SIGN;
        let bytes = crate::crypto::dearmor_all(fixture.public_armored.as_bytes()).into_owned();
        let trust = crate::testutil::packets::packet(12, &[0x00, 0x00]);
        let mut with_trust = Vec::new();
        for packet in PacketParser::new(Cursor::new(bytes)).flatten() {
            with_trust.extend(packet.to_bytes().unwrap());
            if !matches!(packet, Packet::PublicKey(_)) {
                continue;
            }
            with_trust.extend_from_slice(&trust);
        }
        let mut armored = Vec::new();
        pgp::armor::write(
            &MergedRawKeys {
                packet_bytes: with_trust,
            },
            BlockType::PublicKey,
            &mut armored,
            None,
            true,
        )
        .unwrap();
        let meta = extract_metadata_many_internal(&String::from_utf8(armored).unwrap())
            .expect("the certificate imports");
        assert_eq!(meta.len(), 1);
        let reference = build_public_metadata(&SignedPublicKey::from(fixture.secret.clone()));
        assert_eq!(
            meta[0].user_ids.len(),
            reference.user_ids.len(),
            "User IDs kept"
        );
        assert_eq!(
            meta[0].subkeys.len(),
            reference.subkeys.len(),
            "subkeys kept"
        );
    }

    #[test]
    fn a_re_issue_starts_from_the_self_signature_in_force() {
        // RFC 9580 §5.2.3.10: the most recent valid self-signature governs.
        // A re-issue must start from that one, not from an older signature
        // that happens to carry Key Flags: that would resurrect the older
        // signature's expiry and preferences.
        let fixture = &keys::V4_SIGN;
        let created = fixture.secret.primary_key.created_at().as_secs();
        let mut sk = fixture.secret.clone();
        let mut old = self_sig_with(
            &sk,
            0,
            SignatureType::CertPositive,
            created,
            flags(true, false),
            true,
        );
        // The older one also expired the key after a year.
        {
            use pgp::types::{Duration, Password, Timestamp};
            let primary = &sk.primary_key;
            let mut cfg = old.config().unwrap().clone();
            cfg.hashed_subpackets.push(
                Subpacket::regular(SubpacketData::KeyExpirationTime(Duration::from_secs(
                    365 * 86_400,
                )))
                .unwrap(),
            );
            cfg.hashed_subpackets
                .retain(|sp| !matches!(sp.data, SubpacketData::SignatureCreationTime(_)));
            cfg.hashed_subpackets.push(
                Subpacket::regular(SubpacketData::SignatureCreationTime(Timestamp::from_secs(
                    created,
                )))
                .unwrap(),
            );
            old = cfg
                .sign_certification(
                    primary,
                    primary.public_key(),
                    &Password::empty(),
                    Tag::UserId,
                    &sk.details.users[0].id,
                )
                .unwrap();
        }
        // The newer one carries no Key Flags and no expiry.
        let newer = {
            use pgp::types::{Password, Timestamp};
            let primary = &sk.primary_key;
            let mut cfg =
                SignatureConfig::from_key(rand::thread_rng(), primary, SignatureType::CertPositive)
                    .unwrap();
            cfg.hashed_subpackets = vec![
                Subpacket::regular(SubpacketData::SignatureCreationTime(Timestamp::from_secs(
                    created + 10,
                )))
                .unwrap(),
                Subpacket::regular(SubpacketData::IssuerFingerprint(primary.fingerprint()))
                    .unwrap(),
                Subpacket::regular(SubpacketData::IsPrimary(true)).unwrap(),
            ];
            cfg.sign_certification(
                primary,
                primary.public_key(),
                &Password::empty(),
                Tag::UserId,
                &sk.details.users[0].id,
            )
            .unwrap()
        };
        sk.details.users[0].signatures = vec![old, newer];
        let block = sk.to_armored_string(armor_opts()).unwrap();
        let before = SignedPublicKey::from(sk.clone());
        assert_eq!(
            primary_key_expires_at(&before),
            0,
            "precondition: never expires"
        );

        let uid = String::from_utf8_lossy(sk.details.users[0].id.id()).into_owned();
        let out = crate::user_id::set_primary_user_id_internal(0, &block, &uid, None).unwrap();
        let after = SignedPublicKey::from(SignedSecretKey::from_string(&out).unwrap().0);
        assert_eq!(
            primary_key_expires_at(&after),
            0,
            "the re-issue resurrected an old expiry"
        );
    }

    #[test]
    fn a_re_issued_self_signature_never_carries_a_revocation_key() {
        // RFC 9580 §5.2.3.23: the Revocation Key subpacket is deprecated and
        // "Applications MUST NOT generate such a subpacket". Re-issuing a
        // self-signature that carries one (made by older software) generates
        // a new signature, so the subpacket must not be copied over.
        use pgp::types::{Password, RevocationKey, RevocationKeyClass, Timestamp};
        let fixture = &keys::V4_SIGN;
        let created = fixture.secret.primary_key.created_at().as_secs();
        let mut sk = fixture.secret.clone();
        let primary = &sk.primary_key;
        let mut cfg =
            SignatureConfig::from_key(rand::thread_rng(), primary, SignatureType::CertPositive)
                .unwrap();
        cfg.hashed_subpackets = vec![
            Subpacket::regular(SubpacketData::SignatureCreationTime(Timestamp::from_secs(
                created,
            )))
            .unwrap(),
            Subpacket::regular(SubpacketData::IssuerFingerprint(primary.fingerprint())).unwrap(),
            Subpacket::regular(SubpacketData::KeyFlags(flags(true, false))).unwrap(),
            Subpacket::regular(SubpacketData::RevocationKey(RevocationKey::new(
                RevocationKeyClass::Default,
                pgp::crypto::public_key::PublicKeyAlgorithm::EdDSALegacy,
                &[0x11; 20],
            )))
            .unwrap(),
        ];
        let sig = cfg
            .sign_certification(
                primary,
                primary.public_key(),
                &Password::empty(),
                Tag::UserId,
                &sk.details.users[0].id,
            )
            .unwrap();
        sk.details.users[0].signatures = vec![sig];
        let block = sk.to_armored_string(armor_opts()).unwrap();
        let out =
            update_key_expiration_internal(0, &block, None, (created + 365 * 86_400) as u64, None)
                .unwrap();
        let after = SignedSecretKey::from_string(&out.secret).unwrap().0;
        for sig in &after.details.users[0].signatures {
            assert!(
                sig.revocation_key().is_none(),
                "a re-issued self-signature carries a Revocation Key"
            );
        }
    }

    #[test]
    fn a_revocation_reason_must_fit_the_revocation_type() {
        // RFC 9580 §5.2.3.31: codes 1-3 are for Key Revocation signatures,
        // code 32 ("User ID information is no longer valid") for
        // Certification Revocation signatures; 0 fits both.
        let fixture = &keys::V4_SIGN;
        let uid = String::from_utf8_lossy(fixture.secret.details.users[0].id.id()).into_owned();
        assert_eq!(
            generate_key_rev_cert_internal(
                0,
                &fixture.secret_armored,
                GfrRevocationCode::UserIdInvalid,
                None,
                None
            )
            .err(),
            Some(GfrStatus::ErrorInvalidInput)
        );
        assert_eq!(
            revoke_subkey_internal(
                0,
                &fixture.secret_armored,
                fixture.enc_subkey_fpr(),
                GfrRevocationCode::UserIdInvalid,
                None,
                None
            )
            .err(),
            Some(GfrStatus::ErrorInvalidInput)
        );
        for code in [
            GfrRevocationCode::Superseded,
            GfrRevocationCode::Compromised,
            GfrRevocationCode::Retired,
        ] {
            assert_eq!(
                crate::user_id::revoke_user_id_internal(
                    0,
                    &fixture.secret_armored,
                    &uid,
                    code,
                    None,
                    None
                )
                .err(),
                Some(GfrStatus::ErrorInvalidInput),
                "{code:?} on a User ID"
            );
        }
        // Controls: each type with a reason that fits it.
        assert!(
            generate_key_rev_cert_internal(
                0,
                &fixture.secret_armored,
                GfrRevocationCode::Retired,
                None,
                None
            )
            .is_ok()
        );
        assert!(
            crate::user_id::revoke_user_id_internal(
                0,
                &fixture.secret_armored,
                &uid,
                GfrRevocationCode::UserIdInvalid,
                None,
                None
            )
            .is_ok()
        );
    }

    #[test]
    fn a_weak_hash_binding_over_a_pq_subkey_never_counts() {
        // RFC 9980 §7.2: subkey binding signatures over its algorithms "MUST
        // NOT be made with MD5, SHA-1, or RIPEMD-160. A receiving
        // implementation MUST treat such a signature as invalid", whatever
        // its date. The same SHA-1 binding over a classical subkey still
        // falls under the RFC 9580 §9.5 cutoff and counts.
        use crate::types::{GfrKeyAlgo, GfrOpenPGPKeyVersion};
        use pgp::crypto::hash::HashAlgorithm;
        use pgp::packet::KeyFlags;
        use pgp::types::{Password, Timestamp};

        let rsa = &keys::V4_RSA1024.secret;
        let primary = &rsa.primary_key;
        // ML-KEM-768+X25519 is the one PQ algorithm a v4 key may carry (§3.5).
        let donor = keys::generate(
            "Donor <donor@example.test>",
            &keys::cfg(GfrKeyAlgo::ED25519, true, false, GfrOpenPGPKeyVersion::V4),
            &[keys::cfg(
                GfrKeyAlgo::KYBER768X25519,
                false,
                true,
                GfrOpenPGPKeyVersion::V4,
            )],
        );
        let pq_sub = donor.secret_subkeys[0].key.public_key();
        let rsa_sub = rsa.secret_subkeys[0].key.public_key();

        let sha1_binding = |subkey: &PublicSubkey| {
            let mut flags = KeyFlags::default();
            flags.set_encrypt_comms(true);
            flags.set_encrypt_storage(true);
            let mut cfg = SignatureConfig::v4(
                SignatureType::SubkeyBinding,
                primary.algorithm(),
                HashAlgorithm::Sha1,
            );
            cfg.hashed_subpackets = vec![
                Subpacket::regular(SubpacketData::SignatureCreationTime(Timestamp::from_secs(
                    WEAK_CERTIFICATION_HASH_CUTOFF - 86_400,
                )))
                .unwrap(),
                Subpacket::regular(SubpacketData::IssuerFingerprint(primary.fingerprint()))
                    .unwrap(),
                Subpacket::regular(SubpacketData::KeyFlags(flags)).unwrap(),
            ];
            cfg.sign_subkey_binding(primary, primary.public_key(), &Password::empty(), subkey)
                .unwrap()
        };

        let classical = [sha1_binding(&rsa_sub)];
        assert!(
            subkey_binding_signature(primary.public_key(), &rsa_sub, &classical).is_some(),
            "control: a pre-cutoff SHA-1 binding over a classical subkey counts"
        );
        let pq = [sha1_binding(&pq_sub)];
        assert!(
            pq[0]
                .verify_subkey_binding(primary.public_key(), &pq_sub)
                .is_ok(),
            "precondition: the PQ binding is mathematically valid"
        );
        assert!(subkey_binding_signature(primary.public_key(), &pq_sub, &pq).is_none());
    }

    #[test]
    fn the_rsa_size_gate_spares_non_rsa_keys() {
        // §12.4 concerns RSA only; the 255-bit Ed25519 fixture is fine.
        assert!(!rsa_key_too_small(
            keys::V4_SIGN.secret.primary_key.public_params()
        ));
    }

    fn far_future() -> u64 {
        u64::from(pgp::types::Timestamp::now().as_secs()) + 365 * 24 * 3600
    }

    #[test]
    fn changing_the_expiry_keeps_a_revoked_user_id_revoked() {
        // RFC 9580 §5.2.1.13 / §5.2.3.10: a certification revocation (0x30)
        // retires the User ID; re-issuing the self-signatures must not drop it.
        let second = "Second <second@test.example>";
        let block =
            crate::user_id::add_user_id_internal(0, &keys::V4_SIGN.secret_armored, second, None)
                .unwrap();
        let block = crate::user_id::revoke_user_id_internal(
            0,
            &block,
            second,
            GfrRevocationCode::UserIdInvalid,
            None,
            None,
        )
        .unwrap();
        let out = update_key_expiration_internal(0, &block, None, far_future(), None).unwrap();

        let sk = parse_secret(&out.secret);
        let user = sk
            .details
            .users
            .iter()
            .find(|u| u.id.id() == second.as_bytes())
            .expect("the second User ID survives");
        assert!(is_user_id_revoked(sk.primary_key.public_key(), user));
    }

    #[test]
    fn a_v6_expiry_update_draws_a_fresh_salt() {
        // RFC 9580 §5.2.3: the v6 salt is "a random value"; §13.2: a new salt
        // per signature.
        let salt_of = |sk: &SignedSecretKey| -> Vec<u8> {
            let sig = sk
                .details
                .direct_signatures
                .iter()
                .find(|s| matches!(s.typ(), Some(SignatureType::Key)))
                .expect("a direct key signature");
            match &sig.config().unwrap().version_specific {
                pgp::packet::SignatureVersionSpecific::V6 { salt } => salt.to_vec(),
                other => panic!("not a v6 signature: {other:?}"),
            }
        };
        let before = salt_of(&keys::V6_SIGN.secret);
        let out = update_key_expiration_internal(
            0,
            &keys::V6_SIGN.secret_armored,
            None,
            far_future(),
            None,
        )
        .unwrap();
        assert_ne!(salt_of(&parse_secret(&out.secret)), before);
    }

    #[test]
    fn an_expiry_update_never_reissues_a_sha1_self_signature() {
        // RFC 9580 §9.5: "MUST NOT generate signatures with MD5, SHA-1, or
        // RIPEMD-160", even when the self-signature being replaced used one.
        // The SHA-1 self-signature predates the weak-hash cutoff, as on a
        // genuinely old key: a later one is not a valid self-signature at
        // all, so it could not be a template (round two, INV-REISSUE).
        use pgp::crypto::hash::HashAlgorithm;
        let mut sk = keys::V4_RSA2048.secret.clone();
        let primary = sk.primary_key.clone();
        let mut cfg = SignatureConfig::v4(
            SignatureType::CertPositive,
            primary.algorithm(),
            HashAlgorithm::Sha1,
        );
        cfg.hashed_subpackets = vec![
            Subpacket::regular(SubpacketData::SignatureCreationTime(
                pgp::types::Timestamp::from_secs(WEAK_CERTIFICATION_HASH_CUTOFF - 86_400),
            ))
            .unwrap(),
            Subpacket::regular(SubpacketData::IssuerFingerprint(primary.fingerprint())).unwrap(),
        ];
        let user = &mut sk.details.users[0];
        let sha1 = cfg
            .sign_certification(
                &primary,
                primary.public_key(),
                &Password::empty(),
                Tag::UserId,
                &user.id,
            )
            .unwrap();
        user.signatures = vec![sha1];
        let block = sk.to_armored_string(armor_opts()).unwrap();

        let out = update_key_expiration_internal(0, &block, None, far_future(), None).unwrap();
        let sk = parse_secret(&out.secret);
        for sig in &sk.details.users[0].signatures {
            assert_ne!(sig.hash_alg(), Some(HashAlgorithm::Sha1));
        }
    }

    #[test]
    fn subkey_expires_at_is_zero_by_default() {
        let cert = SignedPublicKey::from_string(&keys::V4_SIGN.public_armored)
            .expect("parses")
            .0;
        for sub in &cert.public_subkeys {
            assert_eq!(
                subkey_expires_at(&cert.primary_key, &sub.key, &sub.signatures),
                0
            );
        }
    }

    #[test]
    fn an_expiry_duration_of_zero_means_never() {
        // §5.2.3.13 again, at the arithmetic level.
        assert_eq!(expiration_duration(1_700_000_000, 0), Ok(None));
    }

    #[test]
    fn an_expiry_before_the_creation_time_is_rejected() {
        // The subpacket can only express a forward duration, so a backdated
        // expiry is not representable and must be refused rather than
        // wrapping around.
        assert_eq!(
            expiration_duration(1_700_000_000, 1_600_000_000),
            Err(GfrStatus::ErrorInvalidInput)
        );
    }

    #[test]
    fn an_expiry_equal_to_the_creation_time_is_rejected() {
        assert_eq!(
            expiration_duration(1_700_000_000, 1_700_000_000),
            Err(GfrStatus::ErrorInvalidInput)
        );
    }

    #[test]
    fn an_expiry_one_second_after_creation_is_accepted() {
        assert!(matches!(
            expiration_duration(1_700_000_000, 1_700_000_001),
            Ok(Some(_))
        ));
    }

    #[test]
    fn an_expiry_beyond_the_u32_duration_range_is_rejected() {
        // The subpacket holds a 4-octet duration; anything longer cannot be
        // encoded and must be refused rather than silently truncated.
        assert_eq!(
            expiration_duration(0, u64::from(u32::MAX) + 2),
            Err(GfrStatus::ErrorInvalidInput)
        );
    }

    // -- update_key_expiration_internal --------------------------------------

    #[test]
    fn setting_an_expiry_lands_in_the_self_signature() {
        let key = &keys::V4_SIGN;
        let created = parse_secret(&key.secret_armored)
            .primary_key
            .created_at()
            .as_secs();
        let out = update_key_expiration_internal(
            0,
            &key.secret_armored,
            None,
            u64::from(created) + 86_400,
            None,
        )
        .expect("update");
        assert_eq!(meta_of(&out.secret).expires_at, created + 86_400);
    }

    #[test]
    fn clearing_an_expiry_sets_it_back_to_never() {
        let key = &keys::V4_SHORT_EXPIRY;
        let out =
            update_key_expiration_internal(0, &key.secret_armored, None, 0, None).expect("clear");
        assert_eq!(meta_of(&out.secret).expires_at, 0);
    }

    #[test]
    fn a_backdated_expiry_is_rejected() {
        assert_eq!(
            update_key_expiration_internal(0, &keys::V4_SIGN.secret_armored, None, 1, None).err(),
            Some(GfrStatus::ErrorInvalidInput)
        );
    }

    #[test]
    fn a_subkey_expiry_is_set_independently_of_the_primary() {
        let key = &keys::V4_SIGN;
        let sub_fpr = key.enc_subkey_fpr();
        let created = parse_secret(&key.secret_armored)
            .primary_key
            .created_at()
            .as_secs();
        let out = update_key_expiration_internal(
            0,
            &key.secret_armored,
            Some(sub_fpr),
            u64::from(created) + 86_400,
            None,
        )
        .expect("update subkey");

        let meta = meta_of(&out.secret);
        assert_eq!(meta.expires_at, 0, "the primary is untouched");
        let sub = meta
            .subkeys
            .iter()
            .find(|s| s.fpr.eq_ignore_ascii_case(sub_fpr))
            .expect("the target subkey");
        assert_eq!(sub.expires_at, created + 86_400);
    }

    #[test]
    fn updating_the_expiry_of_an_unknown_component_fails() {
        assert!(
            update_key_expiration_internal(
                0,
                &keys::V4_SIGN.secret_armored,
                Some("0000000000000000000000000000000000000000"),
                0,
                None
            )
            .is_err()
        );
    }

    #[test]
    fn an_expiry_update_preserves_the_key_flags() {
        // Re-issuing the self-signature must not drop the capabilities.
        let key = &keys::V4_SIGN;
        let created = parse_secret(&key.secret_armored)
            .primary_key
            .created_at()
            .as_secs();
        let out = update_key_expiration_internal(
            0,
            &key.secret_armored,
            None,
            u64::from(created) + 86_400,
            None,
        )
        .expect("update");
        let meta = meta_of(&out.secret);
        assert!(meta.can_certify);
        assert!(meta.subkeys.iter().any(|s| s.can_encrypt));
    }

    // -- delete_subkey / revoke_subkey ---------------------------------------

    #[test]
    fn deleting_a_subkey_removes_it() {
        let key = &keys::V4_SIGN;
        let out =
            delete_subkey_internal(&key.secret_armored, key.enc_subkey_fpr()).expect("delete");
        let meta = meta_of(&out.secret);
        assert_eq!(meta.subkeys.len(), 1);
        assert!(
            !meta
                .subkeys
                .iter()
                .any(|s| s.fpr.eq_ignore_ascii_case(key.enc_subkey_fpr()))
        );
    }

    #[test]
    fn deleting_an_unknown_subkey_is_invalid_input() {
        assert_eq!(
            delete_subkey_internal(&keys::V4_SIGN.secret_armored, "DEADBEEF").err(),
            Some(GfrStatus::ErrorInvalidInput)
        );
    }

    #[test]
    fn a_subkey_revocation_signature_verifies() {
        let key = parse_secret(&keys::V4_REVOKED_SUBKEY.secret_armored);
        let primary = key.primary_key.public_key();
        let revoked = key
            .secret_subkeys
            .iter()
            .find(|s| is_subkey_revoked(primary, s.key.public_key(), &s.signatures))
            .expect("a revoked subkey");
        let rev = revoked
            .signatures
            .iter()
            .find(|s| matches!(s.typ(), Some(SignatureType::SubkeyRevocation)))
            .expect("a revocation signature");
        assert!(
            rev.verify_subkey_binding(primary, revoked.key.public_key())
                .is_ok()
        );
    }

    #[test]
    fn revoking_an_unknown_subkey_is_invalid_input() {
        assert_eq!(
            revoke_subkey_internal(
                0,
                &keys::V4_SIGN.secret_armored,
                "0000000000000000000000000000000000000000",
                GfrRevocationCode::Retired,
                None,
                None
            )
            .err(),
            Some(GfrStatus::ErrorInvalidInput)
        );
    }

    #[test]
    fn revoking_a_subkey_does_not_delete_it() {
        let meta = meta_of(&keys::V4_REVOKED_SUBKEY.secret_armored);
        assert_eq!(
            meta.subkeys.len(),
            2,
            "revocation is a statement, not removal"
        );
    }

    // -- revocation certificates --------------------------------------------

    #[test]
    fn a_revocation_certificate_is_an_armored_signature_block() {
        let cert = generate_key_rev_cert_internal(
            0,
            &keys::V4_SIGN.secret_armored,
            GfrRevocationCode::Superseded,
            None,
            None,
        )
        .expect("rev cert");
        assert!(cert.contains("BEGIN PGP"));
        crate::testutil::assert::armor_has_crc24(&cert); // v4: §6.1 interop exception
    }

    #[test]
    fn a_revocation_certificate_names_its_target() {
        let cert = generate_key_rev_cert_internal(
            0,
            &keys::V4_SIGN.secret_armored,
            GfrRevocationCode::Retired,
            None,
            None,
        )
        .expect("rev cert");
        let target = extract_rev_cert_target_fpr_internal(&cert).expect("target");
        assert_eq!(target.to_uppercase(), keys::V4_SIGN.primary_fpr);
    }

    #[test]
    fn extracting_a_target_from_a_non_revocation_block_fails() {
        assert!(extract_rev_cert_target_fpr_internal(&keys::V4_SIGN.public_armored).is_err());
    }

    #[test]
    fn extracting_a_target_from_garbage_fails() {
        assert!(extract_rev_cert_target_fpr_internal("junk").is_err());
    }

    #[test]
    fn importing_a_foreign_revocation_certificate_is_rejected() {
        // A revocation that does not verify under the base primary must never
        // be stored, however convincingly it names it.
        let foreign = generate_key_rev_cert_internal(
            0,
            &keys::V6_SIGN.secret_armored,
            GfrRevocationCode::Compromised,
            None,
            None,
        )
        .expect("rev cert");
        assert!(import_rev_cert_internal(&keys::V4_SIGN.secret_armored, &foreign).is_err());
    }

    #[test]
    fn importing_a_genuine_revocation_certificate_revokes_the_key() {
        let cert = generate_key_rev_cert_internal(
            0,
            &keys::V4_SIGN.secret_armored,
            GfrRevocationCode::Compromised,
            None,
            None,
        )
        .expect("rev cert");
        let out = import_rev_cert_internal(&keys::V4_SIGN.secret_armored, &cert).expect("import");
        assert!(meta_of(&out.secret).is_revoked);
    }

    #[test]
    fn importing_garbage_as_a_revocation_certificate_fails() {
        assert!(import_rev_cert_internal(&keys::V4_SIGN.secret_armored, "junk").is_err());
    }

    // -- extract_public_key / merge -----------------------------------------

    #[test]
    fn extracting_the_public_key_strips_the_secret_material() {
        let public = extract_public_key_internal(&keys::V4_SIGN.secret_armored).expect("extract");
        assert!(public.contains("BEGIN PGP PUBLIC KEY BLOCK"));
        assert!(!public.contains("PRIVATE KEY BLOCK"));
        assert!(SignedSecretKey::from_string(&public).is_err());
    }

    #[test]
    fn the_extracted_public_key_keeps_the_fingerprint() {
        let public = extract_public_key_internal(&keys::V4_SIGN.secret_armored).expect("extract");
        let cert = SignedPublicKey::from_string(&public).expect("parses").0;
        assert_eq!(
            cert.primary_key.fingerprint().to_string().to_uppercase(),
            keys::V4_SIGN.primary_fpr
        );
    }

    #[test]
    fn extracting_a_public_key_from_a_public_block_fails() {
        assert_eq!(
            extract_public_key_internal(&keys::V4_SIGN.public_armored).err(),
            Some(GfrStatus::ErrorInvalidInput)
        );
    }

    #[test]
    fn filtering_keeps_only_the_requested_certificates() {
        let both = format!(
            "{}\n{}",
            keys::V4_SIGN.public_armored,
            keys::V6_SIGN.public_armored
        );
        let v4 = SignedPublicKey::from(keys::V4_SIGN.secret.clone());
        let v4_fpr = v4.fingerprint().to_string();
        let v4_id = v4.legacy_key_id().to_string();

        for wanted in [v4_fpr.as_str(), v4_id.as_str()] {
            let filtered = filter_public_keys_internal(&both, &[wanted]).expect("filter");
            assert_eq!(filtered.dropped, 1, "{wanted}");
            let metas = extract_metadata_many_internal(&filtered.armored).expect("metadata");
            assert_eq!(metas.len(), 1);
            assert_eq!(
                metas[0].fpr.to_ascii_uppercase(),
                v4_fpr.to_ascii_uppercase()
            );
        }

        // A short key ID is anyone's to choose, so it never matches.
        let short = &v4_id[v4_id.len() - 8..];
        let filtered = filter_public_keys_internal(&both, &[short]).expect("filter");
        assert!(filtered.armored.is_empty());
        assert_eq!(filtered.dropped, 2);
    }

    #[test]
    fn merging_two_public_keys_keeps_both() {
        let merged = export_merged_public_keys(&[
            &keys::V4_SIGN.public_armored,
            &keys::V6_SIGN.public_armored,
        ])
        .expect("merge");
        let metas = extract_metadata_many_internal(&merged).expect("metadata");
        assert_eq!(metas.len(), 2);
    }

    #[test]
    fn merging_accepts_a_secret_block_and_publishes_its_public_half() {
        let merged = export_merged_public_keys(&[&keys::V4_SIGN.secret_armored]).expect("merge");
        assert!(!merged.contains("PRIVATE KEY BLOCK"));
    }

    #[test]
    fn merging_v4_keys_emits_armor_with_a_crc24_footer() {
        // RFC 9580 §6.1 interoperability exception: v4-only armor carries the
        // CRC24 footer (GnuPG 2.4); v6 armor never does (utils::crc24_policy_tests).
        let merged = export_merged_public_keys(&[&keys::V4_SIGN.public_armored]).expect("merge");
        crate::testutil::assert::armor_has_crc24(&merged);
    }

    #[test]
    fn merging_an_empty_list_yields_an_empty_or_failed_result() {
        match export_merged_public_keys(&[]) {
            Ok(out) => assert!(extract_metadata_many_internal(&out).is_err()),
            Err(status) => assert!((status as i32) < 0),
        }
    }

    #[test]
    fn merging_garbage_fails_cleanly() {
        let outcome =
            std::panic::catch_unwind(|| export_merged_public_keys(&["junk", "more junk"]).is_ok());
        assert!(outcome.is_ok());
    }

    #[test]
    fn merging_secret_keys_keeps_the_secret_material() {
        let merged = export_merged_secret_keys(&[&keys::V4_SIGN.secret_armored]).expect("merge");
        assert!(SignedSecretKey::from_string(&merged).is_ok());
    }

    #[test]
    fn merging_a_key_with_itself_is_idempotent() {
        let merged = export_merged_public_keys(&[
            &keys::V4_SIGN.public_armored,
            &keys::V4_SIGN.public_armored,
        ])
        .expect("merge");
        assert_eq!(
            extract_metadata_many_internal(&merged).expect("meta").len(),
            1
        );
    }

    #[test]
    fn merge_key_blocks_combines_a_public_update_into_a_secret_key() {
        // The refresh case: a newer public copy (say with a fresh signature)
        // merged onto the locally held secret key must keep the secret.
        let merged =
            merge_key_block_internal(&keys::V4_SIGN.secret_armored, &keys::V4_SIGN.public_armored)
                .expect("merge");
        assert!(SignedSecretKey::from_string(&merged.secret).is_ok());
    }

    #[test]
    fn merging_a_public_update_keeps_a_new_public_only_subkey() {
        // The certificate gained a subkey whose secret part lives elsewhere
        // (another machine, a card). Merging its public copy onto the local
        // secret key must keep that subkey as a public subkey, not drop it.
        let (mut local, _) =
            SignedSecretKey::from_string(&keys::V4_SIGN.secret_armored).expect("parse");
        let new_sub = local.secret_subkeys.pop().expect("a subkey");
        let new_fpr = new_sub.key.fingerprint();
        let local_block = export_secret_key(local).expect("export").secret;

        let merged =
            merge_key_block_internal(&local_block, &keys::V4_SIGN.public_armored).expect("merge");
        let (merged_sk, _) = SignedSecretKey::from_string(&merged.secret).expect("secret kept");
        assert!(
            merged_sk
                .public_subkeys
                .iter()
                .any(|s| s.key.fingerprint() == new_fpr),
            "the new public-only subkey must survive the merge"
        );
        assert!(
            merged_sk
                .secret_subkeys
                .iter()
                .all(|s| s.key.fingerprint() != new_fpr)
        );
        let (merged_pk, _) = SignedPublicKey::from_string(&merged.public).expect("public");
        assert_eq!(
            merged_pk.public_subkeys.len(),
            SignedPublicKey::from_string(&keys::V4_SIGN.public_armored)
                .expect("parse")
                .0
                .public_subkeys
                .len()
        );
    }

    /// A subkey whose secret lives elsewhere (a card) is listed, so it must be
    /// editable where only the primary's secret is needed: revoke it, change
    /// its expiry, delete it.
    #[test]
    fn a_public_only_subkey_can_be_revoked_reexpired_and_deleted() {
        let (mut sk, _) =
            SignedSecretKey::from_string(&keys::V4_SIGN.secret_armored).expect("parse");
        let sub = sk.secret_subkeys.pop().expect("a subkey");
        let fpr = sub.key.fingerprint().to_string().to_uppercase();
        sk.public_subkeys.push(sub.into());
        let block = export_secret_key(sk).expect("export").secret;

        let revoked =
            revoke_subkey_internal(0, &block, &fpr, GfrRevocationCode::Retired, None, None)
                .expect("revoke a public-only subkey");
        let (revoked, _) = SignedSecretKey::from_string(&revoked.secret).expect("parse");
        assert!(
            revoked.public_subkeys[0]
                .signatures
                .iter()
                .any(|s| matches!(s.typ(), Some(SignatureType::SubkeyRevocation)))
        );

        let expiry = pgp::types::Timestamp::now().as_secs() as u64 + 400 * 86_400;
        let extended = update_key_expiration_internal(0, &block, Some(&fpr), expiry, None)
            .expect("re-expire a public-only subkey");
        let metas = extract_metadata_many_internal(&extended.secret).expect("meta");
        let listed = metas[0]
            .subkeys
            .iter()
            .find(|s| s.fpr.to_uppercase() == fpr)
            .expect("listed");
        assert!(listed.expires_at > 0);

        let deleted = delete_subkey_internal(&block, &fpr).expect("delete");
        let (deleted, _) = SignedSecretKey::from_string(&deleted.secret).expect("parse");
        assert!(deleted.public_subkeys.is_empty());
    }

    #[test]
    fn metadata_of_a_secret_key_lists_its_public_only_subkeys() {
        // The key list must show a subkey the engine will encrypt to, and
        // mark that its secret part is not held here.
        let (mut sk, _) =
            SignedSecretKey::from_string(&keys::V4_SIGN.secret_armored).expect("parse");
        let sub = sk.secret_subkeys.pop().expect("a subkey");
        let fpr = sub.key.fingerprint().to_string();
        sk.public_subkeys.push(sub.into());
        let block = export_secret_key(sk).expect("export").secret;

        let metas = extract_metadata_many_internal(&block).expect("meta");
        let meta = &metas[0];
        assert!(meta.has_secret);
        let listed = meta
            .subkeys
            .iter()
            .find(|s| s.fpr == fpr)
            .expect("the public-only subkey is listed");
        assert!(!listed.has_secret);
        assert_eq!(meta.subkeys.len(), 2);
    }

    #[test]
    fn merging_secret_keys_keeps_public_only_subkeys_and_upgrades_them() {
        // secret + secret: a public-only subkey of either side survives, and a
        // subkey public-only on one side but secret on the other ends up secret.
        let (mut with_public_sub, _) =
            SignedSecretKey::from_string(&keys::V4_SIGN.secret_armored).expect("parse");
        let sub = with_public_sub.secret_subkeys.pop().expect("a subkey");
        let fpr = sub.key.fingerprint();
        with_public_sub.public_subkeys.push(sub.into());
        let with_public_sub = export_secret_key(with_public_sub).expect("export").secret;

        let (mut without_sub, _) =
            SignedSecretKey::from_string(&keys::V4_SIGN.secret_armored).expect("parse");
        without_sub
            .secret_subkeys
            .retain(|s| s.key.fingerprint() != fpr);
        let without_sub = export_secret_key(without_sub).expect("export").secret;

        let merged = merge_key_block_internal(&without_sub, &with_public_sub).expect("merge");
        let (sk, _) = SignedSecretKey::from_string(&merged.secret).expect("parse");
        assert!(sk.public_subkeys.iter().any(|s| s.key.fingerprint() == fpr));

        let merged = merge_key_block_internal(&with_public_sub, &keys::V4_SIGN.secret_armored)
            .expect("merge");
        let (sk, _) = SignedSecretKey::from_string(&merged.secret).expect("parse");
        assert!(sk.secret_subkeys.iter().any(|s| s.key.fingerprint() == fpr));
        assert!(
            sk.public_subkeys.iter().all(|s| s.key.fingerprint() != fpr),
            "the subkey must not be listed twice"
        );
    }

    // -- importing: sq's merge and canonicalize policy, stricter -------------

    /// A signature of `typ` by `signer` over `uid` of `target`, naming
    /// `issuer` as its issuer in the hashed area, or no issuer at all.
    fn uid_sig(
        signer: &keys::Fixture,
        target: &SignedPublicKey,
        uid: &pgp::packet::UserId,
        typ: SignatureType,
        issuer: Option<pgp::types::Fingerprint>,
        created: u32,
    ) -> Signature {
        let signing = &signer.secret.primary_key;
        let mut cfg = SignatureConfig::from_key(rand::thread_rng(), signing, typ).unwrap();
        cfg.hashed_subpackets = vec![
            Subpacket::regular(SubpacketData::SignatureCreationTime(
                pgp::types::Timestamp::from_secs(created),
            ))
            .unwrap(),
        ];
        if let Some(fpr) = issuer {
            cfg.hashed_subpackets
                .push(Subpacket::regular(SubpacketData::IssuerFingerprint(fpr)).unwrap());
        }
        cfg.unhashed_subpackets.clear();
        cfg.sign_certification_third_party(
            signing,
            &Password::empty(),
            &target.primary_key,
            Tag::UserId,
            uid,
        )
        .unwrap()
    }

    /// A User ID packet of exactly `bytes`, which need not be UTF-8.
    fn user_id(bytes: &[u8]) -> pgp::packet::UserId {
        use pgp::packet::{Packet, PacketParser};
        let mut packet = vec![0xC0 | 13, bytes.len() as u8];
        packet.extend(bytes);
        match PacketParser::new(Cursor::new(packet)).next() {
            Some(Ok(Packet::UserId(id))) => id,
            other => panic!("not a user id: {other:?}"),
        }
    }

    /// A revocation of `target`'s first subkey made by `signer`, but naming
    /// `target`'s primary as its issuer.
    fn forged_subkey_revocation(
        signer: &keys::Fixture,
        target: &SignedPublicKey,
        created: u32,
    ) -> Signature {
        let signing = &signer.secret.primary_key;
        let mut cfg =
            SignatureConfig::from_key(rand::thread_rng(), signing, SignatureType::SubkeyRevocation)
                .unwrap();
        cfg.hashed_subpackets = vec![
            Subpacket::regular(SubpacketData::SignatureCreationTime(
                pgp::types::Timestamp::from_secs(created),
            ))
            .unwrap(),
            Subpacket::regular(SubpacketData::IssuerFingerprint(
                target.primary_key.fingerprint(),
            ))
            .unwrap(),
        ];
        cfg.sign_subkey_binding(
            signing,
            signing.public_key(),
            &Password::empty(),
            &target.public_subkeys[0].key,
        )
        .unwrap()
    }

    fn canonical(cert: SignedPublicKey) -> SignedPublicKey {
        let block = export_public_key(cert).unwrap().public;
        let out = canonicalize_key_block_internal(&block).expect("canonicalize");
        SignedPublicKey::from_string(&out.public).unwrap().0
    }

    fn merged_public(base: SignedPublicKey, incoming: SignedPublicKey) -> SignedPublicKey {
        let out = merge_key_block_internal(
            &export_public_key(base).unwrap().public,
            &export_public_key(incoming).unwrap().public,
        )
        .expect("merge");
        SignedPublicKey::from_string(&out.public).unwrap().0
    }

    fn holds(sigs: &[Signature], sig: &Signature) -> bool {
        let want = normalized_sig_bytes(sig).unwrap();
        sigs.iter()
            .any(|s| normalized_sig_bytes(s).unwrap() == want)
    }

    fn strip_unhashed(sig: &mut Signature) {
        while sig.unhashed_subpacket_remove(0).is_ok() {}
    }

    #[test]
    fn an_imported_key_keeps_only_what_its_holder_bound() {
        let target = &keys::V4_SIGN;
        let signer = &keys::V4_PRIMARY_ONLY;
        let genuine = SignedPublicKey::from(target.secret.clone());
        let created = genuine.primary_key.created_at().as_secs();
        let me = genuine.primary_key.fingerprint();
        let uid = genuine.details.users[0].id.clone();

        let mut cert = genuine.clone();
        // Made by another key, but claiming to be the holder's own.
        let forged = uid_sig(
            signer,
            &genuine,
            &uid,
            SignatureType::CertPositive,
            Some(me.clone()),
            created + 5,
        );
        // sq counts a signature naming no issuer as a self-signature.
        let issuerless = uid_sig(
            signer,
            &genuine,
            &uid,
            SignatureType::CertPositive,
            None,
            created + 6,
        );
        cert.details.users[0]
            .signatures
            .extend([forged.clone(), issuerless.clone()]);
        let signer_fpr = signer.secret.primary_key.fingerprint();
        for i in 0..(MAX_MERGED_SIGNATURES as u32 + 44) {
            let sig = uid_sig(
                signer,
                &genuine,
                &uid,
                SignatureType::CertGeneric,
                Some(signer_fpr.clone()),
                created + 10 + i,
            );
            cert.details.users[0].signatures.push(sig);
        }
        let mallory = user_id(b"Mallory <mallory@example.org>");
        let mallory_sig = uid_sig(
            signer,
            &genuine,
            &mallory,
            SignatureType::CertPositive,
            Some(me),
            created + 7,
        );
        cert.details
            .users
            .push(SignedUser::new(mallory, vec![mallory_sig]));
        let squatted =
            SignedPublicKey::from(keys::V4_NISTP256.secret.clone()).public_subkeys[0].clone();
        let squatted_fpr = squatted.key.fingerprint();
        cert.public_subkeys.push(squatted);
        let forged_revocation = forged_subkey_revocation(signer, &genuine, created + 8);
        cert.public_subkeys[0]
            .signatures
            .push(forged_revocation.clone());

        let out = canonical(cert);

        assert_eq!(out.details.users.len(), 1, "only the certified User ID");
        assert_eq!(out.details.users[0].id, uid);
        let sigs = &out.details.users[0].signatures;
        assert!(!holds(sigs, &forged), "a forged self-signature was kept");
        // Naming no issuer, it may be someone else's certification: kept,
        // as sq keeps it, but not as the holder's own.
        assert!(holds(sigs, &issuerless));
        assert!(
            !verified_uid_self_sigs(&out.primary_key, &out.details.users[0])
                .iter()
                .any(|s| normalized_sig_bytes(s).unwrap()
                    == normalized_sig_bytes(&issuerless).unwrap())
        );
        let own = genuine.details.users[0].signatures.len();
        assert_eq!(
            sigs.len(),
            own + MAX_MERGED_SIGNATURES,
            "the flood is capped"
        );
        assert!(has_verified_uid_certification(
            &out.primary_key,
            &out.details.users[0]
        ));
        for own in &genuine.details.users[0].signatures {
            assert!(holds(sigs, own), "the holder's own signature was lost");
        }

        assert_eq!(out.public_subkeys.len(), genuine.public_subkeys.len());
        assert!(
            out.public_subkeys
                .iter()
                .all(|s| s.key.fingerprint() != squatted_fpr)
        );
        assert!(!holds(
            &out.public_subkeys[0].signatures,
            &forged_revocation
        ));
        assert!(!is_subkey_revoked(
            &out.primary_key,
            &out.public_subkeys[0].key,
            &out.public_subkeys[0].signatures
        ));
    }

    #[test]
    fn a_revocation_on_the_wrong_user_id_is_refiled_not_lost() {
        // sq re-files a self-signature onto the component it verifies on:
        // a revocation must not be dropped for where it was found.
        let target = &keys::V4_SIGN;
        let genuine = SignedPublicKey::from(target.secret.clone());
        let created = genuine.primary_key.created_at().as_secs();
        let me = genuine.primary_key.fingerprint();
        let first = genuine.details.users[0].id.clone();
        let second = user_id(b"Second <second@example.org>");

        let mut cert = genuine.clone();
        let certification = uid_sig(
            target,
            &genuine,
            &second,
            SignatureType::CertPositive,
            Some(me.clone()),
            created + 5,
        );
        let revocation = uid_sig(
            target,
            &genuine,
            &second,
            SignatureType::CertRevocation,
            Some(me),
            created + 6,
        );
        cert.details
            .users
            .push(SignedUser::new(second.clone(), vec![certification]));
        cert.details.users[0].signatures.push(revocation.clone());

        let out = canonical(cert);
        let find = |id: &pgp::packet::UserId| {
            out.details
                .users
                .iter()
                .find(|u| u.id == *id)
                .expect("kept")
        };
        assert!(!holds(&find(&first).signatures, &revocation));
        assert!(!is_user_id_revoked(&out.primary_key, find(&first)));
        assert!(is_user_id_revoked(&out.primary_key, find(&second)));
    }

    #[test]
    fn a_merge_takes_no_forged_subkey_binding_or_revocation() {
        let target = &keys::V4_SIGN;
        let genuine = SignedPublicKey::from(target.secret.clone());
        let created = genuine.primary_key.created_at().as_secs();

        let mut incoming = genuine.clone();
        let squatted =
            SignedPublicKey::from(keys::V4_NISTP256.secret.clone()).public_subkeys[0].clone();
        let squatted_fpr = squatted.key.fingerprint();
        incoming.public_subkeys.push(squatted);
        let forged_revocation =
            forged_subkey_revocation(&keys::V4_PRIMARY_ONLY, &genuine, created + 8);
        incoming.public_subkeys[0]
            .signatures
            .push(forged_revocation.clone());

        let out = merged_public(genuine.clone(), incoming);
        assert_eq!(out.public_subkeys.len(), genuine.public_subkeys.len());
        assert!(
            out.public_subkeys
                .iter()
                .all(|s| s.key.fingerprint() != squatted_fpr)
        );
        assert!(!holds(
            &out.public_subkeys[0].signatures,
            &forged_revocation
        ));
    }

    #[test]
    fn a_signature_that_differs_only_unhashed_is_stored_once() {
        // GnuPG and key servers rewrite the unhashed area. sq treats such
        // copies as one signature; storing both grew the key on every
        // re-import and reported it as changed.
        let target = &keys::V4_SIGN;
        let genuine = SignedPublicKey::from(target.secret.clone());
        let own = genuine.details.users[0].signatures.len();
        let me = genuine.primary_key.legacy_key_id();
        let stranger = keys::V4_PRIMARY_ONLY.secret.primary_key.legacy_key_id();
        assert!(
            genuine.details.users[0]
                .signatures
                .iter()
                .all(|s| !s.issuer_fingerprint().is_empty()),
            "the fixture names its issuer by fingerprint"
        );

        let mut stored = genuine.clone();
        stored.details.users[0]
            .signatures
            .iter_mut()
            .for_each(strip_unhashed);
        let mut incoming = genuine.clone();
        for sig in &mut incoming.details.users[0].signatures {
            strip_unhashed(sig);
            for id in [me.clone(), stranger.clone()] {
                sig.unhashed_subpacket_push(
                    Subpacket::regular(SubpacketData::IssuerKeyId(id)).unwrap(),
                )
                .unwrap();
            }
        }

        let out = merged_public(stored, incoming);
        let sigs = &out.details.users[0].signatures;
        assert_eq!(sigs.len(), own);
        for sig in sigs {
            let unhashed: Vec<_> = sig.config().unwrap().unhashed_subpackets.clone();
            let ids: Vec<_> = unhashed
                .iter()
                .filter_map(|p| match &p.data {
                    SubpacketData::IssuerKeyId(id) => Some(id.clone()),
                    _ => None,
                })
                .collect();
            assert_eq!(
                ids,
                vec![me.clone()],
                "only an agreeing issuer is taken over"
            );
        }

        // Re-importing what is stored changes nothing, byte for byte.
        let block = export_public_key(out.clone()).unwrap().public;
        let again = merge_key_block_internal(&block, &block).unwrap();
        assert_eq!(again.public, block);
    }

    #[test]
    fn a_merge_never_drops_what_is_stored() {
        let target = &keys::V4_SIGN;
        let genuine = SignedPublicKey::from(target.secret.clone());
        let created = genuine.primary_key.created_at().as_secs();
        let uid = genuine.details.users[0].id.clone();
        let forged = uid_sig(
            &keys::V4_PRIMARY_ONLY,
            &genuine,
            &uid,
            SignatureType::CertPositive,
            Some(genuine.primary_key.fingerprint()),
            created + 5,
        );
        let mut stored = genuine.clone();
        stored.details.users[0].signatures.push(forged.clone());

        let out = merged_public(stored, genuine);
        assert!(holds(&out.details.users[0].signatures, &forged));
    }

    #[test]
    fn user_ids_are_told_apart_by_their_bytes() {
        // Both read as "\u{FFFD}a" once decoded lossily.
        let target = &keys::V4_SIGN;
        let genuine = SignedPublicKey::from(target.secret.clone());
        let created = genuine.primary_key.created_at().as_secs();
        let me = genuine.primary_key.fingerprint();
        let with_uid = |bytes: &[u8]| {
            let id = user_id(bytes);
            let sig = uid_sig(
                target,
                &genuine,
                &id,
                SignatureType::CertPositive,
                Some(me.clone()),
                created + 5,
            );
            let mut cert = genuine.clone();
            cert.details.users.push(SignedUser::new(id, vec![sig]));
            cert
        };

        let out = merged_public(with_uid(b"\xFFa"), with_uid(b"\xFEa"));
        let ids: Vec<&[u8]> = out.details.users.iter().map(|u| u.id.id()).collect();
        assert_eq!(ids.len(), genuine.details.users.len() + 2, "{ids:?}");
        assert!(ids.contains(&&b"\xFFa"[..]));
        assert!(ids.contains(&&b"\xFEa"[..]));
    }

    #[test]
    fn copies_of_one_key_in_one_input_are_merged() {
        // sq imports every copy in turn; the last secret copy used to
        // replace the others, and the first public copy to win outright.
        let target = &keys::V4_SIGN;
        let signer = &keys::V4_PRIMARY_ONLY;
        let genuine = SignedPublicKey::from(target.secret.clone());
        let created = genuine.primary_key.created_at().as_secs();
        let signer_fpr = signer.secret.primary_key.fingerprint();
        let with_cert = |at: u32| {
            let sig = certification_by(signer, &genuine, signer_fpr.clone(), at);
            let mut cert = genuine.clone();
            cert.details.users[0].signatures.push(sig.clone());
            (export_public_key(cert).unwrap().public, sig)
        };
        let (copy_x, x) = with_cert(created + 1);
        let (copy_y, y) = with_cert(created + 2);

        for input in [
            format!("{}{}", target.secret_armored, copy_x),
            format!("{}{}", copy_x, target.secret_armored),
        ] {
            let metas = extract_metadata_many_internal(&input).expect("meta");
            assert_eq!(metas.len(), 1);
            assert!(metas[0].has_secret);
            let secret = metas[0].secret_key_block.as_ref().expect("secret kept");
            let (sk, _) = SignedSecretKey::from_string(secret).unwrap();
            assert!(holds(&sk.details.users[0].signatures, &x));
        }

        let metas = extract_metadata_many_internal(&format!("{copy_x}{copy_y}")).expect("meta");
        assert_eq!(metas.len(), 1);
        let (pk, _) = SignedPublicKey::from_string(&metas[0].public_key_block).unwrap();
        assert!(holds(&pk.details.users[0].signatures, &x));
        assert!(holds(&pk.details.users[0].signatures, &y));
    }

    // -- importing: valid data canonicalization must keep ---------------------

    fn creation(at: u32) -> Subpacket {
        Subpacket::regular(SubpacketData::SignatureCreationTime(
            pgp::types::Timestamp::from_secs(at),
        ))
        .unwrap()
    }

    fn issued_by(fpr: pgp::types::Fingerprint) -> Subpacket {
        Subpacket::regular(SubpacketData::IssuerFingerprint(fpr)).unwrap()
    }

    /// A direct key signature (0x1F) by `target` naming `revoker` as its
    /// designated revoker (RFC 9580 §5.2.3.23).
    fn designating(target: &keys::Fixture, revoker: &keys::Fixture, created: u32) -> Signature {
        use pgp::types::{RevocationKey, RevocationKeyClass};
        let primary = &target.secret.primary_key;
        let revoker_key = &revoker.secret.primary_key;
        let mut cfg =
            SignatureConfig::from_key(rand::thread_rng(), primary, SignatureType::Key).unwrap();
        cfg.hashed_subpackets = vec![
            creation(created),
            issued_by(primary.fingerprint()),
            Subpacket::regular(SubpacketData::RevocationKey(RevocationKey::new(
                RevocationKeyClass::Default,
                revoker_key.algorithm(),
                revoker_key.fingerprint().as_bytes(),
            )))
            .unwrap(),
        ];
        cfg.unhashed_subpackets.clear();
        cfg.sign_key(primary, &Password::empty(), primary.public_key())
            .unwrap()
    }

    /// A revocation (0x28) of `target`'s first subkey made by `revoker`, who
    /// names itself as the issuer.
    fn subkey_revocation_by(
        revoker: &keys::Fixture,
        target: &SignedPublicKey,
        created: u32,
    ) -> Signature {
        let signing = &revoker.secret.primary_key;
        let mut cfg =
            SignatureConfig::from_key(rand::thread_rng(), signing, SignatureType::SubkeyRevocation)
                .unwrap();
        cfg.hashed_subpackets = vec![creation(created), issued_by(signing.fingerprint())];
        cfg.unhashed_subpackets.clear();
        cfg.sign_subkey_binding(
            signing,
            &target.primary_key,
            &Password::empty(),
            &target.public_subkeys[0].key,
        )
        .unwrap()
    }

    /// A key revocation (0x20) by `target` itself.
    fn self_key_revocation(target: &keys::Fixture, created: u32) -> Signature {
        let primary = &target.secret.primary_key;
        let mut cfg =
            SignatureConfig::from_key(rand::thread_rng(), primary, SignatureType::KeyRevocation)
                .unwrap();
        cfg.hashed_subpackets = vec![creation(created), issued_by(primary.fingerprint())];
        cfg.unhashed_subpackets.clear();
        cfg.sign_key(primary, &Password::empty(), primary.public_key())
            .unwrap()
    }

    #[test]
    fn a_designated_revokers_subkey_revocation_is_kept() {
        // Someone else's signature on a subkey means nothing, except a
        // revocation by a revoker the key holder designated.
        let target = &keys::V4_SIGN;
        let revoker = &keys::V4_PRIMARY_ONLY;
        let stranger = &keys::V4_NISTP256;
        let genuine = SignedPublicKey::from(target.secret.clone());
        let created = genuine.primary_key.created_at().as_secs();
        let designation = designating(target, revoker, created + 3);
        let by_revoker = subkey_revocation_by(revoker, &genuine, created + 4);
        let by_stranger = subkey_revocation_by(stranger, &genuine, created + 5);

        let mut cert = genuine.clone();
        cert.details.direct_signatures.push(designation.clone());
        cert.public_subkeys[0]
            .signatures
            .extend([by_revoker.clone(), by_stranger.clone()]);
        let out = canonical(cert);
        assert!(holds(&out.public_subkeys[0].signatures, &by_revoker));
        assert!(!holds(&out.public_subkeys[0].signatures, &by_stranger));

        // The designation may be stored already, the revocation arrive later.
        let mut stored = genuine.clone();
        stored.details.direct_signatures.push(designation);
        let mut incoming = genuine.clone();
        incoming.public_subkeys[0]
            .signatures
            .extend([by_revoker.clone(), by_stranger.clone()]);
        let out = merged_public(stored, incoming);
        assert!(holds(&out.public_subkeys[0].signatures, &by_revoker));
        assert!(!holds(&out.public_subkeys[0].signatures, &by_stranger));
    }

    #[test]
    fn third_party_revocations_are_not_pushed_out_by_a_flood() {
        let target = &keys::V4_SIGN;
        let signer = &keys::V4_PRIMARY_ONLY;
        let genuine = SignedPublicKey::from(target.secret.clone());
        let created = genuine.primary_key.created_at().as_secs();
        let uid = genuine.details.users[0].id.clone();
        let signer_fpr = signer.secret.primary_key.fingerprint();
        let flood: Vec<Signature> = (0..(MAX_MERGED_SIGNATURES as u32 + 20))
            .map(|i| {
                uid_sig(
                    signer,
                    &genuine,
                    &uid,
                    SignatureType::CertGeneric,
                    Some(signer_fpr.clone()),
                    created + 10 + i,
                )
            })
            .collect();
        // The signer withdraws its certification, after all the junk.
        let withdrawal = uid_sig(
            signer,
            &genuine,
            &uid,
            SignatureType::CertRevocation,
            Some(signer_fpr.clone()),
            created + 5_000,
        );

        let mut cert = genuine.clone();
        cert.details.users[0].signatures.extend(flood.clone());
        cert.details.users[0].signatures.push(withdrawal.clone());
        let out = canonical(cert);
        assert!(holds(&out.details.users[0].signatures, &withdrawal));

        // A stored key already full of certifications still takes it.
        let mut stored = genuine.clone();
        stored.details.users[0].signatures.extend(flood);
        let mut incoming = genuine.clone();
        incoming.details.users[0]
            .signatures
            .push(withdrawal.clone());
        let out = merged_public(stored, incoming);
        assert!(holds(&out.details.users[0].signatures, &withdrawal));
    }

    #[test]
    fn a_merge_takes_over_a_verifying_embedded_back_signature() {
        // Only a back-signature that verifies is taken over. GnuPG writes the primary key binding into the hashed area; other
        // tools and key servers carry it in the unhashed one. A stored binding
        // without it leaves a signing subkey unusable.
        let target = &keys::V4_SIGN;
        let primary = &target.secret.primary_key;
        let genuine = SignedPublicKey::from(target.secret.clone());
        let created = genuine.primary_key.created_at().as_secs();
        let signing_sub = target
            .secret
            .secret_subkeys
            .iter()
            .find(|s| engine_can_verify_with(s.key.public_key().public_params()))
            .expect("a subkey that can sign");
        let sub_pub = signing_sub.key.public_key().clone();
        let at = genuine
            .public_subkeys
            .iter()
            .position(|s| s.key.fingerprint() == sub_pub.fingerprint())
            .unwrap();

        let mut cfg =
            SignatureConfig::from_key(rand::thread_rng(), primary, SignatureType::SubkeyBinding)
                .unwrap();
        cfg.hashed_subpackets = vec![creation(created + 7), issued_by(primary.fingerprint())];
        cfg.unhashed_subpackets.clear();
        let bare_binding = cfg
            .sign_subkey_binding(primary, primary.public_key(), &Password::empty(), &sub_pub)
            .unwrap();
        let back = |signer: &pgp::packet::SecretSubkey, signer_pub: &PublicSubkey| {
            let mut cfg =
                SignatureConfig::from_key(rand::thread_rng(), signer, SignatureType::KeyBinding)
                    .unwrap();
            cfg.hashed_subpackets =
                vec![creation(created + 7), issued_by(signer_pub.fingerprint())];
            cfg.unhashed_subpackets.clear();
            cfg.sign_primary_key_binding(
                signer,
                signer_pub,
                &Password::empty(),
                primary.public_key(),
            )
            .unwrap()
        };
        let with_back = |backsig: Signature| {
            let mut sig = bare_binding.clone();
            sig.unhashed_subpacket_push(
                Subpacket::regular(SubpacketData::EmbeddedSignature(Box::new(backsig))).unwrap(),
            )
            .unwrap();
            let mut cert = genuine.clone();
            cert.public_subkeys[at].signatures = vec![sig];
            cert
        };
        let mut stored = genuine.clone();
        stored.public_subkeys[at].signatures = vec![bare_binding.clone()];

        // One made by another key does not verify, and is not taken.
        let stranger = &keys::V4_PRIMARY_ONLY.secret.primary_key;
        let mut cfg =
            SignatureConfig::from_key(rand::thread_rng(), stranger, SignatureType::KeyBinding)
                .unwrap();
        cfg.hashed_subpackets = vec![creation(created + 7), issued_by(sub_pub.fingerprint())];
        cfg.unhashed_subpackets.clear();
        let forged = cfg
            .sign_primary_key_binding(
                stranger,
                stranger.public_key(),
                &Password::empty(),
                primary.public_key(),
            )
            .unwrap();
        let out = merged_public(stored.clone(), with_back(forged));
        let binding = &out.public_subkeys[at].signatures[0];
        assert!(
            binding.embedded_signature().is_none(),
            "a forged back-signature was taken"
        );

        // One that cannot be checked here (a hash rPGP does not implement) is
        // not taken either: only a back-signature that verifies is.
        let mut unknown = uncheckable(SignatureType::KeyBinding, sub_pub.fingerprint(), 7);
        strip_unhashed(&mut unknown);
        let out = merged_public(stored.clone(), with_back(unknown));
        let binding = &out.public_subkeys[at].signatures[0];
        assert!(
            binding.embedded_signature().is_none(),
            "an uncheckable back-signature was taken"
        );

        let genuine_back = back(&signing_sub.key, &sub_pub);
        let out = merged_public(stored, with_back(genuine_back));
        let bindings = &out.public_subkeys[at].signatures;
        assert_eq!(bindings.len(), 1, "still one binding");
        let embedded = bindings[0].embedded_signature().expect("taken over");
        assert!(
            embedded
                .verify_primary_key_binding(&sub_pub, &genuine.primary_key)
                .is_ok()
        );
    }

    #[test]
    fn a_self_signature_that_cannot_be_checked_is_kept() {
        // A hash rPGP does not implement: not known to be bad, so not
        // dropped. It is stored, and no check counts it as verified.
        use pgp::crypto::{hash::HashAlgorithm, public_key::PublicKeyAlgorithm};
        use pgp::types::{Mpi, SignatureBytes};
        let target = &keys::V4_SIGN;
        let genuine = SignedPublicKey::from(target.secret.clone());
        let created = genuine.primary_key.created_at().as_secs();
        let mut cfg = SignatureConfig::v4(
            SignatureType::CertPositive,
            PublicKeyAlgorithm::RSA,
            HashAlgorithm::Other(120),
        );
        cfg.hashed_subpackets = vec![
            creation(created + 9),
            issued_by(genuine.primary_key.fingerprint()),
        ];
        let unknown = Signature::from_config(
            cfg,
            [0, 0],
            SignatureBytes::Mpis(vec![Mpi::from_slice(&[1, 2, 3])]),
        )
        .unwrap();

        let mut cert = genuine.clone();
        cert.details.users[0].signatures.push(unknown.clone());
        let out = canonical(cert);
        assert!(holds(&out.details.users[0].signatures, &unknown));
        assert!(
            !verified_uid_self_sigs(&out.primary_key, &out.details.users[0])
                .iter()
                .any(|s| s.hash_alg() == Some(HashAlgorithm::Other(120)))
        );
    }

    #[test]
    fn an_expired_self_signature_is_kept() {
        // Expiry is a question of when, not of who made it: the signature
        // is valid history and stays.
        let target = &keys::V4_SIGN;
        let primary = &target.secret.primary_key;
        let genuine = SignedPublicKey::from(target.secret.clone());
        let created = genuine.primary_key.created_at().as_secs();
        let uid = genuine.details.users[0].id.clone();
        let mut cfg =
            SignatureConfig::from_key(rand::thread_rng(), primary, SignatureType::CertPositive)
                .unwrap();
        cfg.hashed_subpackets = vec![
            creation(created + 11),
            issued_by(primary.fingerprint()),
            Subpacket::regular(SubpacketData::SignatureExpirationTime(Duration::from_secs(
                1,
            )))
            .unwrap(),
        ];
        cfg.unhashed_subpackets.clear();
        let expired = cfg
            .sign_certification_third_party(
                primary,
                &Password::empty(),
                &genuine.primary_key,
                Tag::UserId,
                &uid,
            )
            .unwrap();

        let mut cert = genuine.clone();
        cert.details.users[0].signatures.push(expired.clone());
        let out = canonical(cert);
        assert!(holds(&out.details.users[0].signatures, &expired));
    }

    #[test]
    fn an_issuerless_certification_is_kept_as_someone_elses() {
        // sq keeps a signature without an issuer it cannot place. Failing as
        // the holder's own does not make it a forgery: it may be anyone's.
        let target = &keys::V4_SIGN;
        let genuine = SignedPublicKey::from(target.secret.clone());
        let created = genuine.primary_key.created_at().as_secs();
        let uid = genuine.details.users[0].id.clone();
        let issuerless = uid_sig(
            &keys::V4_PRIMARY_ONLY,
            &genuine,
            &uid,
            SignatureType::CertGeneric,
            None,
            created + 6,
        );
        let mut incoming = genuine.clone();
        incoming.details.users[0]
            .signatures
            .push(issuerless.clone());

        let out = canonical(incoming.clone());
        assert!(holds(&out.details.users[0].signatures, &issuerless));
        let out = merged_public(genuine, incoming);
        assert!(holds(&out.details.users[0].signatures, &issuerless));
        assert!(has_verified_uid_certification(
            &out.primary_key,
            &out.details.users[0]
        ));
    }

    #[test]
    fn signature_checks_are_bounded() {
        // Thousands of self-claimed signatures: only MAX_SIGNATURE_CHECKS
        // are verified (revocations first); those that fail and those past
        // the budget are not stored.
        let target = &keys::V4_SIGN;
        let genuine = SignedPublicKey::from(target.secret.clone());
        let created = genuine.primary_key.created_at().as_secs();
        let model = genuine.details.users[0].signatures[0].clone();
        let forge = |n: u32| {
            let mut cfg = model.config().unwrap().clone();
            cfg.hashed_subpackets = vec![
                creation(created + 100 + n),
                issued_by(genuine.primary_key.fingerprint()),
            ];
            cfg.unhashed_subpackets.clear();
            Signature::from_config(
                cfg,
                model.signed_hash_value().unwrap(),
                model.signature().unwrap().clone(),
            )
            .unwrap()
        };
        let total = MAX_SIGNATURE_CHECKS as u32 + 2 * MAX_MERGED_SIGNATURES as u32;
        let mut cert = genuine.clone();
        cert.details.users[0]
            .signatures
            .extend((0..total).map(forge));
        let revocation = self_key_revocation(target, created + 50_000);
        cert.details.revocation_signatures.push(revocation);

        let out = canonical(cert);
        let own = genuine.details.users[0].signatures.len();
        let sigs = &out.details.users[0].signatures;
        assert_eq!(sigs.len(), own, "none past the budget is stored");
        assert_eq!(
            verified_uid_self_sigs(&out.primary_key, &out.details.users[0]).len(),
            own
        );
        assert!(is_primary_key_revoked(
            &out.primary_key,
            &out.details.revocation_signatures
        ));
    }

    #[test]
    fn a_certificate_without_user_ids_imports_and_merges() {
        // keys.openpgp.org serves a certificate whose addresses are not
        // verified without its User IDs.
        let target = &keys::V4_SIGN;
        let primary = &target.secret.primary_key;
        let genuine = SignedPublicKey::from(target.secret.clone());
        let created = genuine.primary_key.created_at().as_secs();
        let mut bare = genuine.clone();
        bare.details.users.clear();
        bare.details.user_attributes.clear();

        let out = canonical(bare.clone());
        assert!(out.details.users.is_empty());
        assert_eq!(out.public_subkeys.len(), genuine.public_subkeys.len());

        let mut stored = genuine.clone();
        let new_sub = stored.public_subkeys.pop().unwrap();
        let mut cfg =
            SignatureConfig::from_key(rand::thread_rng(), primary, SignatureType::Key).unwrap();
        cfg.hashed_subpackets = vec![creation(created + 13), issued_by(primary.fingerprint())];
        cfg.unhashed_subpackets.clear();
        let direct = cfg
            .sign_key(primary, &Password::empty(), primary.public_key())
            .unwrap();
        let mut incoming = bare;
        incoming.details.direct_signatures.push(direct.clone());

        let out = merged_public(stored, incoming);
        let ids: Vec<&[u8]> = out.details.users.iter().map(|u| u.id.id()).collect();
        let want: Vec<&[u8]> = genuine.details.users.iter().map(|u| u.id.id()).collect();
        assert_eq!(ids, want, "every stored User ID is kept");
        assert!(
            out.public_subkeys
                .iter()
                .any(|s| s.key.fingerprint() == new_sub.key.fingerprint())
        );
        assert!(holds(&out.details.direct_signatures, &direct));
    }

    // -- importing: what an uncheckable self-signature may do -----------------

    /// A signature of `typ` naming `issuer`, made with a hash rPGP does not
    /// implement: it can never be checked here. `n` keeps them distinct.
    fn uncheckable(typ: SignatureType, issuer: pgp::types::Fingerprint, n: u32) -> Signature {
        use pgp::crypto::{hash::HashAlgorithm, public_key::PublicKeyAlgorithm};
        use pgp::types::{Mpi, SignatureBytes};
        let mut cfg = SignatureConfig::v4(typ, PublicKeyAlgorithm::RSA, HashAlgorithm::Other(120));
        cfg.hashed_subpackets = vec![creation(1_700_000_000 + n), issued_by(issuer)];
        Signature::from_config(
            cfg,
            [0, 0],
            SignatureBytes::Mpis(vec![Mpi::from_slice(&[1, 2, 3])]),
        )
        .unwrap()
    }

    fn is_uncheckable(sig: &Signature) -> bool {
        sig.hash_alg() == Some(pgp::crypto::hash::HashAlgorithm::Other(120))
    }

    #[test]
    fn a_user_id_bound_only_by_an_uncheckable_signature_is_not_stored() {
        let genuine = SignedPublicKey::from(keys::V4_SIGN.secret.clone());
        let me = genuine.primary_key.fingerprint();
        let forged = user_id(b"Mallory <mallory@example.org>");
        let mut incoming = genuine.clone();
        incoming.details.users.push(SignedUser::new(
            forged.clone(),
            vec![uncheckable(SignatureType::CertPositive, me, 1)],
        ));

        let as_new = canonical(incoming.clone());
        assert!(
            as_new.details.users.iter().all(|u| u.id != forged),
            "stored as a new key"
        );
        let merged = merged_public(genuine, incoming);
        assert!(
            merged.details.users.iter().all(|u| u.id != forged),
            "stored by a merge"
        );
    }

    #[test]
    fn a_subkey_bound_only_by_an_uncheckable_signature_is_not_stored() {
        let genuine = SignedPublicKey::from(keys::V4_SIGN.secret.clone());
        let me = genuine.primary_key.fingerprint();
        let mut foreign =
            SignedPublicKey::from(keys::V4_NISTP256.secret.clone()).public_subkeys[0].clone();
        foreign.signatures = vec![uncheckable(SignatureType::SubkeyBinding, me, 2)];
        let foreign_fpr = foreign.key.fingerprint();
        let mut incoming = genuine.clone();
        incoming.public_subkeys.push(foreign);

        let as_new = canonical(incoming.clone());
        assert!(
            as_new
                .public_subkeys
                .iter()
                .all(|s| s.key.fingerprint() != foreign_fpr),
            "stored as a new key"
        );
        let merged = merged_public(genuine, incoming);
        assert!(
            merged
                .public_subkeys
                .iter()
                .all(|s| s.key.fingerprint() != foreign_fpr),
            "stored by a merge"
        );
    }

    #[test]
    fn an_uncheckable_signature_on_a_stored_user_id_is_kept() {
        // The User ID is the holder's already: the incoming copy needs no
        // certification of its own for its signatures to be merged in.
        let genuine = SignedPublicKey::from(keys::V4_SIGN.secret.clone());
        let me = genuine.primary_key.fingerprint();
        let extra = uncheckable(SignatureType::CertPositive, me, 3);
        let mut incoming = genuine.clone();
        incoming.details.users[0].signatures = vec![extra.clone()];

        let merged = merged_public(genuine, incoming);
        assert!(holds(&merged.details.users[0].signatures, &extra));
    }

    #[test]
    fn at_most_16_uncheckable_self_signatures_per_component() {
        let genuine = SignedPublicKey::from(keys::V4_SIGN.secret.clone());
        let me = genuine.primary_key.fingerprint();
        let sigs = |from: u32, n: u32| -> Vec<Signature> {
            (from..from + n)
                .map(|i| uncheckable(SignatureType::CertPositive, me.clone(), 100 + i))
                .collect()
        };

        let mut seventeen = genuine.clone();
        seventeen.details.users[0].signatures.extend(sigs(0, 17));
        let as_new = canonical(seventeen);
        let kept = as_new.details.users[0]
            .signatures
            .iter()
            .filter(|s| is_uncheckable(s));
        assert_eq!(kept.count(), 16, "as a new key");

        // Stored ones count first: a full component takes no more.
        let mut stored = genuine.clone();
        stored.details.users[0].signatures.extend(sigs(0, 16));
        let mut incoming = genuine.clone();
        let late = sigs(16, 1);
        incoming.details.users[0].signatures.extend(late.clone());
        let merged = merged_public(stored, incoming);
        let uids = &merged.details.users[0].signatures;
        assert_eq!(
            uids.iter().filter(|s| is_uncheckable(s)).count(),
            16,
            "on merge"
        );
        assert!(!holds(uids, &late[0]));
    }

    // -- importing: the check budget, signatures nothing takes ---------------

    /// `n` distinct copies of `sig`, differing only in the digest prefix, so
    /// none verifies and none is a duplicate of another.
    fn junk_copies(sig: &Signature, n: u32) -> Vec<Signature> {
        (0..n)
            .map(|i| {
                Signature::from_config(
                    sig.config().unwrap().clone(),
                    [(i >> 8) as u8, i as u8],
                    sig.signature().unwrap().clone(),
                )
                .unwrap()
            })
            .collect()
    }

    /// A key whose incoming copy carries, on its first subkey, as many
    /// forged self-claimed revocations as the check budget, and, past what
    /// the budget reaches, a forged revocation of its second subkey (where
    /// no cap can be what leaves it out) and an issuer-less certification.
    fn flooded_copy(genuine: &SignedPublicKey) -> (SignedPublicKey, Signature, Signature) {
        assert!(
            genuine.public_subkeys.len() >= 2,
            "the fixture has two subkeys"
        );
        let created = genuine.primary_key.created_at().as_secs();
        let junk_model = forged_subkey_revocation(&keys::V4_PRIMARY_ONLY, genuine, created + 8);
        let mut second = genuine.clone();
        second.public_subkeys.swap(0, 1);
        let late = forged_subkey_revocation(&keys::V4_PRIMARY_ONLY, &second, created + 9);
        let uid = genuine.details.users[0].id.clone();
        let issuerless = uid_sig(
            &keys::V4_PRIMARY_ONLY,
            genuine,
            &uid,
            SignatureType::CertGeneric,
            None,
            created + 6,
        );
        let mut flooded = genuine.clone();
        flooded.public_subkeys[0]
            .signatures
            .extend(junk_copies(&junk_model, MAX_SIGNATURE_CHECKS as u32));
        flooded.public_subkeys[1].signatures.push(late.clone());
        flooded.details.users[0].signatures.push(issuerless.clone());
        (flooded, late, issuerless)
    }

    #[test]
    fn self_claimed_signatures_past_the_check_budget_are_not_stored() {
        let genuine = SignedPublicKey::from(keys::V4_SIGN.secret.clone());
        let (flooded, late, issuerless) = flooded_copy(&genuine);

        let out = merged_public(genuine.clone(), flooded.clone());
        let sub = &out.public_subkeys[1];
        assert!(
            !holds(&sub.signatures, &late),
            "an unchecked revocation was stored"
        );
        assert!(!is_subkey_revoked(
            &out.primary_key,
            &sub.key,
            &sub.signatures
        ));
        assert!(
            !holds(&out.details.users[0].signatures, &issuerless),
            "an unchecked issuer-less signature was stored"
        );
        let flood = &flooded.public_subkeys[0].signatures;
        let own = &genuine.public_subkeys[0].signatures;
        assert!(
            !out.public_subkeys[0]
                .signatures
                .iter()
                .any(|s| holds(flood, s) && !holds(own, s)),
            "flood signatures were stored"
        );
        // What was stored is all still there.
        assert_eq!(out, merged_public(genuine.clone(), genuine));
    }

    #[test]
    fn the_check_budget_being_reached_is_reported() {
        let genuine = SignedPublicKey::from(keys::V4_SIGN.secret.clone());
        let stored = export_public_key(genuine.clone()).unwrap().public;
        let (flooded, _, _) = flooded_copy(&genuine);
        let flooded = export_public_key(flooded).unwrap().public;

        let outcome = merge_key_block_report(&stored, &flooded).expect("merge");
        assert!(outcome.check_limit_reached);
        let outcome = canonicalize_key_block_report(&flooded).expect("canonicalize");
        assert!(outcome.check_limit_reached);

        let clean = merge_key_block_report(&stored, &stored).expect("merge");
        assert!(!clean.check_limit_reached);
    }

    #[test]
    fn a_clean_copy_merges_normally_after_a_flooded_one() {
        let genuine = SignedPublicKey::from(keys::V4_SIGN.secret.clone());
        let created = genuine.primary_key.created_at().as_secs();
        let (flooded, _, _) = flooded_copy(&genuine);
        let after_flood = merged_public(genuine.clone(), flooded);

        let signer = &keys::V4_PRIMARY_ONLY;
        let cert = certification_by(
            signer,
            &genuine,
            signer.secret.primary_key.fingerprint(),
            created + 20,
        );
        let mut clean = genuine.clone();
        clean.details.users[0].signatures.push(cert.clone());
        let out = merged_public(after_flood, clean);
        assert!(holds(&out.details.users[0].signatures, &cert));
        assert_eq!(out.public_subkeys.len(), genuine.public_subkeys.len());
    }

    #[test]
    fn an_incoming_key_level_signature_no_component_takes_is_dropped() {
        use pgp::types::SignatureBytes;
        let genuine = SignedPublicKey::from(keys::V4_SIGN.secret.clone());
        let model = genuine.details.users[0].signatures[0].clone();
        let mut cfg = model.config().unwrap().clone();
        cfg.typ = SignatureType::Standalone;
        let orphan = Signature::from_config(
            cfg,
            model.signed_hash_value().unwrap(),
            SignatureBytes::clone(model.signature().unwrap()),
        )
        .unwrap();
        let mut with_orphan = genuine.clone();
        with_orphan.details.direct_signatures.push(orphan.clone());

        let out = merged_public(genuine.clone(), with_orphan.clone());
        assert!(!holds(&out.details.direct_signatures, &orphan), "taken in");
        let as_new = canonical(with_orphan.clone());
        assert!(
            !holds(&as_new.details.direct_signatures, &orphan),
            "stored as new"
        );
        // A stored one stays.
        let out = merged_public(with_orphan, genuine);
        assert!(holds(&out.details.direct_signatures, &orphan));
    }

    // -- ECDH KDF parameters -------------------------------------------------

    #[test]
    fn ecdh_kdf_params_are_four_hex_octets() {
        // §11.5.1: the KDF parameter field is `03 01 <hash> <cipher>`.
        let key = &keys::V4_SIGN;
        let hex = get_ecdh_kdf_params_internal(&key.public_armored, key.enc_subkey_fpr())
            .expect("kdf params");
        assert_eq!(hex.len(), 8);
        assert!(hex.starts_with("0301"));
    }

    #[test]
    fn ecdh_kdf_params_for_p256_use_sha256_and_aes128() {
        // §11.5.1 Table 30 fixes the pairing per curve.
        let key = &keys::V4_NISTP256;
        let hex = get_ecdh_kdf_params_internal(&key.public_armored, key.enc_subkey_fpr())
            .expect("kdf params");
        assert_eq!(hex, "03010807", "SHA2-256 (8) with AES-128 (7)");
    }

    #[test]
    fn ecdh_kdf_params_reject_an_unknown_fingerprint() {
        assert_eq!(
            get_ecdh_kdf_params_internal(&keys::V4_SIGN.public_armored, "DEADBEEF").err(),
            Some(GfrStatus::ErrorInvalidInput)
        );
    }

    #[test]
    fn ecdh_kdf_params_reject_a_signing_subkey() {
        assert_eq!(
            get_ecdh_kdf_params_internal(
                &keys::V4_SIGN.public_armored,
                keys::V4_SIGN.sign_subkey_fpr()
            )
            .err(),
            Some(GfrStatus::ErrorInvalidData)
        );
    }
}
