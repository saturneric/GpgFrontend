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

//! gfpipe's operations, on the rPGP engine, with the keys of the package
//! only. What they say follows gpg: its lines on standard error, its status
//! lines, and its exit codes.

use std::cell::RefCell;
use std::ffi::{CStr, CString, OsStr, OsString, c_char, c_void};
use std::fs::File;
use std::io::{Read, Seek, SeekFrom, Write};
use std::path::{Path, PathBuf};

use pgp::composed::{Deserializable, SignedPublicKey, SignedSecretKey};
use pgp::crypto::sym::SymmetricKeyAlgorithm;
use pgp::types::{KeyDetails, StringToKey};
use zeroize::Zeroizing;

use super::args::{Args, Command, Passphrase, Recipient, SignKind};
use super::say::{self, code};
use super::{Io, Package, PipeError, Status};
use crate::crypto::SignatureResultInternal;
use crate::keypack::{CAP_ENCRYPT, CAP_SIGN, Component, Found, KeyEntry, OpenPackage};
use crate::types::{
    GfrKeyAlgo, GfrPassphraseState, GfrPasswordFetchStatus, GfrRecipientStatus, GfrSignMode,
    GfrSignatureStatus, GfrStatus,
};
use crate::utils::key_armor_opts;

/// The engine channel gfpipe runs on; it has the process to itself.
const CHANNEL: i32 = 0;
/// Past this, a detached signature, the one input read into memory, is
/// refused: no real one comes near it.
const MAX_SIGNATURE: u64 = 16 << 20;
/// Past this, a file of keys shown by the default action is refused.
const MAX_KEYS_FILE: u64 = 64 << 20;

pub fn run(
    command: Command,
    args: &Args,
    package: &Package,
    io: &mut Io,
    status: &mut Status,
) -> Result<(), PipeError> {
    check_inputs(command, args, io)?;
    // Read before the data: with --passphrase-fd 0 the passphrase is the
    // first line of standard input and the data the rest, as with gpg.
    load_passphrase(args, io)?;
    let result = match command {
        Command::Sign(kind) => sign(kind, args, package, io, status),
        Command::Verify => verify(args, package, io, status),
        Command::Encrypt { sign } => encrypt(sign, args, package, io, status),
        Command::Symmetric => symmetric(args, io, status),
        Command::Decrypt => decrypt(args, package, io, status),
        Command::ListKeys { secret } => list(secret, args, package, io, status),
        Command::Export => export(args, package, io, status),
        Command::Default => default_action(args, package, io, status),
        Command::Version | Command::Help => Ok(()),
    };
    PASSPHRASE.with(|p| *p.borrow_mut() = None);
    result
}

/// gpg's checks on the file arguments: one file for an operation on data,
/// and standard input read once.
fn check_inputs(command: Command, args: &Args, io: &mut Io) -> Result<(), PipeError> {
    let usage = match command {
        Command::Sign(SignKind::Inline) => Some("--sign"),
        Command::Sign(SignKind::Detached) => Some("--detach-sign"),
        Command::Sign(SignKind::Clear) => Some("--clear-sign"),
        Command::Encrypt { sign: false } => Some("--encrypt"),
        Command::Encrypt { sign: true } => Some("--sign --encrypt"),
        Command::Symmetric => Some("--symmetric"),
        Command::Decrypt => Some("--decrypt"),
        _ => None,
    };
    let too_many = match command {
        Command::Verify => args.inputs.len() > 2,
        _ => usage.is_some() && args.inputs.len() > 1,
    };
    if too_many {
        // gpg's usage line has no program prefix.
        let operation = usage.unwrap_or("--verify [signature_file]");
        let _ = writeln!(
            io.stderr,
            "usage: {} [options] {operation} [filename]",
            say::program()
        );
        return Err(PipeError::error(""));
    }
    let stdin_reads = args.inputs.iter().filter(|i| *i == "-").count();
    if stdin_reads > 1 && !matches!(command, Command::ListKeys { .. } | Command::Export) {
        return Err(PipeError::error(
            "standard input can be read only once; name a file for all but one input",
        ));
    }
    Ok(())
}

// --------------------------------------------------------------- passphrase

thread_local! {
    /// The passphrase given on the command line, for this run only. An empty
    /// one given explicitly is still given.
    static PASSPHRASE: RefCell<Option<Zeroizing<Vec<u8>>>> = const { RefCell::new(None) };
}

fn has_passphrase() -> bool {
    PASSPHRASE.with(|p| p.borrow().is_some())
}

/// The first line of `reader`, read a byte at a time so nothing after it is
/// taken from a shared stream. A CR before the LF is dropped.
fn read_line(reader: &mut dyn Read) -> std::io::Result<Zeroizing<Vec<u8>>> {
    let mut line = Zeroizing::new(Vec::new());
    let mut byte = [0u8; 1];
    loop {
        match reader.read(&mut byte) {
            Ok(1) if byte[0] != b'\n' => line.push(byte[0]),
            Ok(_) => break,
            Err(e) if e.kind() == std::io::ErrorKind::Interrupted => {}
            Err(e) => return Err(e),
        }
    }
    if line.last() == Some(&b'\r') {
        line.pop();
    }
    Ok(line)
}

fn load_passphrase(args: &Args, io: &mut Io) -> Result<(), PipeError> {
    let cannot = |what: &str, e: std::io::Error| {
        PipeError::error(format!(
            "cannot read the passphrase from {what}: {}",
            say::io_reason(&e)
        ))
    };
    let passphrase = match &args.passphrase {
        None => None,
        Some(Passphrase::Text(text)) => Some(Zeroizing::new(text.as_bytes().to_vec())),
        Some(Passphrase::File(path)) => {
            let what = say::quote(path);
            let mut file = File::open(path).map_err(|e| cannot(&what, e))?;
            Some(read_line(&mut file).map_err(|e| cannot(&what, e))?)
        }
        Some(Passphrase::Fd(0)) => {
            Some(read_line(&mut *io.stdin).map_err(|e| cannot("standard input", e))?)
        }
        Some(Passphrase::Fd(fd)) => {
            Some(read_fd_line(*fd).map_err(|e| cannot("its descriptor", e))?)
        }
    };
    PASSPHRASE.with(|p| *p.borrow_mut() = passphrase);
    Ok(())
}

#[cfg(unix)]
fn read_fd_line(fd: i32) -> std::io::Result<Zeroizing<Vec<u8>>> {
    use std::os::fd::FromRawFd;
    // SAFETY: fcntl only asks whether the descriptor is open.
    if unsafe { libc::fcntl(fd, libc::F_GETFD) } < 0 {
        return Err(std::io::Error::last_os_error());
    }
    // The caller owns the descriptor; it is read, not closed. It is open and
    // not negative (the parser refuses those).
    let mut file = std::mem::ManuallyDrop::new(unsafe { File::from_raw_fd(fd) });
    read_line(&mut *file)
}

#[cfg(not(unix))]
fn read_fd_line(_fd: i32) -> std::io::Result<Zeroizing<Vec<u8>>> {
    Err(std::io::Error::other(
        "only descriptor 0 is supported here; use --passphrase-file",
    ))
}

/// Hands the engine the passphrase from the command line, once: gfpipe never
/// prompts, so a wrong one is not asked for again.
extern "C" fn pipe_password(
    _channel: i32,
    state: GfrPassphraseState,
    out_pwd: *mut *mut u8,
    out_status: *mut GfrPasswordFetchStatus,
    _user_data: *mut c_void,
) -> i32 {
    let given = PASSPHRASE.with(|p| p.borrow().clone());
    match given {
        Some(passphrase) if !state.retry => {
            let boxed = passphrase.to_vec().into_boxed_slice();
            let len = boxed.len();
            // Freed by the engine through the host's secure free routine.
            unsafe {
                *out_pwd = Box::into_raw(boxed).cast::<u8>();
                *out_status = GfrPasswordFetchStatus::Provided;
            }
            i32::try_from(len).unwrap_or(0)
        }
        _ => {
            unsafe { *out_status = GfrPasswordFetchStatus::Failed };
            0
        }
    }
}

// ------------------------------------------------------- operations' words

/// An operation, for gpg's failure lines.
#[derive(Clone, Copy, PartialEq, Eq)]
enum Op {
    Sign,
    Encrypt,
    Symmetric,
    Decrypt,
    Verify,
    Default,
}

impl Op {
    /// The word of gpg's `FAILURE` status line.
    fn status(self) -> &'static str {
        match self {
            Op::Sign => "sign",
            Op::Encrypt => "encrypt",
            Op::Symmetric => "symencrypt",
            Op::Decrypt => "decrypt",
            Op::Verify => "verify",
            Op::Default => "-",
        }
    }

    /// What gpg says failed, for the input `label`.
    fn failed(self, label: &str) -> String {
        match self {
            Op::Sign => "signing failed".into(),
            Op::Encrypt => format!("{label}: encryption failed"),
            Op::Symmetric => format!("symmetric encryption of '{label}' failed"),
            Op::Decrypt => "decrypt_message failed".into(),
            Op::Verify => "verify signatures failed".into(),
            Op::Default => "processing message failed".into(),
        }
    }

    fn error(self, label: &str, reason: &str, code: u32) -> PipeError {
        PipeError::error(format!("{}: {reason}", self.failed(label))).failure(self.status(), code)
    }

    fn io_error(self, label: &str, e: &std::io::Error) -> PipeError {
        self.error(label, &say::io_reason(e), code::system(e))
    }
}

// ------------------------------------------------------------------ inputs

/// The `index`th file argument as gpg names it in messages.
fn label(args: &Args, index: usize) -> String {
    match args.inputs.get(index) {
        Some(name) if name != "-" => name.to_string_lossy().into_owned(),
        _ => "[stdin]".into(),
    }
}

fn open_named(name: &OsStr, op: Op, label: &str, io: &mut Io) -> Result<File, PipeError> {
    File::open(name).map_err(|e| {
        say::line(
            &mut *io.stderr,
            format!("can't open {}: {}", say::quote(name), say::io_reason(&e)),
        );
        op.io_error(label, &e)
    })
}

/// The `index`th file argument, or standard input.
fn input(
    args: &Args,
    index: usize,
    op: Op,
    io: &mut Io,
) -> Result<Box<dyn Read + Send + Sync>, PipeError> {
    match args.inputs.get(index) {
        Some(file) if file != "-" => {
            let f = open_named(file, op, &label(args, index), io)?;
            Ok(Box::new(f))
        }
        _ => Ok(std::mem::replace(&mut io.stdin, Box::new(std::io::empty()))),
    }
}

/// A detached signature, read whole.
fn read_signature(mut reader: impl Read) -> Result<Vec<u8>, PipeError> {
    let mut data = Vec::new();
    reader
        .by_ref()
        .take(MAX_SIGNATURE + 1)
        .read_to_end(&mut data)
        .map_err(|e| Op::Verify.io_error("", &e))?;
    if data.len() as u64 > MAX_SIGNATURE {
        return Err(PipeError::error("the signature is too large"));
    }
    Ok(data)
}

/// The `index`th input as a file, for operations that stream: a named file
/// as it is, standard input spooled into an anonymous temporary file, so
/// input of any size takes flat memory. Its length, for the engine's guards.
/// The spool is in the OS temporary directory (`TMPDIR` on Unix): standard
/// input has no directory of its own.
fn input_file(args: &Args, index: usize, op: Op, io: &mut Io) -> Result<(File, u64), PipeError> {
    let what = label(args, index);
    let file = match args.inputs.get(index) {
        Some(name) if name != "-" => open_named(name, op, &what, io)?,
        _ => {
            let cannot = |e: std::io::Error| op.io_error(&what, &e);
            // No name, so it disappears on every exit path.
            let mut spool = tempfile::tempfile().map_err(cannot)?;
            let mut stdin = std::mem::replace(&mut io.stdin, Box::new(std::io::empty()));
            std::io::copy(&mut stdin, &mut spool).map_err(cannot)?;
            spool.seek(SeekFrom::Start(0)).map_err(cannot)?;
            spool
        }
    };
    let len = file.metadata().map_err(|e| op.io_error(&what, &e))?.len();
    Ok((file, len))
}

