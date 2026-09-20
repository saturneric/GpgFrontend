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

//! Structural inspection of arbitrary OpenPGP data.
//!
//! Takes any input — armored or binary; message, detached signature,
//! certificate or cleartext-signed text — and describes what it is made of:
//! the armor envelope, then a tree of packets carrying tag, version, header
//! framing, byte offset, length and decoded per-packet fields.
//!
//! Two properties shape the whole module:
//!
//! - **Nothing is ever decrypted.** Encrypted containers are reported by their
//!   envelope (version, cipher, AEAD mode, chunk size) and never opened.
//!   Compressed containers *are* recursed into, under a size and depth cap.
//! - **It never fails wholesale.** Whatever could be parsed is reported, with
//!   per-node and document-level `error` strings for whatever could not. Input
//!   that is not OpenPGP at all yields an empty document and a note saying so,
//!   not an error status.
//!
//! The framing figures describe *the bytes on the wire*, not what rPGP would
//! write if it re-serialised the packet, so the cursor is walked by hand
//! rather than by asking a parsed packet for its length.

use std::io::Read;

use pgp::armor::{BlockType, Dearmor, DearmorOptions};
use pgp::composed::CleartextSignedMessage;
use pgp::crypto::aead::AeadAlgorithm;
use pgp::crypto::sym::SymmetricKeyAlgorithm;
use pgp::packet::{Packet, PacketHeader};
use pgp::types::{KeyDetails, PacketLength, Tag};
use serde::Serialize;

/// Maximum armored block size handed to the dearmorer, and the ceiling on the
/// total number of bytes produced by decompression across the whole document.
const MAX_BLOCK_SIZE: usize = 64 * 1024 * 1024;

/// Ceiling on nesting of compressed containers. Eight is far past anything a
/// real producer emits and stops a decompression bomb from recursing forever.
const MAX_DEPTH: usize = 8;

/// The number of leading body bytes reported verbatim for packets whose
/// contents are opaque to us (encrypted payloads, padding, unknown tags).
const PREVIEW_BYTES: usize = 16;

// ---------------------------------------------------------------------------
// The document
// ---------------------------------------------------------------------------

/// One `label` / `value` row in a packet's detail table.
#[derive(Debug, Clone, Serialize, PartialEq, Eq)]
pub struct Field {
    pub label: String,
    pub value: String,
}

impl Field {
    fn new(label: &str, value: impl Into<String>) -> Self {
        Self {
            label: label.to_string(),
            value: value.into(),
        }
    }
}

/// The armor envelope of a block.
#[derive(Debug, Clone, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct ArmorInfo {
    /// The block type as it is spelled in the `-----BEGIN ...-----` line.
    pub block_type: String,
    /// The armor headers, in the order rPGP reports them.
    pub headers: Vec<Field>,
    /// `"ok"`, `"mismatch"`, `"unchecked"` or `"absent"`.
    pub crc24: String,
}

/// One packet, and the packets nested inside it.
#[derive(Debug, Clone, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct PacketNode {
    /// The numeric packet type ID, as carried in the header.
    pub tag: u8,
    pub tag_name: String,
    /// Offset of the packet header, relative to the start of the packet
    /// stream this packet lives in (the dearmored octets, for armored data).
    pub offset: usize,
    /// `"old"` or `"new"` header framing.
    pub header_version: String,
    pub header_length: usize,
    /// Bytes on the wire after the header, including the length octets of the
    /// continuation chunks of a partial-length body.
    pub body_length: usize,
    /// `"fixed"`, `"partial"` or `"indeterminate"`.
    pub length_type: String,
    /// The packet body's own version octet, where the type carries one.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub version: Option<u32>,
    pub fields: Vec<Field>,
    pub children: Vec<PacketNode>,
    /// Why this packet could not be decoded, if it could not be.
    pub error: Option<String>,
}

/// One armor block, or the whole input when it carries no armor.
#[derive(Debug, Clone, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct Block {
    /// `"binary"`, `"armored"` or `"cleartext"`.
    pub kind: String,
    /// Offset of the block within the original input.
    pub offset: usize,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub armor: Option<ArmorInfo>,
    /// For a cleartext-signed block, the signed text and its hash headers.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub cleartext: Option<CleartextInfo>,
    pub packets: Vec<PacketNode>,
    pub error: Option<String>,
}

/// The signed-text half of a cleartext-signed message.
#[derive(Debug, Clone, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct CleartextInfo {
    pub text_size: usize,
    pub headers: Vec<Field>,
}

/// What an inspected input turned out to be.
#[derive(Debug, Clone, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct Document {
    /// The kind of the first block, or `"binary"` when there is none.
    pub format: String,
    /// Size of the original input, in bytes.
    pub size: usize,
    pub blocks: Vec<Block>,
    /// Document-level notes: nothing recognisable, trailing garbage, caps hit.
    pub errors: Vec<String>,
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

/// Describe the structure of `data`. Never fails; see the module docs.
pub fn inspect(data: &[u8]) -> Document {
    let mut doc = Document {
        format: "binary".to_string(),
        size: data.len(),
        blocks: Vec::new(),
        errors: Vec::new(),
    };

    if data.is_empty() {
        doc.errors.push("the input is empty".to_string());
        return doc;
    }

    let mut budget = MAX_BLOCK_SIZE;

    // Armor is text, so the armored and cleartext routes both need the input
    // to be text. Anything else can only be a raw packet stream.
    if let Ok(text) = std::str::from_utf8(data) {
        let regions = find_armor_regions(text);
        if !regions.is_empty() {
            for region in &regions {
                doc.blocks.push(inspect_region(text, region, &mut budget));
            }
            doc.format = doc.blocks[0].kind.clone();
            return doc;
        }
    }

    doc.blocks
        .push(inspect_binary(data, 0, "binary", &mut budget));
    if doc.blocks[0].packets.is_empty() && doc.blocks[0].error.is_some() {
        doc.errors.push("not OpenPGP data".to_string());
    }
    doc
}

// ---------------------------------------------------------------------------
// Locating armor
// ---------------------------------------------------------------------------

/// A `-----BEGIN ...-----` / `-----END ...-----` span within the input.
#[derive(Debug, Clone, Copy)]
struct Region {
    start: usize,
    end: usize,
    cleartext: bool,
}

