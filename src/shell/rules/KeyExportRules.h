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
 * @brief The ways one key is exported to a file.
 */
// NOLINTNEXTLINE(performance-enum-size)
enum class KeyExportKind {
  kPublic,       ///< the public half, meant to be handed out
  kShortSecret,  ///< the private key, without all but the latest
                 ///< self-signatures
  kFullSecret,   ///< the private key with everything on it
};

/**
 * @brief What the engine is asked for, for one kind of export.
 */
struct GF_SHELL_EXPORT KeyExportOptions {
  bool secret = false;    ///< the private key material too
  bool ascii = true;      ///< armored text rather than binary
  bool shortest = false;  ///< drop all but the latest self-signatures
  QString type;           ///< the kind's name in the suggested file name
};

/**
 * @brief The engine options for @p kind.
 *
 * @param kind the kind of export
 * @return the options
 */
auto GF_SHELL_EXPORT KeyExportOptionsFor(KeyExportKind kind)
    -> KeyExportOptions;

}  // namespace GpgFrontend::Shell