/// What the start of the input is.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
enum Kind {
    Empty,
    NotOpenPgp,
    Encrypted,
    /// Signed packets: an inline-signed or a plain literal message.
    Signed,
    ClearSigned,
    /// Signatures alone: a detached signature.
    Signature,
    Keys,
    /// OpenPGP packets, the first of which cannot be read.
    Invalid,
}

/// What the start of a message says, read without loading the rest.
struct Head {
    kind: Kind,
    armored: bool,
    /// Where the armor begins: text before it, as in a mail, is skipped.
    armor_at: u64,
    /// The first packets, dearmored: enough for the first packet's tag and
    /// the recipients, which come first.
    packets: Vec<u8>,
    /// Why the first packet cannot be read, for [`Kind::Invalid`].
    invalid: String,
}

/// How much of a message's start is read to look at it.
const HEAD_BYTES: u64 = 1 << 20;
const ARMOR_BEGIN: &[u8] = b"-----BEGIN PGP ";

/// Where the first line starting an armored block is, as gpg looks for it.
fn armor_start(raw: &[u8]) -> Option<usize> {
    let mut at = 0;
    loop {
        if raw[at..].starts_with(ARMOR_BEGIN) {
            return Some(at);
        }
        at += raw[at..].iter().position(|&b| b == b'\n')? + 1;
    }
}

fn head_of(file: &mut File) -> std::io::Result<Head> {
    let mut raw = Vec::new();
    Read::by_ref(file).take(HEAD_BYTES).read_to_end(&mut raw)?;
    file.seek(SeekFrom::Start(0))?;

    // All of it was read: its first packet can be checked whole.
    let whole = (raw.len() as u64) < HEAD_BYTES;
    if raw.is_empty() {
        return Ok(Head {
            kind: Kind::Empty,
            armored: false,
            armor_at: 0,
            packets: raw,
            invalid: String::new(),
        });
    }
    let Some(at) = armor_start(&raw) else {
        let (kind, invalid) = checked(kind_of_packets(&raw), &raw, whole);
        return Ok(Head {
            kind,
            armored: false,
            armor_at: 0,
            packets: raw,
            invalid,
        });
    };

    let block = &raw[at + ARMOR_BEGIN.len()..];
    let name_end = block
        .windows(5)
        .position(|w| w == b"-----")
        .unwrap_or(block.len());
    let name = String::from_utf8_lossy(&block[..name_end]).to_string();
    // Streamed through the armor: a cut-off block still yields its start.
    let mut packets = Vec::new();
    let _ = pgp::armor::Dearmor::new(std::io::BufReader::new(&raw[at..]))
        .take(HEAD_BYTES)
        .read_to_end(&mut packets);
    let (kind, invalid) = match name.as_str() {
        "SIGNED MESSAGE" => (Kind::ClearSigned, String::new()),
        "SIGNATURE" => (Kind::Signature, String::new()),
        "PUBLIC KEY BLOCK" | "PRIVATE KEY BLOCK" | "SECRET KEY BLOCK" => {
            (Kind::Keys, String::new())
        }
        _ => checked(kind_of_packets(&packets), &packets, whole),
    };
    Ok(Head {
        kind,
        armored: true,
        armor_at: at as u64,
        packets,
        invalid,
    })
}

/// `kind`, or [`Kind::Invalid`] when the first packet of a message read
/// whole cannot be parsed, as gpg tells such a message apart.
fn checked(kind: Kind, packets: &[u8], whole: bool) -> (Kind, String) {
    if !whole || matches!(kind, Kind::Empty | Kind::NotOpenPgp) {
        return (kind, String::new());
    }
    match pgp::packet::PacketParser::new(std::io::Cursor::new(packets)).next() {
        Some(Err(e)) => (Kind::Invalid, e.to_string()),
        _ => (kind, String::new()),
    }
}

/// gpg's lines for a message whose first packet cannot be read.
fn invalid_packet(head: &Head, io: &mut Io, status: &mut Status) {
    say::line(&mut *io.stderr, format!("invalid packet: {}", head.invalid));
    status.push("NODATA 3");
}

/// What a message is by its first packet (RFC 9580 §5).
fn kind_of_packets(packets: &[u8]) -> Kind {
    if packets.first().is_none_or(|b| b & 0x80 == 0) {
        return Kind::NotOpenPgp;
    }
    match crate::keypack::first_packet_tag(packets) {
        // PKESK, SKESK, SED, SEIPD, and LibrePGP's OCB packet.
        Some(1 | 3 | 9 | 18 | 20) => Kind::Encrypted,
        Some(2) if is_detached_signature(packets) => Kind::Signature,
        Some(2 | 4 | 8 | 11) => Kind::Signed,
        Some(5 | 6 | 7 | 14) => Kind::Keys,
        _ => Kind::NotOpenPgp,
    }
}

/// Whether `packets` are only signatures: a detached signature.
fn is_detached_signature(packets: &[u8]) -> bool {
    use pgp::packet::{Packet, PacketParser};
    let mut parsed = PacketParser::new(std::io::Cursor::new(packets)).peekable();
    parsed.peek().is_some() && parsed.all(|p| matches!(p, Ok(Packet::Signature(_))))
}

/// The input from where its armor begins, so the engine never sees the text
/// before it.
fn from_armor(file: File, len: u64, head: &Head, op: Op) -> Result<(File, u64), PipeError> {
    if head.armor_at == 0 {
        return Ok((file, len));
    }
    let cannot = |e: std::io::Error| op.io_error("", &e);
    let mut file = file;
    file.seek(SeekFrom::Start(head.armor_at)).map_err(cannot)?;
    let mut spool = tempfile::tempfile().map_err(cannot)?;
    std::io::copy(&mut file, &mut spool).map_err(cannot)?;
    spool.seek(SeekFrom::Start(0)).map_err(cannot)?;
    Ok((spool, len - head.armor_at))
}

/// gpg's lines for input that is no OpenPGP data at all.
fn no_openpgp_data(head: &Head, op: Op, io: &mut Io, status: &mut Status) -> PipeError {
    if head.kind != Kind::Empty {
        say::line(&mut *io.stderr, "no valid OpenPGP data found.");
        status.push("NODATA 1");
    }
    status.push("NODATA 2");
    op.error("", "Unknown system error", code::UNKNOWN)
}

// ----------------------------------------------------------------- outputs

/// Where an operation writes.
enum Target {
    Stdout,
    File(PathBuf),
}

/// The `-o` file, else `default`, else standard output.
fn target(args: &Args, default: Option<PathBuf>) -> Target {
    match args.output.as_deref() {
        Some(out) if out == "-" => Target::Stdout,
        Some(out) => Target::File(PathBuf::from(out)),
        None => default.map_or(Target::Stdout, Target::File),
    }
}

/// As gpg does when it signs or encrypts a named file: that file with
/// `extension` added.
fn beside_input(args: &Args, extension: &str) -> Option<PathBuf> {
    args.inputs.first().filter(|f| *f != "-").map(|f| {
        let mut name = f.clone();
        name.push(extension);
        PathBuf::from(name)
    })
}

/// What an operation writes into.
enum Sink<'a> {
    /// Standard output, or a file that is not a regular one.
    Stream(&'a mut (dyn Write + Send + Sync)),
    /// A temporary file beside the output, renamed over it only on success
    /// and deleted otherwise.
    Stage(&'a mut File),
}

impl Sink<'_> {
    fn writer(&mut self) -> &mut (dyn Write + Send + Sync) {
        match self {
            Sink::Stream(w) => &mut **w,
            Sink::Stage(f) => &mut **f,
        }
    }
}

fn same_file(a: &Path, b: &Path) -> bool {
    match (std::fs::canonicalize(a), std::fs::canonicalize(b)) {
        (Ok(a), Ok(b)) => a == b,
        _ => false,
    }
}

/// A device name that is no file on Windows.
fn is_device_name(path: &Path) -> bool {
    cfg!(windows)
        && path
            .file_stem()
            .and_then(OsStr::to_str)
            .is_some_and(|s| ["nul", "con"].contains(&s.to_ascii_lowercase().as_str()))
}

/// Run `f` with the output. A regular file is written beside the output
/// and renamed over it once `f` succeeds, so a failed run leaves no file
/// behind and keeps the one it would have replaced; one that exists is
/// replaced only with `--yes`. A device, a pipe or a FIFO is written in
/// place and needs no `--yes`, as with gpg. The output is never one of the
/// inputs: it would be emptied before it is read.
fn with_output<T>(
    args: &Args,
    io: &mut Io,
    target: Target,
    op: Op,
    input_label: &str,
    f: impl FnOnce(Sink<'_>) -> Result<T, PipeError>,
) -> Result<T, PipeError> {
    let path = match target {
        Target::Stdout => return f(Sink::Stream(&mut *io.stdout)),
        Target::File(path) => path,
    };
    if args
        .inputs
        .iter()
        .any(|i| i != "-" && same_file(&path, Path::new(i)))
    {
        return Err(PipeError::error(format!(
            "{} is also an input; name another output",
            say::quote(path.as_os_str())
        )));
    }
    // A link is followed: what it points to is replaced, not the link.
    let path = match std::fs::symlink_metadata(&path) {
        Ok(m) if m.file_type().is_symlink() => std::fs::canonicalize(&path).unwrap_or(path),
        _ => path,
    };
    let exists = |op: Op| {
        op.error(
            input_label,
            "File exists",
            code::system(&std::io::Error::from(std::io::ErrorKind::AlreadyExists)),
        )
    };

    let replaced = match std::fs::metadata(&path) {
        _ if is_device_name(&path) => None,
        Ok(m) if m.is_dir() => return Err(exists(op)),
        Ok(m) if !m.is_file() => None,
        Ok(_) if !args.yes => return Err(exists(op)),
        Ok(m) => Some(Some(m.permissions())),
        Err(e) if e.kind() == std::io::ErrorKind::NotFound => Some(None),
        Err(e) => return Err(op.io_error(input_label, &e)),
    };
    let Some(replaced) = replaced else {
        let mut out = std::fs::OpenOptions::new()
            .write(true)
            .open(&path)
            .map_err(|e| op.io_error(input_label, &e))?;
        let value = f(Sink::Stream(&mut out))?;
        out.flush().map_err(|e| op.io_error(input_label, &e))?;
        return Ok(value);
    };

    let dir = match path.parent() {
        Some(d) if !d.as_os_str().is_empty() => d.to_path_buf(),
        _ => PathBuf::from("."),
    };
    let mut builder = tempfile::Builder::new();
    builder.prefix(".gfpipe-").suffix(".tmp");
    // A new file gets what the umask leaves of 0666, as gpg's would.
    #[cfg(unix)]
    let mode = {
        use std::os::unix::fs::PermissionsExt;
        std::fs::Permissions::from_mode(0o666)
    };
    #[cfg(unix)]
    builder.permissions(mode);
    let mut stage = builder
        .tempfile_in(&dir)
        .map_err(|e| op.io_error(input_label, &e))?;
    let value = f(Sink::Stage(stage.as_file_mut()))?;
    stage
        .as_file_mut()
        .flush()
        .map_err(|e| op.io_error(input_label, &e))?;
    if let Some(permissions) = replaced {
        let _ = std::fs::set_permissions(stage.path(), permissions);
    }
    stage
        .persist(&path)
        .map_err(|e| op.io_error(input_label, &e.error))?;
    Ok(value)
}

/// gpg's file name extension for what it writes.
fn extension(args: &Args, detached: bool) -> &'static str {
    if args.armor {
        ".asc"
    } else if detached {
        ".sig"
    } else {
        ".gpg"
    }
}

// -------------------------------------------------------------------- keys

/// The engine's own words for the last failure, or the status.
fn engine_error(status: GfrStatus) -> String {
    if status == GfrStatus::ErrorFetchPasswordFailed {
        // gfpipe hands over only the passphrase it was given, and the engine
        // takes no empty one.
        let _ = super::engine_detail();
        return "No passphrase given".into();
    }
    let detail = super::engine_detail();
    if detail.is_empty() {
        format!("{status:?}")
    } else {
        detail
    }
}

