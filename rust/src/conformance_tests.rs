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

//! Requirement-organized conformance pins for the rPGP engine.
//!
//! Every test here pins behaviour the engine already implements but that no
//! other test asserted. The suite is organized by RFC area, one `mod` per
//! area; each test is named after the requirement it pins and opens with the
//! RFC section and keyword phrase it enforces. Assertions are on the exact
//! status or packet field, never on a bare success or failure.
//!
//! A pin that turns out not to hold is kept, renamed `candidate_...` and
//! marked `#[ignore]` with the reason: that is a finding, not a test to fix.

#![allow(dead_code)]

use std::io::Cursor;

use pgp::composed::{SignedPublicKey, SignedSecretKey};
use pgp::packet::{Packet, PacketParser};

use crate::crypto::DecryptResultInternal;
use crate::testutil::{cb, keys};
use crate::types::GfrStatus;

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

/// One packet of a binary stream, located by hand so a test can patch the
/// exact octet it is about.
struct RawPacket {
    tag: u8,
    /// Offset of the first header octet.
    start: usize,
    /// Offset of the first body octet (of the first chunk, for a partial body).
    body_start: usize,
    /// Offset one past the last octet of the packet.
    end: usize,
    /// The body with any partial-length continuation octets removed.
    body: Vec<u8>,
}

/// Walk the packets of a binary OpenPGP stream (both header formats,
/// partial bodies included).
fn walk(data: &[u8]) -> Vec<RawPacket> {
    let mut out = Vec::new();
    let mut i = 0usize;
    while i < data.len() {
        let start = i;
        let ctb = data[i];
        assert!(ctb & 0x80 != 0, "not a packet header at offset {i}");
        i += 1;
        if ctb & 0x40 != 0 {
            // RFC 9580 §4.2.1 OpenPGP format.
            let tag = ctb & 0x3F;
            let mut body = Vec::new();
            let mut body_start = None;
            loop {
                let o1 = data[i] as usize;
                let (len, partial) = match o1 {
                    0..=191 => {
                        i += 1;
                        (o1, false)
                    }
                    192..=223 => {
                        let l = ((o1 - 192) << 8) + data[i + 1] as usize + 192;
                        i += 2;
                        (l, false)
                    }
                    255 => {
                        let l = u32::from_be_bytes([
                            data[i + 1],
                            data[i + 2],
                            data[i + 3],
                            data[i + 4],
                        ]) as usize;
                        i += 5;
                        (l, false)
                    }
                    _ => {
                        i += 1;
                        (1usize << (o1 & 0x1F), true)
                    }
                };
                body_start.get_or_insert(i);
                body.extend_from_slice(&data[i..i + len]);
                i += len;
                if !partial {
                    break;
                }
            }
            out.push(RawPacket {
                tag,
                start,
                body_start: body_start.unwrap(),
                end: i,
                body,
            });
        } else {
            // §4.2.2 Legacy format (fixed lengths only).
            let tag = (ctb >> 2) & 0x0F;
            let octets = [1usize, 2, 4, 0][(ctb & 0x03) as usize];
            assert!(octets != 0, "indeterminate legacy length");
            let len = data[i..i + octets]
                .iter()
                .fold(0usize, |acc, b| (acc << 8) | *b as usize);
            i += octets;
            out.push(RawPacket {
                tag,
                start,
                body_start: i,
                end: i + len,
                body: data[i..i + len].to_vec(),
            });
            i += len;
        }
    }
    out
}

/// The first packet with `tag`.
fn first(data: &[u8], tag: u8) -> RawPacket {
    walk(data)
        .into_iter()
        .find(|p| p.tag == tag)
        .unwrap_or_else(|| panic!("no packet with tag {tag}"))
}

/// Every packet rPGP itself parses out of a binary stream.
fn rpgp_packets(data: &[u8]) -> Vec<Packet> {
    PacketParser::new(Cursor::new(data))
        .map(|p| p.expect("rPGP parses the packet"))
        .collect()
}

/// Raw octets that rPGP's armor writer can wrap.
struct Raw(Vec<u8>);

impl pgp::ser::Serialize for Raw {
    fn to_writer<W: std::io::Write>(&self, w: &mut W) -> pgp::errors::Result<()> {
        w.write_all(&self.0)?;
        Ok(())
    }
    fn write_len(&self) -> usize {
        self.0.len()
    }
}

/// Armor raw packet octets as a PUBLIC KEY BLOCK.
fn armor_public(bytes: Vec<u8>) -> String {
    let mut out = Vec::new();
    pgp::armor::write(
        &Raw(bytes),
        pgp::armor::BlockType::PublicKey,
        &mut out,
        None,
        true,
    )
    .expect("armor");
    String::from_utf8(out).unwrap()
}

/// Binary (unarmored) octets of a public certificate.
fn cert_bytes(armored: &str) -> Vec<u8> {
    crate::crypto::dearmor_all(armored.as_bytes()).into_owned()
}

/// Encrypt `payload` to the certificates in `recipients`, binary output.
fn encrypt_bin(recipients: &[&str], payload: &[u8]) -> Vec<u8> {
    crate::crypto::encrypt_internal(0, "", payload, recipients, false)
        .expect("encrypt")
        .data
}

/// Decrypt with `secret` supplied through the secret-key callback.
fn decrypt_with(secret: &str, data: &[u8]) -> Result<DecryptResultInternal, GfrStatus> {
    cb::set_seckey_answer(secret);
    crate::crypto::decrypt_internal(0, data, Some(cb::seckey_fetch), None, std::ptr::null_mut())
}

/// Replace the CRC24 footer line of armored text: `None` drops it.
fn with_crc_line(armored: &str, replacement: Option<&str>) -> String {
    let mut found = false;
    let mut out = Vec::new();
    for line in armored.split_inclusive('\n') {
        let bare = line.trim_end_matches(['\r', '\n']);
        if bare.starts_with('=') && bare.len() == 5 {
            found = true;
            if let Some(r) = replacement {
                out.push(format!("{r}\n"));
            }
            continue;
        }
        out.push(line.to_string());
    }
    assert!(
        found,
        "precondition: the armor carries a CRC24 footer:\n{armored}"
    );
    out.concat()
}

/// A CRC24 footer that is guaranteed to differ from the one in `armored`.
fn wrong_crc(armored: &str) -> &'static str {
    if armored.contains("\n=AAAA") {
        "=BBBB"
    } else {
        "=AAAA"
    }
}

/// The subkey of `sk` at `idx` with every self-signature replaced by one fresh
/// binding (0x18) dated at the subkey's creation, carrying `flags` and, when
/// given, a Key Expiration Time `expiry_secs` after creation.
fn rebind_subkey(
    sk: &mut SignedSecretKey,
    idx: usize,
    flags: pgp::packet::KeyFlags,
    expiry_secs: Option<u32>,
) {
    use pgp::packet::{SignatureConfig, SignatureType, Subpacket, SubpacketData};
    use pgp::types::{Duration, KeyDetails, Password};
    let primary = sk.primary_key.clone();
    let sub = &mut sk.secret_subkeys[idx];
    let created = sub.key.created_at();
    let mut cfg =
        SignatureConfig::from_key(rand::thread_rng(), &primary, SignatureType::SubkeyBinding)
            .unwrap();
    cfg.hashed_subpackets = vec![
        Subpacket::regular(SubpacketData::SignatureCreationTime(created)).unwrap(),
        Subpacket::regular(SubpacketData::IssuerFingerprint(primary.fingerprint())).unwrap(),
        Subpacket::regular(SubpacketData::KeyFlags(flags)).unwrap(),
    ];
    if let Some(secs) = expiry_secs {
        cfg.hashed_subpackets.push(
            Subpacket::regular(SubpacketData::KeyExpirationTime(Duration::from_secs(secs)))
                .unwrap(),
        );
    }
    let sig = cfg
        .sign_subkey_binding(
            &primary,
            primary.public_key(),
            &Password::empty(),
            sub.key.public_key(),
        )
        .unwrap();
    sub.signatures = vec![sig];
}

