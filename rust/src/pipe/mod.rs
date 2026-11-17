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

mod args;
mod ops;

use std::collections::HashMap;
use std::io::{Read, Write};
use std::path::{Path, PathBuf};

use zeroize::Zeroizing;

use crate::keypack::{self, OpenPackage, Protection, Unlock};

pub use args::Command;

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
pub struct Host {
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
#[derive(Debug)]
pub struct PipeError {
    pub code: i32,
    pub message: String,
}

impl PipeError {
    /// A usage or setup problem (gpg's code for errors).
    pub fn usage(message: impl Into<String>) -> Self {
        Self {
            code: 2,
            message: message.into(),
        }
    }

    /// The operation ran and failed, e.g. a bad signature.
    pub fn failed(message: impl Into<String>) -> Self {
        Self {
            code: 1,
            message: message.into(),
        }
    }
}

/// Lines for `--status-fd`, written once the operation is over.
#[derive(Default)]
pub struct Status {
    lines: Vec<String>,
}

impl Status {
    pub fn push(&mut self, line: impl AsRef<str>) {
        self.lines.push(format!("[GNUPG:] {}", line.as_ref()));
    }
}

/// Run gfpipe with `argv` (without the program name). Returns the exit code.
pub fn run(argv: &[String], host: &Host, io: &mut Io) -> i32 {
    let parsed = match args::parse(argv) {
        Ok(parsed) => parsed,
        Err(e) => {
            let _ = writeln!(io.stderr, "gfpipe: {e}");
            return 2;
        }
    };

    let mut status = Status::default();
    let result = dispatch(&parsed, host, io, &mut status);

    flush_status(parsed.status_fd, parsed.status_file.as_deref(), &status, io);
    let _ = io.stdout.flush();

    match result {
        Ok(()) => 0,
        Err(e) => {
            let _ = writeln!(io.stderr, "gfpipe: {}", e.message);
            e.code
        }
    }
}

fn dispatch(
    parsed: &args::Args,
    host: &Host,
    io: &mut Io,
    status: &mut Status,
) -> Result<(), PipeError> {
    let command = parsed.command.ok_or(PipeError::usage("nothing to do"))?;
    match command {
        Command::Help => {
            let _ = io.stdout.write_all(HELP.as_bytes());
            Ok(())
        }
        Command::Version => {
            let _ = io.stdout.write_all(version_text(&host.version).as_bytes());
            Ok(())
        }
        // A passphrase is all symmetric encryption needs.
        Command::Symmetric => ops::run_without_package(command, parsed, io, status),
        _ => {
            let path = package_path(parsed, host)?;
            let pack = open(parsed, &path, host)?;
            ops::run(command, parsed, &path, &pack, io, status)
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
    let named = parsed
        .keypack
        .clone()
        .or_else(|| host.env.get(ENV_KEYPACK).cloned())
        .filter(|s| !s.is_empty());
    let explicit = named.is_some();
    let named = named.unwrap_or_else(|| DEFAULT_PACKAGE.to_string());

    let looks_like_path = named.contains('/')
        || named.contains(std::path::MAIN_SEPARATOR)
        || Path::new(&named)
            .extension()
            .is_some_and(|e| e == PACKAGE_EXTENSION);
    if looks_like_path {
        return Ok(PathBuf::from(named));
    }

    let file = format!("{named}.{PACKAGE_EXTENSION}");
    let Some(dir) = host.program_dir.as_ref() else {
        return Err(PipeError::usage(format!(
            "cannot tell where gfpipe is, so {file} cannot be found beside it; name the \
             package with --keypack PATH"
        )));
    };
    let path = dir.join(&file);
    if path.is_file() {
        return Ok(path);
    }
    Err(PipeError::usage(if explicit {
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
        PipeError::usage(format!(
            "cannot read the key package {}: {e}. Export one for gfpipe from GpgFrontend.",
            path.display()
        ))
    })?;
    let sealed = keypack::SealedPackage::from_file(file)
        .map_err(|_| PipeError::usage(format!("{}: {}", path.display(), engine_detail())))?;
    let header = sealed.header();

    let secret: Zeroizing<Vec<u8>> = match &header.protection {
        Protection::PinFile => {
            return Err(PipeError::usage(format!(
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
            PipeError::usage(format!("the key does not open {}", path.display()))
        } else {
            PipeError::usage(format!("{}: {detail}", path.display()))
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
            .map_err(|e| PipeError::usage(format!("cannot read {what} ({}): {e}", file.display())))
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
        return Err(PipeError::usage(format!(
            "{} opens with its key, which is not beside it ({}). Give it with --key-file, \
             {ENV_KEY} or {ENV_KEY_FILE}.",
            package.display(),
            beside.display()
        )));
    };
    keypack::parse_plain_key(&text)
        .map_err(|_| PipeError::usage(format!("the key in {from} is not a key package key")))
}

fn flush_status(fd: Option<i32>, file: Option<&str>, status: &Status, io: &mut Io) {
    if status.lines.is_empty() {
        return;
    }
    let mut text = status.lines.join("\n");
    text.push('\n');
    if let Some(file) = file {
        if let Err(e) = std::fs::write(file, text.as_bytes()) {
            let _ = writeln!(io.stderr, "gfpipe: cannot write the status to {file}: {e}");
        }
    }
    let Some(fd) = fd else { return };
    match fd {
        1 => {
            let _ = io.stdout.write_all(text.as_bytes());
        }
        2 => {
            let _ = io.stderr.write_all(text.as_bytes());
        }
        _ => write_fd(fd, text.as_bytes()),
    }
}

#[cfg(unix)]
fn write_fd(fd: i32, mut bytes: &[u8]) {
    while !bytes.is_empty() {
        // SAFETY: writes from a valid slice to a descriptor the caller named.
        let n = unsafe { libc::write(fd, bytes.as_ptr().cast(), bytes.len()) };
        if n <= 0 {
            return;
        }
        bytes = &bytes[n as usize..];
    }
}

#[cfg(not(unix))]
fn write_fd(_fd: i32, _bytes: &[u8]) {
    // Only the standard streams are offered as status descriptors here.
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
