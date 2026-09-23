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

#include "GFSDKContext.h"

/**
 * @file GFSDKEditor.h
 * @brief What the user currently has open.
 *
 * Its own capability, separate from "ui", because it is a different
 * permission in substance: one is "draw something", the other is "read the
 * document in front of the user", which may be plaintext they just decrypted.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief The current editor tab's exact octets.
 *
 * Exactly what the application itself operates on: the bytes as the document
 * holds them, line endings included, with any module view flushed back first.
 * A module that wants to act on "what the user is looking at" has to come
 * through here; reading the widget's text would give a re-encoded
 * approximation, and an approximation does not verify.
 *
 * Safe to call from any thread; the read is marshalled to the GUI thread.
 *
 * @return owned handle, or NULL when no text tab is open
 */
GFBufferRef GFEditorTakeCurrentContent(GFSDKContext* ctx);

/**
 * @brief What the current document is, not what it says.
 *
 * A CBOR map: `id` (the document id commands take), `type`, `title`, `path`
 * and `modified`. No content: for the bytes, GFEditorTakeCurrentContent.
 *
 * @return 0 and an owned buffer in @p out, or -1 when no document is open
 */
int GFEditorCurrentDocument(GFSDKContext* ctx, GFBufferRef* out);

#ifdef __cplusplus
}
#endif
