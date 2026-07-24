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
    fs::File,
    io::{Seek, SeekFrom},
    path::Path,
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

#[cfg(test)]
mod tar_tests {
    //! Directory packing for the `*_directory` encrypt entry points.
    //!
    //! Everything happens inside a `tempfile::tempdir()` so the tests stay
    //! parallel-safe and leave nothing behind.

    use super::*;
    use std::io::Read;

    /// Read the whole archive back and return its entry paths.
    fn entries_of(mut archive: File) -> Vec<String> {
        let mut bytes = Vec::new();
        archive.read_to_end(&mut bytes).expect("read archive");
        let mut ar = tar::Archive::new(std::io::Cursor::new(bytes));
        ar.entries()
            .expect("entries")
            .map(|e| {
                e.expect("entry")
                    .path()
                    .expect("path")
                    .to_string_lossy()
                    .into_owned()
            })
            .collect()
    }

    fn write(dir: &Path, rel: &str, contents: &[u8]) {
        let full = dir.join(rel);
        if let Some(parent) = full.parent() {
            std::fs::create_dir_all(parent).expect("mkdir -p");
        }
        std::fs::write(full, contents).expect("write");
    }

    #[test]
    fn a_missing_path_is_invalid_input() {
        let res = build_tar_tempfile_from_directory("/nonexistent/definitely/not/here");
        assert_eq!(res.err(), Some(GfrStatus::ErrorInvalidInput));
    }

    #[test]
    fn a_regular_file_is_invalid_input() {
        // The entry point is for directories; handing it a file must be
        // rejected up front rather than producing an empty archive.
        let dir = tempfile::tempdir().expect("tempdir");
        write(dir.path(), "a.txt", b"x");
        let file = dir.path().join("a.txt");
        let res = build_tar_tempfile_from_directory(&file.to_string_lossy());
        assert_eq!(res.err(), Some(GfrStatus::ErrorInvalidInput));
    }

    #[test]
    fn an_empty_string_path_is_invalid_input() {
        assert_eq!(
            build_tar_tempfile_from_directory("").err(),
            Some(GfrStatus::ErrorInvalidInput)
        );
    }

    #[test]
    fn an_empty_directory_produces_a_readable_archive() {
        let dir = tempfile::tempdir().expect("tempdir");
        let (archive, _hint) =
            build_tar_tempfile_from_directory(&dir.path().to_string_lossy()).expect("pack");
        // Only the "." root entry, or nothing at all -- either way it parses.
        let entries = entries_of(archive);
        assert!(entries.len() <= 1, "{entries:?}");
    }

    #[test]
    fn a_single_file_round_trips() {
        let dir = tempfile::tempdir().expect("tempdir");
        write(dir.path(), "hello.txt", b"world");
        let (archive, _hint) =
            build_tar_tempfile_from_directory(&dir.path().to_string_lossy()).expect("pack");
        let entries = entries_of(archive);
        assert!(
            entries.iter().any(|e| e.ends_with("hello.txt")),
            "{entries:?}"
        );
    }

    #[test]
    fn nested_directories_are_included() {
        let dir = tempfile::tempdir().expect("tempdir");
        write(dir.path(), "a/b/c/deep.txt", b"deep");
        let (archive, _hint) =
            build_tar_tempfile_from_directory(&dir.path().to_string_lossy()).expect("pack");
        let entries = entries_of(archive);
        assert!(
            entries.iter().any(|e| e.contains("deep.txt")),
            "{entries:?}"
        );
    }

    #[test]
    fn every_file_is_included() {
        let dir = tempfile::tempdir().expect("tempdir");
        for i in 0..10 {
            write(dir.path(), &format!("f{i}.bin"), &[i as u8; 32]);
        }
        let (archive, _hint) =
            build_tar_tempfile_from_directory(&dir.path().to_string_lossy()).expect("pack");
        let entries = entries_of(archive);
        for i in 0..10 {
            assert!(
                entries.iter().any(|e| e.ends_with(&format!("f{i}.bin"))),
                "f{i}.bin missing from {entries:?}"
            );
        }
    }

    #[test]
    fn file_contents_survive_packing() {
        let dir = tempfile::tempdir().expect("tempdir");
        write(dir.path(), "payload.bin", b"exact bytes matter");

        let (mut archive, _hint) =
            build_tar_tempfile_from_directory(&dir.path().to_string_lossy()).expect("pack");
        let mut bytes = Vec::new();
        archive.read_to_end(&mut bytes).expect("read");

        let mut ar = tar::Archive::new(std::io::Cursor::new(bytes));
        let mut found = None;
        for entry in ar.entries().expect("entries") {
            let mut entry = entry.expect("entry");
            if entry.path().expect("path").ends_with("payload.bin") {
                let mut s = Vec::new();
                entry.read_to_end(&mut s).expect("read entry");
                found = Some(s);
            }
        }
        assert_eq!(found.as_deref(), Some(&b"exact bytes matter"[..]));
    }

    #[test]
    fn the_filename_hint_is_the_directory_name_with_a_tar_suffix() {
        // The hint lands in the OpenPGP literal data packet (RFC 9580 §5.9)
        // so the recipient sees a sensible name.
        let parent = tempfile::tempdir().expect("tempdir");
        let dir = parent.path().join("my-documents");
        std::fs::create_dir(&dir).expect("mkdir");
        let (_archive, hint) =
            build_tar_tempfile_from_directory(&dir.to_string_lossy()).expect("pack");
        assert_eq!(hint, "my-documents.tar");
    }

    #[test]
    fn the_filename_hint_handles_a_unicode_directory_name() {
        let parent = tempfile::tempdir().expect("tempdir");
        let dir = parent.path().join("réunion-鍵");
        std::fs::create_dir(&dir).expect("mkdir");
        let (_archive, hint) =
            build_tar_tempfile_from_directory(&dir.to_string_lossy()).expect("pack");
        assert_eq!(hint, "réunion-鍵.tar");
    }

    #[test]
    fn a_trailing_separator_does_not_break_the_hint() {
        let parent = tempfile::tempdir().expect("tempdir");
        let dir = parent.path().join("trailing");
        std::fs::create_dir(&dir).expect("mkdir");
        let with_slash = format!("{}/", dir.to_string_lossy());
        let (_archive, hint) = build_tar_tempfile_from_directory(&with_slash).expect("pack");
        assert_eq!(hint, "trailing.tar");
    }

    #[test]
    fn the_archive_is_rewound_ready_to_read() {
        // The caller streams straight from the handle, so a non-zero offset
        // would silently truncate the ciphertext.
        let dir = tempfile::tempdir().expect("tempdir");
        write(dir.path(), "x.txt", b"content");
        let (mut archive, _hint) =
            build_tar_tempfile_from_directory(&dir.path().to_string_lossy()).expect("pack");
        assert_eq!(archive.stream_position().expect("pos"), 0);
        let mut first = [0u8; 4];
        archive.read_exact(&mut first).expect("read from start");
    }

    #[test]
    fn a_large_directory_packs_without_error() {
        let dir = tempfile::tempdir().expect("tempdir");
        for i in 0..64 {
            write(dir.path(), &format!("dir{}/file{}.bin", i % 8, i), &[0xABu8; 1024]);
        }
        let (mut archive, _hint) =
            build_tar_tempfile_from_directory(&dir.path().to_string_lossy()).expect("pack");
        let mut bytes = Vec::new();
        archive.read_to_end(&mut bytes).expect("read");
        assert!(bytes.len() > 64 * 1024, "got {} bytes", bytes.len());
    }
}