/// An armored block, pinned to one component when the query named it.
fn pinned(block: &str, exact: Option<&str>) -> Zeroizing<String> {
    Zeroizing::new(match exact {
        Some(fpr) => format!("{fpr}!\n{block}"),
        None => block.to_string(),
    })
}

/// The engine takes keys as armored text; the package keeps them binary.
fn armored_public(binary: &[u8]) -> Option<Zeroizing<String>> {
    let key = SignedPublicKey::from_bytes(binary).ok()?;
    key.to_armored_string(key_armor_opts(key.primary_key.version()))
        .ok()
        .map(Zeroizing::new)
}

fn armored_secret(binary: &[u8]) -> Option<Zeroizing<String>> {
    let key = SignedSecretKey::from_bytes(binary).ok()?;
    key.to_armored_string(key_armor_opts(key.primary_key.version()))
        .ok()
        .map(Zeroizing::new)
}

fn damaged() -> PipeError {
    PipeError::error(format!(
        "the key package cannot be read: {}",
        super::engine_detail()
    ))
}

fn now() -> u64 {
    std::time::SystemTime::now()
        .duration_since(std::time::UNIX_EPOCH)
        .map_or(0, |d| d.as_secs())
}

/// Every key `query` names, each once.
fn keys_named(pack: &OpenPackage, query: &str) -> Result<Vec<Found>, PipeError> {
    pack.find_all(query).map_err(|_| damaged())
}

/// The components of `found` that could do what `cap` asks: the one the
/// query pinned, or every one of the key with the capability.
fn candidates(pack: &OpenPackage, found: &Found, cap: u8) -> Result<Vec<Component>, PipeError> {
    if found.exact.is_some() {
        return Ok(vec![found.component]);
    }
    Ok(pack
        .key_components(&found.entry, &found.component)
        .map_err(|_| damaged())?
        .into_iter()
        .filter(|c| c.can(cap))
        .collect())
}

fn usable(c: &Component) -> bool {
    !c.revoked && !c.expired_at(now())
}

/// Why no key of a name could be used, with gpg's reason code.
enum Refusal {
    Missing,
    Unusable(Vec<Found>),
    Ambiguous,
}

/// The one key a name gives for `cap`: of the keys it names, the only one
/// that can do it (gpg takes the usable one of several).
fn pick(
    pack: &OpenPackage,
    query: &str,
    cap: u8,
    need_secret: bool,
) -> Result<Result<Found, Refusal>, PipeError> {
    let named: Vec<Found> = keys_named(pack, query)?
        .into_iter()
        .filter(|f| !need_secret || f.component.has_secret)
        .collect();
    if named.is_empty() {
        return Ok(Err(Refusal::Missing));
    }
    let (mut good, mut bad) = (Vec::new(), Vec::new());
    for f in named {
        if candidates(pack, &f, cap)?.iter().any(usable) {
            good.push(f);
        } else {
            bad.push(f);
        }
    }
    Ok(match good.len() {
        0 => Err(Refusal::Unusable(bad)),
        1 => Ok(good.remove(0)),
        _ => Err(Refusal::Ambiguous),
    })
}

/// The package's only secret key, for a command that names none.
fn only_secret_key(pack: &OpenPackage) -> Result<Option<KeyEntry>, PipeError> {
    let secret: Vec<_> = pack
        .primaries()
        .map_err(|_| damaged())?
        .into_iter()
        .filter(|c| c.has_secret)
        .collect();
    match secret.as_slice() {
        [only] => Ok(Some(pack.entry(only).map_err(|_| damaged())?)),
        [] => Ok(None),
        _ => Err(PipeError::error(
            "the key package holds several secret keys; name one with -u",
        )),
    }
}

/// The secret key block for each signer named by `-u` (else
/// `--default-key`), or the package's only secret key. What can be decided
/// from the index is decided before any key is decrypted.
fn signers(
    args: &Args,
    pack: &OpenPackage,
    io: &mut Io,
    status: &mut Status,
) -> Result<Vec<(Zeroizing<String>, KeyEntry)>, PipeError> {
    let names: Vec<String> = if args.local_users.is_empty() {
        args.default_key.clone().into_iter().collect()
    } else {
        args.local_users.clone()
    };

    let mut chosen: Vec<Found> = Vec::new();
    if names.is_empty() {
        let Some(entry) = only_secret_key(pack)? else {
            say::line(&mut *io.stderr, "no default secret key: No secret key");
            return Err(Op::Sign.error("", "No secret key", code::NO_SECKEY));
        };
        let found = pick(pack, &entry.fpr_hex(), CAP_SIGN, true)?;
        match found {
            Ok(found) => chosen.push(found),
            Err(_) => return Err(cannot_sign(&entry, GfrStatus::ErrorUnusableKey)),
        }
    }
    for name in &names {
        let (reason, code) = match pick(pack, name, CAP_SIGN, true)? {
            Ok(found) => {
                chosen.push(found);
                continue;
            }
            Err(Refusal::Missing) => ("9", code::NO_SECKEY),
            Err(Refusal::Unusable(found)) => {
                // Say how to get a usable key back.
                if let Some(f) = found.first() {
                    say::line(
                        &mut *io.stderr,
                        cannot_sign(&f.entry, GfrStatus::ErrorUnusableKey).message,
                    );
                }
                ("0", code::UNUSABLE_SECKEY)
            }
            Err(Refusal::Ambiguous) => ("2", code::AMBIGUOUS_NAME),
        };
        status.push(format!("INV_SGNR {reason} {name}"));
        say::line(
            &mut *io.stderr,
            format!("skipped \"{name}\": {}", code::reason(code)),
        );
        return Err(Op::Sign.error("", code::reason(code), code));
    }

    chosen
        .into_iter()
        .map(|found| {
            let entry = &found.entry;
            let ready: Vec<_> = candidates(pack, &found, CAP_SIGN)?
                .into_iter()
                .filter(usable)
                .collect();
            if ready.iter().all(|c| c.protected) && !has_passphrase() {
                return Err(PipeError::error(format!(
                    "the secret key {} has a passphrase, which gfpipe does not ask for. Give it \
                     with --passphrase-file or --passphrase-fd, or export the package again \
                     with \"Remove the passphrase\".",
                    entry.fpr_hex()
                )));
            }
            let secret = pack
                .secret_key(entry)
                .map_err(|_| damaged())?
                .and_then(|b| armored_secret(&b))
                .ok_or_else(damaged)?;
            Ok((pinned(&secret, found.exact.as_deref()), found.entry))
        })
        .collect()
}

fn cannot_sign(entry: &KeyEntry, status: GfrStatus) -> PipeError {
    if status == GfrStatus::ErrorUnusableKey {
        PipeError::error(format!(
            "the key {} ({}) cannot sign: no subkey that may sign is valid. A package holds \
             the keys as they were when it was exported; renew the key in GpgFrontend and \
             export the package again.",
            entry.fpr_hex(),
            entry.primary_user_id()
        ))
        .failure("sign", code::UNUSABLE_SECKEY)
    } else if status == GfrStatus::ErrorBadPassphrase {
        Op::Sign.error("", "Bad passphrase", code::gpg(code::BAD_PASSPHRASE))
    } else {
        Op::Sign.error("", &engine_error(status), code::gpg(code::GENERAL))
    }
}

/// The numeric OpenPGP id of an algorithm the engine names by its rPGP name.
fn algo_id<T>(name: &str) -> u8
where
    T: From<u8> + std::fmt::Debug,
{
    (0..=u8::MAX)
        .find(|&n| format!("{:?}", T::from(n)).eq_ignore_ascii_case(name))
        .unwrap_or(0)
}

fn pk_algo_id(name: &str) -> u8 {
    algo_id::<pgp::crypto::public_key::PublicKeyAlgorithm>(name)
}

fn hash_algo_id(name: &str) -> u8 {
    algo_id::<pgp::crypto::hash::HashAlgorithm>(name)
}

/// The OpenPGP ids of a payload's cipher and AEAD mode, from the engine's
/// "AES256.OCB" (a SEIPD v1 is "<cipher>.CFB"): gpg's DECRYPTION_INFO.
fn decryption_info(sym_algo: &str) -> Option<String> {
    let (cipher, mode) = sym_algo.split_once('.')?;
    let cipher = match cipher.to_ascii_uppercase().as_str() {
        "IDEA" => 1,
        "3DES" | "TRIPLEDES" => 2,
        "CAST5" => 3,
        "BLOWFISH" => 4,
        "AES" | "AES128" => 7,
        "AES192" => 8,
        "AES256" => 9,
        "TWOFISH" => 10,
        "CAMELLIA128" => 11,
        "CAMELLIA192" => 12,
        "CAMELLIA256" => 13,
        _ => return None,
    };
    Some(match mode.to_ascii_uppercase().as_str() {
        "EAX" => format!("0 {cipher} 1"),
        "OCB" => format!("0 {cipher} 2"),
        "GCM" => format!("0 {cipher} 3"),
        _ => format!("2 {cipher} 0"),
    })
}

/// `YYYY-MM-DD` of a Unix time.
fn date_of(unix: u32) -> String {
    // Howard Hinnant's civil-from-days.
    let z = i64::from(unix) / 86_400 + 719_468;
    let era = z.div_euclid(146_097);
    let doe = z - era * 146_097;
    let yoe = (doe - doe / 1460 + doe / 36_524 - doe / 146_096) / 365;
    let doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    let mp = (5 * doy + 2) / 153;
    let d = doy - (153 * mp + 2) / 5 + 1;
    let m = if mp < 10 { mp + 3 } else { mp - 9 };
    let y = yoe + era * 400 + i64::from(m <= 2);
    format!("{y:04}-{m:02}-{d:02}")
}

/// gpg's name of a key's algorithm and size in a listing: "ed25519",
/// "rsa3072", "nistp256".
fn algo_label(algo: GfrKeyAlgo, key_length: u32, pk: u8) -> String {
    use GfrKeyAlgo as A;
    match algo {
        A::RSA1024 | A::RSA2048 | A::RSA3072 | A::RSA4096 => format!("rsa{key_length}"),
        A::DSA1024 | A::DSA2048 | A::DSA3072 => format!("dsa{key_length}"),
        A::KYBER768X25519 => "ky768_cv25519".into(),
        A::KYBER1024X448 => "ky1024_cv448".into(),
        A::MLDSA65ED25519 => "mldsa65_ed25519".into(),
        A::MLDSA87ED448 => "mldsa87_ed448".into(),
        A::SLHDSASHAKE128S | A::SLHDSASHAKE128F | A::SLHDSASHAKE256S => "slhdsa".into(),
        A::Unknown => match pk {
            1..=3 => format!("rsa{key_length}"),
            16 | 20 => format!("elg{key_length}"),
            17 => format!("dsa{key_length}"),
            _ => format!("unknown{pk}"),
        },
        other => curve_name(other).to_string(),
    }
}

/// The curve name gpg prints in field 17 of a colon listing.
fn curve_name(algo: GfrKeyAlgo) -> &'static str {
    match algo {
        GfrKeyAlgo::ED25519 | GfrKeyAlgo::ED25519LEGACY => "ed25519",
        GfrKeyAlgo::CV25519 => "cv25519",
        GfrKeyAlgo::ED448 => "ed448",
        GfrKeyAlgo::X448 => "cv448",
        GfrKeyAlgo::NISTP256 => "nistp256",
        GfrKeyAlgo::NISTP384 => "nistp384",
        GfrKeyAlgo::NISTP521 => "nistp521",
        GfrKeyAlgo::BRAINPOOLP256 => "brainpoolP256r1",
        GfrKeyAlgo::BRAINPOOLP384 => "brainpoolP384r1",
        GfrKeyAlgo::BRAINPOOLP512 => "brainpoolP512r1",
        GfrKeyAlgo::SECP256K1 => "secp256k1",
        _ => "",
    }
}

