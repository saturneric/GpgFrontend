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

//! Key package v3: a set of OpenPGP keys in one always-encrypted file.
//!
//! ```text
//! "GFKPK3\n" | u32 BE header_len | header (CBOR)
//! nonce (24) | XChaCha20-Poly1305(data_key, search tree)
//! part*: nonce (24) | XChaCha20-Poly1305(data_key, metadata or secret key)
//! ```
//!
//! The header is plaintext CBOR (RFC 8949) so a reader knows how to unlock the
//! package, how many keys it holds and how large the index is. It is the
//! index's associated data, so changing it breaks the index. It names no key.
//!
//! The index is a balanced binary search tree of fixed-size [`NODE_LEN`] nodes
//! in Eytzinger order (the root first, the children of node `i` at `2i+1` and
//! `2i+2`), read as it is once decrypted. A node maps one binary identifier to
//! its key: the fingerprint and key ID of the primary key and of every subkey,
//! and the SHA-256 of every e-mail address in a user ID. A lookup walks the
//! tree in `O(log n)` comparisons. A node points at its key's two parts, and
//! carries what an operation decides on before it reads one (see
//! [`Component`]): the component's capabilities, times, algorithm, and whether
//! it is revoked or its secret has a passphrase.
//!
//! Each key has a metadata part (identifiers, user IDs, digest and the binary
//! public certificate, CBOR) and, when the package carries it, a secret part
//! (the binary secret key). Each is sealed on its own and bound to the
//! package, its key and its kind, so a reader decrypts the index and then
//! only the part it needs: verifying or encrypting never decrypts a secret
//! key. It reads nothing else of the file.
//!
//! The random `data_key` is wrapped by one of two protections:
//! - [`Protection::PinFile`]: by a PIN through Argon2id; the wrapped key lives in
//!   a separate key file, as the version 2 package did;
//! - [`Protection::Plain`]: by a random secret handed to the user as text
//!   ([`plain_key_text`]): in a `.gfkey` file beside gfpipe, an environment
//!   variable or a CI secret.
//!
//! The random secret has full entropy, so it goes through keyed BLAKE2b rather
//! than a password hash, and opening such a package costs no KDF time.
//!
//! XChaCha20's 192-bit nonce makes random nonces safe however many parts one
//! data key seals.
//!
//! # Changing the format
//!
//! A package names the version that wrote it (`version`) and the oldest
//! reader that can read it correctly (`min_reader`). Readers check
//! `min_reader` before anything else, and refuse a package they are too old
//! for with [`GfrStatus::ErrorUnsupportedFeature`] rather than misread it.
//!
//! What an older reader can safely ignore keeps `min_reader`:
//! - a new field in any CBOR map (header, key file, metadata): readers ignore
//!   fields they do not know, so new fields must have a default;
//! - more bytes per tree node: the header's `node_len` gives the node size,
//!   and a reader reads the first [`NODE_LEN`] bytes it knows of each;
//! - a new node kind (a new kind of identifier to find keys by): it never
//!   matches a lookup of a kind the reader knows;
//! - a new flag bit, or a new use of the reserved node bytes, whose absence
//!   (zero) means what older packages meant.
//!
//! What an older reader would get wrong raises `min_reader`: a new protection,
//! cipher or key derivation, a field or bit changing meaning, a part sealed
//! differently. Writers write zero into reserved bytes; readers ignore them.

use std::cmp::Ordering;

use chacha20poly1305::aead::{Aead, KeyInit, Payload};
use chacha20poly1305::{XChaCha20Poly1305, XNonce};
use pgp::composed::{Deserializable, SignedPublicKey, SignedSecretKey};
use pgp::ser::Serialize as PgpSerialize;
use pgp::types::KeyDetails;
use rand::RngCore;
use serde::{Deserialize, Serialize};
use sha2::{Digest, Sha256};
use zeroize::Zeroizing;

use crate::err::set_last_error;
use crate::types::GfrStatus;

/// The first bytes of every version 3 package.
pub const MAGIC: &[u8] = b"GFKPK3\n";
/// The first bytes of the separate key file of a PIN-protected package.
pub const KEY_FILE_MAGIC: &[u8] = b"GFKPK3KEY\n";
/// The prefix of a plain key as the user sees it.
pub const PLAIN_KEY_PREFIX: &str = "gfkp3:";
/// Length of a package ID.
pub const PACKAGE_ID_LEN: usize = 16;

/// One tree node, all integers big-endian:
///
/// | at | size | field                                            |
/// |----|------|--------------------------------------------------|
/// |  0 |  1   | kind: fingerprint, key ID or e-mail digest        |
/// |  1 |  1   | identifier length                                |
/// |  2 |  1   | flags                                            |
/// |  3 |  1   | capabilities of the component                    |
/// |  4 | 32   | identifier, zero-padded                          |
/// | 36 |  2   | component: 0 the primary, `i` the `i`th subkey   |
/// | 38 |  4   | component creation time                          |
/// | 42 |  4   | component expiry time, 0 for never               |
/// | 46 |  1   | component algorithm (RFC 9580 §9.1)              |
/// | 47 |  1   | reserved, zero                                   |
/// | 48 |  4   | metadata part offset, from the first part        |
/// | 52 |  4   | metadata part length                             |
/// | 56 |  4   | secret part offset                               |
/// | 60 |  4   | secret part length, 0 without a secret key       |
///
/// Every node of one key points at the same parts; the metadata offset is
/// the key's identity and gives the order the keys were written in.
pub const NODE_LEN: usize = 64;
const NODE_ID_AT: usize = 4;
const NODE_ID_MAX: usize = 32;
const NODE_COMPONENT_AT: usize = 36;
const NODE_CREATED_AT: usize = 38;
const NODE_EXPIRES_AT: usize = 42;
const NODE_ALGO_AT: usize = 46;
const NODE_PARTS_AT: usize = 48;

const KIND_FPR: u8 = 1;
const KIND_KEY_ID: u8 = 2;
const KIND_EMAIL: u8 = 3;
/// The identifier names a subkey, not the primary.
const FLAG_SUBKEY: u8 = 1;
/// The key's block carries its secret key.
const FLAG_SECRET: u8 = 2;
/// The component's secret has a passphrase of its own.
const FLAG_PROTECTED: u8 = 4;
/// The component is revoked.
const FLAG_REVOKED: u8 = 8;

pub const CAP_SIGN: u8 = 1;
pub const CAP_ENCRYPT: u8 = 2;
pub const CAP_CERTIFY: u8 = 4;
pub const CAP_AUTH: u8 = 8;

/// The format this reader and writer speak.
pub const FORMAT_VERSION: u32 = 3;
/// A writer that adds fields an older reader can ignore leaves this alone.
const MIN_READER: u32 = 3;
const MAX_HEADER_BYTES: usize = 4 * 1024;
const MAX_KEY_FILE_BYTES: usize = 4 * 1024;
const MAX_NODES: usize = 1 << 20;
const MAX_NODE_LEN: usize = 1024;
const MAX_PART_BYTES: usize = 64 * 1024 * 1024;
const KEY_LEN: usize = 32;
const NONCE_LEN: usize = 24;
const TAG_LEN: usize = 16;
const SALT_LEN: usize = 16;
const WRAP_DOMAIN: &[u8] = b"GpgFrontend.KeyPackage.v3.wrap";
const PART_DOMAIN: &[u8] = b"GpgFrontend.KeyPackage.v3.part";

