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
    mut data_stream: R,
    sig_data: &[u8],
    fetch_pubkey_cb: Option<GfrPublicKeyFetchCb>,
    free_cb: Option<GfrFreeCb>,
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
        .map_err(|_| GfrStatus::ErrorInvalidInput)?;

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
                if let Some(f_cb) = free_cb {
                    f_cb(c_key_block as *mut std::ffi::c_void, user_data);
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
    for cert in &certs {
        // Rewind the stream to the beginning before starting verification
        data_stream
            .seek(SeekFrom::Start(0))
            .map_err(|_| GfrStatus::ErrorInternal)?;

        // Try verifying with the primary key first
        let mut is_cert_valid = sig_msg.signature.verify(cert, &mut data_stream).is_ok();

        // Fallback: If primary key fails, test subkeys (to bypass rpgp's strict identity matching)
        if !is_cert_valid {
            for subkey in &cert.public_subkeys {
                // We MUST rewind the stream before each subsequent verification attempt!
                data_stream
                    .seek(SeekFrom::Start(0))
                    .map_err(|_| GfrStatus::ErrorInternal)?;

                if sig_msg.signature.verify(subkey, &mut data_stream).is_ok() {
                    is_cert_valid = true;
                    break;
                }
            }
        }

        update_signatures(cert, is_cert_valid, &mut signatures, &mut is_verified);
    }

    Ok(VerifyStreamResultInternal {
        is_verified,
        signatures,
    })
}
