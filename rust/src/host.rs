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
