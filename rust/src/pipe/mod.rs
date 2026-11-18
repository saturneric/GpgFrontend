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

//! gfpipe: basic OpenPGP operations with the keys of one key package, for
//! pipes and for git.
//!
//! Read-only by design: it opens a package GpgFrontend wrote and never writes
//! one, keeps no state, and does no key management. The keys in the package
//! are trusted as they are; their expiry and revocation are what they were
//! when the package was exported.
//!
//! gfpipe has no settings. GpgFrontend copies it into a folder together with
//! a key package, `default.gfpack`, and that package's key, `default.gfkey`;
//! from there it runs as it is. An option or the environment names another
//! package or key, and wins over what lies beside gfpipe. The host is a few
//! lines of C++ with no dependencies.
//!
//! What it says follows gpg: its lines on standard error, its status lines,
//! and its exit codes (0 good, 1 a signature that is not good, 2 any other
//! error).

mod args;
mod ops;
mod say;

use std::cell::OnceCell;
use std::collections::HashMap;
use std::ffi::OsString;
use std::io::{Read, Write};
use std::path::{Path, PathBuf};
use std::sync::{Arc, Mutex};

use zeroize::Zeroizing;

use crate::keypack::{self, OpenPackage, Protection, Unlock};

pub use args::Command;
use say::code;

/// The environment variable naming the package, by path, or by name beside
/// gfpipe.
pub const ENV_KEYPACK: &str = "GFPIPE_KEYPACK";
/// The environment variable holding a plain package's key.
pub const ENV_KEY: &str = "GFPIPE_KEY";
/// The environment variable naming a file that holds a plain package's key.
pub const ENV_KEY_FILE: &str = "GFPIPE_KEY_FILE";

const PACKAGE_EXTENSION: &str = "gfpack";

/// The package gfpipe uses when none is named, beside gfpipe.
pub const DEFAULT_PACKAGE: &str = "default";
/// The extension of the file holding a package's plain key, beside the
/// package and named as it is.
pub const KEY_FILE_EXTENSION: &str = "gfkey";

/// The engine's own words for its last failure on this thread; reading them
/// clears them.
pub(crate) fn engine_detail() -> String {
    let msg = crate::err::gfr_get_last_error_msg();
    if msg.is_null() {
        return String::new();
    }
    // SAFETY: a non-null pointer from gfr_get_last_error_msg is a C string
    // this crate allocated.
    let text = unsafe { std::ffi::CStr::from_ptr(msg) }
        .to_string_lossy()
        .into_owned();
    crate::ffi::mem::gfr_crypto_free_string(msg);
    text
}

/// What the host provides.
#[derive(Default)]
pub struct Host {
    /// The name gfpipe was started by (`argv[0]`): its messages start with
    /// it, so a gfpipe installed as `gpg` speaks as gpg. Empty means gfpipe.
    pub program: String,
    /// The folder gfpipe is in, where its package and key lie. None when the
    /// platform cannot say.
    pub program_dir: Option<PathBuf>,
    /// The GpgFrontend version gfpipe comes with.
    pub version: String,
    /// The environment, as far as gfpipe reads it.
    pub env: HashMap<String, String>,
}

/// The process's standard streams, or stand-ins for them in tests.
pub struct Io {
    pub stdin: Box<dyn Read + Send + Sync>,
    pub stdout: Box<dyn Write + Send + Sync>,
    pub stderr: Box<dyn Write + Send + Sync>,
}

/// A failure, with the exit code it ends the process with.
#[derive(Debug, Clone)]
pub struct PipeError {
    pub code: i32,
    /// The last line on standard error; none when it was said already.
    pub message: String,
    /// The operation and gpg-error code of gpg's `FAILURE` status line.
    pub failure: Option<(&'static str, u32)>,
}

impl PipeError {
    /// Anything that went wrong but a signature (gpg's exit code 2).
    pub fn error(message: impl Into<String>) -> Self {
        Self {
            code: 2,
            message: message.into(),
            failure: None,
        }
    }

