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

pub(crate) use crate::{
    cache::{PASSWORD_CACHE, PasswordCachePolicy},
    crypto::{
        InvalidRecipientInternal, RecipientResultInternal, SelectedKey, SignatureResultInternal,
        algo_to_string_simple, cert_contains_issuer, parse_signer_block, sniff_signatures,
        with_signing_key,
    },
    err::{IntoGfrResult, set_last_error},
    tar::build_tar_tempfile_from_directory,
    types::{
        GfrFreeCb, GfrPasswordFetchCb, GfrPublicKeyFetchCb, GfrRecipientStatus,
        GfrSecretKeyFetchCb, GfrSignMode, GfrSignatureStatus, GfrStatus,
    },
    utils::{PassphraseStateInternal, fetch_password_with_cache},
};
pub(crate) use core::fmt;
pub(crate) use log::debug;
pub(crate) use pgp::{
    armor::Dearmor,
    composed::{
        ArmorOptions, CleartextSignedMessage, Deserializable, DetachedSignature, Esk, Message,
        MessageBuilder, SignedPublicKey, SignedSecretKey,
    },
    crypto::{hash::HashAlgorithm, sym::SymmetricKeyAlgorithm},
    packet::{Packet, PacketParser},
    ser::Serialize,
    types::{KeyDetails, Password, SecretParams, StringToKey},
};
pub(crate) use rand::thread_rng;
pub(crate) use std::{
    ffi::{CStr, CString, c_void},
    fs::File,
    io::{BufReader, Cursor, Read, Seek, SeekFrom, Write},
    path::Path,
};

mod decrypt;
mod encrypt;
mod sign;
mod verify;

pub use decrypt::*;
pub use encrypt::*;
pub use sign::*;
pub use verify::*;

pub struct EncryptStreamResultInternal {
    pub invalid_recipients: Vec<InvalidRecipientInternal>,
}

pub struct SignStreamResultInternal {
    pub signatures: Vec<SignatureResultInternal>,
}

pub struct VerifyStreamResultInternal {
    pub is_verified: bool,
    pub signatures: Vec<SignatureResultInternal>,
}

pub struct EncryptAndSignStreamResultInternal {
    pub signatures: Vec<SignatureResultInternal>,
    pub invalid_recipients: Vec<InvalidRecipientInternal>,
}

pub struct DecryptAndVerifyStreamResultInternal {
    pub filename: String,
    pub recipients: Vec<RecipientResultInternal>,
    pub is_verified: bool,
    pub signatures: Vec<SignatureResultInternal>,
}

pub struct SymmetricEncryptStreamResultInternal {}

pub(crate) fn create_output_file(out_file_path: &str) -> Result<File, GfrStatus> {
    File::create(out_file_path).map_err(|e| {
        log::error!("Failed to create output file: {}", e);
        set_last_error(&e.to_string());
        GfrStatus::ErrorIo
    })
}

pub(crate) type ParsedSigner = (SignedSecretKey, Option<String>);

pub(crate) fn parse_secret_signers(
    secret_key_blocks: &[&str],
) -> Result<Vec<ParsedSigner>, GfrStatus> {
    let mut parsed_keys = Vec::with_capacity(secret_key_blocks.len());

    for block in secret_key_blocks {
        let (target, armor_block) = parse_signer_block(block);
        let (skey, _) =
            SignedSecretKey::from_string(armor_block).map_err(|_| GfrStatus::ErrorInvalidInput)?;
        parsed_keys.push((skey, target));
    }

    Ok(parsed_keys)
}

// Helper to sniff all intended recipients from the encrypted data
pub fn sniff_recipients(data: &[u8]) -> Vec<RecipientResultInternal> {
    let mut results = Vec::new();
    let mut dearmored = Vec::new();
    let _ = Dearmor::new(Cursor::new(data)).read_to_end(&mut dearmored);
    let payload = if dearmored.is_empty() {
        data
    } else {
        &dearmored
    };

    let parser = PacketParser::new(Cursor::new(payload));
    for packet_result in parser {
        if let Ok(Packet::PublicKeyEncryptedSessionKey(pkesk)) = packet_result {
            if let Ok(id) = pkesk.id() {
                let algo = if let Ok(algo_id) = pkesk.algorithm() {
                    algo_to_string_simple(algo_id)
                } else {
                    String::new()
                };
                results.push(RecipientResultInternal {
                    key_id: id.to_string(),
                    pub_algo: algo,
                    status: GfrRecipientStatus::NoKey,
                });
            }
        }
    }
    results
}
