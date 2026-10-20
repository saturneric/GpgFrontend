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

//! Transactional output for decrypted plaintext.
//!
//! File and archive decryption read SEIPD v1 payloads in rPGP's `Streaming`
//! mode (and SEIPD v2 payloads chunk by chunk), so plaintext is produced
//! *before* the message's integrity has been checked. It is therefore never
//! written under the caller's output name: it goes to a stage next to the
//! destination, and only [`StagedFile::commit`] / [`StagedDir::commit`] move
//! it into place, after decryption has fully succeeded.
//!
//! Dropping a stage without committing deletes it. That one rule covers every
//! failure path: integrity errors, I/O errors (disk full), cancellation and
//! panic unwinding all leave the destination exactly as it was.
//!
//! Residual risk of streaming (documented, accepted): rPGP parses the inner
//! packets of a SEIPD v1 payload before the MDC is checked, and the staged
//! bytes exist on disk (owner-only, in the destination directory) until the
//! check fails and the stage is removed. An attacker who can modify the
//! ciphertext *and* observe those parse errors, timing, or the stage's contents
//! may learn something about the plaintext (RFC 9580 §13.7). A crash between
//! staging and cleanup can leave an owner-only `.gf-*` stage behind; it is
//! never presented as a successful output.

use std::{
    fs, io,
    path::{Path, PathBuf},
};

use tempfile::{NamedTempFile, TempDir};

/// The directory a stage for `dest` lives in: `dest`'s parent, so the commit
/// is a same-filesystem rename.
fn stage_dir_for(dest: &Path) -> &Path {
    match dest.parent() {
        Some(dir) if !dir.as_os_str().is_empty() => dir,
        _ => Path::new("."),
    }
}

/// A file being written that becomes `dest` only on [`commit`](Self::commit).
///
/// The stage is created owner-only (0600 on Unix) next to `dest`.
pub(crate) struct StagedFile {
    tmp: NamedTempFile,
    dest: PathBuf,
}

impl StagedFile {
    pub(crate) fn new(dest: &Path) -> io::Result<Self> {
        let tmp = tempfile::Builder::new()
            .prefix(".gf-decrypt-")
            .suffix(".part")
            .tempfile_in(stage_dir_for(dest))?;
        Ok(StagedFile {
            tmp,
            dest: dest.to_path_buf(),
        })
    }

    pub(crate) fn file_mut(&mut self) -> &mut fs::File {
        self.tmp.as_file_mut()
    }

    #[cfg(test)]
    pub(crate) fn path(&self) -> &Path {
        self.tmp.path()
    }

    /// Flush the stage to disk, then atomically replace `dest` with it.
    ///
    /// An existing `dest` keeps its permission bits; a new one stays
    /// owner-only, since it holds decrypted plaintext. A symlink at `dest` is
    /// replaced by the file, not written through. On error the stage is
    /// deleted and `dest` is untouched.
    pub(crate) fn commit(self) -> io::Result<()> {
        let file = self.tmp.as_file();
        file.sync_all()?;
        if let Ok(meta) = fs::metadata(&self.dest) {
            if meta.is_file() {
                file.set_permissions(meta.permissions())?;
            }
        }
        self.tmp.persist(&self.dest).map_err(|e| e.error)?;
        Ok(())
    }
}

/// A directory tree being extracted that becomes `dest` only on
/// [`commit`](Self::commit).
///
/// The stage is an owner-only (0700 on Unix) work directory next to `dest`,
/// holding scratch files ([`work`](Self::work)) and the tree to commit
/// ([`tree`](Self::tree)). The tree itself is created with default
/// permissions, which a newly created `dest` inherits; the owner-only parent
/// keeps it private while it is unauthenticated.
pub(crate) struct StagedDir {
    tmp: TempDir,
    dest: PathBuf,
}

impl StagedDir {
    const TREE: &'static str = "tree";

    pub(crate) fn new(dest: &Path) -> io::Result<Self> {
        let parent = stage_dir_for(dest);
        fs::create_dir_all(parent)?;
        let tmp = tempfile::Builder::new()
            .prefix(".gf-extract-")
            .tempdir_in(parent)?;
        fs::create_dir(tmp.path().join(Self::TREE))?;
        Ok(StagedDir {
            tmp,
            dest: dest.to_path_buf(),
        })
    }

    /// Scratch space on the destination's filesystem, removed with the stage.
    pub(crate) fn work(&self) -> &Path {
        self.tmp.path()
    }

    /// The directory to extract into.
    pub(crate) fn tree(&self) -> PathBuf {
        self.tmp.path().join(Self::TREE)
    }

    /// Move the staged tree to `dest`, then remove the stage.
    ///
    /// A missing `dest` is replaced by one atomic rename. An existing `dest`
    /// directory is merged into: staged entries replace same-named files and
    /// are merged into same-named directories, the way extracting the archive
    /// in place would. Everything was extracted before the first move, so a
    /// failure here can only come from the filesystem (a rename refused), never
    /// from unauthenticated or partially extracted data.
    pub(crate) fn commit(self) -> io::Result<()> {
        let tree = self.tree();
        match fs::symlink_metadata(&self.dest) {
            Err(e) if e.kind() == io::ErrorKind::NotFound => fs::rename(&tree, &self.dest),
            Err(e) => Err(e),
            Ok(meta) if meta.is_dir() => merge_into(&tree, &self.dest),
            Ok(_) => Err(io::Error::new(
                io::ErrorKind::AlreadyExists,
                format!(
                    "output path {} exists and is not a directory",
                    self.dest.display()
                ),
            )),
        }
    }
}

