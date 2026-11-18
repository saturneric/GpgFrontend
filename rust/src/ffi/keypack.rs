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

//! Key package v3 entry points: write, open, read the header, and remove a
//! key's passphrase before it goes into a package. Keys cross as binary
//! OpenPGP, the header as a plain struct.

use std::ffi::{CString, c_char, c_void};
use std::panic::{AssertUnwindSafe, catch_unwind};

use zeroize::Zeroizing;

use crate::err::{clear_last_error, set_last_error};
use crate::ffi::mem::{gfr_crypto_free_buffer, gfr_crypto_free_string, into_wiped_boxed_slice};
use crate::keypack::{self, Protection, ProtectionRequest, Unlock};
use crate::types::{
    GfrBuffer, GfrKeypackHeaderC, GfrKeypackKeyC, GfrKeypackProtection, GfrKeypackReadCb,
    GfrKeypackWriteResultC, GfrPasswordFetchCb, GfrStatus,
};

fn status_of(result: std::thread::Result<Result<(), GfrStatus>>) -> GfrStatus {
    match result {
        Ok(Ok(())) => GfrStatus::Success,
        Ok(Err(e)) => e,
        Err(_) => GfrStatus::ErrorPanic,
    }
}

fn leak_bytes(data: Vec<u8>) -> (*mut u8, usize) {
    let boxed = into_wiped_boxed_slice(data);
    let len = boxed.len();
    (Box::into_raw(boxed).cast::<u8>(), len)
}

fn leak_string(s: &str) -> Result<*mut c_char, GfrStatus> {
    Ok(CString::new(s)
        .map_err(|_| GfrStatus::ErrorInternal)?
        .into_raw())
}

fn header_c(header: &keypack::Header) -> GfrKeypackHeaderC {
    let mut out = GfrKeypackHeaderC {
        version: header.version,
        protection: match header.protection {
            Protection::PinFile => GfrKeypackProtection::PinFile,
            Protection::Plain(_) => GfrKeypackProtection::Plain,
        },
        secrets_unprotected: header.secrets_unprotected,
        key_count: header.key_count,
        generated_at: header.generated_at,
        package_id: [0; crate::types::GFR_KEYPACK_ID_LEN],
    };
    // The ID length is checked on reading.
    out.package_id.copy_from_slice(&header.package_id);
    out
}

/// Optional buffer: empty or null means "not given".
unsafe fn opt_slice(buf: &GfrBuffer) -> Option<&[u8]> {
    if buf.is_empty() {
        None
    } else {
        unsafe { buf.as_slice() }
    }
}

/// Remove the passphrase from every component of the binary secret key
/// `secret_key`, asking for it through `fetch_pwd_cb`. The key database is not
/// touched.
///
/// On success `*out_key` is the unprotected binary secret key; free it with
/// `gfr_crypto_free_buffer`.
#[unsafe(no_mangle)]
pub extern "C" fn gfr_keypack_remove_key_password(
    channel: i32,
    secret_key: GfrBuffer,
    fetch_pwd_cb: GfrPasswordFetchCb,
    out_key: *mut *mut u8,
    out_key_len: *mut usize,
) -> GfrStatus {
    clear_last_error();
    if out_key.is_null() || out_key_len.is_null() {
        return GfrStatus::ErrorInvalidInput;
    }
    unsafe {
        *out_key = std::ptr::null_mut();
        *out_key_len = 0;
    }

    status_of(catch_unwind(|| {
        let key = unsafe { opt_slice(&secret_key) }.ok_or(GfrStatus::ErrorInvalidInput)?;
        let mut open = crate::key::remove_key_password_internal(channel, key, Some(fetch_pwd_cb))?;
        let (data, len) = leak_bytes(std::mem::take(&mut *open));
        unsafe {
            *out_key = data;
            *out_key_len = len;
        }
        Ok(())
    }))
}

