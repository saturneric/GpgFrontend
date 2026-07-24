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
use crate::utils::armor_opts;

/// Verify a detached signature against a data stream.
///
/// `data_stream` must implement `Seek` because the verifier rewinds it before
/// testing each fetched public key — rPGP's signature API is not resumable.
/// The signature is tried against both the primary key and every subkey to
/// work around rpgp's strict identity matching.
pub fn verify_detached_stream_internal<R>(
    channel: i32,
    mut data_stream: R,
    sig_data: &[u8],
    fetch_pubkey_cb: Option<GfrPublicKeyFetchCb>,
    user_data: *mut std::ffi::c_void,
) -> Result<VerifyStreamResultInternal, GfrStatus>
where
    R: Read + Seek + Send + Sync,
{
    if sig_data.is_empty() {
        return Err(GfrStatus::ErrorInvalidInput);
    }

    // 1. Parse every detached signature packet. A detached blob may legitimately
    //    carry several signature packets (multiple signers over the same data);
    //    attribution is per packet, exactly as on the in-memory detached path.
    let sig_packets = parse_all_signature_packets(sig_data);
    if sig_packets.is_empty() {
        return Err(GfrStatus::ErrorInvalidInput);
    }

    // 2. Build one result entry per packet (issuer fpr/key-id + metadata) so the
    //    signer certs can be fetched before any hashing.
    let mut signatures: Vec<SignatureResultInternal> = sig_packets
        .iter()
        .map(|sig| sig_entry_from_packet(sig, GfrSignMode::Detached))
        .collect();

    // 3. Dynamically fetch the signer certs via the C callback.
    let certs = fetch_certs_for_signatures(&signatures, fetch_pubkey_cb, user_data);
    log::debug!(
        "Fetched and parsed {} public keys for detached stream verification",
        certs.len()
    );

    if crate::cancel::is_cancelled(channel) {
        return Err(GfrStatus::ErrorCanceled);
    }

    // 4. Attribute each packet per index through the shared driver. Hashing the
    //    seekable stream is the long-running part;
    //    `signature_verifies_under_usable_key_stream` rewinds before every attempt
    //    and hashes through a `CancellableReader` that aborts mid-hash once the
    //    user cancels. `verify` only reports success/failure, so afterwards we
    //    consult the cancel flag directly and surface `ErrorCanceled` rather than
    //    a spurious "not verified" result.
    let mut is_verified = false;
    attribute_entries(&mut signatures, &certs, &mut is_verified, |i, cert| {
        signature_verifies_under_usable_key_stream(cert, &sig_packets[i], &mut data_stream, channel)
    });

    if crate::cancel::is_cancelled(channel) {
        return Err(GfrStatus::ErrorCanceled);
    }

    Ok(VerifyStreamResultInternal {
        is_verified,
        signatures,
    })
}

