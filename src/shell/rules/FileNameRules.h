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
#include "core/model/GpgAbstractKey.h"
#include "core/model/KeyDatabaseInfo.h"
#include "core/typedef/GpgTypedef.h"

// What a file is called before and after an operation, by its name.

namespace GpgFrontend {

/**
 * @brief Whether the entry looks like an OpenPGP message container.
 *
 * By its name alone, as every file list decides it: these are the files that
 * can be decrypted or verified inline. Shared by the desktop's file panel and
 * the phone's Files page, so both call the same file the same thing.
 *
 * @param info entry to inspect
 * @return true for .gpg, .pgp and .asc files
 */
auto GF_SHELL_EXPORT IsOpenPGPMessageFile(const QFileInfo& info) -> bool;

/**
 * @brief Whether the entry is any kind of OpenPGP output.
 *
 * Adds detached signatures to the message containers. Used to keep already
 * processed files out of the encrypt/sign side of the operation menu.
 *
 * @param info entry to inspect
 * @return true for .gpg, .pgp, .asc and .sig files
 */
auto GF_SHELL_EXPORT IsOpenPGPRelatedFile(const QFileInfo& info) -> bool;

/**
 * @brief Whether the entry is a detached OpenPGP signature.
 *
 * @param info entry to inspect
 * @return true for .sig files
 */
auto GF_SHELL_EXPORT IsOpenPGPSignatureFile(const QFileInfo& info) -> bool;

/**
 * @brief Whether the entry is named as an encrypted folder archive, as
 * encrypting a folder names one: .tar.gpg, .tar.pgp or .tar.asc.
 *
 * A name only: it says neither that the data is OpenPGP nor that what it
 * decrypts to is a TAR archive.
 *
 * @param info entry to inspect
 */
auto GF_SHELL_EXPORT IsOpenPGPArchiveFile(const QFileInfo& info) -> bool;

/**
 * @brief Return the output file path with an extension appropriate for the
 * given operation.
 *
 * @param path input file path to derive the output path from
 * @param opera GPG operation type (encrypt, sign, etc.)
 * @param ascii true if ASCII-armored output is requested
 * @return output file path with the correct extension
 */
auto GF_SHELL_EXPORT SetExtensionOfOutputFile(const QString& path,
                                              GpgOperation opera, bool ascii)
    -> QString;

/**
 * @brief Return the output file path with an archive-operation extension.
 *
 * @param path input file path
 * @param opera GPG operation type
 * @param ascii true if ASCII-armored output is requested
 * @return output file path with the correct archive extension
 */
auto GF_SHELL_EXPORT SetExtensionOfOutputFileForArchive(const QString& path,
                                                        GpgOperation opera,
                                                        bool ascii) -> QString;

}  // namespace GpgFrontend