fn enc_flags() -> pgp::packet::KeyFlags {
    let mut f = pgp::packet::KeyFlags::default();
    f.set_encrypt_comms(true);
    f.set_encrypt_storage(true);
    f
}

fn public_armored(sk: &SignedSecretKey) -> String {
    SignedPublicKey::from(sk.clone())
        .to_armored_string(crate::utils::armor_opts())
        .expect("armor")
}

fn legacy_key_id(sk: &SignedSecretKey, idx: usize) -> String {
    use pgp::types::KeyDetails;
    sk.secret_subkeys[idx].key.legacy_key_id().to_string()
}

fn now() -> u32 {
    pgp::types::Timestamp::now().as_secs()
}

/// Sign `data` detached with the GF engine (binary or armored).
fn gf_detached(signer: &keys::Fixture, data: &[u8], armor: bool) -> Vec<u8> {
    crate::crypto::sign_internal(
        0,
        "",
        data,
        &[&signer.secret_armored],
        None,
        crate::types::GfrSignMode::Detached,
        armor,
    )
    .expect("sign")
    .data
}

fn verify(
    data: &[u8],
    sig: &[u8],
    mode: crate::types::GfrSignMode,
    cert: &str,
) -> Result<crate::crypto::VerifyResultInternal, GfrStatus> {
    cb::set_pubkey_answer(cert);
    crate::crypto::verify_internal(
        data,
        sig,
        mode,
        Some(cb::pubkey_fetch),
        std::ptr::null_mut(),
    )
}

fn statuses(
    sigs: &[crate::crypto::SignatureResultInternal],
) -> Vec<crate::types::GfrSignatureStatus> {
    sigs.iter().map(|s| s.status).collect()
}

// ---------------------------------------------------------------------------
// RFC 9580 §5.13.2 Version 2 SEIPD
// ---------------------------------------------------------------------------

mod rfc9580_seipd_v2 {
    use super::*;
    use pgp::packet::Packet;

    fn seipd_v2_of(data: &[u8]) -> (u8, [u8; 32]) {
        let p = first(data, 18);
        assert_eq!(p.body[0], 2, "a v6 recipient gets a v2 SEIPD");
        let mut salt = [0u8; 32];
        salt.copy_from_slice(&p.body[4..36]);
        (p.body[3], salt)
    }

    #[test]
    fn generated_chunk_size_octet_is_6_and_at_most_16() {
        // RFC 9580 §5.13.2: "An implementation MUST NOT create data with a
        // chunk size octet value larger than 16 (4 MiB chunks)."
        let out = encrypt_bin(&[&keys::V6_SIGN.public_armored], b"chunk size");
        let (chunk, _) = seipd_v2_of(&out);
        assert_eq!(chunk, 6, "GF writes 4 KiB chunks");
        assert!(chunk <= 16);
        // rPGP agrees on the parsed field.
        let parsed = rpgp_packets(&out)
            .into_iter()
            .find_map(|p| match p {
                Packet::SymEncryptedProtectedData(s) => Some(s),
                _ => None,
            })
            .expect("a SEIPD");
        match parsed.config() {
            pgp::packet::SymEncryptedProtectedDataConfig::V2 { chunk_size, .. } => {
                assert_eq!(*chunk_size, pgp::crypto::aead::ChunkSize::C4KiB)
            }
            other => panic!("expected a v2 SEIPD, got {other:?}"),
        }
    }

    #[test]
    fn seipd_v2_salt_is_fresh_per_message() {
        // RFC 9580 §5.13.2: "32 octets of salt. The salt is used to derive the
        // message key and MUST be securely generated".
        let a = encrypt_bin(&[&keys::V6_SIGN.public_armored], b"same plaintext");
        let b = encrypt_bin(&[&keys::V6_SIGN.public_armored], b"same plaintext");
        let (_, salt_a) = seipd_v2_of(&a);
        let (_, salt_b) = seipd_v2_of(&b);
        assert_ne!(salt_a, salt_b, "two messages must not share a SEIPD salt");
        assert_ne!(salt_a, [0u8; 32]);
    }

    fn password_message() -> Vec<u8> {
        let mut out = Vec::new();
        crate::crypto::encrypt_stream_with_password_internal(
            0,
            "",
            &b"same plaintext"[..],
            &mut out,
            Some(cb::pwd_correct),
            false,
        )
        .expect("encrypt");
        out
    }

    fn argon2_salt_of(data: &[u8]) -> [u8; 16] {
        let skesk = rpgp_packets(data)
            .into_iter()
            .find_map(|p| match p {
                Packet::SymKeyEncryptedSessionKey(s) => Some(s),
                _ => None,
            })
            .expect("a SKESK");
        assert_eq!(skesk.version(), pgp::types::SkeskVersion::V6);
        match skesk.s2k().expect("an S2K") {
            pgp::types::StringToKey::Argon2 { salt, .. } => *salt,
            other => panic!("expected an Argon2 S2K, got {other:?}"),
        }
    }

    #[test]
    fn password_messages_have_fresh_seipd_and_argon2_salts() {
        // RFC 9580 §5.13.2: the v2 SEIPD salt "MUST be securely generated";
        // §3.7.1.4: the Argon2 S2K carries "a 16-octet salt value".
        let lock = crate::crypto::ARGON2_TEST_LOCK
            .lock()
            .unwrap_or_else(|e| e.into_inner());
        let before = crate::crypto::argon2_s2k_params();
        // Cheap parameters: the salts are under test, not the cost.
        assert!(crate::crypto::set_argon2_s2k_params(1, 1, 8));
        let pair = std::panic::catch_unwind(|| (password_message(), password_message()));
        assert!(crate::crypto::set_argon2_s2k_params(
            before.0, before.1, before.2
        ));
        drop(lock);
        let (a, b) = pair.expect("encrypt");

        assert_ne!(
            argon2_salt_of(&a),
            argon2_salt_of(&b),
            "Argon2 salts must differ"
        );
        assert_ne!(
            seipd_v2_of(&a).1,
            seipd_v2_of(&b).1,
            "SEIPD salts must differ"
        );
    }

    #[test]
    fn chunk_size_octet_above_16_is_refused() {
        // RFC 9580 §5.13.2: "An implementation MUST accept chunk size octets
        // with values from 0 to 16"; 17 and above are not a valid encoding.
        let out = encrypt_bin(&[&keys::V6_SIGN.public_armored], b"chunk size");
        let seipd = first(&out, 18);
        let mut bad = out.clone();
        bad[seipd.body_start + 3] = 17;
        let res = decrypt_with(&keys::V6_SIGN.secret_armored, &bad);
        assert_eq!(res.err(), Some(GfrStatus::ErrorInvalidInput));
        // Control: the untouched message decrypts.
        assert_eq!(
            decrypt_with(&keys::V6_SIGN.secret_armored, &out)
                .unwrap()
                .data,
            b"chunk size"
        );
    }
}

// ---------------------------------------------------------------------------
// RFC 9580 §5.14 Padding packet
// ---------------------------------------------------------------------------

mod rfc9580_padding {
    use super::*;
    use crate::testutil::{packets, seipd};

    fn with_padding_before_seipd(msg: &[u8]) -> Vec<u8> {
        let seipd = first(msg, 18);
        let mut out = msg[..seipd.start].to_vec();
        out.extend(packets::padding_packet(32));
        out.extend_from_slice(&msg[seipd.start..]);
        out
    }