/// Verify a signed in-memory buffer and return the extracted plaintext.
///
/// `sig_data` is only inspected in `Detached` mode; pass `&[]` for inline and
/// clear-text modes. Verification is skipped (but not an error) when
/// `fetch_pubkey_cb` is `None`.
pub fn verify_internal(
    data: &[u8],
    sig_data: &[u8],
    mode: GfrSignMode,
    fetch_pubkey_cb: Option<GfrPublicKeyFetchCb>,
    user_data: *mut std::ffi::c_void,
) -> Result<VerifyResultInternal, GfrStatus> {
    match mode {
        // ---------------------------------------------------------
        // MODE 0: INLINE SIGNATURE
        // ---------------------------------------------------------
        GfrSignMode::Inline => {
            let mut msg = Message::from_armor(Cursor::new(data))
                .map(|(m, _)| m)
                .or_else(|_| Message::from_bytes(data))
                .into_gfr()?;

            // One-pass signatures live in trailing packets, so the signature
            // count and per-index hashes only become available AFTER the message
            // body has been read to the end (rPGP: "the message must have been
            // read to the end before calling verify"). Drain the body first —
            // decompressing a compressed wrapper if present, with a size cap so a
            // compression bomb cannot exhaust memory (RFC 9580 §13.14, the same
            // guard the decrypt path applies) — then count and verify. Reading
            // num_signatures() before this yields 0 and silently downgrades every
            // genuine inline signature to BadSignature.
            let clear_data = if msg.is_compressed() {
                msg = msg.decompress().into_gfr()?;
                read_to_end_capped(&mut msg)?
            } else {
                msg.as_data_vec().into_gfr()?
            };

            // Build one entry per actual signature packet from the (possibly
            // decompressed) message. Deriving these from the parsed message rather
            // than sniffing the raw outer bytes is essential for a
            // compression-wrapped inline signature, whose signature packets are
            // invisible until the body is decompressed; it also lets
            // sig_entry_from_packet apply the issuer Key ID fallback (B1).
            let mut signatures = signature_entries_from_message(&msg);
            let certs = fetch_certs_for_signatures(&signatures, fetch_pubkey_cb, user_data);
            let mut is_verified = false;

            // verify() only checks signature at index 0; attribute each index to
            // the exact issuer that made it (per-index attribution).
            attribute_entries(&mut signatures, &certs, &mut is_verified, |i, cert| {
                verify_index_under_usable_key(&msg, i, cert)
            });

            Ok(VerifyResultInternal {
                data: clear_data,
                is_verified,
                signatures,
            })
        }

        // ---------------------------------------------------------
        // MODE 1: CLEARTEXT SIGNATURE
        // ---------------------------------------------------------
        GfrSignMode::ClearText => {
            let text_str = std::str::from_utf8(data).record_err(GfrStatus::ErrorInvalidInput)?;
            let (msg, _) = CleartextSignedMessage::from_string(text_str).into_gfr()?;

            // Build one entry per signature packet and verify each packet
            // individually, so a genuine signature is never mis-attributed to a
            // sibling packet that names the same certificate but does not itself
            // verify (per-index attribution, the analogue of the inline path). The
            // per-cert form stamped every issuer-matching entry `Valid` as soon as
            // any one verified.
            let signed_text = msg.signed_text();
            let sig_packets = msg.signatures();
            let mut signatures: Vec<SignatureResultInternal> = sig_packets
                .iter()
                .map(|sig| sig_entry_from_packet(sig, mode))
                .collect();

            let certs = fetch_certs_for_signatures(&signatures, fetch_pubkey_cb, user_data);
            let mut is_verified = false;

            attribute_entries(&mut signatures, &certs, &mut is_verified, |i, cert| {
                signature_verifies_under_usable_key(cert, &sig_packets[i], signed_text.as_bytes())
            });

            let clear_data = msg.to_armored_bytes(armor_opts()).unwrap_or_default();
            Ok(VerifyResultInternal {
                data: clear_data,
                is_verified,
                signatures,
            })
        }

        // ---------------------------------------------------------
        // MODE 2: DETACHED SIGNATURE
        // ---------------------------------------------------------
        GfrSignMode::Detached => {
            if sig_data.is_empty() {
                return Err(GfrStatus::ErrorInvalidInput);
            }

            // A detached blob may carry several signature packets (multiple
            // signers). Parse them all and verify each independently — the same
            // per-index attribution as the cleartext path.
            let sig_packets = parse_all_signature_packets(sig_data);
            if sig_packets.is_empty() {
                return Err(GfrStatus::ErrorInvalidInput);
            }

            let mut signatures: Vec<SignatureResultInternal> = sig_packets
                .iter()
                .map(|sig| sig_entry_from_packet(sig, mode))
                .collect();

            let certs = fetch_certs_for_signatures(&signatures, fetch_pubkey_cb, user_data);
            let mut is_verified = false;

            attribute_entries(&mut signatures, &certs, &mut is_verified, |i, cert| {
                signature_verifies_under_usable_key(cert, &sig_packets[i], data)
            });

            Ok(VerifyResultInternal {
                data: Vec::new(),
                is_verified,
                signatures,
            })
        }
    }
}

