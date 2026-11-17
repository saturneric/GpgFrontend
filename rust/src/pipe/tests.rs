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

//! gfpipe end to end, in process: the exact command lines git, pass, build
//! tools and scripts send gpg, what they parse from the answers, pipes, and
//! every refusal. The gpg interop tests at the end check the output with gpg
//! itself, both ways.

use std::io::Write;
use std::sync::{Arc, Mutex};

use super::*;
use crate::keypack::{ProtectionRequest, plain_key_text, write_package};
use pgp::composed::{Deserializable, SignedPublicKey};
use pgp::types::KeyDetails;

use crate::testutil::keys::{V4_NISTP256, V4_SIGN, V6_SIGN};
use crate::testutil::rfc9580::A5_V6_SECRET_LOCKED;

/// A key as the package takes it: binary.
fn bin(armored: &str) -> Vec<u8> {
    crate::crypto::dearmor_all(armored.as_bytes()).into_owned()
}

/// A writer the test can read back.
#[derive(Clone, Default)]
struct Shared(Arc<Mutex<Vec<u8>>>);

impl Write for Shared {
    fn write(&mut self, buf: &[u8]) -> std::io::Result<usize> {
        self.0.lock().unwrap().extend_from_slice(buf);
        Ok(buf.len())
    }
    fn flush(&mut self) -> std::io::Result<()> {
        Ok(())
    }
}

impl Shared {
    fn bytes(&self) -> Vec<u8> {
        self.0.lock().unwrap().clone()
    }
    fn text(&self) -> String {
        String::from_utf8_lossy(&self.bytes()).into_owned()
    }
}

struct Outcome {
    code: i32,
    stdout: Shared,
    stderr: Shared,
}

/// A package in a fresh gfpipe folder: V4 with its secret, V6 public only.
struct Fixture {
    dir: tempfile::TempDir,
    key: String,
}

impl Fixture {
    fn new() -> Self {
        Self::with(
            &[bin(&V4_SIGN.secret_armored), bin(&V6_SIGN.public_armored)],
            "default",
        )
    }

    fn with(keys: &[Vec<u8>], name: &str) -> Self {
        let dir = tempfile::tempdir().unwrap();
        let keys: Vec<&[u8]> = keys.iter().map(Vec::as_slice).collect();
        let written = write_package(&keys, ProtectionRequest::Plain, 1_700_000_000).unwrap();
        std::fs::write(dir.path().join(format!("{name}.gfpack")), &written.package).unwrap();
        let key = plain_key_text(written.secret.as_ref().unwrap()).to_string();
        Self { dir, key }
    }

    fn env(&self) -> HashMap<String, String> {
        HashMap::from([(ENV_KEY.to_string(), self.key.clone())])
    }

    fn run(&self, argv: &[&str], stdin: &[u8]) -> Outcome {
        self.run_with(argv, stdin, self.env())
    }

    fn run_with(&self, argv: &[&str], stdin: &[u8], env: HashMap<String, String>) -> Outcome {
        let host = Host {
            program_dir: Some(self.dir.path().to_path_buf()),
            version: "2.3.0".into(),
            env,
        };
        run_on(&host, argv, stdin)
    }

    fn file(&self, name: &str, content: &[u8]) -> String {
        let path = self.dir.path().join(name);
        std::fs::write(&path, content).unwrap();
        path.to_string_lossy().into_owned()
    }
}

fn run_on(host: &Host, argv: &[&str], stdin: &[u8]) -> Outcome {
    let stdout = Shared::default();
    let stderr = Shared::default();
    let mut io = Io {
        stdin: Box::new(std::io::Cursor::new(stdin.to_vec())),
        stdout: Box::new(stdout.clone()),
        stderr: Box::new(stderr.clone()),
    };
    let argv: Vec<String> = argv.iter().map(|s| s.to_string()).collect();
    let code = run(&argv, host, &mut io);
    Outcome {
        code,
        stdout,
        stderr,
    }
}

const PAYLOAD: &[u8] =
    b"tree 4b825dc642cb6eb9a060e54bf8d69288fbee4904\nauthor A <a@b> 0 +0000\n\nmsg\n";

/// What git checks after signing (gpg-interface.c, sign_buffer_gpg).
fn git_accepts_signing(status: &str) -> bool {
    status.starts_with("[GNUPG:] SIG_CREATED ") || status.contains("\n[GNUPG:] SIG_CREATED ")
}

/// What git checks after verifying (verify_gpg_signed_buffer).
fn git_accepts_verification(status: &str) -> bool {
    status.contains("\n[GNUPG:] GOODSIG ") || status.contains("\n[GNUPG:] EXPKEYSIG ")
}

fn git_sign(f: &Fixture, key: &str) -> Outcome {
    f.run(&["--status-fd=2", "-bsau", key], PAYLOAD)
}

#[test]
fn git_can_sign_a_commit() {
    let f = Fixture::new();
    let out = git_sign(&f, &V4_SIGN.primary_fpr);
    assert_eq!(out.code, 0, "{}", out.stderr.text());
    assert!(
        out.stdout
            .text()
            .starts_with("-----BEGIN PGP SIGNATURE-----")
    );
    assert!(
        git_accepts_signing(&out.stderr.text()),
        "{}",
        out.stderr.text()
    );
    assert!(
        out.stderr
            .text()
            .contains(&format!("KEY_CONSIDERED {} 0", V4_SIGN.primary_fpr))
    );
}

#[test]
fn git_can_sign_with_the_committer_ident_alone() {
    let f = Fixture::new();
    let out = git_sign(&f, "V4 Fixture <v4@example.test>");
    assert_eq!(out.code, 0, "{}", out.stderr.text());
    assert!(git_accepts_signing(&out.stderr.text()));
}

