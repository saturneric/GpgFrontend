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
//! only, and gpg's status lines where callers parse them.

use std::cell::RefCell;
use std::ffi::{CStr, CString, c_char, c_void};
use std::fs::File;
use std::io::{Read, Seek, SeekFrom, Write};
use std::path::Path;

use pgp::composed::{Deserializable, SignedPublicKey, SignedSecretKey};
use pgp::types::KeyDetails;
use zeroize::Zeroizing;

use super::args::{Args, Command, Passphrase, SignKind};
use super::{Io, PipeError, Status};
use crate::crypto::SignatureResultInternal;
use crate::keypack::{CAP_SIGN, Found, KeyEntry, OpenPackage};
use crate::types::{
    GfrKeyAlgo, GfrPassphraseState, GfrPasswordFetchStatus, GfrSignMode, GfrSignatureStatus,
    GfrStatus,
};
use crate::utils::key_armor_opts;

/// The engine channel gfpipe runs on; it has the process to itself.
const CHANNEL: i32 = 0;
/// Past this, a detached signature, the one input read into memory, is
/// refused: no real one comes near it.
const MAX_SIGNATURE: u64 = 16 << 20;

pub fn run(
    command: Command,
    args: &Args,
    path: &Path,
    pack: &OpenPackage,
    io: &mut Io,
    status: &mut Status,
) -> Result<(), PipeError> {
    // Read before the data: with --passphrase-fd 0 the passphrase is the
    // first line of standard input and the data the rest, as with gpg.
    load_passphrase(args, io)?;
    let result = match command {
        Command::Sign(kind) => sign(kind, args, pack, io, status),
        Command::Verify => verify(args, pack, io, status),
        Command::Encrypt { sign } => encrypt(sign, args, pack, io, status),
        Command::Symmetric => symmetric(args, io, status),
        Command::Decrypt => decrypt(args, pack, io, status),
        Command::ListKeys { secret } => list(secret, args, path, pack, io),
        Command::Export => export(args, pack, io),
        Command::Version | Command::Help => Ok(()),
    };
    PASSPHRASE.with(|p| *p.borrow_mut() = None);
    result
}

/// Run a command that needs no key package (`-c`).
pub fn run_without_package(
    command: Command,
    args: &Args,
    io: &mut Io,
    status: &mut Status,
) -> Result<(), PipeError> {
    load_passphrase(args, io)?;
    let result = match command {
        Command::Symmetric => symmetric(args, io, status),
        _ => Err(PipeError::usage("this needs a key package")),
    };
    PASSPHRASE.with(|p| *p.borrow_mut() = None);
    result
}

// --------------------------------------------------------------- passphrase

thread_local! {
    /// The passphrase given on the command line, for this run only.
    static PASSPHRASE: RefCell<Option<Zeroizing<Vec<u8>>>> = const { RefCell::new(None) };
}

fn has_passphrase() -> bool {
    PASSPHRASE.with(|p| p.borrow().as_ref().is_some_and(|p| !p.is_empty()))
}

/// The first line of `reader`, read a byte at a time so nothing after it is
/// taken from a shared stream.
fn read_line(reader: &mut dyn Read) -> std::io::Result<Zeroizing<Vec<u8>>> {
    let mut line = Zeroizing::new(Vec::new());
    let mut byte = [0u8; 1];
    while reader.read(&mut byte)? == 1 && byte[0] != b'\n' {
        line.push(byte[0]);
    }
    if line.last() == Some(&b'\r') {
        line.pop();
    }
    Ok(line)
}