/// OpenPGP packet tags of the two primary key packets (RFC 9580 §5).
const TAG_SECRET_KEY: u8 = 5;
const TAG_PUBLIC_KEY: u8 = 6;

/// Argon2id strength for the PIN, the same work the version 2 package asks.
const PIN_M_KIB: u32 = 64 * 1024;
const PIN_T: u32 = 3;
const PIN_P: u32 = 1;

// ---------------------------------------------------------------- the format

/// The plaintext header.
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct Header {
    pub version: u32,
    pub min_reader: u32,
    /// Random; binds a PIN key file and every part to its package.
    #[serde(with = "serde_bytes")]
    pub package_id: Vec<u8>,
    /// Unix seconds.
    pub generated_at: u64,
    pub key_count: u32,
    /// Nodes in the search tree.
    pub node_count: u32,
    /// Bytes per tree node; at least [`NODE_LEN`]. A newer writer may append
    /// fields to the nodes, which a reader skips. Absent means [`NODE_LEN`].
    #[serde(default = "default_node_len")]
    pub node_len: u32,
    /// True when any secret key component in the package has no passphrase of
    /// its own, so the package's protection is all that guards it.
    pub secrets_unprotected: bool,
    pub protection: Protection,
}

fn default_node_len() -> u32 {
    NODE_LEN as u32
}

/// How the data key is wrapped.
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub enum Protection {
    /// Wrapped by a PIN; the wrapped key is in the separate key file.
    PinFile,
    /// Wrapped by a secret the user holds as text.
    Plain(Wrap),
}

impl Protection {
    /// The name used in the wrap's associated data.
    fn kind(&self) -> &'static str {
        match self {
            Protection::PinFile => "pin_file",
            Protection::Plain(_) => "plain",
        }
    }
}

/// A wrapped data key.
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct Wrap {
    #[serde(with = "serde_bytes")]
    pub salt: Vec<u8>,
    #[serde(with = "serde_bytes")]
    pub nonce: Vec<u8>,
    #[serde(with = "serde_bytes")]
    pub wrapped_key: Vec<u8>,
}

/// What every versioned CBOR map starts with, read before the rest so a
/// file from a newer version is refused as such, however the rest changed.
#[derive(Deserialize)]
struct Versions {
    version: u32,
    min_reader: u32,
}

/// Decode `raw` after checking that this reader may read it.
fn from_versioned_cbor<T: serde::de::DeserializeOwned>(
    raw: &[u8],
    what: &str,
) -> Result<T, GfrStatus> {
    let versions: Versions = from_cbor(raw, what)?;
    if versions.min_reader > FORMAT_VERSION {
        set_last_error(&format!(
            "{what} was written by a newer version (format {}) and cannot be read here; \
             update GpgFrontend",
            versions.version
        ));
        return Err(GfrStatus::ErrorUnsupportedFeature);
    }
    from_cbor(raw, what)
}

/// The separate key file of a PIN-protected package, after its magic.
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
struct KeyFile {
    version: u32,
    min_reader: u32,
    #[serde(with = "serde_bytes")]
    package_id: Vec<u8>,
    m_kib: u32,
    t: u32,
    p: u32,
    wrap: Wrap,
}

/// One subkey's identifiers.
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct SubkeyEntry {
    #[serde(with = "serde_bytes")]
    pub fingerprint: Vec<u8>,
    #[serde(with = "serde_bytes")]
    pub key_id: Vec<u8>,
}

/// The public part of a key's block.
#[derive(Clone, Serialize, Deserialize)]
pub struct KeyEntry {
    /// Primary fingerprint.
    #[serde(with = "serde_bytes")]
    pub fingerprint: Vec<u8>,
    /// Primary 64-bit key ID.
    #[serde(with = "serde_bytes")]
    pub key_id: Vec<u8>,
    pub user_ids: Vec<String>,
    pub subkeys: Vec<SubkeyEntry>,
    /// SHA-256 of the binary public certificate: tells a reader whether the
    /// source key changed since the package was written.
    #[serde(with = "serde_bytes")]
    pub digest: Vec<u8>,
    pub has_secret: bool,
    /// The binary public certificate.
    #[serde(with = "serde_bytes")]
    pub public_key: Vec<u8>,
    /// Where the key's parts lie; not stored.
    #[serde(skip)]
    parts: Parts,
}

impl std::fmt::Debug for KeyEntry {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("KeyEntry")
            .field("fingerprint", &self.fpr_hex())
            .field("user_ids", &self.user_ids)
            .field("has_secret", &self.has_secret)
            .finish()
    }
}

impl KeyEntry {
    pub fn has_secret(&self) -> bool {
        self.has_secret
    }

    /// The first user ID, or an empty string.
    pub fn primary_user_id(&self) -> &str {
        self.user_ids.first().map(String::as_str).unwrap_or("")
    }

    /// The primary fingerprint, upper-case hex.
    pub fn fpr_hex(&self) -> String {
        hex::encode_upper(&self.fingerprint)
    }

    /// The primary key ID, upper-case hex.
    pub fn key_id_hex(&self) -> String {
        hex::encode_upper(&self.key_id)
    }

    /// The upper-case hex fingerprint of the component `id` names (the
    /// primary or a subkey, by fingerprint or key ID).
    pub fn component_fpr_hex(&self, id: &[u8]) -> String {
        self.subkeys
            .iter()
            .find(|s| s.fingerprint == id || s.key_id == id)
            .map_or_else(|| self.fpr_hex(), |s| hex::encode_upper(&s.fingerprint))
    }

    /// The upper-case hex key ID of the component `id` names.
    pub fn component_key_id_hex(&self, id: &[u8]) -> String {
        self.subkeys
            .iter()
            .find(|s| s.fingerprint == id || s.key_id == id)
            .map_or_else(|| self.key_id_hex(), |s| hex::encode_upper(&s.key_id))
    }
}

/// What a sealed part holds; part of its associated data.
#[derive(Clone, Copy)]
enum PartKind {
    Meta,
    Secret,
}

impl PartKind {
    fn name(self) -> &'static [u8] {
        match self {
            PartKind::Meta => b"meta",
            PartKind::Secret => b"secret",
        }
    }
}

fn to_cbor<T: Serialize>(value: &T) -> Result<Vec<u8>, GfrStatus> {
    let mut out = Vec::new();
    ciborium::into_writer(value, &mut out).map_err(|_| GfrStatus::ErrorInternal)?;
    Ok(out)
}

fn from_cbor<T: serde::de::DeserializeOwned>(bytes: &[u8], what: &str) -> Result<T, GfrStatus> {
    ciborium::from_reader(bytes).map_err(|e| {
        set_last_error(&format!("{what} is damaged: {e}"));
        GfrStatus::ErrorDataCorrupted
    })
}

// --------------------------------------------------------------------- index

/// The search key of a node: kind, then the identifier's bytes.
fn node_key(node: &[u8]) -> (u8, &[u8]) {
    let len = usize::from(node[1]).min(NODE_ID_MAX);
    (node[0], &node[NODE_ID_AT..NODE_ID_AT + len])
}

fn be_u32(node: &[u8], at: usize) -> u32 {
    u32::from_be_bytes([node[at], node[at + 1], node[at + 2], node[at + 3]])
}