#[test]
fn git_can_verify_what_was_signed() {
    let f = Fixture::new();
    let signed = git_sign(&f, &V4_SIGN.primary_fpr);
    let sig = f.file("sig.asc", &signed.stdout.bytes());

    let out = f.run(
        &[
            "--keyid-format=long",
            "--status-fd=1",
            "--verify",
            &sig,
            "-",
        ],
        PAYLOAD,
    );
    let status = out.stdout.text();
    assert_eq!(out.code, 0, "{}", out.stderr.text());
    assert!(git_accepts_verification(&status), "{status}");
    assert!(status.contains("TRUST_ULTIMATE"));
    assert!(out.stderr.text().contains("Good signature"));

    // VALIDSIG's tenth field is what git records as the primary key.
    let validsig = status
        .lines()
        .find(|l| l.starts_with("[GNUPG:] VALIDSIG "))
        .unwrap();
    let fields: Vec<&str> = validsig.split(' ').skip(2).collect();
    assert_eq!(fields[9], V4_SIGN.primary_fpr);
    assert_eq!(fields[0], V4_SIGN.sign_subkey_fpr());
}

#[test]
fn a_changed_commit_does_not_verify() {
    let f = Fixture::new();
    let signed = git_sign(&f, &V4_SIGN.primary_fpr);
    let sig = f.file("sig.asc", &signed.stdout.bytes());

    let out = f.run(
        &[
            "--keyid-format=long",
            "--status-fd=1",
            "--verify",
            &sig,
            "-",
        ],
        b"other",
    );
    assert_eq!(out.code, 1);
    assert!(!git_accepts_verification(&out.stdout.text()));
    assert!(out.stdout.text().contains("BADSIG"));
}

#[test]
fn a_signer_outside_the_package_is_unknown() {
    // Signed with a key the package does not hold.
    let other = Fixture::with(&[bin(&V6_SIGN.secret_armored)], "default");
    let signed = other.run(&["-bsa"], PAYLOAD);
    assert_eq!(signed.code, 0, "{}", signed.stderr.text());

    let only_v4 = Fixture::with(&[bin(&V4_SIGN.public_armored)], "default");
    let sig = only_v4.file("sig.asc", &signed.stdout.bytes());
    let out = only_v4.run(&["--status-fd=1", "--verify", &sig, "-"], PAYLOAD);
    assert_eq!(out.code, 1);
    assert!(out.stdout.text().contains("ERRSIG"));
    assert!(out.stdout.text().contains("NO_PUBKEY"));
}

#[test]
fn a_public_key_in_the_package_is_trusted_fully() {
    let signer = Fixture::with(&[bin(&V6_SIGN.secret_armored)], "default");
    let signed = signer.run(&["-bsa"], PAYLOAD);

    let f = Fixture::new(); // holds V6 as a public key only
    let sig = f.file("sig.asc", &signed.stdout.bytes());
    let out = f.run(&["--status-fd=1", "--verify", &sig, "-"], PAYLOAD);
    assert_eq!(out.code, 0, "{}", out.stderr.text());
    assert!(out.stdout.text().contains("TRUST_FULLY"));
}

#[test]
fn clear_signed_text_round_trips_through_files() {
    let f = Fixture::new();
    let input = f.file("note.txt", b"hello\n");
    let output = f.dir.path().join("note.txt.asc");
    let out = f.run(
        &["sign", "--clear", "-o", output.to_str().unwrap(), &input],
        b"",
    );
    assert_eq!(out.code, 0, "{}", out.stderr.text());
    let signed = std::fs::read_to_string(&output).unwrap();
    assert!(signed.starts_with("-----BEGIN PGP SIGNED MESSAGE-----"));

    let out = f.run(&["verify", output.to_str().unwrap()], b"");
    assert_eq!(out.code, 0, "{}", out.stderr.text());
}

#[test]
fn encrypted_and_signed_data_round_trips_through_a_pipe() {
    let f = Fixture::new();
    let fpr = V4_SIGN.primary_fpr.as_str();
    let sealed = f.run(&["encrypt", "-a", "-s", "-r", fpr], b"secret data");
    assert_eq!(sealed.code, 0, "{}", sealed.stderr.text());
    assert!(
        sealed
            .stdout
            .text()
            .starts_with("-----BEGIN PGP MESSAGE-----")
    );

    let opened = f.run(&["decrypt", "--status-fd=2"], &sealed.stdout.bytes());
    assert_eq!(opened.code, 0, "{}", opened.stderr.text());
    assert_eq!(opened.stdout.bytes(), b"secret data");
    assert!(opened.stderr.text().contains("[GNUPG:] DECRYPTION_OKAY"));
    assert!(opened.stderr.text().contains("[GNUPG:] GOODSIG"));
}

#[test]
fn only_keys_in_the_package_are_recipients() {
    let f = Fixture::new();
    let out = f.run(&["encrypt", "-r", "nobody@example.test"], b"x");
    assert_eq!(out.code, 2);
    assert!(out.stderr.text().contains("no such key in the package"));
}

#[test]
fn a_message_for_a_key_without_its_secret_cannot_be_decrypted() {
    let f = Fixture::new();
    let sealed = f.run(&["encrypt", "-r", &V6_SIGN.primary_fpr], b"x");
    assert_eq!(sealed.code, 0, "{}", sealed.stderr.text());
    let opened = f.run(&["decrypt"], &sealed.stdout.bytes());
    assert_eq!(opened.code, 1);
    assert!(opened.stdout.bytes().is_empty());
}

#[test]
fn a_key_without_its_secret_cannot_sign() {
    let f = Fixture::new();
    let out = f.run(&["-bsau", &V6_SIGN.primary_fpr], PAYLOAD);
    assert_eq!(out.code, 2);
    assert!(
        out.stderr
            .text()
            .contains("has no secret key in the package")
    );
}

#[test]
fn a_secret_key_with_a_passphrase_is_refused() {
    let f = Fixture::with(&[bin(A5_V6_SECRET_LOCKED)], "default");
    let out = f.run(&["-bsa"], PAYLOAD);
    assert_eq!(out.code, 2);
    assert!(out.stderr.text().contains("has a passphrase"));
}

#[test]
fn key_management_is_refused() {
    let f = Fixture::new();
    let out = f.run(&["--import"], b"");
    assert_eq!(out.code, 2);
    assert!(out.stderr.text().contains("no key management"));
}

#[test]
fn a_plain_package_needs_its_key() {
    let f = Fixture::new();
    let out = f.run_with(&["-bsa"], PAYLOAD, HashMap::new());
    assert_eq!(out.code, 2);
    assert!(out.stderr.text().contains(ENV_KEY));

    let wrong = HashMap::from([(ENV_KEY.to_string(), format!("gfkp3:{}", "00".repeat(32)))]);
    let out = f.run_with(&["-bsa"], PAYLOAD, wrong);
    assert_eq!(out.code, 2);
    assert!(out.stderr.text().contains("does not open"));

    let key_file = f.file("key.txt", f.key.as_bytes());
    let from_file = HashMap::from([(ENV_KEY_FILE.to_string(), key_file)]);
    assert_eq!(f.run_with(&["-bsa"], PAYLOAD, from_file).code, 0);
}

