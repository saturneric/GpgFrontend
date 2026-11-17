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
use std::ffi::{CStr, c_char};
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
        let args: Vec<String> = if argv.is_null() || argc <= 1 {
            Vec::new()
        } else {
            (1..argc as usize)
                .map(|i| c_string(unsafe { *argv.add(i) }))
                .collect()
        };

        let env: HashMap<String, String> = [ENV_KEYPACK, ENV_KEY, ENV_KEY_FILE]
            .iter()
            .filter_map(|name| std::env::var(name).ok().map(|v| (name.to_string(), v)))
            .collect();

        let host = Host {
            program_dir: std::env::current_exe()
                .ok()
                .and_then(|exe| exe.parent().map(Path::to_path_buf)),
            version: c_string(version),
            env,
        };
        let mut io = Io {
            stdin: Box::new(std::io::stdin()),
            stdout: Box::new(std::io::BufWriter::new(std::io::stdout())),
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