/// Where one key's two parts lie, counted from the first part.
#[derive(Debug, Clone, Copy, Default, PartialEq, Eq, PartialOrd, Ord)]
struct Parts {
    meta_at: u32,
    meta_len: u32,
    secret_at: u32,
    /// 0 without a secret key.
    secret_len: u32,
}

/// What the index says about one component (the primary key or a subkey),
/// read from its node without touching any part.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Component {
    /// 0 for the primary, `i` for the `i`th subkey of [`KeyEntry::subkeys`]
    /// (counted from 1).
    pub index: u16,
    /// The key carries its secret key.
    pub has_secret: bool,
    /// This component's secret has a passphrase of its own.
    pub protected: bool,
    pub revoked: bool,
    /// `CAP_*` bits.
    pub caps: u8,
    pub created_at: u32,
    /// `0` means never.
    pub expires_at: u32,
    /// RFC 9580 §9.1 public key algorithm ID.
    pub algo: u8,
    parts: Parts,
}

impl Component {
    fn decode(node: &[u8]) -> Self {
        Component {
            index: u16::from_be_bytes([node[NODE_COMPONENT_AT], node[NODE_COMPONENT_AT + 1]]),
            has_secret: node[2] & FLAG_SECRET != 0,
            protected: node[2] & FLAG_PROTECTED != 0,
            revoked: node[2] & FLAG_REVOKED != 0,
            caps: node[3],
            created_at: be_u32(node, NODE_CREATED_AT),
            expires_at: be_u32(node, NODE_EXPIRES_AT),
            algo: node[NODE_ALGO_AT],
            parts: Parts {
                meta_at: be_u32(node, NODE_PARTS_AT),
                meta_len: be_u32(node, NODE_PARTS_AT + 4),
                secret_at: be_u32(node, NODE_PARTS_AT + 8),
                secret_len: be_u32(node, NODE_PARTS_AT + 12),
            },
        }
    }

    fn encode(&self, kind: u8, id: &[u8], out: &mut [u8]) {
        let flags = [
            (self.subkey(), FLAG_SUBKEY),
            (self.has_secret, FLAG_SECRET),
            (self.protected, FLAG_PROTECTED),
            (self.revoked, FLAG_REVOKED),
        ]
        .iter()
        .filter(|(on, _)| *on)
        .fold(0, |acc, (_, bit)| acc | bit);
        out.fill(0);
        out[0] = kind;
        out[1] = id.len() as u8;
        out[2] = flags;
        out[3] = self.caps;
        out[NODE_ID_AT..NODE_ID_AT + id.len()].copy_from_slice(id);
        out[NODE_COMPONENT_AT..NODE_COMPONENT_AT + 2].copy_from_slice(&self.index.to_be_bytes());
        out[NODE_CREATED_AT..NODE_CREATED_AT + 4].copy_from_slice(&self.created_at.to_be_bytes());
        out[NODE_EXPIRES_AT..NODE_EXPIRES_AT + 4].copy_from_slice(&self.expires_at.to_be_bytes());
        out[NODE_ALGO_AT] = self.algo;
        let p = &self.parts;
        for (i, v) in [p.meta_at, p.meta_len, p.secret_at, p.secret_len]
            .iter()
            .enumerate()
        {
            let at = NODE_PARTS_AT + 4 * i;
            out[at..at + 4].copy_from_slice(&v.to_be_bytes());
        }
    }

    /// The component is a subkey, not the primary.
    pub fn subkey(&self) -> bool {
        self.index != 0
    }

    /// Whether `other` is a component of the same key.
    pub fn same_key(&self, other: &Component) -> bool {
        self.parts.meta_at == other.parts.meta_at
    }

    pub fn can(&self, cap: u8) -> bool {
        self.caps & cap != 0
    }

    /// Whether the component has expired at `now` (Unix seconds).
    pub fn expired_at(&self, now: u64) -> bool {
        self.expires_at != 0 && u64::from(self.expires_at) <= now
    }
}

/// One node before it is placed in the tree.
struct NodeInput {
    kind: u8,
    id: Vec<u8>,
    component: Component,
}

/// Lay `nodes` out as a balanced search tree in Eytzinger order. Equal
/// identifiers (an e-mail address on two keys) keep the order of their keys,
/// so a lookup finds the first.
fn build_tree(mut nodes: Vec<NodeInput>, node_len: usize) -> Vec<u8> {
    nodes.sort_by(|a, b| {
        (a.kind, a.id.as_slice(), a.component.parts).cmp(&(
            b.kind,
            b.id.as_slice(),
            b.component.parts,
        ))
    });
    let mut tree = vec![0u8; nodes.len() * node_len];
    let mut next = 0;
    fill_tree(&nodes, &mut tree, node_len, 1, &mut next);
    tree
}

/// In-order walk of the implicit tree (1-based position `at`) placing the
/// sorted nodes.
fn fill_tree(sorted: &[NodeInput], tree: &mut [u8], node_len: usize, at: usize, next: &mut usize) {
    if at > sorted.len() {
        return;
    }
    fill_tree(sorted, tree, node_len, 2 * at, next);
    let node = &sorted[*next];
    let start = (at - 1) * node_len;
    node.component
        .encode(node.kind, &node.id, &mut tree[start..start + NODE_LEN]);
    *next += 1;
    fill_tree(sorted, tree, node_len, 2 * at + 1, next);
}

/// The first node whose key equals (`kind`, `id`), by a lower-bound descent
/// of the Eytzinger tree of `node_len`-byte nodes; the [`NODE_LEN`] bytes of
/// it this reader knows.
fn tree_find<'t>(tree: &'t [u8], node_len: usize, kind: u8, id: &[u8]) -> Option<&'t [u8]> {
    let n = tree.len() / node_len;
    let node = |at: usize| &tree[(at - 1) * node_len..(at - 1) * node_len + NODE_LEN];
    let mut at = 1usize;
    while at <= n {
        let less = node_key(node(at)).cmp(&(kind, id)) == Ordering::Less;
        at = 2 * at + usize::from(less);
    }
    // Undo the right turns taken after the last left turn: what remains is
    // the lower bound, or 0 when every node is less.
    at >>= at.trailing_ones() + 1;
    (at != 0)
        .then(|| node(at))
        .filter(|found| node_key(found) == (kind, id))
}

/// The SHA-256 an e-mail address is indexed by.
fn email_id(email: &str) -> Vec<u8> {
    Sha256::digest(email.as_bytes()).to_vec()
}

// ------------------------------------------------------------------- writing

/// How the writer should protect the package.
pub enum ProtectionRequest {
    Pin(Zeroizing<Vec<u8>>),
    Plain,
}

/// What [`write_package`] produced.
pub struct WrittenPackage {
    pub package: Vec<u8>,
    /// The separate key file of a PIN-protected package.
    pub key_file: Option<Vec<u8>>,
    /// The random wrap secret of a plain package, for the user as text.
    pub secret: Option<Zeroizing<Vec<u8>>>,
    pub header: Header,
}

/// A key read from its input, before it is sealed.
struct Prepared {
    entry: KeyEntry,
    secret: Option<Zeroizing<Vec<u8>>>,
    unprotected: bool,
    /// The primary first, then the subkeys in the order of `entry.subkeys`;
    /// key number and offset are filled in when the key is placed.
    components: Vec<Component>,
}

