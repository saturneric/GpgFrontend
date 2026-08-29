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

namespace GpgFrontend {

/// Length of the key identifier the kernel derives from a master key. The
/// identifier is a hash of the key, not the key: it is what a later process
/// needs to evict a key, and it is safe to write next to the anchor.
inline constexpr int kFscryptKeyIdentifierSize = 16;

/// Length of the master key handed to the kernel. FSCRYPT_MAX_KEY_SIZE, and the
/// size fscryptctl and systemd-homed both use.
inline constexpr int kFscryptMasterKeySize = 64;

/**
 * @brief Whether this build and this kernel can do fscrypt at all.
 *
 * False on every platform but Linux, on a Linux built against kernel headers
 * older than the v2 policy ioctls, and on a kernel that has them compiled out.
 * Asked first so that a machine which cannot do this never has a directory
 * created on it just to find out.
 *
 * @return true when the fscrypt entry points exist
 */
auto GF_CORE_EXPORT FscryptAvailable() -> bool;

/**
 * @brief Whether a directory could carry a policy of ours.
 *
 * Two questions, both asked of the kernel and neither inferred from a path.
 * *Can* this filesystem encrypt — ext4 and f2fs say so only when the superblock
 * carries the feature, which is off by default on most distributions. And is it
 * *already* encrypted — a home directory under systemd-homed is, and setting a
 * second policy inside one fails. Without the second question the machines most
 * likely to have fscrypt would be the ones that silently fell back on every
 * open.
 *
 * Side-effect free: nothing is created, and no key is added.
 *
 * @param path an existing directory
 * @param reason set to why not, in the words the user will be shown
 * @return true when a policy could be set here
 */
auto GF_CORE_EXPORT FscryptDirectoryIsUsable(const QString &path,
                                             QString &reason) -> bool;

/**
 * @brief Mint a key, put it in the kernel, and encrypt a directory with it.
 *
 * The directory must exist and be empty: a policy can only be set on an empty
 * directory, because everything already inside it would otherwise be plaintext
 * under an encrypted name.
 *
 * The key is generated here, handed to the kernel, and wiped from this
 * process's memory before returning. Nothing keeps it afterwards — the kernel
 * holds it, this process holds only the identifier, and evicting the key is the
 * only thing that identifier can do. That is the whole point: there is no copy
 * to leak and none to destroy at exit beyond a single ioctl.
 *
 * Verified rather than trusted, exactly as the Windows driver verifies EFS: the
 * policy is read back, a file created inside is asked whether it inherited one,
 * and a unix socket is bound to prove gpg-agent will be able to. A driver that
 * claims an encryption it did not get is worse than one that admits it has
 * none.
 *
 * @param dir an existing, empty directory
 * @param identifier set to the kernel's key identifier on success
 * @param reason set to why not, in the words the user will be shown
 * @return true when the directory is encrypted and proven so
 */
auto GF_CORE_EXPORT FscryptProvisionDirectory(const QString &dir,
                                              QByteArray &identifier,
                                              QString &reason) -> bool;

/**
 * @brief Drop this user's claim on a key, which is what makes the tree
 * unreadable.
 *
 * Called after the tree is destroyed rather than before: with the key gone,
 * unlink and rmdir still work but opening a file does not, so evicting first
 * would leave a tree that could no longer be scrubbed.
 *
 * A key outlives the process that added it. That is why the identifier is
 * recorded next to the anchor — a session killed outright leaves its key in the
 * kernel, readable to that user, until something evicts it.
 *
 * Must be given a path *outside* the tree being locked -- the base, not the
 * session root. The ioctl needs an open descriptor on the filesystem, and one
 * taken inside the encrypted tree is itself a file in use under the key being
 * removed, which the kernel reports as FILES_BUSY.
 *
 * @param any_path_on_fs an existing path on the filesystem holding the key, and
 * not one encrypted by it
 * @param identifier the identifier FscryptProvisionDirectory() returned
 * @param reason set to why not
 * @return true when the key is gone -- including when it was already gone, and
 * when this filesystem, kernel or build cannot hold such a key at all, which
 * for a caller deciding whether anything is left to clean up is the same
 * answer. False means a key that may well still be there, so a later attempt
 * is worth making.
 */
auto GF_CORE_EXPORT FscryptRemoveKey(const QString &any_path_on_fs,
                                     const QByteArray &identifier,
                                     QString &reason) -> bool;

/**
 * @brief Spell an identifier for the anchor pointer.
 *
 * @param id a raw identifier
 * @return lowercase hex, or empty when the identifier is not the right length
 */
auto GF_CORE_EXPORT FscryptIdentifierToHex(const QByteArray &id) -> QString;

/**
 * @brief Read an identifier back from an anchor pointer.
 *
 * The pointer is a file on disk that a later process trusts enough to act on,
 * so anything that is not exactly one identifier reads as none rather than as
 * something to try.
 *
 * @param hex the recorded spelling
 * @return the raw identifier, or empty when it is not one
 */
auto GF_CORE_EXPORT FscryptIdentifierFromHex(const QString &hex) -> QByteArray;

}  // namespace GpgFrontend
