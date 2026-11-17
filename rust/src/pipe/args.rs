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

//! gfpipe's command line: its own subcommands, and the part of gpg's that
//! git, pass, build tools and scripts use. Anything else is refused, never
//! ignored: key management with a pointer to GpgFrontend, other keyrings with
//! a pointer to the key package.

/// What kind of signature `sign` makes.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum SignKind {
    Inline,
    Detached,
    Clear,
}

/// The operation asked for.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Command {
    Sign(SignKind),
    Verify,
    Encrypt {
        sign: bool,
    },
    /// Encryption with a passphrase only (`-c`).
    Symmetric,
    Decrypt,
    ListKeys {
        secret: bool,
    },
    /// The public certificates of keys in the package.
    Export,
    Version,
    Help,
}

/// Where a passphrase comes from; gfpipe never prompts.
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum Passphrase {
    Text(String),
    /// The first line of a file.
    File(String),
    /// The first line read from a file descriptor; 0 is standard input.
    Fd(i32),
}

/// A parsed command line.
#[derive(Debug, Default)]
pub struct Args {
    pub command: Option<Command>,
    pub local_users: Vec<String>,
    pub recipients: Vec<String>,
    /// `--default-recipient`, used when no recipient is named.
    pub default_recipient: Option<String>,
    /// `--default-recipient-self`: the package's own secret key.
    pub default_recipient_self: bool,
    pub armor: bool,
    pub output: Option<String>,
    /// `--yes`: an existing output file may be replaced.
    pub yes: bool,
    pub status_fd: Option<i32>,
    pub status_file: Option<String>,
    /// `--with-colons`: the machine-readable listing.
    pub with_colons: bool,
    pub passphrase: Option<Passphrase>,
    pub keypack: Option<String>,
    /// `--key-file`: the file holding a plain package's key.
    pub key_file: Option<String>,
    /// File arguments, in order; `-` is standard input. Key patterns for a
    /// listing or an export.
    pub inputs: Vec<String>,
}

/// Why a command line was refused.
#[derive(Debug, PartialEq, Eq)]
pub enum ParseError {
    /// A gpg key-management command.
    KeyManagement(String),
    /// An option naming another place to take keys from.
    OtherKeyring(String),
    /// Anything else gfpipe does not know.
    Unsupported(String),
    /// An option that needs a value was last.
    MissingValue(String),
    /// Two operations at once, or none.
    NoOperation,
    Conflict(String),
}

impl std::fmt::Display for ParseError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            ParseError::KeyManagement(opt) => write!(
                f,
                "{opt}: gfpipe does no key management. Manage keys in GpgFrontend and export \
                 a key package for gfpipe."
            ),
            ParseError::OtherKeyring(opt) => write!(
                f,
                "{opt}: gfpipe takes keys only from its key package; name another package \
                 with --keypack or GFPIPE_KEYPACK"
            ),
            ParseError::Unsupported(opt) => write!(f, "{opt}: unsupported option"),
            ParseError::MissingValue(opt) => write!(f, "{opt} needs a value"),
            ParseError::NoOperation => {
                write!(f, "nothing to do; try 'gfpipe help'")
            }
            ParseError::Conflict(what) => write!(f, "{what} cannot be combined"),
        }
    }
}

/// gpg commands that change keys or the trust database, or hand out secrets.
const KEY_MANAGEMENT: &[&str] = &[
    "--import",
    "--export-secret-keys",
    "--export-secret-subkeys",
    "--export-ssh-key",
    "--gen-key",
    "--generate-key",
    "--full-gen-key",
    "--full-generate-key",
    "--quick-gen-key",
    "--quick-generate-key",
    "--quick-add-key",
    "--quick-add-uid",
    "--quick-revoke-key",
    "--quick-revoke-uid",
    "--quick-set-expire",
    "--quick-set-primary-uid",
    "--quick-sign-key",
    "--quick-lsign-key",
    "--edit-key",
    "--sign-key",
    "--lsign-key",
    "--delete-key",
    "--delete-keys",
    "--delete-secret-key",
    "--delete-secret-keys",
    "--delete-secret-and-public-key",
    "--delete-secret-and-public-keys",
    "--passwd",
    "--change-passphrase",
    "--gen-revoke",
    "--generate-revocation",
    "--recv-keys",
    "--receive-keys",
    "--send-keys",
    "--refresh-keys",
    "--search-keys",
    "--fetch-keys",
    "--locate-keys",
    "--locate-external-keys",
    "--update-trustdb",
    "--check-trustdb",
    "--import-ownertrust",
    "--export-ownertrust",
    "--card-edit",
    "--edit-card",
    "--card-status",
];