/// Write a package holding `keys`, each one binary OpenPGP key: a secret key
/// puts that key's secret into the package, a public one only its
/// certificate.
pub fn write_package(
    keys: &[&[u8]],
    request: ProtectionRequest,
    generated_at: u64,
) -> Result<WrittenPackage, GfrStatus> {
    write_package_with(keys, request, generated_at, NODE_LEN)
}

/// [`write_package`] with nodes of `node_len` bytes, as a newer writer that
/// appended node fields would write them.
fn write_package_with(
    keys: &[&[u8]],
    request: ProtectionRequest,
    generated_at: u64,
    node_len: usize,
) -> Result<WrittenPackage, GfrStatus> {
    if keys.is_empty() {
        return fail(
            GfrStatus::ErrorInvalidInput,
            "a key package needs at least one key",
        );
    }

    let mut prepared: Vec<Prepared> = Vec::with_capacity(keys.len());
    for input in keys {
        let key = prepare(input)?;
        if prepared
            .iter()
            .any(|p| p.entry.fingerprint == key.entry.fingerprint)
        {
            return fail(
                GfrStatus::ErrorInvalidInput,
                &format!("key {} is given twice", key.entry.fpr_hex()),
            );
        }
        prepared.push(key);
    }
    let secrets_unprotected = prepared.iter().any(|p| p.unprotected);
    let key_count = u32::try_from(prepared.len()).map_err(|_| GfrStatus::ErrorInvalidInput)?;

    let package_id = random_bytes::<PACKAGE_ID_LEN>().to_vec();
    let data_key = Zeroizing::new(random_bytes::<KEY_LEN>());

    let mut parts = Vec::new();
    let mut nodes = Vec::new();
    for key in &prepared {
        let entry = &key.entry;
        let meta = to_cbor(entry)?;
        let meta_at = part_offset(&parts)?;
        let meta_len = push_part(
            &mut parts,
            &data_key,
            &package_id,
            meta_at,
            PartKind::Meta,
            &meta,
        )?;
        let (secret_at, secret_len) = match key.secret.as_ref() {
            Some(secret) => {
                let at = part_offset(&parts)?;
                let len = push_part(
                    &mut parts,
                    &data_key,
                    &package_id,
                    meta_at,
                    PartKind::Secret,
                    secret,
                )?;
                (at, len)
            }
            None => (0, 0),
        };
        let placed = |c: &Component| Component {
            parts: Parts {
                meta_at,
                meta_len,
                secret_at,
                secret_len,
            },
            ..*c
        };

        let mut node = |kind, id: &[u8], component: Component| {
            nodes.push(NodeInput {
                kind,
                id: id.to_vec(),
                component,
            })
        };
        let primary = placed(&key.components[0]);
        node(KIND_FPR, &entry.fingerprint, primary);
        node(KIND_KEY_ID, &entry.key_id, primary);
        for (sub, component) in entry.subkeys.iter().zip(&key.components[1..]) {
            node(KIND_FPR, &sub.fingerprint, placed(component));
            node(KIND_KEY_ID, &sub.key_id, placed(component));
        }
        let mut emails: Vec<String> = entry.user_ids.iter().filter_map(|u| email_of(u)).collect();
        emails.sort();
        emails.dedup();
        for email in emails {
            node(KIND_EMAIL, &email_id(&email), primary);
        }
    }
    part_offset(&parts)?;
    if nodes.len() > MAX_NODES || nodes.iter().any(|n| n.id.len() > NODE_ID_MAX) {
        return fail(
            GfrStatus::ErrorInvalidInput,
            "the keys are too many for one key package",
        );
    }
    let node_count = nodes.len() as u32;
    let index = build_tree(nodes, node_len);

    let mut key_file = None;
    let mut secret = None;
    let protection = match request {
        ProtectionRequest::Pin(pin) => {
            if pin.is_empty() {
                return fail(GfrStatus::ErrorInvalidInput, "the PIN is empty");
            }
            let salt = random_bytes::<SALT_LEN>();
            let wrap_key = pin_wrap_key(&pin, &salt, PIN_M_KIB, PIN_T, PIN_P)?;
            let wrap = wrap_data_key(&wrap_key, &salt, &data_key, &package_id, "pin_file")?;
            let file = KeyFile {
                version: FORMAT_VERSION,
                min_reader: MIN_READER,
                package_id: package_id.clone(),
                m_kib: PIN_M_KIB,
                t: PIN_T,
                p: PIN_P,
                wrap,
            };
            let mut bytes = KEY_FILE_MAGIC.to_vec();
            bytes.extend(to_cbor(&file)?);
            key_file = Some(bytes);
            Protection::PinFile
        }
        ProtectionRequest::Plain => {
            let raw = Zeroizing::new(random_bytes::<KEY_LEN>().to_vec());
            let salt = random_bytes::<SALT_LEN>();
            let wrap_key = secret_wrap_key(&raw, &salt)?;
            let wrap = wrap_data_key(&wrap_key, &salt, &data_key, &package_id, "plain")?;
            secret = Some(raw);
            Protection::Plain(wrap)
        }
    };

    let header = Header {
        version: FORMAT_VERSION,
        min_reader: MIN_READER,
        package_id,
        generated_at,
        key_count,
        node_count,
        node_len: node_len as u32,
        secrets_unprotected,
        protection,
    };
    let header_bytes = to_cbor(&header)?;
    if header_bytes.len() > MAX_HEADER_BYTES {
        return fail(
            GfrStatus::ErrorInternal,
            "the key package header is too large",
        );
    }

    let mut package = MAGIC.to_vec();
    package.extend((header_bytes.len() as u32).to_be_bytes());
    package.extend(&header_bytes);
    let nonce = random_bytes::<NONCE_LEN>();
    let sealed = seal(&data_key, &nonce, &index, &package)?;
    package.extend(nonce);
    package.extend(sealed);
    package.extend(parts);

    Ok(WrittenPackage {
        package,
        key_file,
        secret,
        header,
    })
}

/// Where the next part starts; parts are addressed by 32-bit offsets.
fn part_offset(parts: &[u8]) -> Result<u32, GfrStatus> {
    u32::try_from(parts.len()).or_else(|_| {
        fail(
            GfrStatus::ErrorInvalidInput,
            "the keys are too large for one key package",
        )
    })
}

/// Append `nonce | sealed` to `parts`, bound to the key whose metadata
/// starts at `key`; its length.
fn push_part(
    parts: &mut Vec<u8>,
    data_key: &[u8; KEY_LEN],
    package_id: &[u8],
    key: u32,
    kind: PartKind,
    plaintext: &[u8],
) -> Result<u32, GfrStatus> {
    let nonce = random_bytes::<NONCE_LEN>();
    let sealed = seal(
        data_key,
        &nonce,
        plaintext,
        &part_aad(package_id, key, kind),
    )?;
    let len = NONCE_LEN + sealed.len();
    if len > MAX_PART_BYTES {
        return fail(GfrStatus::ErrorInvalidInput, "a key is too large");
    }
    parts.extend(nonce);
    parts.extend(sealed);
    Ok(len as u32)
}

/// The wrap secret as the user sees it: `gfkp3:` and 64 hex digits.
pub fn plain_key_text(secret: &[u8]) -> Zeroizing<String> {
    Zeroizing::new(format!("{PLAIN_KEY_PREFIX}{}", hex::encode(secret)))
}