#[test]
fn packages_are_found_by_name_path_and_variable() {
    let f = Fixture::with(&[bin(&V4_SIGN.secret_armored)], "work");
    // The default name finds nothing here.
    let out = f.run(&["-bsa"], PAYLOAD);
    assert_eq!(out.code, 2);
    assert!(out.stderr.text().contains("default.gfpack"));

    assert_eq!(f.run(&["--keypack", "work", "-bsa"], PAYLOAD).code, 0);
    let path = f.dir.path().join("work.gfpack");
    assert_eq!(
        f.run(&["--keypack", path.to_str().unwrap(), "-bsa"], PAYLOAD)
            .code,
        0
    );
    let mut env = f.env();
    env.insert(ENV_KEYPACK.into(), "work".into());
    assert_eq!(f.run_with(&["-bsa"], PAYLOAD, env).code, 0);
}

#[test]
fn a_pin_package_is_refused_with_a_way_out() {
    let dir = tempfile::tempdir().unwrap();
    let written = write_package(
        &[&bin(&V4_SIGN.secret_armored)],
        ProtectionRequest::Pin(Zeroizing::new(b"1234".to_vec())),
        0,
    )
    .unwrap();
    std::fs::write(dir.path().join("default.gfpack"), &written.package).unwrap();
    let host = Host {
        program_dir: Some(dir.path().to_path_buf()),
        version: "2.3.0".into(),
        env: HashMap::new(),
    };
    let out = run_on(&host, &["-bsa"], PAYLOAD);
    assert_eq!(out.code, 2);
    assert!(out.stderr.text().contains("protected by a PIN"));
}

#[test]
fn the_package_lists_its_keys() {
    let f = Fixture::new();
    let out = f.run(&["keys"], b"");
    assert_eq!(out.code, 0);
    let text = out.stdout.text();
    // As gpg -k: every key as "pub"; -K lists the secret ones as "sec".
    assert!(text.contains(&format!("pub  {}", V4_SIGN.primary_fpr)));
    assert!(text.contains(&format!("pub  {}", V6_SIGN.primary_fpr)));
    assert!(
        f.run(&["-K"], b"")
            .stdout
            .text()
            .contains(&format!("sec  {}", V4_SIGN.primary_fpr))
    );
    assert!(text.contains("exported 2023-11-14"));

    let secret_only = f.run(&["-K"], b"").stdout.text();
    assert!(!secret_only.contains(&V6_SIGN.primary_fpr));
}

#[test]
fn help_and_version_need_no_package() {
    let dir = tempfile::tempdir().unwrap();
    let host = Host {
        program_dir: Some(dir.path().to_path_buf()),
        version: "2.3.0".into(),
        env: HashMap::new(),
    };
    assert!(
        run_on(&host, &["help"], b"")
            .stdout
            .text()
            .contains("Usage: gfpipe")
    );
    let version = run_on(&host, &["--version"], b"").stdout.text();
    assert!(
        version.starts_with("gfpipe (GpgFrontend) 2.3.0\n"),
        "{version}"
    );
    assert!(version.contains("Read-only: keys are managed in GpgFrontend."));
    assert!(version.contains("Key package format: v3\n"));
    let rpgp = format!(
        "OpenPGP engine: rPGP {}\n",
        pgp::VERSION
            .split('.')
            .take(2)
            .collect::<Vec<_>>()
            .join(".")
    );
    assert!(version.ends_with(&rpgp), "{version}");
}

// ----------------------------------------------------- what tools send gpg

/// `fixture`'s secret key with every component locked by `passphrase`.
/// Each test locks its own fixture: the engine's passphrase cache is
/// process-wide.
fn locked(fixture: &crate::testutil::keys::Fixture, passphrase: &str) -> Vec<u8> {
    use pgp::ser::Serialize as _;
    use pgp::types::Password;
    let pw = Password::from(passphrase);
    let mut key = fixture.secret.clone();
    let mut rng = rand::thread_rng();
    key.primary_key.set_password(&mut rng, &pw).unwrap();
    for sub in key.secret_subkeys.iter_mut() {
        sub.key.set_password(&mut rng, &pw).unwrap();
    }
    key.to_bytes().unwrap()
}

/// The fields of a `--with-colons` line.
fn fields(line: &str) -> Vec<&str> {
    line.split(':').collect()
}

fn colon_lines<'a>(text: &'a str, kind: &str) -> Vec<Vec<&'a str>> {
    text.lines()
        .filter(|l| l.starts_with(&format!("{kind}:")))
        .map(fields)
        .collect()
}

#[test]
fn git_tags_and_merges_read_the_trust_of_the_signer() {
    // git merge --verify-signatures and gpg.minTrustLevel read TRUST_*: a
    // key whose secret is in the package is the user's own.
    let f = Fixture::new();
    let signed = git_sign(&f, "v4@example.test");
    let sig = f.file("tag.sig", &signed.stdout.bytes());
    let out = f.run(
        &[
            "--keyid-format=long",
            "--status-fd=1",
            "--verify",
            &sig,
            "-",
        ],
        PAYLOAD,
    );
    assert_eq!(out.code, 0, "{}", out.stderr.text());
    assert!(git_accepts_verification(&out.stdout.text()));
    assert!(out.stdout.text().contains("\n[GNUPG:] TRUST_ULTIMATE"));
}

#[test]
fn pass_init_finds_the_encryption_subkey_in_the_colon_listing() {
    // pass init: gpg --list-keys --with-colons KEY, then the sub lines whose
    // capabilities (field 12) have an "e" and whose validity is not i, d or r.
    let f = Fixture::new();
    let out = f.run(&["--list-keys", "--with-colons", "v4@example.test"], b"");
    assert_eq!(out.code, 0, "{}", out.stderr.text());
    let text = out.stdout.text();
    let encryption: Vec<String> = colon_lines(&text, "sub")
        .into_iter()
        .filter(|f| !"idr".contains(f[1]) && f[11].contains('e'))
        .map(|f| f[4].to_string())
        .collect();
    let expected = &V4_SIGN.enc_subkey_fpr()[24..];
    assert_eq!(encryption, [expected.to_string()]);
}