    /// A signature that is not good (gpg's exit code 1).
    pub fn bad_signature(message: impl Into<String>) -> Self {
        Self {
            code: 1,
            message: message.into(),
            failure: None,
        }
    }

    /// With gpg's `FAILURE <operation> <code>` status line.
    pub fn failure(mut self, operation: &'static str, code: u32) -> Self {
        self.failure = Some((operation, code));
        self
    }
}

/// A writer more than one part of a run writes to: the data and the status
/// lines on one stream keep the order they were written in.
#[derive(Clone)]
struct Shared(Arc<Mutex<Box<dyn Write + Send + Sync>>>);

impl Shared {
    fn new(inner: Box<dyn Write + Send + Sync>) -> Self {
        Self(Arc::new(Mutex::new(inner)))
    }
}

impl Write for Shared {
    fn write(&mut self, buf: &[u8]) -> std::io::Result<usize> {
        self.0
            .lock()
            .unwrap_or_else(std::sync::PoisonError::into_inner)
            .write(buf)
    }

    fn flush(&mut self) -> std::io::Result<()> {
        self.0
            .lock()
            .unwrap_or_else(std::sync::PoisonError::into_inner)
            .flush()
    }
}

/// gpg's status lines (`--status-fd`, `--status-file`), written as they
/// happen.
#[derive(Default)]
pub struct Status {
    sinks: Vec<Box<dyn Write + Send + Sync>>,
    /// gpg writes one FAILURE line per run.
    failed: bool,
}

impl Status {
    pub fn push(&mut self, line: impl AsRef<str>) {
        let text = format!("[GNUPG:] {}\n", line.as_ref());
        for sink in &mut self.sinks {
            let _ = sink.write_all(text.as_bytes());
            let _ = sink.flush();
        }
    }

