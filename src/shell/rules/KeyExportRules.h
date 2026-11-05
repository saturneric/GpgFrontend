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

/**
 * @brief Build the default filename offered when saving an exported key, on
 * the desktop and the phone alike.
 *
 * Pure so the platform template and the sanitising can be pinned down without
 * touching gpg or a file dialog.
 *
 * @param name key owner's name
 * @param email key owner's email address
 * @param id key id
 * @param type suffix distinguishing what was exported (KeyExportOptions'
 *        type: "pub", "full_secret", "short_secret")
 * @return a filename safe to hand to a save dialog on this platform
 */
auto GF_SHELL_EXPORT ExportKeyFileName(const QString& name,
                                       const QString& email, const QString& id,
                                       const QString& type) -> QString;

}  // namespace GpgFrontend::Shell