#[test]
fn pass_insert_and_show_round_trip() {
    let f = Fixture::new();
    let store = f.dir.path().join("site.gpg");
    let store = store.to_str().unwrap();
    // pass insert
    let out = f.run(
        &[
            "-e",
            "-r",
            &V4_SIGN.primary_fpr,
            "-o",
            store,
            "--quiet",
            "--yes",
            "--compress-algo=none",
            "--no-encrypt-to",
            "--batch",
            "--use-agent",
        ],
        b"hunter2\n",
    );
    assert_eq!(out.code, 0, "{}", out.stderr.text());
    // pass show
    let out = f.run(
        &[
            "-d",
            "--quiet",
            "--yes",
            "--compress-algo=none",
            "--no-encrypt-to",
            "--batch",
            "--use-agent",
            store,
        ],
        b"",
    );
    assert_eq!(out.code, 0, "{}", out.stderr.text());
    assert_eq!(out.stdout.bytes(), b"hunter2\n");
}

#[test]
fn a_build_tool_signs_an_artifact_with_a_passphrase_on_standard_input() {
    // maven-gpg-plugin: the passphrase is the first line of standard input,
    // the artifact a named file, the signature a named output.
    let f = Fixture::with(&[locked(&V6_SIGN, "artifact pass")], "default");
    let jar = f.file("lib-1.0.jar", b"PK\x03\x04 a jar");
    let asc = format!("{jar}.asc");
    let out = f.run(
        &[
            "--batch",
            "--no-tty",
            "--pinentry-mode",
            "loopback",
            "--passphrase-fd",
            "0",
            "--local-user",
            &V6_SIGN.primary_fpr,
            "--armor",
            "--detach-sign",
            "--output",
            &asc,
            &jar,
        ],
        b"artifact pass\n",
    );
    crate::cache::PASSWORD_CACHE.remove_by_fpr(&V6_SIGN.primary_fpr);
    assert_eq!(out.code, 0, "{}", out.stderr.text());
    assert!(
        std::fs::read_to_string(&asc)
            .unwrap()
            .starts_with("-----BEGIN PGP SIGNATURE-----")
    );
    let out = f.run(&["--verify", &asc, &jar], b"");
    assert_eq!(out.code, 0, "{}", out.stderr.text());
}

#[test]
fn a_wrong_or_missing_passphrase_fails_without_asking_again() {
    let f = Fixture::with(&[locked(&V4_NISTP256, "right")], "default");
    let pw = f.file("pw.txt", b"wrong\n");
    let out = f.run(
        &["--status-fd=2", "--passphrase-file", &pw, "-bsa"],
        PAYLOAD,
    );
    assert_eq!(out.code, 1, "{}", out.stderr.text());
    assert!(out.stderr.text().contains("[GNUPG:] BAD_PASSPHRASE"));

    let out = f.run(&["-bsa"], PAYLOAD);
    assert_eq!(out.code, 2);
    assert!(out.stderr.text().contains("--passphrase-fd"));
}

#[test]
fn rpm_signs_a_package_with_a_binary_detached_signature() {
    // rpm's %__gpg_sign_cmd.
    let f = Fixture::new();
    let rpm = f.file("pkg.rpm", b"rpm header and payload");
    let sig = f.dir.path().join("pkg.sig");
    let sig = sig.to_str().unwrap();
    let out = f.run(
        &[
            "--no-verbose",
            "--no-armor",
            "--no-secmem-warning",
            "-u",
            "V4 Fixture",
            "-sbo",
            sig,
            "--digest-algo",
            "sha256",
            "--",
            &rpm,
        ],
        b"",
    );
    assert_eq!(out.code, 0, "{}", out.stderr.text());
    let bytes = std::fs::read(sig).unwrap();
    assert_eq!(bytes[0] & 0x80, 0x80, "binary, not armored");
    assert_eq!(f.run(&["--verify", sig, &rpm], b"").code, 0);
}

#[test]
fn clearsigning_a_file_writes_it_next_to_it_and_replaces_only_with_yes() {
    // debsign, release scripts: gpg --clearsign FILE writes FILE.asc.
    let f = Fixture::new();
    let changes = f.file("pkg.changes", b"Format: 1.8\n");
    assert_eq!(f.run(&["--clearsign", &changes], b"").code, 0);
    let asc = format!("{changes}.asc");
    assert!(
        std::fs::read_to_string(&asc)
            .unwrap()
            .starts_with("-----BEGIN PGP SIGNED MESSAGE-----")
    );
    assert_eq!(f.run(&["--verify", &asc], b"").code, 0);

    let again = f.run(&["--clearsign", &changes], b"");
    assert_eq!(again.code, 2);
    assert!(again.stderr.text().contains("--yes"));
    assert_eq!(f.run(&["--yes", "--clearsign", &changes], b"").code, 0);
}

#[test]
fn files_get_gpg_names_and_a_detached_signature_finds_its_data() {
    let f = Fixture::new();
    let fpr = V4_SIGN.primary_fpr.as_str();
    let doc = f.file("report.pdf", b"%PDF-1.7 report");

    // gpg -e -r KEY FILE -> FILE.gpg; gpg -d FILE.gpg -> standard output.
    assert_eq!(f.run(&["-e", "-r", fpr, &doc], b"").code, 0);
    let out = f.run(&["-d", &format!("{doc}.gpg")], b"");
    assert_eq!(out.code, 0, "{}", out.stderr.text());
    assert_eq!(out.stdout.bytes(), b"%PDF-1.7 report");

    // gpg -b FILE -> FILE.sig, gpg -ab FILE -> FILE.asc; gpg --verify FILE.sig
    // finds FILE.
    assert_eq!(f.run(&["-b", &doc], b"").code, 0);
    assert_eq!(f.run(&["--verify", &format!("{doc}.sig")], b"").code, 0);
    assert_eq!(f.run(&["-ab", &doc], b"").code, 0);
    let out = f.run(&["--status-fd=1", "--verify", &format!("{doc}.asc")], b"");
    assert_eq!(out.code, 0, "{}", out.stderr.text());
    assert!(out.stdout.text().contains("[GNUPG:] VALIDSIG"));

    // A detached signature whose data is not there.
    let lone = f.file("lone.sig", &std::fs::read(format!("{doc}.sig")).unwrap());
    let out = f.run(&["--verify", &lone], b"");
    assert_eq!(out.code, 2);
    assert!(out.stderr.text().contains("detached signature"));
}

