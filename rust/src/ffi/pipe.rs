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

//! gfpipe's one entry point. The gfpipe executable is a few lines of C++ that
//! supply what needs the platform and call this.

use std::collections::HashMap;
use std::ffi::{CStr, OsString, c_char};
use std::panic::catch_unwind;
use std::path::Path;

use crate::pipe::{self, ENV_KEY, ENV_KEY_FILE, ENV_KEYPACK, Host, Io};

fn c_string(ptr: *const c_char) -> String {
    if ptr.is_null() {
        return String::new();
    }
    unsafe { CStr::from_ptr(ptr) }
        .to_string_lossy()
        .into_owned()
}

/// An argument as the system gave it: on Unix the bytes as they are, so a
/// file name that is not UTF-8 still opens.
#[cfg(unix)]
fn os_arg(ptr: *const c_char) -> OsString {
    use std::os::unix::ffi::OsStringExt;
    if ptr.is_null() {
        return OsString::new();
    }
    OsString::from_vec(unsafe { CStr::from_ptr(ptr) }.to_bytes().to_vec())
}

#[cfg(not(unix))]
fn os_arg(ptr: *const c_char) -> OsString {
    OsString::from(c_string(ptr))
}

/// The arguments after the program name. On Windows the C `argv` is in the
/// ANSI code page, so the process's own wide arguments are taken when they
/// are the same in number.
fn arguments(argc: i32, argv: *const *const c_char) -> Vec<OsString> {
    let given: Vec<OsString> = if argv.is_null() || argc <= 1 {
        Vec::new()
    } else {
        (1..argc as usize)
            .map(|i| os_arg(unsafe { *argv.add(i) }))
            .collect()
    };
    if cfg!(windows) {
        let wide: Vec<OsString> = std::env::args_os().skip(1).collect();
        if wide.len() == given.len() {
            return wide;
        }
    }
    given
}

/// Standard output for data. On Windows, Rust's console stream takes only
/// UTF-8 and refuses other bytes, so decrypted binary data written to a
/// console would fail; the handle is written as it is there, as gpg writes it.
#[cfg(windows)]
fn standard_output() -> Box<dyn std::io::Write + Send + Sync> {
    use std::io::{IsTerminal, Write};
    use std::os::windows::io::{AsRawHandle, FromRawHandle};

    struct Raw(std::mem::ManuallyDrop<std::fs::File>);
    impl Write for Raw {
        fn write(&mut self, buf: &[u8]) -> std::io::Result<usize> {
            self.0.write(buf)
        }
        fn flush(&mut self) -> std::io::Result<()> {
            self.0.flush()
        }
    }

    let stdout = std::io::stdout();
    if !stdout.is_terminal() {
        return Box::new(stdout);
    }
    let handle = stdout.as_raw_handle();
    // SAFETY: the process's own standard output handle, open for the whole
    // run; ManuallyDrop keeps it from being closed here.
    let file = unsafe { std::fs::File::from_raw_handle(handle) };
    Box::new(Raw(std::mem::ManuallyDrop::new(file)))
}

#[cfg(not(windows))]
fn standard_output() -> Box<dyn std::io::Write + Send + Sync> {
    Box::new(std::io::stdout())
}

/// Run gfpipe with the process's `argc`/`argv` (program name included) on its
/// standard streams. Returns the exit code.
///
/// `version` is the GpgFrontend version gfpipe comes with. The package and
/// its key are looked for beside the executable unless they are named.
#[unsafe(no_mangle)]
pub extern "C" fn gfr_pipe_run(
    argc: i32,
    argv: *const *const c_char,
    version: *const c_char,
) -> i32 {
    let result = catch_unwind(|| {
        let args = arguments(argc, argv);
        let program = if argv.is_null() || argc < 1 {
            String::new()
        } else {
            c_string(unsafe { *argv })
        };

        let env: HashMap<String, String> = [ENV_KEYPACK, ENV_KEY, ENV_KEY_FILE]
            .iter()
            .filter_map(|name| std::env::var(name).ok().map(|v| (name.to_string(), v)))
            .collect();

        let host = Host {
            program,
            program_dir: std::env::current_exe()
                .ok()
                .and_then(|exe| exe.parent().map(Path::to_path_buf)),
            version: c_string(version),
            env,
        };
        let mut io = Io {
            stdin: Box::new(std::io::stdin()),
            stdout: Box::new(std::io::BufWriter::new(standard_output())),
            stderr: Box::new(std::io::stderr()),
        };
        pipe::run(&args, &host, &mut io)
    });
    result.unwrap_or_else(|_| {
        eprintln!("gfpipe: internal error");
        2
    })
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn help_runs_through_the_entry_point() {
        let program = std::ffi::CString::new("gfpipe").unwrap();
        let version = std::ffi::CString::new("--version").unwrap();
        let argv = [program.as_ptr(), version.as_ptr()];
        assert_eq!(gfr_pipe_run(2, argv.as_ptr(), std::ptr::null()), 0);
    }

    #[test]
    fn a_missing_package_is_an_error_not_a_crash() {
        let program = std::ffi::CString::new("gfpipe").unwrap();
        let keys = std::ffi::CString::new("keys").unwrap();
        let argv = [program.as_ptr(), keys.as_ptr()];
        // Nothing lies beside the test program.
        assert_eq!(gfr_pipe_run(2, argv.as_ptr(), std::ptr::null()), 2);
    }
}