/// The algorithm label, size and creation date of the component `fpr` of a
/// certificate.
struct Shape {
    label: String,
    key_length: u32,
    curve: &'static str,
}

fn shape_of(entry: &KeyEntry, component: &Component, fpr: &str) -> Shape {
    let meta = SignedPublicKey::from_bytes(&entry.public_key[..])
        .ok()
        .map(|cert| crate::key::build_public_metadata(&cert));
    let (algo, key_length) = meta
        .as_ref()
        .and_then(|m| {
            if m.fpr.eq_ignore_ascii_case(fpr) {
                Some((m.algo, m.key_length))
            } else {
                m.subkeys
                    .iter()
                    .find(|s| s.fpr.eq_ignore_ascii_case(fpr))
                    .map(|s| (s.algo, s.key_length))
            }
        })
        .unwrap_or((GfrKeyAlgo::Unknown, 0));
    Shape {
        label: algo_label(algo, key_length, component.algo),
        key_length,
        curve: curve_name(algo),
    }
}

// ----------------------------------------------------- engine key callbacks

/// Hands the engine the public certificate the issuer `id` belongs to.
extern "C" fn fetch_public(id: *const c_char, user_data: *mut c_void) -> *mut c_char {
    lookup(id, user_data, |_, entry| armored_public(&entry.public_key))
}

/// Hands the engine the secret key the key ID `id` belongs to. For a hidden
/// recipient (a key ID of zeros), the package's first secret key that may
/// decrypt: the only one the engine tries.
extern "C" fn fetch_secret(id: *const c_char, user_data: *mut c_void) -> *mut c_char {
    let wildcard = !id.is_null()
        && unsafe { CStr::from_ptr(id) }
            .to_bytes()
            .iter()
            .all(|&b| b == b'0');
    if wildcard && !user_data.is_null() {
        // SAFETY: as in `lookup`.
        let pack = unsafe { &*(user_data as *const OpenPackage) };
        let mut primaries: Vec<Component> = Vec::new();
        for c in pack.components().unwrap_or_default() {
            if c.has_secret
                && c.can(CAP_ENCRYPT)
                && usable(&c)
                && !primaries.iter().any(|p| p.same_key(&c))
            {
                primaries.push(c);
            }
        }
        // Every secret key that may decrypt, in one block: the engine tries
        // each, as gpg tries all its secret keys.
        let mut keys = Zeroizing::new(Vec::new());
        for c in &primaries {
            if let Some(binary) = pack
                .entry(c)
                .ok()
                .and_then(|e| pack.secret_key(&e).ok().flatten())
            {
                keys.extend_from_slice(&binary);
            }
        }
        if keys.is_empty() {
            return std::ptr::null_mut();
        }
        let mut armored = Zeroizing::new(Vec::new());
        if pgp::armor::write(
            &RawPackets(keys.to_vec()),
            pgp::armor::BlockType::PrivateKey,
            &mut *armored,
            None,
            true,
        )
        .is_err()
        {
            return std::ptr::null_mut();
        }
        return CString::new(armored.to_vec()).map_or(std::ptr::null_mut(), CString::into_raw);
    }
    lookup(id, user_data, |pack, entry| {
        armored_secret(&pack.secret_key(entry).ok()??)
    })
}

fn lookup(
    id: *const c_char,
    user_data: *mut c_void,
    pick: impl Fn(&OpenPackage, &KeyEntry) -> Option<Zeroizing<String>>,
) -> *mut c_char {
    if id.is_null() || user_data.is_null() {
        return std::ptr::null_mut();
    }
    // SAFETY: `user_data` is the &OpenPackage the operation passed in, alive
    // for the duration of the engine call that calls back.
    let pack = unsafe { &*(user_data as *const OpenPackage) };
    let Ok(id) = (unsafe { CStr::from_ptr(id) }).to_str() else {
        return std::ptr::null_mut();
    };
    // By exact identifier only: an issuer is never a user ID.
    pack.find(id)
        .ok()
        .flatten()
        .and_then(|found| pick(pack, &found.entry))
        .and_then(|block| CString::new(block.as_str()).ok())
        // Freed by the engine through gfc_secure_free_cstr.
        .map_or(std::ptr::null_mut(), CString::into_raw)
}

fn pack_ptr(pack: Option<&OpenPackage>) -> *mut c_void {
    pack.map_or(std::ptr::null_mut(), |p| {
        p as *const OpenPackage as *mut c_void
    })
}

// ------------------------------------------------------------------ signing

fn sign(
    kind: SignKind,
    args: &Args,
    package: &Package,
    io: &mut Io,
    status: &mut Status,
) -> Result<(), PipeError> {
    let (_, pack) = package.get()?;
    let signers = signers(args, pack, io, status)?;
    for (_, entry) in &signers {
        status.push(format!("KEY_CONSIDERED {} 0", entry.fpr_hex()));
    }
    let blocks: Vec<&str> = signers.iter().map(|(b, _)| b.as_str()).collect();
    let (mode, sig_type, class) = match kind {
        SignKind::Inline => (GfrSignMode::Inline, 'S', "00"),
        SignKind::Detached => (GfrSignMode::Detached, 'D', "00"),
        SignKind::Clear => (GfrSignMode::ClearText, 'C', "01"),
    };
    let armor = args.armor || kind == SignKind::Clear;
    let ext = if kind == SignKind::Clear {
        ".asc"
    } else {
        extension(args, kind == SignKind::Detached)
    };

    let what = label(args, 0);
    let data = input(args, 0, Op::Sign, io)?;
    let out = target(args, beside_input(args, ext));
    let result = with_output(args, io, out, Op::Sign, &what, |mut sink| {
        crate::crypto::sign_stream_internal(
            CHANNEL,
            "",
            data,
            sink.writer(),
            &blocks,
            Some(pipe_password),
            mode,
            armor,
        )
        .map_err(|e| {
            if e == GfrStatus::ErrorBadPassphrase {
                status.push(format!("BAD_PASSPHRASE {}", signers[0].1.key_id_hex()));
            }
            cannot_sign(&signers[0].1, e)
        })
    })?;

    for sig in &result.signatures {
        let hash = hash_algo_id(&sig.hash_algo);
        status.push(format!("BEGIN_SIGNING H{hash}"));
        status.push(format!(
            "SIG_CREATED {sig_type} {} {hash} {class} {} {}",
            pk_algo_id(&sig.pub_algo),
            sig.created_at,
            sig.fpr
        ));
    }
    Ok(())
}

// ------------------------------------------------------------- verification

/// How a signature reads, worst last.
#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord)]
enum Verdict {
    Good,
    /// Not good: gpg's exit code 1.
    Bad,
    /// Not checked, its key is missing: gpg's exit code 2.
    NoKey,
}

/// One signature, as gpg reports it: its status lines and the lines for a
/// person.
fn report(
    sig: &SignatureResultInternal,
    pack: Option<&OpenPackage>,
    status: &mut Status,
    stderr: &mut dyn Write,
) -> Verdict {
    let found = pack.and_then(|p| p.find(&sig.fpr).ok().flatten());
    let issuer = sig.fpr.to_uppercase();
    let (key_id, primary, user_ids, own, expires) = match &found {
        Some(f) => {
            let id = hex::decode(&sig.fpr).unwrap_or_default();
            (
                f.entry.component_key_id_hex(&id),
                f.entry.fpr_hex(),
                f.entry.user_ids.clone(),
                f.component.has_secret,
                f.component.expires_at,
            )
        }
        None => {
            let key_id = match issuer.len() {
                40 => issuer[24..].to_string(),
                64 => issuer[..16].to_string(),
                _ => issuer.clone(),
            };
            (key_id, issuer.clone(), Vec::new(), false, 0)
        }
    };
    let uid = user_ids.first().cloned().unwrap_or_default();
    let pk = pk_algo_id(&sig.pub_algo);
    let hash = hash_algo_id(&sig.hash_algo);
    let class = if sig.sig_type == GfrSignMode::ClearText {
        "01"
    } else {
        "00"
    };

    status.push("NEWSIG");
    say::line(
        stderr,
        format!("Signature made {}", say::local_time(sig.created_at)),
    );
    say::line(
        stderr,
        format!(
            "               using {} key {issuer}",
            say::pk_algo_name(pk)
        ),
    );
    if found.is_some() {
        status.push(format!("KEY_CONSIDERED {primary} 0"));
    }

    let (revoked, expired) = match sig.status {
        GfrSignatureStatus::KeyRevoked | GfrSignatureStatus::KeyRevokedSince => (true, false),
        GfrSignatureStatus::KeyExpiredSince => (false, true),
        _ => (false, false),
    };
    let word = say::validity_word(revoked, expired, own);
    let from = |status_word: &str, stderr: &mut dyn Write| {
        say::line(stderr, format!("{status_word} from \"{uid}\" {word}"));
        for other in user_ids.iter().skip(1) {
            say::line(stderr, format!("                aka \"{other}\" {word}"));
        }
    };
    let validsig = |status: &mut Status| {
        let version = if sig.fpr.len() == 64 { 6 } else { 4 };
        status.push(format!(
            "VALIDSIG {issuer} {} {} {} {version} 0 {pk} {hash} {class} {primary}",
            date_of(sig.created_at),
            sig.created_at,
            sig.expires_at
        ));
        // A key in the package is trusted; one whose secret it holds is the
        // user's own.
        let level = if own { "ULTIMATE" } else { "FULLY" };
        status.push(format!("TRUST_{level} 0 pgp"));
    };

    match sig.status {
        GfrSignatureStatus::Valid => {
            status.push(format!("GOODSIG {key_id} {uid}"));
            from("Good signature", stderr);
            validsig(status);
            Verdict::Good
        }
        GfrSignatureStatus::KeyExpiredSince => {
            status.push(format!("EXPKEYSIG {key_id} {uid}"));
            from("Good signature", stderr);
            validsig(status);
            if expires != 0 {
                status.push(format!("KEYEXPIRED {expires}"));
            }
            say::line(stderr, "Note: This key has expired!");
            Verdict::Good
        }
        GfrSignatureStatus::KeyRevoked | GfrSignatureStatus::KeyRevokedSince => {
            status.push(format!("REVKEYSIG {key_id} {uid}"));
            from("Good signature", stderr);
            validsig(status);
            say::line(stderr, "WARNING: This key has been revoked by its owner!");
            say::line(
                stderr,
                "         This could mean that the signature is forged.",
            );
            // Stricter than gpg: a revoked key's signature does not count.
            Verdict::Bad
        }
        GfrSignatureStatus::SignatureExpired => {
            status.push(format!("EXPSIG {key_id} {uid}"));
            from("Expired signature", stderr);
            validsig(status);
            say::line(
                stderr,
                format!("Signature expired {}", say::local_time(sig.expires_at)),
            );
            // Stricter than gpg: an expired signature does not count.
            Verdict::Bad
        }
        GfrSignatureStatus::NoKey => {
            let fpr = if matches!(issuer.len(), 40 | 64) {
                issuer.clone()
            } else {
                "-".into()
            };
            status.push(format!(
                "ERRSIG {key_id} {pk} {hash} {class} {} 9 {fpr}",
                sig.created_at
            ));
            status.push(format!("NO_PUBKEY {key_id}"));
            say::line(stderr, "Can't check signature: No public key");
            Verdict::NoKey
        }
        other => {
            status.push(format!("BADSIG {key_id} {uid}"));
            from("BAD signature", stderr);
            let why = match other {
                GfrSignatureStatus::KeyExpired => {
                    Some("the signature was made after the key expired")
                }
                GfrSignatureStatus::KeyNotAuthorized => Some("the key may not make signatures"),
                GfrSignatureStatus::WeakHash => {
                    Some("the signature uses a hash too weak to rely on")
                }
                _ => None,
            };
            if let Some(why) = why {
                say::line(stderr, format!("Note: {why}"));
            }
            Verdict::Bad
        }
    }
}

