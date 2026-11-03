/**
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

#pragma once

/// Filesystem and file-handle primitives that have to ask the operating
/// system. Built into gf_core; the policy that decides what to do with the
/// answers stays with the callers.

namespace GpgFrontend::Platform {

/// What a later look at a regular file is compared against, to notice that a
/// path no longer names the same bytes.
struct RegularFileStamp {
  qint64 size = -1;
  qint64 modified_ms = -1;
  quint64 file_id = 0;  ///< inode, or 0 where the platform has none to offer
};

/**
 * @brief Size, modification time and file id of a regular file.
 *
 * @return a stamp whose size is -1 when the path is not a regular file
 */
auto GF_CORE_EXPORT StatRegularFile(const QString &path) -> RegularFileStamp;

/**
 * @brief The nearest ancestor of a path that exists.
 *
 * The interesting candidates have often not been created yet, and the
 * question being asked -- what filesystem is this -- is answerable about the
 * place they would land.
 *
 * @return the path itself if it exists, an ancestor, or empty
 */
auto GF_CORE_EXPORT NearestExistingAncestor(const QString &path) -> QString;

/**
 * @brief Keep a directory out of the desktop search index and system backups.
 *
 * Best effort: the platform's own markers only, so nothing here fails. A no-op
 * where the platform has no such markers.
 */
void GF_CORE_EXPORT ExcludeFromIndexAndBackup(const QString &path);

/**
 * @brief Push what is written to a file descriptor through to the disk.
 *
 * A no-op where the platform offers no such call for a C runtime descriptor.
 */
void GF_CORE_EXPORT FlushToDisk(int fd);

/**
 * @brief How many bytes this process may lock in memory.
 *
 * @return the soft limit, or -1 when there is none or it cannot be read
 */
auto GF_CORE_EXPORT LockedMemoryLimit() -> qint64;

/// dup() and close() on a C runtime file descriptor.
auto GF_CORE_EXPORT DuplicateFd(int fd) -> int;
auto GF_CORE_EXPORT CloseFd(int fd) -> int;

#ifdef Q_OS_UNIX

/// The effective user id of this process.
auto GF_CORE_EXPORT EffectiveUserId() -> uint;

/**
 * @brief Make a base directory private to this user, or refuse one that is
 * not.
 *
 * Created 0700 when it is not there. When it is, it is accepted only if it is
 * a real directory, owned by this user and closed to everyone else.
 */
auto GF_CORE_EXPORT ClaimPrivateDirectory(const QString &path, QString &reason)
    -> bool;

#endif

#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)

/**
 * @brief Whether a path is on a filesystem that lives in memory.
 *
 * Reported rather than assumed: $XDG_RUNTIME_DIR is a tmpfs on every systemd
 * distribution and on none of them is that a guarantee.
 */
auto GF_CORE_EXPORT IsRamBackedPath(const QString &path, QString &reason)
    -> bool;

#endif

#ifdef Q_OS_LINUX

/**
 * @brief Whether the filesystem a path would land on implements fscrypt.
 *
 * A pre-filter and not the answer: the feature is per-superblock, so an ext4
 * here still has to be asked properly. What it settles cheaply is the common
 * case -- a temporary directory on a tmpfs, which cannot carry a policy at all
 * -- without opening or creating anything.
 */
auto GF_CORE_EXPORT IsFscryptCapableOrigin(const QString &path, QString &reason)
    -> bool;

#endif

#ifdef Q_OS_WINDOWS

/**
 * @brief Ask NTFS to encrypt everything created inside a directory from here
 * on.
 *
 * Applied to the directory while it is still empty, which is exactly the
 * provision-then-extract order: a directory-level encrypt marks it so newly
 * created files inherit encryption.
 *
 * Verified by reading the attribute back rather than trusting the return
 * value, and then by creating a file and asking about that. A driver that
 * claims an encryption it did not get is worse than one that admits it has
 * none.
 */
auto GF_CORE_EXPORT EncryptDirectory(const QString &path, QString &reason)
    -> bool;

#endif

}  // namespace GpgFrontend::Platform