    /// gpg's `FAILURE` line, once per run.
    pub fn failure(&mut self, operation: &str, code: u32) {
        if !self.failed {
            self.failed = true;
            self.push(format!("FAILURE {operation} {code}"));
        }
    }
}

/// Run gfpipe with `argv` (without the program name). Returns the exit code.
pub fn run(argv: &[OsString], host: &Host, io: &mut Io) -> i32 {
    say::set_program(&host.program);

    // Data, messages and status lines on one stream stay in order.
    let out = Shared::new(std::mem::replace(&mut io.stdout, Box::new(std::io::sink())));
    let err = Shared::new(std::mem::replace(&mut io.stderr, Box::new(std::io::sink())));
    io.stdout = Box::new(out.clone());
    io.stderr = Box::new(err.clone());

    let parsed = match args::parse(argv) {
        Ok(parsed) => parsed,
        Err(e) => {
            say::line(&mut *io.stderr, &e);
            let (fd, file) = args::early_status(argv);
            if let Ok(mut status) = open_status(fd, file.as_deref(), &out, &err) {
                // gpg's parser refuses options; the commands it read it
                // refuses after.
                let operation = match e {
                    args::ParseError::ConflictingCommands
                    | args::ParseError::Conflict(_)
                    | args::ParseError::NoOperation => "gpg-exit",
                    _ => "option-parser",
                };
                status.failure(operation, code::exit());
            }
            return 2;
        }
    };
    for note in &parsed.notes {
        say::line(&mut *io.stderr, note);
    }

    let mut status = match open_status(parsed.status_fd, parsed.status_file.as_deref(), &out, &err)
    {
        Ok(status) => status,
        Err(e) => {
            say::line(&mut *io.stderr, e.message);
            return 2;
        }
    };

    let mut result = dispatch(&parsed, host, io, &mut status);
    if let Err(e) = io.stdout.flush() {
        if result.is_ok() {
            result = Err(PipeError::error(format!(
                "[stdout]: write error: {}",
                say::io_reason(&e)
            ))
            .failure("write", code::system(&e)));
        }
    }

    match result {
        Ok(()) => 0,
        Err(e) => {
            if let Some((operation, code)) = e.failure {
                status.failure(operation, code);
            }
            if !e.message.is_empty() {
                say::line(&mut *io.stderr, &e.message);
            }
            status.failure("gpg-exit", code::exit());
            let _ = io.stderr.flush();
            e.code
        }
    }
}

/// Where the status lines go, opened before anything runs: a status file
/// that cannot be written fails the run before it starts.
fn open_status(
    fd: Option<i32>,
    file: Option<&std::ffi::OsStr>,
    out: &Shared,
    err: &Shared,
) -> Result<Status, PipeError> {
    let mut status = Status::default();
    if let Some(file) = file {
        let f = std::fs::File::create(file).map_err(|e| {
            PipeError::error(format!(
                "can't create {}: {}",
                say::quote(file),
                say::io_reason(&e)
            ))
        })?;
        status.sinks.push(Box::new(f));
    }
    match fd {
        None => {}
        Some(1) => status.sinks.push(Box::new(out.clone())),
        Some(2) => status.sinks.push(Box::new(err.clone())),
        Some(fd) => status.sinks.push(fd_writer(fd)?),
    }
    Ok(status)
}

#[cfg(unix)]
fn fd_writer(fd: i32) -> Result<Box<dyn Write + Send + Sync>, PipeError> {
    struct Fd(i32);
    impl Write for Fd {
        fn write(&mut self, buf: &[u8]) -> std::io::Result<usize> {
            // SAFETY: writes from a valid slice to a descriptor the caller
            // named and owns; it is never closed here.
            let n = unsafe { libc::write(self.0, buf.as_ptr().cast(), buf.len()) };
            if n < 0 {
                Err(std::io::Error::last_os_error())
            } else {
                Ok(n as usize)
            }
        }
        fn flush(&mut self) -> std::io::Result<()> {
            Ok(())
        }
    }
    // SAFETY: fcntl only asks whether the descriptor is open.
    if unsafe { libc::fcntl(fd, libc::F_GETFD) } < 0 {
        return Err(PipeError::error(format!(
            "can't write status to descriptor {fd}: {}",
            say::io_reason(&std::io::Error::last_os_error())
        )));
    }
    Ok(Box::new(Fd(fd)))
}

#[cfg(not(unix))]
fn fd_writer(fd: i32) -> Result<Box<dyn Write + Send + Sync>, PipeError> {
    Err(PipeError::error(format!(
        "status descriptor {fd} is not available here; use 1, 2 or --status-file"
    )))
}

/// The key package, opened the first time an operation needs it: a message
/// encrypted with a passphrase only, or a listing of who a message is for,
/// needs none.
pub struct Package<'a> {
    parsed: &'a args::Args,
    host: &'a Host,
    opened: OnceCell<Result<(PathBuf, OpenPackage), PipeError>>,
}

impl<'a> Package<'a> {
    fn new(parsed: &'a args::Args, host: &'a Host) -> Self {
        Self {
            parsed,
            host,
            opened: OnceCell::new(),
        }
    }

    /// The package, or why there is none.
    pub fn get(&self) -> Result<(&Path, &OpenPackage), PipeError> {
        let opened = self.opened.get_or_init(|| {
            let path = package_path(self.parsed, self.host)?;
            let pack = open(self.parsed, &path, self.host)?;
            Ok((path, pack))
        });
        match opened {
            Ok((path, pack)) => Ok((path.as_path(), pack)),
            Err(e) => Err(e.clone()),
        }
    }

    /// The package when there is one.
    pub fn try_get(&self) -> Option<&OpenPackage> {
        self.get().ok().map(|(_, pack)| pack)
    }
}