    #[test]
    fn padding_between_esk_and_v1_seipd_is_ignored() {
        // RFC 9580 §5.14: "Such a packet MUST be ignored when received."
        let msg = encrypt_bin(&[&keys::V4_SIGN.public_armored], b"padded v1");
        let padded = with_padding_before_seipd(&msg);
        let res = decrypt_with(&keys::V4_SIGN.secret_armored, &padded).expect("decrypts");
        assert_eq!(res.data, b"padded v1");
    }

    #[test]
    fn padding_between_esk_and_v2_seipd_is_ignored() {
        // RFC 9580 §5.14: "Such a packet MUST be ignored when received."
        let msg = encrypt_bin(&[&keys::V6_SIGN.public_armored], b"padded v2");
        let padded = with_padding_before_seipd(&msg);
        let res = decrypt_with(&keys::V6_SIGN.secret_armored, &padded).expect("decrypts");
        assert_eq!(res.data, b"padded v2");
    }

    #[test]
    fn padding_after_the_literal_inside_the_container_is_ignored() {
        // RFC 9580 §5.14: "Such a packet MUST be ignored when received."
        let mut inner = packets::literal(b'b', b"", 0, b"padded inner");
        inner.extend(packets::padding_packet(64));
        let msg = seipd::v1_with_inner(&keys::V4_SIGN, &inner);
        let res = decrypt_with(&keys::V4_SIGN.secret_armored, &msg).expect("decrypts");
        assert_eq!(res.data, b"padded inner");
    }
}

// ---------------------------------------------------------------------------
// Encryption target selection (RFC 9580 §5.2.3.13, §5.2.3.29)
// ---------------------------------------------------------------------------

mod rfc9580_target_selection {
    use super::*;
    use crate::types::{GfrKeyAlgo, GfrOpenPGPKeyVersion};

    /// Ed25519 primary + two encryption subkeys; generated per test since the
    /// tests rewrite bindings.
    fn two_enc_subkeys() -> SignedSecretKey {
        keys::generate(
            "Two Enc <two-enc@example.test>",
            &keys::cfg(GfrKeyAlgo::ED25519, true, false, GfrOpenPGPKeyVersion::V4),
            &[
                keys::cfg(GfrKeyAlgo::ED25519, false, true, GfrOpenPGPKeyVersion::V4),
                keys::cfg(GfrKeyAlgo::ED25519, false, true, GfrOpenPGPKeyVersion::V4),
            ],
        )
    }

    /// Wait until a component created at `created` with a 1 s lifetime is
    /// unambiguously expired (§5.2.3.13 expiry is relative to creation).
    fn wait_past(created: u32) {
        while now() < created + 2 {
            std::thread::sleep(std::time::Duration::from_millis(100));
        }
    }

    #[test]
    fn an_expired_encryption_subkey_is_never_chosen() {
        // RFC 9580 §5.2.3.13: the Key Expiration Time is "the number of
        // seconds after the key creation time that the key expires"; an expired
        // subkey must not receive a session key.
        use pgp::types::KeyDetails;
        let mut sk = two_enc_subkeys();
        rebind_subkey(&mut sk, 0, enc_flags(), Some(1));
        wait_past(sk.secret_subkeys[0].key.created_at().as_secs());

        let res = crate::crypto::encrypt_internal(0, "", b"x", &[&public_armored(&sk)], false)
            .expect("the live subkey is still usable");
        assert_eq!(res.recipients.len(), 1);
        assert_eq!(res.recipients[0].key_id, legacy_key_id(&sk, 1));
        assert!(res.invalid_recipients.is_empty());

        // With the only live subkey also expired, no target remains.
        rebind_subkey(&mut sk, 1, enc_flags(), Some(1));
        wait_past(sk.secret_subkeys[1].key.created_at().as_secs());
        let res = crate::crypto::encrypt_internal(0, "", b"x", &[&public_armored(&sk)], false);
        assert_eq!(res.err(), Some(GfrStatus::ErrorUnusableKey));
    }

    #[test]
    fn a_subkey_bound_without_encryption_flags_is_never_chosen() {
        // RFC 9580 §5.2.3.29: Key Flags state what a key "may be used for";
        // a binding whose flags omit 0x04/0x08 does not allow encryption.
        let mut sk = two_enc_subkeys();
        let mut auth_only = pgp::packet::KeyFlags::default();
        auth_only.set_authentication(true);
        rebind_subkey(&mut sk, 0, auth_only.clone(), None);
        // Control: the rebinding itself is honoured for the other subkey.
        rebind_subkey(&mut sk, 1, enc_flags(), None);

        let res = crate::crypto::encrypt_internal(0, "", b"x", &[&public_armored(&sk)], false)
            .expect("the flagged subkey is usable");
        assert_eq!(res.recipients.len(), 1);
        assert_eq!(res.recipients[0].key_id, legacy_key_id(&sk, 1));

        rebind_subkey(&mut sk, 1, auth_only, None);
        let res = crate::crypto::encrypt_internal(0, "", b"x", &[&public_armored(&sk)], false);
        assert_eq!(res.err(), Some(GfrStatus::ErrorUnusableKey));
    }
}

// ---------------------------------------------------------------------------
// RFC 9580 §11.5.1 ECDH KDF parameters of v6 keys
// ---------------------------------------------------------------------------

mod rfc9580_ecdh_v6_kdf {
    use super::*;
    use pgp::crypto::hash::HashAlgorithm;
    use pgp::crypto::public_key::PublicKeyAlgorithm;
    use pgp::crypto::sym::SymmetricKeyAlgorithm;
    use pgp::packet::{PubKeyInner, PublicSubkey};
    use pgp::types::{EcdhPublicParams, KeyDetails, KeyVersion, PublicParams};

    /// `V6_SIGN` with its encryption subkey replaced by a v6 ECDH P-256 key
    /// announcing (`hash`, `kek`), validly bound by the primary.
    fn v6_p256_cert(hash: HashAlgorithm, kek: SymmetricKeyAlgorithm) -> String {
        p256_cert(&keys::V6_SIGN, KeyVersion::V6, hash, kek)
    }

    /// `owner` with its encryption subkey replaced by a `version` ECDH P-256
    /// key announcing (`hash`, `kek`), validly bound by the primary.
    fn p256_cert(
        owner: &keys::Fixture,
        version: KeyVersion,
        hash: HashAlgorithm,
        kek: SymmetricKeyAlgorithm,
    ) -> String {
        use pgp::packet::{SignatureConfig, SignatureType, Subpacket, SubpacketData};
        use pgp::types::Password;
        let donor = &keys::V4_NISTP256.secret.secret_subkeys[0].key;
        let p = match donor.public_params() {
            PublicParams::ECDH(EcdhPublicParams::P256 { p, .. }) => *p,
            other => panic!("donor is not ECDH P-256: {other:?}"),
        };
        let inner = PubKeyInner::new(
            version,
            PublicKeyAlgorithm::ECDH,
            pgp::types::Timestamp::now(),
            None,
            PublicParams::ECDH(EcdhPublicParams::P256 {
                p,
                hash,
                alg_sym: kek,
            }),
        )
        .expect("ECDH key");
        let subkey = PublicSubkey::from_inner(inner).expect("subkey");

        let sk = &owner.secret;
        let primary = &sk.primary_key;
        let mut cfg =
            SignatureConfig::from_key(rand::thread_rng(), primary, SignatureType::SubkeyBinding)
                .unwrap();
        cfg.hashed_subpackets = vec![
            Subpacket::regular(SubpacketData::SignatureCreationTime(
                pgp::types::Timestamp::now(),
            ))
            .unwrap(),
            Subpacket::regular(SubpacketData::IssuerFingerprint(primary.fingerprint())).unwrap(),
            Subpacket::regular(SubpacketData::KeyFlags(enc_flags())).unwrap(),
        ];
        let sig = cfg
            .sign_subkey_binding(primary, primary.public_key(), &Password::empty(), &subkey)
            .unwrap();
        let mut cert = SignedPublicKey::from(sk.clone());
        cert.public_subkeys = vec![pgp::composed::SignedPublicSubKey {
            key: subkey,
            signatures: vec![sig],
        }];
        cert.to_armored_string(crate::utils::armor_opts()).unwrap()
    }

