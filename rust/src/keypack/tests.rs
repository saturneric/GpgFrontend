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

//! Key package v3: round trips through every protection, the index tree and
//! what it says, reads of one block, and the ways a damaged, changed or
//! foreign file must fail.

use super::*;
use crate::testutil::keys::{V4_SIGN, V6_SIGN};
use crate::testutil::rfc9580::{A3_V6_CERT, A5_V6_SECRET_LOCKED};

const NOW: u64 = 1_700_000_000;

fn binary(armored: &str) -> Vec<u8> {
    crate::crypto::dearmor_all(armored.as_bytes()).into_owned()
}

fn signer_and_peer() -> Vec<Vec<u8>> {
    vec![
        binary(&V4_SIGN.secret_armored),
        binary(&V6_SIGN.public_armored),
    ]
}

fn write(keys: &[Vec<u8>], request: ProtectionRequest) -> Result<WrittenPackage, GfrStatus> {
    let refs: Vec<&[u8]> = keys.iter().map(Vec::as_slice).collect();
    write_package(&refs, request, NOW)
}

fn write_plain() -> WrittenPackage {
    write(&signer_and_peer(), ProtectionRequest::Plain).expect("write")
}

fn open_plain(written: &WrittenPackage) -> OpenPackage {
    let secret = written.secret.as_ref().expect("plain package has a secret");
    open_package(&written.package, Unlock::Secret(secret)).expect("open")
}

fn fpr_bytes(hex_fpr: &str) -> Vec<u8> {
    hex::decode(hex_fpr).unwrap()
}

#[test]
fn a_plain_package_round_trips_its_keys() {
    let written = write_plain();
    assert!(written.key_file.is_none());
    assert!(matches!(written.header.protection, Protection::Plain(_)));
    assert_eq!(written.header.key_count, 2);

    let pack = open_plain(&written);
    assert_eq!(pack.header, written.header);
    let keys = pack.keys().unwrap();
    assert_eq!(keys.len(), 2);

    let signer = &keys[0];
    assert_eq!(signer.fpr_hex(), V4_SIGN.primary_fpr);
    assert!(signer.has_secret());
    assert_eq!(signer.primary_user_id(), "V4 Fixture <v4@example.test>");
    assert_eq!(signer.subkeys.len(), 2);
    let secret = pack.secret_key(signer).unwrap().expect("a secret block");
    let key = SignedSecretKey::from_bytes(&secret[..]).expect("secret parses");
    assert_eq!(
        key.primary_key.fingerprint().as_bytes(),
        signer.fingerprint.as_slice()
    );

    let peer = &keys[1];
    assert_eq!(peer.fpr_hex(), V6_SIGN.primary_fpr);
    assert!(!peer.has_secret());
    assert!(pack.secret_key(peer).unwrap().is_none());
    SignedPublicKey::from_bytes(&peer.public_key[..]).expect("public parses");
}

#[test]
fn the_digest_is_the_sha256_of_the_binary_certificate() {
    let pack = open_plain(&write_plain());
    let (cert, _) = SignedPublicKey::from_string(&V6_SIGN.public_armored).unwrap();
    let mut bytes = Vec::new();
    cert.to_writer(&mut bytes).unwrap();
    assert_eq!(
        pack.keys().unwrap()[1].digest,
        Sha256::digest(&bytes).to_vec()
    );
}

#[test]
fn the_header_is_cbor() {
    let written = write_plain();
    let len = header_len(&written.package).unwrap();
    let raw = &written.package[MAGIC.len() + 4..MAGIC.len() + 4 + len];
    let value: ciborium::Value = ciborium::from_reader(raw).expect("standard CBOR");
    let map = value.as_map().expect("a CBOR map");
    let field = |name: &str| {
        map.iter()
            .find(|(k, _)| k.as_text() == Some(name))
            .map(|(_, v)| v.clone())
    };
    assert_eq!(
        field("key_count").and_then(|v| v.as_integer()),
        Some(2.into())
    );
    assert_eq!(
        field("package_id").and_then(|v| v.as_bytes().map(Vec::len)),
        Some(PACKAGE_ID_LEN)
    );
}