/// gpg options that choose where keys come from.
const OTHER_KEYRING: &[&str] = &[
    "--homedir",
    "--keyring",
    "--secret-keyring",
    "--primary-keyring",
    "--no-default-keyring",
    "--options",
];

/// gpg options that change nothing for gfpipe and are accepted as such: the
/// package's keys are trusted, nothing is fetched, nothing is prompted for,
/// and the engine chooses algorithms by the recipients' preferences.
const NO_OPS: &[&str] = &[
    "--batch",
    "--no-batch",
    "--no-tty",
    "--quiet",
    "--verbose",
    "--no-verbose",
    "--no-greeting",
    "--no-permission-warning",
    "--no-secmem-warning",
    "--with-fingerprint",
    "--with-subkey-fingerprint",
    "--with-subkey-fingerprints",
    "--with-keygrip",
    "--fixed-list-mode",
    "--no-encrypt-to",
    "--no-default-recipient",
    "--always-trust",
    "--no-auto-key-locate",
    "--no-auto-key-retrieve",
    "--auto-key-retrieve",
    "--disable-dirmngr",
    "--use-agent",
    "--no-use-agent",
    "--lock-never",
    "--lock-once",
    "--lock-multiple",
    "--exit-on-status-write-error",
    "--utf8-strings",
    "--no-utf8-strings",
    "--textmode",
    "--no-textmode",
    "--emit-version",
    "--no-emit-version",
    "--no-comments",
    "--throw-keyids",
    "--no-throw-keyids",
    "--no-mdc-warning",
    "--ignore-time-conflict",
    "--ignore-valid-from",
    "--ignore-mdc-error",
    "--allow-weak-digest-algos",
    "--no-symkey-cache",
    "--force-mdc",
    "--no-force-mdc",
];

/// gpg options that take a value gfpipe does not need.
const NO_OP_WITH_VALUE: &[&str] = &[
    "--keyid-format",
    "--pinentry-mode",
    "--trust-model",
    "--compress-algo",
    "--compress-level",
    "--bzip2-compress-level",
    "--cipher-algo",
    "--digest-algo",
    "--cert-digest-algo",
    "--s2k-mode",
    "--s2k-count",
    "--s2k-cipher-algo",
    "--s2k-digest-algo",
    "--personal-cipher-preferences",
    "--personal-digest-preferences",
    "--personal-compress-preferences",
    "--comment",
    "--set-filename",
    "--display-charset",
    "--charset",
    "--logger-fd",
    "--logger-file",
    "--log-file",
    "--keyserver",
    "--keyserver-options",
    "--auto-key-locate",
    "--passphrase-repeat",
    "--list-options",
    "--verify-options",
    "--export-options",
    "--compliance",
    "--max-output",
    "--agent-program",
    "--dirmngr-program",
];

#[derive(Default)]
struct Flags {
    sign: bool,
    detach: bool,
    clear: bool,
    encrypt: bool,
    symmetric: bool,
    decrypt: bool,
    verify: bool,
    list: bool,
    list_secret: bool,
    export: bool,
    version: bool,
    help: bool,
}