/// Write a key package holding `keys`, each one binary OpenPGP key: a secret
/// key puts that key's secret into the package, a public one only its
/// certificate.
///
/// `pin` is used only for [`GfrKeypackProtection::PinFile`]. Free the result
/// with `gfr_keypack_free_write_result`, whatever the status.
#[unsafe(no_mangle)]
pub extern "C" fn gfr_keypack_write(
    keys: *const GfrBuffer,
    count: usize,
    protection: GfrKeypackProtection,
    pin: GfrBuffer,
    generated_at: u64,
    out_result: *mut GfrKeypackWriteResultC,
) -> GfrStatus {
    clear_last_error();
    if out_result.is_null() {
        return GfrStatus::ErrorInvalidInput;
    }
    unsafe { std::ptr::write_bytes(out_result, 0, 1) };

    status_of(catch_unwind(|| {
        if keys.is_null() || count == 0 {
            return Err(GfrStatus::ErrorInvalidInput);
        }
        let inputs: Vec<&[u8]> = unsafe { std::slice::from_raw_parts(keys, count) }
            .iter()
            .map(|b| unsafe { opt_slice(b) }.ok_or(GfrStatus::ErrorInvalidInput))
            .collect::<Result<_, _>>()?;

        let request = match protection {
            GfrKeypackProtection::PinFile => {
                let pin = unsafe { opt_slice(&pin) }.ok_or(GfrStatus::ErrorInvalidInput)?;
                ProtectionRequest::Pin(Zeroizing::new(pin.to_vec()))
            }
            GfrKeypackProtection::Plain => ProtectionRequest::Plain,
        };

        let written = keypack::write_package(&inputs, request, generated_at)?;

        let out = unsafe { &mut *out_result };
        out.header = header_c(&written.header);
        (out.package, out.package_len) = leak_bytes(written.package);
        if let Some(key_file) = written.key_file {
            (out.key_file, out.key_file_len) = leak_bytes(key_file);
        }
        if let Some(secret) = written.secret.as_ref() {
            out.plain_key = leak_string(&keypack::plain_key_text(secret))?;
        }
        Ok(())
    }))
}

/// The host's file, read through its callback.
struct HostReader {
    read: GfrKeypackReadCb,
    user_data: *mut c_void,
}

impl keypack::ReadAt for HostReader {
    fn read_at(&self, buf: &mut [u8], at: u64) -> std::io::Result<usize> {
        let n = (self.read)(self.user_data, at, buf.as_mut_ptr(), buf.len());
        usize::try_from(n)
            .ok()
            .filter(|&n| n <= buf.len())
            .ok_or_else(|| std::io::Error::other("the host could not read the key package"))
    }
}

fn sealed_from(
    read: GfrKeypackReadCb,
    user_data: *mut c_void,
) -> Result<keypack::SealedPackage, GfrStatus> {
    keypack::SealedPackage::from_reader(Box::new(HostReader { read, user_data }))
}

/// Read a key package's header without unlocking it; only the header is
/// read, through `read`. A file that is no key package version 3 gives
/// `ErrorInvalidData`.
#[unsafe(no_mangle)]
pub extern "C" fn gfr_keypack_read_header(
    read: GfrKeypackReadCb,
    user_data: *mut c_void,
    out_header: *mut GfrKeypackHeaderC,
) -> GfrStatus {
    clear_last_error();
    if out_header.is_null() {
        return GfrStatus::ErrorInvalidInput;
    }
    unsafe { std::ptr::write_bytes(out_header, 0, 1) };

    status_of(catch_unwind(|| {
        let sealed = sealed_from(read, user_data)?;
        unsafe { *out_header = header_c(sealed.header()) };
        Ok(())
    }))
}

/// An opened key package: its header and index are in memory, its keys are
/// read from the file one at a time. Close with `gfr_keypack_close`.
pub struct GfrKeypack {
    pack: keypack::OpenPackage,
    /// The primary of each key, in writing order.
    primaries: Vec<keypack::Component>,
}

