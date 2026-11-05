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

#include "GFShellExport.h"

namespace GpgFrontend::Shell {

/**
 * @brief The kinds of operation on files, as a file list offers them.
 */
// NOLINTNEXTLINE(performance-enum-size)
enum class FileOperation {
  kEncrypt,  ///< to keys, to keys and signed, or to a passphrase
  kSign,     ///< a detached signature
  kDecrypt,  ///< alone or with its signatures checked
  kVerify,   ///< a signature, detached or inline
};

/**
 * @brief What a file list knows of one chosen entry.
 */
struct GF_SHELL_EXPORT FileOperand {
  QString name;         ///< its file name, which decides what it is
  bool is_file = true;  ///< a regular file
  bool is_dir = false;  ///< a folder, which only encrypting takes
};

/**
 * @brief Whether @p op can be done to every one of @p operands.
 *
 * By their names, as every file list decides it: encrypting what is not
 * OpenPGP data (folders too), signing files that are not, decrypting
 * encrypted messages, verifying OpenPGP data. Nothing chosen allows nothing.
 *
 * @param op the operation
 * @param operands the chosen entries
 * @return true when it can be done to all of them
 */
auto GF_SHELL_EXPORT FileOperationAllowed(FileOperation op,
                                          const QList<FileOperand>& operands)
    -> bool;

/**
 * @brief What a file chosen for verifying is, by its name.
 */
// NOLINTNEXTLINE(performance-enum-size)
enum class VerifyFileRole {
  kCombined,   ///< .gpg or .pgp: the data and its signature in one file
  kSignature,  ///< .sig or .asc: a detached signature
  kData,       ///< anything else: the data a detached signature is of
};

/**
 * @brief A file chosen for verifying, and the other half to look for.
 */
struct GF_SHELL_EXPORT VerifyFileMatch {
  VerifyFileRole role = VerifyFileRole::kData;
  /// File names its other half may have, likeliest first, in the same
  /// folder; none for a combined file.
  QStringList partner_names;
};

/**
 * @brief What @p name is for verifying, and the names of its other half.
 *
 * "a.txt.sig" is a signature of "a.txt"; "a.txt" is the data of "a.txt.sig"
 * or "a.txt.asc"; "a.txt.gpg" holds both. Suffixes in any case.
 *
 * @param name a file name, without its folder
 * @return the match
 */
auto GF_SHELL_EXPORT MatchVerifyFile(const QString& name) -> VerifyFileMatch;

}  // namespace GpgFrontend::Shell