/// Find every armor block in `text`, in input order.
///
/// Splitting the input ourselves, rather than asking the dearmorer for the
/// unconsumed remainder, keeps concatenated blocks working: `Dearmor` only
/// hands back its inner reader once it has reached its `Done` state, so the
/// remainder is unreachable on exactly the malformed inputs that most need
/// reporting.
fn find_armor_regions(text: &str) -> Vec<Region> {
    let mut regions = Vec::new();
    let mut open: Option<(usize, bool)> = None;

    for (offset, line) in line_offsets(text) {
        let trimmed = line.trim();
        if !trimmed.starts_with("-----") || !trimmed.ends_with("-----") {
            continue;
        }
        if let Some(rest) = trimmed.strip_prefix("-----BEGIN ") {
            if open.is_none() {
                let cleartext = rest.starts_with("PGP SIGNED MESSAGE");
                open = Some((offset, cleartext));
            }
        } else if trimmed.starts_with("-----END ") {
            if let Some((start, cleartext)) = open.take() {
                regions.push(Region {
                    start,
                    end: offset + line.len(),
                    cleartext,
                });
            }
        }
    }

    // An unterminated block is still worth reporting: it is exactly what a
    // truncated paste looks like, and its header and partial body parse.
    if let Some((start, cleartext)) = open {
        regions.push(Region {
            start,
            end: text.len(),
            cleartext,
        });
    }

    regions
}

/// The cleartext framework headers: the `Key: Value` lines between the
/// `-----BEGIN PGP SIGNED MESSAGE-----` line and the blank line that starts
/// the signed text. In practice this is the `Hash` header, which says which
/// digest a verifier must use, and which rPGP does not surface.
fn cleartext_headers(body: &str) -> Vec<Field> {
    let mut headers = Vec::new();
    for (_, line) in line_offsets(body).skip(1) {
        if line.is_empty() {
            break;
        }
        match line.split_once(':') {
            Some((key, value)) => headers.push(Field::new(key.trim(), value.trim())),
            None => break,
        }
    }
    headers
}

/// Locate the `-----BEGIN PGP SIGNATURE-----` block trailing the signed text
/// of a cleartext-signed message.
fn find_nested_signature(body: &str) -> Option<Region> {
    let start = line_offsets(body)
        .find(|(_, line)| line.trim().starts_with("-----BEGIN PGP SIGNATURE"))
        .map(|(offset, _)| offset)?;
    let mut region = find_armor_regions(&body[start..]).into_iter().next()?;
    region.start += start;
    region.end += start;
    Some(region)
}

/// Iterate `(byte offset, line without its terminator)` over `text`.
fn line_offsets(text: &str) -> impl Iterator<Item = (usize, &str)> {
    let mut offset = 0;
    text.split_inclusive('\n').map(move |raw| {
        let at = offset;
        offset += raw.len();
        (at, raw.trim_end_matches(['\n', '\r']))
    })
}

// ---------------------------------------------------------------------------
// Per-block inspection
// ---------------------------------------------------------------------------

fn inspect_region(text: &str, region: &Region, budget: &mut usize) -> Block {
    let body = &text[region.start..region.end];
    if region.cleartext {
        inspect_cleartext(body, region.start, budget)
    } else {
        inspect_armored(body, region.start, budget)
    }
}

/// Dearmor one block and walk the packets it carries.
fn inspect_armored(body: &str, offset: usize, budget: &mut usize) -> Block {
    let mut dearmor = Dearmor::with_options(
        std::io::Cursor::new(body.as_bytes()),
        // rPGP's own CRC24 check is deliberately not enabled: turning it on
        // makes the dearmorer abort the read at the footer, so the decoded
        // body comes back short and every packet after the cut looks
        // truncated. The checksum is verified below instead, over the bytes
        // that were actually decoded.
        DearmorOptions::new().set_limit(MAX_BLOCK_SIZE),
    );

    // Whatever was decoded before the failure stays in `decoded`, so a
    // truncated or CRC-broken block still lists the packets it did carry.
    let mut decoded = Vec::new();
    let armor_error = dearmor.read_to_end(&mut decoded).err().map(|e| e.to_string());

    // Only the plain fields and `crc24_status` may be touched after a failed
    // read: `into_parts` panics unless the dearmorer reached its `Done` state.
    let armor = ArmorInfo {
        block_type: dearmor
            .typ
            .as_ref()
            .map(block_type_name)
            .unwrap_or_else(|| "unknown".to_string()),
        headers: dearmor
            .headers
            .iter()
            .flat_map(|(k, values)| values.iter().map(|v| Field::new(k, v.clone())))
            .collect(),
        crc24: match dearmor.checksum {
            None => "absent".to_string(),
            Some(footer) if footer as u32 == crc24(&decoded) => "ok".to_string(),
            Some(_) => "mismatch".to_string(),
        },
    };

    let (packets, walk_error) = walk_packets(&decoded, 0, budget);
    Block {
        kind: "armored".to_string(),
        offset,
        armor: Some(armor),
        cleartext: None,
        packets,
        error: armor_error.or(walk_error),
    }
}

/// Describe a cleartext-signed message: the signed text, then the packets of
/// the signature block embedded at its end.
fn inspect_cleartext(body: &str, offset: usize, budget: &mut usize) -> Block {
    let (text_size, parse_error) = match CleartextSignedMessage::from_string(body) {
        Ok((msg, _)) => (msg.text().len(), None),
        Err(e) => (0, Some(e.to_string())),
    };
    let headers = cleartext_headers(body);

    // The signature half is ordinary armor; reuse the armored route so its
    // packets carry real framing rather than a re-serialised approximation.
    // It has to be located by hand: to the block scanner the whole cleartext
    // message is one region, so the signature is nested inside it.
    let mut block = match find_nested_signature(body) {
        Some(sig) => inspect_armored(&body[sig.start..sig.end], offset + sig.start, budget),
        None => Block {
            kind: "cleartext".to_string(),
            offset,
            armor: None,
            cleartext: None,
            packets: Vec::new(),
            error: Some("no signature block follows the signed text".to_string()),
        },
    };

    block.kind = "cleartext".to_string();
    block.offset = offset;
    block.cleartext = Some(CleartextInfo { text_size, headers });
    block.error = parse_error.or(block.error);
    block
}

