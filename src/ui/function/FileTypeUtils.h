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

namespace GpgFrontend::UI {

/**
 * @brief Whether the entry looks like an OpenPGP message container.
 *
 * These are the files that can be decrypted or verified inline.
 *
 * @param info entry to inspect
 * @return true for .gpg, .pgp and .asc files
 */
auto GF_UI_EXPORT IsOpenPGPMessageFile(const QFileInfo& info) -> bool;

/**
 * @brief Whether the entry is any kind of OpenPGP output.
 *
 * Adds detached signatures to the message containers. Used to keep already
 * processed files out of the encrypt/sign side of the operation menu.
 *
 * @param info entry to inspect
 * @return true for .gpg, .pgp, .asc and .sig files
 */
auto GF_UI_EXPORT IsOpenPGPRelatedFile(const QFileInfo& info) -> bool;

/**
 * @brief Whether the entry is a detached OpenPGP signature.
 *
 * @param info entry to inspect
 * @return true for .sig files
 */
auto GF_UI_EXPORT IsOpenPGPSignatureFile(const QFileInfo& info) -> bool;

/**
 * @brief Whether the entry is a profile file.
 *
 * A `.gfp` is not a document at all: it is a whole profile, and opening one
 * means running it or copying it in, never showing its bytes in the editor.
 *
 * @param info entry to inspect
 * @return true for .gfp files
 */
auto GF_UI_EXPORT IsProfilePackageFile(const QFileInfo& info) -> bool;

}  // namespace GpgFrontend::UI
