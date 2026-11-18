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

//! What gfpipe says, in gpg's words: the lines on standard error, the error
//! codes of its status lines, and the dates and names in them.

use std::cell::RefCell;
use std::ffi::OsStr;
use std::io::Write;

thread_local! {
    /// The name gfpipe was started by: gpg's own when it stands in for gpg,
    /// so tools that read gpg's lines find them.
    static PROGRAM: RefCell<String> = RefCell::new(DEFAULT_PROGRAM.to_string());
}

const DEFAULT_PROGRAM: &str = "gfpipe";

/// The name messages start with: the program file's, without `.exe`.
pub fn set_program(argv0: &str) {
    let base = std::path::Path::new(argv0)
        .file_name()
        .and_then(OsStr::to_str)
        .unwrap_or_default();
    let base = base
        .strip_suffix(".exe")
        .or_else(|| base.strip_suffix(".EXE"))
        .unwrap_or(base);
    let name = if base.is_empty() {
        DEFAULT_PROGRAM
    } else {
        base
    };
    PROGRAM.with(|p| *p.borrow_mut() = name.to_string());
}

pub fn program() -> String {
    PROGRAM.with(|p| p.borrow().clone())
}

/// One line on `w`, after the program's name, as gpg writes its own.
pub fn line(w: &mut dyn Write, message: impl std::fmt::Display) {
    let _ = writeln!(w, "{}: {message}", program());
}

/// A file name as gpg quotes it.
pub fn quote(name: &OsStr) -> String {
    format!("'{}'", name.to_string_lossy())
}

/// What an I/O error says, without Rust's " (os error N)".
pub fn io_reason(e: &std::io::Error) -> String {
    let text = e.to_string();
    match text.rfind(" (os error ") {
        Some(at) if text.ends_with(')') => text[..at].to_string(),
        _ => text,
    }
}

/// gpg-error codes, as gpg's status lines carry them.
pub mod code {
    pub const GENERAL: u32 = 1;
    pub const BAD_SIGNATURE: u32 = 8;
    pub const NO_PUBKEY: u32 = 9;
    pub const BAD_PASSPHRASE: u32 = 11;
    pub const NO_USER_ID: u32 = 16;
    pub const INV_PASSPHRASE: u32 = 31;
    pub const NO_SECKEY: u32 = 17;
    pub const UNUSABLE_PUBKEY: u32 = 53;
    pub const UNUSABLE_SECKEY: u32 = 54;
    pub const NO_DATA: u32 = 58;
    pub const BAD_DATA: u32 = 89;
    pub const AMBIGUOUS_NAME: u32 = 107;
    pub const DECRYPT_FAILED: u32 = 152;
    /// What gpg reports for input that is not OpenPGP at all.
    pub const UNKNOWN: u32 = u32::MAX;

    const SYSTEM_ERROR: u32 = 1 << 15;
    /// gpg's error source, which gpg puts on most codes.
    const SOURCE_GPG: u32 = 2 << 24;

    /// `code` with gpg as its source.
    pub fn gpg(code: u32) -> u32 {
        SOURCE_GPG | code
    }

    /// The code `gpg-exit` ends a failed run with.
    pub fn exit() -> u32 {
        gpg(GENERAL)
    }

    /// The gpg-error code of a system error, with gpg as its source.
    pub fn system(e: &std::io::Error) -> u32 {
        use std::io::ErrorKind;
        #[cfg(unix)]
        let by_errno = e.raw_os_error().and_then(|n| {
            Some(match n {
                libc::EACCES => 1,
                libc::EEXIST => 35,
                libc::EINVAL => 48,
                libc::EIO => 49,
                libc::EISDIR => 51,
                libc::ELOOP => 63,
                libc::ENAMETOOLONG => 69,
                libc::ENOENT => 81,
                libc::ENOSPC => 91,
                libc::ENOTDIR => 97,
                libc::EPERM => 107,
                libc::EPIPE => 109,
                libc::EROFS => 122,
                _ => return None,
            })
        });
        #[cfg(not(unix))]
        let by_errno: Option<u32> = None;
        let index = by_errno.unwrap_or(match e.kind() {
            ErrorKind::NotFound => 81,
            ErrorKind::AlreadyExists => 35,
            ErrorKind::PermissionDenied => 1,
            ErrorKind::BrokenPipe => 109,
            ErrorKind::InvalidInput => 48,
            _ => 49,
        });
        gpg(SYSTEM_ERROR | index)
    }