#[test]
fn passphrase_encryption_round_trips_and_needs_no_key_package() {
    // A backup script: gpg -c --batch --passphrase-file F FILE.
    let f = Fixture::new();
    let backup = f.file("db.dump", b"rows");
    let pw = f.file("pw", b"backup secret\n");
    let empty = tempfile::tempdir().unwrap();
    let host = Host {
        program_dir: Some(empty.path().to_path_buf()),
        version: "2.3.0".into(),
        env: HashMap::new(),
    };
    let out = run_on(
        &host,
        &["-c", "--batch", "--passphrase-file", &pw, &backup],
        b"",
    );
    assert_eq!(out.code, 0, "{}", out.stderr.text());

    let sealed = format!("{backup}.gpg");
    let out = f.run(
        &["-d", "--batch", "--passphrase", "backup secret", &sealed],
        b"",
    );
    assert_eq!(out.code, 0, "{}", out.stderr.text());
    assert_eq!(out.stdout.bytes(), b"rows");

    let out = f.run(&["-d", "--batch", &sealed], b"");
    assert_eq!(out.code, 1);
    assert!(out.stderr.text().contains("encrypted with a passphrase"));

    let out = run_on(&host, &["-c", "--yes", &backup], b"");
    assert_eq!(out.code, 2);
    assert!(out.stderr.text().contains("--passphrase"));
}

#[test]
fn secrets_tools_encrypt_and_decrypt_through_pipes() {
    // sops and similar: armored, standard streams, status on fd 2.
    let f = Fixture::new();
    let out = f.run(
        &[
            "--batch",
            "--no-default-recipient",
            "--encrypt",
            "--armor",
            "--recipient",
            &V4_SIGN.primary_fpr,
            "--trust-model",
            "always",
        ],
        b"data key",
    );
    assert_eq!(out.code, 0, "{}", out.stderr.text());
    let opened = f.run(
        &["--batch", "--status-fd=2", "--decrypt"],
        &out.stdout.bytes(),
    );
    assert_eq!(opened.code, 0, "{}", opened.stderr.text());
    assert_eq!(opened.stdout.bytes(), b"data key");
    let status = opened.stderr.text();
    assert!(status.contains("[GNUPG:] ENC_TO "));
    assert!(status.contains("[GNUPG:] DECRYPTION_OKAY"));
    assert!(status.contains("[GNUPG:] END_DECRYPTION"));
}

#[test]
fn encrypting_to_oneself_needs_no_recipient() {
    let f = Fixture::new();
    let out = f.run(&["-e", "--default-recipient-self"], b"note to self");
    assert_eq!(out.code, 0, "{}", out.stderr.text());
    let opened = f.run(&["-d"], &out.stdout.bytes());
    assert_eq!(opened.stdout.bytes(), b"note to self");

    let out = f.run(&["-e"], b"x");
    assert_eq!(out.code, 2);
    assert!(out.stderr.text().contains("recipient"));
}

#[test]
fn decrypting_a_message_that_is_only_signed_gives_its_content() {
    let f = Fixture::new();
    for (sign, kind) in [("-sa", "inline"), ("--clearsign", "clear")] {
        let signed = f.run(&[sign], b"signed only\n");
        assert_eq!(signed.code, 0, "{kind}: {}", signed.stderr.text());
        let out = f.run(&["--status-fd=2", "-d"], &signed.stdout.bytes());
        assert_eq!(out.code, 0, "{kind}: {}", out.stderr.text());
        assert_eq!(out.stdout.bytes(), b"signed only\n", "{kind}");
        assert!(out.stderr.text().contains("[GNUPG:] GOODSIG"), "{kind}");
    }
}

#[test]
fn public_keys_are_exported_and_secret_keys_never() {
    let f = Fixture::new();
    let out = f.run(&["--armor", "--export", "v4@example.test"], b"");
    assert_eq!(out.code, 0, "{}", out.stderr.text());
    let text = out.stdout.text();
    assert!(text.starts_with("-----BEGIN PGP PUBLIC KEY BLOCK-----"));
    let (cert, _) = SignedPublicKey::from_string(&text).unwrap();
    assert_eq!(
        hex::encode_upper(cert.primary_key.fingerprint().as_bytes()),
        V4_SIGN.primary_fpr
    );

    // Every key, binary.
    let all = f.run(&["--export"], b"");
    assert_eq!(all.code, 0);
    let keys: Vec<_> = SignedPublicKey::from_bytes_many(&all.stdout.bytes()[..])
        .unwrap()
        .collect();
    assert_eq!(keys.len(), 2);

    for opt in ["--export-secret-keys", "--export-secret-subkeys"] {
        let out = f.run(&[opt], b"");
        assert_eq!(out.code, 2);
        assert!(out.stderr.text().contains("no key management"));
    }
    assert_eq!(f.run(&["--export", "nobody@example.test"], b"").code, 2);
}

