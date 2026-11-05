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
 * @brief How much an import changed one key, for its colour.
 */
// NOLINTNEXTLINE(performance-enum-size)
enum class KeyImportChange {
  kUnchanged,  ///< nothing new
  kNewKey,     ///< the key was not there before
  kUpdated,    ///< the key was there, and gained something
};

/**
 * @brief What an import did to one key, in words.
 *
 * Reads the status bits (GpgImportInformation::GpgImportedKey::import_status:
 * 1 new key, 2 new user ID, 4 new signature, 8 new subkey, 16 secret key) one
 * by one, so a key that gained a user ID and a signature says both.
 *
 * @param import_status the bits
 * @return e.g. "Private, New UID, New Signature"
 */
auto GF_SHELL_EXPORT DescribeKeyImportStatus(int import_status) -> QString;

/**
 * @brief How much an import changed one key.
 *
 * @param import_status the bits, as for DescribeKeyImportStatus()
 * @return the change
 */
auto GF_SHELL_EXPORT KeyImportChangeOf(int import_status) -> KeyImportChange;

}  // namespace GpgFrontend::Shell
