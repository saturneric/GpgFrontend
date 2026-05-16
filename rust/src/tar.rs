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

//! Tar archive helpers for directory encryption and decryption workflows.

use std::{
    fs::File, io::{Seek, SeekFrom}, path::Path
};

use crate::{err::set_last_error, types::GfrStatus};

/// Pack a directory into an anonymous temporary tar archive.
///
/// Returns `(tempfile, filename_hint)` where `tempfile` is already seeked back
/// to offset 0, ready for the caller to read. The temp file is deleted when
/// the `File` handle is dropped. The `{}` inner scope around the builder
/// ensures the tar trailer is flushed before the seek.
pub fn build_tar_tempfile_from_directory(in_dir_path: &str) -> Result<(File, String), GfrStatus> {
    let dir_path = Path::new(in_dir_path);
    if !dir_path.is_dir() {
        log::error!("Input path is not a directory: {}", in_dir_path);
        return Err(GfrStatus::ErrorInvalidInput);
    }

    let dir_name = dir_path
        .file_name()
        .unwrap_or_default()
        .to_string_lossy()
        .to_string();
    let filename_hint = format!("{}.tar", dir_name);

    let mut temp_archive = tempfile::tempfile().map_err(|e| {
        log::error!("Failed to create temp file for tar: {}", e);
        set_last_error(&e.to_string());
        GfrStatus::ErrorIo
    })?;

    {
        let mut tar_builder = tar::Builder::new(&mut temp_archive);

        tar_builder.append_dir_all(".", dir_path).map_err(|e| {
            log::error!("Failed to build tar archive: {}", e);
            set_last_error(&e.to_string());
            GfrStatus::ErrorIo
        })?;

        tar_builder.into_inner().map_err(|e| {
            log::error!("Failed to finalize tar archive: {}", e);
            set_last_error(&e.to_string());
            GfrStatus::ErrorIo
        })?;
    }

    temp_archive.seek(SeekFrom::Start(0)).map_err(|e| {
        log::error!("Failed to rewind temp tar archive: {}", e);
        set_last_error(&e.to_string());
        GfrStatus::ErrorIo
    })?;

    Ok((temp_archive, filename_hint))
}