    /// gpg's words for a code.
    pub fn reason(code: u32) -> &'static str {
        match code & 0xffff {
            BAD_SIGNATURE => "Bad signature",
            NO_PUBKEY => "No public key",
            BAD_PASSPHRASE => "Bad passphrase",
            NO_USER_ID => "No user ID",
            INV_PASSPHRASE => "Invalid passphrase",
            NO_SECKEY => "No secret key",
            UNUSABLE_PUBKEY => "Unusable public key",
            UNUSABLE_SECKEY => "Unusable secret key",
            NO_DATA => "No data",
            BAD_DATA => "Bad data",
            AMBIGUOUS_NAME => "Ambiguous name",
            DECRYPT_FAILED => "Decryption failed",
            _ => "General error",
        }
    }
}

/// gpg's name for a public-key algorithm, as in "using EDDSA key".
pub fn pk_algo_name(id: u8) -> &'static str {
    match id {
        1..=3 => "RSA",
        16 | 20 => "ELG",
        17 => "DSA",
        18 => "ECDH",
        19 => "ECDSA",
        22 => "EDDSA",
        25 => "X25519",
        26 => "X448",
        27 => "ED25519",
        28 => "ED448",
        _ => "?",
    }
}

/// gpg's validity of a key in a listing or after "Good signature from".
pub fn validity_tag(revoked: bool, expired: bool, own: bool) -> &'static str {
    if revoked {
        "[ revoked]"
    } else if expired {
        "[ expired]"
    } else if own {
        "[ultimate]"
    } else {
        "[  full  ]"
    }
}

/// The same validity, as one word: `Good signature from "x" [full]`.
pub fn validity_word(revoked: bool, expired: bool, own: bool) -> &'static str {
    if revoked {
        "[revoked]"
    } else if expired {
        "[expired]"
    } else if own {
        "[ultimate]"
    } else {
        "[full]"
    }
}

/// A broken-down time.
struct Civil {
    year: i64,
    month: u32,
    day: u32,
    weekday: u32,
    hour: u32,
    minute: u32,
    second: u32,
    zone: String,
}

const WEEKDAYS: [&str; 7] = ["Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"];
const MONTHS: [&str; 12] = [
    "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
];

/// A Unix time as gpg writes it after "Signature made", in local time where
/// the platform says what it is: "Wed 22 Jun 2016 10:00:38 AM CEST".
pub fn local_time(unix: u32) -> String {
    let t = civil_local(unix).unwrap_or_else(|| civil_utc(unix));
    let (hour12, meridiem) = match t.hour {
        0 => (12, "AM"),
        1..=11 => (t.hour, "AM"),
        12 => (12, "PM"),
        h => (h - 12, "PM"),
    };
    format!(
        "{} {:02} {} {} {:02}:{:02}:{:02} {meridiem} {}",
        WEEKDAYS[t.weekday as usize % 7],
        t.day,
        MONTHS[(t.month as usize).saturating_sub(1) % 12],
        t.year,
        hour12,
        t.minute,
        t.second,
        t.zone
    )
}

fn civil_utc(unix: u32) -> Civil {
    let secs = i64::from(unix);
    let days = secs.div_euclid(86_400);
    let rem = secs.rem_euclid(86_400);
    // Howard Hinnant's civil-from-days.
    let z = days + 719_468;
    let era = z.div_euclid(146_097);
    let doe = z - era * 146_097;
    let yoe = (doe - doe / 1460 + doe / 36_524 - doe / 146_096) / 365;
    let doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    let mp = (5 * doy + 2) / 153;
    let day = (doy - (153 * mp + 2) / 5 + 1) as u32;
    let month = if mp < 10 { mp + 3 } else { mp - 9 } as u32;
    let year = yoe + era * 400 + i64::from(month <= 2);
    Civil {
        year,
        month,
        day,
        // 1970-01-01 was a Thursday.
        weekday: (days + 4).rem_euclid(7) as u32,
        hour: (rem / 3600) as u32,
        minute: (rem / 60 % 60) as u32,
        second: (rem % 60) as u32,
        zone: "UTC".into(),
    }
}