#[test]
fn the_colon_listing_follows_gpg() {
    let f = Fixture::new();
    let out = f.run(&["--list-secret-keys", "--with-colons"], b"");
    assert_eq!(out.code, 0, "{}", out.stderr.text());
    let text = out.stdout.text();

    let sec = colon_lines(&text, "sec");
    assert_eq!(sec.len(), 1, "only keys with their secret");
    let sec = &sec[0];
    assert_eq!(sec.len(), 21, "20 fields and the trailing colon");
    assert_eq!(sec[1], "u", "the user's own key");
    assert_eq!(sec[3], "27", "Ed25519 (RFC 9580)");
    assert_eq!(sec[4], &V4_SIGN.primary_fpr[24..]);
    assert!(sec[5].parse::<u64>().unwrap() > 0, "creation time");
    assert_eq!(sec[14], "+", "the secret is here");
    assert_eq!(sec[16], "ed25519");
    assert_eq!(
        sec[11], "scESC",
        "its own and the whole key's, in gpg's order"
    );

    let fprs: Vec<&str> = colon_lines(&text, "fpr").iter().map(|f| f[9]).collect();
    assert_eq!(fprs[0], V4_SIGN.primary_fpr);
    assert!(fprs.contains(&V4_SIGN.sign_subkey_fpr()));
    assert!(
        colon_lines(&text, "uid")
            .iter()
            .any(|f| f[9] == "V4 Fixture <v4@example.test>")
    );
    assert_eq!(colon_lines(&text, "ssb").len(), 2);

    // --list-keys: every key, as pub and sub; a public-only key fully valid.
    let text = f.run(&["-k", "--with-colons"], b"").stdout.text();
    let public = colon_lines(&text, "pub");
    assert_eq!(public.len(), 2);
    assert!(public.iter().any(|f| f[1] == "f" && f[14].is_empty()));
    assert!(colon_lines(&text, "sec").is_empty());

    // --fingerprint KEY lists that key only; an unknown one is an error.
    let text = f
        .run(&["--fingerprint", &V6_SIGN.primary_fpr], b"")
        .stdout
        .text();
    assert!(text.contains(&V6_SIGN.primary_fpr));
    assert!(!text.contains(&V4_SIGN.primary_fpr));
    assert_eq!(f.run(&["-k", "nobody@example.test"], b"").code, 2);
}

#[test]
fn colons_in_a_user_id_are_escaped() {
    assert_eq!(
        super::ops::colon_escape_for_tests("a:b\\c\n"),
        "a\\x3ab\\x5cc\\x0a"
    );
    assert_eq!(super::ops::colon_escape_for_tests("Jürgen"), "Jürgen");
}

#[test]
fn other_keyrings_are_refused_with_a_pointer_to_the_package() {
    let f = Fixture::new();
    for opt in ["--homedir", "--keyring"] {
        let out = f.run(&[opt, "/somewhere", "-bsa"], PAYLOAD);
        assert_eq!(out.code, 2, "{opt}");
        assert!(out.stderr.text().contains("--keypack"), "{opt}");
    }
}

#[test]
fn the_status_can_go_to_a_file() {
    let f = Fixture::new();
    let path = f.dir.path().join("status.txt");
    let out = f.run(&["--status-file", path.to_str().unwrap(), "-bsa"], PAYLOAD);
    assert_eq!(out.code, 0, "{}", out.stderr.text());
    assert!(
        std::fs::read_to_string(&path)
            .unwrap()
            .contains("[GNUPG:] SIG_CREATED ")
    );
}

// ------------------------------------------------------------ gpg interop

mod gpg_interop {
    //! gfpipe's output read by gpg, and gpg's read by gfpipe.

    use super::*;
    use crate::testutil::interop::Gnupg;

    /// A package with V4 (secret) and the gpg home's own key (public), and the
    /// gpg home knowing V4's public key.
    fn setup() -> Option<(Fixture, Gnupg)> {
        let gpg = Gnupg::new()?;
        gpg.import(&V4_NISTP256.public_armored);
        let f = Fixture::with(
            &[bin(&V4_NISTP256.secret_armored), bin(&gpg.cert)],
            "default",
        );
        Some((f, gpg))
    }

    fn gpg_ok(out: &std::process::Output, what: &str) -> Vec<u8> {
        assert!(
            out.status.success(),
            "{what}: {}",
            String::from_utf8_lossy(&out.stderr)
        );
        out.stdout.clone()
    }

    #[test]
    fn gpg_verifies_what_gfpipe_signs() {
        let Some((f, gpg)) = setup() else { return };
        let data = f.file("data.txt", b"interop\n");
        assert_eq!(f.run(&["-ab", &data], b"").code, 0);
        let sig = format!("{data}.asc");
        let out = gpg.gpg(["--status-fd=1", "--verify", &sig, &data]);
        let status = String::from_utf8(gpg_ok(&out, "gpg --verify")).unwrap();
        assert!(status.contains("[GNUPG:] GOODSIG"));

        assert_eq!(
            f.run(&["--clearsign", "-o", &format!("{data}.clear"), &data], b"")
                .code,
            0
        );
        gpg_ok(&gpg.gpg(["--verify", &format!("{data}.clear")]), "clear");
    }

    #[test]
    fn gfpipe_verifies_what_gpg_signs() {
        let Some((f, gpg)) = setup() else { return };
        let data = f.file("data.txt", b"interop\n");
        let sig = format!("{data}.sig");
        gpg_ok(
            &gpg.gpg(["--passphrase", "", "-u", &gpg.fpr, "-b", "-o", &sig, &data]),
            "gpg -b",
        );
        let out = f.run(&["--status-fd=1", "--verify", &sig], b"");
        assert_eq!(out.code, 0, "{}", out.stderr.text());
        assert!(out.stdout.text().contains("TRUST_FULLY"));
    }

    #[test]
    fn gpg_decrypts_what_gfpipe_encrypts_and_back() {
        let Some((f, gpg)) = setup() else { return };
        let sealed = f.run(&["-ea", "-r", &gpg.fpr], b"to gpg");
        assert_eq!(sealed.code, 0, "{}", sealed.stderr.text());
        let path = f.file("to-gpg.asc", &sealed.stdout.bytes());
        let plain = gpg_ok(&gpg.gpg(["--passphrase", "", "-d", &path]), "gpg -d");
        assert_eq!(plain, b"to gpg");

        let from_gpg = gpg.encrypt(&V4_NISTP256.primary_fpr, b"from gpg", "zlib", true);
        let out = f.run(&["--status-fd=2", "-d"], &from_gpg);
        assert_eq!(out.code, 0, "{}", out.stderr.text());
        assert_eq!(out.stdout.bytes(), b"from gpg");
        assert!(out.stderr.text().contains("[GNUPG:] GOODSIG"));
    }