#[cfg(test)]
mod verify_tests {
    //! Signature verification across all three modes.
    //!
    //! This module is the regression lock for the seven consume-side interop
    //! findings (B1-B7) and the four verify gaps found by the RFC 9580
    //! known-answer suite. Each of those has a named test below so a
    //! reappearance fails loudly rather than silently accepting a bad
    //! signature.
    //!
    //! For this engine a signature is valid **only** when the policy gate
    //! leaves it `Valid`. A cryptographically sound signature that fails the
    //! §9.5 weak-hash gate, the §5.2.3.18 expiry gate, or is issued by a
    //! revoked or expired key is deliberately *not* valid.

    use super::*;
    use crate::testutil::{assert as ta, cb, corpus, keys, rfc9580};

    fn verify_detached(data: &[u8], sig: &[u8], cert: Option<&str>) -> VerifyResultInternal {
        match cert {
            Some(block) => cb::set_pubkey_answer(block),
            None => cb::clear_pubkey_answer(),
        }
        verify_internal(
            data,
            sig,
            GfrSignMode::Detached,
            Some(cb::pubkey_fetch),
            std::ptr::null_mut(),
        )
        .expect("verify returns a result")
    }

    /// Verify in any mode. Note the argument split differs by mode: Detached
    /// takes the signed data in `data` and the signature in `sig_data`, while
    /// Inline and ClearText carry the whole self-contained message in `data`
    /// and ignore `sig_data`.
    fn verify_mode(
        data: &[u8],
        sig: &[u8],
        mode: GfrSignMode,
        cert: Option<&str>,
    ) -> Result<VerifyResultInternal, GfrStatus> {
        match cert {
            Some(block) => cb::set_pubkey_answer(block),
            None => cb::clear_pubkey_answer(),
        }
        let (data, sig) = match mode {
            GfrSignMode::Detached => (data, sig),
            // The message is self-contained; it belongs in `data`.
            _ => (sig, &[][..]),
        };
        verify_internal(data, sig, mode, Some(cb::pubkey_fetch), std::ptr::null_mut())
    }

    // -- the happy paths ----------------------------------------------------

    #[test]
    fn a_good_detached_signature_is_valid() {
        let res = verify_detached(corpus::PAYLOAD, corpus::SIG_GOOD_DETACHED, Some(corpus::AUX_GOOD));
        assert!(res.is_verified);
        ta::exactly_one_valid(&res.signatures);
    }

    #[test]
    fn a_good_v6_detached_signature_is_valid() {
        // §5.2.3 v6 signatures are salted and use 4-octet subpacket counts.
        let v6_cert = crate::key::extract_public_key_internal(corpus::AUX_V6_SECRET)
            .expect("public half of the v6 key");
        let res = verify_detached(corpus::PAYLOAD, corpus::SIG_V6_DETACHED, Some(&v6_cert));
        assert!(res.is_verified);
        ta::exactly_one_valid(&res.signatures);
    }

    #[test]
    fn a_good_cleartext_signature_is_valid() {
        // §7: the Cleartext Signature Framework.
        let res = verify_mode(
            &[],
            corpus::SIG_GOOD_CLEARTEXT.as_bytes(),
            GfrSignMode::ClearText,
            Some(corpus::AUX_GOOD),
        )
        .expect("verify");
        assert!(res.is_verified);
        ta::exactly_one_valid(&res.signatures);
    }

    #[test]
    fn a_cleartext_signature_yields_the_signed_text() {
        let res = verify_mode(
            &[],
            corpus::SIG_GOOD_CLEARTEXT.as_bytes(),
            GfrSignMode::ClearText,
            Some(corpus::AUX_GOOD),
        )
        .expect("verify");
        assert!(!res.data.is_empty(), "the cleartext body must be returned");
    }

