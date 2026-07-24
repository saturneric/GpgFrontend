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

//! Host environment utilities from gpgfrontend core

use std::ffi::c_char;

unsafe extern "C" {
    pub fn gfc_secure_free_cstr(ptr: *mut c_char);

    /// Securely free a length-delimited byte buffer the host handed us (e.g. the
    /// passphrase bytes from the password-fetch callback). The length is passed
    /// explicitly, so freeing never depends on a NUL terminator and can never
    /// read past the allocation — unlike [`gfc_secure_free_cstr`], which scans
    /// with `strlen` and must only be used on genuine C strings.
    pub fn gfc_secure_free_buffer(ptr: *mut u8, len: usize);
}

#[cfg(test)]
mod host_tests {
    //! The host free-routine contract.
    //!
    //! In a production build these symbols come from the C++ core; in the test
    //! binary they are provided once by `crate::testutil`. What is testable
    //! here is the *contract*: both must tolerate null, `gfc_secure_free_cstr`
    //! must reclaim a `CString::into_raw` allocation, and
    //! `gfc_secure_free_buffer` must reclaim a length-delimited one.
    //!
    //! The split matters: `gfc_secure_free_cstr` recovers the length with
    //! `strlen`, so calling it on a non-NUL-terminated passphrase buffer reads
    //! past the allocation. That mistake previously produced a
    //! `realloc(): invalid next size` abort, which is why the length-aware
    //! variant exists.

    use super::*;
    use crate::testutil::leak_as_c_buffer;
    use std::ffi::CString;

    #[test]
    fn freeing_a_null_c_string_is_a_no_op() {
        unsafe { gfc_secure_free_cstr(std::ptr::null_mut()) };
    }

    #[test]
    fn freeing_a_null_buffer_is_a_no_op() {
        unsafe { gfc_secure_free_buffer(std::ptr::null_mut(), 0) };
        unsafe { gfc_secure_free_buffer(std::ptr::null_mut(), 64) };
    }

    #[test]
    fn a_c_string_round_trips_through_the_free_routine() {
        let s = CString::new("a passphrase-shaped string").expect("no interior NUL");
        let raw = s.into_raw();
        assert!(!raw.is_null());
        unsafe { gfc_secure_free_cstr(raw) };
    }

    #[test]
    fn a_length_delimited_buffer_round_trips_through_the_free_routine() {
        // Deliberately *not* NUL-terminated and containing an interior zero:
        // this is the shape that breaks `strlen`-based freeing and the reason
        // the passphrase path uses the length-aware routine.
        let (ptr, len) = leak_as_c_buffer(&[0x01, 0x00, 0xFF, 0x7F]);
        assert_eq!(len, 4);
        unsafe { gfc_secure_free_buffer(ptr, len) };
    }

    #[test]
    fn a_zero_length_buffer_is_tolerated() {
        let (ptr, len) = leak_as_c_buffer(&[]);
        assert_eq!(len, 0);
        // A zero-length boxed slice has a dangling-but-aligned pointer and owns
        // no allocation, so declining to free it is correct.
        unsafe { gfc_secure_free_buffer(ptr, len) };
    }

    #[test]
    fn an_empty_c_string_round_trips() {
        let raw = CString::new("").expect("empty is valid").into_raw();
        unsafe { gfc_secure_free_cstr(raw) };
    }

    #[test]
    fn a_large_buffer_round_trips() {
        let (ptr, len) = leak_as_c_buffer(&vec![0xAAu8; 1 << 16]);
        assert_eq!(len, 1 << 16);
        unsafe { gfc_secure_free_buffer(ptr, len) };
    }
}