/// Parse `argv` without the program name.
pub fn parse(argv: &[String]) -> Result<Args, ParseError> {
    let mut args = Args::default();
    let mut flags = Flags::default();
    let mut rest = argv.iter().peekable();

    // A subcommand may come first.
    if let Some(first) = rest.peek() {
        let consumed = match first.as_str() {
            "sign" => {
                flags.sign = true;
                true
            }
            "verify" => {
                flags.verify = true;
                true
            }
            "encrypt" => {
                flags.encrypt = true;
                true
            }
            "decrypt" => {
                flags.decrypt = true;
                true
            }
            "keys" => {
                flags.list = true;
                true
            }
            "export" => {
                flags.export = true;
                true
            }
            "version" => {
                flags.version = true;
                true
            }
            "help" => {
                flags.help = true;
                true
            }
            _ => false,
        };
        if consumed {
            rest.next();
        }
    }

    let mut only_files = false;
    while let Some(arg) = rest.next() {
        if only_files || arg == "-" || !arg.starts_with('-') {
            args.inputs.push(arg.clone());
            continue;
        }
        if arg == "--" {
            only_files = true;
            continue;
        }

        if let Some(long) = arg.strip_prefix("--") {
            let (name, inline) = match long.split_once('=') {
                Some((n, v)) => (format!("--{n}"), Some(v.to_string())),
                None => (arg.clone(), None),
            };
            let mut value = |name: &str| -> Result<String, ParseError> {
                match &inline {
                    Some(v) => Ok(v.clone()),
                    None => rest
                        .next()
                        .cloned()
                        .ok_or_else(|| ParseError::MissingValue(name.to_string())),
                }
            };
            let fd = |name: &str, v: String| -> Result<i32, ParseError> {
                v.parse()
                    .map_err(|_| ParseError::Unsupported(format!("{name}={v}")))
            };
            match name.as_str() {
                "--sign" => flags.sign = true,
                "--detach-sign" | "--detach" => flags.detach = true,
                "--clearsign" | "--clear-sign" | "--clear" => flags.clear = true,
                "--encrypt" => flags.encrypt = true,
                "--symmetric" => flags.symmetric = true,
                "--decrypt" => flags.decrypt = true,
                "--verify" => flags.verify = true,
                "--list-keys" | "--list-public-keys" | "--fingerprint" => flags.list = true,
                "--list-secret-keys" => flags.list_secret = true,
                "--export" => flags.export = true,
                "--version" => flags.version = true,
                "--help" => flags.help = true,
                "--armor" | "--armour" => args.armor = true,
                "--no-armor" | "--no-armour" => args.armor = false,
                "--yes" => args.yes = true,
                "--no" => args.yes = false,
                "--with-colons" => args.with_colons = true,
                "--local-user" | "--default-key" => args.local_users.push(value(&name)?),
                "--recipient" | "--hidden-recipient" | "--encrypt-to" | "--hidden-encrypt-to" => {
                    args.recipients.push(value(&name)?)
                }
                "--default-recipient" => args.default_recipient = Some(value(&name)?),
                "--default-recipient-self" => args.default_recipient_self = true,
                "--output" => args.output = Some(value(&name)?),
                "--keypack" => args.keypack = Some(value(&name)?),
                "--key-file" => args.key_file = Some(value(&name)?),
                "--passphrase" => args.passphrase = Some(Passphrase::Text(value(&name)?)),
                "--passphrase-file" => args.passphrase = Some(Passphrase::File(value(&name)?)),
                "--passphrase-fd" => {
                    let v = value(&name)?;
                    args.passphrase = Some(Passphrase::Fd(fd(&name, v)?));
                }
                "--status-fd" => {
                    let v = value(&name)?;
                    args.status_fd = Some(fd(&name, v)?);
                }
                "--status-file" => args.status_file = Some(value(&name)?),
                n if NO_OPS.contains(&n) => {}
                n if NO_OP_WITH_VALUE.contains(&n) => {
                    value(n)?;
                }
                n if KEY_MANAGEMENT.contains(&n) => {
                    return Err(ParseError::KeyManagement(n.to_string()));
                }
                n if OTHER_KEYRING.contains(&n) => {
                    return Err(ParseError::OtherKeyring(n.to_string()));
                }
                _ => return Err(ParseError::Unsupported(name)),
            }
            continue;
        }

        // A cluster of short options, as gpg takes them: "-bsau KEY".
        let cluster: Vec<char> = arg[1..].chars().collect();
        let mut i = 0;
        while i < cluster.len() {
            let c = cluster[i];
            i += 1;
            let mut value = || -> Result<String, ParseError> {
                if i < cluster.len() {
                    let v: String = cluster[i..].iter().collect();
                    i = cluster.len();
                    Ok(v)
                } else {
                    rest.next()
                        .cloned()
                        .ok_or_else(|| ParseError::MissingValue(format!("-{c}")))
                }
            };
            match c {
                's' => flags.sign = true,
                'b' => flags.detach = true,
                'e' => flags.encrypt = true,
                'c' => flags.symmetric = true,
                'd' => flags.decrypt = true,
                'k' => flags.list = true,
                'K' => flags.list_secret = true,
                'h' => flags.help = true,
                'a' => args.armor = true,
                'q' | 'v' | 't' => {}
                'z' => {
                    value()?;
                }
                'u' => args.local_users.push(value()?),
                'r' | 'R' => args.recipients.push(value()?),
                'o' => args.output = Some(value()?),
                _ => return Err(ParseError::Unsupported(format!("-{c}"))),
            }
        }
    }

    args.command = Some(resolve(&flags)?);
    Ok(args)
}