#[test]
fn a_pin_package_opens_only_with_its_pin_and_its_own_key_file() {
    let pin = || ProtectionRequest::Pin(Zeroizing::new(b"4711".to_vec()));
    let written = write(&signer_and_peer(), pin()).expect("write");
    assert!(written.secret.is_none());
    assert_eq!(written.header.protection, Protection::PinFile);
    let key_file = written
        .key_file
        .as_ref()
        .expect("a PIN package has a key file");
    assert!(key_file.starts_with(KEY_FILE_MAGIC));

    let pack = open_package(
        &written.package,
        Unlock::Pin {
            pin: b"4711",
            key_file,
        },
    )
    .expect("open");
    assert_eq!(pack.keys().unwrap().len(), 2);

    let wrong = open_package(
        &written.package,
        Unlock::Pin {
            pin: b"0000",
            key_file,
        },
    );
    assert_eq!(wrong.err(), Some(GfrStatus::ErrorWrongPassword));

    let other = write(&signer_and_peer(), pin()).expect("write");
    let foreign = open_package(
        &written.package,
        Unlock::Pin {
            pin: b"4711",
            key_file: other.key_file.as_ref().unwrap(),
        },
    );
    assert_eq!(foreign.err(), Some(GfrStatus::ErrorInvalidInput));
}

#[test]
fn the_wrong_kind_of_unlock_is_refused() {
    let written = write_plain();
    let as_pin = open_package(
        &written.package,
        Unlock::Pin {
            pin: b"x",
            key_file: b"x",
        },
    );
    assert_eq!(as_pin.err(), Some(GfrStatus::ErrorInvalidInput));
}

#[test]
fn another_secret_does_not_open_a_package() {
    let written = write_plain();
    let other = write_plain();
    let result = open_package(
        &written.package,
        Unlock::Secret(other.secret.as_ref().unwrap()),
    );
    assert_eq!(result.err(), Some(GfrStatus::ErrorWrongPassword));
}

/// The package with its header replaced by `header`, re-encoded.
fn with_header(package: &[u8], header: &Header) -> Vec<u8> {
    let old_end = MAGIC.len() + 4 + header_len(package).unwrap();
    let raw = to_cbor(header).unwrap();
    let mut bytes = MAGIC.to_vec();
    bytes.extend((raw.len() as u32).to_be_bytes());
    bytes.extend(raw);
    bytes.extend(&package[old_end..]);
    bytes
}

#[test]
fn a_changed_header_breaks_the_index() {
    let written = write_plain();
    let mut header = written.header.clone();
    header.generated_at += 1;
    let bytes = with_header(&written.package, &header);
    assert_eq!(read_header(&bytes).unwrap().generated_at, NOW + 1);

    let result = open_package(&bytes, Unlock::Secret(written.secret.as_ref().unwrap()));
    assert_eq!(result.err(), Some(GfrStatus::ErrorDataCorrupted));
}

#[test]
fn a_changed_or_truncated_block_fails_only_where_it_is_read() {
    let written = write_plain();
    let secret = written.secret.as_ref().unwrap();

    // The second key's block ends in its public part and an empty secret
    // length. Change the public part: the first key still reads, the second
    // does not.
    let mut flipped = written.package.clone();
    let at = flipped.len() - 5;
    flipped[at] ^= 1;
    let pack = open_package(&flipped, Unlock::Secret(secret)).expect("the index is intact");
    let first = pack.find(&V4_SIGN.primary_fpr).unwrap().expect("first key");
    assert!(pack.secret_key(&first.entry).unwrap().is_some());
    assert_eq!(
        pack.find(&V6_SIGN.primary_fpr).err(),
        Some(GfrStatus::ErrorDataCorrupted)
    );

    let cut = &written.package[..written.package.len() - 20];
    let pack = open_package(cut, Unlock::Secret(secret)).expect("the index is intact");
    assert_eq!(pack.keys().err(), Some(GfrStatus::ErrorDataCorrupted));

    let header_only = &written.package[..MAGIC.len() + 2];
    assert_eq!(
        read_header(header_only).err(),
        Some(GfrStatus::ErrorDataCorrupted)
    );
}