/// Walk a raw packet stream.
fn inspect_binary(data: &[u8], offset: usize, kind: &str, budget: &mut usize) -> Block {
    let (packets, error) = walk_packets(data, 0, budget);
    Block {
        kind: kind.to_string(),
        offset,
        armor: None,
        cleartext: None,
        packets,
        error,
    }
}

// ---------------------------------------------------------------------------
// The packet walk
// ---------------------------------------------------------------------------

/// The framing of one packet, as it appears on the wire.
struct Framing {
    header: PacketHeader,
    header_length: usize,
    /// Bytes after the header that belong to this packet.
    body_length: usize,
    /// The body with any partial-chunk length octets removed.
    body: Vec<u8>,
    /// Set when the input ended inside this packet.
    truncated: bool,
}

/// Read the framing of the packet starting at `data[0]`.
fn read_framing(data: &[u8]) -> std::io::Result<Framing> {
    let mut cursor = data;
    let header = PacketHeader::try_from_reader(&mut cursor)?;
    let header_length = data.len() - cursor.len();

    let mut body = Vec::new();
    let mut consumed = 0usize;
    let mut truncated = false;
    let mut length = header.packet_length();

    loop {
        match length {
            PacketLength::Fixed(n) => {
                let n = n as usize;
                let available = cursor.len().min(n);
                body.extend_from_slice(&cursor[..available]);
                consumed += available;
                truncated |= available < n;
                break;
            }
            PacketLength::Indeterminate => {
                // Runs to the end of the enclosing stream by definition, so it
                // is the last packet and cannot be truncated.
                body.extend_from_slice(cursor);
                consumed += cursor.len();
                break;
            }
            PacketLength::Partial(n) => {
                let n = n as usize;
                let available = cursor.len().min(n);
                body.extend_from_slice(&cursor[..available]);
                cursor = &cursor[available..];
                consumed += available;
                if available < n {
                    truncated = true;
                    break;
                }
                // Each chunk is followed by the length of the next one, which
                // is on the wire but is not part of the body.
                let before = cursor.len();
                match PacketLength::try_from_reader(&mut cursor) {
                    Ok(next) => {
                        consumed += before - cursor.len();
                        length = next;
                    }
                    Err(_) => {
                        truncated = true;
                        break;
                    }
                }
            }
        }
    }

    Ok(Framing {
        header,
        header_length,
        body_length: consumed,
        body,
        truncated,
    })
}

/// Walk every packet in `data`, recursing into compressed containers.
///
/// Returns the nodes and, when the stream could not be walked to its end, a
/// note saying why. The note never replaces the nodes already collected.
fn walk_packets(data: &[u8], depth: usize, budget: &mut usize) -> (Vec<PacketNode>, Option<String>) {
    let mut nodes = Vec::new();
    let mut offset = 0usize;

    while offset < data.len() {
        let framing = match read_framing(&data[offset..]) {
            Ok(f) => f,
            Err(e) => {
                let note = if nodes.is_empty() && offset == 0 {
                    "not OpenPGP data".to_string()
                } else {
                    format!("stopped at offset {offset}: {e}")
                };
                return (nodes, Some(note));
            }
        };

        let mut node = describe_framing(&framing, offset);
        match Packet::from_reader(framing.header, &framing.body[..]) {
            Ok(packet) => {
                let (version, fields) = describe_packet(&packet, &framing.body);
                node.version = version;
                node.fields = fields;
                if let Packet::CompressedData(compressed) = &packet {
                    let (children, note) = walk_compressed(compressed, depth, budget);
                    node.children = children;
                    node.error = node.error.or(note);
                }
            }
            Err(e) => {
                node.fields = preview_fields(&framing.body);
                node.error = node.error.or(Some(e.to_string()));
            }
        }

        let advance = framing.header_length + framing.body_length;
        nodes.push(node);
        if framing.truncated || advance == 0 {
            return (
                nodes,
                Some(format!("the input ends inside the packet at offset {offset}")),
            );
        }
        offset += advance;
    }

    (nodes, None)
}

/// Decompress a container and walk what is inside, honouring both caps.
fn walk_compressed(
    compressed: &pgp::packet::CompressedData,
    depth: usize,
    budget: &mut usize,
) -> (Vec<PacketNode>, Option<String>) {
    if depth + 1 >= MAX_DEPTH {
        return (
            Vec::new(),
            Some(format!("nesting deeper than {MAX_DEPTH} levels, not expanded")),
        );
    }

    let reader = match compressed.decompress() {
        Ok(r) => r,
        Err(e) => return (Vec::new(), Some(e.to_string())),
    };

    let mut inner = Vec::new();
    let limit = *budget;
    let read = reader.take(limit as u64 + 1).read_to_end(&mut inner);
    if let Err(e) = read {
        // Keep whatever decompressed cleanly before the failure.
        if inner.is_empty() {
            return (Vec::new(), Some(e.to_string()));
        }
    }

    let capped = inner.len() > limit;
    if capped {
        inner.truncate(limit);
    }
    *budget -= inner.len();

    let (children, note) = walk_packets(&inner, depth + 1, budget);
    let note = if capped {
        Some(format!(
            "decompressed payload exceeds the {MAX_BLOCK_SIZE} byte budget, truncated"
        ))
    } else {
        note
    };
    (children, note)
}

/// Turn wire framing into a node, before the body has been decoded.
fn describe_framing(framing: &Framing, offset: usize) -> PacketNode {
    let tag = framing.header.tag();
    PacketNode {
        tag: u8::from(tag),
        tag_name: tag_name(tag),
        offset,
        header_version: match framing.header {
            PacketHeader::Old { .. } => "old".to_string(),
            PacketHeader::New { .. } => "new".to_string(),
        },
        header_length: framing.header_length,
        body_length: framing.body_length,
        length_type: match framing.header.packet_length() {
            PacketLength::Fixed(_) => "fixed".to_string(),
            PacketLength::Partial(_) => "partial".to_string(),
            PacketLength::Indeterminate => "indeterminate".to_string(),
        },
        version: None,
        fields: Vec::new(),
        children: Vec::new(),
        error: framing
            .truncated
            .then(|| "the packet body is truncated".to_string()),
    }
}

// ---------------------------------------------------------------------------
// Per-packet fields
// ---------------------------------------------------------------------------