fn resolve(f: &Flags) -> Result<Command, ParseError> {
    if f.help {
        return Ok(Command::Help);
    }
    if f.version {
        return Ok(Command::Version);
    }
    let signing = f.sign || f.detach || f.clear;
    let chosen = [
        f.verify,
        f.decrypt,
        f.encrypt || f.symmetric || signing,
        f.list || f.list_secret,
        f.export,
    ]
    .iter()
    .filter(|&&x| x)
    .count();
    if chosen > 1 {
        return Err(ParseError::Conflict("these operations".into()));
    }
    if f.verify {
        return Ok(Command::Verify);
    }
    if f.decrypt {
        return Ok(Command::Decrypt);
    }
    if f.list || f.list_secret {
        return Ok(Command::ListKeys {
            secret: f.list_secret,
        });
    }
    if f.export {
        return Ok(Command::Export);
    }
    if f.symmetric {
        if f.encrypt || signing {
            return Err(ParseError::Conflict(
                "passphrase encryption and keys (encrypting to a key, or signing)".into(),
            ));
        }
        return Ok(Command::Symmetric);
    }
    if f.encrypt {
        if f.detach || f.clear {
            return Err(ParseError::Conflict(
                "encryption and a detached or clear-text signature".into(),
            ));
        }
        return Ok(Command::Encrypt { sign: f.sign });
    }
    if f.detach && f.clear {
        return Err(ParseError::Conflict(
            "a detached and a clear-text signature".into(),
        ));
    }
    if f.clear {
        return Ok(Command::Sign(SignKind::Clear));
    }
    if f.detach {
        return Ok(Command::Sign(SignKind::Detached));
    }
    if f.sign {
        return Ok(Command::Sign(SignKind::Inline));
    }
    Err(ParseError::NoOperation)
}

#[cfg(test)]
mod tests {
    use super::*;

    fn p(args: &[&str]) -> Result<Args, ParseError> {
        parse(&args.iter().map(|s| s.to_string()).collect::<Vec<_>>())
    }

    #[test]
    fn git_signing_parses_as_a_detached_armored_signature() {
        let a = p(&["--status-fd=2", "-bsau", "ABCDEF0123456789"]).unwrap();
        assert_eq!(a.command, Some(Command::Sign(SignKind::Detached)));
        assert!(a.armor);
        assert_eq!(a.status_fd, Some(2));
        assert_eq!(a.local_users, ["ABCDEF0123456789"]);

        // The committer ident git passes when user.signingKey is unset.
        let a = p(&["--status-fd=2", "-bsau", "A U Thor <author@example.com>"]).unwrap();
        assert_eq!(a.local_users, ["A U Thor <author@example.com>"]);
    }

    #[test]
    fn git_verification_parses_with_the_signature_file_and_stdin() {
        let a = p(&[
            "--keyid-format=long",
            "--status-fd=1",
            "--verify",
            "/tmp/sig",
            "-",
        ])
        .unwrap();
        assert_eq!(a.command, Some(Command::Verify));
        assert_eq!(a.status_fd, Some(1));
        assert_eq!(a.inputs, ["/tmp/sig", "-"]);
    }

