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

#include "core/model/GFBuffer.h"

namespace GpgFrontend::UI {

/**
 * @brief Import key material, then show what actually landed.
 *
 * The import itself is synchronous, but the key database refresh it triggers is
 * not, so the detail dialog is deferred until the refresh reports back --
 * otherwise it would describe a cache the rest of the UI has not caught up
 * with yet.
 *
 * The counterpart of ExportKey.h, and exported for the same reason: the module
 * SDK reaches this through GFGpgImportKeys().
 *
 * @param parent dialog parent for the detail dialog
 * @param channel key database / engine channel to import into
 * @param in_buffer the key material, armoured or binary
 * @param rev_cert true to import as a revocation certificate instead of a key
 */
void GF_UI_EXPORT ImportKeys(QWidget* parent, int channel,
                             const GFBuffer& in_buffer, bool rev_cert = false);

/**
 * @brief Import key material from a file the user picks.
 *
 * Rejects anything that is not a readable regular file, and anything larger
 * than a megabyte: a keyring that big is a sign the user picked the wrong file,
 * and importing it would block on a parse that cannot succeed.
 *
 * @param parent dialog parent
 * @param channel key database / engine channel to import into
 */
void GF_UI_EXPORT ImportKeyFromFile(QWidget* parent, int channel);

/**
 * @brief Import key material from the clipboard.
 *
 * @param parent dialog parent
 * @param channel key database / engine channel to import into
 */
void GF_UI_EXPORT ImportKeyFromClipboard(QWidget* parent, int channel);

}  // namespace GpgFrontend::UI