    #[test]
    fn a_good_inline_signature_is_valid() {
        // Gap 2: one-pass signatures live in trailing packets, so the body
        // must be drained before counting them. Before the fix this reported
        // zero signatures and downgraded every genuine one to BadSignature.
        let v6_cert = crate::key::extract_public_key_internal(corpus::AUX_V6_SECRET)
            .expect("public half of the v6 key");
        let res = verify_mode(
            &[],
            corpus::SIG_GOOD_INLINE_V6,
            GfrSignMode::Inline,
            Some(&v6_cert),
        )
        .expect("verify");
        assert!(res.is_verified, "an inline v6 signature must verify");
    }

    #[test]
    fn an_inline_signature_yields_the_signed_payload() {
        let v6_cert = crate::key::extract_public_key_internal(corpus::AUX_V6_SECRET)
            .expect("public half of the v6 key");
        let res = verify_mode(
            &[],
            corpus::SIG_GOOD_INLINE_V6,
            GfrSignMode::Inline,
            Some(&v6_cert),
        )
        .expect("verify");
        assert_eq!(res.data, corpus::PAYLOAD);
    }

    // -- the four verify gaps ------------------------------------------------

    #[test]
    fn gap3_a_signature_from_an_expired_key_is_not_valid() {
        // §5.2.3.13: the signing key's validity period has passed. The
        // signature itself is cryptographically sound -- it was made while the
        // key was live -- which is exactly why the key-expiry gate is needed.
        let res = verify_detached(corpus::PAYLOAD, corpus::SIG_EXPIRED, Some(corpus::AUX_EXPIRED));
        assert!(!res.is_verified);
        ta::none_valid(&res.signatures);
    }

    #[test]
    fn gap4_a_signature_from_a_revoked_key_is_not_valid() {
        // §5.2.1.11: "A revoked key is not to be used."
        let res =
            verify_detached(corpus::PAYLOAD, corpus::SIG_REVOKEDKEY, Some(corpus::AUX_REVOKED));
        assert!(!res.is_verified);
        ta::none_valid(&res.signatures);
    }

    #[test]
    fn a_sha1_signature_is_never_valid() {
        // §9.5: "Implementations MUST NOT validate any recent signature that
        // depends on MD5, SHA-1, or RIPEMD-160."
        let res = verify_detached(corpus::PAYLOAD, corpus::SIG_SHA1_DETACHED, Some(corpus::AUX_SHA1));
        assert!(!res.is_verified);
        ta::none_valid(&res.signatures);
    }

    #[test]
    fn a_sha1_signature_is_still_attributed_to_its_issuer() {
        // Rejecting it must not mean losing track of who made it; the UI still
        // wants to say "signed by X, using a weak hash".
        let res = verify_detached(corpus::PAYLOAD, corpus::SIG_SHA1_DETACHED, Some(corpus::AUX_SHA1));
        assert!(!res.signatures.is_empty());
        assert!(!res.signatures[0].fpr.is_empty());
    }

    // -- negatives ------------------------------------------------------------

    #[test]
    fn a_mutated_signature_is_not_valid() {
        let res =
            verify_detached(corpus::PAYLOAD, corpus::SIG_BAD_MUTATED, Some(corpus::AUX_GOOD));
        assert!(!res.is_verified);
        ta::none_valid(&res.signatures);
    }

    #[test]
    fn a_good_signature_over_the_wrong_data_is_not_valid() {
        let res = verify_detached(b"different payload entirely", corpus::SIG_GOOD_DETACHED, Some(corpus::AUX_GOOD));
        assert!(!res.is_verified);
        ta::none_valid(&res.signatures);
    }

    #[test]
    fn an_unknown_issuer_stays_no_key_rather_than_bad_signature() {
        // The distinction a user actually cares about: "I don't have the key"
        // must never be presented as "this signature is forged".
        let res = verify_detached(corpus::PAYLOAD, corpus::SIG_GOOD_DETACHED, None);
        assert!(!res.is_verified);
        assert_eq!(res.signatures[0].status, GfrSignatureStatus::NoKey);
    }