#[test]
fn parts_cannot_be_swapped() {
    // Two public keys, so the parts are their two metadata parts back to back.
    let keys = vec![
        binary(&V4_SIGN.public_armored),
        binary(&V6_SIGN.public_armored),
    ];
    let written = write(&keys, ProtectionRequest::Plain).unwrap();
    let pack = open_plain(&written);
    let parts_at = pack.parts_at as usize;
    let second = pack.primaries()[1].parts.meta_at as usize;
    let mut bytes = written.package[..parts_at].to_vec();
    bytes.extend(&written.package[parts_at + second..]);
    bytes.extend(&written.package[parts_at..parts_at + second]);

    let swapped = open_package(&bytes, Unlock::Secret(written.secret.as_ref().unwrap())).unwrap();
    assert_eq!(
        swapped.find(&V4_SIGN.primary_fpr).err(),
        Some(GfrStatus::ErrorDataCorrupted)
    );
}

#[test]
fn other_files_are_not_packages() {
    assert!(!is_package(b"GFSEC2 a version 2 key package"));
    assert_eq!(read_header(b"").err(), Some(GfrStatus::ErrorInvalidData));
    assert_eq!(
        read_header(V4_SIGN.public_armored.as_bytes()).err(),
        Some(GfrStatus::ErrorInvalidData)
    );
}

#[test]
fn a_package_for_a_newer_reader_is_refused_before_unlocking() {
    let written = write_plain();
    let mut header = written.header.clone();
    header.min_reader = FORMAT_VERSION + 1;
    assert_eq!(
        read_header(&with_header(&written.package, &header)).err(),
        Some(GfrStatus::ErrorUnsupportedFeature)
    );
}

/// `value` as a CBOR map with `extra` set (added or replaced).
fn cbor_with(value: &impl Serialize, extra: &[(&str, ciborium::Value)]) -> Vec<u8> {
    let mut map = ciborium::Value::serialized(value)
        .unwrap()
        .into_map()
        .unwrap();
    for (name, v) in extra {
        map.retain(|(k, _)| k.as_text() != Some(name));
        map.push((ciborium::Value::Text((*name).into()), v.clone()));
    }
    let mut out = Vec::new();
    ciborium::into_writer(&ciborium::Value::Map(map), &mut out).unwrap();
    out
}

#[test]
fn a_header_from_a_newer_compatible_version_reads() {
    // A later writer added a field and said so in `version`, but older
    // readers can still read it: `min_reader` stays.
    let written = write_plain();
    let raw = cbor_with(
        &written.header,
        &[("version", 7.into()), ("future_field", "ignored".into())],
    );
    let header: Header = from_versioned_cbor(&raw, "the key package").expect("readable");
    assert_eq!(header.version, 7);
    assert_eq!(header.key_count, written.header.key_count);
}

#[test]
fn a_header_this_reader_is_too_old_for_is_refused_whatever_its_shape() {
    // A future protection this reader cannot even decode: the version check
    // comes first, so it says "newer", not "damaged".
    let raw = cbor_with(
        &std::collections::BTreeMap::from([("protection", "Quantum")]),
        &[("version", 9.into()), ("min_reader", 9.into())],
    );
    assert_eq!(
        from_versioned_cbor::<Header>(&raw, "the key package").err(),
        Some(GfrStatus::ErrorUnsupportedFeature)
    );
    assert!(crate::err::last_error().contains("newer version"));
}

#[test]
fn a_key_file_from_a_newer_version_is_refused_as_such() {
    let pin = || ProtectionRequest::Pin(Zeroizing::new(b"4711".to_vec()));
    let written = write(&signer_and_peer(), pin()).unwrap();
    let raw = &written.key_file.as_ref().unwrap()[KEY_FILE_MAGIC.len()..];
    let file: KeyFile = from_cbor(raw, "the key file").unwrap();
    let mut newer = KEY_FILE_MAGIC.to_vec();
    newer.extend(cbor_with(&file, &[("min_reader", 4.into())]));
    let result = open_package(
        &written.package,
        Unlock::Pin {
            pin: b"4711",
            key_file: &newer,
        },
    );
    assert_eq!(result.err(), Some(GfrStatus::ErrorUnsupportedFeature));
}

#[test]
fn metadata_with_unknown_fields_reads() {
    let pack = open_plain(&write_plain());
    let entry = pack.find(&V4_SIGN.primary_fpr).unwrap().unwrap().entry;
    let raw = cbor_with(&entry, &[("photo", ciborium::Value::Bytes(vec![1, 2, 3]))]);
    let read: KeyEntry = from_cbor(&raw, "a key").expect("unknown fields are ignored");
    assert_eq!(read.fingerprint, entry.fingerprint);
    assert_eq!(read.user_ids, entry.user_ids);
}