    #[test]
    fn v6_ecdh_key_announcing_a_different_kdf_is_not_encrypted_to() {
        // RFC 9580 §11.5.1: "An implementation MUST NOT encrypt any message to
        // a version 6 ECDH key over a listed curve that announces a different
        // KDF or KEK parameter."
        let good = v6_p256_cert(HashAlgorithm::Sha256, SymmetricKeyAlgorithm::AES128);
        let ok = crate::crypto::encrypt_internal(0, "", b"x", &[&good], false)
            .expect("control: the RFC-mandated P-256 parameters are usable");
        assert_eq!(ok.recipients.len(), 1);

        let bad = v6_p256_cert(HashAlgorithm::Sha512, SymmetricKeyAlgorithm::AES256);
        let res = crate::crypto::encrypt_internal(0, "", b"x", &[&bad], false);
        assert_eq!(res.err(), Some(GfrStatus::ErrorUnsupportedFeature));
    }

    #[test]
    fn a_certificate_with_a_forbidden_primary_is_never_encrypted_to() {
        // RFC 9580 §12.5 "MUST NOT ... verify using DSA keys": the subkey's
        // binding can only be checked with the DSA primary, so the
        // certificate is as unusable for encryption as it is for signatures
        // (round four, path parity with the verdicts).
        let cert = p256_cert(
            &keys::V4_DSA2048,
            KeyVersion::V4,
            HashAlgorithm::Sha256,
            SymmetricKeyAlgorithm::AES128,
        );
        let res = crate::crypto::encrypt_internal(
            0,
            "",
            b"x",
            &[&keys::V4_SIGN.public_armored, &cert],
            false,
        )
        .expect("the usable recipient still gets the message");
        assert_eq!(res.recipients.len(), 1);
        assert_eq!(res.invalid_recipients.len(), 1);
    }

    #[test]
    fn a_recipient_the_engine_cannot_encrypt_to_is_skipped_not_fatal() {
        // Round four, sibling of R3-7: rPGP 0.21 has no ElGamal encryption
        // and no ECDH over the Brainpool curves. Such a recipient must be
        // reported as unusable, and every other recipient still served.
        for (what, cert) in [
            ("ElGamal", keys::ELGAMAL_SUBKEY_CERT),
            ("Brainpool ECDH", keys::BRAINPOOL_SUBKEY_CERT),
        ] {
            let res = crate::crypto::encrypt_internal(
                0,
                "",
                b"x",
                &[&keys::V4_SIGN.public_armored, cert],
                false,
            )
            .unwrap_or_else(|e| panic!("{what}: every recipient lost: {e:?}"));
            assert_eq!(res.recipients.len(), 1, "{what}");
            assert_eq!(res.invalid_recipients.len(), 1, "{what}");
        }
    }

    #[test]
    fn an_ecdh_key_with_a_weak_kdf_hash_is_never_encrypted_to() {
        // RFC 9580 §9.5: "MUST NOT use MD5, SHA-1, or RIPEMD-160 as a hash
        // function in an ECDH KDF"; §11.4: the KDF hash "SHA2-256 or
        // stronger is REQUIRED". A v4 key announces its own KDF hash, so an
        // old or hostile certificate can ask for SHA-1.
        let good = p256_cert(
            &keys::V4_NISTP256,
            KeyVersion::V4,
            HashAlgorithm::Sha256,
            SymmetricKeyAlgorithm::AES128,
        );
        let ok = crate::crypto::encrypt_internal(0, "", b"x", &[&good], false)
            .expect("control: a SHA2-256 KDF is usable");
        assert_eq!(ok.recipients.len(), 1);
        // rPGP refuses to build the session-key packet for such a key. GF
        // must treat it like any other unusable recipient: skip and report
        // it, and still encrypt to the others, rather than fail them all.
        for weak in [HashAlgorithm::Sha1, HashAlgorithm::Md5] {
            let bad = p256_cert(
                &keys::V4_NISTP256,
                KeyVersion::V4,
                weak,
                SymmetricKeyAlgorithm::AES128,
            );
            let res = crate::crypto::encrypt_internal(
                0,
                "",
                b"x",
                &[&keys::V4_SIGN.public_armored, &bad],
                false,
            )
            .unwrap_or_else(|e| panic!("{weak:?}: the good recipient was dropped too: {e:?}"));
            assert_eq!(res.recipients.len(), 1, "{weak:?}");
            assert_eq!(res.invalid_recipients.len(), 1, "{weak:?}");
        }
    }
}

// ---------------------------------------------------------------------------
// RFC 9580 §5.9 Literal Data packet
// ---------------------------------------------------------------------------

mod rfc9580_literal {
    use super::*;
    use pgp::composed::Message;
    use pgp::packet::DataMode;
    use pgp::types::{PacketHeaderVersion, Password};

    fn literal_of(
        fixture: &keys::Fixture,
        msg: &[u8],
    ) -> (DataMode, Vec<u8>, u32, PacketHeaderVersion) {
        let m = Message::from_bytes(msg).expect("parses");
        let m = m
            .decrypt(&Password::empty(), &fixture.secret)
            .expect("decrypts");
        let hdr = m.literal_data_header().expect("a literal").clone();
        (
            hdr.mode(),
            hdr.file_name().to_vec(),
            hdr.created().as_secs(),
            m.packet_header().version(),
        )
    }

    #[test]
    fn generated_literal_is_binary_with_empty_name_and_zero_date() {
        // RFC 9580 §5.9: "Otherwise, it MUST set the format octet to b. It
        // SHOULD set the filename to the empty string ... and the timestamp to
        // zero".
        for fixture in [&*keys::V4_SIGN, &*keys::V6_SIGN] {
            let msg = encrypt_bin(&[&fixture.public_armored], b"literal");
            let (mode, name, date, _) = literal_of(fixture, &msg);
            assert_eq!(mode, DataMode::Binary);
            assert_eq!(name, b"");
            assert_eq!(date, 0);
            let res = decrypt_with(&fixture.secret_armored, &msg).unwrap();
            assert_eq!(res.filename, "");
        }
    }

    #[test]
    fn generated_literal_uses_the_openpgp_packet_format() {
        // RFC 9580 §5.9: "It MUST NOT generate a Literal Data packet with
        // Legacy format (Section 4.2.2)."
        for fixture in [&*keys::V4_SIGN, &*keys::V6_SIGN] {
            let msg = encrypt_bin(&[&fixture.public_armored], b"literal");
            let (_, _, _, version) = literal_of(fixture, &msg);
            assert_eq!(version, PacketHeaderVersion::New);
        }
        // An unencrypted inline-signed message too.
        let signed = crate::crypto::sign_internal(
            0,
            "",
            b"literal",
            &[&keys::V4_SIGN.secret_armored],
            None,
            crate::types::GfrSignMode::Inline,
            false,
        )
        .unwrap()
        .data;
        let lit = walk(&signed)
            .into_iter()
            .find(|p| p.tag == 11)
            .expect("a literal");
        assert_eq!(signed[lit.start] & 0x40, 0x40, "OpenPGP-format header");
        assert_eq!(lit.body[0], b'b');
        assert_eq!(lit.body[1], 0, "empty filename");
        assert_eq!(&lit.body[2..6], &[0, 0, 0, 0], "zero date");
    }
}

// ---------------------------------------------------------------------------
// RFC 9580 §6.1 CRC24 footer on consume
// ---------------------------------------------------------------------------

mod rfc9580_armor_crc {
    use super::*;
    use crate::types::{GfrSignMode, GfrSignatureStatus};