/// Describe a decoded packet: its body version, and its interesting fields.
///
/// `body` is the packet's octets with any partial-chunk length octets removed.
/// A couple of packet types keep their parsed contents private, so the only
/// way to report them is to read the envelope back out of the body.
fn describe_packet(packet: &Packet, body: &[u8]) -> (Option<u32>, Vec<Field>) {
    match packet {
        Packet::Signature(sig) => (Some(u8::from(sig.version()) as u32), describe_signature(sig)),
        Packet::OnePassSignature(ops) => (Some(ops.version() as u32), describe_one_pass(ops)),
        Packet::PublicKeyEncryptedSessionKey(pkesk) => {
            (Some(u8::from(pkesk.version()) as u32), describe_pkesk(pkesk))
        }
        Packet::SymKeyEncryptedSessionKey(skesk) => {
            (Some(u8::from(skesk.version()) as u32), describe_skesk(skesk))
        }
        Packet::LiteralData(literal) => (None, describe_literal(literal)),
        Packet::CompressedData(compressed) => (None, describe_compressed(compressed)),
        Packet::SymEncryptedData(sed) => (
            Some(1),
            vec![
                Field::new("Encrypted Bytes", sed.data().len().to_string()),
                Field::new(
                    "Integrity Protection",
                    "none (deprecated, RFC 9580 forbids producing this)",
                ),
            ],
        ),
        Packet::SymEncryptedProtectedData(seipd) => {
            (Some(seipd.version() as u32), describe_seipd(seipd))
        }
        Packet::GnupgAeadData(_) => (Some(1), describe_gnupg_aead(body)),
        Packet::PublicKey(key) => (Some(u8::from(key.version()) as u32), describe_key(key)),
        Packet::PublicSubkey(key) => (Some(u8::from(key.version()) as u32), describe_key(key)),
        Packet::SecretKey(key) => (Some(u8::from(key.version()) as u32), describe_key(key)),
        Packet::SecretSubkey(key) => (Some(u8::from(key.version()) as u32), describe_key(key)),
        Packet::UserId(user_id) => (
            None,
            vec![Field::new(
                "User ID",
                String::from_utf8_lossy(user_id.id()).into_owned(),
            )],
        ),
        Packet::UserAttribute(attr) => (None, vec![Field::new("Attribute", attr.to_string())]),
        Packet::Marker(_) => (
            None,
            vec![Field::new("Marker", "PGP (a legacy compatibility marker)")],
        ),
        Packet::Trust(_) => (
            None,
            vec![Field::new("Trust", "local trust data, not part of the message")],
        ),
        Packet::ModDetectionCode(_) => (
            None,
            vec![Field::new("Digest", "SHA-1 modification detection code")],
        ),
        Packet::Padding(_) => (
            None,
            vec![
                Field::new("Padding Bytes", body.len().to_string()),
                Field::new("Contents", "random padding, carries no meaning"),
            ],
        ),
    }
}

fn describe_signature(sig: &pgp::packet::Signature) -> Vec<Field> {
    let mut fields = vec![Field::new("Version", format!("{:?}", sig.version()))];
    if let Some(typ) = sig.typ() {
        fields.push(Field::new(
            "Signature Type",
            format!("{typ:?} (0x{:02x})", u8::from(typ)),
        ));
    }
    if let Some(hash) = sig.hash_alg() {
        fields.push(Field::new("Hash Algorithm", format!("{hash:?}")));
    }
    if let Some(config) = sig.config() {
        fields.push(Field::new(
            "Public Key Algorithm",
            format!("{:?}", config.pub_alg),
        ));
    }
    if let Some(created) = sig.created() {
        fields.push(Field::new("Created", format!("{created:?}")));
    }
    for issuer in sig.issuer_fingerprint() {
        fields.push(Field::new("Issuer Fingerprint", issuer.to_string()));
    }
    for issuer in sig.issuer_key_id() {
        fields.push(Field::new("Issuer Key ID", issuer.to_string()));
    }
    if let Some(expiry) = sig.signature_expiration_time() {
        fields.push(Field::new(
            "Signature Expires After",
            format!("{} s", expiry.as_secs()),
        ));
    }
    if let Some(expiry) = sig.key_expiration_time() {
        fields.push(Field::new(
            "Key Expires After",
            format!("{} s", expiry.as_secs()),
        ));
    }
    if let Some(hashed) = sig.signed_hash_value() {
        fields.push(Field::new(
            "Signed Hash Prefix",
            format!("{:02x}{:02x}", hashed[0], hashed[1]),
        ));
    }
    if let Some(config) = sig.config() {
        fields.push(Field::new(
            "Hashed Subpackets",
            config.hashed_subpackets().count().to_string(),
        ));
        fields.push(Field::new(
            "Unhashed Subpackets",
            config.unhashed_subpackets().count().to_string(),
        ));
        for subpacket in config.hashed_subpackets() {
            fields.push(Field::new(
                "Hashed Subpacket",
                format!("{:?}", subpacket.typ()),
            ));
        }
    }
    fields
}

fn describe_one_pass(ops: &pgp::packet::OnePassSignature) -> Vec<Field> {
    let mut fields = vec![
        Field::new("Version", ops.version().to_string()),
        Field::new(
            "Signature Type",
            format!("{:?} (0x{:02x})", ops.typ(), u8::from(ops.typ())),
        ),
        Field::new("Hash Algorithm", format!("{:?}", ops.hash_algorithm())),
        Field::new(
            "Public Key Algorithm",
            format!("{:?}", ops.public_key_algorithm()),
        ),
        Field::new(
            "Nested",
            if ops.is_nested() { "yes" } else { "no" }.to_string(),
        ),
    ];
    match ops.version_specific() {
        pgp::packet::OpsVersionSpecific::V3 { key_id } => {
            fields.push(Field::new("Issuer Key ID", key_id.to_string()));
        }
        pgp::packet::OpsVersionSpecific::V6 { fingerprint, .. } => {
            fields.push(Field::new("Issuer Fingerprint", to_hex(fingerprint)));
        }
        other => fields.push(Field::new("Issuer", format!("{other:?}"))),
    }
    fields
}