/// Open a key package the host reads through `read`: read its header and
/// index, and nothing else. Every key read later goes through `read` too, so
/// the host keeps `user_data` and its file alive until `gfr_keypack_close`.
///
/// A PIN package needs `pin` and `key_file`; a plain package needs `secret`,
/// the plain key text (`gfkp3:` and 64 hex digits). `*out_header` is filled whenever the header
/// could be read. Close `*out_pack` with `gfr_keypack_close`.
#[unsafe(no_mangle)]
pub extern "C" fn gfr_keypack_open(
    read: GfrKeypackReadCb,
    user_data: *mut c_void,
    pin: GfrBuffer,
    key_file: GfrBuffer,
    secret: GfrBuffer,
    out_header: *mut GfrKeypackHeaderC,
    out_pack: *mut *mut GfrKeypack,
) -> GfrStatus {
    clear_last_error();
    if out_header.is_null() || out_pack.is_null() {
        return GfrStatus::ErrorInvalidInput;
    }
    unsafe {
        std::ptr::write_bytes(out_header, 0, 1);
        *out_pack = std::ptr::null_mut();
    }

    // The handle holds the host's reader; a panic leaves nothing half-done
    // that a caller could see, as every output is written last.
    status_of(catch_unwind(AssertUnwindSafe(|| {
        let sealed = sealed_from(read, user_data)?;
        unsafe { *out_header = header_c(sealed.header()) };

        let parsed_secret;
        let unlock = match unsafe { opt_slice(&key_file) } {
            Some(file) => Unlock::Pin {
                pin: unsafe { opt_slice(&pin) }.ok_or(GfrStatus::ErrorInvalidInput)?,
                key_file: file,
            },
            None => {
                let raw = unsafe { opt_slice(&secret) }.ok_or(GfrStatus::ErrorInvalidInput)?;
                let text = std::str::from_utf8(raw).map_err(|_| GfrStatus::ErrorInvalidInput)?;
                parsed_secret = keypack::parse_plain_key(text)?;
                Unlock::Secret(&parsed_secret)
            }
        };

        let pack = sealed.unlock(unlock)?;
        let primaries = pack.primaries()?;
        if primaries.len() != pack.header.key_count as usize {
            set_last_error("the key package index is damaged");
            return Err(GfrStatus::ErrorDataCorrupted);
        }
        unsafe { *out_pack = Box::into_raw(Box::new(GfrKeypack { pack, primaries })) };
        Ok(())
    })))
}

/// Read key `number` (from 0 to the header's `key_count`) of an opened
/// package: its binary secret key when the package carries it, else its
/// binary public certificate. Free `*out_key` with `gfr_keypack_free_key`
/// once it is used, whatever the status.
#[unsafe(no_mangle)]
pub extern "C" fn gfr_keypack_read_key(
    pack: *const GfrKeypack,
    number: u32,
    out_key: *mut GfrKeypackKeyC,
) -> GfrStatus {
    clear_last_error();
    if pack.is_null() || out_key.is_null() {
        return GfrStatus::ErrorInvalidInput;
    }
    unsafe { std::ptr::write_bytes(out_key, 0, 1) };

    // The handle holds the host's reader; a panic leaves nothing half-done
    // that a caller could see, as every output is written last.
    status_of(catch_unwind(AssertUnwindSafe(|| {
        let handle = unsafe { &*pack };
        let component = handle
            .primaries
            .get(number as usize)
            .ok_or(GfrStatus::ErrorInvalidInput)?;
        let entry = handle.pack.entry(component)?;
        let mut data = match handle.pack.secret_key(&entry)? {
            Some(secret) => secret,
            None => Zeroizing::new(entry.public_key.clone()),
        };
        let fpr = leak_string(&entry.fpr_hex())?;
        let user_id = leak_string(entry.primary_user_id()).unwrap_or(std::ptr::null_mut());
        let (key_data, key_data_len) = leak_bytes(std::mem::take(&mut *data));
        unsafe {
            *out_key = GfrKeypackKeyC {
                fpr,
                user_id,
                has_secret: entry.has_secret(),
                key_data,
                key_data_len,
            };
        }
        Ok(())
    })))
}