/// Parse [`plain_key_text`]; surrounding whitespace is ignored.
pub fn parse_plain_key(text: &str) -> Result<Zeroizing<Vec<u8>>, GfrStatus> {
    let trimmed = text.trim();
    let digits = trimmed.strip_prefix(PLAIN_KEY_PREFIX).unwrap_or(trimmed);
    let bytes = Zeroizing::new(hex::decode(digits).map_err(|_| bad_plain_key())?);
    if bytes.len() != KEY_LEN {
        return Err(bad_plain_key());
    }
    Ok(bytes)
}

fn bad_plain_key() -> GfrStatus {
    set_last_error("the key is not a key package key (gfkp3: followed by 64 hex digits)");
    GfrStatus::ErrorInvalidInput
}

/// The tag of the first OpenPGP packet in `data` (RFC 9580 §4.2).
pub(crate) fn first_packet_tag(data: &[u8]) -> Option<u8> {
    let b = *data.first()?;
    if b & 0x80 == 0 {
        None
    } else if b & 0x40 != 0 {
        Some(b & 0x3f)
    } else {
        Some((b >> 2) & 0x0f)
    }
}

/// Whether the secret key carries any component without a passphrase.
pub fn has_unprotected_secret(key: &SignedSecretKey) -> bool {
    !key.primary_key.secret_params().is_encrypted()
        || key
            .secret_subkeys
            .iter()
            .any(|s| !s.key.secret_params().is_encrypted())
}

fn prepare(input: &[u8]) -> Result<Prepared, GfrStatus> {
    // Per component, primary first: whether its secret has a passphrase.
    let (public, secret, unprotected, protected) = match first_packet_tag(input) {
        Some(TAG_SECRET_KEY) => {
            let secret = SignedSecretKey::from_bytes(input).map_err(|e| {
                set_last_error(&format!("cannot read a secret key: {e}"));
                GfrStatus::ErrorInvalidData
            })?;
            let unprotected = has_unprotected_secret(&secret);
            let protected: Vec<(Vec<u8>, bool)> = std::iter::once((
                secret.primary_key.fingerprint().as_bytes().to_vec(),
                secret.primary_key.secret_params().is_encrypted(),
            ))
            .chain(secret.secret_subkeys.iter().map(|s| {
                (
                    s.key.fingerprint().as_bytes().to_vec(),
                    s.key.secret_params().is_encrypted(),
                )
            }))
            .collect();
            let bytes = Zeroizing::new(secret.to_bytes().map_err(|_| GfrStatus::ErrorInternal)?);
            (
                SignedPublicKey::from(secret),
                Some(bytes),
                unprotected,
                protected,
            )
        }
        Some(TAG_PUBLIC_KEY) => {
            let public = SignedPublicKey::from_bytes(input).map_err(|e| {
                set_last_error(&format!("cannot read a public key: {e}"));
                GfrStatus::ErrorInvalidData
            })?;
            (public, None, false, Vec::new())
        }
        _ => {
            return fail(
                GfrStatus::ErrorInvalidData,
                "a key package takes binary OpenPGP keys",
            );
        }
    };

    let has_secret = secret.is_some();
    let is_protected = |fpr: &[u8]| protected.iter().any(|(f, p)| f == fpr && *p);
    let meta = crate::key::build_public_metadata(&public);
    let caps = |sign: bool, encrypt: bool, certify: bool, auth: bool| {
        [
            (sign, CAP_SIGN),
            (encrypt, CAP_ENCRYPT),
            (certify, CAP_CERTIFY),
            (auth, CAP_AUTH),
        ]
        .iter()
        .filter(|(on, _)| *on)
        .fold(0, |acc, (_, bit)| acc | bit)
    };
    let primary_fpr = public.primary_key.fingerprint().as_bytes().to_vec();
    let mut components = vec![Component {
        index: 0,
        has_secret,
        protected: is_protected(&primary_fpr),
        revoked: meta.is_revoked,
        caps: caps(
            meta.can_sign,
            meta.can_encrypt,
            meta.can_certify,
            meta.can_auth,
        ),
        created_at: meta.created_at,
        expires_at: meta.expires_at,
        algo: u8::from(public.primary_key.algorithm()),
        parts: Parts::default(),
    }];
    for (i, sub) in public.public_subkeys.iter().enumerate() {
        let fpr = sub.key.fingerprint().as_bytes().to_vec();
        let hex_fpr = hex::encode_upper(&fpr);
        // The metadata lists bound subkeys only; an unbound one can do nothing.
        let info = meta
            .subkeys
            .iter()
            .find(|m| m.fpr.eq_ignore_ascii_case(&hex_fpr));
        components.push(Component {
            index: u16::try_from(i + 1).map_err(|_| GfrStatus::ErrorInvalidInput)?,
            has_secret,
            protected: is_protected(&fpr),
            revoked: info.is_none_or(|m| m.is_revoked),
            caps: info.map_or(0, |m| {
                caps(m.can_sign, m.can_encrypt, m.can_certify, m.can_auth)
            }),
            created_at: sub.key.created_at().as_secs(),
            expires_at: info.map_or(0, |m| m.expires_at),
            algo: u8::from(sub.key.algorithm()),
            parts: Parts::default(),
        });
    }

    let public_key = public.to_bytes().map_err(|_| GfrStatus::ErrorInternal)?;
    let entry = KeyEntry {
        fingerprint: public.primary_key.fingerprint().as_bytes().to_vec(),
        key_id: public.primary_key.legacy_key_id().as_ref().to_vec(),
        user_ids: public
            .details
            .users
            .iter()
            .map(|u| String::from_utf8_lossy(u.id.id()).into_owned())
            .collect(),
        subkeys: public
            .public_subkeys
            .iter()
            .map(|s| SubkeyEntry {
                fingerprint: s.key.fingerprint().as_bytes().to_vec(),
                key_id: s.key.legacy_key_id().as_ref().to_vec(),
            })
            .collect(),
        digest: Sha256::digest(&public_key).to_vec(),
        has_secret: secret.is_some(),
        public_key,
        parts: Parts::default(),
    };
    Ok(Prepared {
        entry,
        secret,
        unprotected,
        components,
    })
}

// ------------------------------------------------------------------- reading

/// What unlocks a package.
pub enum Unlock<'a> {
    /// The PIN and the package's separate key file.
    Pin { pin: &'a [u8], key_file: &'a [u8] },
    /// The wrap secret of a plain package.
    Secret(&'a [u8]),
}

/// Where a package's bytes come from: a positioned read, so the package is
/// read in place and only the parts asked for are ever loaded. The host may
/// own the file and serve the reads.
pub trait ReadAt {
    /// Fill as much of `buf` as there is from `at` on; the count read, `0`
    /// at the end.
    fn read_at(&self, buf: &mut [u8], at: u64) -> std::io::Result<usize>;
}

impl ReadAt for Vec<u8> {
    fn read_at(&self, buf: &mut [u8], at: u64) -> std::io::Result<usize> {
        let at = usize::try_from(at).unwrap_or(usize::MAX).min(self.len());
        let n = buf.len().min(self.len() - at);
        buf[..n].copy_from_slice(&self[at..at + n]);
        Ok(n)
    }
}