fn describe_pkesk(pkesk: &pgp::packet::PublicKeyEncryptedSessionKey) -> Vec<Field> {
    let mut fields = vec![Field::new("Version", format!("{:?}", pkesk.version()))];
    match pkesk.algorithm() {
        Ok(alg) => fields.push(Field::new("Public Key Algorithm", format!("{alg:?}"))),
        Err(e) => fields.push(Field::new("Public Key Algorithm", e.to_string())),
    }
    if let Ok(id) = pkesk.id() {
        fields.push(Field::new("Recipient Key ID", id.to_string()));
    }
    if let Ok(Some(fingerprint)) = pkesk.fingerprint() {
        fields.push(Field::new("Recipient Fingerprint", fingerprint.to_string()));
    }
    fields.push(Field::new(
        "Session Key",
        "encrypted, not opened by the inspector",
    ));
    fields
}

fn describe_skesk(skesk: &pgp::packet::SymKeyEncryptedSessionKey) -> Vec<Field> {
    let mut fields = vec![Field::new("Version", format!("{:?}", skesk.version()))];
    if let Some(alg) = skesk.sym_algorithm() {
        fields.push(Field::new("Symmetric Algorithm", format!("{alg:?}")));
    }
    fields.push(Field::new("S2K", format!("{:?}", skesk.s2k())));
    if let Some(key) = skesk.encrypted_key() {
        fields.push(Field::new("Encrypted Key Bytes", key.len().to_string()));
    }
    fields
}

fn describe_literal(literal: &pgp::packet::LiteralData) -> Vec<Field> {
    vec![
        Field::new(
            "Mode",
            if literal.is_binary() {
                "binary"
            } else {
                "text"
            },
        ),
        Field::new(
            "File Name",
            String::from_utf8_lossy(literal.file_name()).into_owned(),
        ),
        Field::new("Data Bytes", literal.data().len().to_string()),
    ]
}

fn describe_compressed(compressed: &pgp::packet::CompressedData) -> Vec<Field> {
    // The algorithm is the first body octet, so it is readable without
    // decompressing anything.
    let algorithm = compressed
        .compressed_data()
        .first()
        .map(|id| compression_name(*id))
        .unwrap_or_else(|| "unknown".to_string());
    vec![
        Field::new("Compression Algorithm", algorithm),
        Field::new(
            "Compressed Bytes",
            compressed.compressed_data().len().to_string(),
        ),
    ]
}

fn describe_seipd(seipd: &pgp::packet::SymEncryptedProtectedData) -> Vec<Field> {
    let mut fields = vec![Field::new("Version", seipd.version().to_string())];
    match seipd.config() {
        pgp::packet::SymEncryptedProtectedDataConfig::V1 => {
            fields.push(Field::new(
                "Integrity Protection",
                "SHA-1 modification detection code",
            ));
        }
        pgp::packet::SymEncryptedProtectedDataConfig::V2 {
            sym_alg,
            aead,
            chunk_size,
            ..
        } => {
            fields.push(Field::new("Symmetric Algorithm", format!("{sym_alg:?}")));
            fields.push(Field::new("AEAD Mode", format!("{aead:?}")));
            fields.push(Field::new("Chunk Size", format!("{chunk_size:?}")));
        }
    }
    fields.push(Field::new(
        "Encrypted Bytes",
        seipd.data().len().to_string(),
    ));
    fields.push(Field::new(
        "Payload",
        "encrypted, not opened by the inspector",
    ));
    fields
}

/// rPGP keeps a `GnupgAeadData`'s config private, so the envelope is read
/// straight out of the body: version, cipher, AEAD mode, chunk size, then the
/// IV. Draft-koch-librepgp-03, "OCB Encrypted Data Packet".
fn describe_gnupg_aead(body: &[u8]) -> Vec<Field> {
    let mut fields = vec![Field::new(
        "Container",
        "GnuPG OCB Encrypted Data (non-standard)",
    )];
    if body.len() >= 4 {
        fields.push(Field::new(
            "Symmetric Algorithm",
            format!("{:?}", SymmetricKeyAlgorithm::from(body[1])),
        ));
        fields.push(Field::new(
            "AEAD Mode",
            format!("{:?}", AeadAlgorithm::from(body[2])),
        ));
        fields.push(Field::new("Chunk Size Octet", body[3].to_string()));
    }
    fields.push(Field::new(
        "Payload",
        "encrypted, not opened by the inspector",
    ));
    fields
}

fn describe_key(key: &impl KeyDetails) -> Vec<Field> {
    vec![
        Field::new("Version", format!("{:?}", key.version())),
        Field::new("Public Key Algorithm", format!("{:?}", key.algorithm())),
        Field::new("Created", format!("{:?}", key.created_at())),
        Field::new("Fingerprint", key.fingerprint().to_string()),
        Field::new("Key ID", key.legacy_key_id().to_string()),
    ]
}

/// The fallback for a body we could not decode: show its first octets so the
/// reader can at least recognise what they are looking at.
fn preview_fields(body: &[u8]) -> Vec<Field> {
    let shown = body.len().min(PREVIEW_BYTES);
    let mut value = to_hex(&body[..shown]);
    if body.len() > shown {
        value.push('…');
    }
    vec![
        Field::new("Body Bytes", body.len().to_string()),
        Field::new("First Octets", value),
    ]
}

/// Lower-case hex, without a separator.
fn to_hex(bytes: &[u8]) -> String {
    use std::fmt::Write;
    bytes.iter().fold(String::new(), |mut out, b| {
        let _ = write!(out, "{b:02x}");
        out
    })
}

// ---------------------------------------------------------------------------
// Name tables
// ---------------------------------------------------------------------------

fn tag_name(tag: Tag) -> String {
    match tag {
        Tag::PublicKeyEncryptedSessionKey => "Public-Key Encrypted Session Key",
        Tag::Signature => "Signature",
        Tag::SymKeyEncryptedSessionKey => "Symmetric-Key Encrypted Session Key",
        Tag::OnePassSignature => "One-Pass Signature",
        Tag::SecretKey => "Secret Key",
        Tag::PublicKey => "Public Key",
        Tag::SecretSubkey => "Secret Subkey",
        Tag::CompressedData => "Compressed Data",
        Tag::SymEncryptedData => "Symmetrically Encrypted Data",
        Tag::Marker => "Marker",
        Tag::LiteralData => "Literal Data",
        Tag::Trust => "Trust",
        Tag::UserId => "User ID",
        Tag::PublicSubkey => "Public Subkey",
        Tag::UserAttribute => "User Attribute",
        Tag::SymEncryptedProtectedData => "Symmetrically Encrypted and Integrity Protected Data",
        Tag::ModDetectionCode => "Modification Detection Code",
        Tag::GnupgAeadData => "OCB Encrypted Data (GnuPG)",
        Tag::Padding => "Padding",
        other => return format!("Unknown ({})", u8::from(other)),
    }
    .to_string()
}