    fn armored_message() -> String {
        let out = crate::crypto::encrypt_internal(
            0,
            "",
            b"crc payload",
            &[&keys::V4_SIGN.public_armored],
            true,
        )
        .unwrap()
        .data;
        String::from_utf8(out).unwrap()
    }

    #[test]
    fn message_with_missing_or_wrong_crc_decrypts() {
        // RFC 9580 §6.1: "An implementation MUST NOT reject an OpenPGP object
        // when the CRC24 footer is present, missing, malformed, or disagrees
        // with the computed CRC24 sum."
        let armored = armored_message();
        for variant in [
            with_crc_line(&armored, None),
            with_crc_line(&armored, Some(wrong_crc(&armored))),
        ] {
            let res =
                decrypt_with(&keys::V4_SIGN.secret_armored, variant.as_bytes()).expect("decrypts");
            assert_eq!(res.data, b"crc payload");
        }
    }

    #[test]
    fn detached_signature_with_missing_or_wrong_crc_verifies() {
        // RFC 9580 §6.1: "MUST NOT reject an OpenPGP object when the CRC24
        // footer is present, missing, malformed, or disagrees".
        let data = b"crc signed data";
        let sig = String::from_utf8(gf_detached(&keys::V4_SIGN, data, true)).unwrap();
        for variant in [
            with_crc_line(&sig, None),
            with_crc_line(&sig, Some(wrong_crc(&sig))),
        ] {
            let res = verify(
                data,
                variant.as_bytes(),
                GfrSignMode::Detached,
                &keys::V4_SIGN.public_armored,
            )
            .expect("verify");
            assert_eq!(statuses(&res.signatures), vec![GfrSignatureStatus::Valid]);
        }
    }

    #[test]
    fn cleartext_message_with_missing_or_wrong_crc_verifies() {
        // RFC 9580 §6.1: "MUST NOT reject an OpenPGP object when the CRC24
        // footer is present, missing, malformed, or disagrees".
        let signed = crate::crypto::sign_internal(
            0,
            "",
            b"crc cleartext\n",
            &[&keys::V4_SIGN.secret_armored],
            None,
            GfrSignMode::ClearText,
            true,
        )
        .unwrap()
        .data;
        let signed = String::from_utf8(signed).unwrap();
        for variant in [
            with_crc_line(&signed, None),
            with_crc_line(&signed, Some(wrong_crc(&signed))),
        ] {
            let res = verify(
                variant.as_bytes(),
                &[],
                GfrSignMode::ClearText,
                &keys::V4_SIGN.public_armored,
            )
            .expect("verify");
            assert_eq!(statuses(&res.signatures), vec![GfrSignatureStatus::Valid]);
        }
    }

    #[test]
    fn public_key_with_missing_or_wrong_crc_is_read() {
        // RFC 9580 §6.1: "MUST NOT reject an OpenPGP object when the CRC24
        // footer is present, missing, malformed, or disagrees".
        // The fixture armor omits the footer; re-armor with it (v4-only).
        let armored = &SignedPublicKey::from(keys::V4_SIGN.secret.clone())
            .to_armored_string(crate::utils::armor_opts_for(true))
            .unwrap();
        for variant in [
            with_crc_line(armored, None),
            with_crc_line(armored, Some(wrong_crc(armored))),
        ] {
            let meta = crate::key::extract_metadata_many_internal(&variant).expect("metadata");
            assert_eq!(meta.len(), 1);
            assert_eq!(meta[0].fpr.to_uppercase(), keys::V4_SIGN.primary_fpr);
            assert_eq!(meta[0].subkeys.len(), 2);
        }
    }
}

// ---------------------------------------------------------------------------
// RFC 9580 §5.6 / §9.4 Compressed Data on consume
// ---------------------------------------------------------------------------

mod rfc9580_compression {
    use super::*;
    use crate::testutil::{packets, seipd};
    use pgp::composed::MessageBuilder;
    use pgp::types::CompressionAlgorithm;

    /// A Compressed Data packet holding a literal of `payload`, built by rPGP.
    fn compressed(alg: CompressionAlgorithm, payload: &[u8]) -> Vec<u8> {
        let mut b = MessageBuilder::from_bytes("", payload.to_vec());
        b.compression(alg);
        let out = b.to_vec(rand::thread_rng()).expect("compress");
        let p = first(&out, 8);
        assert_eq!(p.body[0], u8::from(alg), "precondition: algorithm octet");
        out
    }

    #[test]
    fn zip_compressed_literal_decrypts() {
        // RFC 9580 §9.4: ZIP (ID 1) is a registered compression algorithm;
        // §5.6: the Compressed Data packet "contains a compressed version of"
        // the packets that follow.
        let inner = compressed(CompressionAlgorithm::ZIP, b"zip payload");
        let msg = seipd::v1_with_inner(&keys::V4_SIGN, &inner);
        let res = decrypt_with(&keys::V4_SIGN.secret_armored, &msg).expect("decrypts");
        assert_eq!(res.data, b"zip payload");
    }

    #[test]
    fn bzip2_compressed_literal_decrypts() {
        // RFC 9580 §9.4: BZip2 (ID 3) is a registered compression algorithm.
        let inner = compressed(CompressionAlgorithm::BZip2, b"bzip2 payload");
        let msg = seipd::v1_with_inner(&keys::V4_SIGN, &inner);
        let res = decrypt_with(&keys::V4_SIGN.secret_armored, &msg).expect("decrypts");
        assert_eq!(res.data, b"bzip2 payload");
    }

    #[test]
    fn unknown_compression_algorithm_is_refused() {
        // RFC 9580 §5.6: "A 1-octet compression algorithm"; an unregistered
        // algorithm cannot be decompressed and the message must not be released.
        let mut body = vec![110u8];
        body.extend_from_slice(&packets::literal(b'b', b"", 0, b"opaque"));
        let inner = packets::packet(8, &body);
        let msg = seipd::v1_with_inner(&keys::V4_SIGN, &inner);
        let res = decrypt_with(&keys::V4_SIGN.secret_armored, &msg);
        assert_eq!(res.err(), Some(GfrStatus::ErrorDataCorrupted));
    }
}

// ---------------------------------------------------------------------------
// RFC 9580 §5.2.1.2 / §5.2.4 Text signatures
// ---------------------------------------------------------------------------

mod rfc9580_text_signatures {
    use super::*;
    use crate::types::{GfrSignMode, GfrSignatureStatus};
    use pgp::packet::{Packet, SignatureConfig, SignatureType, Subpacket, SubpacketData};
    use pgp::ser::Serialize;
    use pgp::types::{KeyDetails, Password, Timestamp};

    const CRLF: &[u8] = b"first line\r\nsecond line\r\n";
    const LF: &[u8] = b"first line\nsecond line\n";

    /// A detached Text (0x01) signature over `data` by `V4_SIGN`'s signing
    /// subkey, as binary packet octets.
    fn text_sig(data: &[u8]) -> Vec<u8> {
        let fx = &*keys::V4_SIGN;
        let sub = fx
            .secret
            .secret_subkeys
            .iter()
            .find(|s| s.key.fingerprint().to_string().to_uppercase() == fx.sign_subkey_fpr())
            .expect("signing subkey");
        let mut cfg =
            SignatureConfig::from_key(rand::thread_rng(), &sub.key, SignatureType::Text).unwrap();
        cfg.hashed_subpackets = vec![
            Subpacket::regular(SubpacketData::SignatureCreationTime(Timestamp::now())).unwrap(),
            Subpacket::regular(SubpacketData::IssuerFingerprint(sub.key.fingerprint())).unwrap(),
        ];
        cfg.unhashed_subpackets =
            vec![Subpacket::regular(SubpacketData::IssuerKeyId(sub.key.legacy_key_id())).unwrap()];
        let sig = cfg.sign(&sub.key, &Password::empty(), data).unwrap();
        assert_eq!(sig.typ(), Some(SignatureType::Text));
        Packet::from(sig).to_bytes().unwrap()
    }

