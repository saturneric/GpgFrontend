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

    // 1. Parse the detached signature file (Attempt armored first, fallback to raw bytes)
    let sig_msg = DetachedSignature::from_armor_single(Cursor::new(sig_data))
        .map(|(s, _)| s)
        .or_else(|_| DetachedSignature::from_bytes(sig_data))
        .record_err(GfrStatus::ErrorInvalidInput)?;

    // 2. Sniff signature metadata to know WHO signed it before fetching keys
    let mut signatures = sniff_signatures(sig_data, GfrSignMode::Detached);
    let mut is_verified = false;

    // 3. Dynamically Fetch Public Keys via C Callback based on sniffed fingerprints
    let mut certs = Vec::new();
    if let Some(cb) = fetch_pubkey_cb {
        for sig in &signatures {
            let c_fpr = std::ffi::CString::new(sig.fpr.clone()).unwrap_or_default();
            let c_key_block = cb(c_fpr.as_ptr(), user_data);

            if !c_key_block.is_null() {
                if let Ok(key_str) = unsafe { std::ffi::CStr::from_ptr(c_key_block) }.to_str() {
                    if let Ok((cert, _)) = SignedPublicKey::from_string(key_str) {
                        certs.push(cert);
                    }
                }

                unsafe {
                    gfc_secure_free_cstr(c_key_block);
                }
            }
        }
    }

    log::debug!(
        "Fetched and parsed {} public keys for detached stream verification",
        certs.len()
    );

    // 4. Stream Verification
    //
    // Hashing the data stream is the long-running part, so each pass reads
    // through a `CancellableReader` that aborts mid-hash once the user cancels.
    // `verify` only reports success/failure, so after the passes we consult the
    // cancel flag directly and surface `ErrorCanceled` rather than a spurious
    // "not verified" result.
    for cert in &certs {
        if crate::cancel::is_cancelled(channel) {
            return Err(GfrStatus::ErrorCanceled);
        }

        // Rewind the stream to the beginning before starting verification
        data_stream
            .seek(SeekFrom::Start(0))
            .record_err(GfrStatus::ErrorInternal)?;

        // Try verifying with the primary key first — but only if it is currently
        // usable: not revoked (RFC 9580 §5.2.1.11) and not expired (§5.2.3.13).
        // A signature under a revoked or expired key is not valid.
        let mut is_cert_valid = if !cert_primary_usable(cert) {
            false
        } else {
            let mut cancellable = crate::cancel::CancellableReader::new(channel, &mut data_stream);
            sig_msg.signature.verify(cert, &mut cancellable).is_ok()
        };

        // Fallback: if the primary key fails, test subkeys (to bypass rpgp's strict
        // identity matching). Only usable signing subkeys are attempted: a revoked
        // or encryption-only subkey MUST NOT yield a valid result (RFC 9580 §5.2.1.12).
        if !is_cert_valid {
            for subkey in &cert.public_subkeys {
                if crate::cancel::is_cancelled(channel) {
                    return Err(GfrStatus::ErrorCanceled);
                }
                if !subkey_usable_for_verify(cert, subkey) {
                    continue;
                }

                // We MUST rewind the stream before each subsequent verification attempt!
                data_stream
                    .seek(SeekFrom::Start(0))
                    .record_err(GfrStatus::ErrorInternal)?;

                let mut cancellable =
                    crate::cancel::CancellableReader::new(channel, &mut data_stream);
                if sig_msg.signature.verify(subkey, &mut cancellable).is_ok() {
                    is_cert_valid = true;
                    break;
                }
            }
        }

        finalize_signature_statuses(cert, is_cert_valid, &mut signatures, &mut is_verified);
    }

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

            let num_sigs = if let Message::Signed { ref reader, .. } = msg {
                reader.num_signatures()
            } else {
                0
            };

            // Build one entry per actual signature packet from the (possibly
            // decompressed) message. Deriving these from the parsed message rather
            // than sniffing the raw outer bytes is essential for a
            // compression-wrapped inline signature, whose signature packets are
            // invisible until the body is decompressed; it also lets
            // sig_entry_from_packet apply the issuer Key ID fallback (B1).
            let mut signatures = Vec::new();
            if let Message::Signed { ref reader, .. } = msg {
                for i in 0..num_sigs {
                    if let Some(sig) = reader.signature(i) {
                        signatures.push(sig_entry_from_packet(sig, mode));
                    }
                }
            }

            let certs = fetch_certs_for_signatures(&signatures, fetch_pubkey_cb, user_data);
            let mut is_verified = false;

            // verify() only checks signature at index 0; attribute each index to
            // the exact issuer that made it (per-index attribution).
            for cert in &certs {
                finalize_signature_statuses_by_index(
                    &msg,
                    num_sigs,
                    cert,
                    &mut signatures,
                    &mut is_verified,
                );
            }

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

            for (idx, sig) in sig_packets.iter().enumerate() {
                attribute_signature_per_index(
                    sig,
                    signed_text.as_bytes(),
                    &certs,
                    &mut signatures[idx],
                    &mut is_verified,
                );
            }

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

            for (idx, sig) in sig_packets.iter().enumerate() {
                attribute_signature_per_index(
                    sig,
                    data,
                    &certs,
                    &mut signatures[idx],
                    &mut is_verified,
                );
            }

            Ok(VerifyResultInternal {
                data: Vec::new(),
                is_verified,
                signatures,
            })
        }
    }
}

/// Attribute a single signature packet to its result entry.
///
/// Only certificates that actually carry this signature's issuer are considered,
/// so an unknown signer (no fetched cert) keeps its `NoKey` status rather than
/// being downgraded to `BadSignature`. When a matching cert exists, the packet is
/// verified against a usable key of that cert and the shared §9.5/§5.2.3.10 gates
/// are applied. Shared by the cleartext and detached in-memory verify paths.
fn attribute_signature_per_index(
    sig: &pgp::packet::Signature,
    data: &[u8],
    certs: &[SignedPublicKey],
    entry: &mut SignatureResultInternal,
    is_verified: &mut bool,
) {
    let matching: Vec<&SignedPublicKey> = certs
        .iter()
        .filter(|cert| cert_contains_issuer(cert, &entry.fpr))
        .collect();
    if matching.is_empty() {
        // No key for this issuer: leave the entry as NoKey (unknown signer).
        return;
    }

    let verified = matching
        .iter()
        .any(|cert| signature_verifies_under_usable_key(cert, sig, data));
    apply_signature_gate(entry, verified, is_verified);
}