/// Report every signature; the worst verdict, as an outcome with gpg's exit
/// code.
fn report_all(
    signatures: &[SignatureResultInternal],
    pack: Option<&OpenPackage>,
    status: &mut Status,
    io: &mut Io,
) -> Result<(), PipeError> {
    let worst = signatures
        .iter()
        .map(|sig| report(sig, pack, status, &mut *io.stderr))
        .max()
        .unwrap_or(Verdict::Good);
    match worst {
        Verdict::Good => Ok(()),
        Verdict::Bad => Err(PipeError::bad_signature("")),
        Verdict::NoKey => Err(PipeError::error("")),
    }
}

/// Writes text with its CRLF line endings as LF, as gpg gives back the text
/// of a clear-text signed message. A CR at the end of one write waits for the
/// next; [`finish`](Self::finish) writes one still waiting.
struct CrlfToLf<W: Write> {
    inner: W,
    cr: bool,
}

impl<W: Write> CrlfToLf<W> {
    fn new(inner: W) -> Self {
        Self { inner, cr: false }
    }

    fn finish(mut self) -> std::io::Result<()> {
        if self.cr {
            self.inner.write_all(b"\r")?;
        }
        self.inner.flush()
    }
}

impl<W: Write> Write for CrlfToLf<W> {
    fn write(&mut self, buf: &[u8]) -> std::io::Result<usize> {
        let mut out = Vec::with_capacity(buf.len() + 1);
        for &b in buf {
            if self.cr && b != b'\n' {
                out.push(b'\r');
            }
            self.cr = b == b'\r';
            if !self.cr {
                out.push(b);
            }
        }
        self.inner.write_all(&out)?;
        Ok(buf.len())
    }

    fn flush(&mut self) -> std::io::Result<()> {
        self.inner.flush()
    }
}

/// The file a detached signature `sig` is for, as gpg finds it: the same
/// name without `.sig`, `.asc` or `.sign`.
fn signed_file_of(sig: &OsStr) -> Option<OsString> {
    let text = sig.to_str()?;
    [".sig", ".asc", ".sign"]
        .iter()
        .find_map(|ext| text.strip_suffix(ext))
        .filter(|data| !data.is_empty() && Path::new(data).is_file())
        .map(OsString::from)
}

/// A writer that counts what passes through it, for PLAINTEXT_LENGTH.
struct Counting<'a> {
    inner: &'a mut (dyn Write + Send + Sync),
    written: u64,
}

impl Write for Counting<'_> {
    fn write(&mut self, buf: &[u8]) -> std::io::Result<usize> {
        let n = self.inner.write(buf)?;
        self.written += n as u64;
        Ok(n)
    }

    fn flush(&mut self) -> std::io::Result<()> {
        self.inner.flush()
    }
}

/// gpg's lines for the literal data of a message: its name is not known
/// here, its length is what was written.
fn plaintext(status: &mut Status, filename: &str, length: u64) {
    let name = filename.replace('%', "%25").replace(' ', "%20");
    status.push(format!("PLAINTEXT 62 0 {name}").trim_end());
    status.push(format!("PLAINTEXT_LENGTH {length}"));
}

/// The content of a signed message, verified as a stream into `out`: clear
/// text with gpg's line ends, or the literal data of an inline-signed one.
/// Nothing reaches `out` before the whole message was read.
fn verify_message(
    file: File,
    len: u64,
    clear_signed: bool,
    out: &mut (dyn Write + Send + Sync),
    pack: Option<&OpenPackage>,
) -> Result<Vec<SignatureResultInternal>, GfrStatus> {
    if clear_signed {
        // The signed text is canonical (CRLF); gpg gives it back as written.
        let mut text = CrlfToLf::new(out);
        let result = crate::crypto::verify_cleartext_stream_internal(
            CHANNEL,
            file,
            &mut text,
            None,
            Some(fetch_public),
            pack_ptr(pack),
        )?;
        text.finish().map_err(|_| GfrStatus::ErrorInternal)?;
        Ok(result.signatures)
    } else {
        crate::crypto::verify_inline_stream_staged_internal(
            CHANNEL,
            file,
            len,
            out,
            Some(fetch_public),
            pack_ptr(pack),
        )
        .map(|r| r.signatures)
    }
}

fn verify(
    args: &Args,
    package: &Package,
    io: &mut Io,
    status: &mut Status,
) -> Result<(), PipeError> {
    let (_, pack) = package.get()?;
    // `verify SIG DATA` checks a detached signature, streaming the data;
    // `verify FILE` a signed message, inline or clear-text, or a detached
    // signature whose data is the file of the same name without its
    // extension.
    let signatures = if args.inputs.len() >= 2 {
        let sig = read_signature(input(args, 0, Op::Verify, io)?)?;
        let (data, _) = input_file(args, 1, Op::Verify, io)?;
        verify_detached(data, &sig, pack, status)?
    } else {
        let (mut file, len) = input_file(args, 0, Op::Verify, io)?;
        let head = head_of(&mut file).map_err(|e| Op::Verify.io_error("", &e))?;
        match head.kind {
            Kind::Empty | Kind::NotOpenPgp => {
                let error = no_openpgp_data(&head, Op::Verify, io, status);
                if head.kind == Kind::Empty {
                    return Err(error);
                }
                say::line(&mut *io.stderr, "the signature could not be verified.");
                let _ = writeln!(
                    io.stderr,
                    "Please remember that the signature file (.sig or .asc)\n\
                     should be the first file given on the command line."
                );
                return Err(PipeError::error(""));
            }
            Kind::Invalid => {
                invalid_packet(&head, io, status);
                status.push("NODATA 4");
                say::line(&mut *io.stderr, "no signature found");
                say::line(&mut *io.stderr, "the signature could not be verified.");
                let _ = writeln!(
                    io.stderr,
                    "Please remember that the signature file (.sig or .asc)\n\
                     should be the first file given on the command line."
                );
                return Err(PipeError::error(""));
            }
            Kind::Signature => {
                let sig = read_signature(&mut file)?;
                let named = args.inputs.first().filter(|f| *f != "-");
                let Some(data_file) = named.and_then(|f| signed_file_of(f)) else {
                    say::line(&mut *io.stderr, "no signed data");
                    return Err(PipeError::error("can't hash datafile: No data"));
                };
                let what = data_file.to_string_lossy().into_owned();
                let data = open_named(&data_file, Op::Verify, &what, io)?;
                verify_detached(data, &sig, pack, status)?
            }
            _ => {
                let (file, len) = from_armor(file, len, &head, Op::Verify)?;
                let clear = head.kind == Kind::ClearSigned;
                let unreadable = |e: GfrStatus, status: &mut Status| {
                    status.push("NODATA 4");
                    Op::Verify.error("", &engine_error(e), code::gpg(code::BAD_DATA))
                };
                // With -o the signed content is written out, as gpg does.
                if args.output.is_some() {
                    let out = target(args, None);
                    with_output(args, io, out, Op::Verify, "", |mut sink| {
                        verify_message(file, len, clear, sink.writer(), Some(pack))
                            .map_err(|e| unreadable(e, status))
                    })?
                } else {
                    verify_message(file, len, clear, &mut std::io::sink(), Some(pack))
                        .map_err(|e| unreadable(e, status))?
                }
            }
        }
    };

    if signatures.is_empty() {
        status.push("NODATA 3");
        return Err(PipeError::error("no signature found"));
    }
    report_all(&signatures, Some(pack), status, io)
}

/// A detached signature over `data`, read as a stream.
fn verify_detached(
    data: File,
    sig: &[u8],
    pack: &OpenPackage,
    status: &mut Status,
) -> Result<Vec<SignatureResultInternal>, PipeError> {
    crate::crypto::verify_detached_stream_internal(
        CHANNEL,
        data,
        sig,
        Some(fetch_public),
        pack_ptr(Some(pack)),
    )
    .map(|result| result.signatures)
    .map_err(|e| {
        status.push("NODATA 4");
        Op::Verify.error("", &engine_error(e), code::gpg(code::BAD_DATA))
    })
}

// --------------------------------------------------------------- encryption

/// The recipients: those named, else `--default-recipient`, plus the default
/// key (or the package's only secret key) with `--default-recipient-self`.
fn recipient_queries(args: &Args, pack: &OpenPackage) -> Result<Vec<Recipient>, PipeError> {
    let mut queries = args.recipients.clone();
    if queries.is_empty() {
        queries.extend(args.default_recipient.clone().map(|name| Recipient {
            name,
            hidden: false,
        }));
    }
    if args.default_recipient_self {
        let name = match &args.default_key {
            Some(key) => key.clone(),
            None => only_secret_key(pack)
                .map_err(|_| {
                    PipeError::error(
                        "the key package holds several secret keys; name the recipient with -r",
                    )
                })?
                .ok_or_else(|| PipeError::error("the key package holds no secret key"))?
                .fpr_hex(),
        };
        queries.push(Recipient {
            name,
            hidden: false,
        });
    }
    Ok(queries)
}

fn encrypt(
    with_signature: bool,
    args: &Args,
    package: &Package,
    io: &mut Io,
    status: &mut Status,
) -> Result<(), PipeError> {
    let (_, pack) = package.get()?;
    let what = label(args, 0);
    let queries = recipient_queries(args, pack)?;
    if queries.is_empty() {
        say::line(&mut *io.stderr, "no valid addressees");
        status.push("NO_RECP 0");
        return Err(Op::Encrypt.error(&what, "No user ID", code::NO_USER_ID));
    }

    // Every recipient is checked before anything is written: one that cannot
    // receive the message fails it, as with gpg, never leaving someone out.
    let mut chosen = Vec::new();
    for query in &queries {
        let name = &query.name;
        let (reason, code) = match pick(pack, name, CAP_ENCRYPT, false)? {
            Ok(found) => {
                status.push(format!("KEY_CONSIDERED {} 0", found.entry.fpr_hex()));
                chosen.push((found, query.hidden || args.throw_keyids));
                continue;
            }
            Err(Refusal::Missing) => ("1", code::NO_PUBKEY),
            Err(Refusal::Unusable(found)) => {
                for f in &found {
                    status.push(format!("KEY_CONSIDERED {} 3", f.entry.fpr_hex()));
                }
                ("0", code::UNUSABLE_PUBKEY)
            }
            Err(Refusal::Ambiguous) => ("2", code::AMBIGUOUS_NAME),
        };
        say::line(
            &mut *io.stderr,
            format!("{name}: skipped: {}", code::reason(code)),
        );
        status.push(format!("INV_RECP {reason} {name}"));
        return Err(Op::Encrypt.error(&what, code::reason(code), code));
    }

    let mut recipients = Vec::new();
    let mut hidden = Vec::new();
    for (found, hide) in &chosen {
        let public = armored_public(&found.entry.public_key).ok_or_else(damaged)?;
        recipients.push(pinned(&public, found.exact.as_deref()));
        hidden.push(*hide);
    }
    let public: Vec<&str> = recipients.iter().map(|b| b.as_str()).collect();

    let signers = if with_signature {
        signers(args, pack, io, status)?
    } else {
        Vec::new()
    };
    let secret: Vec<&str> = signers.iter().map(|(b, _)| b.as_str()).collect();

    let data = input(args, 0, Op::Encrypt, io)?;
    let out = target(args, beside_input(args, extension(args, false)));
    status.push(match crate::crypto::planned_container(&public) {
        Some((mdc, cipher, 0)) => format!("BEGIN_ENCRYPTION {mdc} {cipher}"),
        Some((mdc, cipher, aead)) => format!("BEGIN_ENCRYPTION {mdc} {cipher} {aead}"),
        None => "BEGIN_ENCRYPTION 2 9".into(),
    });
    let result = with_output(args, io, out, Op::Encrypt, &what, |mut sink| {
        let result = crate::crypto::encrypt_and_sign_stream_hiding_internal(
            CHANNEL,
            "",
            data,
            sink.writer(),
            &public,
            &hidden,
            &secret,
            Some(pipe_password),
            args.armor,
        )
        .map_err(|e| match signers.first() {
            Some((_, entry)) if e == GfrStatus::ErrorUnusableKey => cannot_sign(entry, e),
            _ => Op::Encrypt.error(&what, &engine_error(e), code::gpg(code::GENERAL)),
        })?;
        // A recipient the engine refused fails the run; a written file is
        // removed with it.
        if let Some(rejected) = result.invalid_recipients.first() {
            status.push(format!("INV_RECP 0 {}", rejected.fpr));
            return Err(Op::Encrypt.error(
                &what,
                code::reason(code::UNUSABLE_PUBKEY),
                code::UNUSABLE_PUBKEY,
            ));
        }
        Ok(result)
    })?;
    for sig in &result.signatures {
        let hash = hash_algo_id(&sig.hash_algo);
        status.push(format!(
            "SIG_CREATED S {} {hash} 00 {} {}",
            pk_algo_id(&sig.pub_algo),
            sig.created_at,
            sig.fpr
        ));
    }
    status.push("END_ENCRYPTION");
    Ok(())
}