    #[test]
    fn the_wrong_certificate_leaves_the_signature_unattributed() {
        let res = verify_detached(corpus::PAYLOAD, corpus::SIG_GOOD_DETACHED, Some(corpus::KEY2_PUBLIC));
        assert!(!res.is_verified);
        assert_eq!(res.signatures[0].status, GfrSignatureStatus::NoKey);
    }

    #[test]
    fn a_garbage_certificate_from_the_callback_is_survivable() {
        cb::clear_pubkey_answer();
        let res = verify_internal(
            corpus::PAYLOAD,
            corpus::SIG_GOOD_DETACHED,
            GfrSignMode::Detached,
            Some(cb::pubkey_garbage),
            std::ptr::null_mut(),
        )
        .expect("verify still returns");
        assert!(!res.is_verified);
    }

    #[test]
    fn verifying_without_a_public_key_callback_is_not_an_error() {
        // Sniffing "who signed this?" without a keyring is a legitimate query.
        let res = verify_internal(
            corpus::PAYLOAD,
            corpus::SIG_GOOD_DETACHED,
            GfrSignMode::Detached,
            None,
            std::ptr::null_mut(),
        )
        .expect("verify returns a result");
        assert!(!res.is_verified);
        assert_eq!(res.signatures.len(), 1, "the issuer is still reported");
    }

    #[test]
    fn an_empty_signature_buffer_is_an_error() {
        assert!(verify_mode(corpus::PAYLOAD, &[], GfrSignMode::Detached, None).is_err());
    }

    #[test]
    fn garbage_as_a_signature_is_an_error_or_yields_nothing() {
        match verify_mode(corpus::PAYLOAD, corpus::GARBAGE, GfrSignMode::Detached, None) {
            Ok(res) => assert!(res.signatures.is_empty() && !res.is_verified),
            Err(status) => assert!((status as i32) < 0),
        }
    }

    #[test]
    fn verifying_never_panics_on_adversarial_input() {
        for vector in [
            corpus::GARBAGE,
            corpus::EMPTY,
            corpus::ENC_SED_TAG9,
            corpus::PKESK_NO_SEIPD,
            corpus::TRUNCATED_ARMOR.as_bytes(),
        ] {
            for mode in [GfrSignMode::Detached, GfrSignMode::Inline, GfrSignMode::ClearText] {
                let outcome = std::panic::catch_unwind(|| {
                    verify_internal(corpus::PAYLOAD, vector, mode, None, std::ptr::null_mut())
                        .is_ok()
                });
                assert!(outcome.is_ok(), "panicked on {mode:?}");
            }
        }
    }

    #[test]
    fn every_truncation_of_a_signature_fails_cleanly() {
        let full = corpus::SIG_GOOD_DETACHED;
        for n in (1..full.len()).step_by(23) {
            let outcome = std::panic::catch_unwind(|| {
                verify_internal(
                    corpus::PAYLOAD,
                    &full[..n],
                    GfrSignMode::Detached,
                    None,
                    std::ptr::null_mut(),
                )
                .is_ok()
            });
            assert!(outcome.is_ok(), "panicked on a {n}-byte prefix");
        }
    }

    // -- B1-B7 regression locks ----------------------------------------------

    #[test]
    fn b2_a_weak_companion_signature_is_surfaced_not_deduped() {
        // Two signatures from one issuer that differ only in hash algorithm.
        // De-duplicating by issuer would drop the weak one and hide it from
        // the §9.5 gate; both must be reported and gated individually.
        let res = verify_detached(
            corpus::PAYLOAD,
            corpus::SIG_STRONG_WEAK_SAME_KEY,
            Some(corpus::AUX_GOOD),
        );
        assert_eq!(res.signatures.len(), 2, "both packets must be surfaced");
        assert!(
            res.signatures.iter().any(|s| sig_hash_algo_is_weak(&s.hash_algo)),
            "the weak one must not be silently discarded"
        );
    }