    #[test]
    fn subcommands_and_their_options_parse() {
        let a = p(&["sign", "--detach", "-a", "-u", "k"]).unwrap();
        assert_eq!(a.command, Some(Command::Sign(SignKind::Detached)));
        let a = p(&["sign", "--clear"]).unwrap();
        assert_eq!(a.command, Some(Command::Sign(SignKind::Clear)));
        let a = p(&[
            "encrypt",
            "-r",
            "a",
            "--recipient=b",
            "-s",
            "-o",
            "out",
            "in",
        ])
        .unwrap();
        assert_eq!(a.command, Some(Command::Encrypt { sign: true }));
        assert_eq!(a.recipients, ["a", "b"]);
        assert_eq!(a.output.as_deref(), Some("out"));
        assert_eq!(a.inputs, ["in"]);
        assert_eq!(p(&["decrypt"]).unwrap().command, Some(Command::Decrypt));
        assert_eq!(
            p(&["keys"]).unwrap().command,
            Some(Command::ListKeys { secret: false })
        );
        assert_eq!(
            p(&["-K"]).unwrap().command,
            Some(Command::ListKeys { secret: true })
        );
        assert_eq!(
            p(&["--keypack", "work", "-s"]).unwrap().keypack.as_deref(),
            Some("work")
        );
        assert_eq!(
            p(&["--key-file", "k.gfkey", "-s"])
                .unwrap()
                .key_file
                .as_deref(),
            Some("k.gfkey")
        );
    }

    #[test]
    fn key_management_is_refused_by_name() {
        for opt in [
            "--import",
            "--gen-key",
            "--edit-key",
            "--recv-keys",
            "--update-trustdb",
        ] {
            assert_eq!(
                p(&[opt]).err(),
                Some(ParseError::KeyManagement(opt.to_string()))
            );
        }
        assert!(
            ParseError::KeyManagement("--import".into())
                .to_string()
                .contains("no key management")
        );
    }

    #[test]
    fn unknown_options_are_refused_not_ignored() {
        assert_eq!(
            p(&["-s", "--frobnicate"]).err(),
            Some(ParseError::Unsupported("--frobnicate".into()))
        );
        assert_eq!(
            p(&["-sx"]).err(),
            Some(ParseError::Unsupported("-x".into()))
        );
        assert_eq!(
            p(&["-s", "-u"]).err(),
            Some(ParseError::MissingValue("-u".into()))
        );
        assert_eq!(p(&[]).err(), Some(ParseError::NoOperation));
        assert!(matches!(
            p(&["-s", "--verify"]),
            Err(ParseError::Conflict(_))
        ));
        assert!(matches!(p(&["-e", "-b"]), Err(ParseError::Conflict(_))));
    }

    #[test]
    fn harmless_gpg_options_change_nothing() {
        let a = p(&[
            "--batch",
            "--no-tty",
            "--yes",
            "--pinentry-mode",
            "loopback",
            "-bs",
        ])
        .unwrap();
        assert_eq!(a.command, Some(Command::Sign(SignKind::Detached)));
        assert!(a.inputs.is_empty());
    }

    #[test]
    fn a_value_glued_to_its_short_option_is_taken() {
        let a = p(&["-sukey"]).unwrap();
        assert_eq!(a.local_users, ["key"]);
        let a = p(&["-e", "-rbob", "--", "-file"]).unwrap();
        assert_eq!(a.recipients, ["bob"]);
        assert_eq!(a.inputs, ["-file"]);
    }