/// `-c`: encryption with the passphrase given on the command line, in the
/// form gpg 2.2 and 2.4 write and read: an iterated and salted S2K (RFC 9580
/// §3.7.1.3) and a SEIPD v1 container, where the engine's own passphrase
/// encryption uses Argon2 and SEIPD v2 that only RFC 9580 readers open.
fn symmetric(args: &Args, io: &mut Io, status: &mut Status) -> Result<(), PipeError> {
    use pgp::composed::MessageBuilder;

    let Some(passphrase) = PASSPHRASE.with(|p| p.borrow().clone()) else {
        return Err(PipeError::error(
            "encrypting with a passphrase needs it: give --passphrase-file, --passphrase-fd or \
             --passphrase",
        ));
    };
    let what = label(args, 0);
    let mut rng = rand::thread_rng();
    let s2k = StringToKey::new_default(&mut rng);
    status.push(format!(
        "NEED_PASSPHRASE_SYM {} {} {}",
        u8::from(SymmetricKeyAlgorithm::AES256),
        s2k.id(),
        s2k_hash_id(&s2k)
    ));
    // As gpg: an empty passphrase protects nothing and is refused.
    if passphrase.is_empty() {
        status.push("MISSING_PASSPHRASE");
        say::line(
            &mut *io.stderr,
            "error creating passphrase: Invalid passphrase",
        );
        return Err(Op::Symmetric.error(
            &what,
            "Invalid passphrase",
            code::gpg(code::INV_PASSPHRASE),
        ));
    }
    let failed = |e: pgp::errors::Error| {
        Op::Symmetric.error(&what, &e.to_string(), code::gpg(code::GENERAL))
    };

    let data = input(args, 0, Op::Symmetric, io)?;
    let out = target(args, beside_input(args, extension(args, false)));
    status.push(format!(
        "BEGIN_ENCRYPTION 2 {}",
        u8::from(SymmetricKeyAlgorithm::AES256)
    ));
    with_output(args, io, out, Op::Symmetric, &what, |mut sink| {
        let out = sink.writer();
        let mut builder = MessageBuilder::from_reader("", data);
        builder.partial_chunk_size(512 * 1024).map_err(failed)?;
        let mut sealed = builder.seipd_v1(&mut rng, SymmetricKeyAlgorithm::AES256);
        let password = crate::utils::password_from_zeroizing_bytes(passphrase);
        sealed
            .encrypt_with_password(s2k, &password)
            .map_err(failed)?;
        if args.armor {
            sealed
                .to_armored_writer(&mut rng, crate::utils::armor_opts_for(true), out)
                .map_err(failed)
        } else {
            sealed.to_writer(&mut rng, out).map_err(failed)
        }
    })?;
    status.push("END_ENCRYPTION");
    Ok(())
}

// --------------------------------------------------------------- decryption

fn decrypt(
    args: &Args,
    package: &Package,
    io: &mut Io,
    status: &mut Status,
) -> Result<(), PipeError> {
    let (mut file, len) = input_file(args, 0, Op::Decrypt, io)?;
    let head = head_of(&mut file).map_err(|e| Op::Decrypt.io_error("", &e))?;
    let out = target(args, None);
    open_message(args, package, io, status, file, len, head, out, Op::Decrypt)
}

/// A message, whatever it is, as `--decrypt` opens it: decrypted, or
/// verified with its content put out, into `out`.
#[allow(clippy::too_many_arguments)]
fn open_message(
    args: &Args,
    package: &Package,
    io: &mut Io,
    status: &mut Status,
    file: File,
    len: u64,
    head: Head,
    out: Target,
    op: Op,
) -> Result<(), PipeError> {
    match head.kind {
        Kind::Empty | Kind::NotOpenPgp => return Err(no_openpgp_data(&head, op, io, status)),
        Kind::Invalid => {
            invalid_packet(&head, io, status);
            return Err(PipeError::error(""));
        }
        Kind::Keys => {
            return Err(PipeError::error(
                "this is a key, not a message; gfpipe does no key management",
            ));
        }
        Kind::Signature => {
            say::line(&mut *io.stderr, "no signed data");
            return Err(PipeError::error("can't hash datafile: No data"));
        }
        Kind::Signed | Kind::ClearSigned | Kind::Encrypted => {}
    }
    let (file, len) = from_armor(file, len, &head, op)?;

    // gpg --decrypt of a message that is only signed puts out its content and
    // checks the signature. It streams too, and reaches the output only once
    // the whole message has been read.
    if head.kind != Kind::Encrypted {
        let (_, pack) = package.get()?;
        let clear = head.kind == Kind::ClearSigned;
        let (signatures, length) = with_output(args, io, out, op, "", |mut sink| {
            let mut counted = Counting {
                inner: sink.writer(),
                written: 0,
            };
            let signatures =
                verify_message(file, len, clear, &mut counted, Some(pack)).map_err(|e| {
                    status.push("NODATA 2");
                    op.error("", &engine_error(e), code::gpg(code::BAD_DATA))
                })?;
            Ok((signatures, counted.written))
        })?;
        plaintext(status, "", length);
        return report_all(&signatures, Some(pack), status, io);
    }

    let recipients = crate::crypto::sniff_recipients(&head.packets);
    // The package holds the secret keys a message to keys needs; one sealed
    // with a passphrase only needs none.
    let pack = if recipients.is_empty() || has_passphrase() || args.list_only {
        package.try_get()
    } else {
        Some(package.get()?.1)
    };

    for r in &recipients {
        status.push(format!(
            "ENC_TO {} {} 0",
            r.key_id.to_uppercase(),
            pk_algo_id(&r.pub_algo)
        ));
    }
    let mut holds_secret = false;
    for r in &recipients {
        let key_id = r.key_id.to_uppercase();
        if args.verbose {
            say::line(&mut *io.stderr, format!("public key is {key_id}"));
        }
        let found = pack.and_then(|p| p.find(&key_id).ok().flatten());
        match &found {
            Some(f) => {
                holds_secret |= f.component.has_secret;
                status.push(format!("KEY_CONSIDERED {} 0", f.entry.fpr_hex()));
                if !args.quiet {
                    let fpr = f.exact.clone().unwrap_or_else(|| f.entry.fpr_hex());
                    let shape = shape_of(&f.entry, &f.component, &fpr);
                    say::line(
                        &mut *io.stderr,
                        format!(
                            "encrypted with {} key, ID {key_id}, created {}",
                            shape.label,
                            date_of(f.component.created_at)
                        ),
                    );
                    let _ = writeln!(io.stderr, "      \"{}\"", f.entry.primary_user_id());
                }
            }
            None if !args.quiet => say::line(
                &mut *io.stderr,
                format!(
                    "encrypted with {} key, ID {key_id}",
                    say::pk_algo_name(pk_algo_id(&r.pub_algo))
                ),
            ),
            None => {}
        }
    }
    // Passphrase packets: what gpg asks a passphrase for, and says.
    let skesks = passphrase_packets(&head.packets);
    for skesk in &skesks {
        status.push(format!("NEED_PASSPHRASE_SYM {}", skesk.status));
        if !args.quiet {
            say::line(&mut *io.stderr, format!("{} encrypted data", skesk.cipher));
        }
        if PASSPHRASE.with(|p| p.borrow().as_ref().is_some_and(|p| p.is_empty())) {
            status.push("MISSING_PASSPHRASE");
        }
    }
    if !skesks.is_empty() && !args.quiet {
        let n = skesks.len();
        say::line(
            &mut *io.stderr,
            format!(
                "encrypted with {n} passphrase{}",
                if n == 1 { "" } else { "s" }
            ),
        );
    }
    if args.list_only {
        return Ok(());
    }

    // No secret key for any recipient: gpg's lines, without trying.
    let anonymous = recipients
        .iter()
        .any(|r| r.key_id.chars().all(|c| c == '0'));
    if !recipients.is_empty() && !holds_secret && !anonymous && !has_passphrase() {
        for r in &recipients {
            status.push(format!("NO_SECKEY {}", r.key_id.to_uppercase()));
        }
        say::line(
            &mut *io.stderr,
            "public key decryption failed: No secret key",
        );
        status.push(format!(
            "ERROR pkdecrypt_failed {}",
            code::gpg(code::NO_SECKEY)
        ));
        status.push("BEGIN_DECRYPTION");
        status.push("DECRYPTION_FAILED");
        say::line(&mut *io.stderr, "decryption failed: No secret key");
        status.push("END_DECRYPTION");
        return Err(PipeError::error(""));
    }

    status.push("BEGIN_DECRYPTION");
    // gpg names the container of a passphrase message once it derived the
    // key, before the data shows whether the passphrase was right.
    let early_info = if recipients.is_empty() {
        skesks.first().and_then(|p| decryption_info(&p.cipher))
    } else {
        None
    };
    if let Some(info) = &early_info {
        status.push(format!("DECRYPTION_INFO {info}"));
    }
    let failed = |e: GfrStatus, status: &mut Status| {
        // The engine's detail says when data that failed its check may only
        // have been opened with a wrong password (a v4 passphrase packet
        // cannot confirm one).
        let detail = super::engine_detail();
        let wrong_password = recipients.is_empty()
            && has_passphrase()
            && (matches!(
                e,
                GfrStatus::ErrorBadPassphrase
                    | GfrStatus::ErrorWrongPassword
                    | GfrStatus::ErrorFetchPasswordFailed
            ) || (e == GfrStatus::ErrorDataCorrupted && detail.contains("wrong password")));
        let reason = match e {
            _ if wrong_password => {
                status.push("ERROR symkey_decrypt.maybe_error 11_BAD_PASSPHRASE");
                "Bad session key".to_string()
            }
            GfrStatus::ErrorNoSecretKey | GfrStatus::ErrorNoKey => {
                for r in &recipients {
                    status.push(format!("NO_SECKEY {}", r.key_id.to_uppercase()));
                }
                "No secret key".to_string()
            }
            GfrStatus::ErrorBadPassphrase | GfrStatus::ErrorWrongPassword => {
                if let Some(r) = recipients.first() {
                    status.push(format!("BAD_PASSPHRASE {}", r.key_id.to_uppercase()));
                }
                "Bad passphrase".to_string()
            }
            GfrStatus::ErrorFetchPasswordFailed => "No passphrase given".to_string(),
            _ if !detail.is_empty() => detail,
            other => format!("{other:?}"),
        };
        status.push("DECRYPTION_FAILED");
        status.push("END_DECRYPTION");
        if recipients.is_empty() && !has_passphrase() {
            PipeError::error(
                "the message is encrypted with a passphrase; give it with --passphrase-file, \
                 --passphrase-fd or --passphrase",
            )
        } else {
            PipeError::error(format!("decryption failed: {reason}"))
        }
    };
    let armored = head.armored;
    let (result, length) = with_output(args, io, out, op, "", |sink| match sink {
        // The output is a stage already: the plaintext goes there directly.
        Sink::Stage(stage) => {
            let result = crate::crypto::decrypt_and_verify_stream_into_stage_internal(
                CHANNEL,
                file,
                len,
                stage,
                armored,
                Some(fetch_secret),
                Some(pipe_password),
                Some(fetch_public),
                pack_ptr(pack),
            )
            .map_err(|e| failed(e, status))?;
            let length = stage.stream_position().unwrap_or(0);
            Ok((result, length))
        }
        Sink::Stream(out) => {
            let mut counted = Counting {
                inner: out,
                written: 0,
            };
            let result = crate::crypto::decrypt_and_verify_stream_staged_internal(
                CHANNEL,
                file,
                len,
                &mut counted,
                armored,
                Some(fetch_secret),
                Some(pipe_password),
                Some(fetch_public),
                pack_ptr(pack),
            )
            .map_err(|e| failed(e, status))?;
            Ok((result, counted.written))
        }
    })?;

    let used = result
        .recipients
        .iter()
        .find(|r| r.status == GfrRecipientStatus::Success)
        .and_then(|r| pack?.find(&r.key_id).ok().flatten());
    if let Some(f) = used {
        let sub = f.exact.clone().unwrap_or_else(|| f.entry.fpr_hex());
        let trust = if f.component.has_secret { 'u' } else { 'f' };
        status.push(format!(
            "DECRYPTION_KEY {sub} {} {trust}",
            f.entry.fpr_hex()
        ));
    }
    if early_info.is_none() {
        if let Some(info) = decryption_info(&result.sym_algo) {
            status.push(format!("DECRYPTION_INFO {info}"));
        }
    }
    plaintext(status, &result.filename, length);
    let verdict = report_all(&result.signatures, pack, status, io);
    status.push("DECRYPTION_OKAY");
    status.push("GOODMDC");
    status.push("END_DECRYPTION");
    verdict
}