#[test]
fn wider_nodes_from_a_newer_writer_read() {
    // A later writer appended fields to every node and said how wide they
    // are; this reader steps over the part it does not know.
    let keys = signer_and_peer();
    let refs: Vec<&[u8]> = keys.iter().map(Vec::as_slice).collect();
    let written = write_package_with(&refs, ProtectionRequest::Plain, NOW, NODE_LEN + 32).unwrap();
    assert_eq!(written.header.node_len as usize, NODE_LEN + 32);
    let pack = open_plain(&written);

    let found = pack.find("v4@example.test").unwrap().expect("found");
    assert!(pack.secret_key(&found.entry).unwrap().is_some());
    assert!(pack.find(V4_SIGN.sign_subkey_fpr()).unwrap().is_some());
    assert_eq!(pack.primaries().len(), 2);
    assert_eq!(pack.keys().unwrap().len(), 2);
}

#[test]
fn a_header_without_node_len_has_the_original_nodes() {
    let written = write_plain();
    let mut map = ciborium::Value::serialized(&written.header)
        .unwrap()
        .into_map()
        .unwrap();
    map.retain(|(k, _)| k.as_text() != Some("node_len"));
    let mut raw = Vec::new();
    ciborium::into_writer(&ciborium::Value::Map(map), &mut raw).unwrap();
    let header: Header = from_versioned_cbor(&raw, "the key package").unwrap();
    assert_eq!(header.node_len as usize, NODE_LEN);
}

#[test]
fn nodes_narrower_than_this_reader_knows_are_damage() {
    let written = write_plain();
    let mut header = written.header.clone();
    header.node_len = (NODE_LEN - 1) as u32;
    assert_eq!(
        read_header(&with_header(&written.package, &header)).err(),
        Some(GfrStatus::ErrorDataCorrupted)
    );
}

#[test]
fn keys_are_found_by_every_identifier() {
    let pack = open_plain(&write_plain());
    let fpr = &V4_SIGN.primary_fpr;
    let key_id = pack.keys().unwrap()[0].key_id_hex();

    for query in [
        fpr.clone(),
        fpr.to_lowercase(),
        format!("0x{fpr}"),
        key_id.clone(),
        "v4@example.test".into(),
        "V4 Fixture <V4@Example.Test>".into(),
        "v4 fixture".into(),
    ] {
        let found = pack
            .find(&query)
            .unwrap()
            .unwrap_or_else(|| panic!("{query} not found"));
        assert_eq!(&found.entry.fpr_hex(), fpr, "{query}");
        assert!(found.exact.is_none(), "{query}");
        assert!(!found.component.subkey(), "{query}");
    }

    let sub = V4_SIGN.sign_subkey_fpr();
    let found = pack.find(sub).unwrap().expect("subkey fingerprint");
    assert_eq!(&found.entry.fpr_hex(), fpr);
    assert_eq!(found.exact.as_deref(), Some(sub));
    assert!(found.component.subkey());
    assert!(found.component.can(CAP_SIGN));

    let pinned = pack
        .find(&format!("{fpr}!"))
        .unwrap()
        .expect("pinned primary");
    assert_eq!(pinned.exact.as_deref(), Some(fpr.as_str()));

    assert!(pack.find("nobody@example.test").unwrap().is_none());
    assert!(pack.find("0123456789ABCDEF").unwrap().is_none());
    assert!(pack.find("").unwrap().is_none());
}