    #[test]
    fn b2_only_the_strong_signature_of_the_pair_can_be_valid() {
        let res = verify_detached(
            corpus::PAYLOAD,
            corpus::SIG_STRONG_WEAK_SAME_KEY,
            Some(corpus::AUX_GOOD),
        );
        for sig in &res.signatures {
            if sig_hash_algo_is_weak(&sig.hash_algo) {
                assert_ne!(sig.status, GfrSignatureStatus::Valid);
            }
        }
    }

    #[test]
    fn b4_two_signers_on_a_cleartext_message_are_attributed_per_index() {
        // Before the fix, attribution was per *certificate*: one genuine
        // signature made every sibling packet naming that cert look Valid.
        let res = verify_mode(
            &[],
            corpus::SIG_TWO_SIGNER_CLEARTEXT.as_bytes(),
            GfrSignMode::ClearText,
            Some(corpus::AUX_GOOD),
        )
        .expect("verify");
        assert!(res.signatures.len() >= 2, "both signers must be listed");
        let valid = res
            .signatures
            .iter()
            .filter(|s| s.status == GfrSignatureStatus::Valid)
            .count();
        assert!(
            valid <= 1,
            "only the signature made by the supplied cert may be valid, got {valid}"
        );
    }

    #[test]
    fn b4_two_packets_naming_one_issuer_do_not_share_a_verdict() {
        // `sig_two_same_issuer_one_valid.sig` holds a genuine signature and a
        // forged sibling naming the same certificate.
        let res = verify_detached(
            corpus::PAYLOAD,
            corpus::SIG_TWO_SAME_ISSUER_ONE_VALID,
            Some(corpus::AUX_GOOD),
        );
        assert_eq!(res.signatures.len(), 2);
        ta::exactly_one_valid(&res.signatures);
    }

    #[test]
    fn b5_a_compressed_inline_signature_verifies() {
        // The inline path must decompress before sniffing: sniffing the outer
        // compressed bytes finds no signature packets at all.
        let res = verify_mode(
            &[],
            corpus::SIG_INLINE_COMPRESSED,
            GfrSignMode::Inline,
            Some(corpus::AUX_GOOD),
        );
        // Whatever the verdict, the signature must at least be *found*.
        if let Ok(res) = res {
            assert!(
                !res.signatures.is_empty(),
                "the wrapped signature must be discovered after decompressing"
            );
        }
    }

    #[test]
    fn b7_a_forged_revocation_does_not_invalidate_a_good_signature() {
        // The tampered certificate carries a fabricated primary-key
        // revocation. Honouring it would let anyone silently invalidate
        // someone else's signatures.
        let res = verify_detached(
            corpus::PAYLOAD,
            corpus::SIG_GOOD_DETACHED,
            Some(corpus::AUX_FORGED_REVOCATION),
        );
        assert!(
            res.is_verified,
            "a forged revocation must not suppress a genuine signature"
        );
    }

    // -- RFC 9580 Appendix A --------------------------------------------------

    #[test]
    fn appendix_a2_detached_signature_is_reported() {
        // A.1/A.2 use Ed25519Legacy (§5.2.3.3, deprecated). The engine must
        // still parse and attribute it, whatever the final verdict.
        let res = verify_mode(
            rfc9580::A2_PLAINTEXT,
            rfc9580::A2_V4_ED25519LEGACY_SIG.as_bytes(),
            GfrSignMode::Detached,
            Some(rfc9580::A1_V4_ED25519LEGACY_CERT),
        )
        .expect("verify returns a result");
        assert_eq!(res.signatures.len(), 1);
    }

    #[test]
    fn appendix_a6_cleartext_message_verifies_end_to_end() {
        let res = verify_mode(
            &[],
            rfc9580::A6_CLEARTEXT.as_bytes(),
            GfrSignMode::ClearText,
            Some(rfc9580::A3_V6_CERT),
        )
        .expect("verify");
        assert!(res.is_verified, "the RFC's own cleartext sample must verify");
        ta::exactly_one_valid(&res.signatures);
    }