impl ReadAt for std::fs::File {
    #[cfg(unix)]
    fn read_at(&self, buf: &mut [u8], at: u64) -> std::io::Result<usize> {
        std::os::unix::fs::FileExt::read_at(self, buf, at)
    }

    #[cfg(windows)]
    fn read_at(&self, buf: &mut [u8], at: u64) -> std::io::Result<usize> {
        std::os::windows::fs::FileExt::seek_read(self, buf, at)
    }
}

/// The reads a package needs, on any [`ReadAt`].
struct Source(Box<dyn ReadAt>);

impl Source {
    fn read_at(&self, at: u64, len: usize) -> Result<Vec<u8>, GfrStatus> {
        let mut buf = vec![0u8; len];
        let mut done = 0;
        while done < len {
            match self.0.read_at(&mut buf[done..], at + done as u64) {
                Ok(0) => return Err(truncated()),
                Ok(n) => done += n,
                Err(e) if e.kind() == std::io::ErrorKind::Interrupted => {}
                Err(e) => {
                    set_last_error(&format!("cannot read the key package: {e}"));
                    return Err(GfrStatus::ErrorIo);
                }
            }
        }
        Ok(buf)
    }
}

/// A package whose header is read and which is not unlocked yet.
pub struct SealedPackage {
    header: Header,
    /// Magic, length and header: the index's associated data.
    prefix: Vec<u8>,
    source: Source,
}

impl SealedPackage {
    #[cfg(test)]
    /// Read the header of a package held in memory.
    pub fn from_bytes(bytes: Vec<u8>) -> Result<Self, GfrStatus> {
        Self::from_reader(Box::new(bytes))
    }

    /// Read the header of a package file; the rest is read when needed.
    pub fn from_file(file: std::fs::File) -> Result<Self, GfrStatus> {
        Self::from_reader(Box::new(file))
    }

    /// Read the header through `reader`; the rest is read when needed.
    pub fn from_reader(reader: Box<dyn ReadAt>) -> Result<Self, GfrStatus> {
        let source = Source(reader);
        let fixed = MAGIC.len() + 4;
        let mut prefix = source.read_at(0, fixed).map_err(|status| {
            if status == GfrStatus::ErrorDataCorrupted {
                set_last_error("the file is not a key package");
                GfrStatus::ErrorInvalidData
            } else {
                status
            }
        })?;
        if !is_package(&prefix) {
            return fail(GfrStatus::ErrorInvalidData, "the file is not a key package");
        }
        let len = header_len(&prefix)?;
        prefix.extend(source.read_at(fixed as u64, len)?);
        let (header, _) = split_header(&prefix)?;
        Ok(SealedPackage {
            header,
            prefix,
            source,
        })
    }

    pub fn header(&self) -> &Header {
        &self.header
    }

    /// Unlock the package and read its index.
    pub fn unlock(self, unlock: Unlock<'_>) -> Result<OpenPackage, GfrStatus> {
        let data_key = unwrap_data_key(&self.header, unlock)?;

        let node_count = self.header.node_count as usize;
        if node_count > MAX_NODES {
            return fail(
                GfrStatus::ErrorDataCorrupted,
                "the key package index is damaged",
            );
        }
        let at = self.prefix.len() as u64;
        let sealed_len = NONCE_LEN + node_count * self.header.node_len as usize + TAG_LEN;
        let sealed = self.source.read_at(at, sealed_len)?;
        let (nonce, sealed) = sealed.split_at(NONCE_LEN);
        let tree = open(&data_key, nonce, sealed, &self.prefix).map_err(|_| changed())?;

        Ok(OpenPackage {
            header: self.header,
            tree,
            data_key,
            source: self.source,
            parts_at: at + sealed_len as u64,
        })
    }
}

/// An opened package: its index is in memory, its keys are read on demand.
pub struct OpenPackage {
    pub header: Header,
    tree: Zeroizing<Vec<u8>>,
    data_key: Zeroizing<[u8; KEY_LEN]>,
    source: Source,
    /// Where the first part starts.
    parts_at: u64,
}

/// A key found in a package, and the subkey the query named, if it named one.
pub struct Found {
    pub entry: KeyEntry,
    /// What the index says about the component the query named (the primary
    /// for an e-mail address or a user ID).
    pub component: Component,
    /// The upper-case hex fingerprint of the component, set when the query
    /// was a subkey's fingerprint or key ID, or ended in `!`: the operation
    /// must use exactly that component.
    pub exact: Option<String>,
}

/// Whether `bytes` start like a version 3 package.
pub fn is_package(bytes: &[u8]) -> bool {
    bytes.starts_with(MAGIC)
}

#[cfg(test)]
/// Read the header without unlocking anything.
pub fn read_header(bytes: &[u8]) -> Result<Header, GfrStatus> {
    split_header(bytes).map(|(header, _)| header)
}

#[cfg(test)]
/// Unlock and open a package held in memory.
pub fn open_package(bytes: &[u8], unlock: Unlock<'_>) -> Result<OpenPackage, GfrStatus> {
    SealedPackage::from_bytes(bytes.to_vec())?.unlock(unlock)
}

fn header_len(bytes: &[u8]) -> Result<usize, GfrStatus> {
    let at = MAGIC.len();
    let len_bytes: [u8; 4] = bytes
        .get(at..at + 4)
        .and_then(|b| b.try_into().ok())
        .ok_or_else(truncated)?;
    let len = u32::from_be_bytes(len_bytes) as usize;
    if len > MAX_HEADER_BYTES {
        return fail(
            GfrStatus::ErrorInvalidData,
            "the key package header is too large",
        );
    }
    Ok(len)
}

fn split_header(bytes: &[u8]) -> Result<(Header, usize), GfrStatus> {
    if !is_package(bytes) {
        return fail(GfrStatus::ErrorInvalidData, "the file is not a key package");
    }
    let start = MAGIC.len() + 4;
    let end = start + header_len(bytes)?;
    let raw = bytes.get(start..end).ok_or_else(truncated)?;
    let header: Header = from_versioned_cbor(raw, "the key package")?;
    if header.package_id.len() != PACKAGE_ID_LEN
        || (header.node_len as usize) < NODE_LEN
        || header.node_len as usize > MAX_NODE_LEN
    {
        return fail(
            GfrStatus::ErrorDataCorrupted,
            "the key package header is damaged",
        );
    }
    Ok((header, end))
}

fn truncated() -> GfrStatus {
    set_last_error("the key package is truncated");
    GfrStatus::ErrorDataCorrupted
}

fn changed() -> GfrStatus {
    set_last_error("the key package is damaged or was changed");
    GfrStatus::ErrorDataCorrupted
}