#[test]
fn the_index_answers_without_reading_a_block() {
    let written = write_plain();
    let pack = open_plain(&written);

    // Cut every block away: what the index knows still reads.
    let parts_at = pack.parts_at as usize;
    let index_only = &written.package[..parts_at];
    let pack = open_package(index_only, Unlock::Secret(written.secret.as_ref().unwrap()))
        .expect("the index alone opens");

    let primaries = pack.primaries();
    assert_eq!(primaries.len(), 2);
    assert!(primaries[0].has_secret && !primaries[1].has_secret);
    assert!(!primaries[0].protected);
    assert!(!primaries[0].revoked);
    assert!(primaries[0].created_at > 0);
    assert_eq!(
        primaries[1].algo, 27,
        "the V6 fixture is Ed25519 (RFC 9580 §9.1)"
    );

    let v4: Vec<_> = pack
        .components()
        .into_iter()
        .filter(|c| c.same_key(&primaries[0]))
        .collect();
    assert_eq!(v4.len(), 3, "the primary and two subkeys");
    assert_eq!(
        v4.iter().map(|c| c.index).collect::<Vec<_>>(),
        [0, 1, 2],
        "in the order of the key's subkeys"
    );
    assert!(v4.iter().any(|c| c.subkey() && c.can(CAP_SIGN)));
    assert!(v4.iter().any(|c| c.can(CAP_ENCRYPT)));
    assert!(v4[0].can(CAP_CERTIFY));

    assert_eq!(pack.keys().err(), Some(GfrStatus::ErrorDataCorrupted));
}

#[test]
fn a_locked_component_is_marked_in_the_index() {
    let keys = vec![binary(A5_V6_SECRET_LOCKED)];
    let written = write(&keys, ProtectionRequest::Plain).unwrap();
    let pack = open_plain(&written);
    assert!(
        pack.components()
            .iter()
            .all(|c| c.protected && c.has_secret)
    );
}

/// A primary component whose key's metadata is at `meta_at`.
fn at_key(meta_at: u32) -> Component {
    Component {
        index: 0,
        has_secret: false,
        protected: false,
        revoked: false,
        caps: 0,
        created_at: 0,
        expires_at: 0,
        algo: 0,
        parts: Parts {
            meta_at,
            meta_len: 1,
            secret_at: 0,
            secret_len: 0,
        },
    }
}

#[test]
fn the_tree_is_balanced_and_finds_every_node() {
    for n in [1usize, 2, 3, 7, 8, 100, 255, 1000] {
        let nodes: Vec<NodeInput> = (0..n)
            .map(|i| NodeInput {
                kind: KIND_FPR,
                id: Sha256::digest((i as u64).to_be_bytes())[..20].to_vec(),
                component: at_key(i as u32),
            })
            .collect();
        let ids: Vec<Vec<u8>> = nodes.iter().map(|n| n.id.clone()).collect();
        let tree = build_tree(nodes, NODE_LEN);
        assert_eq!(tree.len(), n * NODE_LEN);

        // Every node is the search-tree split of its subtree.
        let key = |at: usize| {
            node_key(&tree[(at - 1) * NODE_LEN..at * NODE_LEN])
                .1
                .to_vec()
        };
        for at in 1..=n {
            for child in [2 * at, 2 * at + 1] {
                if child <= n {
                    assert_eq!(key(child) < key(at), child == 2 * at, "n={n} at={at}");
                }
            }
        }

        for (i, id) in ids.iter().enumerate() {
            let node =
                tree_find(&tree, NODE_LEN, KIND_FPR, id).unwrap_or_else(|| panic!("n={n} i={i}"));
            assert_eq!(Component::decode(node).parts.meta_at, i as u32);
        }
        assert!(tree_find(&tree, NODE_LEN, KIND_FPR, &[0u8; 20]).is_none());
        assert!(tree_find(&tree, NODE_LEN, KIND_KEY_ID, &ids[0]).is_none());
    }
}

#[test]
fn a_shared_e_mail_address_finds_the_first_key() {
    let nodes: Vec<NodeInput> = (0..5u32)
        .map(|i| NodeInput {
            kind: KIND_EMAIL,
            id: email_id("same@example.test"),
            component: at_key(4 - i),
        })
        .collect();
    let tree = build_tree(nodes, NODE_LEN);
    let node = tree_find(&tree, NODE_LEN, KIND_EMAIL, &email_id("same@example.test")).unwrap();
    assert_eq!(Component::decode(node).parts.meta_at, 0);
}

