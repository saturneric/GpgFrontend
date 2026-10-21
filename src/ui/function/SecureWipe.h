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

class QTextDocument;

namespace GpgFrontend::UI {

/**
 * @brief Drop @p doc's content and everything that could bring it back.
 *
 * Honest about the limit. A QTextDocument keeps its characters in a private,
 * append-only buffer that no public API can overwrite in place: every mutator
 * appends. Filling the document with a same-length filler before clearing it,
 * as the editor used to do, therefore overwrote nothing — it appended a second
 * copy of the secret and made the internal buffer reallocate, leaving the
 * original behind in freed memory. Heap grooming, clearing first and then
 * allocating a same-sized block hoping to be handed the old one back, is
 * allocator-specific guesswork and is deliberately not attempted either.
 *
 * What is achievable is done here. Undo and redo are disabled before the
 * clear, so the content cannot be restored with Ctrl+Z and the clear itself is
 * not recorded, and the document is released at once rather than at the end of
 * the session. Erasing the released bytes is the allocator's job, not this
 * function's; text that must genuinely be erasable belongs in a GFBuffer,
 * which owns its storage and wipes it.
 *
 * The clipboard is deliberately left alone: if the user copied something out,
 * clearing their clipboard behind their back would be worse than the exposure.
 *
 * @param doc document to wipe, may be null
 */
void GF_UI_EXPORT WipeTextDocument(QTextDocument* doc);

}  // namespace GpgFrontend::UI