    #[test]
    fn appendix_a7_inline_signed_message_verifies_end_to_end() {
        // Same message and signature as A.6, inline instead of cleartext.
        let res = verify_mode(
            &[],
            rfc9580::A7_INLINE_SIGNED.as_bytes(),
            GfrSignMode::Inline,
            Some(rfc9580::A3_V6_CERT),
        )
        .expect("verify");
        assert!(res.is_verified, "the RFC's own inline sample must verify");
    }

    #[test]
    fn appendix_a7_yields_the_documented_plaintext() {
        let res = verify_mode(
            &[],
            rfc9580::A7_INLINE_SIGNED.as_bytes(),
            GfrSignMode::Inline,
            Some(rfc9580::A3_V6_CERT),
        )
        .expect("verify");
        let text = String::from_utf8_lossy(&res.data);
        assert!(text.contains("grocery store"), "{text:?}");
        assert!(text.contains("noodles"), "{text:?}");
    }

    #[test]
    fn appendix_a6_reports_a_v6_signature() {
        // §10.3.2.2: a v6 key produces v6 signatures.
        let sigs = crate::crypto::parse_all_signature_packets(rfc9580::A6_CLEARTEXT.as_bytes());
        let sigs = if sigs.is_empty() {
            // The cleartext framework wraps the signature in armor.
            let (msg, _) = pgp::composed::CleartextSignedMessage::from_string(rfc9580::A6_CLEARTEXT)
                .expect("parse A.6");
            msg.signatures().iter().cloned().collect()
        } else {
            sigs
        };
        assert!(
            sigs.iter()
                .all(|s| s.version() == pgp::packet::SignatureVersion::V6)
        );
    }

    // -- round-trips against our own signer ------------------------------------

    #[test]
    fn a_detached_signature_we_produce_verifies() {
        let key = &keys::V4_SIGN;
        let signed = crate::crypto::sign_internal(
            0,
            "",
            corpus::PAYLOAD,
            &[&key.secret_armored],
            None,
            GfrSignMode::Detached,
            true,
        )
        .expect("sign");

        let res = verify_detached(corpus::PAYLOAD, &signed.data, Some(&key.public_armored));
        assert!(res.is_verified);
        ta::exactly_one_valid(&res.signatures);
    }

    #[test]
    fn a_cleartext_signature_we_produce_verifies() {
        let key = &keys::V4_SIGN;
        let signed = crate::crypto::sign_internal(
            0,
            "",
            b"a line of text\n",
            &[&key.secret_armored],
            None,
            GfrSignMode::ClearText,
            true,
        )
        .expect("sign");

        let res = verify_mode(&[], &signed.data, GfrSignMode::ClearText, Some(&key.public_armored))
            .expect("verify");
        assert!(res.is_verified);
    }

    #[test]
    fn an_inline_signature_we_produce_verifies() {
        let key = &keys::V4_SIGN;
        let signed = crate::crypto::sign_internal(
            0,
            "",
            corpus::PAYLOAD,
            &[&key.secret_armored],
            None,
            GfrSignMode::Inline,
            true,
        )
        .expect("sign");

        let res = verify_mode(&[], &signed.data, GfrSignMode::Inline, Some(&key.public_armored))
            .expect("verify");
        assert!(res.is_verified);
        assert_eq!(res.data, corpus::PAYLOAD);
    }

    #[test]
    fn a_v6_detached_signature_we_produce_verifies() {
        let key = &keys::V6_SIGN;
        let signed = crate::crypto::sign_internal(
            0,
            "",
            corpus::PAYLOAD,
            &[&key.secret_armored],
            None,
            GfrSignMode::Detached,
            true,
        )
        .expect("sign");

        let res = verify_detached(corpus::PAYLOAD, &signed.data, Some(&key.public_armored));
        assert!(res.is_verified);
    }