#[test]
fn a_file_is_read_in_place() {
    let written = write_plain();
    let dir = tempfile::tempdir().unwrap();
    let path = dir.path().join("default.gfpack");
    std::fs::write(&path, &written.package).unwrap();

    let sealed = SealedPackage::from_file(std::fs::File::open(&path).unwrap()).unwrap();
    assert_eq!(sealed.header(), &written.header);
    let pack = sealed
        .unlock(Unlock::Secret(written.secret.as_ref().unwrap()))
        .unwrap();
    let found = pack.find("v4@example.test").unwrap().unwrap();
    assert!(pack.secret_key(&found.entry).unwrap().is_some());
    assert_eq!(pack.keys().unwrap().len(), 2);

    std::fs::write(&path, b"GFKPK3\n").unwrap();
    assert!(SealedPackage::from_file(std::fs::File::open(&path).unwrap()).is_err());
    std::fs::write(&path, b"plain text").unwrap();
    assert_eq!(
        SealedPackage::from_file(std::fs::File::open(&path).unwrap()).err(),
        Some(GfrStatus::ErrorInvalidData)
    );
}

#[test]
fn unprotected_secrets_are_flagged() {
    let written = write_plain();
    assert!(written.header.secrets_unprotected);

    let public_only = write(&[binary(&V6_SIGN.public_armored)], ProtectionRequest::Plain).unwrap();
    assert!(!public_only.header.secrets_unprotected);

    let locked = write(&[binary(A5_V6_SECRET_LOCKED)], ProtectionRequest::Plain).unwrap();
    assert!(!locked.header.secrets_unprotected);
}

#[test]
fn a_key_without_user_ids_is_still_indexed_by_fingerprint() {
    let written = write(&[binary(A3_V6_CERT)], ProtectionRequest::Plain).unwrap();
    let pack = open_plain(&written);
    let keys = pack.keys().unwrap();
    assert_eq!(keys[0].primary_user_id(), "");
    assert!(pack.find(&keys[0].fpr_hex()).unwrap().is_some());
}

#[test]
fn bad_inputs_are_refused() {
    assert_eq!(
        write(&[], ProtectionRequest::Plain).err(),
        Some(GfrStatus::ErrorInvalidInput)
    );
    let twice = vec![
        binary(&V6_SIGN.public_armored),
        binary(&V6_SIGN.public_armored),
    ];
    assert_eq!(
        write(&twice, ProtectionRequest::Plain).err(),
        Some(GfrStatus::ErrorInvalidInput)
    );
    assert_eq!(
        write(
            &signer_and_peer(),
            ProtectionRequest::Pin(Zeroizing::new(Vec::new()))
        )
        .err(),
        Some(GfrStatus::ErrorInvalidInput)
    );
    for garbage in [
        b"not a key".to_vec(),
        V4_SIGN.public_armored.as_bytes().to_vec(),
    ] {
        assert_eq!(
            write(&[garbage], ProtectionRequest::Plain).err(),
            Some(GfrStatus::ErrorInvalidData)
        );
    }
}

#[test]
fn the_plain_key_text_round_trips() {
    let written = write_plain();
    let secret = written.secret.as_ref().unwrap();
    let text = plain_key_text(secret);
    assert!(text.starts_with(PLAIN_KEY_PREFIX));
    assert_eq!(text.len(), PLAIN_KEY_PREFIX.len() + 64);

    let parsed = parse_plain_key(&format!("  {}\n", text.as_str())).unwrap();
    assert_eq!(parsed.as_slice(), secret.as_slice());
    // The bare digits are accepted too.
    let digits = &text[PLAIN_KEY_PREFIX.len()..];
    assert_eq!(
        parse_plain_key(digits).unwrap().as_slice(),
        secret.as_slice()
    );

    assert_eq!(
        parse_plain_key("gfkp3:abcd").err(),
        Some(GfrStatus::ErrorInvalidInput)
    );
    assert_eq!(
        parse_plain_key("hello").err(),
        Some(GfrStatus::ErrorInvalidInput)
    );
}

/// `fixture` with every component locked by the test passphrase, binary.
/// Each test takes its own fixture: the passphrase cache is process-wide, and
/// a cached passphrase would answer for a test running in parallel.
fn protected(fixture: &crate::testutil::keys::Fixture) -> Vec<u8> {
    use pgp::types::Password;
    let pw = Password::from(crate::testutil::cb::CORRECT_PASSPHRASE);
    let mut key = fixture.secret.clone();
    let mut rng = rand::thread_rng();
    key.primary_key.set_password(&mut rng, &pw).unwrap();
    for sub in key.secret_subkeys.iter_mut() {
        sub.key.set_password(&mut rng, &pw).unwrap();
    }
    key.to_bytes().unwrap()
}

