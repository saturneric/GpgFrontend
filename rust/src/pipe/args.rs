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
//!
//! In gpg's form the parsing is gpg's: a long option may be shortened to any
//! unique prefix, and the first file ends the options. After one of gfpipe's
//! own subcommands, options may come anywhere.

use std::ffi::{OsStr, OsString};

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
    /// No command, only files: gpg guesses from what the data is.
    Default,
    Version,
    Help,
}

/// Where a passphrase comes from; gfpipe never prompts.
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum Passphrase {
    Text(String),
    /// The first line of a file.
    File(OsString),
    /// The first line read from a file descriptor; 0 is standard input.
    Fd(i32),
}

/// A key to encrypt to.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Recipient {
    pub name: String,
    /// `--hidden-recipient`: the message does not say who it is for.
    pub hidden: bool,
}

/// A parsed command line.
#[derive(Debug, Default)]
pub struct Args {
    pub command: Option<Command>,
    pub local_users: Vec<String>,
    /// `--default-key`: the signing key when no `-u` names one.
    pub default_key: Option<String>,
    pub recipients: Vec<Recipient>,
    /// `--default-recipient`, used when no recipient is named.
    pub default_recipient: Option<String>,
    /// `--default-recipient-self`: the package's own secret key.
    pub default_recipient_self: bool,
    /// `--throw-keyids`: every recipient is hidden.
    pub throw_keyids: bool,
    pub armor: bool,
    pub output: Option<OsString>,
    /// `--yes`: an existing output file may be replaced.
    pub yes: bool,
    pub status_fd: Option<i32>,
    pub status_file: Option<OsString>,
    /// `--with-colons`: the machine-readable listing.
    pub with_colons: bool,
    /// `--fingerprint`: the listing groups fingerprints as gpg does.
    pub fingerprint: bool,
    /// `--list-only`: name the recipients of a message, decrypt nothing.
    pub list_only: bool,
    /// `-q`: no informational lines on standard error.
    pub quiet: bool,
    pub verbose: bool,
    pub passphrase: Option<Passphrase>,
    pub keypack: Option<OsString>,
    /// `--key-file`: the file holding a plain package's key.
    pub key_file: Option<OsString>,
    /// Notes for standard error about how the line was read.
    pub notes: Vec<String>,
    /// File arguments, in order; `-` is standard input. Key patterns for a
    /// listing or an export.
    pub inputs: Vec<OsString>,
}

impl Args {
    /// The file arguments as text, for key patterns.
    pub fn patterns(&self) -> Vec<String> {
        self.inputs
            .iter()
            .map(|p| p.to_string_lossy().into_owned())
            .collect()
    }
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
    /// A shortened option that more than one option starts with.
    Ambiguous(String),
    /// An option that needs a value was last.
    MissingValue(String),
    /// A value an option cannot take.
    InvalidValue(String),
    /// No operation at all.
    NoOperation,
    /// Two operations at once.
    ConflictingCommands,
    /// Operations gfpipe does not do together, with why.
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
            ParseError::Unsupported(opt) => write!(f, "invalid option \"{opt}\""),
            ParseError::Ambiguous(opt) => write!(f, "option \"{opt}\" is ambiguous"),
            ParseError::MissingValue(opt) => write!(f, "missing argument for option \"{opt}\""),
            ParseError::InvalidValue(opt) => write!(f, "invalid argument for option \"{opt}\""),
            ParseError::NoOperation => {
                write!(f, "nothing to do; try 'gfpipe help'")
            }
            ParseError::ConflictingCommands => write!(f, "conflicting commands"),
            ParseError::Conflict(what) => write!(f, "{what} cannot be combined"),
        }
    }
}

/// What an option is to the parser.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
enum Kind {
    /// Handled by name, without a value.
    Flag,
    /// Handled by name, with a value.
    Value,
    /// Accepted and changes nothing for gfpipe.
    NoOp,
    /// Accepted with a value gfpipe does not need.
    NoOpValue,
    /// A gpg command that changes keys or hands out secrets.
    KeyManagement,
    /// Names another place to take keys from.
    OtherKeyring,
}