    fn verify_both_paths(data: &[u8], sig: &[u8]) {
        let res = verify(
            data,
            sig,
            GfrSignMode::Detached,
            &keys::V4_SIGN.public_armored,
        )
        .expect("verify");
        assert_eq!(statuses(&res.signatures), vec![GfrSignatureStatus::Valid]);

        cb::set_pubkey_answer(&keys::V4_SIGN.public_armored);
        let res = crate::crypto::verify_detached_stream_internal(
            0,
            Cursor::new(data.to_vec()),
            sig,
            Some(cb::pubkey_fetch),
            std::ptr::null_mut(),
        )
        .expect("verify stream");
        assert_eq!(statuses(&res.signatures), vec![GfrSignatureStatus::Valid]);
    }

    #[test]
    fn text_signature_over_crlf_verifies_over_lf() {
        // RFC 9580 §5.2.1.2: a text signature is computed over the text "with
        // its line endings converted to <CR><LF>", so either form verifies.
        let sig = text_sig(CRLF);
        verify_both_paths(CRLF, &sig);
        verify_both_paths(LF, &sig);
    }

    #[test]
    fn text_signature_over_lf_verifies_over_crlf() {
        // RFC 9580 §5.2.1.2: "The signature is calculated over the text data
        // with its line endings converted to <CR><LF>."
        let sig = text_sig(LF);
        verify_both_paths(LF, &sig);
        verify_both_paths(CRLF, &sig);
    }
}

// ---------------------------------------------------------------------------
// RFC 9580 §7.1 Cleartext Signature Framework headers
// ---------------------------------------------------------------------------

mod rfc9580_cleartext_headers {
    use super::*;
    use crate::types::{GfrSignMode, GfrSignatureStatus};

    fn gf_cleartext() -> String {
        let out = crate::crypto::sign_internal(
            0,
            "",
            b"cleartext body\n",
            &[&keys::V4_SIGN.secret_armored],
            None,
            GfrSignMode::ClearText,
            true,
        )
        .unwrap()
        .data;
        String::from_utf8(out).unwrap()
    }

    /// `msg` with its header block (between the BEGIN line and the first
    /// empty line) replaced by `headers`.
    fn with_headers(msg: &str, headers: &[&str]) -> String {
        let begin = "-----BEGIN PGP SIGNED MESSAGE-----";
        let start = msg.find(begin).expect("cleartext header") + begin.len();
        let rest = &msg[start..];
        let rest = rest.trim_start_matches(['\r', '\n']);
        let blank = rest.find("\n\n").expect("header terminator");
        let body = &rest[blank + 1..];
        let mut out = format!("{}\n", &msg[..start]);
        for h in headers {
            out.push_str(h);
            out.push('\n');
        }
        out.push_str(body);
        out
    }

    fn verify_ct(msg: &str) -> Result<crate::crypto::VerifyResultInternal, GfrStatus> {
        verify(
            msg.as_bytes(),
            &[],
            GfrSignMode::ClearText,
            &keys::V4_SIGN.public_armored,
        )
    }

    #[test]
    fn control_rewritten_cleartext_still_verifies() {
        let msg = with_headers(&gf_cleartext(), &[]);
        let res = verify_ct(&msg).expect("verify");
        assert_eq!(statuses(&res.signatures), vec![GfrSignatureStatus::Valid]);
    }

    #[test]
    fn an_armor_header_other_than_hash_is_declined() {
        // RFC 9580 §7.1: "a verifying implementation MUST decline to validate
        // any signature in a cleartext message if that message has any other
        // Armor Header present in this location."
        let msg = with_headers(&gf_cleartext(), &["Comment: x"]);
        let res = verify_ct(&msg);
        assert_eq!(res.err(), Some(GfrStatus::ErrorInternal));
    }

    #[test]
    fn a_well_formed_hash_header_naming_another_hash_is_ignored() {
        // RFC 9580 §6.2.2.3: "When a conformant Hash header is present, a
        // verifying application MUST ignore its contents, deferring to the
        // hash algorithm indicated in the Embedded Signature."
        let msg = with_headers(&gf_cleartext(), &["Hash: SHA1"]);
        let res = verify_ct(&msg).expect("verify");
        assert_eq!(statuses(&res.signatures), vec![GfrSignatureStatus::Valid]);
    }
}

// ---------------------------------------------------------------------------
// RFC 9980 §3.5 Key Version Binding
// ---------------------------------------------------------------------------

mod rfc9980_key_version_binding {
    use super::*;
    use crate::types::{GfrKeyAlgo, GfrOpenPGPKeyVersion};

    /// The v6 public subkey packet of `algo` from a freshly generated v6 key,
    /// rewritten as a v4 key packet (version octet 4, no 4-octet material
    /// length): what a non-conforming implementation would emit.
    fn v4_rewrite(packet: &RawPacket, tag: u8) -> Vec<u8> {
        let body = &packet.body;
        assert_eq!(body[0], 6, "donor is a v6 key packet");
        let mut v4 = vec![4u8];
        v4.extend_from_slice(&body[1..6]); // creation time + algorithm
        v4.extend_from_slice(&body[10..]); // key material, no length prefix
        crate::testutil::packets::packet(tag, &v4)
    }

    fn v6_mlkem1024_subkey_packet() -> RawPacket {
        let sk = keys::generate(
            "ML-KEM-1024 donor <mlkem1024@example.test>",
            &keys::cfg(GfrKeyAlgo::ED448, true, false, GfrOpenPGPKeyVersion::V6),
            &[keys::cfg(
                GfrKeyAlgo::KYBER1024X448,
                false,
                true,
                GfrOpenPGPKeyVersion::V6,
            )],
        );
        let bytes = cert_bytes(&public_armored(&sk));
        let p = first(&bytes, 14);
        assert_eq!(p.body[5], 36, "donor subkey is ML-KEM-1024+X448");
        p
    }

    #[test]
    fn a_v4_mlkem1024_subkey_is_not_usable() {
        // RFC 9980 §3.5: "All PQ(/T) asymmetric algorithms are to be used only
        // in v6 (and newer) keys and certificates, with the single exception of
        // ML-KEM-768+X25519".
        let with_subkey = |subkey: Vec<u8>| {
            let mut cert = cert_bytes(&keys::V4_PRIMARY_ONLY.public_armored);
            cert.extend_from_slice(&subkey);
            armor_public(cert)
        };

        // Control: the same rewrite of a v6 X25519 subkey (legal in v4,
        // RFC 9580 §5.5.5.7) yields a certificate that is read.
        let x25519 = walk(&cert_bytes(&keys::V6_SIGN.public_armored))
            .into_iter()
            .find(|p| p.tag == 14 && p.body[5] == 25)
            .expect("V6_SIGN carries an X25519 subkey");
        let control =
            crate::key::extract_metadata_many_internal(&with_subkey(v4_rewrite(&x25519, 14)))
                .expect("control: a v4 X25519 subkey packet is accepted");
        assert_eq!(control.len(), 1);

        let armored = with_subkey(v4_rewrite(&v6_mlkem1024_subkey_packet(), 14));
        // The whole certificate is refused, not just the subkey.
        assert_eq!(
            crate::key::extract_metadata_many_internal(&armored).err(),
            Some(GfrStatus::ErrorInvalidInput)
        );

        // No usable recipient; the certificate itself is the invalid data.
        let res = crate::crypto::encrypt_internal(0, "", b"x", &[&armored], false);
        assert_eq!(res.err(), Some(GfrStatus::ErrorUnusableKey));
        let rejected = crate::crypto::take_rejected_recipients();
        assert_eq!(rejected.len(), 1);
        assert_eq!(rejected[0].reason, GfrStatus::ErrorInvalidData);
    }