/// Move every entry of `src` into `dst`, merging directories recursively.
/// Symlinks in `dst` are never followed: a same-named non-directory is replaced.
fn merge_into(src: &Path, dst: &Path) -> io::Result<()> {
    for entry in fs::read_dir(src)? {
        let entry = entry?;
        let target = dst.join(entry.file_name());
        let target_is_dir = fs::symlink_metadata(&target).is_ok_and(|m| m.is_dir());
        if entry.file_type()?.is_dir() && target_is_dir {
            merge_into(&entry.path(), &target)?;
        } else {
            fs::rename(entry.path(), &target)?;
        }
    }
    Ok(())
}

#[cfg(test)]
mod staging_tests {
    use super::*;

    fn entries(dir: &Path) -> Vec<String> {
        let mut names: Vec<String> = fs::read_dir(dir)
            .unwrap()
            .map(|e| e.unwrap().file_name().to_string_lossy().into_owned())
            .collect();
        names.sort();
        names
    }

    #[test]
    fn a_dropped_stage_leaves_nothing_behind() {
        let dir = tempfile::tempdir().unwrap();
        let dest = dir.path().join("out.bin");
        {
            let mut stage = StagedFile::new(&dest).unwrap();
            io::Write::write_all(stage.file_mut(), b"unauthenticated").unwrap();
            assert!(stage.path().exists());
        }
        assert!(entries(dir.path()).is_empty());
    }

    #[test]
    fn a_committed_stage_becomes_the_destination() {
        let dir = tempfile::tempdir().unwrap();
        let dest = dir.path().join("out.bin");
        let mut stage = StagedFile::new(&dest).unwrap();
        io::Write::write_all(stage.file_mut(), b"plaintext").unwrap();
        stage.commit().unwrap();
        assert_eq!(fs::read(&dest).unwrap(), b"plaintext");
        assert_eq!(entries(dir.path()), vec!["out.bin"]);
    }

    #[cfg(unix)]
    #[test]
    fn a_new_output_is_owner_only_and_an_overwritten_one_keeps_its_mode() {
        use std::os::unix::fs::PermissionsExt;
        let dir = tempfile::tempdir().unwrap();
        let dest = dir.path().join("out.bin");

        let stage = StagedFile::new(&dest).unwrap();
        assert_eq!(
            fs::metadata(stage.path()).unwrap().permissions().mode() & 0o777,
            0o600,
            "unauthenticated plaintext must never be world-readable"
        );
        stage.commit().unwrap();
        assert_eq!(
            fs::metadata(&dest).unwrap().permissions().mode() & 0o777,
            0o600
        );

        fs::set_permissions(&dest, fs::Permissions::from_mode(0o640)).unwrap();
        StagedFile::new(&dest).unwrap().commit().unwrap();
        assert_eq!(
            fs::metadata(&dest).unwrap().permissions().mode() & 0o777,
            0o640
        );
    }

    #[test]
    fn a_failed_commit_keeps_the_destination_and_removes_the_stage() {
        // A directory cannot be replaced by a file.
        let dir = tempfile::tempdir().unwrap();
        let dest = dir.path().join("taken");
        fs::create_dir(&dest).unwrap();
        fs::write(dest.join("keep.txt"), b"keep").unwrap();
        assert!(StagedFile::new(&dest).unwrap().commit().is_err());
        assert_eq!(entries(dir.path()), vec!["taken"]);
        assert_eq!(fs::read(dest.join("keep.txt")).unwrap(), b"keep");
    }

    #[test]
    fn a_staged_dir_is_renamed_into_a_missing_destination() {
        let dir = tempfile::tempdir().unwrap();
        let dest = dir.path().join("out");
        let stage = StagedDir::new(&dest).unwrap();
        fs::create_dir(stage.tree().join("sub")).unwrap();
        fs::write(stage.tree().join("sub/a.txt"), b"a").unwrap();
        stage.commit().unwrap();
        assert_eq!(fs::read(dest.join("sub/a.txt")).unwrap(), b"a");
        assert_eq!(entries(dir.path()), vec!["out"]);
    }

    #[test]
    fn a_staged_dir_merges_into_an_existing_destination() {
        let dir = tempfile::tempdir().unwrap();
        let dest = dir.path().join("out");
        fs::create_dir_all(dest.join("sub")).unwrap();
        fs::write(dest.join("sub/old.txt"), b"old").unwrap();
        fs::write(dest.join("same.txt"), b"before").unwrap();

        let stage = StagedDir::new(&dest).unwrap();
        fs::create_dir(stage.tree().join("sub")).unwrap();
        fs::write(stage.tree().join("sub/new.txt"), b"new").unwrap();
        fs::write(stage.tree().join("same.txt"), b"after").unwrap();
        stage.commit().unwrap();

        assert_eq!(fs::read(dest.join("sub/old.txt")).unwrap(), b"old");
        assert_eq!(fs::read(dest.join("sub/new.txt")).unwrap(), b"new");
        assert_eq!(fs::read(dest.join("same.txt")).unwrap(), b"after");
        assert_eq!(entries(dir.path()), vec!["out"], "the stage is removed");
    }

    #[test]
    fn a_dropped_staged_dir_leaves_nothing_behind() {
        let dir = tempfile::tempdir().unwrap();
        let dest = dir.path().join("out");
        {
            let stage = StagedDir::new(&dest).unwrap();
            fs::write(stage.tree().join("partial"), b"x").unwrap();
        }
        assert!(
            entries(dir.path()).is_empty(),
            "no stage and no destination"
        );
    }
}