#[cfg(unix)]
fn civil_local(unix: u32) -> Option<Civil> {
    let t: libc::time_t = libc::time_t::from(unix);
    // SAFETY: localtime_r writes into the tm it is given and nothing else.
    let mut tm: libc::tm = unsafe { std::mem::zeroed() };
    if unsafe { libc::localtime_r(&t, &mut tm) }.is_null() {
        return None;
    }
    let zone = if tm.tm_zone.is_null() {
        String::new()
    } else {
        // SAFETY: tm_zone points at a static zone abbreviation.
        unsafe { std::ffi::CStr::from_ptr(tm.tm_zone) }
            .to_string_lossy()
            .into_owned()
    };
    Some(Civil {
        year: i64::from(tm.tm_year) + 1900,
        month: u32::try_from(tm.tm_mon + 1).ok()?,
        day: u32::try_from(tm.tm_mday).ok()?,
        weekday: u32::try_from(tm.tm_wday).ok()?,
        hour: u32::try_from(tm.tm_hour).ok()?,
        minute: u32::try_from(tm.tm_min).ok()?,
        second: u32::try_from(tm.tm_sec).ok()?,
        zone,
    })
}

#[cfg(not(unix))]
fn civil_local(_unix: u32) -> Option<Civil> {
    None
}

/// A fingerprint in gpg's groups of four, with a wider gap in the middle.
pub fn grouped_fingerprint(hex: &str) -> String {
    let groups: Vec<&str> = hex
        .as_bytes()
        .chunks(4)
        .map(|c| std::str::from_utf8(c).unwrap_or_default())
        .collect();
    let half = groups.len() / 2;
    let mut out = String::new();
    for (i, g) in groups.iter().enumerate() {
        if i > 0 {
            out.push_str(if i == half && hex.len() == 40 {
                "  "
            } else {
                " "
            });
        }
        out.push_str(g);
    }
    out
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn the_program_name_is_the_files_without_exe() {
        set_program("/usr/local/bin/gpg");
        assert_eq!(program(), "gpg");
        set_program("C:\\Tools\\gfpipe.exe");
        assert!(program() == "gfpipe" || program() == "C:\\Tools\\gfpipe");
        set_program("");
        assert_eq!(program(), "gfpipe");
    }

    #[test]
    fn io_errors_read_as_gpgs() {
        let e = std::io::Error::from_raw_os_error(2);
        assert!(!io_reason(&e).contains("os error"));
        #[cfg(unix)]
        assert_eq!(code::system(&e), 33_587_281);
        let e = std::io::Error::from(std::io::ErrorKind::AlreadyExists);
        assert_eq!(code::system(&e), 33_587_235);
        assert_eq!(code::exit(), 33_554_433);
        assert_eq!(code::gpg(code::NO_SECKEY), 33_554_449);
        assert_eq!(code::reason(code::gpg(code::NO_SECKEY)), "No secret key");
    }

    #[test]
    fn times_and_fingerprints_read_as_gpgs() {
        let t = civil_utc(1_466_582_438);
        assert_eq!((t.year, t.month, t.day, t.weekday), (2016, 6, 22, 3));
        assert_eq!((t.hour, t.minute, t.second), (8, 0, 38));
        assert!(local_time(1_466_582_438).contains("2016"));
        assert_eq!(
            grouped_fingerprint("D1A66E1A23B182C9980F788CFBFCC82A015E7330"),
            "D1A6 6E1A 23B1 82C9 980F  788C FBFC C82A 015E 7330"
        );
    }
}