    #[test]
    fn a_v4_mldsa_primary_key_is_refused() {
        // RFC 9980 §5.3.2: "The composite ML-DSA + EdDSA schemes MUST be used
        // only with v6 keys".
        // A v6 primary of `algo` rewritten as a v4 key packet, followed by its
        // User ID and (now meaningless) self-signatures.
        let crafted_cert = |algo: GfrKeyAlgo, id: u8| {
            let sk = keys::generate(
                "Primary donor <primary@example.test>",
                &keys::cfg(algo, true, false, GfrOpenPGPKeyVersion::V6),
                &[],
            );
            let bytes = cert_bytes(&public_armored(&sk));
            let packets = walk(&bytes);
            assert_eq!(packets[0].tag, 6);
            assert_eq!(packets[0].body[5], id, "donor primary algorithm");
            let mut crafted = v4_rewrite(&packets[0], 6);
            for p in &packets[1..] {
                if p.tag == 13 || p.tag == 2 {
                    crafted.extend_from_slice(&bytes[p.start..p.end]);
                }
            }
            armor_public(crafted)
        };

        // Control: the same rewrite of an Ed25519 primary (legal in v4) is read.
        let control =
            crate::key::extract_metadata_many_internal(&crafted_cert(GfrKeyAlgo::ED25519, 27))
                .expect("control: a v4 Ed25519 primary key packet is accepted");
        assert_eq!(control.len(), 1);

        let armored = crafted_cert(GfrKeyAlgo::MLDSA65ED25519, 30);

        assert_eq!(
            crate::key::extract_metadata_many_internal(&armored).err(),
            Some(GfrStatus::ErrorInvalidInput)
        );
        // No usable recipient; the certificate itself is the invalid data.
        let res = crate::crypto::encrypt_internal(0, "", b"x", &[&armored], false);
        assert_eq!(res.err(), Some(GfrStatus::ErrorUnusableKey));
        let rejected = crate::crypto::take_rejected_recipients();
        assert_eq!(rejected.len(), 1);
        assert_eq!(rejected[0].reason, GfrStatus::ErrorInvalidData);
    }

    #[test]
    fn add_subkey_refuses_v6_only_pq_algorithms_on_a_v4_key() {
        // RFC 9980 §3.5: "All PQ(/T) asymmetric algorithms are to be used only
        // in v6 (and newer) keys and certificates".
        for (algo, sign, enc) in [
            (GfrKeyAlgo::KYBER1024X448, false, true),
            (GfrKeyAlgo::MLDSA87ED448, true, false),
            (GfrKeyAlgo::SLHDSASHAKE128S, true, false),
        ] {
            let started = std::time::Instant::now();
            let res = crate::keygen::add_subkey_internal(
                0,
                &keys::V4_SIGN.secret_armored,
                &keys::cfg(algo, sign, enc, GfrOpenPGPKeyVersion::V4),
                None,
            );
            assert_eq!(
                res.err(),
                Some(GfrStatus::ErrorUnsupportedAlgorithm),
                "{algo:?} must be refused on a v4 key"
            );
            assert!(
                started.elapsed() < std::time::Duration::from_secs(5),
                "{algo:?} refusal must precede key generation"
            );
        }
    }
}

// ---------------------------------------------------------------------------
// RFC 9980 §4.3.1 / §5.3.1 / §7.1 generation
// ---------------------------------------------------------------------------

mod rfc9980_generation {
    use super::*;
    use crate::types::{GfrKeyAlgo, GfrOpenPGPKeyVersion, GfrRevocationCode, GfrSignMode};
    use once_cell::sync::Lazy;
    use pgp::crypto::aead::AeadAlgorithm;
    use pgp::crypto::sym::SymmetricKeyAlgorithm;
    use pgp::packet::Signature;

    /// v6 ML-DSA-65+Ed25519 primary + ML-KEM-768+X25519 subkey.
    static V6_MLDSA: Lazy<SignedSecretKey> = Lazy::new(|| {
        keys::generate(
            "ML-DSA Fixture <mldsa65@example.test>",
            &keys::cfg(
                GfrKeyAlgo::MLDSA65ED25519,
                true,
                false,
                GfrOpenPGPKeyVersion::V6,
            ),
            &[keys::cfg(
                GfrKeyAlgo::KYBER768X25519,
                false,
                true,
                GfrOpenPGPKeyVersion::V6,
            )],
        )
    });

    fn secret_armored(sk: &SignedSecretKey) -> String {
        sk.to_armored_string(crate::utils::armor_opts()).unwrap()
    }

    fn assert_digest_at_least_256(sig: &Signature, what: &str) {
        let hash = sig.hash_alg().expect("a hash algorithm");
        let bits = hash.digest_size().expect("a known digest") * 8;
        assert!(bits >= 256, "{what}: {hash:?} has a {bits}-bit digest");
    }

    #[test]
    fn mldsa_signatures_use_a_digest_of_at_least_256_bits() {
        // RFC 9980 §5.3.1: "A composite ML-DSA + EdDSA signature MUST use a
        // hash algorithm with a digest size of at least 256 bits".
        let sk = &*V6_MLDSA;
        let mut n = 0;
        for sig in sk.details.direct_signatures.iter() {
            assert_digest_at_least_256(sig, "direct-key self-signature");
            n += 1;
        }
        for user in &sk.details.users {
            for sig in &user.signatures {
                assert_digest_at_least_256(sig, "User ID self-signature");
                n += 1;
            }
        }
        for sub in &sk.secret_subkeys {
            for sig in &sub.signatures {
                assert_digest_at_least_256(sig, "subkey binding");
                n += 1;
            }
        }
        assert!(
            n >= 3,
            "expected direct, User ID and binding signatures, got {n}"
        );

        let armored = secret_armored(sk);
        let signed = crate::crypto::sign_internal(
            0,
            "",
            b"pq data",
            &[&armored],
            None,
            GfrSignMode::Detached,
            false,
        )
        .expect("sign");
        let sigs = crate::crypto::parse_all_signature_packets(&signed.data);
        assert_eq!(sigs.len(), 1);
        assert_eq!(
            sigs[0].config().map(|c| c.pub_alg),
            Some(pgp::crypto::public_key::PublicKeyAlgorithm::MlDsa65Ed25519)
        );
        assert_digest_at_least_256(&sigs[0], "data signature");

        let rev = crate::key::generate_key_rev_cert_internal(
            0,
            &armored,
            GfrRevocationCode::Retired,
            Some("pq"),
            None,
        )
        .expect("revocation");
        let rev_sigs = crate::crypto::parse_all_signature_packets(rev.as_bytes());
        assert_eq!(rev_sigs.len(), 1);
        assert_digest_at_least_256(&rev_sigs[0], "key revocation");
    }

    #[test]
    fn mlkem768_v4_subkey_gets_v3_pkesk_seipd_v1_and_aes() {
        // RFC 9980 §4.3.1: "In the case of v3 PKESK packets for ML-KEM
        // composite schemes, the symmetric algorithm used MUST be AES-128,
        // AES-192 or AES-256".
        let added = crate::keygen::add_subkey_internal(
            0,
            &keys::V4_PRIMARY_ONLY.secret_armored,
            &keys::cfg(
                GfrKeyAlgo::KYBER768X25519,
                false,
                true,
                GfrOpenPGPKeyVersion::V4,
            ),
            None,
        )
        .expect("ML-KEM-768+X25519 is allowed in a v4 subkey");
        let msg = encrypt_bin(&[&added.public], b"pq v4");

        let packets = walk(&msg);
        let pkesks: Vec<_> = packets.iter().filter(|p| p.tag == 1).collect();
        assert_eq!(pkesks.len(), 1);
        assert_eq!(pkesks[0].body[0], 3, "v3 PKESK");
        assert_eq!(pkesks[0].body[9], 35, "to the ML-KEM-768+X25519 subkey");
        let seipd = packets.iter().find(|p| p.tag == 18).expect("a SEIPD");
        assert_eq!(seipd.body[0], 1, "SEIPD v1");

        let res = decrypt_with(&added.secret, &msg).expect("decrypts");
        assert_eq!(res.data, b"pq v4");
        assert!(res.sym_algo.starts_with("AES"), "{}", res.sym_algo);
        assert_eq!(res.sym_algo, "AES256.CFB");
    }