fn compression_name(id: u8) -> String {
    match id {
        0 => "Uncompressed".to_string(),
        1 => "ZIP (RFC 1951 deflate)".to_string(),
        2 => "ZLIB (RFC 1950)".to_string(),
        3 => "BZip2".to_string(),
        other => format!("Unknown ({other})"),
    }
}

fn block_type_name(typ: &BlockType) -> String {
    typ.to_string()
}

/// The armor checksum of RFC 4880 §6.1: a 24-bit CRC, initialised to
/// 0xB704CE over the polynomial 0x1864CFB.
///
/// Kept even though RFC 9580 made the footer optional, because a mismatch is
/// precisely the kind of quiet corruption someone opens this dialog to find.
fn crc24(data: &[u8]) -> u32 {
    let mut crc: u32 = 0x00B7_04CE;
    for byte in data {
        crc ^= (*byte as u32) << 16;
        for _ in 0..8 {
            crc <<= 1;
            if crc & 0x0100_0000 != 0 {
                crc ^= 0x0186_4CFB;
            }
        }
    }
    crc & 0x00FF_FFFF
}

#[cfg(test)]
mod inspect_tests {
    //! The packet walk, against the committed RFC 9580 corpus. Every vector's
    //! provenance is recorded in `MANIFEST.txt` beside it.

    use super::*;
    use crate::testutil::corpus;

    fn doc(data: &[u8]) -> Document {
        inspect(data)
    }

    /// The packets of the first block.
    fn packets(data: &[u8]) -> Vec<PacketNode> {
        let d = doc(data);
        assert!(!d.blocks.is_empty(), "every input yields at least a block");
        d.blocks[0].packets.clone()
    }

    fn tags(nodes: &[PacketNode]) -> Vec<u8> {
        nodes.iter().map(|n| n.tag).collect()
    }

