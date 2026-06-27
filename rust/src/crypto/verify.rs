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

    // Helper closure to update signature statuses uniformly
    let update_signatures = |cert: &SignedPublicKey,
                             is_cert_valid: bool,
                             signatures: &mut Vec<SignatureResultInternal>,
                             is_verified: &mut bool|
     -> bool {
        let mut found = false;
        for sig in signatures.iter_mut() {
            if cert_contains_issuer(cert, &sig.fpr) {
                found = true;
                if is_cert_valid {
                    sig.status = GfrSignatureStatus::Valid;
                    *is_verified = true;
                } else if sig.status == GfrSignatureStatus::NoKey {
                    // Only mark as bad if we haven't already validated it via another key
                    sig.status = GfrSignatureStatus::BadSignature;
                }
            }
        }
        found
    };

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

        // Try verifying with the primary key first
        let mut is_cert_valid = {
            let mut cancellable = crate::cancel::CancellableReader::new(channel, &mut data_stream);
            sig_msg.signature.verify(cert, &mut cancellable).is_ok()
        };

        // Fallback: If primary key fails, test subkeys (to bypass rpgp's strict identity matching)
        if !is_cert_valid {
            for subkey in &cert.public_subkeys {
                if crate::cancel::is_cancelled(channel) {
                    return Err(GfrStatus::ErrorCanceled);
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

        update_signatures(cert, is_cert_valid, &mut signatures, &mut is_verified);
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
    // Helper closure to update signature statuses uniformly across all modes.
    let update_signatures = |cert: &SignedPublicKey,
                             is_cert_valid: bool,
                             signatures: &mut Vec<SignatureResultInternal>,
                             is_verified: &mut bool|
     -> bool {
        let mut found = false;
        for sig in signatures.iter_mut() {
            if cert_contains_issuer(cert, &sig.fpr) {
                found = true;
                if is_cert_valid {
                    sig.status = GfrSignatureStatus::Valid;
                    *is_verified = true;
                } else if sig.status == GfrSignatureStatus::NoKey {
                    sig.status = GfrSignatureStatus::BadSignature;
                }
            }
        }
        found
    };

    match mode {
        // ---------------------------------------------------------
        // MODE 0: INLINE SIGNATURE
        // ---------------------------------------------------------
        GfrSignMode::Inline => {
            let mut msg = Message::from_armor(Cursor::new(data))
                .map(|(m, _)| m)
                .or_else(|_| Message::from_bytes(data))
                .into_gfr()?;

            let mut signatures = sniff_signatures(data, mode);
            let certs = fetch_certs_for_signatures(&signatures, fetch_pubkey_cb, user_data);
            let mut is_verified = false;

            // verify() only checks signature at index 0; iterate all indices for multi-signer messages.
            let num_sigs = if let Message::Signed { ref reader, .. } = msg {
                reader.num_signatures()
            } else {
                0
            };

            for cert in &certs {
                let is_cert_valid = (0..num_sigs).any(|i| {
                    msg.verify_nested_explicit(i, cert).is_ok()
                        || cert
                            .public_subkeys
                            .iter()
                            .any(|sk| msg.verify_nested_explicit(i, sk).is_ok())
                });

                let found =
                    update_signatures(cert, is_cert_valid, &mut signatures, &mut is_verified);

                // Special fallback if verification succeeded but issuer wasn't caught by sniff_signatures
                if is_cert_valid && !found {
                    signatures.push(SignatureResultInternal {
                        fpr: cert.primary_key.fingerprint().to_string(),
                        status: GfrSignatureStatus::Valid,
                        created_at: 0,
                        pub_algo: "None".to_string(),
                        hash_algo: "None".to_string(),
                        sig_type: mode,
                    });
                    is_verified = true;
                }
            }

            let clear_data = msg.as_data_vec().into_gfr()?;
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

            // Sniff directly from the parsed message
            let mut signatures = Vec::new();
            for sig in msg.signatures().iter() {
                for issuer in sig.issuer_fingerprint() {
                    let fpr = issuer.to_string();
                    if !signatures
                        .iter()
                        .any(|r: &SignatureResultInternal| r.fpr == fpr)
                    {
                        let (hash_algo, pub_algo) = sig
                            .config()
                            .map(|c| (c.hash_alg.to_string(), algo_to_string_simple(c.pub_alg)))
                            .unwrap_or_default();

                        signatures.push(SignatureResultInternal {
                            fpr,
                            status: GfrSignatureStatus::NoKey,
                            created_at: sig.created().map(|d| d.as_secs()).unwrap_or(0),
                            pub_algo,
                            hash_algo,
                            sig_type: mode,
                        });
                    }
                }
            }

            let certs = fetch_certs_for_signatures(&signatures, fetch_pubkey_cb, user_data);
            let mut is_verified = false;

            for cert in &certs {
                let is_cert_valid = msg.verify(cert).is_ok()
                    || cert.public_subkeys.iter().any(|sk| msg.verify(sk).is_ok());

                update_signatures(cert, is_cert_valid, &mut signatures, &mut is_verified);
            }

            let clear_data = msg
                .to_armored_bytes(ArmorOptions::default())
                .unwrap_or_default();
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

            let sig_msg = DetachedSignature::from_armor_single(Cursor::new(sig_data))
                .map(|(s, _)| s)
                .or_else(|_| DetachedSignature::from_bytes(sig_data))
                .into_gfr()?;

            let mut signatures = sniff_signatures(sig_data, mode);
            let certs = fetch_certs_for_signatures(&signatures, fetch_pubkey_cb, user_data);
            let mut is_verified = false;

            for cert in &certs {
                let is_cert_valid = sig_msg.verify(cert, data).is_ok()
                    || cert
                        .public_subkeys
                        .iter()
                        .any(|sk| sig_msg.verify(sk, data).is_ok());

                update_signatures(cert, is_cert_valid, &mut signatures, &mut is_verified);
            }

            Ok(VerifyResultInternal {
                data: Vec::new(),
                is_verified,
                signatures,
            })
        }
    }
}