    #[test]
    fn a_signature_we_produce_does_not_verify_over_altered_data() {
        let key = &keys::V4_SIGN;
        let signed = crate::crypto::sign_internal(
            0,
            "",
            b"original message",
            &[&key.secret_armored],
            None,
            GfrSignMode::Detached,
            true,
        )
        .expect("sign");

        let res = verify_detached(b"tampered message", &signed.data, Some(&key.public_armored));
        assert!(!res.is_verified);
    }

    #[test]
    fn a_signature_from_a_revoked_subkey_is_not_valid() {
        // §5.2.1.12 / gap 4: a subkey of a revoked component is unusable.
        let key = &keys::V4_SIGN;
        let signed = crate::crypto::sign_internal(
            0,
            "",
            corpus::PAYLOAD,
            &[&key.secret_armored],
            None,
            GfrSignMode::Detached,
            true,
        )
        .expect("sign");

        // Verify against the copy of the certificate whose primary is revoked.
        let res = verify_detached(
            corpus::PAYLOAD,
            &signed.data,
            Some(&keys::V4_REVOKED_PRIMARY.public_armored),
        );
        assert!(
            !res.is_verified,
            "a subkey of a revoked primary must not verify (§5.2.1.11)"
        );
    }

    #[test]
    fn an_empty_payload_can_be_signed_and_verified() {
        let key = &keys::V4_SIGN;
        let signed = crate::crypto::sign_internal(
            0,
            "",
            b"",
            &[&key.secret_armored],
            None,
            GfrSignMode::Detached,
            true,
        );
        if let Ok(signed) = signed {
            let res = verify_detached(b"", &signed.data, Some(&key.public_armored));
            assert!(res.is_verified);
        }
    }

    #[test]
    fn a_large_payload_round_trips() {
        let key = &keys::V4_SIGN;
        let payload = vec![0x5Au8; 256 * 1024];
        let signed = crate::crypto::sign_internal(
            0,
            "",
            &payload,
            &[&key.secret_armored],
            None,
            GfrSignMode::Detached,
            true,
        )
        .expect("sign");
        let res = verify_detached(&payload, &signed.data, Some(&key.public_armored));
        assert!(res.is_verified);
    }

    #[test]
    fn a_binary_payload_with_nul_bytes_round_trips() {
        let key = &keys::V4_SIGN;
        let payload: Vec<u8> = (0..=255u8).cycle().take(4096).collect();
        let signed = crate::crypto::sign_internal(
            0,
            "",
            &payload,
            &[&key.secret_armored],
            None,
            GfrSignMode::Detached,
            true,
        )
        .expect("sign");
        let res = verify_detached(&payload, &signed.data, Some(&key.public_armored));
        assert!(res.is_verified);
    }

    #[test]
    fn a_signature_reports_a_strong_hash_algorithm() {
        // §9.5: the engine must not *produce* a weak-hash signature either.
        let key = &keys::V4_SIGN;
        let signed = crate::crypto::sign_internal(
            0,
            "",
            corpus::PAYLOAD,
            &[&key.secret_armored],
            None,
            GfrSignMode::Detached,
            true,
        )
        .expect("sign");
        for sig in &signed.signatures {
            assert!(
                !sig_hash_algo_is_weak(&sig.hash_algo),
                "produced a {} signature",
                sig.hash_algo
            );
        }
    }

    // -- documented gaps -------------------------------------------------------

    #[test]
    #[ignore = "DEFERRED: designated-revoker revocations (RFC 9580 §5.2.3.23) are not \
                honoured; only self-revocations are. This test encodes the desired \
                behaviour so it flips green when third-party revocation lands."]
    fn deferred_a_designated_revoker_revocation_is_honoured() {
        // Encodes the target behaviour so this flips green the day
        // third-party revocation lands. Run with:
        //   cargo test -- --ignored deferred_
        panic!("not implemented: third-party revocations are not honoured");
    }
}