fn dispatch(
    parsed: &args::Args,
    host: &Host,
    io: &mut Io,
    status: &mut Status,
) -> Result<(), PipeError> {
    let command = parsed
        .command
        .ok_or_else(|| PipeError::error("nothing to do"))?;
    match command {
        Command::Help => {
            let _ = io.stdout.write_all(HELP.as_bytes());
            Ok(())
        }
        Command::Version => {
            let _ = io.stdout.write_all(version_text(&host.version).as_bytes());
            Ok(())
        }
        _ => {
            if command == Command::Default {
                say::line(
                    &mut *io.stderr,
                    "WARNING: no command supplied.  Trying to guess what you mean ...",
                );
            }
            let package = Package::new(parsed, host);
            ops::run(command, parsed, &package, io, status)
        }
    }
}

/// What `--version` says.
pub fn version_text(version: &str) -> String {
    // rPGP by its minor release, the part that says what it can read.
    let rpgp: Vec<&str> = pgp::VERSION.split('.').take(2).collect();
    format!(
        "gfpipe (GpgFrontend) {version}\n\
         Signs, verifies, encrypts and decrypts with keys from a GpgFrontend key package.\n\
         Built for scripts and batch jobs: starts fast, no agent, no daemon.\n\
         Read-only: keys are managed in GpgFrontend.\n\
         \n\
         Key package format: v{}\n\
         OpenPGP engine: rPGP {}\n",
        keypack::FORMAT_VERSION,
        rpgp.join(".")
    )
}

/// Where the package is: `--keypack`, then `GFPIPE_KEYPACK`, then
/// [`DEFAULT_PACKAGE`]. A value that is not a path is a name, of a package
/// beside gfpipe.
pub fn package_path(parsed: &args::Args, host: &Host) -> Result<PathBuf, PipeError> {
    let named: Option<OsString> = parsed
        .keypack
        .clone()
        .or_else(|| host.env.get(ENV_KEYPACK).map(OsString::from))
        .filter(|s| !s.is_empty());
    let explicit = named.is_some();
    let named = named.unwrap_or_else(|| DEFAULT_PACKAGE.into());

    let text = named.to_string_lossy();
    let looks_like_path = text.contains('/')
        || text.contains(std::path::MAIN_SEPARATOR)
        || Path::new(&named)
            .extension()
            .is_some_and(|e| e == PACKAGE_EXTENSION);
    if looks_like_path {
        return Ok(PathBuf::from(named));
    }

    let file = format!("{text}.{PACKAGE_EXTENSION}");
    let Some(dir) = host.program_dir.as_ref() else {
        return Err(PipeError::error(format!(
            "cannot tell where gfpipe is, so {file} cannot be found beside it; name the \
             package with --keypack PATH"
        )));
    };
    let path = dir.join(&file);
    if path.is_file() {
        return Ok(path);
    }
    Err(PipeError::error(if explicit {
        format!("no key package {}", path.display())
    } else {
        format!(
            "no key package beside gfpipe ({}). Export gfpipe with a key package from \
             GpgFrontend, or name one with --keypack or {ENV_KEYPACK}.",
            path.display()
        )
    }))
}

fn open(parsed: &args::Args, path: &Path, host: &Host) -> Result<OpenPackage, PipeError> {
    // Read in place: the header now, then the index and only the blocks an
    // operation asks for.
    let file = std::fs::File::open(path).map_err(|e| {
        PipeError::error(format!(
            "cannot read the key package {}: {}. Export one for gfpipe from GpgFrontend.",
            path.display(),
            say::io_reason(&e)
        ))
    })?;
    let sealed = keypack::SealedPackage::from_file(file)
        .map_err(|_| PipeError::error(format!("{}: {}", path.display(), engine_detail())))?;
    let header = sealed.header();

    let secret: Zeroizing<Vec<u8>> = match &header.protection {
        Protection::PinFile => {
            return Err(PipeError::error(format!(
                "{} is protected by a PIN, which gfpipe cannot ask for. Export it with gfpipe \
                 from GpgFrontend, which protects it with a plain key.",
                path.display()
            )));
        }
        Protection::Plain(_) => plain_key(parsed, path, host)?,
    };

    sealed.unlock(Unlock::Secret(&secret)).map_err(|status| {
        let detail = engine_detail();
        if status == crate::types::GfrStatus::ErrorWrongPassword {
            PipeError::error(format!("the key does not open {}", path.display()))
        } else {
            PipeError::error(format!("{}: {detail}", path.display()))
        }
    })
}