// ------------------------------------------------------- the default action

/// gpg without a command: each file (or standard input) is looked at and
/// opened as what it is. A message is decrypted or verified into the file of
/// its name without `.gpg`, `.pgp` or `.asc`, a detached signature checked
/// against its data, keys shown.
fn default_action(
    args: &Args,
    package: &Package,
    io: &mut Io,
    status: &mut Status,
) -> Result<(), PipeError> {
    let count = args.inputs.len().max(1);
    for index in 0..count {
        let (mut file, len) = input_file(args, index, Op::Default, io)?;
        let head = head_of(&mut file).map_err(|e| Op::Default.io_error("", &e))?;
        let named = args.inputs.get(index).filter(|f| *f != "-");
        match head.kind {
            Kind::Keys => show_keys(file, &head, io)?,
            Kind::Signature => {
                let sig = read_signature(&mut file)?;
                let Some(data_file) = named.and_then(|f| signed_file_of(f)) else {
                    say::line(&mut *io.stderr, "no signed data");
                    return Err(PipeError::error("can't hash datafile: No data"));
                };
                let (_, pack) = package.get()?;
                let what = data_file.to_string_lossy().into_owned();
                let data = open_named(&data_file, Op::Verify, &what, io)?;
                let signatures = verify_detached(data, &sig, pack, status)?;
                report_all(&signatures, Some(pack), status, io)?;
            }
            _ => {
                let stripped = named.and_then(|f| {
                    let text = f.to_str()?;
                    [".gpg", ".pgp", ".asc"]
                        .iter()
                        .find_map(|ext| text.strip_suffix(ext))
                        .filter(|s| !s.is_empty())
                        .map(PathBuf::from)
                });
                let out = target(args, stripped);
                open_message(args, package, io, status, file, len, head, out, Op::Default)?;
            }
        }
    }
    Ok(())
}

/// Keys in a file, shown as gpg shows them without importing them.
fn show_keys(mut file: File, head: &Head, io: &mut Io) -> Result<(), PipeError> {
    let mut bytes = Vec::new();
    Read::by_ref(&mut file)
        .take(MAX_KEYS_FILE)
        .read_to_end(&mut bytes)
        .map_err(|e| Op::Default.io_error("", &e))?;
    let start = usize::try_from(head.armor_at).unwrap_or(0).min(bytes.len());
    let binary = crate::crypto::dearmor_all(&bytes[start..]).into_owned();
    let mut certs: Vec<(SignedPublicKey, bool)> = Vec::new();
    if let Ok(secret) = SignedSecretKey::from_bytes_many(&binary[..]) {
        certs.extend(
            secret
                .filter_map(Result::ok)
                .map(|k| (SignedPublicKey::from(k), true)),
        );
    }
    if certs.is_empty() {
        if let Ok(public) = SignedPublicKey::from_bytes_many(&binary[..]) {
            certs.extend(public.filter_map(Result::ok).map(|k| (k, false)));
        }
    }
    if certs.is_empty() {
        say::line(&mut *io.stderr, "no valid OpenPGP data found.");
        return Err(Op::Default.error("", "Unknown system error", code::UNKNOWN));
    }

    let mut text = String::new();
    for (cert, secret) in &certs {
        let m = crate::key::build_public_metadata(cert);
        let pk = u8::from(cert.primary_key.algorithm());
        let caps = |s: bool, c: bool, e: bool, a: bool| {
            [(s, 'S'), (c, 'C'), (e, 'E'), (a, 'A')]
                .iter()
                .filter(|(on, _)| *on)
                .map(|(_, l)| *l)
                .collect::<String>()
        };
        let until = |revoked: bool, expires: u32| {
            if revoked {
                " [revoked]".to_string()
            } else if expires != 0 {
                let word = if u64::from(expires) <= now() {
                    "expired"
                } else {
                    "expires"
                };
                format!(" [{word}: {}]", date_of(expires))
            } else {
                String::new()
            }
        };
        text.push_str(&format!(
            "{}   {} {} [{}]{}\n      {}\n",
            if *secret { "sec" } else { "pub" },
            algo_label(m.algo, m.key_length, pk),
            date_of(m.created_at),
            caps(m.can_sign, m.can_certify, m.can_encrypt, m.can_auth),
            until(m.is_revoked, m.expires_at),
            m.fpr.to_uppercase()
        ));
        for uid in &m.user_ids {
            text.push_str(&format!("uid           {}\n", uid.user_id));
        }
        for sub in &m.subkeys {
            let sub_pk = cert
                .public_subkeys
                .iter()
                .find(|k| {
                    hex::encode(k.key.fingerprint().as_bytes()).eq_ignore_ascii_case(&sub.fpr)
                })
                .map_or(0, |k| u8::from(k.key.algorithm()));
            text.push_str(&format!(
                "{}   {} {} [{}]{}\n",
                if *secret { "ssb" } else { "sub" },
                algo_label(sub.algo, sub.key_length, sub_pk),
                date_of(sub.created_at),
                caps(sub.can_sign, sub.can_certify, sub.can_encrypt, sub.can_auth),
                until(sub.is_revoked, sub.expires_at)
            ));
        }
        text.push('\n');
    }
    write_out(io, text.as_bytes())
}

/// Bytes to standard output; a write that fails fails the run.
fn write_out(io: &mut Io, bytes: &[u8]) -> Result<(), PipeError> {
    io.stdout.write_all(bytes).map_err(|e| {
        PipeError::error(format!("[stdout]: write error: {}", say::io_reason(&e)))
            .failure("write", code::system(&e))
    })
}

// ------------------------------------------------------------------ listing

/// gpg's colon fields of a user ID: when its newest self-signature was made,
/// and the RIPEMD-160 of the user ID, upper-case hex.
fn uid_fields(cert: &SignedPublicKey, uid: &str) -> (String, String) {
    let created = cert
        .details
        .users
        .iter()
        .filter(|u| u.id.id() == uid.as_bytes())
        .flat_map(|u| u.signatures.iter())
        .filter_map(|sig| sig.created())
        .map(|t| t.as_secs())
        .max()
        .map(|t| t.to_string())
        .unwrap_or_default();
    let hash = pgp::crypto::hash::HashAlgorithm::Ripemd160
        .digest(uid.as_bytes())
        .map(hex::encode_upper)
        .unwrap_or_default();
    (created, hash)
}

/// A passphrase packet of a message, as gpg reports it.
struct PassphrasePacket {
    /// `<cipher> <s2k mode> <s2k hash>` of NEED_PASSPHRASE_SYM.
    status: String,
    /// "AES256.CFB", as gpg names the data it protects.
    cipher: String,
}

/// gpg's name of a cipher in "AES256.CFB encrypted data".
fn cipher_name(id: u8) -> String {
    match id {
        1 => "IDEA".into(),
        2 => "3DES".into(),
        3 => "CAST5".into(),
        4 => "BLOWFISH".into(),
        7 => "AES".into(),
        8 => "AES192".into(),
        9 => "AES256".into(),
        10 => "TWOFISH".into(),
        11 => "CAMELLIA128".into(),
        12 => "CAMELLIA192".into(),
        13 => "CAMELLIA256".into(),
        other => format!("CIPHER{other}"),
    }
}

fn s2k_hash_id(s2k: &StringToKey) -> u8 {
    match s2k {
        StringToKey::Simple { hash_alg }
        | StringToKey::Salted { hash_alg, .. }
        | StringToKey::IteratedAndSalted { hash_alg, .. } => u8::from(*hash_alg),
        _ => 0,
    }
}

/// The passphrase packets (SKESK) among the first packets of a message.
fn passphrase_packets(packets: &[u8]) -> Vec<PassphrasePacket> {
    use pgp::packet::{Packet, PacketParser};
    PacketParser::new(std::io::Cursor::new(packets))
        .map_while(Result::ok)
        .take_while(|p| {
            matches!(
                p,
                Packet::SymKeyEncryptedSessionKey(_) | Packet::PublicKeyEncryptedSessionKey(_)
            )
        })
        .filter_map(|p| match p {
            Packet::SymKeyEncryptedSessionKey(skesk) => {
                let cipher = skesk.sym_algorithm().map_or(0, u8::from);
                let (mode, hash) = skesk.s2k().map_or((0, 0), |s| (s.id(), s2k_hash_id(s)));
                // A v4 packet protects a v1 SEIPD (CFB); a v6 one an AEAD
                // container.
                let mode_name = match skesk.version() {
                    pgp::types::SkeskVersion::V4 => "CFB",
                    _ => "OCB",
                };
                Some(PassphrasePacket {
                    status: format!("{cipher} {mode} {hash}"),
                    cipher: format!("{}.{mode_name}", cipher_name(cipher)),
                })
            }
            _ => None,
        })
        .collect()
}

/// gpg's capability letters for `CAP_*` bits, in the order of its colon
/// listing (e, s, c, a); upper case for the key as a whole.
fn cap_letters(caps: u8, upper: bool) -> String {
    letters(
        caps,
        &[
            (crate::keypack::CAP_ENCRYPT, 'e'),
            (crate::keypack::CAP_SIGN, 's'),
            (crate::keypack::CAP_CERTIFY, 'c'),
            (crate::keypack::CAP_AUTH, 'a'),
        ],
        upper,
    )
}

fn letters(caps: u8, order: &[(u8, char)], upper: bool) -> String {
    order
        .iter()
        .filter(|(bit, _)| caps & bit != 0)
        .map(|(_, letter)| {
            if upper {
                letter.to_ascii_uppercase()
            } else {
                *letter
            }
        })
        .collect()
}

/// `ed25519 2024-01-01 [SC] [expires: 2027-01-01]`, as gpg's listing has it.
fn describe(c: &Component, shape: &Shape) -> String {
    // The human listing orders them S, C, E, A.
    let usage = letters(
        c.caps,
        &[
            (crate::keypack::CAP_SIGN, 'S'),
            (crate::keypack::CAP_CERTIFY, 'C'),
            (crate::keypack::CAP_ENCRYPT, 'E'),
            (crate::keypack::CAP_AUTH, 'A'),
        ],
        true,
    );
    let mut text = format!("{} {} [{usage}]", shape.label, date_of(c.created_at));
    if c.revoked {
        text.push_str(" [revoked]");
    } else if c.expires_at != 0 {
        let word = if c.expired_at(now()) {
            "expired"
        } else {
            "expires"
        };
        text.push_str(&format!(" [{word}: {}]", date_of(c.expires_at)));
    }
    text
}