#[test]
fn a_protected_key_loses_its_passphrase_for_a_package() {
    let locked = protected(&V6_SIGN);
    let open = crate::key::remove_key_password_internal(
        0,
        &locked,
        Some(crate::testutil::cb::pwd_correct),
    )
    .expect("the test passphrase unlocks the key");
    crate::cache::PASSWORD_CACHE.remove_by_fpr(&V6_SIGN.primary_fpr);

    let key = SignedSecretKey::from_bytes(&open[..]).unwrap();
    assert!(!key.primary_key.secret_params().is_encrypted());
    assert!(
        key.secret_subkeys
            .iter()
            .all(|s| !s.key.secret_params().is_encrypted())
    );

    let locked_package = write(&[locked], ProtectionRequest::Plain).unwrap();
    assert!(!locked_package.header.secrets_unprotected);

    let open_package = write(&[open.to_vec()], ProtectionRequest::Plain).unwrap();
    assert!(open_package.header.secrets_unprotected);
}

#[test]
fn a_wrong_passphrase_keeps_the_key_protected() {
    let locked = protected(&V4_SIGN);
    let result =
        crate::key::remove_key_password_internal(0, &locked, Some(crate::testutil::cb::pwd_wrong));
    assert_eq!(result.err(), Some(GfrStatus::ErrorBadPassphrase));
}

#[test]
fn an_unprotected_key_needs_no_passphrase() {
    let result =
        crate::key::remove_key_password_internal(0, &binary(&V4_SIGN.secret_armored), None)
            .expect("nothing to unlock");
    let key = SignedSecretKey::from_bytes(&result[..]).unwrap();
    assert_eq!(
        key.primary_key.fingerprint().as_bytes(),
        fpr_bytes(&V4_SIGN.primary_fpr).as_slice()
    );
}

/// The packages `scripts/bench_gfpipe.sh` measures gfpipe with: for each
/// count in `GF_BENCH_KEYS` (default `10,1000,10000`), `<count>/default.gfpack`
/// and its `default.gfkey` under `GF_BENCH_DIR`. Key 0 carries its secret and
/// signs; the rest are public, each with a signing and an encryption subkey
/// and the user ID `Bench User <i> <user<i>@bench.test>`.
#[test]
#[ignore = "bench fixture: run by scripts/bench_gfpipe.sh"]
fn deferred_write_bench_packages() {
    use crate::testutil::keys::{cfg, generate};
    use crate::types::{GfrKeyAlgo, GfrOpenPGPKeyVersion};

    let dir = std::path::PathBuf::from(std::env::var("GF_BENCH_DIR").expect("GF_BENCH_DIR"));
    let counts: Vec<usize> = std::env::var("GF_BENCH_KEYS")
        .unwrap_or_else(|_| "10,1000,10000".into())
        .split(',')
        .map(|n| n.trim().parse().expect("GF_BENCH_KEYS is a list of counts"))
        .collect();
    let max = counts.iter().copied().max().unwrap_or(0);

    let v4 = GfrOpenPGPKeyVersion::V4;
    let keys: Vec<Vec<u8>> = (0..max)
        .map(|i| {
            let secret = generate(
                &format!("Bench User {i} <user{i}@bench.test>"),
                &cfg(GfrKeyAlgo::ED25519, true, false, v4),
                &[
                    cfg(GfrKeyAlgo::ED25519, true, false, v4),
                    cfg(GfrKeyAlgo::ED25519, false, true, v4),
                ],
            );
            if i == 0 {
                secret.to_bytes().unwrap()
            } else {
                SignedPublicKey::from(secret).to_bytes().unwrap()
            }
        })
        .collect();

    for count in counts {
        let written = write(&keys[..count], ProtectionRequest::Plain).unwrap();
        let out = dir.join(count.to_string());
        std::fs::create_dir_all(&out).unwrap();
        std::fs::write(out.join("default.gfpack"), &written.package).unwrap();
        let secret = written.secret.as_ref().unwrap();
        std::fs::write(out.join("default.gfkey"), plain_key_text(secret).as_bytes()).unwrap();
        println!(
            "{count} keys: {} nodes, {} bytes",
            written.header.node_count,
            written.package.len()
        );
    }
}