/// A plain package's key: `--key-file`, then `GFPIPE_KEY`, then
/// `GFPIPE_KEY_FILE`, then the `.gfkey` file beside the package and named as
/// it is.
fn plain_key(
    parsed: &args::Args,
    package: &Path,
    host: &Host,
) -> Result<Zeroizing<Vec<u8>>, PipeError> {
    let read = |file: &Path, what: &str| -> Result<Zeroizing<String>, PipeError> {
        std::fs::read_to_string(file)
            .map(Zeroizing::new)
            .map_err(|e| {
                PipeError::error(format!(
                    "cannot read {what} ({}): {}",
                    file.display(),
                    say::io_reason(&e)
                ))
            })
    };
    let beside = package.with_extension(KEY_FILE_EXTENSION);
    let (text, from) = if let Some(file) = &parsed.key_file {
        (
            read(Path::new(file), "--key-file")?,
            "--key-file".to_string(),
        )
    } else if let Some(key) = host.env.get(ENV_KEY) {
        (Zeroizing::new(key.clone()), ENV_KEY.to_string())
    } else if let Some(file) = host.env.get(ENV_KEY_FILE) {
        (
            read(Path::new(file), ENV_KEY_FILE)?,
            ENV_KEY_FILE.to_string(),
        )
    } else if beside.is_file() {
        (read(&beside, "the key file")?, beside.display().to_string())
    } else {
        return Err(PipeError::error(format!(
            "{} opens with its key, which is not beside it ({}). Give it with --key-file, \
             {ENV_KEY} or {ENV_KEY_FILE}.",
            package.display(),
            beside.display()
        )));
    };
    keypack::parse_plain_key(&text)
        .map_err(|_| PipeError::error(format!("the key in {from} is not a key package key")))
}

const HELP: &str = "\
Usage: gfpipe <command> [options] [file]
       gfpipe [gpg options] [file]

Basic OpenPGP operations with the keys of one GpgFrontend key package.
Data is read from standard input and written to standard output, unless
files are named; signing or encrypting a named file writes FILE.gpg,
FILE.sig or FILE.asc next to it, as gpg does.

Commands:
  sign [--detach | --clear] [-u KEY]   sign (-s, -b, --clearsign)
  verify [SIGNATURE] [DATA]            verify a signature (--verify)
  encrypt -r KEY... [-s -u KEY]        encrypt, and sign with -s (-e)
  -c, --symmetric                      encrypt with a passphrase only
  decrypt                              decrypt, and verify if signed (-d)
  keys [KEY...]                        list keys (-k, -K, --with-colons)
  export [KEY...]                      public keys (--export, -a)
  version, help

Options:
  -u, --local-user KEY      the key to sign with
  -r, --recipient KEY       a key to encrypt to (only keys in the package)
  -a, --armor               ASCII armored output
  -o, --output FILE         write to FILE; --yes replaces an existing one
  --passphrase-file FILE    a passphrase: for a key that has one, or -c
  --passphrase-fd N         ... read from descriptor N (0: standard input)
  --status-fd N             gpg status lines on descriptor N
  --keypack NAME|PATH       the key package
  --key-file FILE           the file holding its key

The package: --keypack, then GFPIPE_KEYPACK, then default.gfpack beside
gfpipe; a name is of a package beside gfpipe. Its key, when it has a plain
one: --key-file, then GFPIPE_KEY, then GFPIPE_KEY_FILE, then the .gfkey file
beside the package. GpgFrontend puts gfpipe, the package and the key in a
folder together when it exports a key package with gfpipe.

gpg's command line is understood, so git and other tools can use gfpipe in
place of gpg:  git config --global gpg.program gfpipe
gfpipe does no key management; manage keys in GpgFrontend.
";

#[cfg(test)]
mod tests;