fn unwrap_data_key(
    header: &Header,
    unlock: Unlock<'_>,
) -> Result<Zeroizing<[u8; KEY_LEN]>, GfrStatus> {
    let wrong_key = || {
        set_last_error("the key does not open this key package");
        GfrStatus::ErrorWrongPassword
    };
    match (&header.protection, unlock) {
        (Protection::PinFile, Unlock::Pin { pin, key_file }) => {
            let raw = key_file
                .strip_prefix(KEY_FILE_MAGIC)
                .filter(|raw| raw.len() <= MAX_KEY_FILE_BYTES)
                .ok_or_else(|| {
                    set_last_error("the file is not the key file of a key package");
                    GfrStatus::ErrorInvalidData
                })?;
            let file: KeyFile = from_versioned_cbor(raw, "the key file")?;
            if file.package_id != header.package_id {
                return fail(
                    GfrStatus::ErrorInvalidInput,
                    "the key file belongs to another key package",
                );
            }
            // Bound the cost a hostile key file can ask for.
            if file.m_kib > 1024 * 1024 || file.t > 16 || file.p == 0 || file.p > 16 {
                return fail(GfrStatus::ErrorDataCorrupted, "the key file is damaged");
            }
            let wrap_key = pin_wrap_key(pin, &file.wrap.salt, file.m_kib, file.t, file.p)?;
            unwrap_with(&wrap_key, &file.wrap, &header.package_id, "pin_file")
                .map_err(|_| wrong_key())
        }
        (Protection::Plain(wrap), Unlock::Secret(secret)) => {
            let wrap_key = secret_wrap_key(secret, &wrap.salt)?;
            unwrap_with(
                &wrap_key,
                wrap,
                &header.package_id,
                header.protection.kind(),
            )
            .map_err(|_| wrong_key())
        }
        (Protection::PinFile, Unlock::Secret(_)) => fail(
            GfrStatus::ErrorInvalidInput,
            "this key package is protected by a PIN and its key file",
        ),
        (_, Unlock::Pin { .. }) => fail(
            GfrStatus::ErrorInvalidInput,
            "this key package is not protected by a PIN",
        ),
    }
}

impl OpenPackage {
    fn node_len(&self) -> usize {
        self.header.node_len as usize
    }

    /// Every node, as far as this reader knows its fields.
    fn nodes(&self) -> impl Iterator<Item = &[u8]> {
        self.tree
            .chunks_exact(self.node_len())
            .map(|n| &n[..NODE_LEN])
    }

    /// Read and decrypt one part of the key whose metadata is at `key`.
    fn read_part(
        &self,
        key: u32,
        at: u32,
        len: u32,
        kind: PartKind,
    ) -> Result<Zeroizing<Vec<u8>>, GfrStatus> {
        let len = len as usize;
        if len <= NONCE_LEN || len > MAX_PART_BYTES {
            return Err(changed());
        }
        let sealed = self.source.read_at(self.parts_at + u64::from(at), len)?;
        let (nonce, sealed) = sealed.split_at(NONCE_LEN);
        let aad = part_aad(&self.header.package_id, key, kind);
        open(&self.data_key, nonce, sealed, &aad).map_err(|_| changed())
    }

    /// The metadata of the key a component of the index belongs to.
    pub fn entry(&self, component: &Component) -> Result<KeyEntry, GfrStatus> {
        let p = component.parts;
        let plain = self.read_part(p.meta_at, p.meta_at, p.meta_len, PartKind::Meta)?;
        let mut entry: KeyEntry = from_cbor(&plain, "a key in the key package")?;
        entry.parts = p;
        Ok(entry)
    }

    /// The binary secret key of `entry`, when the package carries it.
    pub fn secret_key(&self, entry: &KeyEntry) -> Result<Option<Zeroizing<Vec<u8>>>, GfrStatus> {
        let p = entry.parts;
        if !entry.has_secret || p.secret_len == 0 {
            return Ok(None);
        }
        self.read_part(p.meta_at, p.secret_at, p.secret_len, PartKind::Secret)
            .map(Some)
    }

    /// Every component the index lists, in the order the keys were written
    /// and each key's primary first, then its subkeys in order. Reads no part.
    pub fn components(&self) -> Vec<Component> {
        let mut all: Vec<Component> = self
            .nodes()
            .filter(|n| n[0] == KIND_FPR)
            .map(Component::decode)
            .collect();
        all.sort_by_key(|c| (c.parts.meta_at, c.index));
        all
    }

    /// The primary component of every key, in writing order. Reads no part.
    pub fn primaries(&self) -> Vec<Component> {
        let mut primaries: Vec<Component> = self
            .components()
            .into_iter()
            .filter(|c| !c.subkey())
            .collect();
        primaries.dedup_by_key(|c| c.parts.meta_at);
        primaries
    }

    /// Every key, in the order they were written.
    #[cfg(test)]
    pub fn keys(&self) -> Result<Vec<KeyEntry>, GfrStatus> {
        let primaries = self.primaries();
        if primaries.len() != self.header.key_count as usize {
            return Err(changed());
        }
        primaries.iter().map(|c| self.entry(c)).collect()
    }

    /// Find a key by fingerprint, key ID (either may have a `0x` prefix and a
    /// trailing `!`), e-mail address, `Name <email>`, or part of a user ID.
    /// Everything but a user ID part is one walk down the index.
    pub fn find(&self, query: &str) -> Result<Option<Found>, GfrStatus> {
        let q = query.trim();
        let (q, bang) = match q.strip_suffix('!') {
            Some(rest) => (rest, true),
            None => (q, false),
        };
        let digits: String = q
            .strip_prefix("0x")
            .or_else(|| q.strip_prefix("0X"))
            .unwrap_or(q)
            .chars()
            .filter(|c| *c != ' ')
            .collect();

        let kind = match digits.len() {
            16 => Some(KIND_KEY_ID),
            40 | 64 => Some(KIND_FPR),
            _ => None,
        };
        if let (Some(kind), Ok(id)) = (kind, hex::decode(&digits)) {
            let Some(node) = tree_find(&self.tree, self.node_len(), kind, &id) else {
                return Ok(None);
            };
            let component = Component::decode(node);
            let entry = self.entry(&component)?;
            let exact = (bang || component.subkey()).then(|| entry.component_fpr_hex(&id));
            return Ok(Some(Found {
                entry,
                component,
                exact,
            }));
        }

        if let Some(email) = email_of(q) {
            if let Some(node) =
                tree_find(&self.tree, self.node_len(), KIND_EMAIL, &email_id(&email))
            {
                let component = Component::decode(node);
                return Ok(Some(Found {
                    entry: self.entry(&component)?,
                    component,
                    exact: None,
                }));
            }
        }

        let needle = q.to_lowercase();
        if needle.is_empty() {
            return Ok(None);
        }
        for component in self.primaries() {
            let entry = self.entry(&component)?;
            if entry
                .user_ids
                .iter()
                .any(|u| u.to_lowercase().contains(&needle))
            {
                return Ok(Some(Found {
                    entry,
                    component,
                    exact: None,
                }));
            }
        }
        Ok(None)
    }
}

