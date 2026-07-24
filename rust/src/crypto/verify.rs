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