    #[test]
    fn passphrase_messages_cross_both_ways() {
        let Some((f, gpg)) = setup() else { return };
        let data = f.file("secret.txt", b"shared");
        assert_eq!(
            f.run(&["-c", "--passphrase", "both ways", &data], b"").code,
            0
        );
        let plain = gpg_ok(
            &gpg.gpg(["--passphrase", "both ways", "-d", &format!("{data}.gpg")]),
            "gpg -d",
        );
        assert_eq!(plain, b"shared");

        let from_gpg = f.dir.path().join("gpg.gpg");
        gpg_ok(
            &gpg.gpg([
                "--passphrase",
                "both ways",
                "-c",
                "-o",
                from_gpg.to_str().unwrap(),
                &data,
            ]),
            "gpg -c",
        );
        let out = f.run(
            &[
                "-d",
                "--passphrase",
                "both ways",
                from_gpg.to_str().unwrap(),
            ],
            b"",
        );
        assert_eq!(out.code, 0, "{}", out.stderr.text());
        assert_eq!(out.stdout.bytes(), b"shared");
    }

    #[test]
    fn the_colon_listing_of_a_gpg_key_matches_gpg() {
        let Some((f, gpg)) = setup() else { return };
        let ours = f.run(&["-k", "--with-colons", &gpg.fpr], b"").stdout.text();
        let theirs = String::from_utf8(gpg_ok(
            &gpg.gpg(["-k", "--with-colons", &gpg.fpr]),
            "gpg -k",
        ))
        .unwrap();
        for kind in ["pub", "sub"] {
            let a = colon_lines(&ours, kind);
            let b = colon_lines(&theirs, kind);
            assert_eq!(a.len(), b.len(), "{kind}");
            for (a, b) in a.iter().zip(&b) {
                // Length, algorithm, key ID, created, expires, capabilities,
                // curve.
                for i in [2, 3, 4, 5, 6, 11, 16] {
                    assert_eq!(a[i], b[i], "{kind} field {}", i + 1);
                }
            }
        }
        let fprs = |t: &str| -> Vec<String> {
            colon_lines(t, "fpr")
                .iter()
                .map(|f| f[9].to_string())
                .collect()
        };
        assert_eq!(fprs(&ours), fprs(&theirs));
    }
}

// ------------------------------------------------- what lies beside gfpipe

/// A package and its key in `dir` under `name`, as GpgFrontend's export with
/// gfpipe writes them.
fn beside(dir: &Path, keys: &[Vec<u8>], name: &str) {
    let keys: Vec<&[u8]> = keys.iter().map(Vec::as_slice).collect();
    let written = write_package(&keys, ProtectionRequest::Plain, 1_700_000_000).unwrap();
    std::fs::write(dir.join(format!("{name}.gfpack")), &written.package).unwrap();
    std::fs::write(
        dir.join(format!("{name}.gfkey")),
        plain_key_text(written.secret.as_ref().unwrap()).as_bytes(),
    )
    .unwrap();
}

fn host_in(dir: &Path, env: HashMap<String, String>) -> Host {
    Host {
        program_dir: Some(dir.to_path_buf()),
        version: String::new(),
        env,
    }
}

#[test]
fn gfpipe_runs_with_the_package_and_key_beside_it() {
    let dir = tempfile::tempdir().unwrap();
    beside(dir.path(), &[bin(&V4_SIGN.secret_armored)], "default");
    // No option, no variable: what lies beside gfpipe.
    let out = run_on(&host_in(dir.path(), HashMap::new()), &["-bsa"], PAYLOAD);
    assert_eq!(out.code, 0, "{}", out.stderr.text());
    assert!(
        out.stdout
            .text()
            .starts_with("-----BEGIN PGP SIGNATURE-----")
    );
}

#[test]
fn nothing_beside_gfpipe_says_how_to_get_it() {
    let dir = tempfile::tempdir().unwrap();
    let out = run_on(&host_in(dir.path(), HashMap::new()), &["-bsa"], PAYLOAD);
    assert_eq!(out.code, 2);
    let text = out.stderr.text();
    assert!(
        text.contains("default.gfpack") && text.contains("--keypack"),
        "{text}"
    );

    // A package without its key: the ways to give it.
    beside(dir.path(), &[bin(&V4_SIGN.secret_armored)], "default");
    std::fs::remove_file(dir.path().join("default.gfkey")).unwrap();
    let out = run_on(&host_in(dir.path(), HashMap::new()), &["-bsa"], PAYLOAD);
    assert_eq!(out.code, 2);
    assert!(out.stderr.text().contains("--key-file"));
}

#[test]
fn a_package_named_by_path_brings_its_own_key_file() {
    let dir = tempfile::tempdir().unwrap();
    beside(dir.path(), &[bin(&V4_SIGN.secret_armored)], "default");
    let other = tempfile::tempdir().unwrap();
    beside(other.path(), &[bin(&V6_SIGN.secret_armored)], "work");

    // --keypack PATH: work.gfkey beside work.gfpack, not default.gfkey.
    let work = other.path().join("work.gfpack");
    let out = run_on(
        &host_in(dir.path(), HashMap::new()),
        &["--keypack", work.to_str().unwrap(), "-K", "--with-colons"],
        b"",
    );
    assert_eq!(out.code, 0, "{}", out.stderr.text());
    assert!(out.stdout.text().contains(&V6_SIGN.primary_fpr));
}

#[test]
fn explicit_options_win_over_what_lies_beside_gfpipe() {
    let dir = tempfile::tempdir().unwrap();
    beside(dir.path(), &[bin(&V4_SIGN.secret_armored)], "default");
    beside(dir.path(), &[bin(&V6_SIGN.secret_armored)], "work");
    beside(dir.path(), &[bin(&V4_NISTP256.secret_armored)], "other");
    let listed = |host: &Host, argv: &[&str]| {
        let mut full = argv.to_vec();
        full.extend(["-K", "--with-colons"]);
        let out = run_on(host, &full, b"");
        assert_eq!(out.code, 0, "{argv:?}: {}", out.stderr.text());
        out.stdout.text()
    };

    // GFPIPE_KEYPACK over the default, --keypack over GFPIPE_KEYPACK.
    let env = HashMap::from([(ENV_KEYPACK.to_string(), "work".to_string())]);
    let host = host_in(dir.path(), env);
    assert!(listed(&host, &[]).contains(&V6_SIGN.primary_fpr));
    assert!(listed(&host, &["--keypack", "other"]).contains(&V4_NISTP256.primary_fpr));

    // A key given explicitly wins over the key file beside the package: the
    // wrong key in GFPIPE_KEY fails although default.gfkey would open it.
    let wrong = format!("gfkp3:{}", "00".repeat(32));
    let host = host_in(dir.path(), HashMap::from([(ENV_KEY.to_string(), wrong)]));
    let out = run_on(&host, &["-K"], b"");
    assert_eq!(out.code, 2);
    assert!(out.stderr.text().contains("does not open"));

    // --key-file wins over GFPIPE_KEY.
    let key_file = dir.path().join("default.gfkey");
    assert!(
        listed(&host, &["--key-file", key_file.to_str().unwrap()]).contains(&V4_SIGN.primary_fpr)
    );
}