/// Free what `gfr_keypack_read_key` filled in. Null is a no-op.
#[unsafe(no_mangle)]
pub extern "C" fn gfr_keypack_free_key(key: *mut GfrKeypackKeyC) {
    if key.is_null() {
        return;
    }
    let k = unsafe { &mut *key };
    gfr_crypto_free_string(k.fpr);
    gfr_crypto_free_string(k.user_id);
    gfr_crypto_free_buffer(k.key_data, k.key_data_len);
    unsafe { std::ptr::write_bytes(key, 0, 1) };
}

/// Close an opened package. Null is a no-op.
#[unsafe(no_mangle)]
pub extern "C" fn gfr_keypack_close(pack: *mut GfrKeypack) {
    if !pack.is_null() {
        drop(unsafe { Box::from_raw(pack) });
    }
}

/// Free a `gfr_keypack_write` result. Null is a no-op.
#[unsafe(no_mangle)]
pub extern "C" fn gfr_keypack_free_write_result(result: *mut GfrKeypackWriteResultC) {
    if result.is_null() {
        return;
    }
    let r = unsafe { &mut *result };
    gfr_crypto_free_buffer(r.package, r.package_len);
    gfr_crypto_free_buffer(r.key_file, r.key_file_len);
    gfr_crypto_free_string(r.plain_key);
    unsafe { std::ptr::write_bytes(result, 0, 1) };
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::testutil::keys::{V4_SIGN, V6_SIGN};

    fn buf(s: &[u8]) -> GfrBuffer {
        GfrBuffer {
            data: s.as_ptr(),
            len: s.len(),
        }
    }

    fn slice<'a>(ptr: *const u8, len: usize) -> &'a [u8] {
        unsafe { std::slice::from_raw_parts(ptr, len) }
    }

    fn cstr<'a>(ptr: *const c_char) -> &'a str {
        unsafe { std::ffi::CStr::from_ptr(ptr) }.to_str().unwrap()
    }

    fn binary(armored: &str) -> Vec<u8> {
        crate::crypto::dearmor_all(armored.as_bytes()).into_owned()
    }

    fn write(protection: GfrKeypackProtection, pin: &[u8]) -> (GfrStatus, GfrKeypackWriteResultC) {
        let secret = binary(&V4_SIGN.secret_armored);
        let public = binary(&V6_SIGN.public_armored);
        let keys = [buf(&secret), buf(&public)];
        let mut out = unsafe { std::mem::zeroed::<GfrKeypackWriteResultC>() };
        let status = gfr_keypack_write(
            keys.as_ptr(),
            keys.len(),
            protection,
            buf(pin),
            42,
            &mut out,
        );
        (status, out)
    }

    /// A host's file: bytes it serves on request, and how many reads it got.
    struct HostFile {
        bytes: Vec<u8>,
        reads: std::cell::Cell<usize>,
        bytes_read: std::cell::Cell<usize>,
    }

    extern "C" fn host_read(user_data: *mut c_void, offset: u64, buf: *mut u8, len: usize) -> i64 {
        let file = unsafe { &*(user_data as *const HostFile) };
        file.reads.set(file.reads.get() + 1);
        let at = (offset as usize).min(file.bytes.len());
        let n = len.min(file.bytes.len() - at);
        unsafe { std::ptr::copy_nonoverlapping(file.bytes[at..].as_ptr(), buf, n) };
        file.bytes_read.set(file.bytes_read.get() + n);
        n as i64
    }

    extern "C" fn failing_read(_: *mut c_void, _: u64, _: *mut u8, _: usize) -> i64 {
        -1
    }

    fn host_file(bytes: &[u8]) -> Box<HostFile> {
        Box::new(HostFile {
            bytes: bytes.to_vec(),
            reads: 0.into(),
            bytes_read: 0.into(),
        })
    }

    fn to_file(written: &GfrKeypackWriteResultC) -> Box<HostFile> {
        host_file(slice(written.package, written.package_len))
    }

    fn ud(file: &HostFile) -> *mut c_void {
        file as *const HostFile as *mut c_void
    }

    fn read_header(file: &HostFile) -> (GfrStatus, GfrKeypackHeaderC) {
        let mut header = unsafe { std::mem::zeroed::<GfrKeypackHeaderC>() };
        let status = gfr_keypack_read_header(host_read, ud(file), &mut header);
        (status, header)
    }

    fn open(
        file: &HostFile,
        pin: &[u8],
        key_file: &[u8],
        secret: &[u8],
    ) -> (GfrStatus, GfrKeypackHeaderC, *mut GfrKeypack) {
        let mut header = unsafe { std::mem::zeroed::<GfrKeypackHeaderC>() };
        let mut pack = std::ptr::null_mut();
        let status = gfr_keypack_open(
            host_read,
            ud(file),
            buf(pin),
            buf(key_file),
            buf(secret),
            &mut header,
            &mut pack,
        );
        (status, header, pack)
    }

    #[test]
    fn a_plain_package_written_here_opens_here_with_its_text_key() {
        let (status, mut written) = write(GfrKeypackProtection::Plain, b"");
        assert_eq!(status, GfrStatus::Success);
        assert!(written.key_file.is_null());
        let text = cstr(written.plain_key).to_owned();
        assert!(text.starts_with(keypack::PLAIN_KEY_PREFIX));
        assert_eq!(written.header.protection, GfrKeypackProtection::Plain);
        let file = to_file(&written);

        let (status, header) = read_header(&file);
        assert_eq!(status, GfrStatus::Success);
        assert_eq!(header.generated_at, 42);
        assert_eq!(header.version, 3);
        assert_eq!(header.key_count, 2);
        assert!(header.secrets_unprotected);
        assert_eq!(header.package_id, written.header.package_id);

        file.bytes_read.set(0);
        let (status, header, pack) = open(&file, b"", b"", text.as_bytes());
        assert_eq!(status, GfrStatus::Success);
        assert_eq!(header.key_count, 2);
        // Header and index only: no key block was read.
        let opened_with = file.bytes_read.get();
        assert!(
            opened_with < file.bytes.len() / 2,
            "{opened_with} of {}",
            file.bytes.len()
        );

        let mut key = unsafe { std::mem::zeroed::<GfrKeypackKeyC>() };
        assert_eq!(gfr_keypack_read_key(pack, 0, &mut key), GfrStatus::Success);
        assert_eq!(cstr(key.fpr), V4_SIGN.primary_fpr);
        assert!(key.has_secret);
        let data = slice(key.key_data, key.key_data_len);
        assert_eq!(data[0] & 0x3f, 5, "a binary secret key packet comes first");
        gfr_keypack_free_key(&mut key);
        assert!(key.key_data.is_null());

        assert_eq!(gfr_keypack_read_key(pack, 1, &mut key), GfrStatus::Success);
        assert!(!key.has_secret);
        let data = slice(key.key_data, key.key_data_len);
        assert_eq!(data[0] & 0x3f, 6, "a binary public key packet comes first");
        gfr_keypack_free_key(&mut key);

        assert_eq!(
            gfr_keypack_read_key(pack, 2, &mut key),
            GfrStatus::ErrorInvalidInput
        );
        gfr_keypack_free_key(&mut key);

        gfr_keypack_close(pack);
        gfr_keypack_free_write_result(&mut written);
        assert!(written.package.is_null());
    }

    #[test]
    fn a_pin_package_needs_its_pin_and_key_file() {
        let (status, mut written) = write(GfrKeypackProtection::PinFile, b"1234");
        assert_eq!(status, GfrStatus::Success);
        assert!(!written.key_file.is_null());
        let key_file = slice(written.key_file, written.key_file_len);
        let file = to_file(&written);

        let (status, _, pack) = open(&file, b"1234", key_file, b"");
        assert_eq!(status, GfrStatus::Success);
        gfr_keypack_close(pack);

        let (status, header, pack) = open(&file, b"9999", key_file, b"");
        assert_eq!(status, GfrStatus::ErrorWrongPassword);
        assert!(pack.is_null());
        // The header is read before the unlock fails.
        assert_eq!(header.protection, GfrKeypackProtection::PinFile);
        gfr_keypack_free_write_result(&mut written);
    }

    #[test]
    fn other_files_are_not_packages() {
        let file = host_file(b"GFSEC2 a version 2 key package");
        assert_eq!(read_header(&file).0, GfrStatus::ErrorInvalidData);
        assert_eq!(read_header(&host_file(b"")).0, GfrStatus::ErrorInvalidData);

        let mut header = unsafe { std::mem::zeroed::<GfrKeypackHeaderC>() };
        assert_eq!(
            gfr_keypack_read_header(failing_read, std::ptr::null_mut(), &mut header),
            GfrStatus::ErrorIo
        );
    }

    #[test]
    fn a_pin_package_without_a_pin_is_refused() {
        let (status, mut written) = write(GfrKeypackProtection::PinFile, b"");
        assert_eq!(status, GfrStatus::ErrorInvalidInput);
        assert!(written.package.is_null());
        gfr_keypack_free_write_result(&mut written);
    }

    #[test]
    fn armored_keys_are_refused() {
        let keys = [buf(V4_SIGN.secret_armored.as_bytes())];
        let mut out = unsafe { std::mem::zeroed::<GfrKeypackWriteResultC>() };
        let status = gfr_keypack_write(
            keys.as_ptr(),
            keys.len(),
            GfrKeypackProtection::Plain,
            GfrBuffer::empty(),
            0,
            &mut out,
        );
        assert_eq!(status, GfrStatus::ErrorInvalidData);
        gfr_keypack_free_write_result(&mut out);
    }

    #[test]
    fn null_arguments_are_refused() {
        assert_eq!(
            gfr_keypack_write(
                std::ptr::null(),
                0,
                GfrKeypackProtection::Plain,
                GfrBuffer::empty(),
                0,
                std::ptr::null_mut()
            ),
            GfrStatus::ErrorInvalidInput
        );
        assert_eq!(
            gfr_keypack_read_header(host_read, std::ptr::null_mut(), std::ptr::null_mut()),
            GfrStatus::ErrorInvalidInput
        );
        let mut key = unsafe { std::mem::zeroed::<GfrKeypackKeyC>() };
        assert_eq!(
            gfr_keypack_read_key(std::ptr::null(), 0, &mut key),
            GfrStatus::ErrorInvalidInput
        );
        gfr_keypack_free_write_result(std::ptr::null_mut());
        gfr_keypack_free_key(std::ptr::null_mut());
        gfr_keypack_close(std::ptr::null_mut());
    }

    #[test]
    fn removing_a_password_through_the_ffi_returns_an_open_binary_key() {
        let secret = binary(&V4_SIGN.secret_armored);
        let mut out = std::ptr::null_mut();
        let mut out_len = 0;
        let status = gfr_keypack_remove_key_password(
            0,
            buf(&secret),
            crate::testutil::cb::pwd_correct,
            &mut out,
            &mut out_len,
        );
        assert_eq!(status, GfrStatus::Success);
        let key = slice(out, out_len);
        assert_eq!(key[0] & 0x3f, 5);
        gfr_crypto_free_buffer(out, out_len);
    }
}