/// Every long option gfpipe knows, by name. A shortened option resolves to
/// the one entry it starts, never to a no-op: a typo is refused, not lost.
const OPTIONS: &[(&str, Kind)] = &[
    // Commands.
    ("--sign", Kind::Flag),
    ("--detach-sign", Kind::Flag),
    ("--clearsign", Kind::Flag),
    ("--clear-sign", Kind::Flag),
    ("--encrypt", Kind::Flag),
    ("--symmetric", Kind::Flag),
    ("--decrypt", Kind::Flag),
    ("--verify", Kind::Flag),
    ("--list-keys", Kind::Flag),
    ("--list-public-keys", Kind::Flag),
    ("--fingerprint", Kind::Flag),
    ("--list-secret-keys", Kind::Flag),
    ("--export", Kind::Flag),
    ("--version", Kind::Flag),
    ("--help", Kind::Flag),
    // Flags.
    ("--armor", Kind::Flag),
    ("--armour", Kind::Flag),
    ("--no-armor", Kind::Flag),
    ("--no-armour", Kind::Flag),
    ("--yes", Kind::Flag),
    ("--no", Kind::Flag),
    ("--with-colons", Kind::Flag),
    ("--default-recipient-self", Kind::Flag),
    ("--throw-keyids", Kind::Flag),
    ("--no-throw-keyids", Kind::Flag),
    ("--list-only", Kind::Flag),
    ("--quiet", Kind::Flag),
    ("--verbose", Kind::Flag),
    ("--no-verbose", Kind::Flag),
    // Options with a value.
    ("--local-user", Kind::Value),
    ("--default-key", Kind::Value),
    ("--recipient", Kind::Value),
    ("--hidden-recipient", Kind::Value),
    ("--encrypt-to", Kind::Value),
    ("--hidden-encrypt-to", Kind::Value),
    ("--default-recipient", Kind::Value),
    ("--output", Kind::Value),
    ("--keypack", Kind::Value),
    ("--key-file", Kind::Value),
    ("--passphrase", Kind::Value),
    ("--passphrase-file", Kind::Value),
    ("--passphrase-fd", Kind::Value),
    ("--status-fd", Kind::Value),
    ("--status-file", Kind::Value),
    // Accepted: the package's keys are trusted, nothing is fetched, nothing
    // is prompted for, and the engine chooses algorithms by the recipients'
    // preferences.
    ("--batch", Kind::NoOp),
    ("--no-batch", Kind::NoOp),
    ("--no-tty", Kind::NoOp),
    ("--no-greeting", Kind::NoOp),
    ("--no-permission-warning", Kind::NoOp),
    ("--no-secmem-warning", Kind::NoOp),
    ("--with-fingerprint", Kind::NoOp),
    ("--with-subkey-fingerprint", Kind::NoOp),
    ("--with-subkey-fingerprints", Kind::NoOp),
    ("--with-keygrip", Kind::NoOp),
    ("--fixed-list-mode", Kind::NoOp),
    ("--no-encrypt-to", Kind::NoOp),
    ("--no-default-recipient", Kind::NoOp),
    ("--always-trust", Kind::NoOp),
    ("--no-auto-key-locate", Kind::NoOp),
    ("--no-auto-key-retrieve", Kind::NoOp),
    ("--auto-key-retrieve", Kind::NoOp),
    ("--disable-dirmngr", Kind::NoOp),
    ("--use-agent", Kind::NoOp),
    ("--no-use-agent", Kind::NoOp),
    ("--lock-never", Kind::NoOp),
    ("--lock-once", Kind::NoOp),
    ("--lock-multiple", Kind::NoOp),
    ("--exit-on-status-write-error", Kind::NoOp),
    ("--utf8-strings", Kind::NoOp),
    ("--no-utf8-strings", Kind::NoOp),
    ("--textmode", Kind::NoOp),
    ("--no-textmode", Kind::NoOp),
    ("--emit-version", Kind::NoOp),
    ("--no-emit-version", Kind::NoOp),
    ("--no-comments", Kind::NoOp),
    ("--no-mdc-warning", Kind::NoOp),
    ("--ignore-time-conflict", Kind::NoOp),
    ("--ignore-valid-from", Kind::NoOp),
    ("--ignore-mdc-error", Kind::NoOp),
    ("--allow-weak-digest-algos", Kind::NoOp),
    ("--no-symkey-cache", Kind::NoOp),
    ("--force-mdc", Kind::NoOp),
    ("--no-force-mdc", Kind::NoOp),
    ("--keyid-format", Kind::NoOpValue),
    ("--pinentry-mode", Kind::NoOpValue),
    ("--trust-model", Kind::NoOpValue),
    ("--compress-algo", Kind::NoOpValue),
    ("--compress-level", Kind::NoOpValue),
    ("--bzip2-compress-level", Kind::NoOpValue),
    ("--cipher-algo", Kind::NoOpValue),
    ("--digest-algo", Kind::NoOpValue),
    ("--cert-digest-algo", Kind::NoOpValue),
    ("--s2k-mode", Kind::NoOpValue),
    ("--s2k-count", Kind::NoOpValue),
    ("--s2k-cipher-algo", Kind::NoOpValue),
    ("--s2k-digest-algo", Kind::NoOpValue),
    ("--personal-cipher-preferences", Kind::NoOpValue),
    ("--personal-digest-preferences", Kind::NoOpValue),
    ("--personal-compress-preferences", Kind::NoOpValue),
    ("--comment", Kind::NoOpValue),
    ("--set-filename", Kind::NoOpValue),
    ("--display-charset", Kind::NoOpValue),
    ("--charset", Kind::NoOpValue),
    ("--logger-fd", Kind::NoOpValue),
    ("--logger-file", Kind::NoOpValue),
    ("--log-file", Kind::NoOpValue),
    ("--keyserver", Kind::NoOpValue),
    ("--keyserver-options", Kind::NoOpValue),
    ("--auto-key-locate", Kind::NoOpValue),
    ("--passphrase-repeat", Kind::NoOpValue),
    ("--list-options", Kind::NoOpValue),
    ("--verify-options", Kind::NoOpValue),
    ("--export-options", Kind::NoOpValue),
    ("--compliance", Kind::NoOpValue),
    ("--max-output", Kind::NoOpValue),
    ("--agent-program", Kind::NoOpValue),
    ("--dirmngr-program", Kind::NoOpValue),
    // Refused: key management, and secrets leaving the package.
    ("--import", Kind::KeyManagement),
    ("--export-secret-keys", Kind::KeyManagement),
    ("--export-secret-subkeys", Kind::KeyManagement),
    ("--export-ssh-key", Kind::KeyManagement),
    ("--gen-key", Kind::KeyManagement),
    ("--generate-key", Kind::KeyManagement),
    ("--full-gen-key", Kind::KeyManagement),
    ("--full-generate-key", Kind::KeyManagement),
    ("--quick-gen-key", Kind::KeyManagement),
    ("--quick-generate-key", Kind::KeyManagement),
    ("--quick-add-key", Kind::KeyManagement),
    ("--quick-add-uid", Kind::KeyManagement),
    ("--quick-revoke-key", Kind::KeyManagement),
    ("--quick-revoke-uid", Kind::KeyManagement),
    ("--quick-set-expire", Kind::KeyManagement),
    ("--quick-set-primary-uid", Kind::KeyManagement),
    ("--quick-sign-key", Kind::KeyManagement),
    ("--quick-lsign-key", Kind::KeyManagement),
    ("--edit-key", Kind::KeyManagement),
    ("--sign-key", Kind::KeyManagement),
    ("--lsign-key", Kind::KeyManagement),
    ("--delete-key", Kind::KeyManagement),
    ("--delete-keys", Kind::KeyManagement),
    ("--delete-secret-key", Kind::KeyManagement),
    ("--delete-secret-keys", Kind::KeyManagement),
    ("--delete-secret-and-public-key", Kind::KeyManagement),
    ("--delete-secret-and-public-keys", Kind::KeyManagement),
    ("--passwd", Kind::KeyManagement),
    ("--change-passphrase", Kind::KeyManagement),
    ("--gen-revoke", Kind::KeyManagement),
    ("--generate-revocation", Kind::KeyManagement),
    ("--recv-keys", Kind::KeyManagement),
    ("--receive-keys", Kind::KeyManagement),
    ("--send-keys", Kind::KeyManagement),
    ("--refresh-keys", Kind::KeyManagement),
    ("--search-keys", Kind::KeyManagement),
    ("--fetch-keys", Kind::KeyManagement),
    ("--locate-keys", Kind::KeyManagement),
    ("--locate-external-keys", Kind::KeyManagement),
    ("--update-trustdb", Kind::KeyManagement),
    ("--check-trustdb", Kind::KeyManagement),
    ("--import-ownertrust", Kind::KeyManagement),
    ("--export-ownertrust", Kind::KeyManagement),
    ("--card-edit", Kind::KeyManagement),
    ("--edit-card", Kind::KeyManagement),
    ("--card-status", Kind::KeyManagement),
    // Refused: keys from elsewhere.
    ("--homedir", Kind::OtherKeyring),
    ("--keyring", Kind::OtherKeyring),
    ("--secret-keyring", Kind::OtherKeyring),
    ("--primary-keyring", Kind::OtherKeyring),
    ("--no-default-keyring", Kind::OtherKeyring),
    ("--options", Kind::OtherKeyring),
];