// ------------------------------------------------------------- native pipes

/// More than gfpipe looks at to tell a message's kind, so the rest streams.
fn big_payload() -> Vec<u8> {
    (0..3 * 1024 * 1024u32)
        .map(|i| (i.wrapping_mul(2_654_435_761) >> 24) as u8)
        .collect()
}

#[test]
fn large_messages_stream_through_pipes() {
    let f = Fixture::new();
    let data = big_payload();
    for armor in [false, true] {
        let mut argv = vec!["-e", "-r", V4_SIGN.primary_fpr.as_str()];
        if armor {
            argv.push("-a");
        }
        let sealed = f.run(&argv, &data);
        assert_eq!(sealed.code, 0, "{}", sealed.stderr.text());

        let opened = f.run(&["--status-fd=2", "-d"], &sealed.stdout.bytes());
        assert_eq!(opened.code, 0, "armor={armor}: {}", opened.stderr.text());
        assert!(opened.stdout.bytes() == data, "armor={armor}");
        assert!(opened.stderr.text().contains("[GNUPG:] ENC_TO "));
    }
}

#[test]
fn a_damaged_message_puts_nothing_into_the_pipe() {
    let f = Fixture::new();
    let sealed = f.run(&["-e", "-r", &V4_SIGN.primary_fpr], &big_payload());
    assert_eq!(sealed.code, 0, "{}", sealed.stderr.text());
    let mut damaged = sealed.stdout.bytes();
    let at = damaged.len() - 100;
    damaged[at] ^= 1;

    let opened = f.run(&["--status-fd=2", "-d"], &damaged);
    assert_ne!(opened.code, 0);
    assert!(
        opened.stdout.bytes().is_empty(),
        "no unauthenticated plaintext"
    );
    assert!(opened.stderr.text().contains("[GNUPG:] DECRYPTION_FAILED"));
}

#[test]
fn large_signed_data_verifies_from_a_pipe() {
    let f = Fixture::new();
    let data = big_payload();
    let signed = f.run(&["-bsa"], &data);
    assert_eq!(signed.code, 0, "{}", signed.stderr.text());
    let sig = f.file("big.asc", &signed.stdout.bytes());
    let out = f.run(&["--status-fd=1", "--verify", &sig, "-"], &data);
    assert_eq!(out.code, 0, "{}", out.stderr.text());
    assert!(git_accepts_verification(&out.stdout.text()));

    let mut changed = data.clone();
    changed[1_500_000] ^= 1;
    let out = f.run(&["--status-fd=1", "--verify", &sig, "-"], &changed);
    assert_eq!(out.code, 1);
    assert!(out.stdout.text().contains("[GNUPG:] BADSIG"));
}

/// Text past what gfpipe looks at to tell a message's kind, with lines a
/// clear-text signature escapes.
fn big_text() -> Vec<u8> {
    let mut text = Vec::new();
    let mut i = 0u32;
    while text.len() < 3 * 1024 * 1024 {
        text.extend_from_slice(format!("line {i}\n- dashed\nFrom here\n").as_bytes());
        i += 1;
    }
    text
}

#[test]
fn large_signed_messages_verify_and_open_from_pipes() {
    let f = Fixture::new();
    let data = big_payload();
    let text = big_text();
    for (argv, content) in [
        (&["-s"][..], &data),
        (&["-sa"][..], &data),
        (&["--clearsign"][..], &text),
    ] {
        let signed = f.run(argv, content);
        assert_eq!(signed.code, 0, "{argv:?}: {}", signed.stderr.text());

        let out = f.run(&["--status-fd=1", "--verify"], &signed.stdout.bytes());
        assert_eq!(out.code, 0, "{argv:?}: {}", out.stderr.text());
        assert!(git_accepts_verification(&out.stdout.text()), "{argv:?}");

        let opened = f.run(&["--status-fd=2", "-d"], &signed.stdout.bytes());
        assert_eq!(opened.code, 0, "{argv:?}: {}", opened.stderr.text());
        assert!(opened.stdout.bytes() == *content, "{argv:?}");
        assert!(
            opened.stderr.text().contains("[GNUPG:] GOODSIG "),
            "{argv:?}"
        );
    }
}

#[test]
fn a_damaged_signed_message_puts_nothing_into_the_pipe() {
    let f = Fixture::new();
    let signed = f.run(&["-s"], &big_payload());
    assert_eq!(signed.code, 0, "{}", signed.stderr.text());
    let bytes = signed.stdout.bytes();
    let cut = &bytes[..bytes.len() - 40];

    let opened = f.run(&["--status-fd=2", "-d"], cut);
    assert_ne!(opened.code, 0);
    assert!(opened.stdout.bytes().is_empty(), "no unverified content");
    assert!(opened.stderr.text().contains("[GNUPG:] NODATA 2"));
}

#[test]
fn a_pasted_detached_signature_still_finds_its_data() {
    let f = Fixture::new();
    let data = b"signed beside its signature\n";
    let signed = f.run(&["-bsa"], data);
    assert_eq!(signed.code, 0, "{}", signed.stderr.text());
    // Indented base64, as a signature pasted from a mail often is.
    let mut in_body = false;
    let mut pasted = String::new();
    for line in signed.stdout.text().lines() {
        if in_body && !line.starts_with("-----") {
            pasted.push_str(&format!("    {line}  \n"));
        } else {
            pasted.push_str(line);
            pasted.push('\n');
        }
        in_body |= line.is_empty();
    }
    f.file("data.txt", data);
    let sig = f.file("data.txt.asc", pasted.as_bytes());
    let out = f.run(&["--status-fd=1", "--verify", &sig], b"");
    assert_eq!(out.code, 0, "{}", out.stderr.text());
    assert!(git_accepts_verification(&out.stdout.text()));
}