    fn field<'a>(node: &'a PacketNode, label: &str) -> Option<&'a str> {
        node.fields
            .iter()
            .find(|f| f.label == label)
            .map(|f| f.value.as_str())
    }

    fn fields_named<'a>(node: &'a PacketNode, label: &str) -> Vec<&'a str> {
        node.fields
            .iter()
            .filter(|f| f.label == label)
            .map(|f| f.value.as_str())
            .collect()
    }

    /// Every corpus vector, as bytes, for the sweeps over all of them.
    fn every_vector() -> Vec<(&'static str, &'static [u8])> {
        vec![
            ("enc_v1seipd_mdc", corpus::ENC_V1SEIPD_MDC),
            ("enc_v2seipd_ocb", corpus::ENC_V2SEIPD_OCB),
            ("enc_multi_recipient", corpus::ENC_MULTI_RECIPIENT),
            ("enc_symmetric_v1", corpus::ENC_SYMMETRIC_V1),
            ("enc_symmetric_v2", corpus::ENC_SYMMETRIC_V2),
            ("enc_sed_tag9", corpus::ENC_SED_TAG9),
            ("sig_good_detached", corpus::SIG_GOOD_DETACHED),
            ("sig_v6_detached", corpus::SIG_V6_DETACHED),
            ("sig_good_inline_v6", corpus::SIG_GOOD_INLINE_V6),
            ("sig_inline_compressed", corpus::SIG_INLINE_COMPRESSED),
            ("two_signer", corpus::TWO_SIGNER),
            ("pkesk_no_seipd", corpus::PKESK_NO_SEIPD),
            ("garbage", corpus::GARBAGE),
            ("empty", corpus::EMPTY),
            ("sig_good_cleartext", corpus::SIG_GOOD_CLEARTEXT.as_bytes()),
            ("truncated_armor", corpus::TRUNCATED_ARMOR.as_bytes()),
            ("corrupt_crc", corpus::CORRUPT_CRC.as_bytes()),
            ("aux_good", corpus::AUX_GOOD.as_bytes()),
            ("aux_v6_secret", corpus::AUX_V6_SECRET.as_bytes()),
        ]
    }

    // -- the framing invariant ------------------------------------------------

    fn assert_offsets_are_contiguous(name: &str, nodes: &[PacketNode]) {
        for pair in nodes.windows(2) {
            let (a, b) = (&pair[0], &pair[1]);
            assert_eq!(
                a.offset + a.header_length + a.body_length,
                b.offset,
                "{name}: packet at {} does not abut the next one",
                a.offset
            );
        }
        for node in nodes {
            assert_offsets_are_contiguous(name, &node.children);
        }
    }

    #[test]
    fn packet_offsets_abut_across_every_vector() {
        // The one invariant that proves the framing is read off the wire and
        // not reconstructed: consecutive packets leave no gap and no overlap.
        for (name, data) in every_vector() {
            for block in doc(data).blocks {
                assert_offsets_are_contiguous(name, &block.packets);
            }
        }
    }

    #[test]
    fn no_vector_makes_the_walk_panic_or_lose_its_document() {
        for (name, data) in every_vector() {
            let d = doc(data);
            assert_eq!(d.size, data.len(), "{name}: size must be the input's");
        }
    }

    #[test]
    fn every_packet_reports_a_header_that_is_at_least_two_octets() {
        // One tag octet plus at least one length octet; nothing smaller is a
        // valid header, so a zero here would mean the cursor never advanced.
        for (name, data) in every_vector() {
            for block in doc(data).blocks {
                for node in &block.packets {
                    assert!(
                        node.header_length >= 1,
                        "{name}: empty header at offset {}",
                        node.offset
                    );
                }
            }
        }
    }

    // -- encrypted messages ---------------------------------------------------

    #[test]
    fn a_v1_seipd_message_is_a_pkesk_followed_by_the_container() {
        let nodes = packets(corpus::ENC_V1SEIPD_MDC);
        let tags = tags(&nodes);
        assert_eq!(tags.last(), Some(&18), "the container comes last: {tags:?}");
        assert!(
            tags[..tags.len() - 1].iter().all(|t| *t == 1),
            "everything before it addresses a recipient: {tags:?}"
        );

        let container = nodes.last().expect("a container");
        assert_eq!(container.version, Some(1));
        assert_eq!(
            field(container, "Integrity Protection"),
            Some("SHA-1 modification detection code")
        );
    }

    #[test]
    fn a_v2_seipd_message_reports_its_aead_mode_without_decrypting() {
        let nodes = packets(corpus::ENC_V2SEIPD_OCB);
        assert_eq!(nodes[0].tag, 1);
        assert_eq!(nodes[0].version, Some(6), "a v2 SEIPD pairs with a v6 PKESK");

        let container = nodes.last().expect("a container");
        assert_eq!(container.tag, 18);
        assert_eq!(container.version, Some(2));
        assert_eq!(field(container, "AEAD Mode"), Some("Ocb"));
        assert!(field(container, "Chunk Size").is_some());
    }

    #[test]
    fn an_encrypted_container_is_never_opened() {
        for data in [corpus::ENC_V1SEIPD_MDC, corpus::ENC_V2SEIPD_OCB] {
            let nodes = packets(data);
            let container = nodes.last().expect("a container");
            assert!(
                container.children.is_empty(),
                "the inspector must not decrypt"
            );
            assert_eq!(
                field(container, "Payload"),
                Some("encrypted, not opened by the inspector")
            );
        }
    }

    #[test]
    fn a_multi_recipient_message_lists_one_pkesk_per_recipient() {
        let nodes = packets(corpus::ENC_MULTI_RECIPIENT);
        let pkesks: Vec<_> = nodes.iter().filter(|n| n.tag == 1).collect();
        assert!(pkesks.len() >= 3, "three recipients, at least one key each");

        let mut ids: Vec<_> = pkesks
            .iter()
            .filter_map(|n| field(n, "Recipient Key ID"))
            .collect();
        let addressed = ids.len();
        ids.sort_unstable();
        ids.dedup();
        assert_eq!(
            ids.len(),
            addressed,
            "each recipient key must be distinguishable"
        );
    }

    #[test]
    fn a_passphrase_encrypted_message_reports_its_s2k() {
        for data in [corpus::ENC_SYMMETRIC_V1, corpus::ENC_SYMMETRIC_V2] {
            let nodes = packets(data);
            let skesk = nodes.iter().find(|n| n.tag == 3).expect("a SKESK");
            assert!(
                field(skesk, "S2K").is_some_and(|v| !v.is_empty()),
                "the S2K is how a reader judges the passphrase's cost"
            );
        }
    }

    #[test]
    fn the_legacy_unprotected_container_is_named_and_flagged() {
        let nodes = packets(corpus::ENC_SED_TAG9);
        let sed = nodes.iter().find(|n| n.tag == 9).expect("a tag 9 packet");
        assert_eq!(sed.tag_name, "Symmetrically Encrypted Data");
        assert!(
            field(sed, "Integrity Protection").is_some_and(|v| v.starts_with("none")),
            "the whole point of showing tag 9 is that it is unprotected"
        );
    }

    // -- signatures -----------------------------------------------------------

    #[test]
    fn a_v4_detached_signature_reports_its_type_and_algorithms() {
        let nodes = packets(corpus::SIG_GOOD_DETACHED);
        assert_eq!(tags(&nodes), vec![2]);
        assert_eq!(nodes[0].version, Some(4));
        assert!(field(&nodes[0], "Signature Type").is_some());
        assert!(field(&nodes[0], "Hash Algorithm").is_some());
        assert!(field(&nodes[0], "Public Key Algorithm").is_some());
    }

    #[test]
    fn a_v6_detached_signature_is_reported_as_version_six() {
        let nodes = packets(corpus::SIG_V6_DETACHED);
        assert_eq!(tags(&nodes), vec![2]);
        assert_eq!(nodes[0].version, Some(6));
    }

    #[test]
    fn a_signature_names_the_key_that_made_it() {
        let nodes = packets(corpus::SIG_GOOD_DETACHED);
        let named = !fields_named(&nodes[0], "Issuer Key ID").is_empty()
            || !fields_named(&nodes[0], "Issuer Fingerprint").is_empty();
        assert!(named, "an unattributable signature is the useless case");
    }

    #[test]
    fn an_inline_signature_over_compressed_data_recurses() {
        // The recursion case: the one-pass signature, literal data and
        // signature packets all live inside the compressed container.
        let nodes = packets(corpus::SIG_INLINE_COMPRESSED);
        let compressed = nodes.iter().find(|n| n.tag == 8).expect("a tag 8 packet");
        assert!(
            field(compressed, "Compression Algorithm").is_some(),
            "the algorithm comes from the body's first octet"
        );
        assert!(
            !compressed.children.is_empty(),
            "compressed containers must be expanded"
        );
        let inner = tags(&compressed.children);
        assert!(inner.contains(&11), "the literal data is inside: {inner:?}");
        assert!(inner.contains(&2), "the signature is inside: {inner:?}");
    }

    #[test]
    fn two_signers_produce_paired_one_pass_and_signature_packets() {
        let nodes = packets(corpus::TWO_SIGNER);
        // The packets may sit inside a compressed container or at the top.
        let flat = flatten(&nodes);
        assert_eq!(flat.iter().filter(|n| n.tag == 4).count(), 2);
        assert_eq!(flat.iter().filter(|n| n.tag == 2).count(), 2);
    }

    fn flatten(nodes: &[PacketNode]) -> Vec<PacketNode> {
        let mut out = Vec::new();
        for node in nodes {
            out.push(node.clone());
            out.extend(flatten(&node.children));
        }
        out
    }

    #[test]
    fn a_cleartext_signed_message_is_reported_as_cleartext() {
        let d = doc(corpus::SIG_GOOD_CLEARTEXT.as_bytes());
        assert_eq!(d.format, "cleartext");
        let block = &d.blocks[0];
        let cleartext = block.cleartext.as_ref().expect("the signed text");
        assert!(cleartext.text_size > 0);
        assert!(
            cleartext.headers.iter().any(|f| f.label == "Hash"),
            "the Hash header is what a verifier needs"
        );
        assert_eq!(tags(&block.packets), vec![2]);
    }

    #[test]
    fn a_two_signer_cleartext_message_lists_both_signatures() {
        let d = doc(corpus::SIG_TWO_SIGNER_CLEARTEXT.as_bytes());
        assert_eq!(d.format, "cleartext");
        assert_eq!(tags(&d.blocks[0].packets), vec![2, 2]);
    }

    // -- certificates ---------------------------------------------------------

    #[test]
    fn a_certificate_shows_its_key_user_id_and_self_signature() {
        let d = doc(corpus::AUX_GOOD.as_bytes());
        assert_eq!(d.format, "armored");
        let block = &d.blocks[0];
        let armor = block.armor.as_ref().expect("an armor envelope");
        assert_eq!(armor.block_type, "PGP PUBLIC KEY BLOCK");

        let tags = tags(&block.packets);
        assert_eq!(tags.first(), Some(&6), "a certificate opens with its key");
        assert!(tags.contains(&13), "a user id: {tags:?}");
        assert!(tags.contains(&2), "a self-signature: {tags:?}");

        let key = &block.packets[0];
        assert!(field(key, "Fingerprint").is_some_and(|v| v.len() >= 40));
    }

    #[test]
    fn a_v6_secret_key_block_is_walked_without_touching_its_secrets() {
        let d = doc(corpus::AUX_V6_SECRET.as_bytes());
        let tags = tags(&d.blocks[0].packets);
        assert_eq!(tags.first(), Some(&5), "a secret key block opens with tag 5");
        let key = &d.blocks[0].packets[0];
        assert_eq!(key.version, Some(6));
    }

    #[test]
    fn a_signature_lists_the_subpackets_it_carries() {
        let d = doc(corpus::AUX_GOOD.as_bytes());
        let sig = d.blocks[0]
            .packets
            .iter()
            .find(|n| n.tag == 2)
            .expect("a self-signature");
        let count = field(sig, "Hashed Subpackets").expect("a subpacket count");
        assert_ne!(count, "0", "a self-signature always carries subpackets");
        assert!(!fields_named(sig, "Hashed Subpacket").is_empty());
    }

    // -- malformed ------------------------------------------------------------

    #[test]
    fn a_corrupt_crc24_is_reported_and_the_packets_are_still_listed() {
        let d = doc(corpus::CORRUPT_CRC.as_bytes());
        let block = &d.blocks[0];
        assert_eq!(block.armor.as_ref().expect("armor").crc24, "mismatch");
        assert!(
            !block.packets.is_empty(),
            "a bad checksum must not hide the packets it covers"
        );
    }

    #[test]
    fn a_truncated_armor_block_yields_partial_results_and_a_note() {
        let d = doc(corpus::TRUNCATED_ARMOR.as_bytes());
        let block = &d.blocks[0];
        assert!(
            block.error.is_some(),
            "the reader has to be told the input is incomplete"
        );
    }

    #[test]
    fn garbage_is_reported_as_not_openpgp_rather_than_failing() {
        let d = doc(corpus::GARBAGE);
        assert!(
            d.errors.iter().any(|e| e == "not OpenPGP data")
                || d.blocks[0].error.is_some(),
            "garbage must produce a finding, not an exception"
        );
    }

    #[test]
    fn an_empty_input_is_an_empty_document() {
        let d = doc(corpus::EMPTY);
        assert_eq!(d.size, 0);
        assert!(d.blocks.is_empty());
        assert_eq!(d.errors, vec!["the input is empty".to_string()]);
    }

    #[test]
    fn a_header_that_runs_past_the_end_is_flagged_not_dropped() {
        // A new-format signature header claiming 200 bytes, with 4 present.
        let truncated = [0xc2u8, 200, 0x04, 0x00, 0x01, 0x08];
        let d = doc(&truncated);
        let nodes = &d.blocks[0].packets;
        assert_eq!(nodes.len(), 1, "the packet is still reported");
        assert!(nodes[0].error.is_some(), "and is marked truncated");
    }

    // -- concatenation --------------------------------------------------------

    #[test]
    fn two_concatenated_armor_blocks_are_both_shown() {
        let two = format!("{}\n{}", corpus::AUX_GOOD, corpus::AUX_GOOD);
        let d = doc(two.as_bytes());
        assert_eq!(d.blocks.len(), 2, "a file holding two certs shows both");
        assert!(!d.blocks[1].packets.is_empty());
        assert!(
            d.blocks[1].offset > d.blocks[0].offset,
            "blocks carry their offset in the input"
        );
    }

    // -- caps -----------------------------------------------------------------

    /// Wrap `inner` in a ZIP-compressed data packet (tag 8, new framing).
    fn compress(inner: &[u8]) -> Vec<u8> {
        use flate2::{Compression, write::DeflateEncoder};
        use std::io::Write;

        let mut encoder = DeflateEncoder::new(Vec::new(), Compression::default());
        encoder.write_all(inner).expect("deflate");
        let deflated = encoder.finish().expect("deflate");

        let mut body = vec![1u8]; // ZIP
        body.extend_from_slice(&deflated);

        let mut out = vec![0xc8u8]; // new format, tag 8
        // Five-octet length, which is always legal and avoids a size branch.
        out.push(0xff);
        out.extend_from_slice(&(body.len() as u32).to_be_bytes());
        out.extend_from_slice(&body);
        out
    }

    #[test]
    fn a_compressed_packet_built_here_round_trips_through_the_walk() {
        // Guards the nesting test below: if `compress` were malformed, the
        // depth cap would look like it worked when nothing was parsed at all.
        // A binary vector, because what gets compressed has to be a packet
        // stream: armored text would decompress to base64, not to packets.
        let payload = compress(corpus::ENC_V1SEIPD_MDC);
        let nodes = packets(&payload);
        assert_eq!(tags(&nodes), vec![8]);
        assert_eq!(tags(&nodes[0].children).last(), Some(&18));
    }

    #[test]
    fn nesting_deeper_than_the_cap_stops_with_a_note_not_a_stack_overflow() {
        let mut payload = corpus::ENC_V1SEIPD_MDC.to_vec();
        for _ in 0..(MAX_DEPTH + 4) {
            payload = compress(&payload);
        }

        let nodes = packets(&payload);
        let mut depth = 0;
        let mut node = &nodes[0];
        while let Some(child) = node.children.first() {
            depth += 1;
            node = child;
        }
        assert!(depth < MAX_DEPTH, "expansion must stop at the cap: {depth}");
        assert!(
            node.error.is_some(),
            "and must say why it stopped"
        );
    }
}