/// gfpipe's own spellings, matched only in full.
const ALIASES: &[(&str, &str)] = &[("--detach", "--detach-sign"), ("--clear", "--clearsign")];

/// The option `name` (with its dashes) stands for, exactly or as a unique
/// prefix; spellings of one option count once. A prefix that only an
/// option without effect starts is refused: a typo would be lost.
fn resolve_name(name: &str) -> Result<(&'static str, Kind), ParseError> {
    if let Some((_, full)) = ALIASES.iter().find(|(alias, _)| *alias == name) {
        return resolve_name(full);
    }
    if let Some(&(full, kind)) = OPTIONS.iter().find(|(n, _)| *n == name) {
        return Ok((full, kind));
    }
    let mut matches: Vec<(&'static str, Kind)> = OPTIONS
        .iter()
        .filter(|(n, _)| n.len() > 2 && name.len() > 2 && n.starts_with(name))
        .copied()
        .collect();
    // "--armo" names both spellings of one option.
    matches.sort_by_key(|(n, _)| canonical(n));
    matches.dedup_by_key(|(n, _)| canonical(n));
    match matches.as_slice() {
        [] | [(_, Kind::NoOp | Kind::NoOpValue)] => Err(ParseError::Unsupported(name.to_string())),
        [one] => Ok(*one),
        _ => Err(ParseError::Ambiguous(name.to_string())),
    }
}

/// One name per option that has several spellings.
fn canonical(name: &str) -> &str {
    match name {
        "--armour" => "--armor",
        "--no-armour" => "--no-armor",
        "--clear-sign" => "--clearsign",
        "--list-public-keys" => "--list-keys",
        other => other,
    }
}

/// gfpipe's own subcommands, which may come first.
fn subcommand(word: &OsStr, flags: &mut Flags) -> bool {
    match word.to_str() {
        Some("sign") => flags.sign = true,
        Some("verify") => flags.verify = true,
        Some("encrypt") => flags.encrypt = true,
        Some("decrypt") => flags.decrypt = true,
        Some("keys") => flags.list = true,
        Some("export") => flags.export = true,
        Some("version") => flags.version = true,
        Some("help") => flags.help = true,
        _ => return false,
    }
    true
}

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

fn descriptor(name: &str, v: &OsStr) -> Result<i32, ParseError> {
    v.to_str()
        .and_then(|v| v.parse::<i32>().ok())
        .filter(|fd| *fd >= 0)
        .ok_or_else(|| ParseError::InvalidValue(name.to_string()))
}

fn text(v: OsString) -> String {
    v.to_string_lossy().into_owned()
}

/// Where the status lines go, as far as the options before a refused one
/// say: gpg reports a command line it refuses there.
pub fn early_status(argv: &[OsString]) -> (Option<i32>, Option<OsString>) {
    let (mut fd, mut file) = (None, None);
    let mut rest = argv.iter();
    while let Some(arg) = rest.next() {
        let Some(word) = arg.to_str() else { break };
        if word == "--" || !word.starts_with('-') || word == "-" {
            break;
        }
        let (name, inline) = match word.split_once('=') {
            Some((n, v)) => (n, Some(OsString::from(v))),
            None => (word, None),
        };
        let mut value = || inline.clone().or_else(|| rest.next().cloned());
        match resolve_name(name) {
            Ok(("--status-fd", _)) => {
                fd = value().and_then(|v| descriptor("--status-fd", &v).ok());
            }
            Ok(("--status-file", _)) => file = value(),
            Ok((_, Kind::Value | Kind::NoOpValue)) => {
                value();
            }
            Ok(_) => {}
            Err(_) => break,
        }
    }
    (fd, file)
}

/// Parse `argv` without the program name.
pub fn parse(argv: &[OsString]) -> Result<Args, ParseError> {
    let mut args = Args::default();
    let mut flags = Flags::default();
    let mut rest = argv.iter().peekable();

    // One of gfpipe's subcommands may come first; after it, options may come
    // anywhere. In gpg's form the first file ends the options, as with gpg.
    let own_form = rest.peek().is_some_and(|w| subcommand(w, &mut flags));
    if own_form {
        rest.next();
    }

    let mut only_files = false;
    // In gpg's form a file ended the options, rather than "--".
    let mut ended_by_file = false;
    while let Some(arg) = rest.next() {
        let Some(word) = arg.to_str() else {
            // Not text, so not an option: a file name.
            args.inputs.push(arg.clone());
            if !own_form && !only_files {
                only_files = true;
                ended_by_file = true;
            }
            continue;
        };
        if only_files || word == "-" || !word.starts_with('-') {
            if ended_by_file && word.starts_with('-') && word != "-" {
                args.notes
                    .push(format!("Note: '{word}' is not considered an option"));
            }
            args.inputs.push(arg.clone());
            if !own_form && !only_files {
                only_files = true;
                ended_by_file = true;
            }
            continue;
        }
        if word == "--" {
            only_files = true;
            continue;
        }

        if let Some(long) = word.strip_prefix("--") {
            let (given, inline) = match long.split_once('=') {
                Some((n, v)) => (format!("--{n}"), Some(OsString::from(v))),
                None => (word.to_string(), None),
            };
            let (name, kind) = resolve_name(&given)?;
            let mut value = || -> Result<OsString, ParseError> {
                match &inline {
                    Some(v) => Ok(v.clone()),
                    None => rest
                        .next()
                        .cloned()
                        .ok_or_else(|| ParseError::MissingValue(name.to_string())),
                }
            };
            match kind {
                Kind::KeyManagement => return Err(ParseError::KeyManagement(name.to_string())),
                Kind::OtherKeyring => return Err(ParseError::OtherKeyring(name.to_string())),
                Kind::NoOp => continue,
                Kind::NoOpValue => {
                    value()?;
                    continue;
                }
                Kind::Flag | Kind::Value => {}
            }
            match name {
                "--sign" => flags.sign = true,
                "--detach-sign" => flags.detach = true,
                "--clearsign" | "--clear-sign" => flags.clear = true,
                "--encrypt" => flags.encrypt = true,
                "--symmetric" => flags.symmetric = true,
                "--decrypt" => flags.decrypt = true,
                "--verify" => flags.verify = true,
                "--list-keys" | "--list-public-keys" => flags.list = true,
                "--fingerprint" => {
                    flags.list = true;
                    args.fingerprint = true;
                }
                "--list-secret-keys" => flags.list_secret = true,
                "--export" => flags.export = true,
                "--version" => flags.version = true,
                "--help" => flags.help = true,
                "--armor" | "--armour" => args.armor = true,
                "--no-armor" | "--no-armour" => args.armor = false,
                "--yes" => args.yes = true,
                "--no" => args.yes = false,
                "--with-colons" => args.with_colons = true,
                "--default-recipient-self" => args.default_recipient_self = true,
                "--throw-keyids" => args.throw_keyids = true,
                "--no-throw-keyids" => args.throw_keyids = false,
                "--list-only" => args.list_only = true,
                "--quiet" => args.quiet = true,
                "--verbose" => args.verbose = true,
                "--no-verbose" => args.verbose = false,
                "--local-user" => args.local_users.push(text(value()?)),
                "--default-key" => args.default_key = Some(text(value()?)),
                "--recipient" | "--encrypt-to" => args.recipients.push(Recipient {
                    name: text(value()?),
                    hidden: false,
                }),
                "--hidden-recipient" | "--hidden-encrypt-to" => args.recipients.push(Recipient {
                    name: text(value()?),
                    hidden: true,
                }),
                "--default-recipient" => args.default_recipient = Some(text(value()?)),
                "--output" => args.output = Some(value()?),
                "--keypack" => args.keypack = Some(value()?),
                "--key-file" => args.key_file = Some(value()?),
                "--passphrase" => args.passphrase = Some(Passphrase::Text(text(value()?))),
                "--passphrase-file" => args.passphrase = Some(Passphrase::File(value()?)),
                "--passphrase-fd" => {
                    let v = value()?;
                    args.passphrase = Some(Passphrase::Fd(descriptor(name, &v)?));
                }
                "--status-fd" => {
                    let v = value()?;
                    args.status_fd = Some(descriptor(name, &v)?);
                }
                "--status-file" => args.status_file = Some(value()?),
                _ => return Err(ParseError::Unsupported(given)),
            }
            continue;
        }

        // A cluster of short options, as gpg takes them: "-bsau KEY".
        let cluster: Vec<char> = word[1..].chars().collect();
        let mut i = 0;
        while i < cluster.len() {
            let c = cluster[i];
            i += 1;
            let mut value = || -> Result<OsString, ParseError> {
                if i < cluster.len() {
                    let v: String = cluster[i..].iter().collect();
                    i = cluster.len();
                    Ok(OsString::from(v))
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
                'q' => args.quiet = true,
                'v' => args.verbose = true,
                't' => {}
                'z' => {
                    value()?;
                }
                'u' => args.local_users.push(text(value()?)),
                'r' => args.recipients.push(Recipient {
                    name: text(value()?),
                    hidden: false,
                }),
                'R' => args.recipients.push(Recipient {
                    name: text(value()?),
                    hidden: true,
                }),
                'o' => args.output = Some(value()?),
                _ => return Err(ParseError::Unsupported(format!("-{c}"))),
            }
        }
    }

    args.command = Some(resolve(&flags, !args.inputs.is_empty())?);
    Ok(args)
}

fn resolve(f: &Flags, has_files: bool) -> Result<Command, ParseError> {
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
        f.list,
        f.list_secret,
        f.export,
    ]
    .iter()
    .filter(|&&x| x)
    .count();
    if chosen > 1 {
        return Err(ParseError::ConflictingCommands);
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
    if has_files {
        return Ok(Command::Default);
    }
    Err(ParseError::NoOperation)
}

#[cfg(test)]
mod tests {
    use super::*;

    fn p(args: &[&str]) -> Result<Args, ParseError> {
        parse(&args.iter().map(OsString::from).collect::<Vec<_>>())
    }

    fn names(recipients: &[Recipient]) -> Vec<&str> {
        recipients.iter().map(|r| r.name.as_str()).collect()
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
        assert_eq!(names(&a.recipients), ["a", "b"]);
        assert_eq!(a.output.as_deref(), Some(OsStr::new("out")));
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
            Some(OsStr::new("work"))
        );
        assert_eq!(
            p(&["--key-file", "k.gfkey", "-s"])
                .unwrap()
                .key_file
                .as_deref(),
            Some(OsStr::new("k.gfkey"))
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
        // Shortened, it is still refused by its full name.
        assert_eq!(
            p(&["--gen-k"]).err(),
            Some(ParseError::KeyManagement("--gen-key".into()))
        );
        assert_eq!(
            p(&["--impo"]).err(),
            Some(ParseError::Ambiguous("--impo".into()))
        );
    }

    #[test]
    fn unknown_options_are_refused_not_ignored() {
        assert_eq!(
            p(&["-s", "--frobnicate"]).err(),
            Some(ParseError::Unsupported("--frobnicate".into()))
        );
        assert_eq!(
            ParseError::Unsupported("--frobnicate".into()).to_string(),
            "invalid option \"--frobnicate\""
        );
        assert_eq!(
            p(&["-sx"]).err(),
            Some(ParseError::Unsupported("-x".into()))
        );
        assert_eq!(
            p(&["-s", "-u"]).err(),
            Some(ParseError::MissingValue("-u".into()))
        );
        assert_eq!(
            ParseError::MissingValue("-u".into()).to_string(),
            "missing argument for option \"-u\""
        );
        assert_eq!(p(&[]).err(), Some(ParseError::NoOperation));
        assert_eq!(
            p(&["-s", "--verify"]).err(),
            Some(ParseError::ConflictingCommands)
        );
        assert_eq!(
            p(&["-k", "-K"]).err(),
            Some(ParseError::ConflictingCommands)
        );
        assert!(matches!(p(&["-e", "-b"]), Err(ParseError::Conflict(_))));
    }

    #[test]
    fn long_options_may_be_shortened_as_with_gpg() {
        assert_eq!(p(&["--decr"]).unwrap().command, Some(Command::Decrypt));
        assert!(p(&["--arm", "--detach-s"]).unwrap().armor);
        // Both spellings of --armor are one option.
        assert!(p(&["--armo", "-s"]).unwrap().armor);
        assert_eq!(p(&["--recip", "k", "-e"]).unwrap().recipients[0].name, "k");
        assert_eq!(
            p(&["--verif", "x"]).err(),
            Some(ParseError::Ambiguous("--verif".into()))
        );
        assert_eq!(
            ParseError::Ambiguous("--de".into()).to_string(),
            "option \"--de\" is ambiguous"
        );
        // Never to an option that is accepted and does nothing: a typo would
        // be lost.
        assert_eq!(
            p(&["--bat", "-s"]).err(),
            Some(ParseError::Unsupported("--bat".into()))
        );
    }

    #[test]
    fn in_gpg_form_the_first_file_ends_the_options() {
        let a = p(&["-s", "file", "--armor"]).unwrap();
        assert!(!a.armor);
        assert_eq!(a.inputs, ["file", "--armor"]);
        assert_eq!(a.notes, ["Note: '--armor' is not considered an option"]);

        // After a subcommand, options come anywhere.
        let a = p(&["keys", "alice", "--with-colons"]).unwrap();
        assert!(a.with_colons);
        assert_eq!(a.inputs, ["alice"]);
        assert!(a.notes.is_empty());
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
        assert_eq!(names(&a.recipients), ["bob"]);
        assert_eq!(a.inputs, ["-file"]);
        assert!(a.notes.is_empty());
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
        assert_eq!(a.output.as_deref(), Some(OsStr::new("x.sig")));
        assert_eq!(a.inputs, ["x.rpm"]);

        // rpm 4.14 and later sign standard input to standard output.
        let a = p(&["-u", "K", "-sbo", "-", "--", "-"]).unwrap();
        assert_eq!(a.output.as_deref(), Some(OsStr::new("-")));
        assert_eq!(a.inputs, ["-"]);

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
        assert!(a.quiet);
        let a = p(&["--list-keys", "--with-colons", "K"]).unwrap();
        assert_eq!(a.command, Some(Command::ListKeys { secret: false }));
        assert!(a.with_colons);
        assert_eq!(a.inputs, ["K"]);
        // pass init and mv: who a file is for.
        let a = p(&[
            "-v",
            "--no-secmem-warning",
            "--no-permission-warning",
            "--decrypt",
            "--list-only",
            "--keyid-format",
            "long",
            "f.gpg",
        ])
        .unwrap();
        assert_eq!(a.command, Some(Command::Decrypt));
        assert!(a.list_only && a.verbose);
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
        for bad in ["x", "-1", "-7", ""] {
            assert_eq!(
                p(&["-c", "--passphrase-fd", bad]).err(),
                Some(ParseError::InvalidValue("--passphrase-fd".into())),
                "{bad}"
            );
            assert_eq!(
                p(&["-s", "--status-fd", bad]).err(),
                Some(ParseError::InvalidValue("--status-fd".into())),
                "{bad}"
            );
        }
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
        let a = p(&["--fingerprint"]).unwrap();
        assert_eq!(a.command, Some(Command::ListKeys { secret: false }));
        assert!(a.fingerprint);
        let a = p(&["-e", "-R", "a", "--encrypt-to", "b", "--hidden-recipient=c"]).unwrap();
        assert_eq!(names(&a.recipients), ["a", "b", "c"]);
        assert_eq!(
            a.recipients.iter().map(|r| r.hidden).collect::<Vec<_>>(),
            [true, false, true]
        );
        assert!(
            p(&["-e", "-r", "a", "--throw-keyids"])
                .unwrap()
                .throw_keyids
        );
        let a = p(&["-e", "--default-recipient", "d", "--default-recipient-self"]).unwrap();
        assert_eq!(a.default_recipient.as_deref(), Some("d"));
        assert!(a.default_recipient_self);
        assert!(!p(&["-a", "--no-armor", "-s"]).unwrap().armor);
        assert_eq!(
            p(&["--status-file", "s", "-s"])
                .unwrap()
                .status_file
                .as_deref(),
            Some(OsStr::new("s"))
        );
    }

    #[test]
    fn the_default_key_is_used_only_without_local_users() {
        let a = p(&["--default-key", "D", "-u", "U", "-s"]).unwrap();
        assert_eq!(a.local_users, ["U"]);
        assert_eq!(a.default_key.as_deref(), Some("D"));
    }

    #[test]
    fn files_alone_ask_for_gpgs_default_action() {
        let a = p(&["message.gpg"]).unwrap();
        assert_eq!(a.command, Some(Command::Default));
        assert_eq!(p(&["-q"]).err(), Some(ParseError::NoOperation));
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
        for (opt, kind) in OPTIONS {
            match kind {
                Kind::NoOp => assert!(p(&[opt, "-s"]).is_ok(), "{opt}"),
                Kind::NoOpValue => assert!(p(&[opt, "v", "-s"]).is_ok(), "{opt}"),
                _ => {}
            }
        }
        assert!(p(&["-t", "-z", "9", "-s"]).is_ok());
        assert!(p(&["-z9", "-s"]).is_ok());
    }

    #[test]
    fn every_full_option_name_resolves_to_itself() {
        for (opt, _) in OPTIONS {
            assert_eq!(resolve_name(opt).unwrap().0, *opt);
        }
    }

    #[cfg(unix)]
    #[test]
    fn file_names_that_are_not_text_are_kept_as_they_are() {
        use std::os::unix::ffi::OsStringExt;
        let name = OsString::from_vec(b"caf\xe9.txt".to_vec());
        let a = parse(&[OsString::from("-s"), name.clone()]).unwrap();
        assert_eq!(a.inputs, [name]);
    }
}