impl OpenPackage {
    /// Every key a query names, read as gpg reads a key name: a fingerprint
    /// or a key ID, long or short (8 hex digits), with an optional `0x` and a
    /// trailing `!`; `=` an exact user ID; `<addr>` an exact address; `@` part
    /// of an address; `*` part of a user ID; `Name <addr>` its address; else
    /// part of a user ID, ignoring case. A keygrip (`&`) or a card serial
    /// (`#`) names nothing here. A key appears once, in package order.
    pub fn find_all(&self, query: &str) -> Result<Vec<Found>, GfrStatus> {
        let q = query.trim();
        let (body, bang) = match q.strip_suffix('!') {
            Some(rest) => (rest, true),
            None => (q, false),
        };
        let digits: String = body
            .strip_prefix("0x")
            .or_else(|| body.strip_prefix("0X"))
            .unwrap_or(body)
            .chars()
            .filter(|c| *c != ' ')
            .collect();
        let is_hex = !digits.is_empty() && digits.chars().all(|c| c.is_ascii_hexdigit());
        if is_hex && matches!(digits.len(), 16 | 40 | 64) {
            return Ok(self.find(q)?.into_iter().collect());
        }
        if is_hex && digits.len() == 8 {
            let short = hex::decode(&digits).unwrap_or_default();
            let mut found = Vec::new();
            for component in self.components() {
                let entry = self.entry(&component)?;
                let id = if component.subkey() {
                    match entry.subkeys.get(usize::from(component.index) - 1) {
                        Some(sub) => sub.key_id.clone(),
                        None => continue,
                    }
                } else {
                    entry.key_id.clone()
                };
                if id.ends_with(&short) {
                    let exact = (bang || component.subkey()).then(|| entry.component_fpr_hex(&id));
                    found.push(Found {
                        entry,
                        component,
                        exact,
                    });
                }
            }
            return Ok(found);
        }
        if q.is_empty() || q.starts_with('&') || q.starts_with('#') {
            return Ok(Vec::new());
        }

        let lower = q.to_lowercase();
        let matches = |uid: &str| -> bool {
            if let Some(exact) = q.strip_prefix('=') {
                uid == exact
            } else if let Some(addr) = q.strip_prefix('<') {
                let addr = addr.strip_suffix('>').unwrap_or(addr).to_lowercase();
                email_of(uid).is_some_and(|m| m == addr)
            } else if let Some(part) = lower.strip_prefix('@') {
                email_of(uid).is_some_and(|m| m.contains(part))
            } else if let Some(part) = lower.strip_prefix('*') {
                uid.to_lowercase().contains(part)
            } else if let Some(addr) = email_of(q) {
                email_of(uid).is_some_and(|m| m == addr)
            } else {
                uid.to_lowercase().contains(&lower)
            }
        };
        let mut found = Vec::new();
        for component in self.primaries() {
            let entry = self.entry(&component)?;
            if entry.user_ids.iter().any(|u| matches(u)) {
                found.push(Found {
                    entry,
                    component,
                    exact: None,
                });
            }
        }
        Ok(found)
    }
}

/// The lower-case address in `Name <addr>`, or `s` itself when it is one.
fn email_of(s: &str) -> Option<String> {
    let candidate = match (s.rfind('<'), s.rfind('>')) {
        (Some(open), Some(close)) if open < close => &s[open + 1..close],
        _ => s,
    };
    let candidate = candidate.trim();
    (candidate.contains('@') && !candidate.contains(char::is_whitespace))
        .then(|| candidate.to_lowercase())
}

// ------------------------------------------------------------------ crypto

fn random_bytes<const N: usize>() -> [u8; N] {
    let mut out = [0u8; N];
    rand::rngs::OsRng.fill_bytes(&mut out);
    out
}

fn pin_wrap_key(
    pin: &[u8],
    salt: &[u8],
    m_kib: u32,
    t: u32,
    p: u32,
) -> Result<Zeroizing<[u8; KEY_LEN]>, GfrStatus> {
    let params = argon2::Params::new(m_kib, t, p, Some(KEY_LEN)).map_err(|_| {
        set_last_error("the key file names impossible Argon2 parameters");
        GfrStatus::ErrorDataCorrupted
    })?;
    let argon = argon2::Argon2::new(argon2::Algorithm::Argon2id, argon2::Version::V0x13, params);
    let mut out = Zeroizing::new([0u8; KEY_LEN]);
    argon
        .hash_password_into(pin, salt, out.as_mut())
        .map_err(|_| {
            set_last_error("the key file is damaged");
            GfrStatus::ErrorDataCorrupted
        })?;
    Ok(out)
}

fn secret_wrap_key(secret: &[u8], salt: &[u8]) -> Result<Zeroizing<[u8; KEY_LEN]>, GfrStatus> {
    use blake2::digest::Mac;
    use blake2::digest::consts::U32;

    if secret.len() != KEY_LEN {
        return fail(
            GfrStatus::ErrorInvalidInput,
            "the key package key has the wrong length",
        );
    }
    let mut mac = <blake2::Blake2bMac<U32> as Mac>::new_from_slice(secret)
        .map_err(|_| GfrStatus::ErrorInternal)?;
    mac.update(WRAP_DOMAIN);
    mac.update(salt);
    let mut out = Zeroizing::new([0u8; KEY_LEN]);
    out.copy_from_slice(&mac.finalize().into_bytes());
    Ok(out)
}

fn wrap_aad(package_id: &[u8], kind: &str) -> Vec<u8> {
    let mut aad = WRAP_DOMAIN.to_vec();
    aad.push(0);
    aad.extend(package_id);
    aad.push(0);
    aad.extend(kind.as_bytes());
    aad
}

fn part_aad(package_id: &[u8], key: u32, kind: PartKind) -> Vec<u8> {
    let mut aad = PART_DOMAIN.to_vec();
    aad.push(0);
    aad.extend(package_id);
    aad.extend(key.to_be_bytes());
    aad.extend(kind.name());
    aad
}

fn wrap_data_key(
    wrap_key: &[u8; KEY_LEN],
    salt: &[u8],
    data_key: &[u8; KEY_LEN],
    package_id: &[u8],
    kind: &str,
) -> Result<Wrap, GfrStatus> {
    let nonce = random_bytes::<NONCE_LEN>();
    let wrapped = seal(wrap_key, &nonce, data_key, &wrap_aad(package_id, kind))?;
    Ok(Wrap {
        salt: salt.to_vec(),
        nonce: nonce.to_vec(),
        wrapped_key: wrapped,
    })
}

fn unwrap_with(
    wrap_key: &[u8; KEY_LEN],
    wrap: &Wrap,
    package_id: &[u8],
    kind: &str,
) -> Result<Zeroizing<[u8; KEY_LEN]>, GfrStatus> {
    let key = open(
        wrap_key,
        &wrap.nonce,
        &wrap.wrapped_key,
        &wrap_aad(package_id, kind),
    )?;
    let mut out = Zeroizing::new([0u8; KEY_LEN]);
    if key.len() != KEY_LEN {
        return Err(GfrStatus::ErrorDataCorrupted);
    }
    out.copy_from_slice(&key);
    Ok(out)
}

fn seal(key: &[u8; KEY_LEN], nonce: &[u8], msg: &[u8], aad: &[u8]) -> Result<Vec<u8>, GfrStatus> {
    XChaCha20Poly1305::new(key.into())
        .encrypt(XNonce::from_slice(nonce), Payload { msg, aad })
        .map_err(|_| GfrStatus::ErrorInternal)
}

fn open(
    key: &[u8; KEY_LEN],
    nonce: &[u8],
    sealed: &[u8],
    aad: &[u8],
) -> Result<Zeroizing<Vec<u8>>, GfrStatus> {
    if nonce.len() != NONCE_LEN {
        return Err(GfrStatus::ErrorDataCorrupted);
    }
    XChaCha20Poly1305::new(key.into())
        .decrypt(XNonce::from_slice(nonce), Payload { msg: sealed, aad })
        .map(Zeroizing::new)
        .map_err(|_| GfrStatus::ErrorDecryptionFailed)
}

fn fail<T>(status: GfrStatus, msg: &str) -> Result<T, GfrStatus> {
    set_last_error(msg);
    Err(status)
}

#[cfg(test)]
mod tests;