    #[test]
    fn tool_command_lines_parse() {
        // maven-gpg-plugin
        let a = p(&[
            "--batch",
            "--pinentry-mode",
            "loopback",
            "--passphrase-fd",
            "0",
            "--local-user",
            "K",
            "--armor",
            "--detach-sign",
            "--output",
            "a.jar.asc",
            "a.jar",
        ])
        .unwrap();
        assert_eq!(a.command, Some(Command::Sign(SignKind::Detached)));
        assert_eq!(a.passphrase, Some(Passphrase::Fd(0)));
        assert_eq!(a.inputs, ["a.jar"]);

        // rpm
        let a = p(&[
            "--no-verbose",
            "--no-armor",
            "--no-secmem-warning",
            "-u",
            "K",
            "-sbo",
            "x.sig",
            "--digest-algo",
            "sha256",
            "--",
            "x.rpm",
        ])
        .unwrap();
        assert_eq!(a.command, Some(Command::Sign(SignKind::Detached)));
        assert!(!a.armor);
        assert_eq!(a.output.as_deref(), Some("x.sig"));
        assert_eq!(a.inputs, ["x.rpm"]);

        // pass
        let a = p(&[
            "-e",
            "-r",
            "K",
            "-o",
            "f.gpg",
            "--quiet",
            "--yes",
            "--compress-algo=none",
            "--no-encrypt-to",
        ])
        .unwrap();
        assert_eq!(a.command, Some(Command::Encrypt { sign: false }));
        assert!(a.yes);
        let a = p(&["--list-keys", "--with-colons", "K"]).unwrap();
        assert_eq!(a.command, Some(Command::ListKeys { secret: false }));
        assert!(a.with_colons);
        assert_eq!(a.inputs, ["K"]);
    }

    #[test]
    fn passphrases_come_from_text_files_and_descriptors() {
        assert_eq!(
            p(&["-c", "--passphrase", "pw"]).unwrap().passphrase,
            Some(Passphrase::Text("pw".into()))
        );
        assert_eq!(
            p(&["-c", "--passphrase-file=/p"]).unwrap().passphrase,
            Some(Passphrase::File("/p".into()))
        );
        assert_eq!(
            p(&["-c", "--passphrase-fd", "3"]).unwrap().passphrase,
            Some(Passphrase::Fd(3))
        );
        assert!(matches!(
            p(&["-c", "--passphrase-fd", "x"]),
            Err(ParseError::Unsupported(_))
        ));
    }

    #[test]
    fn symmetric_export_and_recipient_forms_parse() {
        assert_eq!(p(&["-c"]).unwrap().command, Some(Command::Symmetric));
        assert!(matches!(p(&["-c", "-e"]), Err(ParseError::Conflict(_))));
        assert!(matches!(p(&["-cs"]), Err(ParseError::Conflict(_))));
        assert_eq!(
            p(&["--export", "-a", "K"]).unwrap().command,
            Some(Command::Export)
        );
        assert_eq!(p(&["export"]).unwrap().command, Some(Command::Export));
        assert_eq!(
            p(&["--fingerprint"]).unwrap().command,
            Some(Command::ListKeys { secret: false })
        );
        let a = p(&["-e", "-R", "a", "--encrypt-to", "b", "--hidden-recipient=c"]).unwrap();
        assert_eq!(a.recipients, ["a", "b", "c"]);
        let a = p(&["-e", "--default-recipient", "d", "--default-recipient-self"]).unwrap();
        assert_eq!(a.default_recipient.as_deref(), Some("d"));
        assert!(a.default_recipient_self);
        assert!(!p(&["-a", "--no-armor", "-s"]).unwrap().armor);
        assert_eq!(
            p(&["--status-file", "s", "-s"])
                .unwrap()
                .status_file
                .as_deref(),
            Some("s")
        );
    }

    #[test]
    fn secret_exports_and_other_keyrings_are_refused() {
        for opt in [
            "--export-secret-keys",
            "--export-secret-subkeys",
            "--card-status",
        ] {
            assert_eq!(
                p(&[opt]).err(),
                Some(ParseError::KeyManagement(opt.to_string()))
            );
        }
        for opt in ["--homedir", "--keyring", "--no-default-keyring"] {
            assert_eq!(
                p(&[opt, "x", "-s"]).err(),
                Some(ParseError::OtherKeyring(opt.to_string()))
            );
        }
        assert!(
            ParseError::OtherKeyring("--homedir".into())
                .to_string()
                .contains("--keypack")
        );
    }

    #[test]
    fn every_listed_harmless_option_is_accepted() {
        for opt in NO_OPS {
            assert!(p(&[opt, "-s"]).is_ok(), "{opt}");
        }
        for opt in NO_OP_WITH_VALUE {
            assert!(p(&[opt, "v", "-s"]).is_ok(), "{opt}");
        }
        assert!(p(&["-t", "-z", "9", "-s"]).is_ok());
        assert!(p(&["-z9", "-s"]).is_ok());
    }
}
