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

//! A read-only map of a whole package file. Every process that maps the same
//! file reads the same pages of the system's file cache, so a thousand
//! readers of one package hold its bytes once, and a read is a borrow, with
//! no copy and no system call.
//!
//! The file must not shrink while it is mapped: reading a page past its new
//! end stops the process (SIGBUS on Unix). GpgFrontend writes a package to a
//! new file and renames it over the old one (`WriteFileGFBuffer`), so a
//! reader keeps the file it opened. On Windows, a file that is mapped cannot
//! be replaced, so a new export waits until the readers are done.

use std::fs::File;
use std::io;

pub(super) struct Mapped {
    ptr: *const u8,
    len: usize,
}

impl Mapped {
    /// Map `file`, when it is a regular file that is not empty; `None` for
    /// anything else (a pipe, a device), which is read as it comes.
    pub(super) fn new(file: &File) -> io::Result<Option<Mapped>> {
        let meta = file.metadata()?;
        if !meta.file_type().is_file() || meta.len() == 0 {
            return Ok(None);
        }
        let len = usize::try_from(meta.len()).map_err(|_| io::Error::other("file too large"))?;
        map(file, len).map(|ptr| Some(Mapped { ptr, len }))
    }

    pub(super) fn bytes(&self) -> &[u8] {
        // SAFETY: `ptr` maps `len` readable bytes until `self` is dropped.
        unsafe { std::slice::from_raw_parts(self.ptr, self.len) }
    }
}

#[cfg(unix)]
fn map(file: &File, len: usize) -> io::Result<*const u8> {
    use std::os::fd::AsRawFd;

    // SAFETY: a read-only shared map of an open file descriptor; the map
    // outlives the descriptor, which may be closed.
    let ptr = unsafe {
        libc::mmap(
            std::ptr::null_mut(),
            len,
            libc::PROT_READ,
            libc::MAP_SHARED,
            file.as_raw_fd(),
            0,
        )
    };
    if ptr == libc::MAP_FAILED {
        return Err(io::Error::last_os_error());
    }
    // A lookup reads a few pages here and there: read no more than it asks.
    // SAFETY: the range was just mapped; advice never changes its content.
    unsafe { libc::madvise(ptr, len, libc::MADV_RANDOM) };
    Ok(ptr as *const u8)
}

#[cfg(windows)]
fn map(file: &File, _len: usize) -> io::Result<*const u8> {
    use std::os::windows::io::AsRawHandle;
    use windows_sys::Win32::Foundation::CloseHandle;
    use windows_sys::Win32::System::Memory::{
        CreateFileMappingW, FILE_MAP_READ, MapViewOfFile, PAGE_READONLY,
    };

    // SAFETY: a read-only mapping of an open file handle, the whole file;
    // the view keeps the mapping object alive once its handle is closed.
    unsafe {
        let mapping = CreateFileMappingW(
            file.as_raw_handle(),
            std::ptr::null(),
            PAGE_READONLY,
            0,
            0,
            std::ptr::null(),
        );
        if mapping.is_null() {
            return Err(io::Error::last_os_error());
        }
        let view = MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0);
        let error = io::Error::last_os_error();
        CloseHandle(mapping);
        if view.Value.is_null() {
            return Err(error);
        }
        Ok(view.Value as *const u8)
    }
}

impl Drop for Mapped {
    fn drop(&mut self) {
        // SAFETY: unmaps exactly what `map` mapped, once.
        #[cfg(unix)]
        unsafe {
            libc::munmap(self.ptr as *mut libc::c_void, self.len);
        }
        #[cfg(windows)]
        unsafe {
            use windows_sys::Win32::System::Memory::{MEMORY_MAPPED_VIEW_ADDRESS, UnmapViewOfFile};
            UnmapViewOfFile(MEMORY_MAPPED_VIEW_ADDRESS {
                Value: self.ptr as *mut std::ffi::c_void,
            });
        }
    }
}