/// The keys a listing or an export covers: those the patterns name, or all.
/// A pattern that names nothing is passed over, as with gpg; whether any
/// named something is for the caller.
fn selected(
    patterns: &[String],
    secret_only: bool,
    pack: &OpenPackage,
) -> Result<Vec<Component>, PipeError> {
    let mut chosen = pack.primaries().map_err(|_| damaged())?;
    if secret_only {
        chosen.retain(|c| c.has_secret);
    }
    if patterns.is_empty() {
        return Ok(chosen);
    }
    let mut named = Vec::new();
    for pattern in patterns {
        named.extend(keys_named(pack, pattern)?.into_iter().map(|f| f.component));
    }
    chosen.retain(|c| named.iter().any(|n| n.same_key(c)));
    Ok(chosen)
}

/// gpg's validity letter: revoked, expired, ultimate (the user's own key) or
/// full (every key in the package is trusted).
fn validity(c: &Component, own: bool) -> char {
    if c.revoked {
        'r'
    } else if c.expired_at(now()) {
        'e'
    } else if own {
        'u'
    } else {
        'f'
    }
}

/// gpg's escaping of a user ID in a colon listing: colons, backslashes and
/// control characters as `\xNN`; everything else, UTF-8 included, as it is.
fn colon_escape(text: &str) -> String {
    let mut out = String::with_capacity(text.len());
    for ch in text.chars() {
        if ch == ':' || ch == '\\' || (ch as u32) < 0x20 {
            out.push_str(&format!("\\x{:02x}", ch as u32));
        } else {
            out.push(ch);
        }
    }
    out
}

#[cfg(test)]
pub(super) fn colon_escape_for_tests(text: &str) -> String {
    colon_escape(text)
}

/// One key or subkey line of `--with-colons` output (gpg's DETAILS file).
fn colon_key_line(
    typ: &str,
    c: &Component,
    key_id: &str,
    shape: &Shape,
    own: bool,
    caps: String,
) -> String {
    let created = c.created_at.to_string();
    let expires = if c.expires_at == 0 {
        String::new()
    } else {
        c.expires_at.to_string()
    };
    let validity = validity(c, own).to_string();
    let ownertrust = if c.subkey() {
        ""
    } else if own {
        "u"
    } else {
        "f"
    };
    let token = if typ == "sec" || typ == "ssb" {
        "+"
    } else {
        ""
    };
    let fields: [&str; 20] = [
        typ,
        &validity,
        &shape.key_length.to_string(),
        &c.algo.to_string(),
        key_id,
        &created,
        &expires,
        "",
        ownertrust,
        "",
        "",
        &caps,
        "",
        "",
        token,
        "",
        shape.curve,
        "",
        "",
        "0",
    ];
    format!("{}:", fields.join(":"))
}

fn list(
    secret_only: bool,
    args: &Args,
    package: &Package,
    io: &mut Io,
    status: &mut Status,
) -> Result<(), PipeError> {
    let (path, pack) = package.get()?;
    let patterns = args.patterns();
    let chosen = selected(&patterns, secret_only, pack)?;
    if !patterns.is_empty() && chosen.is_empty() {
        let (reason, code) = if secret_only {
            ("No secret key", code::NO_SECKEY)
        } else {
            ("No public key", code::NO_PUBKEY)
        };
        say::line(&mut *io.stderr, format!("error reading key: {reason}"));
        status.push(format!("ERROR keylist.getkey {code}"));
        return Err(PipeError::error(""));
    }
    let components = pack.components().map_err(|_| damaged())?;
    let (key_word, sub_word) = if secret_only {
        ("sec", "ssb")
    } else {
        ("pub", "sub")
    };

    let mut out = String::new();
    if args.with_colons {
        if !secret_only {
            // gpg's trust database record; every key in a package is trusted.
            out.push_str(&format!("tru::1:{}:0:3:1:5\n", pack.header.generated_at));
        }
    } else if !chosen.is_empty() {
        let shown = path.display().to_string();
        out.push_str(&format!("{shown}\n{}\n", "-".repeat(shown.chars().count())));
    }
    // Both are in writing order: one walk along the keys finds each one's.
    let mut keys = components.chunk_by(|a, b| a.same_key(b));
    for primary in &chosen {
        let entry = pack.entry(primary).map_err(|_| damaged())?;
        let subs: Vec<_> = keys
            .by_ref()
            .find(|key| key[0].same_key(primary))
            .map(|key| key.iter().filter(|c| c.subkey()).collect())
            .unwrap_or_default();
        let own = primary.has_secret;
        let primary_shape = shape_of(&entry, primary, &entry.fpr_hex());
        let sub_fpr = |sub: &Component| {
            entry
                .subkeys
                .get(usize::from(sub.index) - 1)
                .map(|s| hex::encode_upper(&s.fingerprint))
        };

        if !args.with_colons {
            let fpr = if args.fingerprint {
                say::grouped_fingerprint(&entry.fpr_hex())
            } else {
                entry.fpr_hex()
            };
            out.push_str(&format!(
                "{key_word}   {}\n      {fpr}\n",
                describe(primary, &primary_shape)
            ));
            let tag = say::validity_tag(primary.revoked, primary.expired_at(now()), own);
            for uid in &entry.user_ids {
                out.push_str(&format!("uid           {tag} {uid}\n"));
            }
            for sub in &subs {
                let fpr = sub_fpr(sub).unwrap_or_default();
                let shape = shape_of(&entry, sub, &fpr);
                out.push_str(&format!("{sub_word}   {}\n", describe(sub, &shape)));
            }
            out.push('\n');
            continue;
        }

        let usable: u8 = std::iter::once(primary)
            .chain(subs.iter().copied())
            .filter(|c| usable(c))
            .fold(0, |acc, c| acc | c.caps);
        let caps = format!(
            "{}{}",
            cap_letters(primary.caps, false),
            cap_letters(usable, true)
        );
        out.push_str(&colon_key_line(
            key_word,
            primary,
            &entry.key_id_hex(),
            &primary_shape,
            own,
            caps,
        ));
        out.push('\n');
        out.push_str(&format!("fpr:::::::::{}:\n", entry.fpr_hex()));
        let v = validity(primary, own);
        let cert = SignedPublicKey::from_bytes(&entry.public_key[..]).ok();
        for uid in &entry.user_ids {
            let (created, hash) = cert
                .as_ref()
                .map_or((String::new(), String::new()), |cert| uid_fields(cert, uid));
            out.push_str(&format!(
                "uid:{v}::::{created}::{hash}::{}::::::::::0:\n",
                colon_escape(uid)
            ));
        }
        for sub in &subs {
            let Some(info) = entry.subkeys.get(usize::from(sub.index) - 1) else {
                continue;
            };
            let fpr = hex::encode_upper(&info.fingerprint);
            let shape = shape_of(&entry, sub, &fpr);
            out.push_str(&colon_key_line(
                sub_word,
                sub,
                &hex::encode_upper(&info.key_id),
                &shape,
                own,
                cap_letters(sub.caps, false),
            ));
            out.push('\n');
            out.push_str(&format!("fpr:::::::::{fpr}:\n"));
        }
    }
    write_out(io, out.as_bytes())
}

/// Bytes rPGP's armor writer takes as they are.
struct RawPackets(Vec<u8>);

impl pgp::ser::Serialize for RawPackets {
    fn to_writer<W: Write>(&self, writer: &mut W) -> pgp::errors::Result<()> {
        writer.write_all(&self.0)?;
        Ok(())
    }

    fn write_len(&self) -> usize {
        self.0.len()
    }
}

/// The public certificates of the keys named, or of all keys: never a secret
/// key. Naming nothing exports nothing, which is no error, as with gpg.
fn export(
    args: &Args,
    package: &Package,
    io: &mut Io,
    status: &mut Status,
) -> Result<(), PipeError> {
    let (_, pack) = package.get()?;
    let chosen = selected(&args.patterns(), false, pack)?;
    if chosen.is_empty() {
        say::line(&mut *io.stderr, "WARNING: nothing exported");
        status.push("EXPORT_RES 0 0 0");
        return Ok(());
    }
    let mut certs = Vec::new();
    // The CRC24 footer GnuPG needs, only when every key is v4-era (RFC 9580
    // §6.1), as the engine armors keys.
    let mut v4_only = true;
    for c in &chosen {
        let entry = pack.entry(c).map_err(|_| damaged())?;
        status.push(format!("EXPORTED {}", entry.fpr_hex()));
        v4_only &= SignedPublicKey::from_bytes(&entry.public_key[..])
            .is_ok_and(|k| crate::utils::is_v4_era(k.primary_key.version()));
        certs.extend(entry.public_key);
    }
    let bytes = if args.armor {
        let mut out = Vec::new();
        pgp::armor::write(
            &RawPackets(certs),
            pgp::armor::BlockType::PublicKey,
            &mut out,
            None,
            v4_only,
        )
        .map_err(|_| PipeError::error("cannot armor the keys"))?;
        out
    } else {
        certs
    };
    let out = target(args, None);
    with_output(args, io, out, Op::Default, "", |mut sink| {
        sink.writer().write_all(&bytes).map_err(|e| {
            PipeError::error(format!("[stdout]: write error: {}", say::io_reason(&e)))
                .failure("write", code::system(&e))
        })
    })?;
    status.push(format!("EXPORT_RES {} 0 {}", chosen.len(), chosen.len()));
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn dates_are_civil_dates() {
        assert_eq!(date_of(0), "1970-01-01");
        assert_eq!(date_of(951_782_400), "2000-02-29");
        assert_eq!(date_of(1_700_000_000), "2023-11-14");
    }

    #[test]
    fn crlf_becomes_lf_across_writes() {
        let text = b"a\r\nb\r\r\nlone\rcr\r\nend\r";
        for split in 0..=text.len() {
            let mut out = Vec::new();
            let mut lf = CrlfToLf::new(&mut out);
            lf.write_all(&text[..split]).unwrap();
            lf.write_all(&text[split..]).unwrap();
            lf.finish().unwrap();
            assert_eq!(out, b"a\nb\r\nlone\rcr\nend\r", "split at {split}");
        }
    }

    #[test]
    fn algorithm_names_map_to_their_numbers() {
        assert_eq!(hash_algo_id("Sha512"), 10);
        assert_eq!(hash_algo_id("SHA512"), 10);
        assert_eq!(pk_algo_id("Ed25519"), 27);
        assert_eq!(pk_algo_id("not an algorithm"), 0);
    }

    #[test]
    fn decryption_info_names_cipher_and_mode() {
        assert_eq!(decryption_info("AES256.CFB").as_deref(), Some("2 9 0"));
        assert_eq!(decryption_info("AES128.OCB").as_deref(), Some("0 7 2"));
        assert_eq!(decryption_info(""), None);
    }

    #[test]
    fn the_start_of_a_message_says_what_it_is() {
        let at = |raw: &[u8]| armor_start(raw);
        assert_eq!(at(b"-----BEGIN PGP MESSAGE-----\n"), Some(0));
        assert_eq!(at(b"Hi,\r\n\r\n-----BEGIN PGP MESSAGE-----\n"), Some(7));
        // A marker inside a line is quoted text, not armor.
        assert_eq!(at(b"see -----BEGIN PGP SIGNED MESSAGE----- here\n"), None);
        assert_eq!(kind_of_packets(b"plain text"), Kind::NotOpenPgp);
        assert_eq!(kind_of_packets(&[]), Kind::NotOpenPgp);
    }

    #[test]
    fn a_line_is_read_byte_by_byte_and_retried_when_interrupted() {
        struct Flaky(Vec<u8>, bool);
        impl Read for Flaky {
            fn read(&mut self, buf: &mut [u8]) -> std::io::Result<usize> {
                self.1 = !self.1;
                if self.1 {
                    return Err(std::io::ErrorKind::Interrupted.into());
                }
                if self.0.is_empty() {
                    return Ok(0);
                }
                buf[0] = self.0.remove(0);
                Ok(1)
            }
        }
        let mut r = Flaky(b"secret\r\nrest".to_vec(), false);
        assert_eq!(&read_line(&mut r).unwrap()[..], b"secret");
        assert_eq!(r.0, b"rest");
    }
}