    #[test]
    fn pq_certificate_prefers_aes256_and_aes256_ocb() {
        // RFC 9980 §7.1: "A certificate that contains a PQ(/T) key SHOULD
        // include AES-256 in the 'Preferred Symmetric Ciphers for v1 SEIPD'
        // subpacket and SHOULD include the pair AES-256 with OCB in the
        // 'Preferred AEAD Ciphersuites' subpacket".
        let ed25519_primary = keys::V6_PQ.secret.clone();
        for sk in [&*V6_MLDSA, &ed25519_primary] {
            let cert = SignedPublicKey::from(sk.clone());
            let pref = crate::key::preference_self_signature(&cert).expect("preference sig");
            assert!(
                pref.preferred_symmetric_algs()
                    .contains(&SymmetricKeyAlgorithm::AES256),
                "{:?}",
                pref.preferred_symmetric_algs()
            );
            assert!(
                pref.preferred_aead_algs()
                    .contains(&(SymmetricKeyAlgorithm::AES256, AeadAlgorithm::Ocb)),
                "{:?}",
                pref.preferred_aead_algs()
            );
        }
    }
}

// ---------------------------------------------------------------------------
// RFC 9580 §6 armor: whitespace inside the base64 body
// ---------------------------------------------------------------------------

mod rfc9580_armor_whitespace {
    use super::*;
    use crate::testutil::assert as ta;
    use crate::types::{GfrSignMode, GfrSignatureStatus};

    /// `armored` with spaces and tabs added inside its base64 body, as a key
    /// or signature pasted from a web page or a mail client often arrives.
    fn with_body_whitespace(armored: &str) -> String {
        let mut out = String::new();
        let mut in_body = false;
        for line in armored.lines() {
            if line.starts_with("-----END") {
                in_body = false;
            }
            if in_body && !line.is_empty() {
                out.push('\t');
                out.push_str(line);
                out.push_str("  ");
            } else {
                out.push_str(line);
            }
            out.push('\n');
            if line.is_empty() && !in_body {
                in_body = true;
            }
        }
        out
    }

    #[test]
    fn a_cleartext_body_keeps_its_whitespace() {
        // The whitespace a cleartext-signed body carries is signed content;
        // only the base64 of its signature block may be normalized.
        let fixture = &keys::V4_SIGN;
        let body = b"a  b\tc\n    indented\n";
        let signed = crate::crypto::sign_internal(
            0,
            "",
            body,
            &[&fixture.secret_armored],
            None,
            GfrSignMode::ClearText,
            true,
        )
        .unwrap()
        .data;
        let text = std::str::from_utf8(&signed).unwrap();
        assert_eq!(crate::utils::strip_armor_whitespace(text), text);
        // Whitespace added inside the signature block's base64 only.
        let split = text.find("-----BEGIN PGP SIGNATURE-----").unwrap();
        let pasted = format!("{}{}", &text[..split], with_body_whitespace(&text[split..]));
        assert_ne!(pasted, text, "precondition: whitespace added");
        let normalized = crate::utils::strip_armor_whitespace(&pasted);
        assert!(
            normalized.contains("a  b\tc\n    indented\n"),
            "{normalized}"
        );
        cb::set_pubkey_answer(&fixture.public_armored);
        let res = crate::crypto::verify_internal(
            normalized.as_bytes(),
            &[],
            GfrSignMode::ClearText,
            Some(cb::pubkey_fetch),
            std::ptr::null_mut(),
        )
        .expect("verifies");
        ta::only_status(&res.signatures, GfrSignatureStatus::Valid);
    }

    #[test]
    fn whitespace_in_the_base64_of_a_message_is_ignored_when_decrypting_and_verifying() {
        // Path parity (round four): decrypt-and-verify reads the same pasted
        // text that plain decryption accepts.
        let fixture = &keys::V4_SIGN;
        let sealed = crate::crypto::encrypt_and_sign_internal(
            0,
            "",
            b"pasted message",
            &[&fixture.public_armored],
            &[&fixture.secret_armored],
            None,
            true,
        )
        .unwrap()
        .data;
        let pasted = with_body_whitespace(std::str::from_utf8(&sealed).unwrap());
        cb::set_seckey_answer(&fixture.secret_armored);
        cb::set_pubkey_answer(&fixture.public_armored);
        let res = crate::crypto::decrypt_and_verify_internal(
            0,
            pasted.as_bytes(),
            Some(cb::seckey_fetch),
            None,
            Some(cb::pubkey_fetch),
            std::ptr::null_mut(),
        )
        .expect("the pasted message decrypts");
        assert_eq!(res.data, b"pasted message");
        ta::only_status(&res.signatures, GfrSignatureStatus::Valid);
    }

    #[test]
    fn whitespace_in_a_cleartext_signature_block_is_ignored() {
        let fixture = &keys::V4_SIGN;
        let signed = crate::crypto::sign_internal(
            0,
            "",
            b"pasted cleartext\n",
            &[&fixture.secret_armored],
            None,
            GfrSignMode::ClearText,
            true,
        )
        .unwrap()
        .data;
        let text = std::str::from_utf8(&signed).unwrap();
        let split = text.find("-----BEGIN PGP SIGNATURE-----").unwrap();
        let pasted = format!("{}{}", &text[..split], with_body_whitespace(&text[split..]));
        cb::set_pubkey_answer(&fixture.public_armored);
        let res = crate::crypto::verify_internal(
            pasted.as_bytes(),
            &[],
            GfrSignMode::ClearText,
            Some(cb::pubkey_fetch),
            std::ptr::null_mut(),
        )
        .expect("the pasted cleartext verifies");
        ta::only_status(&res.signatures, GfrSignatureStatus::Valid);
    }

    #[test]
    fn whitespace_in_the_base64_of_a_key_is_ignored() {
        // RFC 9580 §6: "When decoding base64, an OpenPGP implementation MUST
        // ignore all whitespace."
        let fixture = &keys::V4_SIGN;
        let pasted = with_body_whitespace(&fixture.public_armored);
        assert_ne!(
            pasted, fixture.public_armored,
            "precondition: whitespace added"
        );
        let meta =
            crate::key::extract_metadata_many_internal(&pasted).expect("the pasted key imports");
        assert_eq!(meta.len(), 1);
        assert!(meta[0].fpr.eq_ignore_ascii_case(&fixture.primary_fpr));
    }

    #[test]
    fn whitespace_in_the_base64_of_a_detached_signature_is_ignored() {
        let fixture = &keys::V4_SIGN;
        let data = b"pasted signature";
        let sig = crate::crypto::sign_internal(
            0,
            "",
            data,
            &[&fixture.secret_armored],
            None,
            GfrSignMode::Detached,
            true,
        )
        .unwrap()
        .data;
        let pasted = with_body_whitespace(std::str::from_utf8(&sig).unwrap());
        cb::set_pubkey_answer(&fixture.public_armored);
        let res = crate::crypto::verify_internal(
            data,
            pasted.as_bytes(),
            GfrSignMode::Detached,
            Some(cb::pubkey_fetch),
            std::ptr::null_mut(),
        )
        .expect("verifies");
        ta::only_status(&res.signatures, GfrSignatureStatus::Valid);
    }
}