fn load_passphrase(args: &Args, io: &mut Io) -> Result<(), PipeError> {
    let cannot = |what: &str, e: std::io::Error| {
        PipeError::usage(format!("cannot read the passphrase from {what}: {e}"))
    };
    let passphrase = match &args.passphrase {
        None => None,
        Some(Passphrase::Text(text)) => Some(Zeroizing::new(text.as_bytes().to_vec())),
        Some(Passphrase::File(path)) => {
            let mut file = File::open(path).map_err(|e| cannot(path, e))?;
            Some(read_line(&mut file).map_err(|e| cannot(path, e))?)
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
    // The caller owns the descriptor; it is read, not closed.
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
    let given = PASSPHRASE.with(|p| p.borrow().as_ref().filter(|p| !p.is_empty()).cloned());
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

// ------------------------------------------------------------------ helpers

/// The `index`th file argument, or standard input.
fn input(args: &Args, index: usize, io: &mut Io) -> Result<Box<dyn Read + Send + Sync>, PipeError> {
    match args.inputs.get(index).map(String::as_str) {
        Some(file) if file != "-" => {
            let f = File::open(file)
                .map_err(|e| PipeError::usage(format!("cannot read {file}: {e}")))?;
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
        .map_err(|e| PipeError::usage(format!("cannot read the signature: {e}")))?;
    if data.len() as u64 > MAX_SIGNATURE {
        return Err(PipeError::usage("the signature is too large"));
    }
    Ok(data)
}

/// The `index`th input as a file, for operations that stream: a named file
/// as it is, standard input spooled into an anonymous temporary file, so
/// input of any size takes flat memory. Its length, for the engine's guards.
/// The spool is in the OS temporary directory (`TMPDIR` on Unix): standard
/// input has no directory of its own.
fn input_file(args: &Args, index: usize, io: &mut Io) -> Result<(File, u64), PipeError> {
    let file = match args.inputs.get(index).map(String::as_str) {
        Some(name) if name != "-" => {
            File::open(name).map_err(|e| PipeError::usage(format!("cannot read {name}: {e}")))?
        }
        _ => {
            let cannot =
                |e: std::io::Error| PipeError::usage(format!("cannot read the input: {e}"));
            // No name, so it disappears on every exit path.
            let mut spool = tempfile::tempfile().map_err(cannot)?;
            let mut stdin = std::mem::replace(&mut io.stdin, Box::new(std::io::empty()));
            std::io::copy(&mut stdin, &mut spool).map_err(cannot)?;
            spool.seek(SeekFrom::Start(0)).map_err(cannot)?;
            spool
        }
    };
    let len = file
        .metadata()
        .map_err(|e| PipeError::usage(format!("cannot read the input: {e}")))?
        .len();
    Ok((file, len))
}

/// What the start of a message says, read without loading the rest.
struct Head {
    armored: bool,
    clear_signed: bool,
    /// The first packets, dearmored: enough for the first packet's tag and
    /// the recipients, which come first.
    packets: Vec<u8>,
}

/// How much of a message's start is read to look at it.
const HEAD_BYTES: u64 = 1 << 20;

fn head_of(file: &mut File) -> Result<Head, PipeError> {
    let cannot = |e: std::io::Error| PipeError::usage(format!("cannot read the input: {e}"));
    let mut raw = Vec::new();
    Read::by_ref(file)
        .take(HEAD_BYTES)
        .read_to_end(&mut raw)
        .map_err(cannot)?;
    file.seek(SeekFrom::Start(0)).map_err(cannot)?;

    let clear_signed = is_clear_signed(&raw);
    let armored = raw
        .iter()
        .position(|b| !b.is_ascii_whitespace())
        .is_some_and(|at| raw[at..].starts_with(b"-----BEGIN PGP"));
    let packets = if armored && !clear_signed {
        // Streamed through the armor: a cut-off block still yields its start.
        let mut decoded = Vec::new();
        let _ = pgp::armor::Dearmor::new(std::io::BufReader::new(&raw[..]))
            .take(HEAD_BYTES)
            .read_to_end(&mut decoded);
        decoded
    } else {
        raw
    };
    Ok(Head {
        armored,
        clear_signed,
        packets,
    })
}

/// Where the output goes: the `-o` file; or, as gpg does when it signs or
/// encrypts a named file, that file with `extension` added; or standard
/// output.
fn output_path(args: &Args, extension: Option<&str>) -> Option<String> {
    match args.output.as_deref() {
        Some("-") => None,
        Some(file) => Some(file.to_string()),
        None => {
            let ext = extension?;
            args.inputs
                .first()
                .filter(|f| f.as_str() != "-")
                .map(|f| format!("{f}{ext}"))
        }
    }
}

/// Run `f` with the output (see [`output_path`]). An existing file is only
/// replaced with `--yes`; a file that was not finished is removed.
fn with_output<T>(
    args: &Args,
    io: &mut Io,
    extension: Option<&str>,
    f: impl FnOnce(&mut (dyn Write + Send + Sync)) -> Result<T, PipeError>,
) -> Result<T, PipeError> {
    let Some(file) = output_path(args, extension) else {
        return f(&mut *io.stdout);
    };
    if !args.yes && Path::new(&file).exists() {
        return Err(PipeError::usage(format!(
            "{file} exists; add --yes to replace it"
        )));
    }
    let mut out =
        File::create(&file).map_err(|e| PipeError::usage(format!("cannot write {file}: {e}")))?;
    let result = f(&mut out).and_then(|v| {
        out.flush()
            .map_err(|e| PipeError::usage(format!("cannot write {file}: {e}")))?;
        Ok(v)
    });
    if result.is_err() {
        let _ = std::fs::remove_file(&file);
    }
    result
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

/// The engine's own words for the last failure, or the status.
fn engine_error(status: GfrStatus) -> String {
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
    PipeError::failed(format!(
        "the key package cannot be read: {}",
        super::engine_detail()
    ))
}

fn find(pack: &OpenPackage, query: &str) -> Result<Option<Found>, PipeError> {
    pack.find(query).map_err(|_| damaged())
}

fn find_required(pack: &OpenPackage, query: &str) -> Result<Found, PipeError> {
    find(pack, query)?
        .ok_or_else(|| PipeError::usage(format!("{query}: no such key in the package")))
}

fn now() -> u64 {
    std::time::SystemTime::now()
        .duration_since(std::time::UNIX_EPOCH)
        .map_or(0, |d| d.as_secs())
}

/// The secret key block for each signer named by `-u`, or the package's
/// only secret key. What can be decided from the index is decided before any
/// key is decrypted.
fn signers(
    args: &Args,
    pack: &OpenPackage,
) -> Result<Vec<(Zeroizing<String>, KeyEntry)>, PipeError> {
    let chosen: Vec<Found> = if args.local_users.is_empty() {
        let secret: Vec<_> = pack
            .primaries()
            .into_iter()
            .filter(|c| c.has_secret)
            .collect();
        match secret.as_slice() {
            [only] => vec![Found {
                entry: pack.entry(only).map_err(|_| damaged())?,
                component: *only,
                exact: None,
            }],
            [] => return Err(PipeError::usage("the key package holds no secret key")),
            _ => {
                return Err(PipeError::usage(
                    "the key package holds several secret keys; name one with -u",
                ));
            }
        }
    } else {
        args.local_users
            .iter()
            .map(|query| find_required(pack, query))
            .collect::<Result<_, _>>()?
    };

    let now = now();
    chosen
        .into_iter()
        .map(|found| {
            let entry = &found.entry;
            if !found.component.has_secret {
                return Err(PipeError::usage(format!(
                    "{} ({}) has no secret key in the package",
                    entry.fpr_hex(),
                    entry.primary_user_id()
                )));
            }
            // The components that could make the signature: the one named,
            // or any of the key that may sign.
            let candidates: Vec<_> = if found.exact.is_some() {
                vec![found.component]
            } else {
                pack.components()
                    .into_iter()
                    .filter(|c| c.same_key(&found.component) && c.can(CAP_SIGN))
                    .collect()
            };
            let usable: Vec<_> = candidates
                .iter()
                .filter(|c| !c.revoked && !c.expired_at(now))
                .collect();
            if usable.is_empty() {
                return Err(cannot_sign(entry, GfrStatus::ErrorUnusableKey));
            }
            if usable.iter().all(|c| c.protected) && !has_passphrase() {
                return Err(PipeError::usage(format!(
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
    let detail = if status == GfrStatus::ErrorUnusableKey {
        "no subkey that may sign is valid".to_string()
    } else {
        engine_error(status)
    };
    if status == GfrStatus::ErrorUnusableKey {
        PipeError::failed(format!(
            "the key {} ({}) cannot sign: {detail}. A package holds the keys as they were when \
             it was exported; renew the key in GpgFrontend and export the package again.",
            entry.fpr_hex(),
            entry.primary_user_id()
        ))
    } else {
        PipeError::failed(format!("signing failed: {detail}"))
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

/// `YYYY-MM-DD` of a Unix time, for VALIDSIG.
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

// ----------------------------------------------------- engine key callbacks

/// Hands the engine the public certificate the issuer `id` belongs to.
extern "C" fn fetch_public(id: *const c_char, user_data: *mut c_void) -> *mut c_char {
    lookup(id, user_data, |_, entry| armored_public(&entry.public_key))
}

/// Hands the engine the secret key the key ID `id` belongs to.
extern "C" fn fetch_secret(id: *const c_char, user_data: *mut c_void) -> *mut c_char {
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
    pack.find(id)
        .ok()
        .flatten()
        .and_then(|found| pick(pack, &found.entry))
        .and_then(|block| CString::new(block.as_str()).ok())
        // Freed by the engine through gfc_secure_free_cstr.
        .map_or(std::ptr::null_mut(), CString::into_raw)
}

fn pack_ptr(pack: &OpenPackage) -> *mut c_void {
    pack as *const OpenPackage as *mut c_void
}

// --------------------------------------------------------------- operations

fn sign(
    kind: SignKind,
    args: &Args,
    pack: &OpenPackage,
    io: &mut Io,
    status: &mut Status,
) -> Result<(), PipeError> {
    let signers = signers(args, pack)?;
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

    let data = input(args, 0, io)?;
    let result = with_output(args, io, Some(ext), |out| {
        crate::crypto::sign_stream_internal(
            CHANNEL,
            "",
            data,
            out,
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
        let id = hex::decode(&sig.fpr).unwrap_or_default();
        let primary = signers
            .iter()
            .map(|(_, entry)| entry)
            .find(|e| {
                e.fingerprint == id
                    || e.subkeys
                        .iter()
                        .any(|s| s.fingerprint == id || s.key_id == id)
            })
            .map_or_else(|| sig.fpr.to_uppercase(), KeyEntry::fpr_hex);
        let hash = hash_algo_id(&sig.hash_algo);
        status.push(format!("KEY_CONSIDERED {primary} 0"));
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

/// How one signature reads: its status line, whether it counts as good, and
/// the sentence for a person.
fn report(
    sig: &SignatureResultInternal,
    pack: &OpenPackage,
    status: &mut Status,
    stderr: &mut dyn Write,
) -> bool {
    let found = pack.find(&sig.fpr).ok().flatten();
    let (key_id, primary, uid, has_secret) = match &found {
        Some(f) => {
            let id = hex::decode(&sig.fpr).unwrap_or_default();
            (
                f.entry.component_key_id_hex(&id),
                f.entry.fpr_hex(),
                f.entry.primary_user_id().to_string(),
                f.component.has_secret,
            )
        }
        None => {
            let id = sig.fpr.to_uppercase();
            let key_id = match id.len() {
                40 => id[24..].to_string(),
                64 => id[..16].to_string(),
                _ => id.clone(),
            };
            (key_id, id, String::new(), false)
        }
    };
    let pk = pk_algo_id(&sig.pub_algo);
    let hash = hash_algo_id(&sig.hash_algo);
    let class = if sig.sig_type == GfrSignMode::ClearText {
        "01"
    } else {
        "00"
    };

    status.push("NEWSIG");
    let validsig = |status: &mut Status| {
        let version = if sig.fpr.len() == 64 { 6 } else { 4 };
        status.push(format!(
            "VALIDSIG {} {} {} {} {version} 0 {pk} {hash} {class} {primary}",
            sig.fpr.to_uppercase(),
            date_of(sig.created_at),
            sig.created_at,
            sig.expires_at
        ));
        // A key in the package is trusted; one whose secret it holds is the
        // user's own.
        status.push(if has_secret {
            "TRUST_ULTIMATE 0 pgp"
        } else {
            "TRUST_FULLY 0 pgp"
        });
    };

    let (good, message) = match sig.status {
        GfrSignatureStatus::Valid => {
            status.push(format!("GOODSIG {key_id} {uid}"));
            validsig(status);
            (true, format!("Good signature from \"{uid}\" [{primary}]"))
        }
        GfrSignatureStatus::KeyExpiredSince => {
            status.push(format!("EXPKEYSIG {key_id} {uid}"));
            validsig(status);
            (
                true,
                format!("Good signature from \"{uid}\" [{primary}], made before the key expired"),
            )
        }
        GfrSignatureStatus::KeyRevoked | GfrSignatureStatus::KeyRevokedSince => {
            status.push(format!("REVKEYSIG {key_id} {uid}"));
            (false, format!("The key \"{uid}\" [{primary}] is revoked"))
        }
        GfrSignatureStatus::SignatureExpired => {
            status.push(format!("EXPSIG {key_id} {uid}"));
            (
                false,
                format!("Expired signature from \"{uid}\" [{primary}]"),
            )
        }
        GfrSignatureStatus::NoKey => {
            status.push(format!(
                "ERRSIG {key_id} {pk} {hash} {class} {} 9 {}",
                sig.created_at,
                sig.fpr.to_uppercase()
            ));
            status.push(format!("NO_PUBKEY {key_id}"));
            (
                false,
                format!("Signed by {key_id}, a key that is not in the key package"),
            )
        }
        other => {
            status.push(format!("BADSIG {key_id} {uid}"));
            let why = match other {
                GfrSignatureStatus::KeyExpired => "made after the key expired",
                GfrSignatureStatus::KeyNotAuthorized => "the key may not make signatures",
                GfrSignatureStatus::WeakHash => "made with a hash too weak to rely on",
                _ => "it does not match the data",
            };
            (
                false,
                format!("BAD signature from \"{uid}\" [{primary}]: {why}"),
            )
        }
    };
    let _ = writeln!(stderr, "gfpipe: {message}");
    good
}

const CLEAR_SIGNED: &[u8] = b"-----BEGIN PGP SIGNED MESSAGE-----";

fn is_clear_signed(message: &[u8]) -> bool {
    message
        .windows(CLEAR_SIGNED.len())
        .any(|w| w == CLEAR_SIGNED)
}

/// Whether `message` is only signatures: a detached signature.
fn is_detached_signature(message: &[u8]) -> bool {
    use pgp::packet::{Packet, PacketParser};
    if is_clear_signed(message) {
        return false;
    }
    let bytes = crate::crypto::dearmor_all(message);
    let mut packets = PacketParser::new(std::io::Cursor::new(&bytes[..])).peekable();
    packets.peek().is_some() && packets.all(|p| matches!(p, Ok(Packet::Signature(_))))
}

/// Whether a message, by its first packets (dearmored), is encrypted rather
/// than only signed.
fn is_encrypted_packets(packets: &[u8]) -> bool {
    // PKESK, SKESK, SED, SEIPD, and LibrePGP's OCB packet (RFC 9580 §5).
    matches!(
        crate::keypack::first_packet_tag(packets),
        Some(1 | 3 | 9 | 18 | 20)
    )
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
fn signed_file_of(sig: &str) -> Option<String> {
    [".sig", ".asc", ".sign"]
        .iter()
        .find_map(|ext| sig.strip_suffix(ext))
        .filter(|data| !data.is_empty() && Path::new(data).is_file())
        .map(str::to_string)
}

/// Report every signature; true when all are good.
fn report_all(
    signatures: &[SignatureResultInternal],
    pack: &OpenPackage,
    status: &mut Status,
    io: &mut Io,
) -> bool {
    let mut all_good = true;
    for sig in signatures {
        all_good &= report(sig, pack, status, &mut *io.stderr);
    }
    all_good
}

fn verify(
    args: &Args,
    pack: &OpenPackage,
    io: &mut Io,
    status: &mut Status,
) -> Result<(), PipeError> {
    // `verify SIG DATA` checks a detached signature, streaming the data;
    // `verify FILE` a signed message, inline or clear-text, or a detached
    // signature whose data is the file of the same name without its
    // extension.
    let signatures = if args.inputs.len() >= 2 {
        let sig = read_signature(input(args, 0, io)?)?;
        let (data, _) = input_file(args, 1, io)?;
        verify_detached(data, &sig, pack, status)?
    } else {
        // Streamed: a signed message is read once, its start alone told
        // apart. A detached signature is small, so a file whose start holds
        // all of it and nothing but signatures is one.
        let (mut file, len) = input_file(args, 0, io)?;
        let head = head_of(&mut file)?;
        let detached = if !head.clear_signed && len <= HEAD_BYTES {
            let raw = read_signature(&mut file)?;
            file.seek(SeekFrom::Start(0))
                .map_err(|e| PipeError::usage(format!("cannot read the input: {e}")))?;
            is_detached_signature(&raw).then_some(raw)
        } else {
            None
        };
        if let Some(sig) = detached {
            let named = args.inputs.first().filter(|f| f.as_str() != "-");
            let data_file = named.and_then(|f| signed_file_of(f)).ok_or_else(|| {
                status.push("NODATA 1");
                PipeError::usage(
                    "this is a detached signature; give the signed data too: verify SIG DATA",
                )
            })?;
            let data = File::open(&data_file)
                .map_err(|e| PipeError::usage(format!("cannot read {data_file}: {e}")))?;
            verify_detached(data, &sig, pack, status)?
        } else {
            let result = if head.clear_signed {
                crate::crypto::verify_cleartext_stream_internal(
                    CHANNEL,
                    file,
                    std::io::sink(),
                    None,
                    Some(fetch_public),
                    pack_ptr(pack),
                )
            } else {
                crate::crypto::verify_inline_stream_internal(
                    CHANNEL,
                    file,
                    len,
                    std::io::sink(),
                    Some(fetch_public),
                    pack_ptr(pack),
                )
            };
            result
                .map_err(|e| {
                    status.push("NODATA 4");
                    PipeError::failed(format!("cannot verify: {}", engine_error(e)))
                })?
                .signatures
        }
    };

    if signatures.is_empty() {
        status.push("NODATA 3");
        return Err(PipeError::failed("no signature found"));
    }
    if report_all(&signatures, pack, status, io) {
        Ok(())
    } else {
        Err(PipeError::failed("the signature is not good"))
    }
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
        pack_ptr(pack),
    )
    .map(|result| result.signatures)
    .map_err(|e| {
        status.push("NODATA 4");
        PipeError::failed(format!("cannot verify: {}", engine_error(e)))
    })
}

/// The recipients: those named, else `--default-recipient`, plus the
/// package's own secret key with `--default-recipient-self`.
fn recipient_queries(args: &Args, pack: &OpenPackage) -> Result<Vec<String>, PipeError> {
    let mut queries = args.recipients.clone();
    if queries.is_empty() {
        queries.extend(args.default_recipient.clone());
    }
    if args.default_recipient_self {
        let secret: Vec<_> = pack
            .primaries()
            .into_iter()
            .filter(|c| c.has_secret)
            .collect();
        match secret.as_slice() {
            [only] => queries.push(pack.entry(only).map_err(|_| damaged())?.fpr_hex()),
            [] => return Err(PipeError::usage("the key package holds no secret key")),
            _ => {
                return Err(PipeError::usage(
                    "the key package holds several secret keys; name the recipient with -r",
                ));
            }
        }
    }
    if queries.is_empty() {
        return Err(PipeError::usage("name at least one recipient with -r"));
    }
    Ok(queries)
}

fn encrypt(
    with_signature: bool,
    args: &Args,
    pack: &OpenPackage,
    io: &mut Io,
    status: &mut Status,
) -> Result<(), PipeError> {
    let recipients: Vec<Zeroizing<String>> = recipient_queries(args, pack)?
        .iter()
        .map(|query| {
            let found = find_required(pack, query)?;
            let public = armored_public(&found.entry.public_key).ok_or_else(damaged)?;
            Ok(pinned(&public, found.exact.as_deref()))
        })
        .collect::<Result<_, _>>()?;
    let public: Vec<&str> = recipients.iter().map(|b| b.as_str()).collect();

    let signers = if with_signature {
        signers(args, pack)?
    } else {
        Vec::new()
    };
    let secret: Vec<&str> = signers.iter().map(|(b, _)| b.as_str()).collect();

    let data = input(args, 0, io)?;
    status.push("BEGIN_ENCRYPTION 2 9");
    let ext = extension(args, false);
    let result = with_output(args, io, Some(ext), |out| {
        crate::crypto::encrypt_and_sign_stream_internal(
            CHANNEL,
            "",
            data,
            out,
            &public,
            &secret,
            Some(pipe_password),
            args.armor,
        )
        .map_err(|e| match signers.first() {
            Some((_, entry)) if e == GfrStatus::ErrorUnusableKey => cannot_sign(entry, e),
            _ => PipeError::failed(format!("encryption failed: {}", engine_error(e))),
        })
    })?;
    for rejected in &result.invalid_recipients {
        status.push(format!("INV_RECP 0 {}", rejected.fpr));
        let _ = writeln!(
            io.stderr,
            "gfpipe: {} cannot receive encrypted messages ({:?}); left out",
            rejected.fpr, rejected.reason
        );
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
    use pgp::crypto::sym::SymmetricKeyAlgorithm;
    use pgp::types::StringToKey;

    let Some(passphrase) =
        PASSPHRASE.with(|p| p.borrow().as_ref().filter(|p| !p.is_empty()).cloned())
    else {
        return Err(PipeError::usage(
            "encrypting with a passphrase needs it: give --passphrase-file, --passphrase-fd or \
             --passphrase",
        ));
    };
    let failed = |e: pgp::errors::Error| PipeError::failed(format!("encryption failed: {e}"));

    let data = input(args, 0, io)?;
    status.push("BEGIN_ENCRYPTION 2 9");
    with_output(args, io, Some(extension(args, false)), |out| {
        let mut rng = rand::thread_rng();
        let mut builder = MessageBuilder::from_reader("", data);
        builder.partial_chunk_size(512 * 1024).map_err(failed)?;
        let mut sealed = builder.seipd_v1(&mut rng, SymmetricKeyAlgorithm::AES256);
        let password = crate::utils::password_from_zeroizing_bytes(passphrase);
        sealed
            .encrypt_with_password(StringToKey::new_default(&mut rng), &password)
            .map_err(failed)?;
        if args.armor {
            sealed
                .to_armored_writer(&mut rng, crate::utils::armor_opts_for(true), &mut *out)
                .map_err(failed)
        } else {
            sealed.to_writer(&mut rng, &mut *out).map_err(failed)
        }
    })?;
    status.push("END_ENCRYPTION");
    Ok(())
}

fn decrypt(
    args: &Args,
    pack: &OpenPackage,
    io: &mut Io,
    status: &mut Status,
) -> Result<(), PipeError> {
    // Streamed: standard input is spooled to a temporary file, and the
    // plaintext is staged in another until it is authenticated.
    let (mut file, ciphertext_len) = input_file(args, 0, io)?;
    let head = head_of(&mut file)?;

    // gpg --decrypt of a message that is only signed puts out its content and
    // checks the signature. It streams too, and reaches the output only once
    // the whole message has been read.
    if head.clear_signed || !is_encrypted_packets(&head.packets) {
        let unreadable = |e: GfrStatus, status: &mut Status| {
            status.push("NODATA 2");
            PipeError::failed(format!("cannot read the message: {}", engine_error(e)))
        };
        let result = with_output(args, io, None, |out| {
            if head.clear_signed {
                // The signed text is canonical (CRLF); gpg gives it back as
                // written.
                let mut text = CrlfToLf::new(out);
                let result = crate::crypto::verify_cleartext_stream_internal(
                    CHANNEL,
                    file,
                    &mut text,
                    None,
                    Some(fetch_public),
                    pack_ptr(pack),
                )
                .map_err(|e| unreadable(e, status))?;
                text.finish()
                    .map_err(|e| PipeError::usage(format!("cannot write the output: {e}")))?;
                Ok(result)
            } else {
                crate::crypto::verify_inline_stream_staged_internal(
                    CHANNEL,
                    file,
                    ciphertext_len,
                    out,
                    Some(fetch_public),
                    pack_ptr(pack),
                )
                .map_err(|e| unreadable(e, status))
            }
        })?;
        return if report_all(&result.signatures, pack, status, io) {
            Ok(())
        } else {
            Err(PipeError::failed("the signature is not good"))
        };
    }

    let recipients = crate::crypto::sniff_recipients(&head.packets);
    for r in &recipients {
        status.push(format!(
            "ENC_TO {} {} 0",
            r.key_id.to_uppercase(),
            pk_algo_id(&r.pub_algo)
        ));
    }
    status.push("BEGIN_DECRYPTION");
    let failed = |e: GfrStatus, status: &mut Status| {
        if matches!(e, GfrStatus::ErrorNoSecretKey | GfrStatus::ErrorNoKey) {
            for r in &recipients {
                status.push(format!("NO_SECKEY {}", r.key_id.to_uppercase()));
            }
        }
        status.push("DECRYPTION_FAILED");
        status.push("END_DECRYPTION");
        let detail = engine_error(e);
        if recipients.is_empty() && !has_passphrase() {
            PipeError::failed(
                "the message is encrypted with a passphrase; give it with --passphrase-file, \
                 --passphrase-fd or --passphrase",
            )
        } else if matches!(
            e,
            GfrStatus::ErrorNoSecretKey | GfrStatus::ErrorNoKey | GfrStatus::ErrorDecryptionFailed
        ) && pack.primaries().iter().all(|c| !c.has_secret)
        {
            PipeError::failed("the key package holds no secret key to decrypt with")
        } else {
            PipeError::failed(format!("cannot decrypt: {detail}"))
        }
    };
    let result = with_output(args, io, None, |out| {
        crate::crypto::decrypt_and_verify_stream_staged_internal(
            CHANNEL,
            file,
            ciphertext_len,
            out,
            head.armored,
            Some(fetch_secret),
            Some(pipe_password),
            Some(fetch_public),
            pack_ptr(pack),
        )
        .map_err(|e| failed(e, status))
    })?;
    status.push("DECRYPTION_OKAY");
    status.push("GOODMDC");

    let all_good = report_all(&result.signatures, pack, status, io);
    status.push("END_DECRYPTION");
    if all_good {
        Ok(())
    } else {
        Err(PipeError::failed(
            "the message was decrypted, but its signature is not good",
        ))
    }
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

/// `[SC] [expires: 2027-01-01]`, from the index alone.
fn describe(c: &crate::keypack::Component) -> String {
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
    let mut text = format!("[{usage}]");
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
/// A pattern naming no key is an error, as with gpg.
fn selected(
    patterns: &[String],
    secret_only: bool,
    pack: &OpenPackage,
) -> Result<Vec<crate::keypack::Component>, PipeError> {
    let mut chosen = pack.primaries();
    if secret_only {
        chosen.retain(|c| c.has_secret);
    }
    if patterns.is_empty() {
        return Ok(chosen);
    }
    let mut named = Vec::new();
    for pattern in patterns {
        let found = find(pack, pattern)?
            .ok_or_else(|| PipeError::usage(format!("{pattern}: no such key in the package")))?;
        named.push(found.component);
    }
    chosen.retain(|c| named.iter().any(|n| n.same_key(c)));
    if chosen.is_empty() {
        return Err(PipeError::usage("no such secret key in the package"));
    }
    Ok(chosen)
}

/// gpg's validity letter: revoked, expired, ultimate (the user's own key) or
/// full (every key in the package is trusted).
fn validity(c: &crate::keypack::Component, own: bool) -> char {
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

/// The curve name gpg prints in field 17.
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
    c: &crate::keypack::Component,
    key_id: &str,
    key_length: u32,
    curve: &str,
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
        &key_length.to_string(),
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
        curve,
        "",
        "",
        "0",
    ];
    format!("{}:", fields.join(":"))
}

fn list(
    secret_only: bool,
    args: &Args,
    path: &Path,
    pack: &OpenPackage,
    io: &mut Io,
) -> Result<(), PipeError> {
    let chosen = selected(&args.inputs, secret_only, pack)?;
    let components = pack.components();
    let (key_word, sub_word) = if secret_only {
        ("sec", "ssb")
    } else {
        ("pub", "sub")
    };
    let out = &mut io.stdout;

    if !args.with_colons {
        let _ = writeln!(
            out,
            "{}\nexported {}\n",
            path.display(),
            date_of(u32::try_from(pack.header.generated_at).unwrap_or(u32::MAX))
        );
    }
    for primary in &chosen {
        let entry = pack.entry(primary).map_err(|_| damaged())?;
        let subs: Vec<_> = components
            .iter()
            .filter(|c| c.subkey() && c.same_key(primary))
            .collect();
        let own = primary.has_secret;

        if !args.with_colons {
            let _ = writeln!(out, "{key_word}  {} {}", entry.fpr_hex(), describe(primary));
            for uid in &entry.user_ids {
                let _ = writeln!(out, "uid  {uid}");
            }
            for sub in &subs {
                let fpr = entry
                    .subkeys
                    .get(usize::from(sub.index) - 1)
                    .map(|s| hex::encode_upper(&s.fingerprint))
                    .unwrap_or_default();
                let _ = writeln!(out, "{sub_word}  {fpr} {}", describe(sub));
            }
            let _ = writeln!(out);
            continue;
        }

        // Sizes and curves come from the certificate itself.
        let meta = SignedPublicKey::from_bytes(&entry.public_key[..])
            .ok()
            .map(|cert| crate::key::build_public_metadata(&cert));
        let usable: u8 = std::iter::once(primary)
            .chain(subs.iter().copied())
            .filter(|c| !c.revoked && !c.expired_at(now()))
            .fold(0, |acc, c| acc | c.caps);
        let caps = format!(
            "{}{}",
            cap_letters(primary.caps, false),
            cap_letters(usable, true)
        );
        let (len, curve) = meta
            .as_ref()
            .map_or((0, ""), |m| (m.key_length, curve_name(m.algo)));
        let _ = writeln!(
            out,
            "{}",
            colon_key_line(
                key_word,
                primary,
                &entry.key_id_hex(),
                len,
                curve,
                own,
                caps
            )
        );
        let _ = writeln!(out, "fpr:::::::::{}:", entry.fpr_hex());
        let v = validity(primary, own);
        for uid in &entry.user_ids {
            let _ = writeln!(out, "uid:{v}::::::::{}::::::::::0:", colon_escape(uid));
        }
        for sub in &subs {
            let Some(info) = entry.subkeys.get(usize::from(sub.index) - 1) else {
                continue;
            };
            let fpr = hex::encode_upper(&info.fingerprint);
            let (len, curve) = meta
                .as_ref()
                .and_then(|m| m.subkeys.iter().find(|s| s.fpr.eq_ignore_ascii_case(&fpr)))
                .map_or((0, ""), |s| (s.key_length, curve_name(s.algo)));
            let _ = writeln!(
                out,
                "{}",
                colon_key_line(
                    sub_word,
                    sub,
                    &hex::encode_upper(&info.key_id),
                    len,
                    curve,
                    own,
                    cap_letters(sub.caps, false),
                )
            );
            let _ = writeln!(out, "fpr:::::::::{fpr}:");
        }
    }
    Ok(())
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
/// key.
fn export(args: &Args, pack: &OpenPackage, io: &mut Io) -> Result<(), PipeError> {
    let mut certs = Vec::new();
    for c in selected(&args.inputs, false, pack)? {
        certs.extend(pack.entry(&c).map_err(|_| damaged())?.public_key);
    }
    let bytes = if args.armor {
        let mut out = Vec::new();
        pgp::armor::write(
            &RawPackets(certs),
            pgp::armor::BlockType::PublicKey,
            &mut out,
            None,
            true,
        )
        .map_err(|_| PipeError::failed("cannot armor the keys"))?;
        out
    } else {
        certs
    };
    with_output(args, io, None, |out| {
        out.write_all(&bytes)
            .map_err(|e| PipeError::usage(format!("cannot write the output: {e}")))
    })
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
}
